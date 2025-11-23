#include "custom_flow.hpp"
#include <cstring>

CustomFlow::CustomFlow(const std::string& rootConfigPath) { pushMenu(rootConfigPath); }

void CustomFlow::pushMenu(const std::string& configPath) {
    MenuLevel level;
    level.configPath = configPath;
    level.frame = std::make_unique<CustomFrame>(configPath);
    menuStack.push(std::move(level));
}

void CustomFlow::popMenu() {
    if (menuStack.size() > 1) {
        menuStack.pop();
    }
}

bool CustomFlow::isCustomAction(const std::string& result) const { return result.find("__SUBMENU__:") == 0; }

std::string CustomFlow::extractCustomPath(const std::string& result) const {
    if (isCustomAction(result)) {
        return result.substr(12); // skip "__SUBMENU__:"
    }
    return "";
}

Frame* CustomFlow::getCurrentFrame() {
    if (menuStack.empty()) {
        return nullptr;
    }
    return menuStack.top().frame.get();
}

bool CustomFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        std::string value = result.value;

        // check for special submenu navigation
        if (value == "__BACK__") {
            // go back to parent menu
            if (menuStack.size() > 1) {
                popMenu();
                return true; // continue with parent menu
            } else {
                // already at root treat as cancel
                done = true;
                return false;
            }
        } else if (isCustomAction(value)) {
            // navigate to submenu
            std::string submenuPath = extractCustomPath(value);

            // if path is relative, make it relative to current config's directory
            if (!submenuPath.empty() && submenuPath[0] != '/') {
                // extract directory from current config path
                std::string currentPath = menuStack.top().configPath;
                size_t lastSlash = currentPath.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    std::string dir = currentPath.substr(0, lastSlash + 1);
                    submenuPath = dir + submenuPath;
                }
            }

            pushMenu(submenuPath);
            return true; // continue with new submenu
        } else {
            // normal submit, we're done
            finalResult = value;
            done = true;
            return false;
        }
    } else if (result.action == FrameResult::Action::CANCEL) {
        // on cancel, go back if we're in a submenu, otherwise exit
        if (menuStack.size() > 1) {
            popMenu();
            return true; // continue with parent menu
        } else {
            // at root menu, cancel means exit
            done = true;
            return false;
        }
    }

    return true;
}

bool CustomFlow::isDone() const { return done; }

std::string CustomFlow::getResult() const { return finalResult; }
