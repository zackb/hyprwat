#include "dbus.hpp"

DBusClient::DBusClient(const std::string& busName, const std::string& objectPath) {
    _connection = sdbus::createSystemBusConnection();
    sdbus::ServiceName svc("org.freedesktop.NetworkManager");
    sdbus::ObjectPath path("/org/freedesktop/NetworkManager");
    _proxy = sdbus::createProxy(*_connection, svc, path);
}

sdbus::IProxy& DBusClient::proxy() { return *_proxy; }
