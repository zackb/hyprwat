#pragma once

#include "../compositor/compositor.hpp"
#include "../frames/images.hpp"
#include "flow.hpp"
#include <atomic>
#include <thread>

class WallpaperFlow : public Flow {
public:
    WallpaperFlow(compositor::Compositor& comp,
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
    compositor::Compositor& comp;
    std::unique_ptr<ImageList> imageList;
    std::string finalResult;
    std::thread loadingThread;
    std::atomic<bool> loadingStarted{false};
    bool done = false;
};
