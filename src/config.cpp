#include "config.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

Config::Config(const std::string& path) : reader(expandUser(path).string()) {
    if (reader.ParseError() < 0) {
        std::cerr << "Warning: could not load config file: " << path << "\n";
    }
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& def) const {
    return reader.Get(section, key, def);
}

int Config::getInt(const std::string& section, const std::string& key, int def) const {
    return reader.GetInteger(section, key, def);
}

bool Config::getBool(const std::string& section, const std::string& key, bool def) const {
    return reader.GetBoolean(section, key, def);
}

std::filesystem::path Config::expandUser(const std::string& path) {
    if (path.empty() || path[0] != '~')
        return path;

    const char* home = std::getenv("HOME");
    if (!home)
        throw std::runtime_error("Cannot expand '~': HOME not set");

    if (path.size() == 1)
        return home;

    return std::filesystem::path(home) / path.substr(2); // skip "~/"
}
