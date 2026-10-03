#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Display mode enumeration, resolution list and video options menu.
 */
class VideoOptions {
public:
    VideoOptions() = delete;

    static void display_modes_enumerate(uint32_t format);
    static void populate(uint8_t *context, uint8_t *settings);
    static uint8_t update(widget_instance *screen);
    static uint8_t reset_to_defaults(widget_instance *button);
    static uint32_t refresh_rate_compare(const uint32_t *a, const uint32_t *b);
    static uint32_t refresh_rate_find_index(int32_t resolution_index, int32_t refresh_rate);
    static void resolution_add(int32_t height, int32_t width, int32_t refresh_rate);
    static int resolution_compare(const video_resolution *a, const video_resolution *b);
    static void resolution_list_build();

private:
    static uint8_t video_mode_memory_limit_applies(void);
};

} // namespace halo::interface
