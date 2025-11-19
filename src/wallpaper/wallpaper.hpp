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
    WallpaperManager(const std::string& wallpaperDir) : wallpaperDir(wallpaperDir), thumbnailCache(getCacheDir()) {}
    void applyWallpaper(const std::string& wallpaperPath);
    void loadWallpapers();
    const std::vector<Wallpaper>& getWallpapers() const { return wallpapers; }

private:
    std::string wallpaperDir;
    std::vector<Wallpaper> wallpapers;
    ThumbnailCache thumbnailCache;
    const std::set<std::string> wallpaperExts = {".jpg", ".png", ".jpeg", ".bmp", ".gif"};
};
