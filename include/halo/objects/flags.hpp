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
    needs_cluster_update = 0x00000800,
    mirrored_geometry = 0x00001000,
    unknown_2000 = 0x00002000,
    unknown_4000 = 0x00004000,
    in_tracked_list = 0x00010000,
    unknown_20000 = 0x00020000,
    definition_flag0 = 0x00040000,
    connected_to_map = 0x00080000,
    do_not_delete = 0x00100000,
    outside_map = 0x00200000,
    unknown_400000 = 0x00400000,
    has_collision_model = 0x02000000,
    changed = 0x04000000,
    took_network_update = 0x08000000,
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
    region_response_100 = 0x0100,
    region_response_200 = 0x0200,
    region_response_400 = 0x0400,
    hash_flag = 0x0800,
    stunned = 0x1000,
    shield_stationary = 0x2000,
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
