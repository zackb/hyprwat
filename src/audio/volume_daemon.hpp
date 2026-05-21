#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class VolumeDaemon {
public:
    VolumeDaemon() = default;
    ~VolumeDaemon();

    // Tries to connect to an existing running daemon instance and send a command.
    // Returns true if successfully connected and command sent (meaning another instance is running).
    static bool sendCommand(const std::string& command);

    // Starts the socket server listener in a background thread.
    // Commands received will trigger the provided callback.
    bool startServer(std::function<void(const std::string&)> commandCallback);

    // Stops the server listener and cleans up socket files.
    void stopServer();

    // Gets the path to the Unix domain socket.
    static std::string getSocketPath();

private:
    int serverFd = -1;
    std::thread serverThread;
    std::atomic<bool> running{false};
    std::function<void(const std::string&)> callback;
};
