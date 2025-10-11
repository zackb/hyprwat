# hyprwat

A simple Wayland panel to present selectable options with a customizable interface. Designed to work seamlessly with Hyprland compositor.

## Description

hyprwat creates a popup menu at your cursor position where you can select from a list of options. It's particularly useful for creating interactive menus for system controls, WiFi networks, or any other selectable items in a Wayland environment. It can also present input prompts for text or passwords. Different modes can be triggered via command line arguments including a WiFi network selector and audio device selector.

## Features

- **Wayland native**: Built specifically for Wayland compositors
- **Hyprland integration**: Designed to work with Hyprland compositor
- **Cursor positioning**: Automatically appears at your current cursor position
- **Flexible input**: Accept options via command line arguments or stdin
- **UI**: Interface built with ImGui
- **Pre-selection support**: Mark items as initially selected

## Usage

```
hyprwat [OPTIONS] [id[:displayName][*]]...
```

### Command Line Arguments

You can pass a list of items directly as command-line arguments, where each item is a tuple in the form:

```
id[:displayName][*]
```

- `id`: Required identifier string (used internally)
- `displayName`: Optional label to show in the UI (defaults to id)
- `*`: Optional suffix to mark this item as initially selected

If no arguments are provided, hyprwat will read from stdin, expecting one item per line in the same format.


```
--input <hint>
```
Show an input prompt instead of a selection menu with optional hint text

```
--password <hint>
```
Show a password input prompt (masked input) with optional hint text

### Examples

#### WiFi Network Selector

![wifi](examples/img/wifi.png)

#### Power Profile Selector

![powerprofilesctl](examples/img/powerprofiles.png)

See the [examples](examples) directory for more.

```bash
# Simple options with custom display names and pre-selection
hyprwat performance:Performance* balanced:Balanced powersave:PowerSaver

# Using stdin input
echo -e "wifi0:Home*\nwifi1:Work\nwifi2:Other" | hyprwat

# Input prompt
hyprwat --input "Enter Name"

# Password prompt
hyprwat --password "Enter Passphrase"

# WiFi network selection
hyprwat --wifi

# Audio device selection
hyprwat --audio

```
See the [examples](examples) directory for more.

### Options

- `-h, --help`: Show help message
- `--input <hint>`: Show an input prompt instead of a selection menu with optional hint text
- `--password <hint>`: Show a password input prompt (masked input) with optional hint text
- `--audio`: Show audio input/output device selector (requires pipewire)
- `--wifi`:  Show WiFi network selection


## Theming
You can customize the appearance of the UI by modifying the configuration file located at `~/.config/hyprwat/hyprwat.conf`. The file uses a simple INI format to define colors and styles for various UI elements.

Example `hyprwat.conf`:

```ini
[theme]
font_color = #cdd6f4
font_path = ~/.local/share/fonts/MesloLGSDZNerdFont-Regular.ttf
font_size = 14.0
background_color = #1e1e2e
window_rounding = 10.0
frame_rounding = 6.0
background_blur = 0.95
hover_color = #3366b3ff
active_color = #3366b366
```

## Build Instructions

### Dependencies

#### Arch Linux

```bash
sudo pacman -S cmake make gcc wayland wayland-protocols mesa fontconfig pkgconf libxkbcommon pipewire sdbus-c++
```

#### Debian/Ubuntu

```bash
sudo apt update
sudo apt install cmake make g++ libwayland-dev wayland-protocols \
                 libegl1-mesa-dev libgl1-mesa-dev libfontconfig1-dev \
                 pkg-config libxkbcommon-dev libsdbus-c++-dev libpipewire-0.3-dev
```

### Building

1. **Clone the repository**:
   ```bash
   git clone git@github.com:zackb/hyprwat.git
   cd hyprwat
   git submodule update --init --recursive
   ```

2. **Build the project**:
   ```bash
   # Debug build (default)
   make debug
   
   # Or release build
   make release
   ```

3. **Install** (optional):
   ```bash
   make install
   ```

### Manual Build with CMake

If you prefer to use CMake directly:

```bash
# Configure
cmake --preset debug
# or: cmake --preset release

# Build
cmake --build --preset debug
# or: cmake --build --preset release
```

## Development

### Project Structure

- `src/`: Main source code
  - `main.cpp`: Entry point and argument parsing
  - `ui.cpp`: User interface logic
  - `wayland/`: Wayland protocol implementations
  - `renderer/`: EGL/OpenGL rendering context
  - `selection/`: Selection/Menu handling logic and UI
  - `hyprland/`: Hyprland IPC integration
- `ext/imgui/`: ImGui library (git submodule)
- `CMakeLists.txt`: Build configuration
- `Makefile`: Convenience build targets

### Code Formatting

Format the codebase with:

```bash
make fmt
```

### Testing

Run the application with test data:

```bash
# Various test scenarios
make run   # Basic test with long text
make run2  # Simple test
make run3  # Stdin input test
make run-wifi  # WiFi selection test
```

## Integration Examples

### WiFi Network Selector

The included `wifi.sh` script demonstrates integration with NetworkManager:

```bash
#!/bin/bash
nmcli -t -f active,ssid,signal dev wifi | \
  awk -F: '!seen[$2]++ { printf "%s:%s (%s%%)%s\n", $2, $2, $3, ($1 == "yes" ? "*" : "") }' | \
  ./build/debug/hyprwat
```

This is just an example showing scriptability; you should use --wifi.

### Power Profile Selector

```bash
hyprwat \
  performance:"⚡ Performance" \
  balanced:"⚖ Balanced*" \
  powersave:"▽ Power Saver"
```

### Passphrase Input Prompt
```bash
PASSPHRASE=$(./build/debug/hyprwat --input Passphrase)
echo $PASSPHRASE
```

## Requirements

- **Wayland compositor** (tested with Hyprland)
- **C++20 compatible compiler**
- **CMake 3.15+**
- **OpenGL/EGL support**
- **Fontconfig**
- **xkbcommon**
- **Pipewire**
- **sdbus-c++**


## Why?
I created hyprwat to fill a gap in the Wayland ecosystem for a simple, flexible, and visually appealing menu system that can be easily integrated into various use cases. Particularly, for waybar I needed a menu selection for wifi networks, power profiles, pavcontrol, etc. 

[My first attempt](https://github.com/zackb/code/tree/master/cpp/wat) used SDL2/3 and ImGui, but I didn't like the menu being a window (there's no layer shell support in SDL). So I rewrote it using pure Wayland protocols and EGL for rendering.

If nothing else a pretty cool [cpp wrapper for Wayland protocols](src/wayland).

If there's enough interest I'd like to expand it to have different kinds of UI like text input (wifi passwords) or sliders (volume control) and have it be a more general purpose hyprland control.

## License

[MIT](LICENSE)

## Contributing

Get at me on [GitHub](https://github.com/zackb) or [BlueSky](https://bsky.app/profile/zackzbz.bsky.social) if you have ideas, issues, or want to contribute!
