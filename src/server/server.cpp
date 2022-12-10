#include <cstdlib>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <logger.hpp>
#include <logger_console_sink.hpp>
#include <exception>
#include <future>
#include <vector>
#include <map>
#include <sys/time.h>
#include <arpa/inet.h>
#include <algo.hpp>

enum class SERVER_METHOD {
    TCP,
    UDP,
};

// * TODO: Event dispatcher (accept event, read event)
// * TODO: send function (sent a message to a client id)
// * TODO: UDP mode
class server {
public:
    
    server(const int& t_port, const SERVER_METHOD& t_serverMethod) {
        // * set port and server method
        m_port = t_port; 
        m_serverMethod = t_serverMethod;
        m_listening = false;
        
        // * create logger
        serverLogger.addSink<util::logger_console_sink>(CONSOLE_LOGGER_ID, "[%^%l%$] %v");
        serverLogger.logInfo("server logger initalized");

        // * initalize the server socket
        sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);

        // * create the server socket
        if ((m_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            serverLogger.logCritical("failed to create server socket");
            throw std::runtime_error("socket creation failed");
        }

        // * set the socket options
        if (setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            serverLogger.logCritical("field to set server socket options");
            serverLogger.logError("setsockopt error: {}", strerror(errno));
            throw std::runtime_error("setsockopt error");
        }

        // * initalize the address and port to use 
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(m_port);

        // * bind the socket to the addresss and port
        if (bind(m_serverFd, (sockaddr*)&address, sizeof(address)) < 0) {
            serverLogger.logCritical("failed to bind server socket to port {}", m_port);
            serverLogger.logError("bind error: {}", strerror(errno));
            throw std::runtime_error("failed to bind to socket");
        }

        // * setup the socket to listen for connections with a backlog queue of 3
        if (listen(m_serverFd, 3) < 0) {
            serverLogger.logCritical("failed to listen for incoming connections");
            serverLogger.logError("listen error: {}", strerror(errno));
            throw std::runtime_error("failed to accept connection");
        }

        serverLogger.logInfo("initalized server on port {}", m_port);

        // * start the listening thread
        m_listening = true;
        listenThread = std::async(std::launch::async, &server::serverListen, this);
    }

    ~server() {
        // * make the server stop listinging for connections
        m_listening = false;
        listenThread.wait();

        // * close connected sockets
        for (auto& con : m_serverConnections) {
            close(con.second);
        }

        // * shutdown server socker
        serverLogger.logInfo("shutting down server");
        shutdown(m_serverFd, SHUT_RDWR);
    }


private:

    virtual void handelRead(char* t_buff, int t_buffSize, std::string t_conId) {
        std::string message = std::string(t_buff);
        serverLogger.logInfo("client ({}) sent: {}", t_conId, message);
    }

    void serverListen() {
        serverLogger.logInfo("server started listening for connections...");

        // * set of socket descriptors
        fd_set readfds;
        int max_sd, sd, activity;
        timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 50;

        while (m_listening) {
            // * clear the socket set
            FD_ZERO(&readfds);

            // * add master (server) socket to set
            FD_SET(m_serverFd, &readfds);
            max_sd = m_serverFd;
            
            for (auto &con : m_serverConnections) {
                sd = con.second;

                if (sd > 0) {
                    FD_SET(sd, &readfds);
                }

                max_sd = std::max(max_sd, sd);
            }

            activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

            int errnum = errno;
            if ((activity < 0) && (errnum!=EINTR)) {
                serverLogger.logCritical("failed to read socket activity");
                serverLogger.logError("select error: {}", strerror(errnum));
                throw std::runtime_error("failed to get socket activity");
            }

            if (activity == 0) {
                continue;
            }

            if (FD_ISSET(m_serverFd, &readfds)) {
                sockaddr_in adress;
                int addrlen = sizeof(adress);
                int newsock;
                if ((newsock = accept(m_serverFd, (sockaddr*)&adress, (socklen_t*)&addrlen)) < 0) {
                    serverLogger.logCritical("failed to accept new connection");
                    serverLogger.logError("accept error: {}", strerror(errno));
                    throw std::runtime_error("failed to accept new connection");
                }

                std::string conId = util::generate_uuid_v4();
                serverLogger.logInfo("accepted new connection ({}) at IP {} at port {}", conId, inet_ntoa(adress.sin_addr), ntohs(adress.sin_port));

                m_serverConnections[conId] = newsock;
            } else {
                std::vector<std::string> disconnectedClients;
                for (auto &con : m_serverConnections) {
                    sd = con.second;

                    // * the current connection has had activity
                    if (FD_ISSET(sd, &readfds)) {
                        char buff[1024];
                        int size;

                        // * check if client disconeccted
                        if ((size = read(sd, buff, 1024)) == 0) {
                            sockaddr_in address;
                            int addrlen = sizeof(address);
                            getpeername(sd, (sockaddr*)&address, (socklen_t*)&addrlen);
                            serverLogger.logInfo("client ({}) disconnected from ip {}, port {}", con.first, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                            close(con.second);
                            disconnectedClients.push_back(con.first);
                            continue;
                        }

                        handelRead(std::move(buff), size, con.first);
                    }
                }

                for (auto &conId : disconnectedClients) {
                    m_serverConnections.erase(conId);
                }
            }


        }

        serverLogger.logInfo("finished listing");
    }
    
    std::map<std::string, int> m_serverConnections;
    std::future<void> listenThread;
    int m_port;
    int m_serverFd;
    SERVER_METHOD m_serverMethod;
    util::logger serverLogger;
    const std::string CONSOLE_LOGGER_ID = "console_logger";
    bool m_listening;
};

int main(int argc, char** argv) {
    
    server s(5001, SERVER_METHOD::TCP);

    std::string command = "";
    while (command != "exit") {
        std::getline(std::cin, command);
    }

    return EXIT_SUCCESS;
}
