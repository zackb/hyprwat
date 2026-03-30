#include "overview_flow.hpp"
#include "../frames/overview.hpp"
#include "../wayland/display.hpp"

OverviewFlow::OverviewFlow(hyprland::Control& hyprctl, wl::Display& wlDisplay, int logicalWidth, int logicalHeight)
    : hyprctl(hyprctl), logicalWidth(logicalWidth), logicalHeight(logicalHeight) {
    mainFrame = std::make_unique<OverviewFrame>(hyprctl, wlDisplay, logicalWidth, logicalHeight);
}

OverviewFlow::~OverviewFlow() = default;

Frame* OverviewFlow::getCurrentFrame() { return mainFrame.get(); }

bool OverviewFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        finalResult = result.value;
        done = true;

        // dispatch to selected workspace
        int workspaceId = std::stoi(finalResult);
        hyprctl.dispatchWorkspace(workspaceId);
    } else if (result.action == FrameResult::Action::CANCEL) {
        done = true;
    }

    return !done;
}

bool OverviewFlow::isDone() const { return done; }

std::string OverviewFlow::getResult() const { return finalResult; }
