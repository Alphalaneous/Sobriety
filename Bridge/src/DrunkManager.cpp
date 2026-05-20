#include "DrunkManager.hpp"
#include <matjson.hpp>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <fmt/format.h>
#include <sys/wait.h>
#include "Client.hpp"

void DrunkManager::openFilePicker(const std::string& id, std::string_view title, bool multiple, bool isDirectory, std::string_view directory, std::string_view name, std::string_view filters, FilePicker::Type type) {
    std::shared_ptr<FilePicker> picker;

    {
        std::lock_guard<std::mutex> lock(m_filePickersMutex);
        if (m_filePickers.contains(id)) return;

        picker = std::make_shared<FilePicker>(id, title, multiple, isDirectory, directory, name, filters, type);
        m_filePickers[id] = picker;
    }

    picker->openFilePicker();
}

void DrunkManager::removeFilePicker(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_filePickersMutex);

    m_filePickers.erase(id);
}

// opens faster than system("xdg-open...") by like 1 second
void DrunkManager::browseFiles(const std::string& directory) {
    pid_t pid = fork();

    if (pid == 0) {
        std::vector<char*> args;
        args.push_back(const_cast<char*>("xdg-open"));
        args.push_back(const_cast<char*>(directory.c_str()));
        args.push_back(nullptr);

        execvp("xdg-open", args.data());

        _exit(1);
    }
}

void DrunkManager::onFilePicked(const std::string& id, const std::vector<std::string>& paths) {
    matjson::Value json;
    json["type"] = "file_picker";
    json["status"] = "picked";
    json["id"] = id;
    
    auto vec = std::vector<matjson::Value>();

    for (const auto& path : paths) {
        vec.push_back(path);
    }

    json["items"] = vec;

    Client::get()->sendMessage(json.dump());
}

void DrunkManager::onFileCancelled(const std::string& id) {
    matjson::Value json;
    json["type"] = "file_picker";
    json["status"] = "cancelled";
    json["id"] = id;

    Client::get()->sendMessage(json.dump());
}

void DrunkManager::openConsole(const std::string& directory, int fontSize) {
    if (m_consoleOpen) return;

    if (system("which xterm > /dev/null 2>&1") != 0) {
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {

        prctl(PR_SET_PDEATHSIG, SIGTERM);

        if (getppid() == 1)
            _exit(1);

        std::string fontSizeStr = std::to_string(fontSize);

        execlp(
            "xterm",
            "xterm",
            "-fa", "Monospace",
            "-T", "Geometry Dash",
            "-fs", fontSizeStr.c_str(),
            "-xrm", "XTerm*VT100.Translations: #override Ctrl Shift <Key>C: copy-selection(CLIPBOARD)",
            "-e", "tail", "-F", directory.c_str(),
            (char*)nullptr
        );

        perror("execlp");
        _exit(1);
    }

    m_consoleOpen = true;

    std::thread([this, pid]() {
        int status = 0;

        if (waitpid(pid, &status, 0) == pid) {
            matjson::Value json;
            json["type"] = "close";
            Client::get()->sendMessage(json.dump());
        }
    }).detach();
}