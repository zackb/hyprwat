#!/bin/bash
# A script to set the wallpaper for Hyprland using hyprwat
# selects a wallpaper using hyprwat and updates the hyprlock and hyprpaper 
# configuration files accordingly.

wall=$(hyprwat --wallpaper ~/.local/share/wallpapers)

if [ -n "$wall" ]; then
    sed -i "s|^\$image = .*|\$image = $wall|" ~/.config/hypr/hyprlock.conf
    # hyprpaper < 0.8.0
    sed -i "s|^preload = .*|preload = $wall|" ~/.config/hypr/hyprpaper.conf
    sed -i "s|^wallpaper =.*,.*|wallpaper = , $wall|" ~/.config/hypr/hyprpaper.conf
    # hyprpaper >= 0.8.0
    sed -i "s|\(path[[:space:]]*=[[:space:]]*\).*|\1$wall|" ~/.config/hypr/hyprpaper.conf
fi
