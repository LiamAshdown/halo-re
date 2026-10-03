#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo::cutscene {

/**
 * Bits of the unit flags dword that the recorded animation player toggles around playback and that
 * have no name in the shared unit_flags enum.
 */
enum class unit_playback_flags : uint32_t {
    none = 0,
    restore_marker = 0x00000040,
};

/**
 * Bits of the biped flags dword (biped_data.flags) that playback sets when it hands a unit back to the world.
 */
enum class biped_playback_flags : uint32_t {
    none = 0,
    airborne = 0x1,
    jumping = 0x2,
};

}

namespace halo {
template <> struct enable_bit_flags<cutscene::unit_playback_flags> : std::true_type {};
template <> struct enable_bit_flags<cutscene::biped_playback_flags> : std::true_type {};
}

namespace halo::cutscene {

static_assert(sizeof(cutscene_constants) == 4);
static_assert(sizeof(recorded_animation_event_type) == 4);
static_assert(sizeof(recorded_animation_vector_mask) == 4);
static_assert(sizeof(recorded_animation_compressed_delay) == 4);
static_assert(sizeof(recorded_animation_flags) == 4);
static_assert(sizeof(cinematic_title_slot) == 4);
static_assert(sizeof(cinematic_globals) == 28);
static_assert(sizeof(recorded_animation_angles) == 4);
static_assert(sizeof(recorded_animation_decoder_state) == 12);
static_assert(sizeof(recorded_animation_char_difference) == 2);
static_assert(sizeof(recorded_animation_short_difference) == 4);
static_assert(sizeof(recorded_animation_event_v1) == 4);
static_assert(sizeof(recorded_animation_animation_state_set_event_v1) == 6);
static_assert(sizeof(recorded_animation_aiming_speed_set_event_v1) == 6);
static_assert(sizeof(recorded_animation_control_flags_set_event_v1) == 6);
static_assert(sizeof(recorded_animation_weapon_index_set_event_v1) == 6);
static_assert(sizeof(recorded_animation_throttle_set_event_v1) == 12);
static_assert(sizeof(recorded_animation_multi_vector_set_event_v1) == 16);
static_assert(sizeof(recorded_animation_angle_vector_set_event_v1) == 12);
static_assert(sizeof(unit_control_data_field_layout) == 12);
static_assert(sizeof(recorded_animation_codec) == 8);
static_assert(sizeof(recorded_animation) == 100);

}
