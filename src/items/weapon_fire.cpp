#include "halo/core/lcg.hpp"
#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/items/vars.hpp"

extern "C" {
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern void unit_update_active_camouflage_depower(datum_index player_handle);
extern uint32_t local_player_index_for_weapon(datum_index item_index);
extern void first_person_weapon_process_action(uint32_t handle, int32_t action);
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code);
uint32_t halo::items::weapon_fire_trigger(datum_index item_index, int16_t trigger_index);
}
static auto &weapon_infinite_ammo = halo::link::ref<uint8_t>(halo::items::vars().weapon_infinite_ammo);
static auto &weapon_bottomless_clip = halo::link::ref<uint8_t>(halo::items::vars().weapon_bottomless_clip);
static auto &weapon_client_side_projectiles = halo::link::ref<uint8_t>(halo::items::vars().weapon_client_side_projectiles);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::items {

/**
 * Fires one round of a weapon trigger: consumes ammo (or misfires), resolves the firing effect
 * and heat/age gain, spawns projectiles (or defers to the host), applies a self-damage/knockback
 * impulse to the holder while overheated, and updates the trigger's effect state.
 *
 * @address 0x4c3f10
 */
uint32_t weapon_ref::fire_trigger(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    weapon_trigger_state *trigger;
    datum_index holder_index;
    datum_index selected_damage_tag;
    real misfire_chance;
    int32_t is_alternate_shot;
    int32_t has_ammo;
    int32_t is_misfire;
    int32_t effect_variant;
    datum_index selected_effect_tag;
    real effect_scale_a;
    real effect_scale_b;

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    trigger = &wd->triggers[trigger_index];

    holder_index = (datum_index)0xffffffff;
    if (item_obj->parent_object != (datum_index)0xffffffff &&
        halo::objects::object_try_and_get(item_obj->parent_object, _object_mask_unit) != 0) {
        holder_index = item_obj->parent_object;
    }

    selected_damage_tag = (datum_index)0xffffffff;
    selected_effect_tag = (datum_index)0xffffffff;
    effect_scale_a = 0.0f;
    effect_scale_b = 0.0f;
    misfire_chance = 0.0f;
    is_alternate_shot = 0;
    has_ammo = 0;
    is_misfire = 0;

    if (trigger_index == 1 && (weapon_tag->secondary_trigger_mode == 3 || weapon_tag->secondary_trigger_mode == 4)) {
        is_alternate_shot = 1;
    }

    if (tag_trigger->magazine == (uint16_t)-1) {
        has_ammo = 1;
    } else {
        int16_t magazine_index = tag_trigger->magazine;
        weapon_magazine_state *magazine = &wd->magazines[magazine_index];

        if (!is_alternate_shot || wd->alternate_shots_loaded < weapon_tag->maximum_alternate_shots_loaded) {
            int16_t rounds_loaded = magazine->rounds_loaded;

            if ((tag_trigger->rounds_per_shot <= rounds_loaded || (tag_trigger->flags & 4) != 0) &&
                ((weapon_tag->weapon_flags & 0x800) == 0 || wd->age < 1.0f) &&
                (tag_trigger->minimum_rounds_loaded <= rounds_loaded || (trigger->flags & _weapon_trigger_not_pulled_bit) == 0)) {
                uint8_t emptied = 0;

                if (weapon_infinite_ammo == 0) {
                    rounds_loaded = (int16_t)(rounds_loaded - tag_trigger->rounds_per_shot);
                    magazine->rounds_loaded = rounds_loaded;
                    if (rounds_loaded < 1) {
                        magazine->rounds_loaded = 0;
                        emptied = 1;
                    }
                }
                if (!emptied) {
                    WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
                    if (magazine_tag->flags & 2) {
                        magazine->state = _weapon_magazine_chamber_pending;
                        magazine->state_ticks = 0;
                    }
                }
                has_ammo = 1;
            }
        }
    }

    if (weapon_infinite_ammo != 0) {
        has_ammo = 1;
    }

    if (tag_trigger->firing_effects.count > 0) {
        WeaponTriggerFiringEffect *effects = (WeaponTriggerFiringEffect *)tag_trigger->firing_effects.pointer;

        if (trigger->firing_effect_rounds < 1) {
            uint16_t start_index = trigger->firing_effect_index;
            uint16_t chosen_index = start_index;

            if (tag_trigger->flags & 2) {
                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                chosen_index = (uint16_t)((halo::math::globals().random_seed_global >> 0x10) % (uint32_t)tag_trigger->firing_effects.count);
            }
            do {
                uint16_t used_mask;
                int16_t lower, upper, rounds;

                if (trigger->firing_effect_used_mask == (1u << tag_trigger->firing_effects.count) - 1u) {
                    trigger->firing_effect_used_mask = 0;
                }
                used_mask = trigger->firing_effect_used_mask;
                do {
                    chosen_index = chosen_index + 1;
                    if (chosen_index >= tag_trigger->firing_effects.count) {
                        chosen_index = 0;
                    }
                } while ((used_mask & (1u << chosen_index)) != 0);

                trigger->firing_effect_index = chosen_index;
                trigger->firing_effect_used_mask = used_mask | (1u << chosen_index);

                lower = effects[chosen_index].shot_count_lower_bound;
                upper = effects[chosen_index].shot_count_upper_bound;
                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                rounds = lower + (int16_t)(((int32_t)(upper - lower) * (int32_t)(halo::math::globals().random_seed_global >> 0x10)) >> 0x10);
                trigger->firing_effect_rounds = rounds;
            } while (trigger->firing_effect_rounds < 1 && chosen_index != start_index);
        }

        {
            int16_t chosen_index = trigger->firing_effect_index;

            trigger->firing_effect_rounds = trigger->firing_effect_rounds - 1;

            if (weapon_tag->age_misfire_start > 0.0f && weapon_tag->age_misfire_start < 1.0f &&
                weapon_tag->age_misfire_start < wd->age) {
                misfire_chance = ((wd->age - weapon_tag->age_misfire_start) * weapon_tag->age_misfire_chance) /
                                  (1.0f - weapon_tag->age_misfire_start);
                if (trigger->effect_state == _weapon_trigger_effect_spewing) {
                    misfire_chance = misfire_chance + misfire_chance;
                }
                if (halo::math::random_real() < misfire_chance) {
                    is_misfire = 1;
                }
            }

            if (has_ammo) {
                effect_scale_a = trigger->firing_rate;
                if (is_misfire) {
                    effect_variant = 1;
                    effect_scale_b = 0.0f;
                } else {
                    effect_variant = 0;
                    effect_scale_b = (weapon_tag->overheated_threshold == 0.0f) ? 0.0f
                        : wd->heat / weapon_tag->overheated_threshold;
                }
            } else {
                effect_variant = 2;
                effect_scale_a = 1.0f;
                effect_scale_b = 0.0f;
            }
            selected_effect_tag = *(datum_index *)((uint8_t *)&effects[chosen_index] + (effect_variant + 3) * 0x10);
            selected_damage_tag = *(datum_index *)((uint8_t *)&effects[chosen_index] + (effect_variant + 6) * 0x10);
        }
    }

    if (!has_ammo) {
        goto tail;
    }

    if ((id->flags & _item_held_by_player_bit) != 0 && halo::game::globals().current_engine != 0) {
        datum_index player = halo::game::player_index_from_unit_index(holder_index);

        if (player != (datum_index)0xffffffff) {
            halo::game::unit_update_active_camouflage_depower(player);
        }
    }

    wd->last_fire_game_time = halo::game::globals().game_time->game_time;

    {
        int8_t action = is_misfire ? (int8_t)((trigger_index != 0) + 2) : (int8_t)(trigger_index != 0);
        uint32_t action_handle = halo::interface::local_player_index_for_weapon(item_index);
        halo::interface::first_person_weapon_process_action(action_handle, action);
        if ((int16_t)action_handle == -1) {
            halo::interface::hud_play_pickup_notification(item_index, (int16_t)action);
        }
    }

    if (tag_trigger->ejection_port_recovery_time > 0.0f && (*(uint8_t *)&tag_trigger->flags & 0x80) == 0) {
        trigger->ejection_port_recovery = 1.0f;
    }
    if (tag_trigger->illumination_recovery_time > 0.0f) {
        trigger->illumination_recovery = 1.0f;
    }

    if (weapon_infinite_ammo == 0) {
        wd->heat = wd->heat + tag_trigger->heat_generated_per_round;
    }
    if ((id->flags & _item_held_by_player_bit) != 0 || wd->heat <= weapon_tag->overheated_threshold) {
        if (wd->heat > 1.0f) {
            wd->heat = 1.0f;
        }
    } else {
        wd->heat = weapon_tag->overheated_threshold;
    }

    if ((id->flags & _item_held_by_player_bit) != 0 && weapon_bottomless_clip == 0) {
        real new_age = wd->age + tag_trigger->age_generated_per_round;
        wd->age = new_age;
        if (new_age > 1.0f) {
            wd->age = 1.0f;
            item_obj->flags = item_obj->flags | _object_changed_bit;
        }
    }

    halo::items::weapon_set_state(item_index, (trigger_index != 0) + 1, 0);

    if (!is_misfire) {
        if (is_alternate_shot) {
            wd->alternate_shots_loaded = wd->alternate_shots_loaded + 1;
        } else {
            int32_t role;
            int32_t create_locally = 1;

            if (halo::networking::globals().game_mode == 1) {
                if ((tag_trigger->flags & 0x2000) != 0 && weapon_client_side_projectiles == 1) {
                    role = 3;
                } else {
                    create_locally = 0;
                    role = 0;
                }
            } else if (halo::networking::globals().game_mode == 2 && ((tag_trigger->flags & 0x2000) == 0 || weapon_client_side_projectiles != 1)) {
                role = 0;
            } else {
                role = 3;
            }
            if (create_locally) {
                halo::items::trigger_create_projectiles(item_index, trigger_index, role);
            }
            halo::ai::ai_refresh_unit_stimulus_and_alert(holder_index, *(int16_t *)&((struct WeaponTrigger *)tag_trigger)->firing_noise, 1);
        }
    }

    if (holder_index != (datum_index)0xffffffff && selected_damage_tag != (datum_index)0xffffffff) {
        object *holder_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)holder_index].data;
        uint8_t *holder_bytes = (uint8_t *)holder_obj;
        damage_data dd;
        int32_t *zero = (int32_t *)&dd;
        int32_t i;

        for (i = 0; i < (int32_t)(sizeof(dd) / sizeof(int32_t)); i++) {
            zero[i] = 0;
        }
        dd.damage_effect_tag = selected_damage_tag;
        dd.flags = dd.flags | 8;
        dd.responsible_player = (datum_index)0xffffffff;
        dd.responsible_object = (datum_index)0xffffffff;
        dd.team_index = -1;
        dd.material_type = -1;
        dd.unknown_1a = -1;
        dd.location_cluster_index = -1;
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.direction.i = -*(real *)(holder_bytes + 0x23c);
        dd.direction.j = -*(real *)(holder_bytes + 0x240);
        dd.direction.k = -*(real *)(holder_bytes + 0x244);
        dd.epicentre = holder_obj->bounding_center;
        dd.origin = holder_obj->bounding_center;
        halo::objects::object_apply_damage(&dd, holder_index, -1, -1, -1, 0);
    }

    if (weapon_tag->weapon_type == 3 && trigger_index == 1) {
        wd->flags = wd->flags | 4;
    }

tail:
    if (weapon_tag->heat_detonation_threshold < wd->heat) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((real)(halo::math::globals().random_seed_global >> 0x10) * halo::k_unit_word_scale < weapon_tag->heat_detonation_fraction) {
            halo::items::weapon_reload_recovery_finish(item_index);
        }
    }

    if (has_ammo) {
        if (trigger->effect_state != _weapon_trigger_effect_spewing || is_misfire) {
            if ((tag_trigger->flags & 1) == 0) {
                halo::items::weapon_trigger_finish_shot(item_index, trigger_index);
            } else {
                trigger->effect_state = _weapon_trigger_effect_tracking;
                trigger->effect_state_ticks = -1;
            }
        }
    } else {
        trigger->effect_state = _weapon_trigger_effect_out_of_ammo;
        trigger->effect_state_ticks = -1;
    }

    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_not_pulled_bit;
    return halo::items::weapon_play_trigger_tag_effect(item_index, selected_effect_tag, effect_scale_a, effect_scale_b);
}

}

namespace halo::items {

uint32_t weapon_fire_trigger(datum_index item_index, int16_t trigger_index)
{
    return halo::items::weapon_ref(item_index).fire_trigger(trigger_index);
}

}
