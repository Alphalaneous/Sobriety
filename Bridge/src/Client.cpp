#include "Client.hpp"
#include <matjson.hpp>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>
#include "DrunkManager.hpp"
#include "FilePicker.hpp"

void Client::sendMessage(const std::string& str) {
    std::lock_guard<std::mutex> lock(m_sendMutex);

    if (m_webSocket.getReadyState() == ix::ReadyState::Open) {
        m_webSocket.send(str);
    }
}

bool Client::isClosed() {
    return m_webSocket.getReadyState() == ix::ReadyState::Closed;
}

void Client::startClient() {
    ix::initNetSystem();

    m_webSocket.setUrl("ws://127.0.0.1:20613");

    m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Close || msg->type == ix::WebSocketMessageType::Error) {
            m_conditionVariable.notify_one();
        }
        else if (msg->type == ix::WebSocketMessageType::Open) {
            matjson::Value json;
            json["type"] = "connected";

            sendMessage(json.dump());
        }
        else if (msg->type == ix::WebSocketMessageType::Message) {
            onMessage(msg->str);
        }
    });

    m_webSocket.start();
    m_webSocket.connect(100);
}

void Client::wait() {
    std::unique_lock<std::mutex> lock(m_conditionVariableMutex);
    m_conditionVariable.wait(lock);
}

void Client::onMessage(const std::string& str) {
    auto jsonRes = matjson::parse(str);
    if (jsonRes.isErr()) return;

    auto json = jsonRes.unwrap();

    auto type = json["type"].asString().unwrapOrDefault();

    if (type == "close") {
        m_conditionVariable.notify_one();
    }
    else if (type == "file_picker") {
        auto actionRes = json["action"].asString();
        if (!actionRes) return;
        auto action = actionRes.unwrap();

        if (action == "browse") {
            DrunkManager::get()->browseFiles(json["directory"].asString().unwrapOrDefault());
        }
        else {
            auto idRes = json["id"].asString();
            if (!idRes) return;
            auto id = idRes.unwrap();

            DrunkManager::get()->openFilePicker(id, 
                json["title"].asString().unwrapOr("Pick File"), 
                json["isMultiple"].asBool().unwrapOrDefault(), 
                json["isDirectory"].asBool().unwrapOrDefault(),
                json["directory"].asString().unwrapOrDefault(), 
                json["name"].asString().unwrapOrDefault(), 
                json["filters"].asString().unwrapOrDefault(),
                action == "save" ? FilePicker::Type::SAVE : FilePicker::Type::PICK
            );
        }
    }
    else if (type == "console") {
        auto directoryRes = json["directory"].asString();
        if (!directoryRes) return;
        auto directory = directoryRes.unwrap();

        int fontSize = json["font-size"].asInt().unwrapOr(10);

        DrunkManager::get()->openConsole(directory, fontSize);
    }
}