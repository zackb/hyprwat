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
    int selected_index = 0;
    float scroll_offset = 0.0f;
    int logical_width;
    int logical_height;
    float image_rounding = 8;
    float width_ratio = 0.8f;
    std::vector<GLuint> textures;
    std::vector<Wallpaper> wallpapers;
    std::vector<Wallpaper> pending_wallpapers;
    std::mutex wallpapers_mutex;
    ImVec4 hover_color = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);

    void processPendingWallpapers();
    GLuint LoadTextureFromFile(const char* filename);
    void navigate(int direction);
};
