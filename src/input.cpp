#include "input.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

std::mutex Input::mutex;
std::vector<Choice> Input::parseArgv(int argc, const char* argv[]) {

    std::vector<Choice> items;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            items.push_back(parseLine(argv[i]));
        }
    }

    // Ensure only one item is selected
    bool anySelected = std::any_of(items.begin(), items.end(), [](const Choice& item) { return item.selected; });

    /*
    if (!anySelected && !items.empty()) {
        items[0].selected = true; // Default to the first
    }
    */

    return items;
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
