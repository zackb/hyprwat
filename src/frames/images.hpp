#pragma once

#include "../ui.hpp"
#include "../wallpaper/wallpaper.hpp"
#include <GL/gl.h>

class ImageList : public Frame {
public:
    ImageList(const std::vector<Wallpaper>& wallpapers, const int logicalWidth, const int logicalHeight);
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;
    virtual void applyTheme(const Config& config) override;

private:
    int selectedIndex = 0;
    float scrollOffset = 0.0f;
    int logicalWidth;
    int logicalHeight;
    std::vector<GLuint> textures;
    const std::vector<Wallpaper>& wallpapers;
    ImVec4 hoverColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    GLuint LoadTextureFromFile(const char* filename);
    void navigate(int direction);
};
