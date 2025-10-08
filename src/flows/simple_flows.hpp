#pragma once

#include "flow.hpp"
#include <memory>
#include <string>
#include <vector>

class Selector;
class TextInput;
struct Choice;

// flow that shows a menu and exits with the selected item
class SimpleMenuFlow : public Flow {
public:
    SimpleMenuFlow(const std::vector<Choice>& choices);
    ~SimpleMenuFlow();

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    std::unique_ptr<Selector> selector;
    bool done = false;
    std::string finalResult;
};

// flow that shows a text input and exits with the entered text
class SimpleInputFlow : public Flow {
public:
    SimpleInputFlow(const std::string& hint);
    ~SimpleInputFlow();

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

private:
    std::unique_ptr<TextInput> input;
    bool done = false;
    std::string finalResult;
};

// flow that shows a menu, then prompts for input based on selection
class MenuThenInputFlow : public Flow {
public:
    MenuThenInputFlow(const std::vector<Choice>& choices, const std::string& inputHintPrefix = "Enter value for");

    Frame* getCurrentFrame() override;
    bool handleResult(const FrameResult& result) override;
    bool isDone() const override;
    std::string getResult() const override;

    std::string getSelectedItem() const;
    std::string getInputValue() const;

private:
    enum class State { SELECTING, INPUT };
    State currentState = State::SELECTING;

    std::unique_ptr<Selector> selector;
    std::unique_ptr<TextInput> input;

    std::string inputHintPrefix;
    std::string selectedItem;
    std::string inputValue;
    bool done = false;
};
