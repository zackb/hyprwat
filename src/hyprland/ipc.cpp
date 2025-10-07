#include "ipc.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace hyprland {

    static std::string getSocketPath(const char* filename) {
        const char* runtime = std::getenv("XDG_RUNTIME_DIR");
        const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");

        if (!runtime || !sig) {
            throw std::runtime_error("Not running inside Hyprland (env vars missing)");
        }
        return std::string(runtime) + "/hypr/" + sig + "/" + filename;
    }

    // Control
    Control::Control() : Control(getSocketPath(".socket.sock")) {}
    Control::Control(const std::string& socketPath) : socketPath(socketPath) {}

    Control::~Control() {}

    std::string Control::send(const std::string& command) {
        int wfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (wfd < 0)
            throw std::runtime_error("Failed to create socket");

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(wfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(wfd);
            throw std::runtime_error("Failed to connect to control socket");
        }

        // send command
        write(wfd, command.c_str(), command.size());

        // read response
        char buf[4096];
        ssize_t n = read(wfd, buf, sizeof(buf) - 1);
        if (n < 0) {
            close(wfd);
            throw std::runtime_error("Failed to read response");
        }
        buf[n] = '\0';

        close(wfd);
        return std::string(buf);
    }
    Vec2 Control::getCursorPos() {
        std::string response = send("cursorpos");
        int x = 0, y = 0;
        if (sscanf(response.c_str(), "%d, %d", &x, &y) != 2) {
            throw std::runtime_error("Failed to parse cursor position");
        }
        return {(float)x, (float)y};
    }

    // Events

    Events::Events() : Events(getSocketPath(".socket2.sock")) {}

    Events::Events(const std::string& socketPath) : socketPath(socketPath) {}

    Events::~Events() { stop(); }

    void Events::start(EventCallback cb) {
        if (running)
            return;
        running = true;
        thread = std::thread(&Events::run, this, cb);
    }

    void Events::stop() {
        if (!running) {
            return;
        }
        running = false;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (fd != -1) {
                shutdown(fd, SHUT_RD);
                fd = -1;
            }
        }

        if (thread.joinable()) {
            thread.join();
        }
    }

    void Events::run(EventCallback cb) {
        int localFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (localFd < 0) {
            std::cerr << "Failed to create event socket\n";
            return;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(localFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "Failed to connect to event socket\n";
            close(fd);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            fd = localFd;
        }

        char buf[1024];
        std::string line;

        while (running) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0)
                break; // socket closed or error
            line.append(buf, n);

            // simple line splitting
            size_t pos;
            while ((pos = line.find('\n')) != std::string::npos) {
                std::string event = line.substr(0, pos);
                line.erase(0, pos + 1);
                if (!event.empty())
                    cb(event);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (fd != -1) {
                close(fd);
            }
        }
    }

} // namespace hyprland
