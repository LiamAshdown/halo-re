/**
 * @file include/halo/input/directinput_constants.hpp
 * Named DirectInput values the input module passes as raw numbers: error codes, the interface version, object type
 * selectors and the point-of-view angle quantisation.
 */
#pragma once

#include <stdint.h>

namespace halo::input {

/** DIERR_NOTACQUIRED / DIERR_INPUTLOST: the device must be re-acquired. */
inline constexpr int32_t k_dierr_not_acquired = static_cast<int32_t>(0x8007000cu);
inline constexpr int32_t k_dierr_input_lost = static_cast<int32_t>(0x8007001eu);

/** DirectInput8Create version (DIRECTINPUT_VERSION 0x0800). */
inline constexpr uint32_t k_directinput_version = 0x800;

/** DIDFT_OPTIONAL | DIDFT_ANYINSTANCE plus the object class: axis (0x03), point-of-view (0x10), button (0x0c). */
inline constexpr uint32_t k_didft_optional_any_instance = 0x80ffff00;
inline constexpr uint32_t k_didft_axis = 0x03;
inline constexpr uint32_t k_didft_pov = 0x10;
inline constexpr uint32_t k_didft_button = 0x0c;

/** Range assigned to every joystick axis: -4096 .. 4096. */
inline constexpr int32_t k_joystick_axis_range = 0x1000;

/** A point-of-view hat reports hundredths of a degree; each of the eight octants is 45 degrees wide, centred on its direction. */
inline constexpr int32_t k_pov_octant_width = 4500;
inline constexpr int32_t k_pov_octant_half_width = k_pov_octant_width / 2;
inline constexpr int32_t k_pov_octant_count = 8;

/** Capacity of the ASCII device name buffer (MAX_PATH + 1). */
inline constexpr int32_t k_device_name_ascii_capacity = 0x105;

}  // namespace halo::input
