#pragma once

#include <dbus/dbus.h>
#include <string_view>
#include <optional>
#include <thread>

class FilePicker {
public:
    enum class Type {
        PICK,
        SAVE
    };

    FilePicker(std::string_view id, std::string_view title, bool isMultiple, bool isDirectory, std::string_view directory, std::string_view name, std::string_view filters, FilePicker::Type type);

    ~FilePicker() = default;

    void openFilePicker();
protected:
    void setupPicker();

    void listenForFileResponse(std::stop_token st);
    void handleFileResponse(DBusMessage* msg);
    std::string urlDecode(const std::string& str);
    std::optional<std::string> fileUriToPath(const std::string& uri);

    template <class T>
    void appendVariant(DBusMessageIter* dict, std::string_view key, const std::string& type, T value) {
        DBusMessageIter entry, variant;

        dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);

        const char* key_cstr = key.data();
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key_cstr);

        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, type.c_str(), &variant);

        using U = std::decay_t<T>;

        if constexpr (std::is_same_v<U, std::string>) {
            const char* str = value.c_str();
            dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &str);

        } else if constexpr (std::is_same_v<U, const char*>) {
            dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);

        } else if constexpr (std::is_same_v<U, bool>) {
            dbus_bool_t b = value;
            dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &b);

        } else if constexpr (std::is_integral_v<U>) {
            dbus_message_iter_append_basic(&variant, type[0], &value);

        } else {
            static_assert(!sizeof(T*), "Unsupported DBus type");
        }

        dbus_message_iter_close_container(&entry, &variant);
        dbus_message_iter_close_container(dict, &entry);
    }

    std::string m_id;
    std::string m_title;
    std::string m_directory;
    std::string m_filters;
    std::string m_name;
    bool m_isMultiple;
    bool m_isDirectory;
    Type m_type;

    std::jthread m_thread;

    DBusConnection* m_conn;
};