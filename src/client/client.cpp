#include <iostream>
#include <cstdlib>
#include <csignal>
#include <event.hpp>
#include <client.hpp>
#include <logger.hpp>
#include <logger_console_sink.hpp>

util::client *c;

void flushStdout() {
    std::cout << "\r> ";
    fflush(stdout);
}

bool handleRecv(util::client_receve_event& t_event) {
    std::string message = t_event.getDataAsString();

    std::cout << "\r" << message << std::endl;
    flushStdout();

    return true;
}

void onEvent(util::event<util::CLIENT_EVENTS>& t_event) {
    util::event_dispatcher<util::CLIENT_EVENTS> dispatcher(t_event);
    dispatcher.dispatch<util::client_receve_event>(handleRecv);
}

int main(int argc, char** argv) {
    util::logger logger;
    logger.addSink<util::logger_console_sink>("console_logger");
    logger.setSinkLogLevel("console_logger", util::LOGGER_SINK_LEVEL::ERROR);
    c = new util::client("127.0.0.1", 5001, &onEvent, logger);

    std::string command = "";
    while (command != "exit") {
        flushStdout();
        std::getline(std::cin, command);

        if (command == "reconnect") {
            c->reconnect();
        } else if (command == "disconnect") {
            c->disconnect();
        } else if (command != "exit") {
            c->sendServer(command);
        }
    }

    c->disconnect();

    delete c;

    return EXIT_SUCCESS;
}
