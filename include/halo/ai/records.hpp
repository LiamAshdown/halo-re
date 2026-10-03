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
static_assert(offsetof(unit_speech, unknown_10) == 0x10);
static_assert(offsetof(Scenario, encounters) == 0x42c);
static_assert(sizeof(ScenarioEncounter) == 0xb0 && offsetof(ScenarioEncounter, flags) == 0x20 && offsetof(ScenarioEncounter, squads) == 0x80);
static_assert(sizeof(ScenarioSquad) == 0xe8);
static_assert(sizeof(UnitSeat) == 0x11c);
static_assert(offsetof(Weapon, minimum_target_range) == 0x40c);
static_assert(offsetof(Vehicle, vehicle_flags) == 0x2f0);
static_assert(sizeof(ModelAnimationsAnimationGraphUnitSeat) == 0x64 && offsetof(ModelAnimationsAnimationGraphUnitSeat, animations) == 0x40);
static_assert(offsetof(ModelAnimations, units) == 0xc);
static_assert(offsetof(Object, animation_graph) == 0x38);

/** Returns the communication record that fills the second half of a unit_speech (offsets 0x10..0x2f). */
inline ai_communication_target_result &speech_target(unit_speech &speech)
{
    return *reinterpret_cast<ai_communication_target_result *>(reinterpret_cast<uint8_t *>(&speech) + offsetof(unit_speech, unknown_10));
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
