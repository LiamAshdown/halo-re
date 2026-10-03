#include "halo/core/slot_mask.hpp"
#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/items/records.hpp"


namespace halo::items {

/**
 * Reports whether any live item object currently has a positive detonation_countdown.
 *
 * @address 0x4bcf50
 */
uint32_t item_ref::any_detonating()
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = _object_header_active_bit;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = halo::items::item_data_of(obj);
        if (item->detonation_countdown > 0) {
            return 1;
        }
        obj = halo::objects::object_iterator_next(&iterator);
    }
    return 0;
}

/**
 * Lazily seeds an item's detonation_countdown, exactly once, from the Item tag's
 * detonation_delay range converted to ticks.
 *
 * @address 0x4bd450
 */
void item_ref::detonation_timer_start()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    item_data *item = halo::items::item_data_of(obj);

    if (item->detonation_countdown == 0) {
        Item *tag = (Item *)halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;

        halo::effects::effect_new_on_object(object_index, *(datum_index *)&((struct Item *)tag)->detonating_effect.tag_id, object_index, -1, 0.0f, 0.0f,
            0, 0);

        item->detonation_countdown =
            (int16_t)(halo::math::random_real_range(tag->detonation_delay[0], tag->detonation_delay[1]) * 30.0f);
    }
}

/**
 * The item sub-row's query_create hook (object_type_definition +0x28), run for every freshly
 * activated weapon, equipment or garbage object (all three chain through this row). Sets two
 * unnamed object flags, stamps held_game_time from the current game tick, and clears
 * ignore_object_index. Always reports success.
 *
 * @address 0x4bc580
 */
uint8_t item_ref::create()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    item_data *id = halo::items::item_data_of(obj);

    obj->flags |= 0x6000;
    id->held_game_time = halo::game::globals().game_time->game_time;
    id->ignore_object_index = (datum_index)k_datum_index_none;

    return 1;
}

/**
 * Sets (or, with holder_index == -1, clears) the object holding an item, updating the item's
 * in-inventory / held-by-player flags, its cached owner linkage, and its tracked/cluster
 * membership.
 *
 * @address 0x4bcfc0
 */
void item_ref::set_holder(datum_index holder_index)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[item_index & halo::k_slot_mask].data;
    item_data *item = halo::items::item_data_of(obj);

    if (holder_index == k_datum_index_none) {
        item->flags &= ~(uint32_t)(_item_in_inventory_bit | _item_held_by_player_bit);
        return;
    }

    {
        uint32_t original_flags = item->flags;
        object *holder = ((object_header *)halo::objects::globals().object_data->data)[holder_index & halo::k_slot_mask].data;
        unit_data *holder_unit = (unit_data *)((uint8_t *)holder + k_unit_data_offset);

        item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit) | _item_in_inventory_bit;

        if (holder_unit->controlling_player == k_datum_index_none) {
            item->flags = (original_flags & ~(uint32_t)(_item_held_by_player_bit | _item_unknown_40_bit))
                | _item_in_inventory_bit;
        } else {
            item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit)
                | (_item_in_inventory_bit | _item_held_by_player_bit);
        }

        obj->owner_linkage = holder_unit->controlling_player;

        halo::objects::object_list_membership_set(item_index, 0);

        item->flags &= ~(uint32_t)(_item_at_rest_on_structure_bit | _item_does_not_accelerate_bit);
        obj->location_leaf_index = -1;
        obj->location_cluster_index = -1;
        obj->location_reserved = -1;

        if (obj->network_role == 0) {
            obj->flags |= _object_changed_bit;
        }
    }
}

/**
 * The weapon and equipment rows' override_call_7c hook. Stamps object.network_update_tick with the
 * current game tick, restarting the age clock that weapon_is_old_enough / equipment_is_old_enough
 * test.
 * FIXED (register inputs, objdump): the original never reads ESI as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x4bc460
 */
void item_ref::stamp_age_timestamp()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;

    obj->network_update_tick = halo::game::globals().game_time->game_time;
}

}

namespace halo::items {

uint32_t item_any_detonating()
{
    return halo::items::item_ref::any_detonating();
}

void item_detonation_timer_start(uint32_t object_index)
{
    halo::items::item_ref(object_index).detonation_timer_start();
}

uint8_t item_new(uint32_t object_index)
{
    return halo::items::item_ref(object_index).create();
}

void item_set_holder(uint32_t item_index, datum_index holder_index)
{
    halo::items::item_ref(item_index).set_holder(holder_index);
}

void item_stamp_age_timestamp(uint32_t object_index)
{
    halo::items::item_ref(object_index).stamp_age_timestamp();
}

}
