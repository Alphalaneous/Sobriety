#include <Geode/Geode.hpp>
#include "Server.hpp"
#include "SoberManager.hpp"
#include "Utils.hpp"

using namespace geode::prelude;

$on_game(Exiting) {
    if (sobriety::utils::isWine()) {
        Server::get()->close();
    }
}

$on_mod(Loaded) {
    if (sobriety::utils::isWine()) {
        SoberManager::get()->startLogger();
        Server::get()->startServer();
        SoberManager::get()->startScript();
    }
    else {
        (void) Mod::get()->uninstall();
    }
}

$on_game(Loaded) {
    if (!sobriety::utils::isWine()) {
        createQuickPopup("Windows User Detected!", "Sobriety only works on <cg>Linux</c> systems and will do nothing on <cb>Windows</c>.\nIt has been <cr>uninstalled</c>.", "OK", nullptr, nullptr);
    }
}