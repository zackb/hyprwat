#pragma once

#include <INIReader.h>
#include <filesystem>
#include <string>

class Config {
public:
    explicit Config(const std::string& path);
    std::string getString(const std::string& section, const std::string& key, const std::string& def = "") const;
    int getInt(const std::string& section, const std::string& key, int def = 0) const;
    bool getBool(const std::string& section, const std::string& key, bool def = false) const;

    static std::filesystem::path expandUser(const std::string& path);

private:
    INIReader reader;
};
