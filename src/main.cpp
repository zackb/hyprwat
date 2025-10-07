#include "input.hpp"
#include "selection/selector.hpp"
#include "src/hyprland/ipc.hpp"
#include "ui.hpp"
#include "wayland/wayland.hpp"

#include <GL/gl.h>
#include <cstdio>

void usage() {
    fprintf(stderr, R"(Usage:
  hyprwat [OPTIONS] [id[:displayName][*]]...

Description:
  A simple Wayland panel to present selectable options.

  You can pass a list of items directly as command-line arguments, where each
  item is a tuple in the form:

      id[:displayName][*]

  - `id`           : Required identifier string (used internally)
  - `displayName`  : Optional label to show in the UI (defaults to id)
  - `*`            : Optional suffix to mark this item as initially selected

  Examples:
    wat performance:Performance* balanced:Balanced powersave:PowerSaver
    wat wifi0:Home wifi1:Work wifi2:Other

Alternatively, if no arguments are passed, options can be provided via stdin:

  echo "wifi0:Home*" | wat
  echo -e "wifi0:Home*\nwifi1:Work\nwifi2:Other" | wat

Options:
  -h, --help       Show this help message
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

    ui.init((int)pos.x, (int)pos.y);
    if (argc > 1) {
        // parse argv for choices
        auto choices = Input::parseArgv(argc, argv);
        int i = 0;
        for (auto& choice : choices) {
            frame.add({choice.id, choice.display});
            if (choice.selected) {
                frame.setSelected(i);
            }
            ++i;
        }
    } else {
        // parse stdin for choices asynchronously
        Input::parseStdin([&](Choice choice) { frame.add(choice); });
    }

    // run the UI loop
    ui.run(frame);

    return 0;
}
