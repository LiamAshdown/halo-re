#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/flags.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"

namespace halo::projectiles {

/**
 * projectile_data.flags as a bit set (the same bits as the projectile_flags enum).
 */
enum class projectile_flag : uint32_t {
    none = 0,
    rotation_valid = 0x01,
    tracer = 0x02,
    hit_ground = 0x04,
    attached = 0x08,
    at_rest = 0x10,
    detonation_timer_started = 0x20,
    super_detonation_counted = 0x40,
    super_detonation = 0x80,
};

/**
 * Projectile tag flags as a bit set (the same bits as the projectile_definition_flags enum).
 */
enum class projectile_definition_flag : uint32_t {
    none = 0,
    oriented_along_velocity = 0x01,
    ai_must_use_ballistic_aiming = 0x02,
    detonation_max_time_if_attached = 0x04,
    has_super_combining_explosion = 0x08,
    combine_initial_velocity_with_parent = 0x10,
    random_attached_detonation_time = 0x20,
    minimum_unattached_detonation_time = 0x40,
};

/**
 * Object flag bits the projectile code reads or sets in object.flags.
 */
enum class projectile_object_flag : uint32_t {
    none = 0,
    in_water = 0x10,
    at_rest = 0x20,
    unknown_2000 = 0x2000,
};

/**
 * Game constants of the projectile simulation.
 */
inline constexpr float k_seconds_per_tick = 0.033333335f;
inline constexpr float k_ticks_per_second = 30.0f;

inline constexpr uint32_t k_random_multiplier = 0x0019660d;
inline constexpr uint32_t k_random_increment = 0x3c6ef35f;
inline constexpr uint32_t k_random_high_shift = 16;

/** One step of the engine linear congruential generator. */
constexpr uint32_t advance_random_seed(uint32_t seed) noexcept { return seed * k_random_multiplier + k_random_increment; }

/** Tag group of a contrail attachment ('cont'). */
inline constexpr uint32_t k_contrail_group_tag = groups::contrail;

/** A network update is stale unless its sequence is newer than the stored one by less than this many steps. */
inline constexpr int32_t k_projectile_network_stale_window = 29;

/** Index of the header pointer inside a decoded compound update record. */
inline constexpr int32_t k_projectile_update_header_slot = 17;

/** Size of the shared network message scratch buffer. */
inline constexpr uint32_t k_network_message_scratch_size = 0x7ff8;

}

namespace halo {
template <> struct enable_bit_flags<projectiles::projectile_flag> : std::true_type {};
template <> struct enable_bit_flags<projectiles::projectile_definition_flag> : std::true_type {};
template <> struct enable_bit_flags<projectiles::projectile_object_flag> : std::true_type {};
}


namespace halo::projectiles {

static_assert(offsetof(Projectile, final_velocity) == 0x1e8);
static_assert(offsetof(Projectile, maximum_range) == 0x1c8);
static_assert(offsetof(Projectile, minimum_velocity) == 0x1c4);
static_assert(offsetof(Projectile, timer) + sizeof(float) == 0x1c0);
static_assert(offsetof(projectile_object, projectile.detonation_timer) == 0x240);
static_assert(offsetof(projectile_object, projectile.rotation_axis) == 0x264);
static_assert(offsetof(projectile_object, projectile.thrown_grenade) == 0x278);
static_assert(offsetof(projectile_object, projectile.network_baseline_index) == 0x27a);
static_assert(offsetof(projectile_object, projectile.network_state) == 0x27c);
static_assert(sizeof(projectile_constants) == 4);
static_assert(sizeof(projectile_flags) == 4);
static_assert(sizeof(projectile_definition_flags) == 4);
static_assert(sizeof(projectile_state) == 4);
static_assert(sizeof(projectile_network_state) == 24);
static_assert(sizeof(projectile_data) == 188);
static_assert(sizeof(projectile_object) == 688);
static_assert(sizeof(collision_result_type) == 4);
static_assert(sizeof(collision_result) == 80);
static_assert(sizeof(projectile_creation_message) == 84);
static_assert(sizeof(projectile_detonation_message) == 16);
static_assert(sizeof(projectile_attach_message) == 10);
static_assert(sizeof(projectile_network_update_header) == 7);

}
