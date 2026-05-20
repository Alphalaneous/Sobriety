#include "FilePicker.hpp"
#include <poll.h>
#include <vector>
#include <fmt/format.h>
#include "DrunkManager.hpp"

FilePicker::FilePicker(std::string_view id, std::string_view title, bool isMultiple, bool isDirectory, std::string_view directory, std::string_view name, std::string_view filters, FilePicker::Type type) {
    m_id = id;
    m_title = title;
    m_isMultiple = isMultiple;
    m_isDirectory = isDirectory;
    m_directory = directory;
    m_filters = filters;
    m_type = type;
    m_name = name;
}

void FilePicker::openFilePicker() {
    m_thread = std::jthread([this] (std::stop_token st) {
        DBusError err;

        dbus_error_init(&err);

        m_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            return;
        }

        if (!m_conn) return;
        
        setupPicker();
        listenForFileResponse(st);
        m_thread.detach();
        DrunkManager::get()->removeFilePicker(m_id);
    });
}

void FilePicker::setupPicker() {
    DBusMessage* msg;
    DBusMessageIter args, dict, array, struct_iter;
    DBusPendingCall* pending;
    DBusError err;

    dbus_error_init(&err);

    msg = dbus_message_new_method_call(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.FileChooser",
        m_type == Type::PICK ? "OpenFile" : "SaveFile"
    );

    if (!msg) return;

    dbus_message_iter_init_append(msg, &args);

    auto parent = "";
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parent);

    auto title = m_title.c_str();
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &title);

    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);

    appendVariant(&dict, "handle_token", DBUS_TYPE_STRING_AS_STRING, m_id);

    if (!m_directory.empty()) {
        DBusMessageIter entry, variant, array;

        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);

        const char* key = "current_folder";
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "ay", &variant);

        dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &array);

        const char* path = m_directory.c_str();
        int len = strlen(path) + 1;

        dbus_message_iter_append_fixed_array(
            &array,
            DBUS_TYPE_BYTE,
            &path,
            len
        );

        dbus_message_iter_close_container(&variant, &array);
        dbus_message_iter_close_container(&entry, &variant);
        dbus_message_iter_close_container(&dict, &entry);
    }
    
    appendVariant(&dict, "multiple", DBUS_TYPE_BOOLEAN_AS_STRING, m_isMultiple);
    appendVariant(&dict, "directory", DBUS_TYPE_BOOLEAN_AS_STRING, m_isDirectory);

    if (m_type == Type::SAVE) {
        appendVariant(&dict, "current_name", DBUS_TYPE_STRING_AS_STRING, m_name);
    }

    if (!m_filters.empty()) {
        DBusMessageIter entry, variant;

        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);

        auto key = "filters";
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "a(sa(us))", &variant);

        DBusMessageIter array_iter;
        dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "(sa(us))", &array_iter);

        size_t start = 0;
        while (start < m_filters.size()) {
            size_t colon = m_filters.find(':', start);
            size_t semi = m_filters.find(';', start);

            if (colon == std::string::npos) break;

            std::string name = m_filters.substr(start, colon - start);

            size_t end = (semi == std::string::npos) ? m_filters.size() : semi;
            std::string exts = m_filters.substr(colon + 1, end - colon - 1);

            DBusMessageIter struct_it, mimetypes;
            dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, nullptr, &struct_it);

            const char* filter_name = name.c_str();
            dbus_message_iter_append_basic(&struct_it, DBUS_TYPE_STRING, &filter_name);

            dbus_message_iter_open_container(&struct_it, DBUS_TYPE_ARRAY, "(us)", &mimetypes);

            size_t ext_start = 0;
            while (ext_start < exts.size()) {
                size_t comma = exts.find(',', ext_start);
                size_t ext_end = (comma == std::string::npos) ? exts.size() : comma;

                std::string ext = exts.substr(ext_start, ext_end - ext_start);
                ext.erase(0, ext.find_first_not_of(" \t"));
                ext.erase(ext.find_last_not_of(" \t") + 1);

                if (ext.empty()) continue;

                std::string pattern;

                // I couldn't find the docs for what format geode expects so I just let any format cuz who cares
                if (ext == "*" || ext == "*.*") {
                    pattern = "*";
                }
                else if (ext.find('*') != std::string::npos) {
                    pattern = ext;
                }
                else if (ext.find('/') != std::string::npos) {
                    pattern = ext;
                }
                else if (ext[0] == '.') {
                    pattern = "*" + ext;
                }
                else {
                    pattern = "*." + ext;
                }

                DBusMessageIter mime_entry;
                dbus_message_iter_open_container(&mimetypes, DBUS_TYPE_STRUCT, nullptr, &mime_entry);

                dbus_uint32_t active = 0;
                dbus_message_iter_append_basic(&mime_entry, DBUS_TYPE_UINT32, &active);

                const char* pattern_cstr = pattern.c_str();
                dbus_message_iter_append_basic(&mime_entry, DBUS_TYPE_STRING, &pattern_cstr);

                dbus_message_iter_close_container(&mimetypes, &mime_entry);

                if (comma == std::string::npos) break;
                ext_start = comma + 1;
            }

            dbus_message_iter_close_container(&struct_it, &mimetypes);
            dbus_message_iter_close_container(&array_iter, &struct_it);

            if (semi == std::string::npos) break;
            start = semi + 1;
        }

        dbus_message_iter_close_container(&variant, &array_iter);
        dbus_message_iter_close_container(&entry, &variant);
        dbus_message_iter_close_container(&dict, &entry);
    }

    dbus_message_iter_close_container(&args, &dict);

    if (!dbus_connection_send_with_reply(m_conn, msg, &pending, -1) || !pending) return;

    dbus_connection_flush(m_conn);
    dbus_message_unref(msg);
    dbus_pending_call_unref(pending);
}

