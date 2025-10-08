#pragma once

#include "../ui.hpp"
#include <string>

// result returned by a frame after user interaction
struct FrameResult {
    enum class Action {
        CONTINUE, // Keep showing this frame
        SUBMIT,   // User submitted (Enter pressed, item selected)
        CANCEL    // User cancelled (ESC pressed)
    };

    Action action = Action::CONTINUE;
    std::string value; // The submitted value (selected id, input text, password, etc.)

    static FrameResult Continue() { return {Action::CONTINUE, ""}; }
    static FrameResult Submit(const std::string& val) { return {Action::SUBMIT, val}; }
    static FrameResult Cancel() { return {Action::CANCEL, ""}; }
};

// Base class for multi-step flows
class Flow {
public:
    virtual ~Flow() = default;

    // Get the current frame to display
    virtual Frame* getCurrentFrame() = 0;

    // Handle the result from the current frame
    // Returns true if flow should continue, false if done
    virtual bool handleResult(const FrameResult& result) = 0;

    // Check if flow is complete
    virtual bool isDone() const = 0;

    // Get the final result of the flow
    virtual std::string getResult() const { return ""; }
};
