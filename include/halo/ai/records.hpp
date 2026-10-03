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

/** Returns the object record that `handle` indexes in the object data array (slot bits only, no validity check). */
inline object *object_at(uint32_t handle)
{
    return static_cast<object_header *>(halo::objects::globals().object_data->data)[handle & k_slot_mask].data;
}

}  // namespace halo::ai
