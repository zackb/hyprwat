#pragma once

#include "../vec.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace hyprland {
    struct Workspace {
        int id;
        std::string name;
        std::string monitor;
        bool active = false;
    };

    struct Client {
        std::string address;
        std::string title;
        std::string class_;
        std::string initialClass;
        std::string initialTitle;
        int workspaceId = -1;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool mapped = false;
        bool hidden = false;
    };

    struct Monitor {
        int id;
        std::string name;
        int x;
        int y;
        int width;
        int height;
        float scale;
        bool focused;
    };

    class Control {
    public:
        explicit Control();
        explicit Control(const std::string& socketPath);
        ~Control();

        // Send command, return raw response
        std::string send(const std::string& command);

        // these are in fractional scale pixels
        float scale();
        Vec2 cursorPos();
        std::vector<Monitor> getMonitors();
        Monitor monitorAtCursor();

        void setWallpaper(const std::string& path);

        std::vector<Workspace> getWorkspaces();
        std::vector<Client> getClients();
        int getActiveWorkspaceId();
        void dispatchWorkspace(int id);

    private:
        std::string socketPath;
        bool luaProtocol = false;

        void detectLuaProtocol();
    };

    class Events {
    public:
        using EventCallback = std::function<void(const std::string&)>;

        explicit Events();
        explicit Events(const std::string& socketPath);
        ~Events();

        // Start listening on a background thread
        void start(EventCallback cb);

        // Stop listening
        void stop();

    private:
        void run(EventCallback cb);

        std::string socketPath;
        std::thread thread;
        std::atomic<bool> running{false};
        std::mutex mtx;
        int fd{-1};
    };
} // namespace hyprland
