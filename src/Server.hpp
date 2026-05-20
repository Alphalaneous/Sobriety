#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include "Utils.hpp"

class Server : public sobriety::utils::Singleton<Server> {
public:
    Server();
    void startServer();
    void sendMessage(const std::string& str);
    void close();
protected:
    void onMessage(const std::string& str);

    std::shared_ptr<ix::WebSocketServer> m_server;
};