#include <iostream>
#include <logger.hpp>
#include <logger_console_sink.hpp>
#include <algo.hpp>
#include <server.hpp>

void onEvent(util::event<util::SERVER_EVENTS>& t_event) {
    // std::cout << t_event.toString() << std::endl;
}

int main(int argc, char** argv) {
    
    util::logger serverLogger;
    serverLogger.addSink<util::logger_console_sink>("server_console_logger");
    util::server s(5001, util::SERVER_METHOD::TCP, &onEvent, serverLogger);

    std::string command = "";
    while (command != "exit") {
        std::getline(std::cin, command);
        if (command != "exit") {
            s.sendAllClients(command);
        }
    }

    return EXIT_SUCCESS;
}
