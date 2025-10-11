#!/bin/bash

declare -A profiles=(
    ["performance"]="⚡ Performance"
    ["balanced"]="⚖ Balanced"
    ["power-saver"]="▽ Power Saver"
)

current_profile=$(powerprofilesctl get)

args=()
for id in "${!profiles[@]}"; do
    label="${profiles[$id]}"
    if [[ "$id" == "$current_profile" ]]; then
        args+=("${id}:${label}*")
    else
        args+=("${id}:${label}")
    fi
done

selection=$(hyprwat "${args[@]}")

if [[ -n "$selection" ]]; then
    powerprofilesctl set "$selection"
fi

