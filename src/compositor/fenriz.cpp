#include "fenriz.hpp"
#include "../debug/log.hpp"
#include <yaml-cpp/yaml.h>

#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace compositor {

    static std::string getSocketPath() {
        const char* path = std::getenv("FENRIZ_SOCKET");
        if (!path) {
            throw std::runtime_error("Not running inside fenriz (FENRIZ_SOCKET missing)");
        }
        return path;
    }

    static int connectSocket(const std::string& socketPath) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0)
            throw std::runtime_error("Failed to create socket");

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);
            throw std::runtime_error("Failed to connect to fenriz socket");
        }
        return fd;
    }

    // fenriz sends the snapshot on accept, freshly built, so one read is all a one-shot
    // client needs.
    static std::string readSnapshotLine(const std::string& socketPath) {
        int fd = connectSocket(socketPath);

        char buf[4096];
        std::string line;
        while (true) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n < 0) {
                close(fd);
                throw std::runtime_error("Failed to read fenriz snapshot");
            }
            if (n == 0)
                break; // EOF before a newline: take what we have and let the parser complain
            line.append(buf, n);
            size_t nl = line.find('\n');
            if (nl != std::string::npos) {
                line.resize(nl);
                break;
            }
        }

        close(fd);
        return line;
    }

    // JSON is a YAML 1.2 subset, so yaml-cpp parses it -- same trick used for hyprctl output.
    FenrizSnapshot parseFenrizSnapshot(const std::string& line) {
        FenrizSnapshot snap;
        auto node = YAML::Load(line);

        if (node["cursor"]) {
            snap.cursor = {node["cursor"]["x"].as<float>(), node["cursor"]["y"].as<float>()};
            snap.hasCursor = true;
        }

        if (node["outputs"] && node["outputs"].IsSequence()) {
            int id = 0;
            for (const auto& o : node["outputs"]) {
                Monitor m;
                m.id = id++; // fenriz keys outputs by name; index is a stable-enough stand-in
                m.name = o["name"].as<std::string>();
                m.x = o["x"].as<int>();
                m.y = o["y"].as<int>();
                m.width = o["width"].as<int>();
                m.height = o["height"].as<int>();
                m.scale = o["scale"].as<float>();
                m.focused = o["focused"].as<bool>();
                snap.monitors.push_back(m);
            }
        }

        if (node["workspaces"]) {
            const auto& ws = node["workspaces"];
            if (ws["active"]) {
                int active = ws["active"].as<int>();
                // fenriz reports 0 when no output is focused
                snap.activeWorkspace = active > 0 ? active : -1;
            }
            if (ws["occupied"] && ws["occupied"].IsSequence()) {
                for (const auto& id : ws["occupied"]) {
                    Workspace w;
                    w.id = id.as<int>();
                    w.name = std::to_string(w.id);
                    w.active = (w.id == snap.activeWorkspace);
                    snap.workspaces.push_back(w);
                }
            }
        }

        return snap;
    }

    Fenriz::Fenriz() : Fenriz(getSocketPath()) {}

    Fenriz::Fenriz(const std::string& socketPath) : socketPath(socketPath) {
        snapshot = parseFenrizSnapshot(readSnapshotLine(socketPath));
    }

    Vec2 Fenriz::cursorPos() {
        if (!snapshot.hasCursor) {
            throw std::runtime_error("fenriz did not report a cursor position (compositor too old?)");
        }
        return snapshot.cursor;
    }

    std::optional<Monitor> Fenriz::monitorAtCursor(const Vec2& cursor) {
        for (auto& m : snapshot.monitors) {
            if (cursor.x >= m.x && cursor.x < m.x + m.width && cursor.y >= m.y && cursor.y < m.y + m.height) {
                return m;
            }
        }
        return std::nullopt;
    }

    std::vector<Workspace> Fenriz::getWorkspaces() { return snapshot.workspaces; }

    int Fenriz::getActiveWorkspaceId() { return snapshot.activeWorkspace; }

    std::vector<Client> Fenriz::getClients() { return {}; }

    bool Fenriz::supportsOverview() const { return false; }

    void Fenriz::dispatchWorkspace(int id) {
        std::string cmd = "{\"cmd\":\"workspace\",\"n\":" + std::to_string(id) + "}\n";
        int fd = connectSocket(socketPath);
        if (write(fd, cmd.c_str(), cmd.size()) < 0) {
            debug::log(ERR, "Failed to dispatch workspace {}", id);
        }
        close(fd);
    }

    void Fenriz::setWallpaper(const std::string& path) { (void)path; }
} // namespace compositor
