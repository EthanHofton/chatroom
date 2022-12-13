#include "event.hpp"
#include <cstdlib>
#include <iostream>
#include <client.hpp>
#include <logger_console_sink.hpp>
#include <csignal>

void onEvent(util::event<util::CLIENT_EVENTS>& t_event) {
    util::event_dispatcher<util::CLIENT_EVENTS> dispatcher(t_event);
}

int main(int argc, char** argv) {
    util::logger logger;
    logger.addSink<util::logger_console_sink>("console_logger");
    util::client c("127.0.0.1", 5001, &onEvent, logger);

    std::string command = "";
    while (command != "exit") {
        std::getline(std::cin, command);

        if (command == "reconnect") {
            c.reconnect();
        } else if (command == "disconnect") {
            c.disconnect();
        } else if (command != "exit") {
            c.sendServer(command);
        }
    }

    c.disconnect();

    return EXIT_SUCCESS;
}
