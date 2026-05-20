#include <Geode/Geode.hpp>
#include "Server.hpp"
#include "SoberManager.hpp"

using namespace geode::prelude;

$on_game(Exiting) {
    Server::get()->close();
}

$on_mod(Loaded) {
    Server::get()->startServer();
    SoberManager::get()->startScript();
}