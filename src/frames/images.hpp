#pragma once

#include "../ui.hpp"

class ImageList : public Frame {
public:
    ImageList() {}
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;

private:
};
