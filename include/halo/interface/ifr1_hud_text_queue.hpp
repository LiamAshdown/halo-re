#pragma once

#include "win32.h"
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Queue of timed text messages drawn on the HUD.
 */
class HudTextQueue {
public:
    static void draw_configure(int16_t font_table_index, uint16_t color_or_flags, int16_t column, uint32_t unknown_4730, int16_t color_table_index, int16_t color_index);
    static int32_t message_queue_add(uint16_t *text, int32_t start_time, int32_t tag);
    static uint32_t message_queue_init(void);
    static uint32_t message_queue_update_and_draw(widget_instance *widget);
};

}
