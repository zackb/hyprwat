#include "audio_flow.hpp"

AudioFlow::AudioFlow() {
    typeSelector = std::make_unique<Selector>();
    typeSelector->add(Choice{"input", "Input (Microphones)", false});
    typeSelector->add(Choice{"output", "Output (Speakers/Headphones)", false});
}

void AudioFlow::populateDeviceSelector() {
    deviceSelector = std::make_unique<Selector>();

    if (selectedType == "input") {
        auto devices = audioManager.listInputDevices();
        for (const auto& dev : devices) {
            std::string label = dev.description;
            deviceSelector->add(Choice{std::to_string(dev.id), label, dev.isDefault});
        }
    } else if (selectedType == "output") {
        auto devices = audioManager.listOutputDevices();
        for (const auto& dev : devices) {
            std::string label = dev.description;
            deviceSelector->add(Choice{std::to_string(dev.id), label, dev.isDefault});
        }
    }
}

Frame* AudioFlow::getCurrentFrame() {
    switch (currentState) {
    case State::SELECT_TYPE:
        return typeSelector.get();
    case State::SELECT_DEVICE:
        return deviceSelector.get();
    case State::DONE:
    default:
        return nullptr;
    }
}

bool AudioFlow::handleResult(const FrameResult& result) {
    switch (currentState) {
    case State::SELECT_TYPE:
        if (result.action == FrameResult::Action::SUBMIT) {
            selectedType = result.value;
            populateDeviceSelector();
            currentState = State::SELECT_DEVICE;
            return true;
        } else if (result.action == FrameResult::Action::CANCEL) {
            done = true;
            return false;
        }
        break;

    case State::SELECT_DEVICE:
        if (result.action == FrameResult::Action::SUBMIT) {
            selectedDeviceId = std::stoul(result.value);

            // Set the default device
            bool success = false;
            if (selectedType == "input") {
                success = audioManager.setDefaultInput(selectedDeviceId);
            } else if (selectedType == "output") {
                success = audioManager.setDefaultOutput(selectedDeviceId);
            }

            // TODO: Maybe show a confirmation/error frame?
            currentState = State::DONE;
            done = true;
            return false;
        } else if (result.action == FrameResult::Action::CANCEL) {
            // Go back to type selection
            currentState = State::SELECT_TYPE;
            return true;
        }
        break;

    case State::DONE:
        done = true;
        return false;
    }

    return true;
}

bool AudioFlow::isDone() const { return done; }

std::string AudioFlow::getResult() const {
    if (!selectedType.empty() && selectedDeviceId != 0) {
        return selectedType + ":" + std::to_string(selectedDeviceId);
    }
    return "";
}
