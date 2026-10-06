/**
 * @file src/shell/hardware_posix.cpp
 * The hardware probe off Windows (Emscripten, Linux): what config.txt and the video options read, without DirectDraw
 * or DxDiag. One display adapter (named like the OpenGL adapter identifier, src/rasterizer/gl_direct3d.cpp), 1 GB of
 * memory (the most the original reports), the CPU clock from the time-stamp counter, and no sound devices to match.
 */

#include "halo/shell/hardware.hpp"
#include "halo/core/link.hpp"
#include "halo/shell/vars.hpp"
#include "halo/platform/cpu.hpp"
#include "halo/platform/time.hpp"

#include <string.h>

static auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
static auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
static auto &display_adapter_count = halo::link::ref<uint32_t>(halo::shell::vars().display_adapter_count);
static auto &display_adapters = halo::link::ref<shell_display_adapter [k_shell_maximum_display_adapters]>(halo::shell::vars().display_adapters);
static auto &sound_device_count = halo::link::ref<uint32_t>(halo::shell::vars().sound_device_count);
static auto &selected_sound_device = halo::link::ref<int32_t>(halo::shell::vars().selected_sound_device);

namespace halo::shell {

namespace {

class PosixHardwareProbe final : public HardwareProbe {
public:
    void detect() const override
    {
        uint64_t ticks_start;
        uint32_t start;
        uint64_t ticks;

        physical_memory = 1024;
        // CPU clock: counter ticks over 100 ms (a nanosecond clock off x86 reads as 1000 MHz)
        ticks_start = halo::platform::time_stamp_counter();
        start = halo::platform::tick_milliseconds();
        while (halo::platform::tick_milliseconds() - start < 100) {
        }
        ticks = halo::platform::time_stamp_counter() - ticks_start;
        cpu_speed = static_cast<uint32_t>(ticks / 100000);
#if !defined(__i386__)
        // a nanosecond clock always reads 1000 MHz, which the game takes for a slow machine (640x480, low detail)
        cpu_speed = 3000;
#endif

        memset(display_adapters, 0, sizeof(display_adapters));
        strcpy(display_adapters[0].driver_name, "\\\\.\\DISPLAY1");
        display_adapters[0].video_memory = 256u << 20;
        display_adapter_count = 1;
        sound_device_count = 0;
        selected_sound_device = 0;
    }
};

constexpr PosixHardwareProbe k_posix_hardware_probe{};

}  // namespace

const HardwareProbe &HardwareProbe::current()
{
    return k_posix_hardware_probe;
}

}  // namespace halo::shell
