#pragma once

#include <format>
#include <iostream>
enum LogLevel { TRACE = 0, DEBUG, INFO, WARN, ERR, CRIT };

namespace debug {
    inline bool verbose = false;
    inline bool quiet = false;
    template <typename... Args> void log(LogLevel level, const std::string& fmt, Args&&... args) {

        if (quiet && level < ERR) {
            return;
        }
        if (!verbose && level == TRACE) {
            return;
        }
        std::cout << '[';

        switch (level) {
        case TRACE:
            std::cout << "TRACE";
            break;
        case DEBUG:
            std::cout << "DEBUG";
            break;
        case INFO:
            std::cout << "INFO";
            break;
        case WARN:
            std::cout << "WARN";
            break;
        case ERR:
            std::cout << "ERR";
            break;
        case CRIT:
            std::cout << "CRIT";
            break;
        default:
            break;
        }

        std::cout << "] ";

        std::cout << std::vformat(fmt, std::make_format_args(args...)) << std::endl;
    }
} // namespace debug
