#include "images.hpp"

// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

ImageList::ImageList(const int logicalWidth, const int logicalHeight)
    : Frame(), wallpapers(), logicalWidth(logicalWidth), logicalHeight(logicalHeight) {}

void ImageList::addImages(const std::vector<Wallpaper>& newWallpapers) {
    pendingWallpapers.insert(pendingWallpapers.end(), newWallpapers.begin(), newWallpapers.end());
}

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
FrameResult ImageList::render() {

    // process any new wallpapers to load their textures on the main thread
    processPendingWallpapers();

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        navigate(-1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        navigate(1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (selectedIndex >= 0 && selectedIndex < wallpapers.size()) {
            return FrameResult::Submit(wallpapers[selectedIndex].path);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        return FrameResult::Cancel();
    }

    ImGuiIO& io = ImGui::GetIO();
    Vec2 viewportSize = getSize();

    float width = viewportSize.x;
    float height = viewportSize.y;

    // window padding
    float edge_padding = 20.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(edge_padding, edge_padding));

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    ImGui::Begin("Wallpapers",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    // image display area
    ImVec2 content_region = ImGui::GetContentRegionAvail();
    float image_area_height = content_region.y;

    // image size
    float image_height = 225; // image_area_height - 40.0f; // padding
    float image_width = 400;  // image_height * 0.75f;
    float spacing = 20.0f;
    float total_width_per_image = image_width + spacing;

    // smooth scroll to selected image
    float target_scroll = selectedIndex * total_width_per_image - (content_region.x - image_width) * 0.5f;
    scrollOffset += (target_scroll - scrollOffset) * 0.15f; // smooth interpolation

    std::lock_guard<std::mutex> lock(wallpapersMutex);

    if (textures.empty()) {
        ImGui::Text("Generating thumbnails...");
    } else {

        ImGui::BeginChild("ScrollRegion",
                          ImVec2(0, image_area_height),
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetScrollX(scrollOffset);

        // render images horizontally
        for (int i = 0; i < textures.size(); i++) {
            if (i > 0)
                ImGui::SameLine();

            ImGui::BeginGroup();

            // highlight selected image
            bool is_selected = (i == selectedIndex);

            if (is_selected) {
                ImVec2 p_min = ImGui::GetCursorScreenPos();
                ImVec2 p_max = ImVec2(p_min.x + image_width, p_min.y + image_height);
                ImU32 color = ImGui::GetColorU32(hoverColor);
                // draw on foreground layer to avoid child clipping
                ImGui::GetForegroundDrawList()->AddRect(p_min, p_max, color, 0.0f, 0, 4.0f);
            }

            // make images clickable
            ImGui::PushID(i);
            ImGui::Image((void*)(intptr_t)textures[i], ImVec2(image_width, image_height));
            ImGui::PopID();

            ImGui::EndGroup();

            // add spacing
            if (i < textures.size() - 1) {
                ImGui::SameLine(0.0f, spacing);
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();

    ImGui::PopStyleVar();
    return FrameResult::Continue();
}

void ImageList::processPendingWallpapers() {
    std::lock_guard<std::mutex> lock(wallpapersMutex);

    for (const auto& wallpaper : pendingWallpapers) {
        GLuint texture = LoadTextureFromFile(wallpaper.thumbnailPath.c_str());
        if (texture != 0) {
            textures.push_back(texture);
        } else {
            std::cerr << "Failed to load texture: " << wallpaper.thumbnailPath << std::endl;
            textures.push_back(0);
        }
        wallpapers.push_back(wallpaper);
    }

    pendingWallpapers.clear();
}

void ImageList::navigate(int direction) {
    if (textures.empty())
        return;
    selectedIndex += direction;
    if (selectedIndex < 0)
        selectedIndex = 0;
    if (selectedIndex >= textures.size())
        selectedIndex = textures.size() - 1;
}

Vec2 ImageList::getSize() {
    // 2/3 width, 1/2 height
    // return Vec2{, (float)logicalHeight / 0.5f};
    // float w = (float)logicalWidth * 0.666f;
    float w = (float)logicalWidth * 0.8;
    float edge_padding = 20.0f;    // padding we want on all sides
    float content_height = 225.0f; // height of the image area
    // add padding to width and height to account for the space we need
    return Vec2{w + (edge_padding * 2), content_height + (edge_padding * 2)};
}

// load image and create an OpenGL texture
GLuint ImageList::LoadTextureFromFile(const char* filename) {

    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);

    if (data == nullptr) {
        return 0;
    }

    // create OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // upload pixels to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    return texture;
}

void ImageList::applyTheme(const Config& config) { hoverColor = config.getColor("theme", "hover_color", "#3366B366"); }
