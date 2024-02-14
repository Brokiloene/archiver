#pragma once

#include <iostream>
#include <string>
#include <chrono>

// time in seconds
class TimerGuard {
private:
    std::chrono::_V2::system_clock::time_point start;
    std::ostream& stream;
    std::string msg;
public:
    TimerGuard(std::string message = "", std::ostream& out = std::cout) : start(std::chrono::high_resolution_clock::now()),
    stream(out), msg(message) {}
    ~TimerGuard() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        stream << msg << ' ' << diff.count() << '\n';
    }
};
