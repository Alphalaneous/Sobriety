#pragma once

#include <asp/fs/Path.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <mutex>
#include <asp/fs/std.hpp>

class FileAppender {
public:
    FileAppender(asp::fs::PathView path) {
        if (path.hasParent()) {
            auto stdParent = path.parent()->toStdPath();
            if (stdParent) {
                std::filesystem::create_directories(stdParent.unwrap());
            }
        }

        auto stdPathRes = path.toStdPath();
        if (!stdPathRes) {
            geode::log::error("Failed to convert to std path: {}", path.string());
            return;
        }

        m_ofs.open(stdPathRes.unwrap(), std::ios::out | std::ios::trunc);
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