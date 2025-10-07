# hyprwat

A simple Wayland panel to present selectable options with a modern GUI interface. Built with ImGui and designed to work seamlessly with Hyprland compositor.

## Description

hyprwat creates a popup menu at your cursor position where you can select from a list of options. It's particularly useful for creating interactive menus for system controls, WiFi networks, or any other selectable items in a Wayland environment.

## Features

- **Wayland native**: Built specifically for Wayland compositors
- **Cursor positioning**: Automatically appears at your current cursor position
- **Flexible input**: Accept options via command line arguments or stdin
- **Modern UI**: Clean interface built with ImGui
- **Pre-selection support**: Mark items as initially selected
- **Hyprland integration**: Designed to work with Hyprland compositor

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

### Examples

```bash
# Simple options with custom display names and pre-selection
hyprwat performance:Performance* balanced:Balanced powersave:PowerSaver

# WiFi network selection
hyprwat wifi0:Home wifi1:Work wifi2:Other

# Using stdin input
echo "wifi0:Home*" | hyprwat
echo -e "wifi0:Home*\nwifi1:Work\nwifi2:Other" | hyprwat

# Real-world WiFi example (using included script)
./wifi.sh
```

### Options

- `-h, --help`: Show help message

## Build Instructions

### Dependencies

#### Arch Linux

```bash
sudo pacman -S cmake make gcc wayland wayland-protocols mesa fontconfig pkgconf
```

#### Debian/Ubuntu

```bash
sudo apt update
sudo apt install cmake make g++ libwayland-dev wayland-protocols \
                 libegl1-mesa-dev libgl1-mesa-dev libfontconfig1-dev \
                 pkg-config
```

### Building

1. **Clone the repository** (if not already done):
   ```bash
   git clone <repository-url>
   cd hyprwat
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
  - `selection/`: Selection handling logic
  - `hyprland/`: Hyprland IPC integration
- `ext/imgui/`: ImGui library (included)
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

### Power Profile Selector

```bash
hyprwat \
  performance:"🚀 Performance" \
  balanced:"⚖️ Balanced*" \
  powersave:"🔋 Power Saver"
```

## Requirements

- **Wayland compositor** (tested with Hyprland)
- **C++20 compatible compiler**
- **CMake 3.15+**
- **OpenGL/EGL support**
- **Fontconfig**

## License

TODO

## Contributing

TODO
