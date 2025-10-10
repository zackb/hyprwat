#include "simple_flows.hpp"
#include "../frames/input.hpp"
#include "../frames/selector.hpp"

MenuFlow::MenuFlow() { selector = std::make_unique<Selector>(); }

MenuFlow::MenuFlow(const std::vector<Choice>& choices) {
    selector = std::make_unique<Selector>();
    for (auto& choice : choices) {
        selector->add(Choice{choice.id, choice.display, choice.selected});
    }
}

MenuFlow::~MenuFlow() = default;

void MenuFlow::addChoice(const Choice& choice) { selector->add(choice); }

Frame* MenuFlow::getCurrentFrame() { return selector.get(); }

bool MenuFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        finalResult = result.value;
        done = true;
        return false;
    } else if (result.action == FrameResult::Action::CANCEL) {
        done = true;
        return false;
    }
    return true;
}

bool MenuFlow::isDone() const { return done; }

std::string MenuFlow::getResult() const { return finalResult; }

// SimpleInputFlow implementation
InputFlow::InputFlow(const std::string& hint, bool password) { input = std::make_unique<TextInput>(hint, password); }

InputFlow::~InputFlow() = default;

Frame* InputFlow::getCurrentFrame() { return input.get(); }

bool InputFlow::handleResult(const FrameResult& result) {
    if (result.action == FrameResult::Action::SUBMIT) {
        finalResult = result.value;
        done = true;
        return false;
    } else if (result.action == FrameResult::Action::CANCEL) {
        done = true;
        return false;
    }
    return true;
}

bool InputFlow::isDone() const { return done; }

std::string InputFlow::getResult() const { return finalResult; }

// MenuThenInputFlow implementation
MenuThenInputFlow::MenuThenInputFlow(const std::vector<Choice>& choices, const std::string& inputHintPrefix)
    : inputHintPrefix(inputHintPrefix) {
    selector = std::make_unique<Selector>();
    for (auto& choice : choices) {
        selector->add(Choice{choice.id, choice.display, choice.selected});
    }
}

Frame* MenuThenInputFlow::getCurrentFrame() {
    if (currentState == State::SELECTING) {
        return selector.get();
    } else if (currentState == State::INPUT) {
        return input.get();
    }
    return nullptr;
}

bool MenuThenInputFlow::handleResult(const FrameResult& result) {
    if (currentState == State::SELECTING) {
        if (result.action == FrameResult::Action::SUBMIT) {
            selectedItem = result.value;
            // Create input frame with hint based on selection
            input = std::make_unique<TextInput>(inputHintPrefix + " " + selectedItem);
            currentState = State::INPUT;
            return true;
        } else if (result.action == FrameResult::Action::CANCEL) {
            done = true;
            return false;
        }
    } else if (currentState == State::INPUT) {
        if (result.action == FrameResult::Action::SUBMIT) {
            inputValue = result.value;
            done = true;
            return false;
        } else if (result.action == FrameResult::Action::CANCEL) {
            // Go back to menu
            currentState = State::SELECTING;
            return true;
        }
    }
    return true;
}

bool MenuThenInputFlow::isDone() const { return done; }

std::string MenuThenInputFlow::getResult() const { return selectedItem + ":" + inputValue; }

std::string MenuThenInputFlow::getSelectedItem() const { return selectedItem; }

std::string MenuThenInputFlow::getInputValue() const { return inputValue; }
