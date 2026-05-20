#pragma once

#include "Utils.hpp"
#include <vector>
#include <Geode/utils/file.hpp>
#include "FileAppender.hpp"

class WaitingPopup;

class SoberManager : public sobriety::utils::Singleton<SoberManager> {
public:
    void startScript();
    std::string getLinuxPath(std::string_view path);
    bool isWine();

    void pickFile(const std::string& startPath, geode::utils::file::PickMode mode, bool isMultiple, const std::vector<geode::utils::file::FilePickOptions::Filter>& filters);
    void onFilePicked(const std::vector<std::string>& items);
    void onFileCancelled();
    std::string generateExtensionString(std::vector<geode::utils::file::FilePickOptions::Filter> filters);

    bool isPickerActive();
    void setPickerActive(bool active);

    void browseFiles(const std::string& startPath);

    arc::Notified notified();
    std::optional<std::filesystem::path> getPath();
    std::optional<std::vector<std::filesystem::path>> getPaths();

    void setConsoleColors();
    void startLogger();

protected:
    void createTempDir();
    void runCommand(const std::string& cmd);

    unsigned int m_id = 0;
    bool m_pickerActive;
    bool m_isMultipleFiles;
    geode::utils::file::PickMode m_pickerMode;

    arc::Notify m_notify;
    std::optional<std::filesystem::path> m_path;
    std::optional<std::vector<std::filesystem::path>> m_paths;
    WaitingPopup* m_waitingPopup;

    std::shared_ptr<FileAppender> m_logAppender;
};