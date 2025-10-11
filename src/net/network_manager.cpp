#include "network_manager.hpp"
#include "../util.hpp"
#include <iostream>

NetworkManagerClient::NetworkManagerClient() {
    connection = sdbus::createSystemBusConnection();
    connection->enterEventLoopAsync();
    proxy = sdbus::createProxy(*connection,
                               sdbus::ServiceName("org.freedesktop.NetworkManager"),
                               sdbus::ObjectPath("/org/freedesktop/NetworkManager"));
}

// returns all wifi capable devices
std::vector<sdbus::ObjectPath> NetworkManagerClient::getWifiDevices() {
    std::vector<sdbus::ObjectPath> devices;

    proxy->callMethod("GetDevices").onInterface("org.freedesktop.NetworkManager").storeResultsTo(devices);

    // Filter only wifi devices
    std::vector<sdbus::ObjectPath> wifiDevices;
    for (auto& devPath : devices) {
        auto devProxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), devPath);

        sdbus::Variant devTypeVar;
        devProxy->callMethod("Get")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments("org.freedesktop.NetworkManager.Device", "DeviceType")
            .storeResultsTo(devTypeVar);

        uint32_t devType = devTypeVar.get<uint32_t>();

        // NM_DEVICE_TYPE_WIFI = 2
        if (devType == 2) {
            wifiDevices.push_back(devPath);
        }
    }
    return wifiDevices;
}

// AP object paths on a given wifi device
std::vector<sdbus::ObjectPath> NetworkManagerClient::getAccessPoints(const sdbus::ObjectPath& devicePath) {
    std::vector<sdbus::ObjectPath> aps;

    auto devProxy = sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), devicePath);

    try {
        devProxy->callMethod("GetAllAccessPoints")
            .onInterface("org.freedesktop.NetworkManager.Device.Wireless")
            .storeResultsTo(aps);
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to get access points for device " << devicePath << ": " << e.getMessage() << std::endl;
    }
    return aps;
}

// list networks
// just returns currently known access points, no scanning
std::vector<WifiNetwork> NetworkManagerClient::listWifiNetworks() {
    std::map<std::string, int> networkMap; // SSID -> max strength

    auto wifiDevices = getWifiDevices();
    if (wifiDevices.empty()) {
        std::cerr << "No Wi-Fi devices found" << std::endl;
        return {};
    }

    for (auto& devicePath : wifiDevices) {
        std::vector<sdbus::ObjectPath> apPaths;
        try {
            apPaths = getAccessPoints(devicePath);
        } catch (const sdbus::Error& e) {
            std::cerr << "Failed to get access points for device " << devicePath << ": " << e.getMessage() << std::endl;
            continue;
        }

        for (auto& apPath : apPaths) {
            try {
                auto apProxy =
                    sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), apPath);

                sdbus::Variant ssidVar;
                apProxy->callMethod("Get")
                    .onInterface("org.freedesktop.DBus.Properties")
                    .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Ssid")
                    .storeResultsTo(ssidVar);
                auto ssidBytes = ssidVar.get<std::vector<uint8_t>>();
                std::string ssid(ssidBytes.begin(), ssidBytes.end());

                sdbus::Variant strengthVar;
                apProxy->callMethod("Get")
                    .onInterface("org.freedesktop.DBus.Properties")
                    .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Strength")
                    .storeResultsTo(strengthVar);
                int strength = static_cast<int>(strengthVar.get<uint8_t>());

                if (networkMap.find(ssid) == networkMap.end() || networkMap[ssid] < strength) {
                    networkMap[ssid] = strength;
                }
            } catch (const sdbus::Error& e) {
                // Ignore errors
            }
        }
    }

    std::vector<WifiNetwork> networks;
    for (const auto& [ssid, strength] : networkMap) {
        networks.push_back({ssid, strength});
    }

    return networks;
}

