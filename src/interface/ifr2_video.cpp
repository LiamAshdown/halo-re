#include "win32.h"
#include "halo/interface/ifr2_video.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include <string.h>
#include "halo/platform/window.hpp"

#ifdef interface
#undef interface
#endif

static auto &rasterizer_direct3d = halo::link::ref<d3d9_interface *>(halo::ui::vars().rasterizer_direct3d);
static auto &d3d_adapter = halo::link::ref<uint32_t>(halo::ui::vars().d3d_adapter);
static auto &video_memory = halo::link::ref<uint32_t>(halo::ui::vars().video_memory);

namespace halo::interface {

uint8_t VideoOptions::video_mode_memory_limit_applies(void)
{
    int32_t i;

    if (halo::shell::globals().maximum_resolution == 4096) {
        return 1;
    }
    for (i = 0; i < halo::shell::globals().argc; i++) {
        const char *argument = halo::shell::globals().argv[i];

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
    halo::platform::desktop_bounds(reinterpret_cast<halo::platform::window_rect *>(&desktop));
    index = rasterizer_direct3d->vtable->get_adapter_mode_count(rasterizer_direct3d, d3d_adapter, format);
    while (index != 0) {
        index--;
        if (rasterizer_direct3d->vtable->enum_adapter_modes(rasterizer_direct3d, d3d_adapter, format, index, &mode) < 0) {
            continue;
        }
        if (video_mode_memory_limit_applies()) {
            if (video_memory <= (32u << 20) && mode.width > 1024) {
                continue;
            }
            if (video_memory <= (64u << 20) && mode.width > 1280) {
                continue;
            }
            if (video_memory <= (128u << 20) && mode.width > 1600) {
                continue;
            }
        }
        if (halo::rasterizer::globals().fullscreen == 0 || halo::rasterizer::globals().device == 0) {
            if (mode.width >= (uint32_t)desktop.right || mode.height >= (uint32_t)desktop.bottom) {
                continue;
            }
        }
        if (mode.width > halo::shell::globals().maximum_resolution || mode.width < halo::interface::k_base_screen_width || mode.height < halo::interface::k_base_screen_height || mode.width > 4800 ||
            mode.height > 3600 || mode.refresh_rate > 120) {
            continue;
        }
        if ((mode.width == 720 || mode.width == 848) && (mode.height == 576 || mode.height == halo::interface::k_base_screen_height)) {
            continue;
        }
        halo::interface::video_resolution_add((int32_t)mode.height, (int32_t)mode.width, (int32_t)mode.refresh_rate);
    }
}

} // namespace halo::interface

namespace halo::interface {

void video_display_modes_enumerate(uint32_t format)
{
    halo::interface::VideoOptions::display_modes_enumerate(format);
}

}
