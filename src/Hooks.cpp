#include "SoberManager.hpp"
#include "Utils.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

arc::Future<Result<std::optional<std::filesystem::path>>> file_pick_h(utils::file::PickMode mode, utils::file::FilePickOptions options) {
    if (SoberManager::get()->isPickerActive()) co_return Err("File picker is already open");

    SoberManager::get()->setPickerActive(true);

    auto defaultPath = SoberManager::get()->getLinuxPath(utils::string::pathToString(options.defaultPath.value_or(dirs::getGameDir())));

    SoberManager::get()->pickFile(defaultPath, mode, false, options.filters);

    co_await SoberManager::get()->notified();

    auto path = SoberManager::get()->getPath();
    if (!path) {
        co_return Ok(std::nullopt);
    }

    co_return Ok(std::move(path.value()));
}

arc::Future<Result<std::vector<std::filesystem::path>>> file_pickMany_h(utils::file::FilePickOptions options) {
    if (SoberManager::get()->isPickerActive()) co_return Err("File picker is already open");

    SoberManager::get()->setPickerActive(true);

    auto defaultPath = SoberManager::get()->getLinuxPath(utils::string::pathToString(options.defaultPath.value_or(dirs::getGameDir())));

    SoberManager::get()->pickFile(defaultPath, file::PickMode::OpenFile, true, options.filters);

    co_await SoberManager::get()->notified();

    auto paths = SoberManager::get()->getPaths();
    if (!paths) {
        co_return Ok(std::vector<std::filesystem::path>{});
    }

    co_return Ok(std::move(paths.value()));
}

bool file_openFolder_h(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) {
        auto defaultPath = SoberManager::get()->getLinuxPath(utils::string::pathToString(path));
        SoberManager::get()->browseFiles(defaultPath);
        return true;
    }
    return false;
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
    }
}