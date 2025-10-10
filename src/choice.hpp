#pragma once

#include <string>

struct Choice {
    std::string id;
    std::string display;
    bool selected = false;
    int strength = -1; // convenience field for wifi signal strength
};
