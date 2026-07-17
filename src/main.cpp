#include "compositor/compositor.hpp"
#include "flows/audio_flow.hpp"
#include "flows/custom_flow.hpp"
#include "flows/flow.hpp"
#include "flows/overview_flow.hpp"
#include "flows/simple_flows.hpp"
#include "flows/volume_flow.hpp"
#include "flows/wallpaper_flow.hpp"
#include "flows/wifi_flow.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "wayland/wayland.hpp"

#include "daemon/daemon.hpp"
#include <cstdio>
#include <iostream>
#include <memory>

void usage() {
    fprintf(stderr, R"(Usage:
  hyprwat [OPTIONS] [id[:displayName][*]]...
  hyprwat --input [hint]
  hyprwat --password [hint]
  hyprwat --wifi
  hyprwat --audio
  hyprwat --volume-up
  hyprwat --volume-down
  hyprwat --overview
  hyprwat --wallpaper <directory>

Description:
  A simple Wayland panel to present selectable options or text input, connect to wifi networks, update audio sinks/sources, and more.

MENU MODE (default):
  You can pass a list of items directly as command-line arguments, where each
  item is a tuple in the form:

      id[:displayName][*]

  - `id`           : Required identifier string (used internally)
  - `displayName`  : Optional label to show in the UI (defaults to id)
  - `*`            : Optional suffix to mark this item as initially selected

  Examples:
    hyprwat performance:Performance* balanced:Balanced powersave:PowerSaver
    hyprwat wifi0:Home wifi1:Work wifi2:Other

  Alternatively, if no arguments are passed, options can be provided via stdin:

    echo "wifi0:Home*" | hyprwat
    echo -e "wifi0:Home*\nwifi1:Work\nwifi2:Other" | hyprwat

INPUT MODE:
  Use --input to show a text input field instead of a menu.

  Examples:
    hyprwat --input passphrase
    hyprwat --input "Enter your name"

WIFI MODE:
  Use --wifi to show available WiFi networks, select one, and enter the password if required.

AUDIO MODE:
  Use --audio to show available audio input/output devices and select one.

VOLUME MODE:
  Use --volume-up or --volume-down to adjust output volume and show centered dots OSD.

CUSTOM MODE:
    Use --custom <file> to render a fully custom menu from a YAML configuration file.

WALLPAPER MODE:
    Use --wallpaper <dir> to select wallpapers from a specified directory and set them using hyprpaper.

OVERVIEW MODE:
    Use --overview to show a visual grid of all workspaces and windows and navigate between them.

Options:
  -h, --help        Show this help message
  --input [hint]    Show text input mode with optional hint text
  --password [hint] Show password input mode with optional hint text
  --wifi            Show WiFi network selection mode
  --audio           Show audio input/output device selection mode
  --volume-up       Adjust volume up and show centered volume HUD
  --volume-down     Adjust volume down and show centered volume HUD
  --custom <path>   Load a custom flow from the specified configuration file
  --wallpaper <dir> Select wallpapers from the specified directory and set using hyprpaper
  --overview        Show a visual workspace overview and selector
)");
}

int main(const int argc, const char** argv) {

    // check for help flag
    if (argc == 2 && !strncmp(argv[1], "--help", strlen(argv[1]))) {
        usage();
        return 1;
    }

    // initialize Wayland connection
    wl::Wayland wayland;

    // setup ui with Wayland
    UI ui(wayland);

    // find cursor position for meny x/y
    auto comp = compositor::detect();
    if (!comp) {
        debug::log(ERR, "No supported compositor found (need Hyprland or fenriz), aborting");
        return 1;
    }
    Vec2 pos = comp->cursorPos();

    // get the monitor the cursor is currently on
    auto monitorAt = comp->monitorAtCursor(pos);
    if (!monitorAt) {
        debug::log(ERR, "Failed to find monitor at cursor, aborting");
        return 1;
    }
    auto& monitor = *monitorAt;

    float monitorScale = monitor.scale;

    // global offset of this monitor
    int monitorOffsetX = monitor.x;
    int monitorOffsetY = monitor.y;

    // cursor position local to this monitor in compositor logical
    float localX = pos.x - monitorOffsetX;
    float localY = pos.y - monitorOffsetY;

    // convert compositor logical to physical to wayland logical
    int waylandScale = wayland.display().getMaxScale();
    int x_physical = (int)(localX * monitorScale);
    int y_physical = (int)(localY * monitorScale);
    int x_wayland = x_physical / waylandScale;
    int y_wayland = y_physical / waylandScale;

    auto [displayWidth, displayHeight] = wayland.display().getOutputSize();
    int logicalDisplayWidth = displayWidth / monitorScale;
    int logicalDisplayHeight = displayHeight / monitorScale;

    // parse command line arguments
    auto args = Input::parseArgv(argc, argv);

    // load config
    Config config(args.configFile);

    // initialize UI at wayland scaled cursor position
    ui.init(x_wayland, y_wayland, monitorScale);

    // apply theme to UI
    ui.applyTheme(config);

    // if it's a volume operation, check single-instance
    if (args.mode == InputMode::VOLUME_OSD) {
        std::string cmd = (args.volumeAction == VolumeAction::UP) ? "up" : "down";
        if (Daemon::sendCommand(cmd)) {
            return 0; // Command sent to existing instance, exit successfully!
        }
    }

    Daemon daemon;

    // find which flow to run
    std::unique_ptr<Flow> flow;

    // INPUT or PASSWORD mode
    switch (args.mode) {
    case InputMode::INPUT:
        flow = std::make_unique<InputFlow>(args.hint, false);
        break;
    case InputMode::PASSWORD:
        flow = std::make_unique<InputFlow>(args.hint, true);
        break;
    case InputMode::WIFI: {
        auto wifiFlow = std::make_unique<WifiFlow>();
        wifiFlow->start();
        flow = std::move(wifiFlow);
        break;
    }
    case InputMode::AUDIO:
        flow = std::make_unique<AudioFlow>();
        break;
    case InputMode::VOLUME_OSD: {
        auto volumeFlow = std::make_unique<VolumeFlow>(args.volumeAction);
        VolumeFlow* flowPtr = volumeFlow.get();

        daemon.startServer([flowPtr](const std::string& cmd) { flowPtr->handleCommand(cmd); });

        flow = std::move(volumeFlow);
        break;
    }
    case InputMode::CUSTOM:
        flow = std::make_unique<CustomFlow>(args.configPath);
        break;
    case InputMode::OVERVIEW:
        if (!comp->supportsOverview()) {
            debug::log(ERR, "--overview is not supported on this compositor");
            return 1;
        }
        flow = std::make_unique<OverviewFlow>(*comp, wayland.display(), logicalDisplayWidth, logicalDisplayHeight);
        break;
    case InputMode::WALLPAPER:
        flow = std::make_unique<WallpaperFlow>(*comp, args.wallpaperDir, logicalDisplayWidth, logicalDisplayHeight);
        break;
    case InputMode::MENU:
        if (args.choices.size() > 0) {
            // use choices from argv
            flow = std::make_unique<MenuFlow>(args.choices);
        } else {
            // parse stdin asynchronously for choices
            flow = std::make_unique<MenuFlow>();
            MenuFlow* menuFlow = static_cast<MenuFlow*>(flow.get());
            Input::parseStdin([menuFlow](Choice choice) { menuFlow->addChoice(choice); });
        }
        break;
    }

    // run the flow
    ui.runFlow(*flow);

    // print result if flow completed successfully
    std::string result = flow->getResult();
    if (!result.empty()) {
        std::cout << result << std::endl;
        std::cout.flush();
    }

    return 0;
}
