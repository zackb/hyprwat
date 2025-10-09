#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>

struct WifiNetwork {
    std::string ssid;
    int strength; // 0-100
};

class NetworkManagerClient {

public:
    NetworkManagerClient();
    std::vector<WifiNetwork> listWifiNetworks();
    bool connectToNetwork(const std::string& ssid, const std::string& password);

private:
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IProxy> proxy;

    std::vector<sdbus::ObjectPath> getWifiDevices();
    std::vector<sdbus::ObjectPath> getAccessPoints(const sdbus::ObjectPath& device);
};
