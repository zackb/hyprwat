#include "simple_flows.hpp"
#include "../frames/input.hpp"
#include "../frames/selector.hpp"

// SimpleMenuFlow implementation
SimpleMenuFlow::SimpleMenuFlow(const std::vector<Choice>& choices) {
    selector = std::make_unique<Selector>();
    for (auto& choice : choices) {
        selector->add(Choice{choice.id, choice.display, choice.selected});
    }
}

SimpleMenuFlow::~SimpleMenuFlow() = default;

Frame* SimpleMenuFlow::getCurrentFrame() { return selector.get(); }

bool SimpleMenuFlow::handleResult(const FrameResult& result) {
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

bool SimpleMenuFlow::isDone() const { return done; }

std::string SimpleMenuFlow::getResult() const { return finalResult; }

// SimpleInputFlow implementation
SimpleInputFlow::SimpleInputFlow(const std::string& hint) { input = std::make_unique<TextInput>(hint); }

SimpleInputFlow::~SimpleInputFlow() = default;

Frame* SimpleInputFlow::getCurrentFrame() { return input.get(); }

bool SimpleInputFlow::handleResult(const FrameResult& result) {
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

bool SimpleInputFlow::isDone() const { return done; }

std::string SimpleInputFlow::getResult() const { return finalResult; }

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
