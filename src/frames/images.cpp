#include "images.hpp"
#include "imgui.h"

// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

ImageList::ImageList(const int logicalWidth, const int logicalHeight)
    : Frame(), wallpapers(), logical_width(logicalWidth), logical_height(logicalHeight) {}

void ImageList::addImages(const std::vector<Wallpaper>& newWallpapers) {
    pending_wallpapers.insert(pending_wallpapers.end(), newWallpapers.begin(), newWallpapers.end());
}

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
FrameResult ImageList::render() {

    // process any new wallpapers to load their textures on the main thread
    processPendingWallpapers();

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_H)) {
        navigate(-1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_L)) {
        navigate(1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (selected_index >= 0 && selected_index < wallpapers.size()) {
            return FrameResult::Submit(wallpapers[selected_index].path);
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
    float target_scroll = selected_index * total_width_per_image - (content_region.x - image_width) * 0.5f;
    scroll_offset += (target_scroll - scroll_offset) * 0.15f; // smooth interpolation

    std::lock_guard<std::mutex> lock(wallpapers_mutex);

    if (textures.empty()) {
        ImGui::SetWindowFontScale(2.0f);

        const char* text = "Generating thumbnails...";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
        ImGui::Text("%s", text);

        ImGui::SetWindowFontScale(1.0f);
    } else {

        ImGui::BeginChild("ScrollRegion",
                          ImVec2(0, image_area_height),
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetScrollX(scroll_offset);

        // render images horizontally
        for (int i = 0; i < textures.size(); i++) {
            if (i > 0)
                ImGui::SameLine();

            ImGui::BeginGroup();

            // highlight selected image
            bool is_selected = (i == selected_index);

            if (is_selected) {
                ImVec2 p_min = ImGui::GetCursorScreenPos();
                ImVec2 p_max = ImVec2(p_min.x + image_width, p_min.y + image_height);
                ImU32 color = ImGui::GetColorU32(hover_color);
                // draw on foreground layer to avoid child clipping
                ImGui::GetForegroundDrawList()->AddRect(p_min, p_max, color, image_rounding, 0, 4.0f);
            }

            // make images clickable
            ImGui::PushID(i);
            // ImGui::Image((void*)(intptr_t)textures[i], ImVec2(image_width, image_height));
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + image_width, p_min.y + image_height);
            ImGui::GetWindowDrawList()->AddImageRounded(
                (void*)(intptr_t)textures[i], p_min, p_max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, image_rounding);
            ImGui::Dummy(ImVec2(image_width, image_height));
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
    std::lock_guard<std::mutex> lock(wallpapers_mutex);

    for (const auto& wallpaper : pending_wallpapers) {
        GLuint texture = LoadTextureFromFile(wallpaper.thumbnailPath.c_str());
        if (texture != 0) {
            textures.push_back(texture);
        } else {
            std::cerr << "Failed to load texture: " << wallpaper.thumbnailPath << std::endl;
            textures.push_back(0);
        }
        wallpapers.push_back(wallpaper);
    }

    pending_wallpapers.clear();
}

void ImageList::navigate(int direction) {
    if (textures.empty())
        return;
    selected_index += direction;
    if (selected_index < 0)
        selected_index = 0;
    if (selected_index >= textures.size())
        selected_index = textures.size() - 1;
}

Vec2 ImageList::getSize() {
    float w = (float)logical_width * width_ratio;
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

void ImageList::applyTheme(const Config& config) {
    hover_color = config.getColor("theme", "hover_color", "#3366B366");
    image_rounding = config.getFloat("theme", "frame_rounding", 8.0);
    width_ratio = config.getFloat("theme", "wallpaper_width_ratio", 0.8f);
}
