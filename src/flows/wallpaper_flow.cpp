#include "wallpaper_flow.hpp"

// wallpaper selection flow
// logicalWidth and logicalHeight are the size of the display in logical pixels
WallpaperFlow::WallpaperFlow(hyprland::Control& hyprctl,
                             const std::string& dir,
                             const int logicalWidth,
                             const int logicalHeight)
    : wallpaperManager(dir), hyprctl(hyprctl) {

    imageList = std::make_unique<ImageList>(logicalWidth, logicalHeight);
}

WallpaperFlow::~WallpaperFlow() = default;

Frame* WallpaperFlow::getCurrentFrame() {
    if (!hasLoaded) {
        // generate thumbnails in background
        loadingThread = std::thread([this]() {
            const auto wallpapers = wallpaperManager.getWallpapers();
            imageList->addImages(wallpapers);
            hasLoaded = true;
        });
    }

    return imageList.get();
}

bool WallpaperFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        finalResult = result.value;
        done = true;
        hyprctl.setWallpaper(finalResult);
    } else if (result.action == FrameResult::Action::CANCEL) {
        done = true;
    }

    if (done) {
        if (loadingThread.joinable()) {
            loadingThread.join();
        }
        return false;
    }

    return true;
}

bool WallpaperFlow::isDone() const { return done; }

std::string WallpaperFlow::getResult() const { return finalResult; }
