#include "images.hpp"

// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

ImageList::ImageList(const std::vector<Wallpaper>& wallpapers, const int logicalWidth, const int logicalHeight)
    : Frame(), wallpapers(wallpapers), logicalWidth(logicalWidth), logicalHeight(logicalHeight) {
    // load textures
    for (const auto& wallpaper : wallpapers) {
        GLuint texture = LoadTextureFromFile(wallpaper.thumbnailPath.c_str());
        if (texture != 0) {
            textures.push_back(texture);
        } else {
            std::cerr << "Failed to load texture: " << wallpaper.thumbnailPath << std::endl;
            textures.push_back(0);
        }
    }
}

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
FrameResult ImageList::render() {

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

    ImGui::Begin("Wallpapers",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    ImGuiIO& io = ImGui::GetIO();
    Vec2 viewportSize = getSize();

    float width = viewportSize.x;
    float height = viewportSize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    // image display area
    ImVec2 content_region = ImGui::GetContentRegionAvail();
    float image_area_height = content_region.y;

    // image size
    float image_height = 200; // image_area_height - 40.0f; // padding
    float image_width = 400;  // image_height * 0.75f;
    float spacing = 20.0f;
    float total_width_per_image = image_width + spacing;

    // smooth scroll to selected image
    float target_scroll = selectedIndex * total_width_per_image - (content_region.x - image_width) * 0.5f;
    scrollOffset += (target_scroll - scrollOffset) * 0.15f; // smooth interpolation

    ImGui::BeginChild("ScrollRegion", ImVec2(0, image_area_height), false, ImGuiWindowFlags_NoScrollbar);

    ImGui::SetScrollX(scrollOffset);
    ImGui::SameLine(0.0f, spacing);
    ImGui::Dummy(ImVec2(1, 1));

    // render images horizontally
    for (int i = 0; i < textures.size(); i++) {
        if (i > 0)
            ImGui::SameLine();

        ImGui::BeginGroup();

        // Highlight selected image
        bool is_selected = (i == selectedIndex);

        if (is_selected) {
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + image_width, p_min.y + image_height);
            ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(100, 150, 255, 255), 0.0f, 0, 4.0f);
        }

        // make images clickable
        ImGui::PushID(i);
        ImGui::Image((void*)(intptr_t)textures[i], ImVec2(image_width, image_height));
        /*
        if (ImGui::ImageButton(
                wallpapers[i].path.c_str(), (void*)(intptr_t)textures[i], ImVec2(image_width, image_height))) {
            selectedIndex = i;
            // Confirm();
        }
        */
        ImGui::PopID();

        ImGui::EndGroup();

        // add spacing
        if (i < textures.size() - 1) {
            ImGui::SameLine(0.0f, spacing);
        }
    }

    ImGui::EndChild();

    ImGui::End();
    return FrameResult::Continue();
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
    float w = (float)logicalWidth * 0.666f;
    return Vec2{w, 220.0f};
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
