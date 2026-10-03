/**
 * @file include/halo/tags/objects_layout.hpp
 * Compile-time proof that the objects tag-definition members the engine code names sit at the retail offsets it used to hard-code.
 */
#pragma once

#include <cstddef>
#include "tags.h"

static_assert(offsetof(ModelCollisionGeometry, flags) == 0x0);
static_assert(offsetof(ModelCollisionGeometry, indirect_damage_material) == 0x4);
static_assert(offsetof(ModelCollisionGeometry, unknown_20) == 0x20);
static_assert(offsetof(ModelCollisionGeometry, friendly_damage_resistance) == 0x44);
static_assert(offsetof(ModelCollisionGeometry, shield_failure_function) == 0xec);
static_assert(offsetof(ModelCollisionGeometry, shield_damaged_threshold) == 0x184);
static_assert(offsetof(ModelCollisionGeometry, shield_recharging_effect) == 0x1a8);
static_assert(offsetof(ModelCollisionGeometry, shield_recharge_rate) == 0x1c0);
static_assert(offsetof(ModelCollisionGeometry, materials) == 0x234);
static_assert(offsetof(ModelCollisionGeometry, regions) == 0x240);
static_assert(offsetof(ModelCollisionGeometry, nodes) == 0x28c);
static_assert(offsetof(ModelCollisionGeometryRegion, flags) == 0x20);
static_assert(offsetof(ModelCollisionGeometryRegion, damage_threshold) == 0x28);
static_assert(offsetof(ModelCollisionGeometryRegion, permutations) == 0x48);
static_assert(offsetof(ModelCollisionGeometryMaterial, shield_leak_percentage) == 0x28);
static_assert(offsetof(ModelCollisionGeometryMaterial, body_damage_multiplier) == 0x3c);
static_assert(offsetof(DamageEffect, damage_side_effect) == 0x1c4);
static_assert(offsetof(DamageEffect, damage_flags) == 0x1c8);
static_assert(offsetof(DamageEffect, damage_aoe_core_radius) == 0x1cc);
static_assert(offsetof(DamageEffect, damage_lower_bound) == 0x1d0);
static_assert(offsetof(DamageEffect, damage_upper_bound) == 0x1d4);
static_assert(offsetof(DamageEffect, damage_vehicle_passthrough_penalty) == 0x1dc);
static_assert(offsetof(DamageEffect, damage_instantaneous_acceleration) == 0x1f4);
static_assert(offsetof(DamageEffect, dirt) == 0x200);
static_assert(offsetof(DamageEffect, ice) == 0x27c);
static_assert(offsetof(Object, collision_model) == 0x70);
static_assert(offsetof(Object, physics) == 0x80);
static_assert(offsetof(Object, attachments) == 0x140);
static_assert(offsetof(Object, functions) == 0x158);
static_assert(offsetof(Object, predicted_resources) == 0x170);
static_assert(offsetof(Object, a_in) == 0x108 && offsetof(Object, d_in) == 0x10e);
static_assert(sizeof(ObjectFunction) == 0x168 && sizeof(ObjectChangeColors) == 0x2c);
static_assert(sizeof(ScenarioStructureBSPLeaf) == 0x10 && offsetof(ScenarioStructureBSPLeaf, cluster) == 8);
static_assert(offsetof(Scenario, object_names) == 0x204);
static_assert(offsetof(Object, change_colors) == 0x164);
static_assert(offsetof(Glow, attachment_marker) == 0x0);
static_assert(offsetof(Glow, number_of_particles) == 0x20);
static_assert(offsetof(Glow, glow_flags) == 0x28);
static_assert(offsetof(Glow, effect_rotational_velocity) == 0x64);
static_assert(offsetof(Glow, color_bound_0) == 0xb4);
static_assert(offsetof(Glow, texture) == 0x144);
static_assert(offsetof(Light, flags) == 0x0);
static_assert(offsetof(Light, radius) == 0x4);
static_assert(offsetof(Light, radius_modifer) == 0x8);
static_assert(offsetof(Light, specular_radius_multiplier) == 0x24);
static_assert(offsetof(Light, interpolation_flags) == 0x34);
static_assert(offsetof(Light, color_lower_bound) == 0x38);
static_assert(offsetof(Light, lens_flare) == 0xac);
static_assert(offsetof(Light, duration) == 0xf4);
static_assert(offsetof(Light, falloff_function) == 0xfa);
static_assert(offsetof(LightVolume, attachment_marker) == 0x0);
static_assert(offsetof(LightVolume, flags) == 0x22);
static_assert(offsetof(LightVolume, near_fade_distance) == 0x34);
static_assert(offsetof(LightVolume, brightness_scale_source) == 0x44);
static_assert(offsetof(LightVolume, map) == 0x5c);
static_assert(offsetof(LightVolume, count) == 0x6e);
static_assert(offsetof(LightVolume, frame_animation_source) == 0xb8);
static_assert(offsetof(LightVolume, frames) == 0x120);
static_assert(offsetof(LightVolumeFrame, offset_from_marker) == 0x10);
static_assert(offsetof(LightVolumeFrame, radius_hither) == 0x3c);
static_assert(offsetof(LightVolumeFrame, tint_color_hither) == 0x68);
static_assert(offsetof(LightVolumeFrame, brightness_exponent) == 0x8c);
static_assert(offsetof(Lightning, count) == 0x2);
static_assert(offsetof(Lightning, bitmap) == 0x34);
static_assert(offsetof(Lightning, markers) == 0x98);
static_assert(offsetof(LightningMarker, flags) == 0x20);
static_assert(offsetof(LightningMarker, octaves_to_next_marker) == 0x24);
static_assert(offsetof(LightningMarker, thickness) == 0x84);
static_assert(offsetof(LightningMarker, tint) == 0x88);
static_assert(offsetof(ModelNode, next_sibling_node_index) == 0x20);
static_assert(offsetof(ModelNode, parent_node_index) == 0x24);
static_assert(offsetof(ModelNode, default_translation) == 0x28);
static_assert(offsetof(ModelNode, scale) == 0x68);
static_assert(offsetof(GBXModel, nodes) == 0xb8);
static_assert(offsetof(GBXModel, regions) == 0xc4 && sizeof(ModelRegion) == 0x4c);
static_assert(offsetof(ObjectChangeColorsPermutation, color_lower_bound) == 0x4);
static_assert(offsetof(ObjectChangeColorsPermutation, color_upper_bound) == 0x10);
static_assert(offsetof(ObjectAttachment, marker) == 0x10);
static_assert(offsetof(Physics, radius) == 0x0);
static_assert(offsetof(Physics, mass) == 0x8);
static_assert(offsetof(Physics, center_of_mass) == 0xc);
static_assert(offsetof(Physics, xx_moment) == 0x50);
static_assert(offsetof(PhysicsMassPoint, position) == 0x38);
static_assert(offsetof(PhysicsMassPoint, up) == 0x50);
static_assert(offsetof(PhysicsMassPoint, radius) == 0x68);
static_assert(offsetof(Bitmap, bitmap_group_sequence) == 0x54);
static_assert(offsetof(Bitmap, bitmap_data) == 0x60);
static_assert(offsetof(BitmapGroupSequence, sprites) == 0x34);
