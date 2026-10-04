/**
 * @file src/platform/time_win32.cpp
 * halo::platform clocks on Windows: the same Win32 calls the engine made directly.
 */

#include "halo/platform/time.hpp"

#include "win32.h"

namespace halo::platform {

int64_t performance_counter()
{
    LARGE_INTEGER counter;

    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

int64_t performance_frequency()
{
    LARGE_INTEGER frequency;

    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

uint32_t tick_milliseconds()
{
    return GetTickCount();
}

void sleep_milliseconds(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

void timer_resolution_begin(uint32_t milliseconds)
{
    timeBeginPeriod(milliseconds);
}

void timer_resolution_end(uint32_t milliseconds)
{
    timeEndPeriod(milliseconds);
}

}  // namespace halo::platform
