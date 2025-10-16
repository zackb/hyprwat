#pragma once

#include "choice.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

enum class InputMode {
    MENU,     // default menu selection mode
    INPUT,    // text input mode
    PASSWORD, // password input mode
    WIFI,     // wifi selection + password mode
    AUDIO,    // audio input/output selection mode
    CUSTOM,   // custom menu from config file
};

struct ParseResult {
    InputMode mode = InputMode::MENU;
    std::vector<Choice> choices;
    std::string hint;       // For INPUT mode only
    std::string configPath; // For CUSTOM mode only
};

class Input {

public:
    using Callback = std::function<void(Choice)>;
    static ParseResult parseArgv(int argc, const char* argv[]);
    static void parseStdin(Callback callback);
    static std::mutex mutex;

private:
    static Choice parseLine(std::string line);
};
