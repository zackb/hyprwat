#include "images.hpp"

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
FrameResult ImageList::render() { return FrameResult::Continue(); }

Vec2 ImageList::getSize() { return Vec2(400, 300); }
