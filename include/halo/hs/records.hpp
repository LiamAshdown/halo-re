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

/** Returns the global storage slot at the raw index `index` (builtin globals use their definition index, scenario globals follow them). */
inline hs_global *global_slot(int32_t index)
{
    return &static_cast<hs_global *>(globals().globals_data->data)[index];
}

/** Returns the global storage slot that `handle` indexes (slot bits only). */
inline hs_global *global_at(uint32_t handle)
{
    return global_slot(static_cast<int32_t>(handle & k_slot_mask));
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

/**
 * The evaluated argument slots of a script function call are 32-bit words; these read a slot as the value type the function declares.
 * Small integer types take the low bytes of the slot and reals reinterpret its bits, as a read through a pointer cast would.
 */
inline int16_t argument_short(int32_t slot) { return static_cast<int16_t>(slot); }
inline uint16_t argument_ushort(int32_t slot) { return static_cast<uint16_t>(slot); }
inline uint8_t argument_byte(int32_t slot) { return static_cast<uint8_t>(slot); }
inline char argument_char(int32_t slot) { return static_cast<char>(slot); }
inline float argument_real(int32_t slot) { return __builtin_bit_cast(float, slot); }
inline char *argument_string(int32_t slot) { return reinterpret_cast<char *>(static_cast<intptr_t>(slot)); }

/** Returns the first byte of `frame` scratch that is still free, typed as `T`; the caller then adds the bytes it carved to `frame->size`. */
template <typename T>
inline T *frame_scratch(hs_stack_frame *frame)
{
    return reinterpret_cast<T *>(frame->scratch + frame->size);
}

/** Returns the place the frame for a child evaluation goes: just past the scratch handed out so far. */
inline hs_stack_frame *frame_next(hs_stack_frame *frame)
{
    return reinterpret_cast<hs_stack_frame *>(reinterpret_cast<uint8_t *>(frame) + 0x10 + frame->size);
}

}  // namespace halo::hs
