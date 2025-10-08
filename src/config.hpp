#pragma once

#include <INIReader.h>
#include <filesystem>
#include <iostream>
#include <string>

#include "INIReader.h" // from inih
#include <imgui.h>     // for ImVec4
#include <stdexcept>
#include <string>

class Config {
public:
    explicit Config(const std::string& path) : reader(expandUser(path)) {
        if (reader.ParseError() != 0) {
            // file not found or parse error
            std::cerr << "Warning: Config file '" << path << "' not found or invalid. Using defaults.\n";
        }
    }

    std::string getString(const std::string& section, const std::string& name, const std::string& def = "") const {
        return expandUser(reader.Get(section, name, def)).string();
    }

    float getFloat(const std::string& section, const std::string& name, float def = 0.0f) const {
        return static_cast<float>(reader.GetReal(section, name, def));
    }

    ImVec4 getColor(const std::string& section, const std::string& name, const std::string& def = "#000000FF") const {
        std::string val = reader.Get(section, name, def);
        return parseColor(val);
    }

private:
    INIReader reader;

    static ImVec4 parseColor(const std::string& hex) {
        unsigned int r = 0, g = 0, b = 0, a = 255;
        std::string s = hex;
        if (!s.empty() && s[0] == '#')
            s.erase(0, 1);

        if (s.size() == 6) {
            sscanf(s.c_str(), "%02x%02x%02x", &r, &g, &b);
        } else if (s.size() == 8) {
            sscanf(s.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a);
        } else {
            throw std::runtime_error("Invalid color format: " + hex);
        }

        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    std::filesystem::path expandUser(const std::string& path) const {
        if (path.empty() || path[0] != '~')
            return path;

        const char* home = std::getenv("HOME");
        if (!home)
            throw std::runtime_error("Cannot expand '~': HOME not set");

        if (path.size() == 1)
            return home;

        return std::filesystem::path(home) / path.substr(2); // skip "~/"
    }
};
