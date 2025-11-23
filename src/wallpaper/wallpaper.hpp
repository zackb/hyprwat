#pragma once

#include "thumbnail.hpp"
#include <set>
#include <string>
#include <vector>

std::string getCacheDir();

struct Wallpaper {
    std::string path;
    std::string thumbnailPath;
    std::filesystem::file_time_type modified;
};

class WallpaperManager {

public:
    WallpaperManager(const std::string& wallpaperDir) : wallpaperDir(wallpaperDir), thumbnailCache(getCacheDir()) {}
    void loadWallpapers();
    const std::vector<Wallpaper>& getWallpapers() const { return wallpapers; }

private:
    std::string wallpaperDir;
    std::vector<Wallpaper> wallpapers;
    ThumbnailCache thumbnailCache;
    const std::set<std::string> wallpaperExts = {".jpg", ".png", ".jpeg", ".bmp", ".gif"};
};
