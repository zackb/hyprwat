#/bin/bash

set -e

choice=$(../build/debug/hyprwat \
  performance:"⚡ Performance" \
  balanced:"⚖ Balanced*" \
  power-saver:"▽ Power Saver")

echo "Setting power profile: $choice"
powerprofilesctl set "$choice"

