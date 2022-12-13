#include <iostream>
#include <logger.hpp>
#include <logger_console_sink.hpp>
#include <algo.hpp>
#include <server.hpp>
#include <event.hpp>

util::server *s;

bool handleRecv(util::server_read_event& t_e) {
    std::string clientId = t_e.getClientId();
    std::string message = util::fmt("client ({}) said: {}", clientId, std::string(t_e.getClientBuff(), t_e.getBuffSize()));

    for (auto& client : s->getClientMap()) {
        if (client.first != clientId) {
            s->sendClient(client.first, message); 
        }
    }

    return true;
}

bool handleClientDisconnect(util::server_client_disconnect_event& t_e) {
    std::string clientId = t_e.getClientId();
    std::string message = util::fmt("client ({}) has left the chatroom", clientId);

    for (auto& client : s->getClientMap()) {
        if (client.first != clientId) {
            s->sendClient(client.first, message); 
        }
    }

    return true;
}

bool handleClientConnect(util::server_client_connect_event& t_e) {
    std::string clientId = t_e.getClientId();
    std::string message = util::fmt("client ({}) has joined the chatroom", clientId);

    for (auto& client : s->getClientMap()) {
        if (client.first != clientId) {
            s->sendClient(client.first, message); 
        }
    }

    return true;
}

void onEvent(util::event<util::SERVER_EVENTS>& t_event) {
    util::event_dispatcher<util::SERVER_EVENTS> dispatcher(t_event);

    dispatcher.dispatch<util::server_read_event>(&handleRecv);
    dispatcher.dispatch<util::server_client_disconnect_event>(&handleClientDisconnect);
    dispatcher.dispatch<util::server_client_connect_event>(&handleClientConnect);
}

int main(int argc, char** argv) {
    
    // * create the logger
    util::logger serverLogger;
    // * give the logger a console sink
    serverLogger.addSink<util::logger_console_sink>("server_console_logger");

    // * create the server
    s = new util::server(5001, util::SERVER_METHOD::TCP, &onEvent, serverLogger);

    std::string command = "";
    while (command != "exit") {
        std::getline(std::cin, command);
    }

    // * delete the server
    delete s;

    return EXIT_SUCCESS;
}
