#include "volume_flow.hpp"

VolumeFlow::VolumeFlow(VolumeAction action) { frame = std::make_unique<VolumeFrame>(audioManager, action); }

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
            frame->adjustVolume(0.05f);
        } else if (cmd == "down") {
            frame->adjustVolume(-0.05f);
        } else if (cmd == "mute") {
            frame->toggleMute();
        }
    }
}
