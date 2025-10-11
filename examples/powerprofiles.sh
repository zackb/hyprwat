#!/bin/bash

# Define profiles: id -> display name
declare -A profiles=(
    ["performance"]="⚡ Performance"
    ["balanced"]="⚖ Balanced"
    ["power-saver"]="▽ Power Saver"
)

# Get the current active profile
current_profile=$(powerprofilesctl get)

# Build hyprwat arguments
args=()
for id in "${!profiles[@]}"; do
    label="${profiles[$id]}"
    if [[ "$id" == "$current_profile" ]]; then
        args+=("${id}:${label}*")
    else
        args+=("${id}:${label}")
    fi
done

# Launch hyprwat and capture the selection
selection=$(hyprwat "${args[@]}")

# If user made a selection, apply it
if [[ -n "$selection" ]]; then
    powerprofilesctl set "$selection"
fi

