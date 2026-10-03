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

/**
 * The server's per-player cache of what it last broadcast about a remote player, laid over the update queue and
 * position bookkeeping of the player record (+0x120 .. +0x1c8). Each message family keeps the tick of its last delta
 * and of its last full send, the update id it stamped, and the 3-, 12- or 16-dword baseline the next delta is
 * coded against.
 */
struct remote_player_update_cache {
    int32_t action_delta_tick;        // 0x120
    int32_t action_full_tick;         // 0x124
    uint32_t action_update_id;        // 0x128
    uint8_t action_baseline_id;       // 0x12c
    uint8_t pad_12d[3];               // 0x12d
    uint32_t action_baseline[12];     // 0x130
    uint32_t position_counter;        // 0x160
    int32_t biped_delta_tick;         // 0x164
    int32_t biped_full_tick;          // 0x168
    uint32_t biped_update_id;         // 0x16c
    uint32_t biped_baseline[3];       // 0x170
    int32_t vehicle_delta_tick;       // 0x17c
    int32_t vehicle_full_tick;        // 0x180
    uint32_t vehicle_update_id;       // 0x184
    uint32_t vehicle_baseline[16];    // 0x188
};

inline remote_player_update_cache &remote_update_cache(player *p) noexcept {
    return *reinterpret_cast<remote_player_update_cache *>(reinterpret_cast<uint8_t *>(p) + 0x120);
}

static_assert(offsetof(remote_player_update_cache, action_baseline_id) == 0x12c - 0x120);
static_assert(offsetof(remote_player_update_cache, action_baseline) == 0x130 - 0x120);
static_assert(offsetof(remote_player_update_cache, position_counter) == 0x160 - 0x120);
static_assert(offsetof(remote_player_update_cache, biped_baseline) == 0x170 - 0x120);
static_assert(offsetof(remote_player_update_cache, vehicle_delta_tick) == 0x17c - 0x120);
static_assert(offsetof(remote_player_update_cache, vehicle_baseline) == 0x188 - 0x120);
static_assert(sizeof(remote_player_update_cache) == 0x1c8 - 0x120);

static_assert(sizeof(player) == 0x200);
static_assert(offsetof(Unit, seats) == 0x2e4);
static_assert(offsetof(UnitSeat, marker_name) == 0x24);
static_assert(offsetof(UnitSeat, yaw_minimum) == 0xf0);
static_assert(offsetof(UnitSeat, yaw_maximum) == 0xf4);
static_assert(offsetof(Model, nodes) == 0xb8);
static_assert(offsetof(ModelNode, default_translation) == 0x28);
static_assert(offsetof(ModelNode, scale) == 0x68);
static_assert(offsetof(HUDGlobals, fullscreen_font) + offsetof(TagDependency, tag_id) == 0x54);
static_assert(offsetof(HUDGlobals, splitscreen_font) + offsetof(TagDependency, tag_id) == 0x64);
static_assert(offsetof(HUDGlobals, icon_color) == 0x70);
static_assert(offsetof(HUDGlobals, carnage_report_bitmap) + offsetof(TagDependency, tag_id) == 0x3d4);
static_assert(offsetof(Bitmap, bitmap_data) == 0x60);
static_assert(offsetof(ScenarioPlayerStartingLocation, team_index) == 0x10);
static_assert(offsetof(ScenarioPlayerStartingLocation, type_0) == 0x14);
static_assert(offsetof(ScenarioNetgameFlags, type) == 0x10);
static_assert(offsetof(ScenarioNetgameFlags, usage_id) == 0x12);
static_assert(offsetof(vehicle_object, vehicle) == 0x4cc);
static_assert(offsetof(biped_object, biped) == 0x4cc);
static_assert(offsetof(update_client_queue_entry, held_control_flags) == 0x08);
static_assert(offsetof(unit_object, unit.weapons) == 0x2f8);
static_assert(offsetof(unit_object, unit.current_weapon_index) == 0x2f2);

}  // namespace halo::game
