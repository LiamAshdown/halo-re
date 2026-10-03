/**
 * @file include/halo/objects/record_access.hpp
 * Inline accessors for the raw bytes of an object record, its pool header and a tag instance, addressed by datum handle.
 * They replace the OBJECT_DATA / OBJECT_HEADER / TAG_DATA macros the per-module sources used to define locally.
 */
#pragma once

#include <cstdint>
#include "halo/core/datum.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/core/raw_access.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "halo/objects/api.hpp"
#include "halo/cache/api.hpp"
#include "objects.h"

namespace halo::objects {

/** Pool header of the object with the given handle. */
inline object_header &object_header_of(uint32_t handle)
{
    return reinterpret_cast<object_header *>(globals().object_data->data)[halo::datum_slot(handle)];
}

/** First byte of the object record with the given handle. */
inline uint8_t *object_record_bytes(uint32_t handle)
{
    return reinterpret_cast<uint8_t *>(object_header_of(handle).data);
}

/** The object record with the given handle, viewed as the record struct T (object, unit_object, biped_object, ...). */
template <typename T>
inline T *object_as(uint32_t handle)
{
    return reinterpret_cast<T *>(object_record_bytes(handle));
}

/** First byte of the loaded tag data with the given tag handle. */
inline uint8_t *tag_record_bytes(uint32_t tag_handle)
{
    return reinterpret_cast<uint8_t *>(halo::cache::globals().tag_instances[halo::datum_slot(tag_handle)].data);
}

/** Loaded tag data with the given tag handle, viewed as the tag definition struct T. */
template <typename T>
inline T *tag_as(uint32_t tag_handle)
{
    return reinterpret_cast<T *>(tag_record_bytes(tag_handle));
}

/** The elements of a tag block (reflexive), viewed as the element struct T. */
template <typename T>
inline T *block_elements(const TagReflexive &block)
{
    return reinterpret_cast<T *>(static_cast<uintptr_t>(block.pointer));
}

/** Element `index` of a tag block. */
template <typename T>
inline T &block_element(const TagReflexive &block, int32_t index)
{
    return block_elements<T>(block)[index];
}

/** The array an object keeps in its record block (node matrices, node function values), found through a block reference. */
template <typename T>
inline T *object_block(object &record, const object_block_reference &block)
{
    return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(&record) + block.offset);
}

/** The datum handle stored in the tag id of a tag reference. */
inline uint32_t tag_handle(const TagDependency &reference)
{
    return halo::bit_cast<uint32_t>(reference.tag_id);
}

}  // namespace halo::objects
