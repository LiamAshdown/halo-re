/**
 * @file include/halo/objects/flags.hpp
 * Bit-flag enums for the runtime object record: object.flags, object_header.flags, object.vitality_flags and the object type masks.
 */
#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"
#include "halo/core/flag_bits.hpp"

namespace halo::objects {

using halo::operator|;
using halo::operator&;
using halo::operator^;
using halo::operator~;
using halo::operator|=;
using halo::operator&=;
using halo::operator^=;
using halo::has;
using halo::any;
using halo::to_bits;

/** object.flags (the dword at object +0x10). */
enum class object_flag : uint32_t {
    none = 0,
    no_collision = 0x00000001,
    in_water = 0x00000010,
    at_rest = 0x00000020,
    needs_cluster_update = 1u << 11,
    mirrored_geometry = 1u << 12,
    unknown_2000 = 1u << 13,
    unknown_4000 = 1u << 14,
    in_tracked_list = 1u << 16,
    unknown_20000 = 1u << 17,
    definition_flag0 = 1u << 18,
    connected_to_map = 1u << 19,
    do_not_delete = 1u << 20,
    outside_map = 1u << 21,
    unknown_400000 = 1u << 22,
    has_collision_model = 1u << 25,
    changed = 1u << 26,
    took_network_update = 1u << 27,
    unknown_2 = 0x2,
    unknown_4 = 0x4,
    unknown_8 = 0x8,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
    unknown_100 = 1u << 8,
    unknown_200 = 1u << 9,
    unknown_400 = 1u << 10,
    unknown_8000 = 1u << 15,
    unknown_800000 = 1u << 23,
    unknown_1000000 = 1u << 24,
    unknown_10000000 = 1u << 28,
    unknown_20000000 = 1u << 29,
    unknown_40000000 = 1u << 30,
    unknown_80000000 = 1u << 31,
};

/** object_header.flags (the byte at header +0x02). */
enum class object_header_flag : uint8_t {
    none = 0,
    active = 0x01,
    unknown_02 = 0x02,
    needs_update = 0x04,
    delete_pending = 0x08,
    just_created = 0x10,
    connected = 0x20,
    in_pvs_pass = 0x40,
    unknown_80 = 0x80,
};

/** object.vitality_flags (the word at object +0x106). */
enum class vitality_flag : uint16_t {
    none = 0,
    health_below_low = 0x0001,
    shield_below_low = 0x0002,
    health_frozen = 0x0004,
    shield_depleted = 0x0008,
    shield_recharging = 0x0010,
    region_response_80 = 0x0080,
    region_response_100 = 1u << 8,
    region_response_200 = 1u << 9,
    region_response_400 = 1u << 10,
    hash_flag = 1u << 11,
    stunned = 1u << 12,
    shield_stationary = 1u << 13,
    unknown_20 = 0x20,
    unknown_40 = 0x40,
    unknown_4000 = 1u << 14,
    unknown_8000 = 1u << 15,
};

/** Masks built from 1 << object type, accepted by object_try_and_get and the object iterators. */
enum class object_mask : uint32_t {
    none = 0,
    biped = 0x001,
    vehicle = 0x002,
    unit = 0x003,
    weapon = 0x004,
    equipment = 0x008,
    garbage = 0x010,
    item = 0x01c,
    projectile = 0x020,
    scenery = 0x040,
    device_machine = 0x080,
    device_control = 0x100,
    device_light_fixture = 0x200,
    device = 0x380,
    placeholder = 0x400,
    sound_scenery = 0x800,
    scenery_and_light_fixture = 0x240,
    no_node_functions = 0xfe0,
    all = 0xffffffff,
};

/** True when `type` (an object type index, as stored in object.type) is covered by `mask`. */
constexpr bool object_mask_has_type(object_mask mask, int32_t type) noexcept {
    return ((1u << (static_cast<uint32_t>(type) & 0x1f)) & static_cast<uint32_t>(mask)) != 0;
}

}  // namespace halo::objects

namespace halo {
template <> struct enable_bit_flags<objects::object_flag> : std::true_type {};
template <> struct enable_bit_flags<objects::object_header_flag> : std::true_type {};
template <> struct enable_bit_flags<objects::vitality_flag> : std::true_type {};
template <> struct enable_bit_flags<objects::object_mask> : std::true_type {};
}  // namespace halo
