#include "wallpaper.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::string getCacheDir() {
    if (const char* xdgCache = std::getenv("XDG_CACHE_HOME")) {
        return std::string(xdgCache) + "/hyprwat/wallpapers/";
    }

    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("Neither XDG_CACHE_HOME nor HOME is set");
    }

    return std::string(home) + "/.cache/hyprwat/wallpapers/";
}

void WallpaperManager::applyWallpaper(const std::string& wallpaperPath) {
    std::cout << "Applying wallpaper: " << wallpaperPath << '\n';
}

void WallpaperManager::loadWallpapers() {
    wallpapers.clear();
    try {
        for (const auto& entry : fs::recursive_directory_iterator(wallpaperDir)) {
            if (fs::is_regular_file(entry.path())) {
                // only add images
                if (wallpaperExts.contains(entry.path().extension().string())) {
                    // std::cout << "Adding wallpaper: " << entry.path() << '\n';
                    wallpapers.push_back({entry.path().string(),
                                          thumbnailCache.getOrCreateThumbnail(entry.path().string(), 400, 225),
                                          fs::last_write_time(entry.path())});
                }
            } else if (fs::is_directory(entry.path())) {
                // std::cout << "Directory: " << entry.path() << '\n';
            }
        }
        std::sort(
            wallpapers.begin(), wallpapers.end(), [](auto const& a, auto const& b) { return a.modified > b.modified; });
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
}
