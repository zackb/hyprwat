#pragma once

#include <filesystem>
#include <string>

class ThumbnailCache {
public:
    ThumbnailCache(const std::string& cacheDir) : filepath_(cacheDir) { std::filesystem::create_directories(cacheDir); }
    std::string getOrCreateThumbnail(const std::string& imagePath, int width, int height);

private:
    std::string filepath_;
    int hashFileKey(std::string&& path);
    bool resizeImage(std::string inPath, std::string outPath, int newW, int newH);
    uint64_t fileLastWriteTime(const std::filesystem::path& file);
};
