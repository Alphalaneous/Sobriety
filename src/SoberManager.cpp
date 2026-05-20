#include "SoberManager.hpp"
#include <Geode/Geode.hpp>
#include "Utils.hpp"
#include "WaitingPopup.hpp"
#include "Server.hpp"

using namespace geode::prelude;

std::string SoberManager::getLinuxPath(std::string_view path) {
    using wine_get_unix_file_name_t = char* (CDECL*)(LPCWSTR);
    static const auto fn = reinterpret_cast<wine_get_unix_file_name_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "wine_get_unix_file_name"));
    if (!fn) return "";

    return fn(utils::string::utf8ToWide(path).c_str());
}

void SoberManager::runCommand(const std::string& cmd) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessA(
            nullptr,
            const_cast<char*>(cmd.c_str()),
            nullptr, nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi
        )) {
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void SoberManager::createTempDir() {
    auto path = fmt::format("/tmp/{}", Mod::get()->getID());

    if (!std::filesystem::exists(path)) {
        auto tmpDirRes = utils::file::createDirectoryAll(path);
        if (!tmpDirRes) return log::error("Failed to create {} directory", path);
    }
}

void SoberManager::startScript() {
    createTempDir();
    auto modID = Mod::get()->getID();

    // .exe extension required for wine to launch it without it itself being marked executable
    auto bridgeStartDir = fmt::format("/tmp/{}/bridge-start.exe", modID);

    std::string bridgeStartScript = 
R"script(#!/bin/bash

export PATH=$PATH:/usr/bin

BINARY="$1"
shift

SAVE="$1"

chmod +x "$BINARY"

{
"$BINARY"
echo "exit_code=$?"
} > "latest_bridge_log.txt" 2>&1

)script";

    auto res = utils::file::writeString(bridgeStartDir, bridgeStartScript);
    if (!res) log::error("failed to write starter script");

    auto bridgeDir = getLinuxPath(fmt::format("geode\\unzipped\\{}\\resources\\{}\\bridge", modID, modID));
    auto saveDir = getLinuxPath(utils::string::pathToString(Mod::get()->getSaveDir()));

    runCommand(fmt::format("\"{}\" \"{}\" \"{}\"", bridgeStartDir, bridgeDir, saveDir));

    if (sobriety::utils::hasConsole()) {
        FreeConsole();
        startLogger();
    }
}

std::string SoberManager::generateExtensionString(std::vector<utils::file::FilePickOptions::Filter> filters) {
    utils::StringBuffer<> filterString;

    bool firstFilter = true;

    for (const auto& filter : filters) {
        if (!firstFilter) {
            filterString.append(";");
        }
        firstFilter = false;

        filterString.append("{}:", filter.description);

        bool firstExtension = true;
        for (const auto& extension : filter.files) {
            if (!firstExtension) {
                filterString.append(",");
            }
            firstExtension = false;
            filterString.append(extension);
        }
    }

    if (!firstFilter) {
        filterString.append(";");
    }

    filterString.append("All Files:*");

    return filterString.str();
}

void SoberManager::pickFile(const std::string& startPath, file::PickMode mode, bool isMultiple, const std::vector<utils::file::FilePickOptions::Filter>& filters) {
    auto id = fmt::format("alphalaneous_sobriety_{}", m_id);
    m_id++;
    m_pickerMode = mode;
    m_isMultipleFiles = isMultiple;
    
    matjson::Value json;
    json["type"] = "file_picker";
    json["id"] = id;

    if (mode == file::PickMode::OpenFolder) {
        json["title"] = "Select a folder";
        json["isDirectory"] = true;
        json["action"] = "pick";
    }
    else if (mode == file::PickMode::OpenFile){
        if (isMultiple) {
            json["title"] = "Select files";
        }
        else {
            json["title"] = "Select a file";
        }
        json["action"] = "pick";
    }
    else if (mode == file::PickMode::SaveFile) {
        json["title"] = "Save...";
        json["action"] = "save";
        json["name"] = utils::string::pathToString(std::filesystem::path(startPath).filename());
    }

    json["isMultiple"] = isMultiple;
    json["directory"] = startPath;
    json["filters"] = generateExtensionString(filters);

    Server::get()->sendMessage(json.dump());
}

