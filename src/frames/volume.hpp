#pragma once

#include "../audio/audio.hpp"
#include "../input.hpp"
#include "../ui.hpp"
#include <chrono>
#include <mutex>

class VolumeFrame : public Frame {
public:
    VolumeFrame(AudioManagerClient& audioManager, VolumeAction initialAction);
    ~VolumeFrame() override = default;

    FrameResult render() override;
    Vec2 getSize() override { return Vec2{280.0f, 95.0f}; }

    bool shouldRepositionOnResize() const override { return false; }
    bool shouldPositionAtCursor() const override { return false; }
    void applyTheme(const Config& config) override;

    void adjustVolume(float delta);
    void toggleMute();
    std::mutex frameMutex;

private:
    AudioManagerClient& audioManager;
    std::chrono::steady_clock::time_point lastUpdateTime;
    float baseAlpha = 0.95f;
    bool initialAdjustDone = false;
    VolumeAction initialAction;
};
