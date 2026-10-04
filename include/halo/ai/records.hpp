/**
 * @file include/halo/ai/records.hpp
 * Typed accessors for the records of the ai data arrays (actor, prop, encounter, swarm), replacing byte-pointer arithmetic on `data_array::data`.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "halo/ai/api.hpp"
#include "halo/ai/modes.hpp"
#include "halo/objects/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cache/globals.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/records.hpp"

namespace halo::ai {

static_assert(sizeof(actor) == k_actor_size);
static_assert(sizeof(prop) == k_prop_size);
static_assert(sizeof(encounter) == k_encounter_size);
static_assert(sizeof(swarm) == k_swarm_size);

/** Returns the actor record that `handle` indexes (slot bits only, no validity check, like the engine's own accessors). */
inline actor *actor_at(uint32_t handle)
{
    return reinterpret_cast<actor *>(static_cast<uint8_t *>(globals().actor_data->data) + (handle & k_slot_mask) * k_actor_size);
}

/** Returns the prop record that `handle` indexes. */
inline prop *prop_at(uint32_t handle)
{
    return reinterpret_cast<prop *>(static_cast<uint8_t *>(globals().prop_data->data) + (handle & k_slot_mask) * k_prop_size);
}

/** Returns the encounter record that `handle` indexes. */
inline encounter *encounter_at(uint32_t handle)
{
    return reinterpret_cast<encounter *>(static_cast<uint8_t *>(globals().encounter_data->data) + (handle & k_slot_mask) * k_encounter_size);
}

/** Returns the swarm record that `handle` indexes. */
inline swarm *swarm_at(uint32_t handle)
{
    return reinterpret_cast<swarm *>(static_cast<uint8_t *>(globals().swarm_data->data) + (handle & k_slot_mask) * k_swarm_size);
}

static_assert(sizeof(ai_conversation) == k_ai_conversation_size);
static_assert(sizeof(swarm_component) == k_swarm_component_size);

/** Returns the conversation record that `handle` indexes. */
inline ai_conversation *conversation_at(uint32_t handle)
{
    return reinterpret_cast<ai_conversation *>(static_cast<uint8_t *>(globals().conversation_data->data) + (handle & k_slot_mask) * k_ai_conversation_size);
}

/** Returns the swarm component record that `handle` indexes. */
inline swarm_component *swarm_component_at(uint32_t handle)
{
    return reinterpret_cast<swarm_component *>(static_cast<uint8_t *>(globals().swarm_component_data->data) + (handle & k_slot_mask) * k_swarm_component_size);
}

/** Returns the object record that `handle` indexes in the object data array (slot bits only, no validity check). */
inline object *object_at(uint32_t handle)
{
    return static_cast<object_header *>(halo::objects::globals().object_data->data)[handle & k_slot_mask].data;
}

/** Returns the object header (not the object data) of the slot that `handle` indexes in the object data array. */
inline object_header &object_header_at(uint32_t handle)
{
    return static_cast<object_header *>(halo::objects::globals().object_data->data)[handle & k_slot_mask];
}

