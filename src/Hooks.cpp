#include "Geode/loader/Log.hpp"
#include "SoberManager.hpp"
#include "Utils.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/GameToolbox.hpp>
#include <Shellapi.h>
#include <WinUser.h>
#include <commdlg.h>
#include <minwindef.h>
#include <shobjidl.h>

using namespace geode::prelude;

arc::Future<geode::utils::file::PickResult> file_pick_h(utils::file::PickMode mode, utils::file::FilePickOptions options) {
    if (SoberManager::get()->isPickerActive()) co_return Err("File picker is already open");

    SoberManager::get()->setPickerActive(true);

    auto defaultPath = SoberManager::get()->getLinuxPath(options.defaultPath.value_or(dirs::getGameDir()).string());

    SoberManager::get()->pickFile(defaultPath, mode, false, options.filters, true);

    co_await SoberManager::get()->notified();

    auto path = SoberManager::get()->getPath();
    if (!path) {
        co_return Ok(std::nullopt);
    }

    co_return Ok(std::move(path.value()));
}

arc::Future<geode::utils::file::PickManyResult> file_pickMany_h(utils::file::FilePickOptions options) {
    if (SoberManager::get()->isPickerActive()) co_return Err("File picker is already open");

    SoberManager::get()->setPickerActive(true);

    auto defaultPath = SoberManager::get()->getLinuxPath(options.defaultPath.value_or(dirs::getGameDir()).string());

    SoberManager::get()->pickFile(defaultPath, file::PickMode::OpenFile, true, options.filters, true);

    co_await SoberManager::get()->notified();

    auto paths = SoberManager::get()->getPaths();
    if (!paths) {
        co_return Ok(std::vector<asp::fs::Path>{});
    }

    co_return Ok(std::move(paths.value()));
}

bool file_openFolder_h(asp::fs::PathView path) {
    if (path.isDir()) {
        auto defaultPath = SoberManager::get()->getLinuxPath(path.string());
        SoberManager::get()->browseFiles(defaultPath);
        return true;
    }
    return false;
}

template<class T>
void safeLog(ZStringView label, T value) {
    if constexpr (std::is_pointer_v<T>) {
        if (!value) {
            log::info("{}: nullptr", label);
            return;
        }
        log::info("{}: {}", label, value);
    }
    else {
        log::info("{}: {}", label, value);
    }
}


std::vector<utils::file::FilePickOptions::Filter> parseFilters(LPOPENFILENAMEA ofn) {
    std::vector<utils::file::FilePickOptions::Filter> filters;

    if (!ofn || !ofn->lpstrFilter) return filters;

    auto ptr = ofn->lpstrFilter;

    while (*ptr) {
        std::string description = ptr;
        ptr += description.size() + 1;

        if (!*ptr) break;

        std::string pattern = ptr;
        ptr += pattern.size() + 1;

        std::unordered_set<std::string> files;
        auto split = utils::string::split(pattern, ";");
        for (auto& str : split) {
            files.insert(str);
        }
        
        filters.emplace_back(
            utils::file::FilePickOptions::Filter{
                std::move(description),
                std::move(files)
            }
        );
    }

    return filters;
}

bool setSelectedFiles(LPOPENFILENAMEA ofn, const std::vector<asp::fs::Path>& files) {
    if (!ofn || !ofn->lpstrFile || files.empty()) return false;

    auto buffer = ofn->lpstrFile;
    auto capacity = ofn->nMaxFile;

    if (files.size() == 1) {
        auto& path = files.front();

        if (path.size() + 1 > capacity) return false;

        std::memcpy(buffer, path.c_str(), path.size() + 1);
    }
    else {
        if (!(ofn->Flags & OFN_EXPLORER) || !(ofn->Flags & OFN_ALLOWMULTISELECT)) return false;

        auto directory = files.front().parent();
        if (!directory) return false;

        std::string result = directory.value().string();
        result += '\0';

        for (auto& file : files) {
            auto filename = file.filename();
            if (!filename) continue;

            result += filename.value().string();
            result += '\0';
        }

        result += '\0';

        if (result.size() > capacity) return false;

        std::memcpy(buffer, result.data(), result.size());
    }

    return true;
}

