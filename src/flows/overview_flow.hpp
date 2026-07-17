#pragma once

#include "../compositor/compositor.hpp"
#include "../ui.hpp"
#include "flow.hpp"

namespace wl {
    class Display;
}

class OverviewFlow : public Flow {
public:
    OverviewFlow(compositor::Compositor& comp, wl::Display& wlDisplay, int logicalWidth, int logicalHeight);
    ~OverviewFlow() override;

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    compositor::Compositor& comp;
    int logicalWidth;
    int logicalHeight;
    std::unique_ptr<Frame> mainFrame;
    bool done = false;
    std::string finalResult;
};
