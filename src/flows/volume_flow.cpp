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
