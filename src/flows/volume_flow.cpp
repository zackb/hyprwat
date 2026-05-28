#include "volume_flow.hpp"
#include "../debug/log.hpp"
#include <chrono>
#include <thread>

VolumeFlow::VolumeFlow(VolumeAction action) {
    int retries = 100; // wait up to 500ms
    debug::log(INFO, "[VolumeFlow] constructor started. Initial volume query: {}", audioManager.getVolume().volume);
    while (audioManager.getVolume().volume < 0.0f && retries > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        retries--;
    }
    debug::log(INFO,
               "[VolumeFlow] constructor finished. Retries left: {}, final volume query: {}",
               retries,
               audioManager.getVolume().volume);

    frame = std::make_unique<VolumeFrame>(audioManager, action);
}

Frame* VolumeFlow::getCurrentFrame() {
    if (done) {
        return nullptr;
    }
    return frame.get();
}

bool VolumeFlow::handleResult(const FrameResult& result) {
    done = true;
    return false;
}

bool VolumeFlow::isDone() const { return done; }

void VolumeFlow::handleCommand(const std::string& cmd) {
    if (frame) {
        if (cmd == "up") {
            frame->adjustVolume(VOLUME_DELTA);
        } else if (cmd == "down") {
            frame->adjustVolume(-VOLUME_DELTA);
        } else if (cmd == "mute") {
            frame->toggleMute();
        }
    }
}