static_assert(sizeof(unit_speech) == 0x30);
static_assert(offsetof(unit_speech, ai_target_unit_index) == 0x10);
static_assert(offsetof(Scenario, encounters) == 0x42c);
static_assert(sizeof(ScenarioEncounter) == 0xb0 && offsetof(ScenarioEncounter, flags) == 0x20 && offsetof(ScenarioEncounter, squads) == 0x80);
static_assert(sizeof(ScenarioSquad) == 0xe8);
static_assert(offsetof(ScenarioSquad, flags) == 0x28 && offsetof(ScenarioSquad, attacking_guard) == 0x5c && offsetof(ScenarioSquad, defending_guard) == 0x68);
static_assert(offsetof(ScenarioSquad, major_upgrade) == 0x80 && offsetof(ScenarioSquad, starting_locations) == 0xd0);
static_assert(sizeof(ScenarioActorStartingLocation) == 0x1c && offsetof(ScenarioActorStartingLocation, actor_type) == 0x18);
static_assert(sizeof(ScenarioPlatoon) == 0xac && offsetof(ScenarioPlatoon, change_attacking_defending_state_when) == 0x30 && offsetof(ScenarioPlatoon, maneuver_when) == 0x3c);
static_assert(offsetof(ActorVariant, major_variant) + offsetof(TagDependency, tag_id) == 0x30);
static_assert(offsetof(actor, saved_encounter_index) == 0x44 && offsetof(actor, saved_squad_index) == 0x48);
static_assert(offsetof(Actor, hearing_distance) == 0x4c && offsetof(Actor, berserk_proximity) == 0x3a0 && offsetof(Actor, more_flags) == 0x4);
static_assert(offsetof(ActorVariant, first_burst_delay_time) == 0x80 && offsetof(ActorVariant, special_fire_mode) == 0x154 && offsetof(ActorVariant, grenade_type) == 0x180);
static_assert(offsetof(Biped, biped_flags) == 0x2f4 && offsetof(Projectile, danger_radius) == 0x1a8 && offsetof(Weapon, triggers) == 0x4fc);
static_assert(offsetof(Equipment, powerup_type) == 0x308);
static_assert(sizeof(ai_vehicle_offer) == sizeof(ai_object_attention_record) && offsetof(ai_vehicle_offer, filters) == 0x10);
static_assert(offsetof(ModelCollisionGeometry, pathfinding_spheres) == 0x280 && sizeof(ModelCollisionGeometrySphere) == 0x20 && offsetof(ModelCollisionGeometrySphere, radius) == 0x1c);
static_assert(offsetof(Object, collision_model) + offsetof(TagDependency, tag_id) == 0x7c && offsetof(DeviceMachine, machine_flags) == 0x292);
static_assert(sizeof(path_find_result) == 0x5c && offsetof(path_find_result, waypoints) == 0x1c);
static_assert(offsetof(actor, moving) - offsetof(actor, movement_action_complete) == sizeof(path_find_result));
static_assert(offsetof(actor, path_end_point) - offsetof(actor, movement_action_complete) == offsetof(path_find_result, end_point));
static_assert(offsetof(actor, waypoint_count) - offsetof(actor, movement_action_complete) == offsetof(path_find_result, waypoint_count));
static_assert(sizeof(actor_target_tally) == 0x7b && sizeof(path_find_request) == 0x48);
static_assert(sizeof(actor_firing_positions) == 0x38 && offsetof(actor_firing_positions, location) == 0x24 && offsetof(actor_firing_positions, velocity) == 0x2c);
static_assert(offsetof(actor, body_position) - offsetof(actor, aim_origin) == 0xc && offsetof(actor, location) - offsetof(actor, aim_origin) == 0x24);
static_assert(offsetof(actor, active_unit_index) - offsetof(actor, aim_origin) == 0x38);
static_assert(offsetof(object, location_cluster_index) - offsetof(object, location_leaf_index) == offsetof(bsp_leaf_reference, cluster_index));
static_assert(offsetof(prop, head_position) - offsetof(prop, location) == sizeof(bsp_leaf_reference));
static_assert(sizeof(actor_burst_parameters) == 0x28 && sizeof(actor_burst_scale) == 0x10);
static_assert(offsetof(ActorVariant, burst_origin_radius) == 0xcc && offsetof(ActorVariant, burst_angular_velocity) == 0xf0);
static_assert(offsetof(ActorVariant, new_target_burst_duration) == 0x100 && offsetof(ActorVariant, moving_burst_duration) == 0x118 && offsetof(ActorVariant, berserk_burst_duration) == 0x130);
static_assert(sizeof(ScenarioAIAnimationReference) == 0x3c && offsetof(ScenarioAIAnimationReference, animation_graph) + offsetof(TagDependency, tag_id) == 0x2c);
static_assert(offsetof(UnitSeat, built_in_gunner) + offsetof(TagDependency, tag_id) == 0x104);
static_assert(sizeof(UnitSeat) == 0x11c);
static_assert(offsetof(Weapon, minimum_target_range) == 0x40c);
static_assert(offsetof(Vehicle, vehicle_flags) == 0x2f0);
static_assert(sizeof(ModelAnimationsAnimationGraphUnitSeat) == 0x64 && offsetof(ModelAnimationsAnimationGraphUnitSeat, animations) == 0x40);
static_assert(offsetof(ModelAnimations, units) == 0xc);
static_assert(offsetof(Object, animation_graph) == 0x38);

