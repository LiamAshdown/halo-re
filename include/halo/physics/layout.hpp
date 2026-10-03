#pragma once

#include <cstdint>

#include "halo/core/collision_flags.hpp"
#include "halo/core/flags.hpp"

namespace halo::physics {

/**
 * PhysicsPoweredMassPoint.flags (the tag bitfield), in tag order. The same bits select which
 * powered_mass_point_state inputs a powered mass point honours.
 */
enum class powered_mass_point_flag : uint32_t {
    none = 0,
    ground_friction = 1u << 0,
    water_friction = 1u << 1,
    air_friction = 1u << 2,
    water_lift = 1u << 3,
    air_lift = 1u << 4,
    thrust = 1u << 5,
    antigrav = 1u << 6,
};

/**
 * Object flag bits object_physics_integrate_and_test_at_rest mirrors from its mass points.
 */
enum class object_contact_flag : uint32_t {
    none = 0,
    ground_contact = 0x02,
    water_contact = 0x04,
    water_contact_2 = 0x08,
    fully_submerged = 0x10,
};

/**
 * Collision test mask of the sphere queries a mass point and an antigrav probe run: structure BSP
 * plus nearby machines and scenery.
 */
inline constexpr uint32_t k_mass_point_collision_mask = halo::to_bits(
    halo::collision_test_flag::structure_bsp | halo::collision_test_flag::nearby_objects |
    halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine);

/**
 * The same mask with front-face testing, used by the per-vertex overlap sweep.
 */
inline constexpr uint32_t k_mass_point_sweep_collision_mask =
    k_mass_point_collision_mask | halo::to_bits(halo::collision_test_flag::front_face);

/**
 * Mask physics_point_find_clear_position uses when pushing a unit out of a vehicle: world, bipeds,
 * vehicles, scenery and machines, plus the unresolved bit 0x200000.
 */
inline constexpr uint32_t k_clear_position_collision_mask = k_mass_point_collision_mask |
    halo::to_bits(halo::collision_test_flag::object_biped | halo::collision_test_flag::object_vehicle) | 0x200000u;

}  // namespace halo::physics

namespace halo {
template <>
struct enable_bit_flags<physics::powered_mass_point_flag> : std::true_type {};
template <>
struct enable_bit_flags<physics::object_contact_flag> : std::true_type {};
}  // namespace halo

namespace halo::physics {

inline bool powered_has(uint32_t tag_flags, powered_mass_point_flag flag) noexcept
{
    return has(static_cast<powered_mass_point_flag>(tag_flags), flag);
}

}  // namespace halo::physics
