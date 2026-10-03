#include "halo/core/slot_mask.hpp"
#include "halo/items/items.hpp"
#include "halo/models/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/items/vars.hpp"

static auto &weapon_blur_permutation_names = halo::link::ref<char *[2]>(halo::items::vars().weapon_blur_permutation_names);

namespace halo::items {

/**
 * 0x4c1f6a: a hidden weapon (object flag 1) that is attached shows its blur on the holder
 */
static uint32_t weapon_blur_target(uint32_t item_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[item_index & halo::k_slot_mask].data;

    if ((((object *)obj)->flags & 1) && ((object *)obj)->parent_object != (datum_index)0xffffffff) {
        return ((object *)obj)->parent_object;
    }
    return item_index;
}

/**
 * Per-tick update for a weapon item: heat/age/charge meters, the ready_timer countdown, the
 * per-magazine recharge and reload state machines, and the per-trigger firing-decision switch.
 * Returns true (the original packs unrelated leftover register bits into the unused upper 24
 * bits of the return value; no caller in this module's export reads them).
 *
 * @address 0x4c1530
 */
int32_t weapon_ref::update()
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (halo::units::globals().updates_suppressed == 1) {
        return 1;
    }

    if (*(datum_index *)&weapon_tag->base.base.animation_graph.tag_id != (datum_index)0xffffffff &&
        item_obj->animation_index != -1) {
        int16_t kind = (int16_t)halo::models::animation_state_advance(*(datum_index *)&weapon_tag->base.base.animation_graph.tag_id,
                                                        (animation_state *)((uint8_t *)item_obj + 0xd0), 0,
                                                        (animation_random_stream)1);
        if (kind == 1) {
            halo::items::weapon_set_state_indicator_flags(item_index);
        } else if (kind == 2) {
            halo::items::weapon_force_settled_state(item_index);
        }
    }

    if ((weapon_tag->weapon_flags & 0x400) != 0 && item_obj->parent_object == (datum_index)0xffffffff) {
        halo::items::item_detonation_timer_start(item_index);
    }

    if (wd->ready_timer > 0.0f) {
        int skip_decrement = 0;
        if (item_obj->parent_object == (datum_index)0xffffffff) {
            skip_decrement = 0;
        } else {
            object *holder = halo::objects::object_try_and_get(item_obj->parent_object, _object_mask_unit);
            if (holder == 0) {
                skip_decrement = 0;
            } else if (holder->definition_tag == (datum_index)0xffffffff) {
                skip_decrement = 0;
            } else {
                Unit *holder_tag = (Unit *)halo::cache::globals().tag_instances[(uint16_t)holder->definition_tag].data;
                skip_decrement = (holder_tag->unit_flags & 0x800000) != 0;
            }
        }
        if (!skip_decrement) {
            wd->ready_timer = wd->ready_timer - 0.041666668f;
            if (wd->ready_timer < 0.0f) {
                wd->ready_timer = 0.0f;
            }
        }
    }

    if (wd->heat > 0.0f) {
        if (weapon_tag->overheated_threshold <= wd->heat && (wd->flags & 1) == 0) {
            wd->flags = wd->flags | 1;
            int32_t action = 0xf;

            if (weapon_tag->weapon_type == 3 && (wd->flags & 4) != 0) {
                wd->flags = (wd->flags & ~(uint32_t)4) | 1;
                action = 0x10;
            }
            halo::interface::weapon_action_notify_for_weapon(item_index, action);
            wd->overheat_effect_handle = halo::items::weapon_stop_object_effect(item_index, *(datum_index *)&weapon_tag->overheated.tag_id);
        }

        if (wd->charged_fraction == 0.0f) {
            real loss = weapon_tag->heat_loss_rate * 0.033333335f;
            if (weapon_tag->age_heat_recovery_penalty > 0.0f) {
                loss = (1.0f - wd->age * weapon_tag->age_heat_recovery_penalty) * loss;
            }
            {
                real new_heat = wd->heat - loss;
                wd->heat = new_heat;
                if (new_heat < 0.0f) {
                    wd->heat = 0.0f;
                }
            }
            if ((wd->flags & 1) != 0 && (wd->flags & 2) == 0) {
                real fraction = (wd->heat - weapon_tag->heat_recovery_threshold) / loss;
                if ((fraction < 1.0f) != (fraction == 1.0f)) {
                    wd->flags = wd->flags | 2;
                }
            }
        }

        if ((wd->flags & 1) != 0 && wd->heat < weapon_tag->heat_recovery_threshold) {
            wd->flags = wd->flags & ~(uint32_t)3;
            if (wd->overheat_effect_handle != (datum_index)0xffffffff) {
                halo::effects::effect_stop(wd->overheat_effect_handle, 1);
            }
        }
    }
    wd->charged_fraction = 0.0f;

