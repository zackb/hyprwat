#pragma once
#include "src/dbus/dbus.hpp"

class PipeWireClient {
public:
    PipeWireClient();
    std::vector<AudioDevice> listSinks();
    void setDefaultSink(const std::string& id);

private:
    DBusClient pw;
};
