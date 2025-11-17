#pragma once

#include "thumbnail.hpp"
#include <set>
#include <string>
#include <vector>

std::string getCacheDir();

struct Wallpaper {
    std::string path;
    std::string thumbnailPath;
};

class WallpaperManager {

public:
    WallpaperManager(const std::string& wallpaperDir) : wallpaperDir(wallpaperDir), thumbnailCache(getCacheDir()) {
        loadWallpapers();
    }
    void applyWallpaper(const std::string& wallpaperPath);

private:
    std::string wallpaperDir;
    std::vector<Wallpaper> wallpapers;
    ThumbnailCache thumbnailCache;
    const std::set<std::string> wallpaperExts = {".jpg", ".png", ".jpeg", ".bmp", ".gif"};

    void loadWallpapers();
};
