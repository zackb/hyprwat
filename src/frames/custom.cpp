#include "custom.hpp"
#include <cstdlib>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

CustomFrame::CustomFrame(const std::string& configPath) : title("Custom Menu") {
    if (!loadConfig(configPath)) {
        // add error widget if config fails to load
        auto error_widget = std::make_unique<TextWidget>();
        error_widget->content = "Failed to load config: " + configPath;
        error_widget->style = TextWidget::Bold;
        widgets.push_back(std::move(error_widget));
    }
}

bool CustomFrame::loadConfig(const std::string& path) {
    try {
        YAML::Node config = YAML::LoadFile(path);

        // title
        if (config["title"]) {
            title = config["title"].as<std::string>();
        }

        // dimensions
        if (config["width"]) {
            fixedWidth = config["width"].as<float>();
        }
        if (config["height"]) {
            fixedHeight = config["height"].as<float>();
        }

        // sections/widgets
        if (config["sections"]) {
            for (const auto& section : config["sections"]) {
                auto widget = parseWidget(section);
                if (widget) {
                    widgets.push_back(std::move(widget));
                }
            }
        }

        // shortcuts
        if (config["shortcuts"]) {
            for (const auto& shortcut : config["shortcuts"]) {
                Shortcut sc;
                std::string keyStr = shortcut["key"].as<std::string>();

                // parse key combo
                if (keyStr.find("Ctrl+") == 0) {
                    sc.ctrl = true;
                    keyStr = keyStr.substr(5);
                }
                if (keyStr.find("Shift+") == 0) {
                    sc.shift = true;
                    keyStr = keyStr.substr(6);
                }
                if (keyStr.find("Alt+") == 0) {
                    sc.alt = true;
                    keyStr = keyStr.substr(4);
                }

                // map key string to ImGuiKey
                if (keyStr == "Escape")
                    sc.key = ImGuiKey_Escape;
                else if (keyStr == "Enter")
                    sc.key = ImGuiKey_Enter;
                else if (keyStr.length() == 1) {
                    char c = std::toupper(keyStr[0]);
                    sc.key = (ImGuiKey)(ImGuiKey_A + (c - 'A'));
                }

                sc.action = parseAction(shortcut["action"]);
                shortcuts.push_back(sc);
            }
        }

        return true;
    } catch (const std::exception& e) {
        debug::log(ERR, "Error loading custom config: %s", e.what());
        return false;
    }
}

