#include "Client.hpp"

int main() {
    Client::get()->startClient();
    Client::get()->wait();
}