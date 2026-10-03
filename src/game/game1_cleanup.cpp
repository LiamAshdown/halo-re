/**
 * Removal of stray and dropped objects and per-unit game engine flags.
 */

#include "tags.h"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#include <stdint.h>

#include "halo/game/game1_cleanup.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern game_time_globals *game_time;
extern game_variant game_engine_variant;
extern game_engine_definition *current_game_engine;
extern data_array *player_data;
}

namespace halo::game::engine1 {

/**
 * Periodically deletes old corpses and expired dropped items that have exceeded their lifetime (about 30
 * seconds).
 *
 * @address 0x45f320
 */
void ObjectCleanup::cleanup_dropped_objects(void)
{
    int32_t now = game_time->game_time;
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

        if ((int32_t)item->held_game_time < now - 900 &&
            (item->flags & _item_in_inventory_bit) == 0) {
            object_header *hdr = (object_header *)halo::memory::datum_get(iterator.handle, halo::objects::globals().object_data);
            uint8_t wake_flag = 0;

            if (hdr != 0) {
                tag_instance *ti = &halo::cache::globals().tag_instances[obj->definition_tag & halo::k_datum_slot_mask];
                wake_flag = (uint8_t)((*(uint32_t *)((uint8_t *)ti->data + 0x308) >> 3) & 1);
            }

            if ((hdr == 0 || (1u << hdr->type) != _object_mask_weapon ||
                 hdr->data == 0 || wake_flag == 0) &&
                (obj->network_role != 1 && (item->flags & 0x40) == 0)) {
                halo::objects::object_delete(iterator.handle);
            }
        }

        obj = halo::objects::object_iterator_next(&iterator);
    }

    iterator.type_mask = _object_mask_biped;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != 0) {
        if (900 < *(int16_t *)&((struct object *)obj)->dead_at_rest_ticks &&
            (*(uint8_t *)&((object *)obj)->vitality_flags & 4) != 0) {
            if (obj->network_role == 0) {
                halo::objects::object_delete_unparented(iterator.handle);
            } else if (obj->network_role == 3) {
                halo::objects::object_delete_recursive(iterator.handle, 0);
            }
        }
        obj = halo::objects::object_iterator_next(&iterator);
    }
}

/**
 * ...except when engine type 1 (CTF) applies special handling.
 *
 * @address 0x468010
 */
void ObjectCleanup::cleanup_stray_items(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = _object_mask_item;
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)halo::k_dword_none;

    obj = halo::objects::object_iterator_next(&iter);
    while (obj != (object *)0) {
        if (halo::networking::globals().game_mode != 1 || obj->network_role == 3) {
            int16_t index16 = (int16_t)(uint32_t)iter.handle;

            if (iter.handle != (datum_index)halo::k_dword_none && index16 >= 0 &&
                index16 < halo::objects::globals().object_data->maximum_count) {
                object_header *hdr = (object_header *)
                    ((uint8_t *)halo::objects::globals().object_data->data + (int32_t)halo::objects::globals().object_data->size * index16);
                int16_t salt = (int16_t)((uint32_t)iter.handle >> 16);

                if (hdr->identifier != 0 &&
                    (salt == 0 || hdr->identifier == salt) &&
                    ((1 << (hdr->type & 0x1f)) & _object_mask_weapon) != 0) {
                    if (hdr->data != (object *)0) {
                        uint32_t *tag_data = (uint32_t *)halo::cache::globals().tag_instances[obj->definition_tag & halo::k_datum_slot_mask].data;

                        if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0 &&
                            game_engine_variant.game_engine_index != _game_engine_oddball) {
                            goto next;
                        }
                    }
                }
            }

            {
                item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
                if ((item->flags & (_item_in_inventory_bit | _item_unknown_40_bit)) == 0) {
                    if (obj->network_role == 0) {
                        halo::objects::object_delete_unparented(iter.handle);
                    } else if (obj->network_role != 3) {
                        goto next;
                    }
                    halo::objects::object_delete_recursive(iter.handle, 0);
                }
            }
        }
next:
        obj = halo::objects::object_iterator_next(&iter);
    }
}

/**
 * Sweeps the projectile objects: those with network role 0 are deleted unparented and recursively, those with
 * role 3 recursively.
 *
 * @address 0x467f70
 */
void ObjectCleanup::cleanup_stray_projectiles(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = 0x020;
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)halo::k_dword_none;

    obj = halo::objects::object_iterator_next(&iter);
    while (obj != (object *)0) {
        if (obj->network_role == 0) {
            halo::objects::object_delete_unparented(iter.handle);
            halo::objects::object_delete_recursive(iter.handle, 0);
        } else if (obj->network_role == 3) {
            halo::objects::object_delete_recursive(iter.handle, 0);
        }
        obj = halo::objects::object_iterator_next(&iter);
    }
}

/**
 * Zeroes the shield vitality and its maximum on the unit of a player when the variant flag 0x08 is set.
 *
 * Original register convention: EAX -> player.
 *
 * @address 0x45fd20
 */
void ObjectCleanup::clear_unit_shields_when_disabled(datum_index player_handle)
{
    player *p;
    object *unit_obj;

    if (current_game_engine == 0 || player_handle == (datum_index)halo::k_dword_none ||
        (game_engine_variant.flags & 0x08) == 0) {
        return;
    }

    p = halo::game::player_at(player_handle);
    if (p->unit == (datum_index)halo::k_dword_none) {
        return;
    }

    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
    unit_obj->shield_vitality = 0.0f;
    unit_obj->maximum_shield_vitality = 0.0f;
}

/**
 * Marks players whose quit tick has arrived for deletion: a player without a unit is removed at once,
 * otherwise its unit is flagged.
 *
 * @address 0x45b590
 */
void ObjectCleanup::flag_local_player_units(void)
{
    int32_t current_tick;
    data_iterator iterator;
    player *p;
    object *unit_obj;

    if (current_game_engine != (game_engine_definition *)0) {
        current_tick = game_time->game_time;
        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        p = (player *)halo::memory::data_iterator_next(&iterator);
        while (p != (player *)0) {
            if (p->quit_tick != k_datum_index_none && p->marked_for_deletion == 0 &&
                (halo::networking::globals().game_mode == 1 || current_tick == (int32_t)p->quit_tick)) {
                p->marked_for_deletion = 1;
                if (p->unit == k_datum_index_none) {
                    halo::game::player_remove(iterator.index);
                    iterator.data = player_data;
                    iterator.next_index = 0;
                    iterator.index = k_datum_index_none;
                    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
                } else {
                    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
                    *((uint8_t *)unit_obj + 0x107) |= 0x20;
                }
            }
            p = (player *)halo::memory::data_iterator_next(&iterator);
        }
    }
}

/**
 * Low-confidence boolean accessor for one bit of the game engine option bitfield, gated on the engine being
 * active and a valid object id.
 *
 * @address 0x462c10
 */
uint8_t ObjectCleanup::object_flag_bit3_clear(int32_t handle)
{
    uint8_t result = 1;

    if (current_game_engine != 0 && handle != -1) {
        result = (~(uint8_t)(game_engine_variant.flags >> 3)) & 1;
    }
    return result;
}

}
