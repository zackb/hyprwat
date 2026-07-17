#pragma once

#include "../vec.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace compositor {
    struct Workspace {
        int id;
        std::string name;
        std::string monitor;
        bool active = false;
    };

    struct Client {
        std::string address;
        std::string title;
        std::string class_;
        std::string initialClass;
        std::string initialTitle;
        int workspaceId = -1;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool mapped = false;
        bool hidden = false;
    };

    struct Monitor {
        int id;
        std::string name;
        int x;
        int y;
        int width;
        int height;
        float scale;
        bool focused;
    };

    // What hyprwat needs from a compositor. Only cursorPos/monitorAtCursor are used by every
    // mode; the rest serve --overview and --wallpaper.
    class Compositor {
    public:
        virtual ~Compositor() = default;

        // these are in fractional scale pixels
        virtual Vec2 cursorPos() = 0;
        virtual std::optional<Monitor> monitorAtCursor(const Vec2& cursor) = 0;

        virtual std::vector<Workspace> getWorkspaces() = 0;
        virtual std::vector<Client> getClients() = 0;
        virtual int getActiveWorkspaceId() = 0;
        virtual void dispatchWorkspace(int id) = 0;

        virtual void setWallpaper(const std::string& path) = 0;

        // --overview needs a per-window capture protocol; not every compositor has one
        virtual bool supportsOverview() const = 0;
    };

    // Returns nullptr when no supported compositor is running.
    std::unique_ptr<Compositor> detect();
} // namespace compositor
