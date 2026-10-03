#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Low-level HUD drawing primitives: bitmaps, numbers, overlays, anchors and multitexture overlays.
 */
class HudDraw {
public:
    static void anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale, const int16_t *offset, int16_t *out, int32_t selector);
    static void bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv, float *out_extents, int16_t anchor);
    static void bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor, const Point2DInt *screen_position, float scale, float rotation, uint32_t color);
    static void bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs, void *meter_parameters, BitmapData *bitmap, uint16_t *anchor, float scale, float rotation, uint32_t color, uint8_t split_screen);
    static void message_icon(const hud_messaging_information *information, Rectangle2D *cursor, uint32_t color);
    static void message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text, uint8_t allow_button_prompts);
    static void multitexture_overlay(const float *scale, const HUDInterfaceMultitextureOverlay *overlay, int16_t local_player_index, const Point2DInt *screen_position, const float *uv, const float *extents, float rotation, uint32_t color);
    static void number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value, int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale);
    static void overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask, int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen);
    static void rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale, void *meter_parameters, BitmapData *bitmap, const float *uv, const float *extents, float rotation, uint32_t color);
    static void static_element(int16_t local_player_index, uint16_t *anchor, const hud_static_element_placement *element, uint32_t draw_flags, int32_t flash_start_time);
};

}
