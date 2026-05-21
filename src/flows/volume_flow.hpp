#pragma once

#include "../frames/volume.hpp"
#include "flow.hpp"
#include <memory>

class VolumeFlow : public Flow {
public:
    VolumeFlow(VolumeAction action);
    ~VolumeFlow() override = default;

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;

    void handleCommand(const std::string& cmd);

private:
    AudioManagerClient audioManager;
    std::unique_ptr<VolumeFrame> frame;
    bool done = false;
};
