#pragma once

#include "../frames/images.hpp"
#include "../hyprland/ipc.hpp"
#include "flow.hpp"

class WallpaperFlow : public Flow {
public:
    WallpaperFlow(hyprland::Control& hyprctl,
                  const std::string& wallpaperDir,
                  const int logicalWidth,
                  const int logicalHeight);
    ~WallpaperFlow();

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    WallpaperManager wallpaperManager;
    hyprland::Control& hyprctl;
    std::unique_ptr<ImageList> imageList;
    std::string finalResult;
    std::thread loadingThread;
    std::atomic<bool> loadingStarted{false};
    bool done = false;
};
