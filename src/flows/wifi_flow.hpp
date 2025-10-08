#pragma once

#include "flow.hpp"
#include <memory>
#include <string>
#include <vector>

class Selector;
class TextInput;

// TODO: implement networkmanager dbus
// 1. show list of wifi networks
// 2. user selects a network
// 3. prompt for password
// 4. return network:password for connection
class WiFiFlow : public Flow {
public:
    // Constructor takes a list of network SSIDs
    WiFiFlow(const std::vector<std::string>& networks);

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

    // Accessors for individual values
    std::string getSelectedNetwork() const;
    std::string getPassword() const;

private:
    enum class State { SELECT_NETWORK, ENTER_PASSWORD };

    State currentState = State::SELECT_NETWORK;

    std::unique_ptr<Selector> networkSelector;
    std::unique_ptr<TextInput> passwordInput;

    std::string selectedNetwork;
    std::string password;
    bool done = false;
};
