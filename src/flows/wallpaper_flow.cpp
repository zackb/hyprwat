#include "wallpaper_flow.hpp"

WallpaperFlow::WallpaperFlow(const std::string& dir) : wallpaperDir(dir) { imageList = std::make_unique<ImageList>(); }

WallpaperFlow::~WallpaperFlow() = default;

Frame* WallpaperFlow::getCurrentFrame() { return imageList.get(); }

bool WallpaperFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        finalResult = result.value;
        done = true;
        return false;
    } else if (result.action == FrameResult::Action::CANCEL) {
        done = true;
        return false;
    }
    return true;
}

bool WallpaperFlow::isDone() const { return done; }

std::string WallpaperFlow::getResult() const { return finalResult; }