// initiates scan and calls callback for each newly discovered AP
void NetworkManagerClient::scanWifiNetworks(std::function<void(const WifiNetwork&)> callback, int timeoutSeconds) {
    auto wifiDevices = getWifiDevices();
    if (wifiDevices.empty()) {
        std::cerr << "No Wi-Fi devices found" << std::endl;
        return;
    }

    for (auto& devicePath : wifiDevices) {
        auto devProxy =
            sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), devicePath);

        // setup signal handler for newly discovered APs
        devProxy->uponSignal("AccessPointAdded")
            .onInterface("org.freedesktop.NetworkManager.Device.Wireless")
            .call([&, callback](sdbus::ObjectPath apPath) {
                try {
                    auto apProxy =
                        sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), apPath);

                    sdbus::Variant ssidVar;
                    apProxy->callMethod("Get")
                        .onInterface("org.freedesktop.DBus.Properties")
                        .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Ssid")
                        .storeResultsTo(ssidVar);
                    auto ssidBytes = ssidVar.get<std::vector<uint8_t>>();
                    std::string ssid(ssidBytes.begin(), ssidBytes.end());

                    sdbus::Variant strengthVar;
                    apProxy->callMethod("Get")
                        .onInterface("org.freedesktop.DBus.Properties")
                        .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Strength")
                        .storeResultsTo(strengthVar);
                    int strength = static_cast<int>(strengthVar.get<uint8_t>());

                    if (!ssid.empty()) {
                        callback({ssid, strength});
                    }
                } catch (const sdbus::Error& e) {
                    // Ignore errors
                }
            });

        // Request scan
        try {
            devProxy->callMethod("RequestScan")
                .onInterface("org.freedesktop.NetworkManager.Device.Wireless")
                .withArguments(std::map<std::string, sdbus::Variant>{});
        } catch (const sdbus::Error& e) {
            std::cerr << "RequestScan failed on device " << devicePath << ": " << e.getMessage() << std::endl;
        }

        // Process events until timeout
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(timeoutSeconds);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            connection->processPendingEvent();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// connect to network
bool NetworkManagerClient::connectToNetwork(const std::string& ssid,
                                            const std::string& password,
                                            std::function<void(ConnectionState, const std::string&)> statusCallback) {

    auto wifiDevices = getWifiDevices();
    if (wifiDevices.empty())
        return false;

    if (connectionProxy) {
        connectionProxy.reset();
    }

    auto devicePath = wifiDevices[0]; // pick first wifi device for now
    auto apPaths = getAccessPoints(devicePath);
    sdbus::ObjectPath apPath("/"); // can optionally pick specific AP

    // Convert ssid to byte array
    std::vector<uint8_t> ssidBytes(ssid.begin(), ssid.end());

    std::map<std::string, std::map<std::string, sdbus::Variant>> conMap;
    conMap["connection"] = {{"id", sdbus::Variant(ssid)},
                            {"type", sdbus::Variant("802-11-wireless")},
                            {"uuid", sdbus::Variant(generateUuid())}};

    conMap["802-11-wireless"] = {{"ssid", sdbus::Variant(ssidBytes)}, {"mode", sdbus::Variant("infrastructure")}};

    if (!password.empty()) {
        conMap["802-11-wireless-security"] = {{"key-mgmt", sdbus::Variant("wpa-psk")},
                                              {"psk", sdbus::Variant(password)}};
    }

    try {
        sdbus::ObjectPath activeConnection, resultDevice;
        proxy->callMethod("AddAndActivateConnection")
            .onInterface("org.freedesktop.NetworkManager")
            .withArguments(conMap, devicePath, apPath)
            .storeResultsTo(activeConnection, resultDevice);

        std::cout << "Active connection path: " << std::string(activeConnection) << std::endl;

        // create proxy for the active connection
        connectionProxy =
            sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), activeConnection);

        // register signal handler for state changes
        connectionProxy->uponSignal("PropertiesChanged")
            .onInterface("org.freedesktop.DBus.Properties")
            .call([this, statusCallback](const std::string& interface,
                                         const std::map<std::string, sdbus::Variant>& changed,
                                         const std::vector<std::string>& invalidated) {
                std::cout << "PropertiesChanged signal received on interface: " << interface << std::endl;
                std::cout << "Changed properties:" << std::endl;

                // Print ALL changed properties
                for (const auto& [key, value] : changed) {
                    std::cout << "  - " << key << std::endl;
                }

                // Print invalidated properties
                std::cout << "Invalidated properties:" << std::endl;
                for (const auto& prop : invalidated) {
                    std::cout << "  - " << prop << std::endl;
                }

                if (changed.count("State")) {
                    uint32_t state = changed.at("State").get<uint32_t>();
                    std::cout << "Connection state changed: " << state << std::endl;
                    if (statusCallback) {
                        switch (state) {
                        case 0: // NM_ACTIVE_CONNECTION_STATE_UNKNOWN
                            statusCallback(ConnectionState::UNKNOWN, "Unknown state");
                            break;
                        case 1: // NM_ACTIVE_CONNECTION_STATE_ACTIVATING
                            statusCallback(ConnectionState::ACTIVATING, "Connecting...");
                            break;
                        case 2: // NM_ACTIVE_CONNECTION_STATE_ACTIVATED
                            statusCallback(ConnectionState::ACTIVATED, "Connected");
                            break;
                        case 3: // NM_ACTIVE_CONNECTION_STATE_DEACTIVATING
                            statusCallback(ConnectionState::DEACTIVATING, "Disconnecting...");
                            break;
                        case 4: // NM_ACTIVE_CONNECTION_STATE_DEACTIVATED
                            statusCallback(ConnectionState::DEACTIVATED, "Disconnected");
                            break;
                        default:
                            statusCallback(ConnectionState::UNKNOWN, "Unknown state");
                            break;
                        }
                    }
                }
            });
        return true;
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to connect: " << e.getName() << " " << e.getMessage() << std::endl;
        return false;
    }
}

void NetworkManagerClient::processEvents() {
    if (connection)
        connection->processPendingEvent();
}
