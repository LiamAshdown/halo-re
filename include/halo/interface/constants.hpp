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

}  // namespace halo::interface
