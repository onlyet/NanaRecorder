#ifndef ONLYET_TIMER_H
#define ONLYET_TIMER_H

#include <chrono>
#include <ratio>
#include <iostream>

using namespace std::chrono;

namespace onlyet {

/**
 * @brief √Î±Ì
 * @tparam clock_type 
*/
template <typename clock_type = std::chrono::system_clock>
class Timer {
public:
    using s  = std::ratio<1, 1>;
    using ms = std::ratio<1, 1000>;
    using us = std::ratio<1, 1000000>;
    using ns = std::ratio<1, 1000000000>;

public:
    Timer() : tpStart(clock_type::now()), tpStop(tpStart) {}
    ~Timer() {}

public:
    void start() { tpStart = clock_type::now(); }
    void restart() { tpStart = clock_type::now(); }
    void stop() { tpStop = clock_type::now(); }

    template <typename span>
    auto delta() const { return duration<double, span>(clock_type::now() - tpStart).count(); }

    template <typename span>
    auto delta_restart() {
        auto ts = duration<double, span>(clock_type::now() - tpStart).count();
        start();
        return ts;
    }

    template <typename span>
    auto stop_delta() {
        stop();
        return duration<double, span>(tpStop - tpStart).count();
    }

    template <typename span>
    auto stop_delta_start() {
        stop();
        auto ts = duration<double, span>(tpStop - tpStart).count();
        start();
        return ts;
    }

private:
    time_point<clock_type> tpStart;
    time_point<clock_type> tpStop;
};

}  // namespace onlyet

#endif  // !ONLYET_TIMER_H
