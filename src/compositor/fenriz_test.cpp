// Checks the fenriz snapshot parser against canned IPC lines, so it can be verified
// without a live compositor. See ../fenriz/docs/IPC.md for the format.
#include "fenriz.hpp"
#include <cassert>
#include <cstdio>

using namespace compositor;

static void testFullSnapshot() {
    // Note fenriz emits scale via std::to_string(float), hence "2.000000" not "2.0".
    auto snap = parseFenrizSnapshot(
        R"({"outputs":[{"name":"eDP-1","active":1,"focused":true,"x":0,"y":0,"width":2560,"height":1600,"scale":2.000000,"internal":true},)"
        R"({"name":"DP-1","active":3,"focused":false,"x":2560,"y":0,"width":1920,"height":1080,"scale":1.000000,"internal":false}],)"
        R"("lid":"open","cursor":{"x":100,"y":200},)"
        R"("workspaces":{"active":1,"occupied":[1,2,4]},"activeWindow":{"appId":"foot","title":"~"}})");

    assert(snap.hasCursor);
    assert(snap.cursor.x == 100 && snap.cursor.y == 200);

    assert(snap.monitors.size() == 2);
    assert(snap.monitors[0].name == "eDP-1");
    assert(snap.monitors[0].width == 2560 && snap.monitors[0].height == 1600);
    assert(snap.monitors[0].scale == 2.0f);
    assert(snap.monitors[0].focused);
    assert(snap.monitors[1].x == 2560);
    assert(!snap.monitors[1].focused);

    assert(snap.activeWorkspace == 1);
    assert(snap.workspaces.size() == 3);
    assert(snap.workspaces[0].id == 1 && snap.workspaces[0].active);
    assert(snap.workspaces[1].id == 2 && !snap.workspaces[1].active);
    assert(snap.workspaces[2].id == 4);
}

// fenriz reports active:0 when no output is focused; hyprwat's contract is -1.
static void testNoFocusedOutput() {
    auto snap = parseFenrizSnapshot(
        R"({"outputs":[],"lid":"closed","cursor":{"x":0,"y":0},"workspaces":{"active":0,"occupied":[]},"activeWindow":null})");
    assert(snap.activeWorkspace == -1);
    assert(snap.workspaces.empty());
    assert(snap.monitors.empty());
}

// A fenriz without the cursor patch must be detected, not silently read as 0,0.
static void testMissingCursor() {
    auto snap = parseFenrizSnapshot(
        R"({"outputs":[],"lid":"open","workspaces":{"active":1,"occupied":[1]},"activeWindow":null})");
    assert(!snap.hasCursor);
}

int main() {
    testFullSnapshot();
    testNoFocusedOutput();
    testMissingCursor();
    printf("fenriz parser tests passed\n");
    return 0;
}
