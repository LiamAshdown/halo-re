#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern object *object_iterator_next(object_iterator *iterator);
extern data_array *object_data;
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
extern game_time_globals *game_time;
extern void object_list_membership_set(uint32_t object_index, char add);
uint32_t halo::items::item_any_detonating();
void halo::items::item_detonation_timer_start(uint32_t object_index);
uint8_t halo::items::item_new(uint32_t object_index);
void halo::items::item_set_holder(uint32_t item_index, datum_index holder_index);
void halo::items::item_stamp_age_timestamp(uint32_t object_index);
}

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
    iterator.handle = (datum_index)0xffffffff;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
        if (item->detonation_countdown > 0) {
            return 1;
        }
        obj = object_iterator_next(&iterator);
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
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if (item->detonation_countdown == 0) {
        Item *tag = (Item *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

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
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    obj->flags |= 0x6000;
    id->held_game_time = game_time->game_time;
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
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if (holder_index == (datum_index)0xffffffff) {
        item->flags &= ~(uint32_t)(_item_in_inventory_bit | _item_held_by_player_bit);
        return;
    }

    {
        uint32_t original_flags = item->flags;
        object *holder = ((object_header *)object_data->data)[holder_index & 0xffff].data;
        unit_data *holder_unit = (unit_data *)((uint8_t *)holder + k_unit_data_offset);

        item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit) | _item_in_inventory_bit;

        if (holder_unit->controlling_player == (datum_index)0xffffffff) {
            item->flags = (original_flags & ~(uint32_t)(_item_held_by_player_bit | _item_unknown_40_bit))
                | _item_in_inventory_bit;
        } else {
            item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit)
                | (_item_in_inventory_bit | _item_held_by_player_bit);
        }

        obj->owner_linkage = holder_unit->controlling_player;

        object_list_membership_set(item_index, 0);

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
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    obj->network_update_tick = game_time->game_time;
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
