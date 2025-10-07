#!/bin/bash

nmcli -t -f active,ssid,signal dev wifi | awk -F: '!seen[$2]++ { printf "%s:%s (%s%%)%s\n", $2, $2, $3, ($1 == "yes" ? "*" : "") }' | ./build/debug/hyprwat