void updateFileOffsets(LPOPENFILENAMEA ofn, bool multiple) {
    if (multiple) {
        ofn->nFileExtension = 0;
        return;
    }

    auto path = std::string_view(ofn->lpstrFile);
    auto filename = path.find_last_of("\\/");
    filename = filename == std::string_view::npos ? 0 : filename + 1;

    auto extension = path.find_last_of('.');

    ofn->nFileOffset = static_cast<WORD>(filename);
    ofn->nFileExtension = extension == std::string_view::npos ? 0 : static_cast<WORD>(extension + 1);
}

BOOL GetOpenFileNameA_h(LPOPENFILENAMEA param1) {
    auto defaultPath = SoberManager::get()->getLinuxPath(dirs::getGameDir().string());

    SoberManager::get()->setPickerActive(true);
    SoberManager::get()->pickFile(defaultPath, file::PickMode::OpenFile, param1->Flags & OFN_ALLOWMULTISELECT, parseFilters(param1), false, param1->lpstrTitle);

    while (SoberManager::get()->isPickerActive());

    auto paths = SoberManager::get()->getPaths();
    if (!paths) return false;

    setSelectedFiles(param1, *paths);
    updateFileOffsets(param1, paths->size() > 1);

    return true;
}

BOOL GetSaveFileNameA_h(LPOPENFILENAMEA param1) {
    auto defaultPath = SoberManager::get()->getLinuxPath((dirs::getGameDir() / param1->lpstrFile).string());

    SoberManager::get()->setPickerActive(true);
    SoberManager::get()->pickFile(defaultPath, file::PickMode::SaveFile, false, parseFilters(param1), false, param1->lpstrTitle);

    while (SoberManager::get()->isPickerActive());

    auto paths = SoberManager::get()->getPaths();
    if (!paths) return false;

    setSelectedFiles(param1, *paths);
    updateFileOffsets(param1, paths->size() > 1);

    return true;
}

HINSTANCE ShellExecuteA_h(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd) {
    auto isFile = GetFileAttributesA(lpFile) != INVALID_FILE_ATTRIBUTES; 
    if (!hwnd && std::string_view(lpOperation) == "open" && isFile && !lpParameters && !lpDirectory && nShowCmd == SW_SHOWDEFAULT) {
        SoberManager::get()->browseFiles(SoberManager::get()->getLinuxPath(lpFile));
        return 0; // GD doesn't care so I don't care
    }

    return ShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

$on_mod(Loaded) {
    if (sobriety::utils::isWine()) {
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&utils::file::pick)),
            &file_pick_h,
            "utils::file::pick"
        );
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&utils::file::pickMany)),
            &file_pickMany_h,
            "utils::file::pickMany"
        );
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&utils::file::openFolder)),
            &file_openFolder_h,
            "utils::file::openFolder"
        );
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&GetOpenFileNameA)),
            &GetOpenFileNameA_h,
            "GetOpenFileNameA"
        );
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&GetSaveFileNameA)),
            &GetSaveFileNameA_h,
            "GetSaveFileNameA"
        );
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addresser::getNonVirtual(&ShellExecuteA)),
            &ShellExecuteA_h,
            "ShellExecuteA"
        );
    }
}

class $modify(GameToolbox) {

    static bool showFolderDialog(char const* title, std::string& out) {
        out.clear();

        auto defaultPath = SoberManager::get()->getLinuxPath(dirs::getGameDir().string());

        SoberManager::get()->setPickerActive(true);
        SoberManager::get()->pickFile(defaultPath, file::PickMode::OpenFolder, false, {}, false, title);

        while (SoberManager::get()->isPickerActive());

        auto path = SoberManager::get()->getPath();
        if (!path) return false;

        out = path.value().string();

        return true;
    }
};