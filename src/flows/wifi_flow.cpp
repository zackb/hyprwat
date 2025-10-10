#include "wifi_flow.hpp"
#include "../input.hpp"

// initializes frames
WifiFlow::WifiFlow() { networkSelector = std::make_unique<Selector>(); }

WifiFlow::~WifiFlow() {
    if (scanThread.joinable()) {
        scanThread.join();
    }
}

// starts scanning for networks
void WifiFlow::start() {
    // load known networks
    std::vector<WifiNetwork> knownNets = nm.listWifiNetworks();
    for (const auto& net : knownNets) {
        networkDiscovered(net);
    }
    // scan for networks and add them
    scanThread =
        std::thread([this]() { nm.scanWifiNetworks([this](const WifiNetwork& net) { networkDiscovered(net); }, 5); });
}

void WifiFlow::networkDiscovered(const WifiNetwork& network) {
    if (!networkSelector)
        return;

    std::string display = network.ssid + " (" + std::to_string(network.strength) + "%)";
    if (auto* existing = networkSelector->findChoiceById(network.ssid)) {
        if (network.strength > existing->strength) {
            existing->strength = network.strength;
            existing->display = display;
        }
    } else {
        networkSelector->add(Choice{network.ssid, display, false, network.strength});
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
            // TODO: perhaps another frame to show progress and handle errors
            nm.connectToNetwork(selectedNetwork, password);
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
