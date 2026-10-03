#include "halo/items/items.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/interface/api.hpp"


namespace halo::items {

/**
 * Puts a weapon away: refuses (returns 0) if it has active trigger/reload state and the request
 * isn't forced, otherwise clears control_flags, resets every trigger, deletes any overheat
 * particle system and plays the put-away first-person action.
 *
 * @address 0x4c28f0
 */
int32_t weapon_ref::put_away(int8_t force)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    uint32_t action_handle;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    if (force == 0 && halo::items::weapon_has_active_state(item_index) != 0) {
        return 0;
    }
    if (halo::items::weapon_set_state(item_index, _weapon_state_put_away, 0) == 0) {
        return 0;
    }

    wd->control_flags = 0;
    halo::items::weapon_reset_triggers(item_index);

    if (wd->overheat_effect_handle != k_datum_index_none) {
        halo::effects::effect_delete(wd->overheat_effect_handle);
        wd->overheat_effect_handle = k_datum_index_none;
    }

    action_handle = halo::interface::local_player_index_for_weapon(item_index);
    halo::interface::first_person_weapon_process_action(action_handle, 0x0b);
    if ((int16_t)action_handle == -1) {
        halo::interface::hud_play_pickup_notification(item_index, 0xb);
    }

    return 1;
}

}

namespace halo::items {

int32_t weapon_put_away(datum_index item_index, int8_t force)
{
    return halo::items::weapon_ref(item_index).put_away(force);
}

}