std::unique_ptr<CustomFrame::Widget> CustomFrame::parseWidget(const YAML::Node& node) {
    std::string type = node["type"].as<std::string>();

    if (type == "text") {
        auto widget = std::make_unique<TextWidget>();
        widget->content = node["content"].as<std::string>();
        if (node["style"]) {
            std::string style = node["style"].as<std::string>();
            if (style == "bold")
                widget->style = TextWidget::Bold;
            else if (style == "italic")
                widget->style = TextWidget::Italic;
        }
        return widget;
    }

    if (type == "separator") {
        return std::make_unique<SeparatorWidget>();
    }

    if (type == "button") {
        auto widget = std::make_unique<ButtonWidget>();
        widget->label = node["label"].as<std::string>();
        widget->action = parseAction(node["action"]);
        return widget;
    }

    if (type == "selectable_list") {
        auto widget = std::make_unique<SelectableListWidget>();
        if (node["items"]) {
            int idx = 0;
            for (const auto& item : node["items"]) {
                SelectableItem si;
                si.id = item["id"].as<std::string>();
                si.label = item["label"].as<std::string>();
                si.selected = item["selected"] && item["selected"].as<bool>();
                si.action = parseAction(item["action"]);

                if (si.selected) {
                    widget->selectedIndex = idx;
                }

                widget->items.push_back(si);
                idx++;
            }
        }
        return widget;
    }

    if (type == "input") {
        auto widget = std::make_unique<InputWidget>();
        widget->id = node["id"].as<std::string>();
        widget->hint = node["hint"] ? node["hint"].as<std::string>() : "";
        widget->password = node["password"] && node["password"].as<bool>();
        widget->action = parseAction(node["action"]);
        return widget;
    }

    if (type == "checkbox") {
        auto widget = std::make_unique<CheckboxWidget>();
        widget->id = node["id"].as<std::string>();
        widget->label = node["label"].as<std::string>();
        widget->checked = node["default"] && node["default"].as<bool>();
        widget->action = parseAction(node["action"]);
        return widget;
    }

    if (type == "slider") {
        auto widget = std::make_unique<SliderWidget>();
        widget->id = node["id"].as<std::string>();
        widget->label = node["label"].as<std::string>();
        widget->minValue = node["min"] ? node["min"].as<float>() : 0.0f;
        widget->maxValue = node["max"] ? node["max"].as<float>() : 100.0f;
        widget->value = node["default"] ? node["default"].as<float>() : widget->minValue;
        widget->action = parseAction(node["action"]);
        return widget;
    }

    if (type == "combo") {
        auto widget = std::make_unique<ComboWidget>();
        widget->id = node["id"].as<std::string>();
        widget->label = node["label"].as<std::string>();
        if (node["items"]) {
            for (const auto& item : node["items"]) {
                widget->items.push_back(item.as<std::string>());
            }
        }
        widget->currentIndex = node["default"] ? node["default"].as<int>() : 0;
        widget->action = parseAction(node["action"]);
        return widget;
    }

    if (type == "color_picker") {
        auto widget = std::make_unique<ColorPickerWidget>();
        widget->id = node["id"].as<std::string>();
        widget->label = node["label"].as<std::string>();
        if (node["default"]) {
            // parse hex color
            std::string hex = node["default"].as<std::string>();
            if (hex[0] == '#' && hex.length() >= 7) {
                int r, g, b;
                sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
                widget->color[0] = r / 255.0f;
                widget->color[1] = g / 255.0f;
                widget->color[2] = b / 255.0f;
            }
        }
        widget->action = parseAction(node["action"]);
        return widget;
    }

    return nullptr;
}

CustomFrame::Action CustomFrame::parseAction(const YAML::Node& node) {
    Action action;

    if (!node || !node["type"]) {
        action.type = ActionType::Cancel;
        return action;
    }

    std::string typeStr = node["type"].as<std::string>();

    if (typeStr == "execute") {
        action.type = ActionType::Execute;
        action.command = node["command"].as<std::string>();
        action.closeOnSuccess = !node["close_on_success"] || node["close_on_success"].as<bool>();

        if (node["trigger"]) {
            std::string trigger = node["trigger"].as<std::string>();
            if (trigger == "on_change")
                action.trigger = Action::OnChange;
            else if (trigger == "on_release")
                action.trigger = Action::OnRelease;
        }
    } else if (typeStr == "submit") {
        action.type = ActionType::Submit;
        action.value = node["value"] ? node["value"].as<std::string>() : "";
    } else if (typeStr == "submenu") {
        action.type = ActionType::SubMenu;
        action.submenu_path = node["path"].as<std::string>();
    } else if (typeStr == "back") {
        action.type = ActionType::Back;
    } else if (typeStr == "cancel") {
        action.type = ActionType::Cancel;
    }

    return action;
}

std::string CustomFrame::replaceTokens(const std::string& str, const std::string& value) {
    std::string result = str;

    // replace {value}
    size_t pos = 0;
    while ((pos = result.find("{value}", pos)) != std::string::npos) {
        result.replace(pos, 7, value);
        pos += value.length();
    }

    // replace {index} for combo boxes
    pos = 0;
    while ((pos = result.find("{index}", pos)) != std::string::npos) {
        result.replace(pos, 7, value);
        pos += value.length();
    }

    // replace {state} for checkboxes (on/off)
    pos = 0;
    while ((pos = result.find("{state}", pos)) != std::string::npos) {
        result.replace(pos, 7, value);
        pos += value.length();
    }

    return result;
}

