/**
 * @file include/halo/ai/records.hpp
 * Typed accessors for the records of the ai data arrays (actor, prop, encounter, swarm), replacing byte-pointer arithmetic on `data_array::data`.
 */
#pragma once

#include <stdint.h>
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "halo/ai/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cache/globals.hpp"
#include "halo/core/slot_mask.hpp"

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

}  // namespace halo::ai
