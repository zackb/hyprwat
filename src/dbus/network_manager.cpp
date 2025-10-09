#include "network_manager.hpp"
#include "../util.hpp"
#include <iostream>

NetworkManagerClient::NetworkManagerClient() {
    connection = sdbus::createSystemBusConnection();
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
std::vector<WifiNetwork> NetworkManagerClient::listWifiNetworks() {
    std::vector<WifiNetwork> networks;

    auto wifiDevices = getWifiDevices();
    if (wifiDevices.empty()) {
        std::cerr << "No Wi-Fi devices found" << std::endl;
        return networks;
    }

    std::cout << "Found " << wifiDevices.size() << " Wi-Fi devices" << std::endl;

    for (auto& devicePath : wifiDevices) {
        auto devProxy =
            sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), devicePath);

        // request a scan on this device
        try {
            devProxy->callMethod("RequestScan")
                .onInterface("org.freedesktop.NetworkManager.Device.Wireless")
                .withArguments(std::map<std::string, sdbus::Variant>{});
        } catch (const sdbus::Error& e) {
            std::cerr << "RequestScan failed on device " << devicePath << ": " << e.getMessage() << std::endl;
        }

        // give nm time to complete scan
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // get all access points for this device
        std::vector<sdbus::ObjectPath> apPaths;
        try {
            apPaths = getAccessPoints(devicePath);
        } catch (const sdbus::Error& e) {
            std::cerr << "Failed to get access points for device " << devicePath << ": " << e.getMessage() << std::endl;
            continue;
        }

        // iterate over access points and get ssid and signal strength
        for (auto& apPath : apPaths) {
            try {
                auto apProxy =
                    sdbus::createProxy(*connection, sdbus::ServiceName("org.freedesktop.NetworkManager"), apPath);

                // ssid
                sdbus::Variant ssidVar;
                apProxy->callMethod("Get")
                    .onInterface("org.freedesktop.DBus.Properties")
                    .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Ssid")
                    .storeResultsTo(ssidVar);
                auto ssidBytes = ssidVar.get<std::vector<uint8_t>>();
                std::string ssid(ssidBytes.begin(), ssidBytes.end());

                // signal strength
                sdbus::Variant strengthVar;
                apProxy->callMethod("Get")
                    .onInterface("org.freedesktop.DBus.Properties")
                    .withArguments("org.freedesktop.NetworkManager.AccessPoint", "Strength")
                    .storeResultsTo(strengthVar);
                uint8_t strength = strengthVar.get<uint8_t>();

                networks.push_back({ssid, strength});
            } catch (const sdbus::Error& e) {
                std::cerr << "Skipping AP " << apPath << " due to D-Bus error: " << e.getMessage() << std::endl;
            }
        }
    }

    return networks;
}

// connect to network
bool NetworkManagerClient::connectToNetwork(const std::string& ssid, const std::string& password) {
    auto wifiDevices = getWifiDevices();
    if (wifiDevices.empty())
        return false;

    auto devicePath = wifiDevices[0]; // pick first wifi device for now
    auto apPaths = getAccessPoints(devicePath);
    sdbus::ObjectPath apPath("/"); // can optionally pick specific AP

    // Convert ssid to byte array
    std::vector<uint8_t> ssidBytes(ssid.begin(), ssid.end());

    std::map<std::string, std::map<std::string, sdbus::Variant>> connection;
    connection["connection"] = {{"id", sdbus::Variant(ssid)},
                                {"type", sdbus::Variant("802-11-wireless")},
                                {"uuid", sdbus::Variant(generateUuid())}};

    connection["802-11-wireless"] = {{"ssid", sdbus::Variant(ssidBytes)}, {"mode", sdbus::Variant("infrastructure")}};

    if (!password.empty()) {
        connection["802-11-wireless-security"] = {{"key-mgmt", sdbus::Variant("wpa-psk")},
                                                  {"psk", sdbus::Variant(password)}};
    }

    try {
        sdbus::ObjectPath activeConnection, resultDevice;
        proxy->callMethod("AddAndActivateConnection")
            .onInterface("org.freedesktop.NetworkManager")
            .withArguments(connection, devicePath, apPath)
            .storeResultsTo(activeConnection, resultDevice);

        std::cout << "Active connection path: " << std::string(activeConnection) << std::endl;
        return true;
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to connect: " << e.getName() << " " << e.getMessage() << std::endl;
        return false;
    }
}
