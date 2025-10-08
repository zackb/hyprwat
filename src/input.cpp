#include "input.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

std::mutex Input::mutex;
ParseResult Input::parseArgv(int argc, const char* argv[]) {

    ParseResult result;

    if (argc <= 1) {
        return result;
    }

    // Check for --input mode
    if (std::string(argv[1]) == "--input") {
        result.mode = InputMode::INPUT;
        // If there's a second argument, use it as the hint
        if (argc > 2) {
            result.hint = argv[2];
        }
        return result;
    }

    // Default MENU mode - parse all arguments as choices
    result.mode = InputMode::MENU;
    for (int i = 1; i < argc; ++i) {
        result.choices.push_back(parseLine(argv[i]));
    }

    // Ensure only one item is selected
    bool anySelected =
        std::any_of(result.choices.begin(), result.choices.end(), [](const Choice& item) { return item.selected; });

    return result;
}

void Input::parseStdin(Callback callback) {

    std::thread inputThread([&]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }
            auto item = parseLine(line);
            {
                std::lock_guard<std::mutex> lock(mutex);
                callback(item);
            }
        }
    });

    inputThread.detach();
}

Choice Input::parseLine(std::string line) {
    Choice item;
    std::string s = line;

    if (!s.empty() && s.back() == '*') {
        item.selected = true;
        s.pop_back();
    }

    size_t colon = s.find(':');
    if (colon != std::string::npos) {
        item.id = s.substr(0, colon);
        item.display = s.substr(colon + 1);
    } else {
        item.id = s;
        item.display = s;
    }

    return item;
}
