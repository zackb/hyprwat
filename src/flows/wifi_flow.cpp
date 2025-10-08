#include "wifi_flow.hpp"
#include "../frames/selector.hpp"
#include "../frames/input.hpp"
#include "../choice.hpp"

WiFiFlow::WiFiFlow(const std::vector<std::string>& networks) {
    networkSelector = std::make_unique<Selector>();
    for (const auto& network : networks) {
        networkSelector->add(Choice{network, network, false});
    }
}

Frame* WiFiFlow::getCurrentFrame() {
    switch (currentState) {
    case State::SELECT_NETWORK:
        return networkSelector.get();
    case State::ENTER_PASSWORD:
        return passwordInput.get();
    default:
        return nullptr;
    }
}

bool WiFiFlow::handleResult(const FrameResult& result) {
    switch (currentState) {
    case State::SELECT_NETWORK:
        if (result.action == FrameResult::Action::SUBMIT) {
            selectedNetwork = result.value;
            // Create password input with network name in hint
            passwordInput = std::make_unique<TextInput>("Password for " + selectedNetwork);
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

bool WiFiFlow::isDone() const {
    return done;
}

std::string WiFiFlow::getResult() const {
    if (!selectedNetwork.empty() && !password.empty()) {
        return selectedNetwork + ":" + password;
    }
    return "";
}

std::string WiFiFlow::getSelectedNetwork() const {
    return selectedNetwork;
}

std::string WiFiFlow::getPassword() const {
    return password;
}
