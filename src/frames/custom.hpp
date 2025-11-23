#pragma once

#include "../config.hpp"
#include "../ui.hpp"
#include "imgui.h"
#include <memory>
#include <string>
#include <vector>

namespace YAML {
    class Node;
}

class CustomFrame : public Frame {
public:
    CustomFrame(const std::string& configPath);
    ~CustomFrame() override = default;

    FrameResult render() override;
    Vec2 getSize() override;
    void applyTheme(const Config& config) override;

private:
    // action types that can be triggered
    enum class ActionType {
        Execute, // execute shell command
        Submit,  // return value
        Cancel,  // close/cancel
        SubMenu, // navigate to submenu
        Back,    // go back to parent menu
        SetState // internal state change
    };

    struct Action {
        ActionType type;
        std::string command;        // Execute
        std::string value;          // Submit
        std::string submenu_path;   // SubMenu
        bool closeOnSuccess = true; // Execute

        enum TriggerType { OnClick, OnChange, OnRelease } trigger = OnClick;
    };

    struct Widget {
        std::string id;
        virtual ~Widget() = default;
        virtual FrameResult render(CustomFrame& frame) = 0;
        virtual float getHeight() const = 0;
    };

    // widget implementations
    struct TextWidget : Widget {
        std::string content;
        enum Style { Normal, Bold, Italic } style = Normal;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct SeparatorWidget : Widget {
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct ButtonWidget : Widget {
        std::string label;
        Action action;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct SelectableItem {
        std::string id;
        std::string label;
        bool selected;
        Action action;
    };

    struct SelectableListWidget : Widget {
        std::vector<SelectableItem> items;
        int selectedIndex = -1;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct InputWidget : Widget {
        std::string hint;
        std::string value;
        bool password = false;
        Action action;
        char buffer[256] = {0};
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct CheckboxWidget : Widget {
        std::string label;
        bool checked = false;
        Action action;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct SliderWidget : Widget {
        std::string label;
        float value = 0.0f;
        float minValue = 0.0f;
        float maxValue = 100.0f;
        Action action;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct ComboWidget : Widget {
        std::string label;
        std::vector<std::string> items;
        int currentIndex = 0;
        Action action;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    struct ColorPickerWidget : Widget {
        std::string label;
        float color[3] = {0.2f, 0.4f, 0.7f};
        Action action;
        FrameResult render(CustomFrame& frame) override;
        float getHeight() const override;
    };

    bool loadConfig(const std::string& path);
    std::unique_ptr<Widget> parseWidget(const YAML::Node& node);
    Action parseAction(const YAML::Node& node);
    FrameResult executeAction(const Action& action, const std::string& value = "");
    std::string replaceTokens(const std::string& str, const std::string& value);

    std::vector<std::unique_ptr<Widget>> widgets;
    std::string title;
    float fixedWidth = 0.0f;
    float fixedHeight = 0.0f;
    ImVec2 lastSize = ImVec2(400, 300);

    // theme colors
    ImVec4 hoverColor;
    ImVec4 activeColor;

    // keyboard shortcuts
    struct Shortcut {
        ImGuiKey key;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        Action action;
    };
    std::vector<Shortcut> shortcuts;

    bool firstFrame = true;
};
