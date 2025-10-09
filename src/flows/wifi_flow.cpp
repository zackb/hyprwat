#include "wifi_flow.hpp"
#include "../input.hpp"

WifiFlow::WifiFlow() {}

void WifiFlow::networkDiscovered(const WifiNetwork& network) {
    if (networkSelector) {
        std::lock_guard<std::mutex> lock(Input::mutex);
        std::string label = network.ssid + " (" + std::to_string(network.strength) + "%)";
        networkSelector->add(Choice{network.ssid, label, false});
    }
}

Frame* WifiFlow::getCurrentFrame() {
    switch (currentState) {
    case State::SELECT_NETWORK:
        return networkSelector.get();
    case State::ENTER_PASSWORD:
        return passwordInput.get();
    default:
        return nullptr;
    }
}

bool WifiFlow::handleResult(const FrameResult& result) {
    switch (currentState) {
    case State::SELECT_NETWORK:
        if (result.action == FrameResult::Action::SUBMIT) {
            selectedNetwork = result.value;
            // Create password input with network name in hint
            passwordInput = std::make_unique<TextInput>("Passphrase for " + selectedNetwork, true);
            currentState = State::ENTER_PASSWORD;
            return true;
        } else if (result.action == FrameResult::Action::CANCEL) {
            done = true;
            return false;
        }
        break;

    case State::ENTER_PASSWORD:
        if (result.action == FrameResult::Action::SUBMIT) {
            password = result.value;
            done = true;
            return false;
        } else if (result.action == FrameResult::Action::CANCEL) {
            // Go back to network selection
            currentState = State::SELECT_NETWORK;
            return true;
        }
        break;
    }
    return true;
}

bool WifiFlow::isDone() const { return done; }

std::string WifiFlow::getResult() const {
    if (!selectedNetwork.empty() && !password.empty()) {
        return selectedNetwork + ":" + password;
    }
    return "";
}

std::string WifiFlow::getSelectedNetwork() const { return selectedNetwork; }

std::string WifiFlow::getPassword() const { return password; }
