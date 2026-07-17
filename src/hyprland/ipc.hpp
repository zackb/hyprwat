#pragma once

#include "../compositor/compositor.hpp"
#include "../vec.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace hyprland {
    using compositor::Client;
    using compositor::Monitor;
    using compositor::Workspace;

    class Control : public compositor::Compositor {
    public:
        explicit Control();
        explicit Control(const std::string& socketPath);
        ~Control();

        // Send command, return raw response
        std::string send(const std::string& command) const;

        // these are in fractional scale pixels
        float scale();
        Vec2 cursorPos() override;
        std::optional<Monitor> monitorAtCursor(const Vec2& cursor) override;

        void setWallpaper(const std::string& path) override;

        std::vector<Workspace> getWorkspaces() override;
        std::vector<Client> getClients() override;
        int getActiveWorkspaceId() override;
        void dispatchWorkspace(int id) override;

        bool supportsOverview() const override { return true; }

    private:
        std::vector<Monitor> getMonitors();

        std::string socketPath;
        mutable bool luaProtocol = false;
        mutable bool luaProtocolDetected = false;

        void detectLuaProtocol() const;
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
