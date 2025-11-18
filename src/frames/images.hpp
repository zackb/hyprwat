#pragma once

#include "../ui.hpp"
#include "../wallpaper/wallpaper.hpp"
#include <GL/gl.h>

class ImageList : public Frame {
public:
    ImageList(const std::vector<Wallpaper>& wallpapers, const int logicalWidth, const int logicalHeight);
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;

private:
    int selectedIndex = 0;
    float scrollOffset = 0.0f;
    int logicalWidth;
    int logicalHeight;
    std::vector<GLuint> textures;
    const std::vector<Wallpaper>& wallpapers;
    GLuint LoadTextureFromFile(const char* filename);
    void navigate(int direction);
};
