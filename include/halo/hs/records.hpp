/**
 * @file include/halo/hs/records.hpp
 * Typed accessors for the records of the hs data arrays (syntax node, thread) and of the object list arrays the scripts walk,
 * replacing byte-pointer arithmetic on `data_array::data`.
 */
#pragma once

#include <stdint.h>
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "hs.h"
#include "halo/hs/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/slot_mask.hpp"

namespace halo::hs {

static_assert(sizeof(hs_syntax_node) == 0x14);
static_assert(sizeof(hs_thread) == 0x218);
static_assert(sizeof(object_list_header) == 0x0c);
static_assert(sizeof(object_list_reference) == 0x0c);

/** Returns the syntax node that `handle` indexes (slot bits only, no validity check). */
inline hs_syntax_node *syntax_node_at(uint32_t handle)
{
    return reinterpret_cast<hs_syntax_node *>(static_cast<uint8_t *>(globals().syntax_data->data) + (handle & k_slot_mask) * sizeof(hs_syntax_node));
}

/** Returns the script thread that `handle` indexes. */
inline hs_thread *thread_at(uint32_t handle)
{
    return reinterpret_cast<hs_thread *>(static_cast<uint8_t *>(globals().thread_data->data) + (handle & k_slot_mask) * sizeof(hs_thread));
}

/** Returns the object list header that `handle` indexes. */
inline object_list_header *object_list_header_at(uint32_t handle)
{
    return reinterpret_cast<object_list_header *>(static_cast<uint8_t *>(halo::objects::globals().object_list_header_data->data) +
                                                  (handle & k_slot_mask) * sizeof(object_list_header));
}

/** Returns the object list reference node that `handle` indexes. */
inline object_list_reference *object_list_reference_at(uint32_t handle)
{
    return reinterpret_cast<object_list_reference *>(static_cast<uint8_t *>(halo::objects::globals().object_list_reference_data->data) +
                                                     (handle & k_slot_mask) * sizeof(object_list_reference));
}

}  // namespace halo::hs
