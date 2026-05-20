#pragma once

#include <dbus/dbus.h>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "Utils.hpp"
#include "FilePicker.hpp"

class DrunkManager : public bridge::utils::Singleton<DrunkManager> {
public:
    void openFilePicker(const std::string& id, std::string_view title, bool multiple, bool isDirectory, std::string_view directory, std::string_view name, std::string_view filters, FilePicker::Type type);
    void removeFilePicker(const std::string& id);

    void browseFiles(const std::string& directory);

    void onFilePicked(const std::string& id, const std::vector<std::string>& paths);
    void onFileCancelled(const std::string& id);

    void openConsole(const std::string& directory, int fontSize);

protected:
    std::mutex m_filePickersMutex;
    std::unordered_map<std::string, std::shared_ptr<FilePicker>> m_filePickers;

    bool m_consoleOpen = false;
};