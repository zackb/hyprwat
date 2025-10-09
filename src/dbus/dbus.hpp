#pragma once

#pragma once
#include <sdbus-c++/sdbus-c++.h>

class DBusClient {
public:
    DBusClient(const std::string& busName, const std::string& objectPath);
    sdbus::IProxy& proxy();

private:
    std::unique_ptr<sdbus::IConnection> _connection;
    std::unique_ptr<sdbus::IProxy> _proxy;
};
