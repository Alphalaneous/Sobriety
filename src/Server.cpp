#include "Server.hpp"
#include "Geode/loader/Log.hpp"
#include "SoberManager.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

Server::Server() {
    m_server = std::make_shared<ix::WebSocketServer>(20613, "127.0.0.1");
}

void Server::startServer() {
    ix::initNetSystem();
    m_server->setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            onMessage(msg->str);
        }
    });

    auto res = m_server->listen();
    if (!res.first) {
        log::error("{}", res.second);
        return;
    }

    m_server->start();
}

void Server::sendMessage(const std::string& str) {
    for (auto& client : m_server->getClients()) {
        client->sendText(str);
    }
}

void Server::close() {
    matjson::Value value;
    value["type"] = "close";
    sendMessage(value.dump());
}

void Server::onMessage(const std::string& str) {
    auto jsonRes = matjson::parse(str);
    if (jsonRes.isErr()) return;

    auto json = jsonRes.unwrap();

    auto type = json["type"].asString().unwrapOrDefault();
    auto message = json["message"].asString().unwrapOrDefault();

    if (type == "close") {
        utils::game::exit(false);
    }
    else if (type == "info") {
        log::info("[Bridge]: {}", message);
    }
    else if (type == "warn") {
        log::warn("[Bridge]: {}", message);
    }
    else if (type == "error") {
        log::error("[Bridge]: {}", message);
    }
    else if (type == "debug") {
        log::debug("[Bridge]: {}", message);
    }
    else if (type == "file_picker") {
        auto status = json["status"].asString().unwrapOrDefault();
        auto idRes = json["id"].asString();
        if (!idRes) return;

        auto id = idRes.unwrap();
        if (status == "cancelled") {
            SoberManager::get()->onFileCancelled();
        }
        else if (status == "picked") {
            auto itemsRes = json["items"].asArray();
            if (!itemsRes) {
                SoberManager::get()->onFileCancelled();
                return;
            }

            auto items = itemsRes.unwrap();
            std::vector<std::string> itemsVec;

            for (const auto& item : items) {
                auto strRes = item.asString();
                if (!strRes) continue;
                auto str = strRes.unwrap();
                itemsVec.push_back(str);
            }

            SoberManager::get()->onFilePicked(itemsVec);
        }
    }
    else if (type == "connected") {
        auto logPath = fmt::format("/tmp/{}/console.ansi", Mod::get()->getID());
        
        matjson::Value json;
        json["type"] = "console";
        json["directory"] = logPath;
        json["font-size"] = sobriety::utils::getFontSize();

        Server::get()->sendMessage(json.dump());

        SoberManager::get()->setConsoleColors();
    }
}