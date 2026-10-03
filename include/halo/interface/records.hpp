/**
 * @file include/halo/interface/records.hpp
 * Typed lookups of the engine records the interface code reads by datum handle (tag data, player and object records).
 */
#pragma once

#include <stdint.h>

#include "halo/core/slot_mask.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "halo/cache/api.hpp"
#include "halo/game/api.hpp"
#include "halo/objects/api.hpp"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/** Size in bytes of one player record in the players data array. */
inline constexpr uint32_t k_player_record_size = 0x200;

/** The loaded tag structure behind a tag datum handle, viewed as T. */
template <typename T = uint8_t>
inline T *tag_data(uint32_t tag_handle) {
    return static_cast<T *>(halo::cache::globals().tag_instances[tag_handle & halo::k_slot_mask].data);
}

/** The datum handle a loaded tag reference stores: the index in the low half and the identifier in the high half. */
inline uint32_t tag_handle(const TagID &id) {
    return (static_cast<uint32_t>(id.id) << 16) | id.index;
}

/** The first element of a loaded tag block (reflexive), viewed as T. */
template <typename T>
inline T *reflexive_elements(const TagReflexive &block) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(block.pointer));
}

/** The player record a player datum handle designates. */
inline player *player_record(uint32_t player_handle) {
    return reinterpret_cast<player *>(static_cast<uint8_t *>(halo::game::globals().player_data->data) +
                                      (player_handle & halo::k_slot_mask) * k_player_record_size);
}

/** The object structure an object datum handle designates, viewed as T. */
template <typename T = uint8_t>
inline T *object_record(uint32_t object_handle) {
    return reinterpret_cast<T *>(reinterpret_cast<object_header *>(halo::objects::globals().object_data->data)[object_handle & halo::k_slot_mask].data);
}

}  // namespace halo::interface
