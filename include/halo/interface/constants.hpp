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

}  // namespace halo::interface
