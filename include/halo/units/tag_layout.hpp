/**
 * @file include/halo/units/tag_layout.hpp
 * Compile-time proof that the units tag-definition members the engine code names sit at the retail offsets it used to hard-code.
 */
#pragma once

#include <cstddef>
#include "tags.h"

static_assert(offsetof(Unit, unit_flags) == 0x17c);
static_assert(offsetof(Unit, rider_damage_fraction) == 0x184);
static_assert(offsetof(Unit, seats) == 0x2e4);
static_assert(offsetof(Unit, powered_seats) == 0x2cc);
static_assert(offsetof(UnitSeat, flags) == 0x0);
static_assert(offsetof(UnitSeat, label) == 0x4);
static_assert(offsetof(UnitSeat, marker_name) == 0x24);
static_assert(offsetof(Biped, biped_flags) == 0x2f4);
static_assert(offsetof(Biped, minimum_soft_landing_velocity) == 0x3dc);
static_assert(offsetof(Biped, maximum_hard_landing_velocity) == 0x3e4);
static_assert(offsetof(Biped, contact_point) == 0x4e8);
static_assert(offsetof(Vehicle, suspension_sound) == 0x3b0);
static_assert(offsetof(Vehicle, material_effects) == 0x3d0);
static_assert(offsetof(ModelAnimations, objects) == 0x0);
static_assert(offsetof(ModelAnimations, units) == 0xc);
static_assert(offsetof(ModelAnimations, animations) == 0x74);
static_assert(offsetof(ModelAnimations, vehicles) == 0x24);
static_assert(offsetof(ModelAnimations, nodes) == 0x68);
static_assert(offsetof(ModelAnimationsAnimationGraphUnitSeat, right_yaw_per_frame) == 0x20);
static_assert(offsetof(ModelAnimationsAnimationGraphUnitSeat, animations) == 0x40);
static_assert(offsetof(ModelAnimationsAnimationGraphUnitSeat, weapons) == 0x58);
static_assert(offsetof(ModelAnimationsAnimationGraphWeapon, right_yaw_per_frame) == 0x60);
static_assert(offsetof(ModelAnimationsAnimationGraphWeapon, animations) == 0x98);
static_assert(offsetof(ModelAnimationsAnimationGraphNode, parent_node_index) == 0x24);
static_assert(offsetof(ModelAnimationsAnimationGraphNode, node_joint_flags) == 0x28);
static_assert(offsetof(ModelAnimationsAnimationGraphNode, base_vector) == 0x2c);
static_assert(offsetof(ModelAnimationSuspensionAnimation, full_extension_ground_depth) == 0x4);
static_assert(offsetof(GlobalsFallingDamage, falling_damage) == 0x10);
static_assert(offsetof(GlobalsFallingDamage, maximum_falling_velocity) == 0x8c);