void FilePicker::listenForFileResponse(std::stop_token st) {
    int fd = -1;
    if (!dbus_connection_get_unix_fd(m_conn, &fd) || fd < 0) return;

    while (!st.stop_requested()) {
        struct pollfd pfd {
            .fd = fd,
            .events = POLLIN,
            .revents = 0
        };

        int ret = poll(&pfd, 1, 100); 

        if (ret < 0) break;
        if (ret == 0) continue;
        
        dbus_connection_read_write_dispatch(m_conn, 0);

        bool done = false;

        while (DBusMessage* msg = dbus_connection_pop_message(m_conn)) {
            auto path = dbus_message_get_path(msg);

            if (!done && dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response") && path && std::string_view(path).find(m_id) != std::string::npos) {
                handleFileResponse(msg);
                dbus_message_unref(msg);
                return;
            }

            dbus_message_unref(msg);
        }
    }
}

std::string FilePicker::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            std::string hex = str.substr(i + 1, 2);
            char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded;
            i += 2;
        } else {
            result += str[i];
        }
    }

    return result;
}

std::optional<std::string> FilePicker::fileUriToPath(const std::string& uri) {
    constexpr std::string_view prefix = "file://";

    if (!uri.starts_with(prefix)) {
        return std::nullopt;
    }

    std::string path = std::string(uri.substr(prefix.size()));

    return urlDecode(path);
}

void FilePicker::handleFileResponse(DBusMessage* msg) {
    DBusMessageIter args;

    if (!dbus_message_iter_init(msg, &args)) return;

    unsigned int response = 1;
    dbus_message_iter_get_basic(&args, &response);

    if (response != 0) {
        DrunkManager::get()->onFileCancelled(m_id);
        return;
    }

    dbus_message_iter_next(&args);

    DBusMessageIter results;
    dbus_message_iter_recurse(&args, &results);

    std::vector<std::string> paths;

    while (dbus_message_iter_get_arg_type(&results) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&results, &entry);

        const char* key;
        dbus_message_iter_get_basic(&entry, &key);

        if (std::string_view(key) == "uris") {
            dbus_message_iter_next(&entry);

            DBusMessageIter variant;
            dbus_message_iter_recurse(&entry, &variant);

            DBusMessageIter array;
            dbus_message_iter_recurse(&variant, &array);

            while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
                const char* uri;
                dbus_message_iter_get_basic(&array, &uri);

                auto pathRes = fileUriToPath(uri);
                if (pathRes) {
                    paths.push_back(pathRes.value());
                }

                dbus_message_iter_next(&array);
            }
        }

        dbus_message_iter_next(&results);
    }

    DrunkManager::get()->onFilePicked(m_id, paths);
}