FrameResult CustomFrame::executeAction(const Action& action, const std::string& value) {
    switch (action.type) {
    case ActionType::Execute: {
        std::string cmd = replaceTokens(action.command, value);
        int result = system(cmd.c_str());
        if (action.closeOnSuccess && result == 0) {
            return FrameResult::Submit(cmd);
        }
        return FrameResult::Continue();
    }
    case ActionType::Submit:
        return FrameResult::Submit(value.empty() ? action.value : value);
    case ActionType::SubMenu:
        // a special marker for submenu
        return FrameResult::Submit("__SUBMENU__:" + action.submenu_path);
    case ActionType::Back:
        // special marker for going back
        return FrameResult::Submit("__BACK__");
    case ActionType::Cancel:
        return FrameResult::Cancel();
    default:
        return FrameResult::Continue();
    }
}

FrameResult CustomFrame::render() {
    // calculate desired size
    if (fixedWidth > 0 && fixedHeight > 0) {
        lastSize = ImVec2(fixedWidth, fixedHeight);
    } else {
        ImGuiStyle& style = ImGui::GetStyle();
        float totalHeight = style.WindowPadding.y * 2;
        float maxWidth = fixedWidth > 0 ? fixedWidth : 400.0f;

        for (const auto& widget : widgets) {
            totalHeight += widget->getHeight() + style.ItemSpacing.y;
        }

        lastSize = ImVec2(maxWidth, totalHeight);
    }

    // window to fill display
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin(title.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    // render all widgets
    FrameResult result = FrameResult::Continue();
    for (auto& widget : widgets) {
        FrameResult widgetResult = widget->render(*this);
        if (widgetResult.action != FrameResult::Action::CONTINUE) {
            result = widgetResult;
        }
    }

    // check shortcuts
    for (const auto& shortcut : shortcuts) {
        bool ctrlMatch = !shortcut.ctrl || ImGui::GetIO().KeyCtrl;
        bool shiftMatch = !shortcut.shift || ImGui::GetIO().KeyShift;
        bool altMatch = !shortcut.alt || ImGui::GetIO().KeyAlt;

        if (ctrlMatch && shiftMatch && altMatch && ImGui::IsKeyPressed(shortcut.key)) {
            result = executeAction(shortcut.action);
        }
    }

    ImGui::End();

    firstFrame = false;
    return result;
}

Vec2 CustomFrame::getSize() { return Vec2{lastSize.x, lastSize.y}; }

void CustomFrame::applyTheme(const Config& config) {
    hoverColor = config.getColor("theme", "hover_color", "#3366B3FF");
    activeColor = config.getColor("theme", "active_color", "#3366B366");
}

// widget implementations
FrameResult CustomFrame::TextWidget::render(CustomFrame& frame) {
    if (style == Bold) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    }
    ImGui::TextWrapped("%s", content.c_str());
    if (style == Bold) {
        ImGui::PopFont();
    }
    return FrameResult::Continue();
}

float CustomFrame::TextWidget::getHeight() const {
    ImVec2 text_size = ImGui::CalcTextSize(content.c_str());
    return text_size.y;
}

FrameResult CustomFrame::SeparatorWidget::render(CustomFrame& frame) {
    ImGui::Separator();
    return FrameResult::Continue();
}

float CustomFrame::SeparatorWidget::getHeight() const { return ImGui::GetStyle().ItemSpacing.y; }

FrameResult CustomFrame::ButtonWidget::render(CustomFrame& frame) {
    if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
        return frame.executeAction(action);
    }
    return FrameResult::Continue();
}

float CustomFrame::ButtonWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    return text_size.y + style.FramePadding.y * 2;
}

FrameResult CustomFrame::SelectableListWidget::render(CustomFrame& frame) {
    FrameResult result = FrameResult::Continue();
    for (size_t i = 0; i < items.size(); i++) {
        bool is_selected = (int)i == selectedIndex;
        if (ImGui::Selectable(items[i].label.c_str(), is_selected)) {
            selectedIndex = i;
            result = frame.executeAction(items[i].action, items[i].id);
        }
    }
    return result;
}

