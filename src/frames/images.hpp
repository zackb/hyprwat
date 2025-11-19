#pragma once

#include "../ui.hpp"
#include "../wallpaper/wallpaper.hpp"
#include <GL/gl.h>

class ImageList : public Frame {
public:
    ImageList(const int logicalWidth, const int logicalHeight);
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;
    virtual void applyTheme(const Config& config) override;
    virtual bool shouldRepositionOnResize() const override { return false; }
    virtual bool shouldPositionAtCursor() const override { return false; }

    void addImages(const std::vector<Wallpaper>& wallpapers);

private:
    int selectedIndex = 0;
    float scrollOffset = 0.0f;
    int logicalWidth;
    int logicalHeight;
    std::vector<GLuint> textures;
    std::vector<Wallpaper> wallpapers;
    std::vector<Wallpaper> pendingWallpapers;
    std::mutex wallpapersMutex;
    ImVec4 hoverColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);

    void processPendingWallpapers();
    GLuint LoadTextureFromFile(const char* filename);
    void navigate(int direction);
};