/** The visibility bit row of `cluster` in the structure bsp's cluster data: one bit per cluster, rows of (cluster count + 31) / 32 words. */
inline uint32_t *cluster_visibility_row(const ScenarioStructureBSP *bsp, int32_t cluster)
{
    int32_t row_words = (static_cast<int32_t>(bsp->clusters.count) + 0x1f) >> 5;

    return reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(bsp->cluster_data.pointer)) + row_words * cluster;
}

/** The structure bsp a path search or obstacle search context traces in (the contexts store its address in a 32 bit field). */
inline ScenarioStructureBSP *structure_bsp_of(const path_find_context &context)
{
    return reinterpret_cast<ScenarioStructureBSP *>(static_cast<uintptr_t>(context.structure_bsp));
}

inline ScenarioStructureBSP *structure_bsp_of(const ai_search_context &context)
{
    return reinterpret_cast<ScenarioStructureBSP *>(static_cast<uintptr_t>(context.structure_bsp));
}

/** The actor's own aim origin .. velocity run, viewed as the block actor_get_firing_positions hands out. */
inline actor_firing_positions *own_firing_positions(actor *a)
{
    return reinterpret_cast<actor_firing_positions *>(&a->aim_origin);
}

/** The scenario firing position a firing position candidate refers to (the candidate stores its address in a 32 bit field). */
inline ScenarioFiringPosition *candidate_firing_position(const actor_firing_position_candidate &candidate)
{
    return reinterpret_cast<ScenarioFiringPosition *>(static_cast<uintptr_t>(candidate.position));
}

/** The world position of the firing position a candidate refers to. */
inline real_point3d *candidate_point(const actor_firing_position_candidate &candidate)
{
    return reinterpret_cast<real_point3d *>(&candidate_firing_position(candidate)->position);
}

/** The path record that overlays the actor's movement_action_complete .. waypoint run (what path_find_reconstruct_path fills). */
inline path_find_result *path_result(actor *a)
{
    return reinterpret_cast<path_find_result *>(&a->movement_action_complete);
}

/** The bsp leaf and cluster an object record stores (object + 0x98) as a bsp_leaf_reference. */
inline bsp_leaf_reference *object_location(object *record)
{
    return reinterpret_cast<bsp_leaf_reference *>(&record->location_leaf_index);
}

/** Returns the communication record that fills the second half of a unit_speech (offsets 0x10..0x2f). */
inline ai_communication_target_result &speech_target(unit_speech &speech)
{
    return *reinterpret_cast<ai_communication_target_result *>(reinterpret_cast<uint8_t *>(&speech) + offsetof(unit_speech, ai_target_unit_index));
}

/** Returns the elements a tag reflexive points at, typed as `T` (the reflexive pointer is a 32 bit address in the loaded tag). */
template <typename T>
inline T *reflexive_data(const TagReflexive &reflexive)
{
    return reinterpret_cast<T *>(static_cast<uintptr_t>(reflexive.pointer));
}

/** Returns the datum handle of the tag a tag-reference field names. */
inline datum_index tag_handle(const TagDependency &reference)
{
    return __builtin_bit_cast(datum_index, reference.tag_id);
}

/** Returns the loaded tag data of the tag that `handle` names, typed as the tag structure `T` (slot bits only, no validity check). */
template <typename T>
inline T *tag_data(uint32_t handle)
{
    return static_cast<T *>(halo::cache::globals().tag_instances[handle & k_slot_mask].data);
}

/** Returns the actor record that `handle` indexes as raw bytes, for offset-based field access that has no typed member yet. */
inline uint8_t *actor_bytes(uint32_t handle)
{
    return reinterpret_cast<uint8_t *>(actor_at(handle));
}

/** Returns the prop record that `handle` indexes as raw bytes. */
inline uint8_t *prop_bytes(uint32_t handle)
{
    return reinterpret_cast<uint8_t *>(prop_at(handle));
}

/** Returns the object record that `handle` indexes as raw bytes. */
inline uint8_t *object_bytes(uint32_t handle)
{
    return reinterpret_cast<uint8_t *>(object_at(handle));
}

/** Returns the loaded tag data of the tag that `handle` names as raw bytes. */
inline uint8_t *tag_bytes(uint32_t handle)
{
    return static_cast<uint8_t *>(halo::cache::globals().tag_instances[handle & k_slot_mask].data);
}

}  // namespace halo::ai
