#pragma once

#include "../frames/images.hpp"
#include "flow.hpp"

class WallpaperFlow : public Flow {
public:
    WallpaperFlow(const std::string& wallpaperDir, const int logicalWidth, const int logicalHeight);
    ~WallpaperFlow();

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    WallpaperManager wallpaperManager;
    std::unique_ptr<ImageList> imageList;
    std::string finalResult;
    bool done = false;
};
