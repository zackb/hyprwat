#pragma once

#include "compositor.hpp"

namespace compositor {

    // fenriz pushes a full state snapshot as one NDJSON line; it has no request/reply.
    struct FenrizSnapshot {
        Vec2 cursor{0, 0};
        bool hasCursor = false;
        std::vector<Monitor> monitors;
        std::vector<Workspace> workspaces;
        int activeWorkspace = -1;
    };

    // Split from the socket read so it can be tested without a live compositor.
    FenrizSnapshot parseFenrizSnapshot(const std::string& line);

    class Fenriz : public Compositor {
    public:
        explicit Fenriz();
        explicit Fenriz(const std::string& socketPath);

        Vec2 cursorPos() override;
        std::optional<Monitor> monitorAtCursor(const Vec2& cursor) override;
        std::vector<Workspace> getWorkspaces() override;
        std::vector<Client> getClients() override;
        int getActiveWorkspaceId() override;
        void dispatchWorkspace(int id) override;
        void setWallpaper(const std::string& path) override;
        bool supportsOverview() const override;

    private:
        std::string socketPath;
        FenrizSnapshot snapshot;
    };
} // namespace compositor
