#pragma once

#include "../frames/input.hpp"
#include "../frames/selector.hpp"
#include "../frames/text.hpp"
#include "../net/network_manager.hpp"
#include "flow.hpp"
#include <memory>
#include <string>

class WifiFlow : public Flow {
public:
    WifiFlow();
    ~WifiFlow();

    void start();

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

    // Accessors for individual values
    std::string getSelectedNetwork() const;
    std::string getPassword() const;

    // network availability callback
    void networkDiscovered(const WifiNetwork& network);

private:
    enum class State { SELECT_NETWORK, ENTER_PASSWORD, CONNECTIING };

    State currentState = State::SELECT_NETWORK;

    NetworkManagerClient nm;
    std::thread scanThread;

    std::unique_ptr<Selector> networkSelector;
    std::unique_ptr<TextInput> passwordInput;
    std::unique_ptr<Text> connectingFrame;

    std::string selectedNetwork;
    std::string password;
    bool done = false;
    int scanTimeout = 5; // seconds
};
