#pragma once

#include "../frames/custom.hpp"
#include "flow.hpp"
#include <memory>
#include <stack>
#include <string>

class CustomFlow : public Flow {
public:
    explicit CustomFlow(const std::string& rootConfigPath);
    ~CustomFlow() override = default;

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    struct MenuLevel {
        std::unique_ptr<CustomFrame> frame;
        std::string configPath;
        std::string title;
    };

    // Stack of menu levels (allows back navigation)
    std::stack<MenuLevel> menuStack;

    // Current state
    bool done = false;
    std::string finalResult;

    // Helper methods
    void pushMenu(const std::string& configPath);
    void popMenu();
    bool isCustomAction(const std::string& result) const;
    std::string extractCustomPath(const std::string& result) const;
};