void SoberManager::browseFiles(const std::string& startPath) {
    matjson::Value json;
    json["type"] = "file_picker";
    json["directory"] = startPath;
    json["action"] = "browse";

    Server::get()->sendMessage(json.dump());
}

void SoberManager::onFilePicked(const std::vector<std::string>& items) {
    m_path = std::filesystem::path(items[0]);
    std::vector<std::filesystem::path> paths;
    for (const auto& path : items) {
        paths.push_back(path);
    }
    m_paths = paths;

    setPickerActive(false);
}

void SoberManager::onFileCancelled() {
    m_path = std::nullopt;
    m_paths = std::nullopt;

    setPickerActive(false);
}

void SoberManager::setPickerActive(bool active) {
    m_pickerActive = active;

    if (active) {
        queueInMainThread([this] {

            std::string label;
            if (m_pickerMode == file::PickMode::OpenFolder) {
                label = "Select a folder";
            }
            else if (m_pickerMode == file::PickMode::OpenFile){
                if (m_isMultipleFiles) {
                    label = "Select files";
                }
                else {
                    label = "Select a file";
                }
            }
            else if (m_pickerMode == file::PickMode::SaveFile) {
                label = "Save...";
            }

            m_waitingPopup = WaitingPopup::create(label);
            m_waitingPopup->show();
        });
    }
    else {
        if (m_waitingPopup) m_waitingPopup->removeFromParent();
        m_notify.notifyAll();
    }
}

bool SoberManager::isPickerActive() {
    return m_pickerActive;
}

arc::Notified SoberManager::notified() {
    return m_notify.notified();
}

std::optional<std::filesystem::path> SoberManager::getPath() {
    return m_path;
}

std::optional<std::vector<std::filesystem::path>> SoberManager::getPaths() {
    return m_paths;
}

void SoberManager::setConsoleColors() {
    if (m_logAppender) {
        m_logAppender->append(fmt::format("\033]10;#{}\007", cc3bToHexString(sobriety::utils::getConsoleForegroundColor([this] {setConsoleColors();}))));
        m_logAppender->append(fmt::format("\033]11;#{}\007", cc3bToHexString(sobriety::utils::getConsoleBackgroundColor([this] {setConsoleColors();}))));

        m_logAppender->append(fmt::format("\033]4;33;#{}\007", cc3bToHexString(sobriety::utils::getLogInfoColor([this] {setConsoleColors();}))));
        m_logAppender->append(fmt::format("\033]4;229;#{}\007", cc3bToHexString(sobriety::utils::getLogWarnColor([this] {setConsoleColors();}))));
        m_logAppender->append(fmt::format("\033]4;9;#{}\007", cc3bToHexString(sobriety::utils::getLogErrorColor([this] {setConsoleColors();}))));
        m_logAppender->append(fmt::format("\033]4;243;#{}\007", cc3bToHexString(sobriety::utils::getLogDebugColor([this] {setConsoleColors();}))));
    
        m_logAppender->append("\033[A\033[B"); // forces a refresh
    }
}

void SoberManager::startLogger() {
    auto logPath = fmt::format("/tmp/{}/console.ansi", Mod::get()->getID());
    auto res = geode::utils::file::writeString(logPath, "");
    m_logAppender = std::make_shared<FileAppender>(logPath);

    log::LogEvent().listen([this] (log::BorrowedLog const& log) {
        if (log.m_mod) {
            if (!log.m_mod->isLoggingEnabled()) return;
            if (log.m_severity < log.m_mod->getLogLevel()) return;
        }
        if (log.m_severity < sobriety::utils::getConsoleLogLevel()) return;

        StringBuffer<> buffer;
        log.formatTo(buffer, sobriety::utils::shouldLogMillisconds());

        int color = 0;
        switch (log.m_severity) {
            case Severity::Debug:
                color = 243;
                break;
            case Severity::Info:
                color = 33;
                break;
            case Severity::Warning:
                color = 229;
                break;
            case Severity::Error:
                color = 9;
                break;
            default:
                color = 7;
                break;
        }

        size_t colorEnd = buffer.view().find_first_of('[') - 1;

        auto str = fmt::format("\033[38;5;{}m{}\033[0m{}\n", color, buffer.view().substr(0, colorEnd), buffer.view().substr(colorEnd));

        if (m_logAppender) m_logAppender->append(str);
    }).leak();
}