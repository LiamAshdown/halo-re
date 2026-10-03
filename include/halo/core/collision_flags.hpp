#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo {

/**
 * First argument of collision_test_movement_segment: which faces and which kinds of geometry the segment test visits.
 * The low five bits are the model collision bsp options, bit 0x20 and up select the structure bsp, the water surface
 * and nearby objects, and bits 0x100..0x80000 are the object-type mask (one bit per object type, biped = 0x100).
 *
 * @address 0x00505880
 */
enum class collision_test_flag : uint32_t {
    none = 0,
    front_face = 0x00001,
    back_face = 0x00002,
    double_sided = 0x00004,
    ignore_invisible = 0x00008,
    ignore_breakable = 0x00010,
    structure_bsp = 0x00020,
    water_surface = 0x00040,
    nearby_objects = 0x00080,
    object_biped = 0x00100,
    object_vehicle = 0x00200,
    object_weapon = 0x00400,
    object_equipment = 0x00800,
    object_garbage = 0x01000,
    object_projectile = 0x02000,
    object_scenery = 0x04000,
    object_machine = 0x08000,
    object_control = 0x10000,
    object_light_fixture = 0x20000,
    object_placeholder = 0x40000,
    object_sound_scenery = 0x80000,
    unstick = 0x100000,
};

template <> struct enable_bit_flags<collision_test_flag> : std::true_type {};

}  // namespace halo
