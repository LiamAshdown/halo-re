#include "win32.h"
#include "halo/interface/ifr2_video.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern d3d9_interface *rasterizer_direct3d;
extern uint32_t d3d_adapter;
extern uint32_t config_maximum_resolution;
extern int32_t shell_argc;
extern char **shell_argv;
extern uint32_t video_memory;
extern uint8_t rasterizer_fullscreen;
extern void *rasterizer_device;
extern int _stricmp(const char *a, const char *b);
extern void video_resolution_add(int32_t height, int32_t width, int32_t refresh_rate);
}

namespace halo::interface {

uint8_t VideoOptions::video_mode_memory_limit_applies(void)
{
    int32_t i;

    if (config_maximum_resolution == 0x1000) {
        return 1;
    }
    for (i = 0; i < shell_argc; i++) {
        const char *argument = shell_argv[i];

        if (argument[0] == '-' && _stricmp("-vidmode", argument) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * blam-cc: EBX -> format
 *
 * @address 0x4baba0
 */
void VideoOptions::display_modes_enumerate(uint32_t format)
{
    win32_rect desktop;
    d3d_display_mode mode;
    uint32_t index;

    if (rasterizer_direct3d == 0) {
        return;
    }
    GetWindowRect(GetDesktopWindow(), &desktop);
    index = rasterizer_direct3d->vtable->get_adapter_mode_count(rasterizer_direct3d, d3d_adapter, format);
    while (index != 0) {
        index--;
        if (rasterizer_direct3d->vtable->enum_adapter_modes(rasterizer_direct3d, d3d_adapter, format, index, &mode) < 0) {
            continue;
        }
        if (video_mode_memory_limit_applies()) {
            if (video_memory <= 0x2000000 && mode.width > 0x400) {
                continue;
            }
            if (video_memory <= 0x4000000 && mode.width > 0x500) {
                continue;
            }
            if (video_memory <= 0x8000000 && mode.width > 0x640) {
                continue;
            }
        }
        if (rasterizer_fullscreen == 0 || rasterizer_device == 0) {
            if (mode.width >= (uint32_t)desktop.right || mode.height >= (uint32_t)desktop.bottom) {
                continue;
            }
        }
        if (mode.width > config_maximum_resolution || mode.width < 0x280 || mode.height < 0x1e0 || mode.width > 0x12c0 ||
            mode.height > 0xe10 || mode.refresh_rate > 0x78) {
            continue;
        }
        if ((mode.width == 0x2d0 || mode.width == 0x350) && (mode.height == 0x240 || mode.height == 0x1e0)) {
            continue;
        }
        video_resolution_add((int32_t)mode.height, (int32_t)mode.width, (int32_t)mode.refresh_rate);
    }
}

} // namespace halo::interface

extern "C" {

void video_display_modes_enumerate(uint32_t format)
{
    halo::interface::VideoOptions::display_modes_enumerate(format);
}

}
