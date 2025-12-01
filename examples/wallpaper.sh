#!/bin/bash
# A script to set the wallpaper for Hyprland using hyprwat
# selects a wallpaper using hyprwat and updates the hyprlock and hyprpaper 
# configuration files accordingly.

wall=$(hyprwat --wallpaper ~/.local/share/wallpapers)

if [ -n "$wall" ]; then
    sed -i "s|^\$image = .*|\$image = $wall|" ~/.config/hypr/hyprlock.conf
    sed -i "s|^preload = .*|preload = $wall|" ~/.config/hypr/hyprpaper.conf
    sed -i "s|^wallpaper =.*,.*|wallpaper = , $wall|" ~/.config/hypr/hyprpaper.conf
fi

