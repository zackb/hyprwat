#pragma once

#include "choice.hpp"
#include <functional>
#include <mutex>
#include <vector>

class Input {

public:
    using Callback = std::function<void(Choice)>;
    static std::vector<Choice> parseArgv(int argc, const char* argv[]);
    static void parseStdin(Callback callback);
    static std::mutex mutex;

private:
    static Choice parseLine(std::string line);
};
