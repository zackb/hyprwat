#include "flows/flow.hpp"
#include "flows/simple_flows.hpp"
#include "hyprland/ipc.hpp"
#include "input.hpp"
#include "src/flows/audio_flow.hpp"
#include "src/flows/wifi_flow.hpp"
#include "ui.hpp"
#include "wayland/wayland.hpp"

#include <GL/gl.h>
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

Options:
  -h, --help       Show this help message
  --input [hint]   Show text input mode with optional hint text
  --wifi           Show WiFi network selection mode
  --audio          Show audio input/output device selection mode
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
    hyprland::Control hyprctl;
    Vec2 pos = hyprctl.cursorPos();

    // deal with hyprland fractional scaling vs wayland integer scaling
    float hyprlandScale = hyprctl.scale();
    int waylandScale = wayland.display().getMaxScale();

    // convert hyprland logical->physical->wayland logical
    int x_physical = (int)(pos.x * hyprlandScale);
    int y_physical = (int)(pos.y * hyprlandScale);
    int x_wayland = x_physical / waylandScale;
    int y_wayland = y_physical / waylandScale;

    // load config
    Config config("~/.config/hyprwat/hyprwat.conf");

    // initialize UI at wayland scaled cursor position
    ui.init(pos.x, pos.y, hyprlandScale);

    // apply theme to UI
    ui.applyTheme(config);

    // parse command line arguments
    auto args = Input::parseArgv(argc, argv);

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
    case InputMode::MENU:
        if (argc > 1) {
            // use choices from argv
            flow = std::make_unique<MenuFlow>(args.choices);
        } else {
            // parse stdin asynchronously for choices
            auto menuFlowPtr = std::make_unique<MenuFlow>();
            MenuFlow* menuFlow = menuFlowPtr.get();
            flow = std::move(menuFlowPtr);
            Input::parseStdin([&](Choice choice) { menuFlow->addChoice(choice); });
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