    if (wd->action_ticks > 0) {
        wd->action_ticks = wd->action_ticks - 1;
    }

    {
        uint8_t pulled[2];
        int32_t local_trigger_index = 0;

        pulled[0] = 0;
        if ((wd->control_flags & 0x10) == 0 && wd->action_ticks < 1) {
            pulled[0] = (uint8_t)((wd->control_flags >> 1) & 1);
            if ((weapon_tag->weapon_flags & 0x1000) != 0 && (wd->control_flags & 4) != 0) {
                pulled[1] = 1;
            } else {
                pulled[1] = 0;
            }
        } else {
            pulled[1] = 0;
        }

        if (weapon_tag->secondary_trigger_mode == 1) {
            if (pulled[1] != 0 && weapon_tag->triggers.count > 0 && wd->triggers[0].firing_rate != 1.0f) {
                pulled[1] = 0;
            }
        } else if (weapon_tag->secondary_trigger_mode == 2 && pulled[1] != 0) {
            pulled[0] = 0;
        }

        if (item_obj->network_role != 1 && (wd->control_flags & 8) != 0 && weapon_tag->magazines.count > 0) {
            wd->flags = wd->flags | 8;
        }
        if ((wd->flags & 8) != 0) {
            halo::items::weapon_trigger_begin_reload(item_index, 0, 1);
        }

        if (weapon_tag->magazines.count > 0) {
            for (i = 0; i < weapon_tag->magazines.count; i++) {
                WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
                weapon_magazine_state *magazine = &wd->magazines[i];

                if (magazine_tag->rounds_recharged > 0 && magazine->rounds_loaded < magazine_tag->rounds_loaded_maximum) {
                    int16_t rate = magazine_tag->rounds_recharged;
                    int16_t old_loaded = magazine->rounds_loaded;

                    magazine->rounds_recharged_accumulator =
                        (int16_t)(magazine->rounds_recharged_accumulator + rate % 30);
                    magazine->rounds_loaded = (int16_t)(rate / 30 + old_loaded);
                    if (magazine->rounds_recharged_accumulator > 29) {
                        magazine->rounds_loaded = (int16_t)(magazine->rounds_loaded + 1);
                        magazine->rounds_recharged_accumulator =
                            (int16_t)(magazine->rounds_recharged_accumulator - 30);
                    }
                    if (magazine->rounds_loaded > magazine_tag->rounds_loaded_maximum) {
                        magazine->rounds_loaded = magazine_tag->rounds_loaded_maximum;
                    }
                }

                if (magazine->state_ticks != 0) {
                    magazine->state_ticks = magazine->state_ticks - 1;
                }

                if (magazine->state == _weapon_magazine_reloading) {
                    if (magazine->state_ticks == 1 || magazine->state_ticks - 1 < 0) {
                        if (item_obj->network_role == 1) {
                            halo::items::weapon_magazine_reload_tick_predicted(item_index, i);
                        } else {
                            halo::items::weapon_magazine_reload_tick(item_index, i);
                        }
                    }
                } else if (magazine->state == _weapon_magazine_chamber_pending) {
                    halo::items::weapon_magazine_begin_chamber(item_index, i);
                } else if (magazine->state == _weapon_magazine_chambering && magazine->state_ticks == 0) {
                    magazine->state = 0;
                    magazine->state_ticks = 0;
                }
            }
        }

        for (local_trigger_index = 0; local_trigger_index < weapon_tag->triggers.count; local_trigger_index++) {
            WeaponTrigger *tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + local_trigger_index;
            weapon_trigger_state *trigger = &wd->triggers[local_trigger_index];
            uint8_t is_pulled = pulled[local_trigger_index];

            if ((tag_trigger->flags & 0x200) != 0 && (id->flags & _item_held_by_player_bit) != 0) {
                pulled[local_trigger_index] = wd->primary_trigger > 0.05f;
                is_pulled = pulled[local_trigger_index];
            }
            if ((tag_trigger->flags & 0x40) != 0 && item_obj->parent_object == (datum_index)0xffffffff) {
                pulled[local_trigger_index] = 1;
                is_pulled = 1;
            }

            if (trigger->effect_state_ticks != 0) {
                trigger->effect_state_ticks = trigger->effect_state_ticks - 1;
            }

            if ((tag_trigger->flags & 0x10) != 0) {
                if ((trigger->flags & _weapon_trigger_was_pulled_bit) == 0 && is_pulled != 0) {
                    trigger->flags = trigger->flags ^ (uint32_t)_weapon_trigger_latched_bit;
                }
                if (is_pulled == 0) {
                    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_was_pulled_bit;
                } else {
                    trigger->flags = trigger->flags | (uint32_t)_weapon_trigger_was_pulled_bit;
                }
                pulled[local_trigger_index] = (uint8_t)((trigger->flags >> 2) & 1);
                is_pulled = pulled[local_trigger_index];
            }

            if (is_pulled == 0) {
                trigger->flags = trigger->flags | (uint32_t)_weapon_trigger_not_pulled_bit;
            }

            if (0.0f < trigger->ejection_port_recovery) {
                trigger->ejection_port_recovery -= tag_trigger->ejection_port_recovery_rate;
                if (trigger->ejection_port_recovery <= 0.0f) {
                    trigger->ejection_port_recovery = 0.0f;
                }
            }
            if (0.0f < trigger->illumination_recovery) {
                trigger->illumination_recovery -= tag_trigger->illumination_recovery_rate;
                if (trigger->illumination_recovery <= 0.0f) {
                    trigger->illumination_recovery = 0.0f;
                }
            }

            switch (trigger->effect_state) {
            case 0: {
                int32_t ready = 1;
                if ((wd->control_flags & 0x10) == 0 && item_obj->parent_object != (datum_index)0xffffffff &&
                    tag_trigger->magazine != (uint16_t)-1) {
                    int16_t magazine_index = tag_trigger->magazine;
                    int16_t rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

                    if ((rounds_loaded < tag_trigger->rounds_per_shot && (tag_trigger->flags & 4) == 0) ||
                        rounds_loaded < tag_trigger->minimum_rounds_loaded || rounds_loaded == 0) {
                        int32_t nag = 0;
                        if (item_obj->network_role == 0 && tag_trigger->rounds_per_shot > 0) {
                            int8_t empty_ticks = trigger->empty_ticks + 1;
                            trigger->empty_ticks = empty_ticks;
                            ready = wd->magazines[magazine_index].rounds_unloaded < 1;
                            if (empty_ticks > 10) {
                                ready = 1;
                                nag = 1;
                                trigger->empty_ticks = 0;
                            }
                        }
                        if (item_obj->network_role == 3 || nag) {
                            halo::items::weapon_trigger_begin_reload(item_index, magazine_index, 1);
                        }
                    }
                }
                if (is_pulled == 0 || halo::items::weapon_trigger_ready_to_fire(item_index, local_trigger_index) == 0 || !ready) {
                    if (trigger->idle_ticks < 0x7f) {
                        trigger->idle_ticks = trigger->idle_ticks + 1;
                    }
                } else {
                    halo::items::weapon_trigger_fire_or_reload(item_index, local_trigger_index, 0);
                }
                break;
            }
            case 1:
                if (is_pulled == 0) {
                    halo::items::weapon_trigger_fire_or_reload(item_index, local_trigger_index, 1);
                } else if (trigger->effect_state_ticks == 0 && wd->alternate_shots_loaded < weapon_tag->maximum_alternate_shots_loaded) {
                    halo::items::weapon_trigger_continue_burst(item_index, local_trigger_index);
                }
                break;
            case 2:
                if (trigger->effect_state_ticks == 0) {
                    halo::items::weapon_trigger_become_charged(item_index, local_trigger_index);
                } else if (is_pulled == 0) {
                    if (local_trigger_index == 0 && weapon_tag->triggers.count > 1 && (trigger->flags & 0x20) == 0) {
                        halo::items::weapon_trigger_fire_or_reload(item_index, 0, 1);
                    } else {
                        trigger->effect_state = 0;
                        trigger->effect_state_ticks = 0;
                    }
                    if (trigger->effect_handle != (datum_index)0xffffffff) {
                        halo::effects::effect_stop(trigger->effect_handle, 1);
                        trigger->effect_handle = (datum_index)0xffffffff;
                    }
                }
                break;
            case 3:
                if (is_pulled == 0) {
                    halo::items::weapon_trigger_enter_recovery(item_index, local_trigger_index);
                } else {
                    wd->charged_fraction = 1.0f - ((real)trigger->effect_state_ticks * 0.033333335f) / tag_trigger->charged_time;
                    if (trigger->effect_state_ticks == 0) {
                        halo::items::weapon_trigger_handle_empty(item_index, local_trigger_index);
                    } else {
                        int16_t rounds_loaded = wd->magazines[tag_trigger->magazine].rounds_loaded;
                        if (rounds_loaded < tag_trigger->rounds_per_shot && (tag_trigger->flags & 4) == 0) {
                            halo::items::weapon_trigger_enter_recovery(item_index, local_trigger_index);
                        }
                    }
                }
                break;
            case 4:
                if (trigger->effect_state_ticks == 0) {
                    if ((tag_trigger->flags & 8) == 0 || (id->flags & _item_held_by_player_bit) == 0 ||
                        (trigger->flags & _weapon_trigger_not_pulled_bit) != 0) {
                        trigger->effect_state = 0;
                        trigger->effect_state_ticks = 0;
                    } else {
                        halo::items::weapon_trigger_effect_set_out_of_ammo(item_index, local_trigger_index);
                    }
                }
                break;
            case 5:
                if (is_pulled == 0 || wd->tracked_object_index == (datum_index)0xffffffff) {
                    halo::items::weapon_trigger_reset_tracking(item_index, local_trigger_index);
                }
                break;
            case 6:
                if (trigger->effect_state_ticks != 0) {
                    halo::items::weapon_trigger_fire_or_reload(item_index, local_trigger_index, 1);
                } else {
                    halo::items::weapon_trigger_finish_shot(item_index, local_trigger_index);
                }
                break;
            case 7:
                if (is_pulled == 0) {
                    trigger->effect_state = 0;
                    trigger->effect_state_ticks = 0;
                }
                break;
            case 8:
                if (trigger->effect_state_ticks == 0) {
                    trigger->effect_state = 0;
                    trigger->effect_state_ticks = 0;
                }
                break;
            }

            if (is_pulled == 0) {
                real new_rate = trigger->firing_rate - tag_trigger->firing_deceleration_rate;
                trigger->firing_rate = (new_rate < 0.0f) ? 0.0f : new_rate;
                if ((trigger->flags & _weapon_trigger_blur_applied_bit) != 0 &&
                    trigger->firing_rate < tag_trigger->blurred_rate_of_fire) {
                    halo::objects::object_set_permutation_by_name(weapon_blur_target(item_index), weapon_blur_permutation_names[local_trigger_index],
                                                   -1, 0);
                    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_blur_applied_bit;
                }
            } else {
                real new_rate = tag_trigger->firing_acceleration_rate + trigger->firing_rate;
                trigger->firing_rate = (new_rate > 1.0f) ? 1.0f : new_rate;
                if (tag_trigger->blurred_rate_of_fire != 0.0f &&
                    (trigger->flags & _weapon_trigger_blur_applied_bit) == 0 &&
                    tag_trigger->blurred_rate_of_fire < trigger->firing_rate) {
                    halo::objects::object_set_permutation_by_name(weapon_blur_target(item_index), weapon_blur_permutation_names[local_trigger_index],
                                                   -1, 1);
                    trigger->flags = trigger->flags | _weapon_trigger_blur_applied_bit;
                }
            }

            {
                int8_t effect_state = trigger->effect_state;
                if (effect_state == 6 || effect_state == 4 || is_pulled != 0) {
                    real new_error = tag_trigger->error_acceleration_rate + trigger->error;
                    trigger->error = (new_error > 1.0f) ? 1.0f : new_error;
                } else {
                    real new_error = trigger->error - tag_trigger->error_deceleration_rate;
                    trigger->error = (new_error < 0.0f) ? 0.0f : new_error;
                }
            }
        }
    }

