#include "thumbnail.hpp"

#include <iostream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace fs = std::filesystem;

void ThumbnailCache::go() { std::cout << hashFileKey("/home/zackb/Downloads/foo.bar") << std::endl; }

int ThumbnailCache::hashFileKey(std::string&& path) {

    fs::path file(path);
    if (!fs::exists(file)) {
        std::cerr << "File does not exist: " << path << std::endl;
        return 0;
    }

    // WTF: auto ftime = fs::last_write_time(file).time_since_epoch().count();
    // auto ftime =
    // std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(file).time_since_epoch()).count();
    // auto filetime = fs::last_write_time(file);
    // std::time_t ftime = std::chrono::system_clock::to_time_t(filetime);

    auto ftime = fileLastWriteTime(file);
    auto fsize = fs::file_size(file);
    auto fname = file.string();

    std::cout << "File: " << fname << ", Size: " << fsize << ", Last Write Time: " << ftime << std::endl;

    std::hash<std::string> str_hash;
    size_t h1 = str_hash(fname);
    size_t h2 = std::hash<uintmax_t>{}(fsize);
    size_t h3 = std::hash<decltype(ftime)>{}(ftime);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

// unbelievable
uint64_t ThumbnailCache::fileLastWriteTime(const fs::path& file) {
    auto ftime_fs = fs::last_write_time(file);

    // convert file_time_type to system_clock::time_point
    auto ftime_sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime_fs - fs::file_time_type::clock::now() + std::chrono::system_clock::now());

    // seconds since epoch
    return std::chrono::duration_cast<std::chrono::seconds>(ftime_sctp.time_since_epoch()).count();
}

bool ThumbnailCache::resizeImage(std::string inPath, std::string outPath, int newW, int newH) {
    int w, h, ch;
    unsigned char* input = stbi_load(inPath.c_str(), &w, &h, &ch, 0);
    if (!input) {
        return false;
    }

    std::vector<unsigned char> output(newW * newH * ch);

    int inputStride = w * ch;
    int outputStride = newW * ch;

    stbir_pixel_layout layout;
    switch (ch) {
    case 1:
        layout = STBIR_1CHANNEL;
        break;
    case 2:
        layout = STBIR_2CHANNEL;
        break;
    case 3:
        layout = STBIR_RGB;
        break;
    case 4:
        layout = STBIR_RGBA;
        break;
    default:
        layout = STBIR_RGB; // fallback
        break;
    }

    // perform resizing using the srgb-aware function
    unsigned char* result =
        stbir_resize_uint8_srgb(input, w, h, inputStride, output.data(), newW, newH, outputStride, layout);

    if (result == nullptr) {
        // resizing failed
        stbi_image_free(input);
        return false;
    }

    // save as png
    stbi_write_png(outPath.c_str(), newW, newH, ch, output.data(), outputStride);

    stbi_image_free(input);
    return true;
};
