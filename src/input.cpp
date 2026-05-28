#include "input.hpp"
#include "src/debug/log.hpp"
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

    int argi = 1;
    if (std::string(argv[argi]) == "--config") {
        // --config and its value
        if (argc <= argi + 1) {
            debug::log(ERR, "--config flag requires a file path argument");
            exit(1);
        }
        argi += 2;
        result.configFile = argv[2];
    } else {
    }

    const char* arg = argv[argi];

    // Check for --input mode
    if (std::string(arg) == "--input" || std::string(arg) == "--password") {
        result.mode = (std::string(arg) == "--input") ? InputMode::INPUT : InputMode::PASSWORD;
        // If there's a second argument, use it as the hint
        if (argc > argi + 1) {
            result.hint = argv[argi + 1];
        }
        return result;
    }

    if (std::string(arg) == "--wifi") {
        result.mode = InputMode::WIFI;
        return result;
    }

    if (std::string(arg) == "--audio") {
        result.mode = InputMode::AUDIO;
        return result;
    }

    if (std::string(arg) == "--volume-up") {
        result.mode = InputMode::VOLUME_OSD;
        result.volumeAction = VolumeAction::UP;
        return result;
    }

    if (std::string(arg) == "--volume-down") {
        result.mode = InputMode::VOLUME_OSD;
        result.volumeAction = VolumeAction::DOWN;
        return result;
    }

    if (std::string(arg) == "--custom") {
        result.mode = InputMode::CUSTOM;
        if (argc > argi + 1) {
            result.configPath = argv[argi + 1];
        }
        return result;
    }

    if (std::string(arg) == "--overview") {
        result.mode = InputMode::OVERVIEW;
        return result;
    }

    if (std::string(arg) == "--wallpaper") {
        result.mode = InputMode::WALLPAPER;
        if (argc > argi + 1) {
            result.wallpaperDir = Input::expandPath(argv[argi + 1]);
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
    for (int i = argi; i < argc; ++i) {
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
