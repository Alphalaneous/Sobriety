#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <mutex>

class FileAppender {
public:
    FileAppender(const std::filesystem::path& path) {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        m_ofs.open(path, std::ios::out | std::ios::trunc);
        if (!m_ofs) {
            geode::log::error("Failed to open file: {}", path.string());
        }
    }

    ~FileAppender() {
        std::lock_guard lock(m_mtx);
        if (m_ofs.is_open()) {
            m_ofs.flush();
            m_ofs.close();
        }
    }

    void append(const std::string& data) {
        std::lock_guard lock(m_mtx);
        if (m_ofs.is_open()) {
            m_ofs << data;
            m_ofs.flush();
        }
    }

private:
    std::ofstream m_ofs;
    std::mutex m_mtx;
};