float CustomFrame::SelectableListWidget::getHeight() const {
    float height = 0;
    ImGuiStyle& style = ImGui::GetStyle();
    for (const auto& item : items) {
        ImVec2 text_size = ImGui::CalcTextSize(item.label.c_str());
        height += text_size.y + style.FramePadding.y * 2 + style.ItemSpacing.y;
    }
    return height;
}

FrameResult CustomFrame::InputWidget::render(CustomFrame& frame) {
    ImGui::SetNextItemWidth(-1);

    if (frame.firstFrame) {
        ImGui::SetKeyboardFocusHere();
    }

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (password) {
        flags |= ImGuiInputTextFlags_Password;
    }

    bool enter_pressed = ImGui::InputTextWithHint(("##" + id).c_str(), hint.c_str(), buffer, sizeof(buffer), flags);

    if (enter_pressed) {
        return frame.executeAction(action, std::string(buffer));
    }

    return FrameResult::Continue();
}

float CustomFrame::InputWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize("Ay");
    return text_size.y + style.FramePadding.y * 2;
}

FrameResult CustomFrame::CheckboxWidget::render(CustomFrame& frame) {
    if (ImGui::Checkbox(label.c_str(), &checked)) {
        return frame.executeAction(action, checked ? "on" : "off");
    }
    return FrameResult::Continue();
}

float CustomFrame::CheckboxWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    return text_size.y + style.FramePadding.y * 2;
}

FrameResult CustomFrame::SliderWidget::render(CustomFrame& frame) {
    ImGui::Text("%s", label.c_str());

    bool value_changed = ImGui::SliderFloat(("##" + id).c_str(), &value, minValue, maxValue);

    if (action.trigger == Action::OnChange && value_changed) {
        return frame.executeAction(action, std::to_string((int)value));
    }

    if (ImGui::IsItemDeactivatedAfterEdit() && action.trigger == Action::OnRelease) {
        return frame.executeAction(action, std::to_string((int)value));
    }

    return FrameResult::Continue();
}

float CustomFrame::SliderWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    return text_size.y * 2 + style.FramePadding.y * 2 + style.ItemSpacing.y;
}

FrameResult CustomFrame::ComboWidget::render(CustomFrame& frame) {
    ImGui::Text("%s", label.c_str());

    if (ImGui::BeginCombo(("##" + id).c_str(),
                          currentIndex >= 0 && currentIndex < items.size() ? items[currentIndex].c_str() : "")) {
        for (int i = 0; i < items.size(); i++) {
            bool is_selected = (currentIndex == i);
            if (ImGui::Selectable(items[i].c_str(), is_selected)) {
                currentIndex = i;
                ImGui::EndCombo();
                return frame.executeAction(action, std::to_string(i));
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return FrameResult::Continue();
}

float CustomFrame::ComboWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    return text_size.y * 2 + style.FramePadding.y * 2 + style.ItemSpacing.y;
}

FrameResult CustomFrame::ColorPickerWidget::render(CustomFrame& frame) {
    ImGui::Text("%s", label.c_str());

    bool color_chosen = false;

    bool valueChanged = ImGui::ColorEdit3(("##" + id).c_str(), color);
    if (action.trigger == Action::OnChange && valueChanged) {
        color_chosen = true;
    } else if (action.trigger == Action::OnRelease && ImGui::IsItemDeactivatedAfterEdit()) {
        color_chosen = true;
    }
    if (color_chosen) {
        // convert to hex
        char hex[8];
        snprintf(
            hex, sizeof(hex), "#%02X%02X%02X", (int)(color[0] * 255), (int)(color[1] * 255), (int)(color[2] * 255));
        return frame.executeAction(action, std::string(hex));
    }
    return FrameResult::Continue();
}

float CustomFrame::ColorPickerWidget::getHeight() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    return text_size.y * 3 + style.FramePadding.y * 2 + style.ItemSpacing.y;
}
