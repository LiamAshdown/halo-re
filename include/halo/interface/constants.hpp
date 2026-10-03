/**
 * @file include/halo/interface/constants.hpp
 * Named constants shared by the interface module: the base UI resolution and the sentinel values the widget events use.
 */
#pragma once

#include <stdint.h>

namespace halo::interface {

/** The 640 x 480 coordinate space every widget, HUD element and menu is laid out in. */
inline constexpr int k_base_screen_width = 640;
inline constexpr int k_base_screen_height = 480;

/** The "no value" word that event arguments and binding tables use for an unassigned action. */
inline constexpr int16_t k_action_none = 0x7fff;

/** Mask of the size bits of a widget memory pool block header (the sign bit marks the block as free). */
inline constexpr uint32_t k_pool_block_size_mask = 0x7fffffff;

/** Mask of the RGB channels of a packed ARGB colour. */
inline constexpr uint32_t k_rgb_mask = 0xffffff;

/** Bit pattern of the float scale (about 1/3) every freshly built list row and menu child starts with. */
inline constexpr uint32_t k_widget_default_scale_bits = 0x3eaa7efa;

/** Offset of the client-side game state record inside the network client record. */
inline constexpr uint32_t k_client_game_offset = 0xb14;

/** Character capacity of the shared text and path scratch buffers. */
inline constexpr int k_text_buffer_chars = 256;

/** Game ticks per second and per minute: the unit game-time limits and delays are stored in. */
inline constexpr int32_t k_ticks_per_second = 30;
inline constexpr int32_t k_ticks_per_minute = 60 * k_ticks_per_second;

/** Character capacity of the HUD hint text buffer. */
inline constexpr int k_hint_text_chars = 1024;

/** Alpha byte of a fully opaque packed ARGB colour. */
inline constexpr uint32_t k_argb_alpha_opaque = 0xff000000;

/** Packed ARGB tints of the team icon in team games (blue team and red team). */
inline constexpr uint32_t k_team_color_blue = 0xb00201e3;
inline constexpr uint32_t k_team_color_red = 0xb0fe0000;

/** Packed RGB colours of the shield meter layers. */
inline constexpr uint32_t k_rgb_black = 0x000000;
inline constexpr uint32_t k_rgb_red = 0xff0000;
inline constexpr uint32_t k_rgb_green = 0x00ff00;
inline constexpr uint32_t k_rgb_yellow = 0xffff00;
inline constexpr uint32_t k_rgb_purple = 0x7f00ff;

/** Size in bytes of the controls input capture buffer the controls menu scans bindings into. */
inline constexpr int32_t k_controls_capture_buffer_size = 656;

/** The twelve menu buttons a widget close suppresses until they are released. */
inline constexpr uint16_t k_menu_button_mask = 0xfff;

/** Ticks a list spinner arrow flashes after a single step and after a page step (negative scroll_blink values count up). */
inline constexpr int16_t k_scroll_blink_ticks = 4;
inline constexpr int16_t k_scroll_blink_long = 15;

/** The word a widget input event stores in an argument slot that holds no value. */
inline constexpr int16_t k_event_argument_unset = INT16_MIN;

/** Collision mask of the line-of-sight test the HUD waypoints use to decide whether a target is occluded. */
inline constexpr uint32_t k_hud_sight_line_collision_mask = 0xc2ad;

/** The float "indefinite" NaN the HUD number evaluation stores for a value that is not available. */
inline constexpr uint32_t k_float_indefinite_bits = 0xffc00000;

/** Value of a weapon HUD element flash time that still holds its tag default (compared as the raw bits of the float). */
inline constexpr int32_t k_weapon_hud_flash_unset_bits = 0x3f80;

/** Size of the motion sensor state record, in 32-bit words. */
inline constexpr int32_t k_motion_sensor_dwords = 348;

/** Byte capacity of the heap blocks that hold a widget's name text and its longer description text. */
inline constexpr int32_t k_name_text_bytes = 256;
inline constexpr int32_t k_description_text_bytes = 512;

/** Bit of a saved-item handle that marks a built-in (read-only) game variant. */
inline constexpr int32_t k_saved_item_builtin_marker = 0x40000000;

/** The pixel shader version (1.1) the device must report for specular and shadows, and the raster capability bits that allow decals. */
inline constexpr uint32_t k_pixel_shader_version_1_1 = 0xffff0101u;
inline constexpr uint32_t k_decal_capability_mask = 0x6000000;

}  // namespace halo::interface
