#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>

struct WifiNetwork {
    std::string ssid;
    int strength; // 0-100
};

namespace ConnectionState {
    constexpr uint32_t UNKNOWN = 0;
    constexpr uint32_t ACTIVATING = 1;
    constexpr uint32_t ACTIVATED = 2;
    constexpr uint32_t DEACTIVATING = 3;
    constexpr uint32_t DEACTIVATED = 4;
} // namespace ConnectionState

class NetworkManagerClient {

public:
    NetworkManagerClient();
    std::vector<WifiNetwork> listWifiNetworks();
    void scanWifiNetworks(std::function<void(const WifiNetwork&)> callback, int timeoutSeconds = 5);
    bool connectToNetwork(const std::string& ssid, const std::string& password);

private:
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IProxy> proxy;
    std::unique_ptr<sdbus::IProxy> connectionProxy;

    std::vector<sdbus::ObjectPath> getWifiDevices();
    std::vector<sdbus::ObjectPath> getAccessPoints(const sdbus::ObjectPath& device);
};
