#pragma once

#include "../ui.hpp"
#include <string>

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
