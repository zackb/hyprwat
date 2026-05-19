#include "ipc.hpp"
#include "../debug/log.hpp"
#include <yaml-cpp/yaml.h>

#include <cstring>
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
    Control::Control(const std::string& socketPath) : socketPath(socketPath) { detectLuaProtocol(); }

    Control::~Control() {}

    void Control::detectLuaProtocol() {
        try {
            std::string response = send("dispatch workspace __hyprwat_probe__");
            if (response.find("hl.dispatch") != std::string::npos) {
                luaProtocol = true;
                debug::log(INFO, "Hyprland Lua IPC protocol detected");
            } else {
                luaProtocol = false;
            }
        } catch (...) {
            // fallback to legacy
            luaProtocol = false;
        }
    }

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
        std::string response;
        while (true) {
            ssize_t n = read(wfd, buf, sizeof(buf));
            if (n < 0) {
                close(wfd);
                throw std::runtime_error("Failed to read response");
            }
            if (n == 0) {
                break;
            }
            response.append(buf, n);
        }

        close(wfd);
        return response;
    }

    Vec2 Control::cursorPos() {
        std::string response = send("cursorpos");
        int x = 0, y = 0;
        if (sscanf(response.c_str(), "%d, %d", &x, &y) != 2) {
            throw std::runtime_error("Failed to parse cursor position");
        }
        return {(float)x, (float)y};
    }

    std::vector<Monitor> Control::getMonitors() {
        std::vector<Monitor> result;
        std::string response = send("j/monitors");
        try {
            auto node = YAML::Load(response);
            if (node.IsSequence()) {
                for (const auto& m : node) {
                    Monitor monitor;
                    monitor.id = m["id"].as<int>();
                    monitor.name = m["name"].as<std::string>();
                    monitor.x = m["x"].as<int>();
                    monitor.y = m["y"].as<int>();
                    monitor.width = m["width"].as<int>();
                    monitor.height = m["height"].as<int>();
                    monitor.scale = m["scale"].as<float>();
                    monitor.focused = m["focused"].as<bool>();
                    result.push_back(monitor);
                }
            }
        } catch (const std::exception& e) {
            debug::log(ERR, "Failed to parse monitors: {}", e.what());
        }
        return result;
    }

    Monitor hyprland::Control::monitorAtCursor() {
        Vec2 cursor = cursorPos();
        auto monitors = getMonitors();
        for (auto& m : monitors) {
            if (cursor.x >= m.x && cursor.x < m.x + m.width && cursor.y >= m.y && cursor.y < m.y + m.height) {
                return m;
            }
        }
        return monitors[0]; // fallback
    }

    float Control::scale() {
        std::string response = send("monitors");

        // "scale: " followed by a number
        size_t pos = response.find("scale: ");
        if (pos != std::string::npos) {
            float scale;
            if (sscanf(response.c_str() + pos + 7, "%f", &scale) == 1) {
                return scale;
            }
        }

        return 1.0f; // fallback
    }

    void Control::setWallpaper(const std::string& path) {
        if (luaProtocol) {
            std::string response =
                send("/dispatch hl.dsp.exec_cmd(\"hyprctl hyprpaper preload \\\"" + path + "\\\"\")");
            if (response != "ok") {
                debug::log(ERR, "Failed to preload wallpaper: {}", response);
            }
            response = send("/dispatch hl.dsp.exec_cmd(\"hyprctl hyprpaper wallpaper \\\", " + path + "\\\"\")");
            if (response != "ok") {
                debug::log(ERR, "Failed to set wallpaper: {}", response);
            }
            send("/dispatch hl.dsp.exec_cmd(\"hyprctl hyprpaper unload unused\")");
        } else {
            std::string response = send("/keyword exec hyprctl hyprpaper preload \"" + path + "\"");
            if (response != "ok") {
                debug::log(ERR, "Failed to preload wallpaper: {}", response);
            }
            response = send("/keyword exec hyprctl hyprpaper wallpaper \"," + path + "\"");
            if (response != "ok") {
                debug::log(ERR, "Failed to set wallpaper: {}", response);
            }
            send("/keyword exec hyprctl hyprpaper unload unused");
        }
    }

    std::vector<Workspace> Control::getWorkspaces() {
        std::vector<Workspace> result;
        std::string response = send("j/workspaces");
        try {
            auto node = YAML::Load(response);
            if (node.IsSequence()) {
                for (const auto& w : node) {
                    Workspace workspace;
                    workspace.id = w["id"].as<int>();
                    workspace.name = w["name"].as<std::string>();
                    workspace.monitor = w["monitor"].as<std::string>();
                    workspace.active = false; // not provided easily, hyprctl activeworkspace has it
                    result.push_back(workspace);
                }
            }
        } catch (const std::exception& e) {
            debug::log(ERR, "Failed to parse workspaces: {}", e.what());
        }
        return result;
    }

    std::vector<Client> Control::getClients() {
        std::vector<Client> result;
        std::string response = send("j/clients");
        try {
            auto node = YAML::Load(response);
            if (node.IsSequence()) {
                for (const auto& c : node) {
                    Client client;
                    client.address = c["address"].as<std::string>();
                    client.title = c["title"].as<std::string>();
                    client.class_ = c["class"].as<std::string>();
                    client.initialClass = c["initialClass"].as<std::string>();
                    client.initialTitle = c["initialTitle"].as<std::string>();
                    client.workspaceId = c["workspace"]["id"].as<int>();
                    auto at = c["at"].as<std::vector<int>>();
                    if (at.size() >= 2) {
                        client.x = at[0];
                        client.y = at[1];
                    }
                    auto size = c["size"].as<std::vector<int>>();
                    if (size.size() >= 2) {
                        client.width = size[0];
                        client.height = size[1];
                    }
                    client.mapped = c["mapped"].as<bool>(true);
                    client.hidden = c["hidden"].as<bool>(false);
                    result.push_back(client);
                }
            }
        } catch (const std::exception& e) {
            debug::log(ERR, "Failed to parse clients: {}", e.what());
        }
        return result;
    }

    int Control::getActiveWorkspaceId() {
        std::string response = send("j/activeworkspace");
        try {
            auto node = YAML::Load(response);
            if (node["id"]) {
                return node["id"].as<int>();
            }
        } catch (const std::exception& e) {
            debug::log(ERR, "Failed to parse activeworkspace: {}", e.what());
        }
        return -1;
    }

    void Control::dispatchWorkspace(int id) {
        if (luaProtocol) {
            send("/dispatch hl.dsp.focus({ workspace = \"" + std::to_string(id) + "\" })");
        } else {
            send("dispatch workspace " + std::to_string(id));
        }
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
            debug::log(ERR, "Failed to create event socket");
            return;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(localFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            debug::log(ERR, "Failed to connect to event socket: {}", std::strerror(errno));
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
