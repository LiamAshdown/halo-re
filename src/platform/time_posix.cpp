/**
 * @file src/platform/time_posix.cpp
 * halo::platform clocks on POSIX (Emscripten, Linux): the monotonic clock, counted in 100 ns units like the Windows
 * performance counter.
 */

#include "halo/platform/time.hpp"

#include <time.h>

namespace halo::platform {

namespace {

int64_t monotonic_nanoseconds()
{
    timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000000 + now.tv_nsec;
}

}  // namespace

int64_t performance_counter()
{
    return monotonic_nanoseconds() / 100;
}

int64_t performance_frequency()
{
    return 10000000;
}

uint32_t tick_milliseconds()
{
    return static_cast<uint32_t>(monotonic_nanoseconds() / 1000000);
}

void sleep_milliseconds(uint32_t milliseconds)
{
    timespec duration = {static_cast<time_t>(milliseconds / 1000), static_cast<long>(milliseconds % 1000) * 1000000};

    nanosleep(&duration, nullptr);
}

void timer_resolution_begin(uint32_t)
{
}

void timer_resolution_end(uint32_t)
{
}

}  // namespace halo::platform
