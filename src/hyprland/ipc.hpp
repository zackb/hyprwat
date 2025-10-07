#pragma once

#include "../vec2.hpp"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace hyprland {
    class Control {
    public:
        explicit Control();
        explicit Control(const std::string& socketPath);
        ~Control();

        // Send command, return raw response
        std::string send(const std::string& command);

        Vec2 getCursorPos();

    private:
        std::string socketPath;
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
