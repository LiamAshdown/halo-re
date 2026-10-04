/**
 * @file include/halo/platform/time.hpp
 * Clocks and sleeping, independent of the operating system. The engine calls these instead of the Win32 timing
 * functions; src/platform/time_win32.cpp implements them on Windows with exactly those functions.
 */
#pragma once

#include <cstdint>
#include <cstring>

namespace halo::platform {

/** The high-resolution counter (QueryPerformanceCounter on Windows). */
int64_t performance_counter();

/** Counts per second of performance_counter. */
int64_t performance_frequency();

/** Milliseconds since the system started, wrapping at 2^32 (GetTickCount on Windows). */
uint32_t tick_milliseconds();

/** Gives up the processor for at least the given milliseconds; 0 only yields. */
void sleep_milliseconds(uint32_t milliseconds);

/** Asks for a finer scheduler / timer resolution until timer_resolution_end with the same value. */
void timer_resolution_begin(uint32_t milliseconds);
void timer_resolution_end(uint32_t milliseconds);

/** Stores performance_counter into an 8-byte engine field (an int64 or a large_integer record). */
template <class T> void read_performance_counter(T *out)
{
    static_assert(sizeof(T) == sizeof(int64_t), "the counter is 8 bytes");
    int64_t value = performance_counter();

    std::memcpy(out, &value, sizeof(value));
}

/** Stores performance_frequency into an 8-byte engine field. */
template <class T> void read_performance_frequency(T *out)
{
    static_assert(sizeof(T) == sizeof(int64_t), "the frequency is 8 bytes");
    int64_t value = performance_frequency();

    std::memcpy(out, &value, sizeof(value));
}

}  // namespace halo::platform