    return 1;
}

/**
 * Member form of the original weapon_update_function_values: update function values.
 *
 * @address 0x4c2110
 */
void weapon_ref::update_function_values()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    Weapon *tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    object *destination = obj;

    while ((destination->flags & _object_no_collision_bit) != 0 &&
           destination->parent_object != (datum_index)k_datum_index_none) {
        destination = ((object_header *)halo::objects::globals().object_data->data)[destination->parent_object & halo::k_slot_mask].data;
    }

    {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        WeaponFunctionIn_t *sources = &tag->weapon_a_in;
        float *function_in = destination->function_in_values;
        WeaponMagazine *tag_magazine = (WeaponMagazine *)tag->magazines.pointer;
        WeaponTrigger *tag_trigger = (WeaponTrigger *)tag->triggers.pointer;
        int32_t k;

        for (k = 0; k < 4; k++) {
            int16_t source = sources[k];
            real value = 0.0f;

            if (source != weaponfunctionin_none) {
                int16_t idx;

                switch (source) {
                case weaponfunctionin_heat:
                    value = wd->heat;
                    break;

                case weaponfunctionin_primary_ammunition:
                case weaponfunctionin_secondary_ammunition:
                    idx = (int16_t)(source - weaponfunctionin_primary_ammunition);
                    if (idx < (int32_t)tag->magazines.count && tag_magazine[idx].rounds_loaded_maximum != 0) {
                        value = (real)wd->magazines[idx].rounds_loaded / (real)tag_magazine[idx].rounds_loaded_maximum;
                    }
                    break;

                case weaponfunctionin_primary_rate_of_fire:
                case weaponfunctionin_secondary_rate_of_fire:
                    idx = (int16_t)(source - weaponfunctionin_primary_rate_of_fire);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                    }
                    break;

                case weaponfunctionin_ready:
                    value = 1.0f;
                    break;

                case weaponfunctionin_primary_ejection_port:
                case weaponfunctionin_secondary_ejection_port:
                    idx = (int16_t)(source - weaponfunctionin_primary_ejection_port);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].ejection_port_recovery;
                    }
                    break;

                case weaponfunctionin_overheated:
                    if ((wd->flags & _weapon_overheated_bit) != 0 && tag->heat_recovery_threshold != 1.0f) {
                        value = (wd->heat - tag->heat_recovery_threshold) / (1.0f - tag->heat_recovery_threshold);
                    }
                    break;

                case weaponfunctionin_primary_charged:
                case weaponfunctionin_secondary_charged:
                    idx = (int16_t)(source - weaponfunctionin_primary_charged);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = halo::items::weapon_trigger_get_charge_fraction(object_index, idx);
                    }
                    break;

                case weaponfunctionin_illumination: {
                    real peak = 0.0f;
                    int16_t i;

                    for (i = 0; i < (int32_t)tag->triggers.count; i++) {
                        real charge_illum = 0.0f;

                        if (tag_trigger[i].charging_time > 0.0f) {
                            charge_illum = halo::items::weapon_trigger_get_charge_fraction(object_index, i) *
                                           tag_trigger[i].charged_illumination;
                            if (!(peak > charge_illum)) {
                                peak = charge_illum;
                            }
                        }
                        if (wd->triggers[i].effect_state == _weapon_trigger_effect_charged) {
                            real charged_blend = (1.0f - tag_trigger[i].charged_illumination) * wd->charged_fraction +
                                                  tag_trigger[i].charged_illumination;
                            if (!(peak > charged_blend)) {
                                peak = charged_blend;
                            }
                        }
                        if (!(peak > wd->triggers[i].illumination_recovery)) {
                            peak = wd->triggers[i].illumination_recovery;
                        }
                        wd->triggers[i].illumination_recovery = peak;
                    }
                    value = peak;
                    {
                        real heat_illum = tag->heat_illumination * wd->heat;
                        if (!(value > heat_illum)) {
                            value = heat_illum;
                        }
                    }
                    break;
                }

                case weaponfunctionin_age:
                    value = wd->age;
                    break;

                case weaponfunctionin_integrated_light:
                    value = wd->ready_timer;
                    break;

                case weaponfunctionin_primary_firing:
                case weaponfunctionin_secondary_firing:
                    idx = (int16_t)(source - weaponfunctionin_primary_firing);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                        if (halo::game::globals().game_time->game_time - wd->last_fire_game_time > 1) {
                            value = 0.0f;
                        }
                    }
                    break;

                case weaponfunctionin_primary_firing_on:
                case weaponfunctionin_secondary_firing_on:
                    idx = (int16_t)(source - weaponfunctionin_primary_firing_on);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                        if (wd->magazines[idx].rounds_loaded == 0 ||
                            (wd->flags & _weapon_overheated_bit) != 0 ||
                            (uint8_t)halo::items::weapon_is_reloading(object_index) != 0) {
                            value = 0.0f;
                        }
                    }
                    break;
                }

                function_in[k] = value;
            }
        }
    }
}

}

namespace halo::items {

int32_t weapon_update(datum_index item_index)
{
    return halo::items::weapon_ref(item_index).update();
}

void weapon_update_function_values(uint32_t object_index)
{
    halo::items::weapon_ref(object_index).update_function_values();
}

}
