#include "daemon.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

std::string Daemon::getSocketPath() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    std::string path;
    if (xdg) {
        path = std::string(xdg) + "/hyprwatd.sock";
    } else {
        const char* user = std::getenv("USER");
        path = "/tmp/hyprwatd-" + std::string(user ? user : "default") + ".sock";
    }
    return path;
}

bool Daemon::sendCommand(const std::string& command) {
    std::string sock_path = getSocketPath();
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(client_fd, command.c_str(), command.size(), 0);
        close(client_fd);
        return true;
    }

    close(client_fd);
    return false;
}

bool Daemon::startServer(std::function<void(const std::string&)> commandCallback) {
    if (running) {
        return false;
    }

    std::string sock_path = getSocketPath();
    unlink(sock_path.c_str()); // remove stale socket if it exists

    serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) != 0 || listen(serverFd, 5) != 0) {
        close(serverFd);
        serverFd = -1;
        return false;
    }

    callback = commandCallback;
    running = true;

    serverThread = std::thread([this]() {
        while (running) {
            int conn_fd = accept(serverFd, nullptr, nullptr);
            if (conn_fd < 0) {
                break;
            }

            char buffer[128];
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes = recv(conn_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes > 0 && callback && running) {
                callback(std::string(buffer));
            }
            close(conn_fd);
        }
    });

    return true;
}

void Daemon::stopServer() {
    running = false;
    if (serverFd >= 0) {
        // shutdown first to wake up blocking accept()
        shutdown(serverFd, SHUT_RDWR);
        close(serverFd);
        serverFd = -1;
    }
    if (serverThread.joinable()) {
        serverThread.join();
    }
    std::string sock_path = getSocketPath();
    unlink(sock_path.c_str());
}

Daemon::~Daemon() { stopServer(); }
