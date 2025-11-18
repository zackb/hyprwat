#pragma once

#include "../ui.hpp"
#include "../wallpaper/wallpaper.hpp"
#include <GL/gl.h>

class ImageList : public Frame {
public:
    ImageList(std::vector<Wallpaper>& wallpapers);
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;

private:
    int selectedIndex = 0;
    float scrollOffset = 0.0f;
    std::vector<GLuint> textures;
    std::vector<Wallpaper>& wallpapers;
    GLuint LoadTextureFromFile(const char* filename);
    void navigate(int direction);
};
