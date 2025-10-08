#include "flows/flow.hpp"
#include "flows/simple_flows.hpp"
#include "frames/selector.hpp"
#include "hyprland/ipc.hpp"
#include "input.hpp"
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

Description:
  A simple Wayland panel to present selectable options or text input.

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

Options:
  -h, --help       Show this help message
  --input [hint]   Show text input mode with optional hint text
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

    Selector frame;

    UI ui(wayland);

    // find cursor position for meny x/y
    hyprland::Control hyprctl;
    Vec2 pos = hyprctl.getCursorPos();

    // load config
    Config config("~/.config/hyprwat/hyprwat.conf");

    ui.init((int)pos.x, (int)pos.y);
    // apply theme to UI
    ui.applyTheme(config);

    // parse command line arguments
    auto parseResult = Input::parseArgv(argc, argv);

    std::unique_ptr<Flow> flow;

    // INPUT mode
    if (parseResult.mode == InputMode::INPUT) {
        flow = std::make_unique<SimpleInputFlow>(parseResult.hint.empty() ? "Input" : parseResult.hint);
    } else {
        // MENU mode (default)
        if (argc > 1) {
            // use choices from argv
            flow = std::make_unique<SimpleMenuFlow>(parseResult.choices);
        } else {
            // Create selector and parse stdin asynchronously
            auto selector = std::make_unique<Selector>();
            Input::parseStdin([&selector](Choice choice) { selector->add(choice); });

            // Apply theme and run directly (stdin mode doesn't use flow)
            selector->applyTheme(config);
            FrameResult result = ui.run(*selector);

            // print result if submitted
            if (result.action == FrameResult::Action::SUBMIT) {
                std::cout << result.value << std::endl;
                std::cout.flush();
            }

            return 0;
        }
    }

    // run the flow
    ui.runFlow(*flow);

    // Print result if flow completed successfully
    std::string result = flow->getResult();
    if (!result.empty()) {
        std::cout << result << std::endl;
        std::cout.flush();
    }

    return 0;
}
