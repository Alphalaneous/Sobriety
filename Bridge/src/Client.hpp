#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include "Utils.hpp"

class Client : public bridge::utils::Singleton<Client> {
public:
    void startClient();
    void wait();
    void sendMessage(const std::string& str);
    bool isClosed();

protected:
    void onMessage(const std::string& str);

    ix::WebSocket m_webSocket;

    std::condition_variable m_conditionVariable;
    std::mutex m_conditionVariableMutex;

    std::mutex m_sendMutex;
};