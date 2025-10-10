#pragma once

#include "../audio/audio.hpp"
#include "../frames/selector.hpp"
#include "flow.hpp"
#include <memory>
#include <string>

class AudioFlow : public Flow {
public:
    AudioFlow();
    ~AudioFlow() override = default;

    void start();
    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    enum class State {
        SELECT_TYPE,   // input or output
        SELECT_DEVICE, // specific device
        DONE
    };

    AudioManagerClient audioManager;
    State currentState = State::SELECT_TYPE;
    bool done = false;

    std::unique_ptr<Selector> typeSelector;
    std::unique_ptr<Selector> deviceSelector;

    std::string selectedType; // "input" or "output"
    uint32_t selectedDeviceId = 0;

    void populateDeviceSelector();
};
