#include "util.hpp"
#include <random>
#include <sstream>

std::string generateUuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::stringstream ss;
    std::string fmt = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    for (char c : fmt) {
        if (c == 'x')
            ss << std::hex << dist(gen);
        else if (c == 'y')
            ss << std::hex << ((dist(gen) & 0x3) | 0x8);
        else
            ss << c;
    }
    return ss.str();
}
