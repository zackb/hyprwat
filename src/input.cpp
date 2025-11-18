#include "input.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

std::mutex Input::mutex;
ParseResult Input::parseArgv(int argc, const char* argv[]) {

    ParseResult result;

    if (argc <= 1) {
        return result;
    }

    // Check for --input mode
    if (std::string(argv[1]) == "--input" || std::string(argv[1]) == "--password") {
        result.mode = (std::string(argv[1]) == "--input") ? InputMode::INPUT : InputMode::PASSWORD;
        // If there's a second argument, use it as the hint
        if (argc > 2) {
            result.hint = argv[2];
        }
        return result;
    }

    if (std::string(argv[1]) == "--wifi") {
        result.mode = InputMode::WIFI;
        return result;
    }

    if (std::string(argv[1]) == "--audio") {
        result.mode = InputMode::AUDIO;
        return result;
    }

    if (std::string(argv[1]) == "--custom") {
        result.mode = InputMode::CUSTOM;
        if (argc > 2) {
            result.configPath = argv[2];
        }
        return result;
    }

    if (std::string(argv[1]) == "--wallpaper") {
        result.mode = InputMode::WALLPAPER;
        if (argc > 2) {
            result.wallpaperDir = Input::expandPath(argv[2]);
        } else {
            std::filesystem::path userWallpapers = Input::expandPath("~/.local/share/wallpapers");
            if (std::filesystem::exists(userWallpapers)) {
                result.wallpaperDir = userWallpapers.string();
                return result;
            } else if (std::filesystem::exists("/usr/share/wallpapers")) {
                result.wallpaperDir = "/usr/share/wallpapers";
                return result;
            }
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

std::filesystem::path Input::expandPath(const std::string& path) {
    if (path.empty())
        return path;

    // ~ expansion
    if (path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (!home) {
            throw std::runtime_error("HOME environment variable not set");
        }
        return std::filesystem::path(home) / path.substr(2);
    }

    if (path.find("$HOME") == 0) {
        const char* home = std::getenv("HOME");
        if (!home) {
            throw std::runtime_error("HOME environment variable not set");
        }
        return std::filesystem::path(home) / path.substr(6);
    }

    return path;
}
