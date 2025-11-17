#pragma once

#include <filesystem>
#include <string>

class ThumbnailCache {
public:
    ThumbnailCache(const std::string& cacheDir) : filepath_(cacheDir) {}
    void go();

private:
    std::string filepath_;
    int hashFileKey(std::string&& path);
    bool resizeImage(std::string inPath, std::string outPath, int newW, int newH);
    uint64_t fileLastWriteTime(const std::filesystem::path& file);
};
