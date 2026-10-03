#include "halo/interface/ifr1_hud_draw.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudDraw::anchor_offset_to_screen_position.
 * blam-cc: see header
 *
 * @address 0x4ab690
 */
extern "C" void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale, const int16_t *offset, int16_t *out, int32_t selector)
{
    halo::interface::HudDraw::anchor_offset_to_screen_position(anchor, has_scale, scale, offset, out, selector);
}

/**
 * C ABI entry point; forwards to halo::interface::HudDraw::bitmap_anchor_extents.
 * blam-cc: CL -> pixel_uvs, ESI -> bitmap, EDX -> uv, EAX -> out_extents, stack -> anchor
 *
 * @address 0x4acc50
 */
extern "C" void hud_bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv, float *out_extents, int16_t anchor)
{
    halo::interface::HudDraw::bitmap_anchor_extents(pixel_uvs, bitmap, uv, out_extents, anchor);
}

/**
 * 0x4acd50, blam-cc: EAX, ESI
 * C ABI entry point; forwards to halo::interface::HudDraw::bitmap_at.
 * blam-cc: uv -> EAX, bitmap -> EDX, pixel_uvs -> CL
 *
 * @address 0x4acbb0
 */
extern "C" void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor, const Point2DInt *screen_position, float scale, float rotation, uint32_t color)
{
    halo::interface::HudDraw::bitmap_at(uv, bitmap, pixel_uvs, anchor, screen_position, scale, rotation, color);
}

/**
 * 0x4acd50, blam-cc: EAX, ESI
 * C ABI entry point; forwards to halo::interface::HudDraw::bitmap_element.
 * blam-cc: uv -> EAX, placement -> EDX, pixel_uvs -> BL
 *
 * @address 0x4acad0
 */
extern "C" void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs, void *meter_parameters, BitmapData *bitmap, uint16_t *anchor, float scale, float rotation, uint32_t color, uint8_t split_screen)
{
    halo::interface::HudDraw::bitmap_element(uv, placement, pixel_uvs, meter_parameters, bitmap, anchor, scale, rotation, color, split_screen);
}

/**
 * 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs
 * C ABI entry point; forwards to halo::interface::HudDraw::message_icon.
 * blam-cc: information -> ESI
 *
 * @address 0x4ad970
 */
extern "C" void hud_draw_message_icon(const hud_messaging_information *information, Rectangle2D *cursor, uint32_t color)
{
    halo::interface::HudDraw::message_icon(information, cursor, color);
}

/**
 * 0x514ab0; blam-cc: EAX clip, ECX bounds
 * C ABI entry point; forwards to halo::interface::HudDraw::message_text_span.
 * blam-cc: cursor -> EAX, origin -> ECX
 *
 * @address 0x4ad8e0
 */
extern "C" void hud_draw_message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text, uint8_t allow_button_prompts)
{
    halo::interface::HudDraw::message_text_span(cursor, origin, text, allow_button_prompts);
}

/**
 * C ABI entry point; forwards to halo::interface::HudDraw::multitexture_overlay.
 * blam-cc: scale -> EAX
 *
 * @address 0x4acfe0
 */
extern "C" void hud_draw_multitexture_overlay(const float *scale, const HUDInterfaceMultitextureOverlay *overlay, int16_t local_player_index, const Point2DInt *screen_position, const float *uv, const float *extents, float rotation, uint32_t color)
{
    halo::interface::HudDraw::multitexture_overlay(scale, overlay, local_player_index, screen_position, uv, extents, rotation, color);
}

/**
 * C ABI entry point; forwards to halo::interface::HudDraw::number.
 *
 * @address 0x4ac0b0
 */
extern "C" void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value, int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale)
{
    halo::interface::HudDraw::number(unused, anchor, placement, value, fraction, flags, flash_start_time, scale);
}

/**
 * 0x4acad0, blam-cc: EAX uv, EDX placement, BL pixel_uvs
 * C ABI entry point; forwards to halo::interface::HudDraw::overlays.
 *
 * @address 0x4ac950
 */
extern "C" void hud_draw_overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask, int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen)
{
    halo::interface::HudDraw::overlays(anchor, list, type_mask, flash_start_time, draw_flags, split_screen);
}

/**
 * 0x51c9a0, rasterizer quad submitter, blam-cc: EAX state
 * C ABI entry point; forwards to halo::interface::HudDraw::rotated_bitmap_quad.
 * blam-cc: screen_position -> EAX, scale -> ESI
 *
 * @address 0x4acd50
 */
extern "C" void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale, void *meter_parameters, BitmapData *bitmap, const float *uv, const float *extents, float rotation, uint32_t color)
{
    halo::interface::HudDraw::rotated_bitmap_quad(screen_position, scale, meter_parameters, bitmap, uv, extents, rotation, color);
}

/**
 * 0x4acfe0, blam-cc: EAX scale
 * C ABI entry point; forwards to halo::interface::HudDraw::static_element.
 *
 * @address 0x4ac6f0
 */
extern "C" void hud_draw_static_element(int16_t local_player_index, uint16_t *anchor, const hud_static_element_placement *element, uint32_t draw_flags, int32_t flash_start_time)
{
    halo::interface::HudDraw::static_element(local_player_index, anchor, element, draw_flags, flash_start_time);
}
