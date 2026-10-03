#pragma once

#include <cstdint>

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "scenario.h"
#include "structures.h"
#include "objects.h"
#include "game.h"
#include "units.h"

#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/game/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/tags/flags.hpp"
#include "tags.h"

namespace halo::game {

/**
 * The player record in the slot named by a player datum handle.
 *
 * Stands for the `players` data array arithmetic (data + slot * sizeof(player)) the engine code open-coded everywhere.
 */
inline player *player_at(uint32_t handle) noexcept {
    return reinterpret_cast<player *>(static_cast<uint8_t *>(globals().player_data->data) + (handle & k_datum_slot_mask) * sizeof(player));
}

/** The live object in the slot named by an object datum handle (the object header's data pointer). */
inline object *object_at(uint32_t handle) noexcept {
    return reinterpret_cast<object_header *>(objects::globals().object_data->data)[handle & k_datum_slot_mask].data;
}

/** The raw bytes of the object in the slot named by an object datum handle. */
inline uint8_t *object_bytes(uint32_t handle) noexcept { return reinterpret_cast<uint8_t *>(object_at(handle)); }

/** The object header of the slot named by an object datum handle. */
inline object_header &object_header_at(uint32_t handle) noexcept {
    return reinterpret_cast<object_header *>(objects::globals().object_data->data)[handle & k_datum_slot_mask];
}

/** The unit object in the slot named by an object datum handle. */
inline unit_object *unit_at(uint32_t handle) noexcept { return reinterpret_cast<unit_object *>(object_at(handle)); }

/** The weapon datum in the unit's current weapon slot, or none when the unit is unarmed. */
inline datum_index unit_current_weapon(const unit_data &unit) noexcept {
    return unit.current_weapon_index != -1 ? unit.weapons[unit.current_weapon_index] : static_cast<datum_index>(k_datum_index_none);
}

/** The loaded tag data of a tag datum handle. */
inline uint8_t *tag_data_at(uint32_t tag) noexcept {
    return static_cast<uint8_t *>(cache::globals().tag_instances[tag & k_datum_slot_mask].data);
}

/** True when `flag` is set in the weapon_flags word (+0x308) of a weapon tag's data. */
inline bool weapon_flag_set(const void *weapon_tag, tags::weapon_tag_flag flag) noexcept {
    return has(static_cast<tags::weapon_tag_flag>(static_cast<const Weapon *>(weapon_tag)->weapon_flags), flag);
}

/** The powerup type (+0x308) of an equipment tag's data. */
inline EquipmentPowerupType_t equipment_powerup_type(const void *equipment_tag) noexcept {
    return static_cast<const Equipment *>(equipment_tag)->powerup_type;
}

static_assert(offsetof(Weapon, weapon_flags) == 0x308);
static_assert(offsetof(Equipment, powerup_type) == 0x308);

static_assert(sizeof(player) == 0x200);
static_assert(offsetof(unit_object, unit.weapons) == 0x2f8);
static_assert(offsetof(unit_object, unit.current_weapon_index) == 0x2f2);

}  // namespace halo::game
