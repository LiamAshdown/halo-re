#include "halo/units/records.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/game/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "networking.h"
#include "effects.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/items/vars.hpp"
#include "halo/units/vars.hpp"
static constexpr float k_look_blend_old = 0.7f;
static constexpr float k_look_blend_new = 0.3f;

static auto &object_network_id_table = halo::link::ref<uint8_t *>(halo::units::vars().object_network_id_table);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &unit_updates_suppressed = halo::link::ref<uint8_t>(halo::units::vars().unit_updates_suppressed);
static auto &ai_update_stagger = halo::link::ref<uint8_t *>(halo::units::vars().ai_update_stagger);
static auto &weapon_bottomless_clip = halo::link::ref<uint8_t>(halo::items::vars().weapon_bottomless_clip);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &s_stand = halo::link::ref<char *>(halo::units::vars().s_stand);
static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace halo::units {

/**
 * Dispatches a local scripted event (id 9) built from a hashed lookup value and a byte parameter, and on
 * success forwards it through network_session_broadcast_to_flagged.
 *
 * Original register convention: param_1, in_ECX.
 *
 * @address 0x56c370
 */
void halo::units::unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key)
{
    int32_t looked_up = 0;
    if (hash_key != -1) {
        looked_up = halo::objects::hash_table_get((hash_table *)(object_network_id_table + 0xc), hash_key);
        if (looked_up == -1) {
            looked_up = 0;
        }
    }

    struct { int32_t looked_up; uint8_t event_byte; } item;
    void *items[1];
    int32_t encoded_len;
    item.looked_up = looked_up;
    item.event_byte = event_byte;
    items[0] = &item;
    encoded_len = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 9, 0, items, 0, 1, 0);
    if (0 < encoded_len) {
        halo::networking::network_session_broadcast_to_flagged(encoded_len, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    }
    return;
}

/**
 * Engine function unit_update.
 *
 * @address 0x5625b0
 */
uint8_t UnitView::update()
{
    uint32_t unit_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    Unit *tag = halo::objects::tag_as<Unit>(*(datum_index *)obj);
    uint8_t over_budget = 0;
    uint8_t valid_team_player;
    uint8_t riding = 0;
    float speed_scale;
    float rate;
    float acceleration;
    real_vector3d previous_aim;
    real_point3d *zero_vector;

    valid_team_player = halo::game::game_engine_is_valid_team_player(unit_index);
    if (!unit_updates_suppressed) {
        uint8_t *stagger = ai_update_stagger;

        (obj->unit.update_tick_counter)++;
        if (stagger[4] == 0 && obj->unit.update_tick_counter > *(int16_t *)stagger) {
            stagger[4] = 1;
            over_budget = 1;
            obj->unit.update_tick_counter = 0;
        } else if (*(int16_t *)(stagger + 2) <= obj->unit.update_tick_counter) {
            *(int16_t *)(stagger + 2) = obj->unit.update_tick_counter;
        }
    }
    if (test_flag(obj->unit.flags, units::unit_flag::idle_turn_seeded)) {
        UnitView(unit_index).update_random_turn_angle((real_vector3d *)&obj->unit.desired_facing_vector);
        *(real_vector3d *)&obj->unit.desired_aiming_vector.i = *(real_vector3d *)&obj->unit.desired_facing_vector.i;
        *(real_vector3d *)&obj->unit.desired_looking_vector.i = *(real_vector3d *)&obj->unit.desired_facing_vector.i;
        *(real_vector3d *)&obj->unit.throttle.i = *halo::math::globals().global_forward3d_pointer;
        obj->unit.control_flags = 0;
    } else if (!test_flag(obj->unit.flags, units::unit_flag::unattended)) {
        *(real_vector3d *)&obj->unit.desired_looking_vector.i = *(real_vector3d *)&obj->base.forward.i;
        *(real_vector3d *)&obj->unit.desired_aiming_vector.i = *(real_vector3d *)&obj->base.forward.i;
        *(real_vector3d *)&obj->unit.desired_facing_vector.i = *(real_vector3d *)&obj->base.forward.i;
        *(real_point3d *)&obj->unit.throttle.i = *global_origin3d_pointer;
        obj->unit.control_flags = 0;
    }

    if (!test_flag(tag->unit_flags, tags::unit_tag_flag::simple_creature)) {
        int32_t ticks = obj->unit.persistent_control_ticks;

        if (ticks > 0) {
            uint32_t bits = obj->unit.persistent_control_flags;
            uint32_t control = obj->unit.control_flags | bits;

            if (test_flag(bits, units::unit_control_flag::primary_trigger)) {
                control = (ticks % 7 == 0) ? (control | 0x800) : (control & ~0x800u);
                obj->unit.primary_trigger = 1.0f;
            } else {
                obj->unit.primary_trigger = 0.0f;
            }
            obj->unit.control_flags = control;
            obj->unit.persistent_control_ticks = --ticks;
            if (ticks == 0) {
                obj->unit.persistent_control_flags = 0;
            }
        }
        if (!test_flag(obj->unit.flags, units::unit_flag::possessed_by_recording)) {
            datum_index driver = obj->unit.driver_unit_index;
            datum_index gunner = obj->unit.gunner_unit_index;

            if (driver != k_datum_index_none && !test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) {
                unit_object *d = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(driver));

                obj->base.owner_team = d->base.owner_team;
                riding = 1;
                if (d->unit.controlling_player != k_datum_index_none || ((uint8_t)d->unit.animation_state != animation_state_value(unit_animation_state_id::seat_exit) && (uint8_t)d->unit.animation_state != animation_state_value(unit_animation_state_id::seat_enter))) {
                    obj->unit.control_flags |= d->unit.control_flags & 0x3f;
                    *(real_vector3d *)&obj->unit.desired_facing_vector.i = d->unit.desired_facing_vector;
                    *(real_point3d *)&obj->unit.throttle.i = *(real_point3d *)&d->unit.throttle.i;
                }
            }
            if (gunner != k_datum_index_none && !test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) {
                unit_object *g = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(gunner));

                if (!riding) {
                    obj->base.owner_team = g->base.owner_team;
                }
                if (g->unit.controlling_player != k_datum_index_none || ((uint8_t)g->unit.animation_state != animation_state_value(unit_animation_state_id::seat_exit) && (uint8_t)g->unit.animation_state != animation_state_value(unit_animation_state_id::seat_enter))) {
                    *(real_vector3d *)&obj->unit.desired_aiming_vector.i = g->unit.desired_aiming_vector;
                    *(real_vector3d *)&obj->unit.desired_looking_vector.i = g->unit.desired_aiming_vector;
                    obj->unit.control_flags |= g->unit.control_flags & 0x7c00;
                    obj->unit.primary_trigger = g->unit.primary_trigger;
                }
            }
            if (test_flag(obj->unit.control_flags, units::unit_control_flag::reload | units::unit_control_flag::primary_trigger | units::unit_control_flag::secondary_trigger | units::unit_control_flag::grenade | units::unit_control_flag::exchange_weapon)) {
                obj->unit.weapon_control_idle_ticks = 0;
            } else if ((int8_t)(uint8_t)obj->unit.weapon_control_idle_ticks < 0x7f) {
                obj->unit.weapon_control_idle_ticks++;
            }
        }
        if (!unit_updates_suppressed) {
            if (test_flag(obj->unit.flags, units::unit_flag::active_camouflaged)) {
                float step = 0.008333334f;

                if (halo::game::globals().current_engine != 0 && obj->unit.active_camouflage_regrowth != 0 && obj->unit.active_camouflage_regrowth == 1) {
                    datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.current_weapon_index);

                    if (weapon != k_datum_index_none) {
                        Weapon *weapon_tag = halo::objects::tag_as<Weapon>(*(datum_index *)halo::objects::object_record_bytes(weapon));

                        if (weapon_tag->active_camo_regrowth_rate != 0.0f) {
                            step = weapon_tag->active_camo_regrowth_rate;
                        }
                    }
                }
                obj->unit.active_camouflage_power += step;
                if (obj->unit.active_camouflage_power > 1.0f) {
                    obj->unit.active_camouflage_power = 1.0f;
                    obj->unit.active_camouflage_regrowth = 0;
                }
            } else {
                obj->unit.active_camouflage_power -= 0.008333334f;
                if (obj->unit.active_camouflage_power < 0.0f) {
                    obj->unit.active_camouflage_power = 0.0f;
                }
            }
            if (test_flag(obj->unit.flags, units::unit_flag::super_camouflaged)) {
                obj->unit.super_active_camouflage_power += 0.011111111f;
                if (obj->unit.super_active_camouflage_power > 1.0f) {
                    obj->unit.super_active_camouflage_power = 1.0f;
                }
            } else {
                obj->unit.super_active_camouflage_power -= 0.011111111f;
                if (obj->unit.super_active_camouflage_power < 0.0f) {
                    obj->unit.super_active_camouflage_power = 0.0f;
                }
            }
            if (obj->unit.stun_ticks > 0 && --obj->unit.stun_ticks == 0) {
                *(int32_t *)&obj->unit.stun = 0;
            }
            bool skip_feign_death = false;
            if ((int8_t)(uint8_t)obj->unit.delayed_weapon_drop_ticks > 0 && --obj->unit.delayed_weapon_drop_ticks == 0) {
                UnitView(unit_index).drop_current_weapon(1);
                skip_feign_death = unit_updates_suppressed;
            }
            if (!skip_feign_death && obj->unit.feign_death_ticks > 0 && test_flag(obj->base.flags, objects::object_flag::at_rest) && --obj->unit.feign_death_ticks == 0) {
                if (obj->base.body_vitality > 0.0f) {
                    int16_t state = (int16_t)((~((uint8_t)obj->unit.animation_state_flags >> 3) & 1) | 0x22);

                    clear_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen);
                    UnitView(unit_index).refresh_targeting_flag_and_weapons(1);
                    UnitView(unit_index).set_or_test_seat_and_weapon_label(s_stand, 0, 1);
                    UnitView(unit_index).try_set_animation_state(state);
                    clear_flag(obj->unit.animation_state_flags, units::unit_animation_state_flag::unknown_4);
                    if (obj->base.type == _object_type_biped) {
                        UnitView(unit_index).clear_ground_adjust_dirty();
                    }
                    UnitView(unit_index).dispatch_reaction_animation(5);
                } else {
                    UnitView(unit_index).release_transient_state(0);
                }
            }
        }
    }

    if (!test_flag(tag->unit_flags, tags::unit_tag_flag::has_no_aiming)) {
        if (!test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen) && !unit_updates_suppressed) {
            if (test_flag(obj->base.vitality_flags, objects::vitality_flag::region_response_400)) {
                UnitView(unit_index).drop_current_weapon(1);
            } else if (obj->unit.desired_weapon_index != obj->unit.current_weapon_index &&
                       !::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(obj))) {
                datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.desired_weapon_index);

                if (weapon != k_datum_index_none && UnitView(unit_index).check_weapon_use_permission(weapon)) {
                    UnitView(unit_index).ready_desired_weapon(1);
                }
            }
            if ((uint8_t)obj->unit.desired_grenade_index != (uint8_t)obj->unit.current_grenade_index && !::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(obj))) {
                int16_t grenade = UnitView(unit_index).find_next_grenade_type_with_count((int16_t)(int8_t)(uint8_t)obj->unit.desired_grenade_index, 0);

                if (grenade != -1) {
                    obj->unit.current_grenade_index = (uint8_t)grenade;
                }
            }
            if (weapon_bottomless_clip && obj->unit.controlling_player != k_datum_index_none) {
                int32_t i;

                for (i = 0; i < 2; i++) {
                    if ((int8_t)reinterpret_cast<uint8_t *>(obj)[0x31e + i] <= 1) {
                        reinterpret_cast<uint8_t *>(obj)[0x31e + i] = 1;
                    }
                }
                if ((uint8_t)obj->unit.desired_grenade_index == 0xff) {
                    obj->unit.desired_grenade_index = 0;
                }
            }
            if ((uint8_t)obj->unit.desired_zoom_level != (uint8_t)obj->unit.zoom_level) {
                obj->unit.zoom_level = (uint8_t)obj->unit.desired_zoom_level;
                if ((uint8_t)obj->unit.zoom_level == 0xff) {
                    *(int32_t *)&obj->unit.integrated_night_vision_power = 0;
                }
                if (halo::game::player_index_from_unit_index(unit_index) != k_datum_index_none &&
                    halo::game::player_at(halo::game::player_index_from_unit_index(unit_index))->local_player_index != -1) {
                    datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.current_weapon_index);

                    if (weapon != k_datum_index_none) {
                        Weapon *weapon_tag = halo::objects::tag_as<Weapon>(*(datum_index *)halo::objects::object_record_bytes(weapon));
                        datum_index sound = ((uint8_t)obj->unit.zoom_level == 0xff) ? halo::objects::tag_handle(weapon_tag->zoom_out_sound)
                                                                 : halo::objects::tag_handle(weapon_tag->zoom_in_sound);
                        float fraction = 1.0f;

                        if ((uint8_t)obj->unit.zoom_level != 0xff && weapon_tag->zoom_levels > 1) {
                            fraction = (float)(int8_t)(uint8_t)obj->unit.zoom_level / (float)(weapon_tag->zoom_levels - 1);
                        }
                        if (sound != k_datum_index_none) {
                            halo::sound::sound_start_unspatialized(sound, fraction);
                        }
                    }
                }
            }
        }

        speed_scale = ((uint8_t)obj->unit.aiming_speed == 1) ? tag->casual_aiming_modifier : 1.0f;
        rate = speed_scale * tag->aiming_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * tag->aiming_acceleration_maximum * 0.0011111111f;
        previous_aim = *(real_vector3d *)&obj->unit.aiming_vector.i;
        zero_vector = global_zero_vector3d_pointer;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&obj->unit.aiming_vector.i = *(real_vector3d *)&obj->unit.desired_aiming_vector.i;
            if (UnitView(unit_index).is_look_target_valid()) {
                UnitView(unit_index).clamp_direction_to_aim_or_look_bounds((real_vector3d *)&obj->unit.aiming_vector, 1);
            }
            *(real_point3d *)&obj->unit.aiming_velocity.i = *global_origin3d_pointer;
        } else if ((uint8_t)obj->unit.aiming_bounds_valid != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            halo::objects::object_get_orientation(&basis.forward, unit_index, &basis.up);
            halo::math::vector3d_cross_product(basis.left, basis.forward, basis.up);
            basis.position = *zero_vector;
            halo::math::vector3d_rotate_toward_bounded((real_vector3d *)&obj->unit.aiming_vector, (real_vector3d *)&obj->unit.aiming_velocity,
                (real *)&obj->unit.aiming_bounds, rate, acceleration, *((real_vector3d *)&obj->unit.desired_aiming_vector), &basis);
        } else {
            halo::math::vector3d_rotate_toward_with_acceleration((real_vector3d *)&obj->unit.aiming_vector, *((real_vector3d *)&obj->unit.desired_aiming_vector),
                *((real_vector3d *)&obj->unit.aiming_velocity), rate, acceleration);
        }
        {
            float change = 0.0f;

            if (tag->aiming_velocity_maximum != 0.0f) {
                change = halo::math::vector3d_angle_between_4cd4f0(previous_aim, *((real_vector3d *)&obj->unit.aiming_vector)) /
                    (tag->aiming_velocity_maximum * 0.033333335f);
                if (change < 0.0f) {
                    change = 0.0f;
                } else if (change > 1.0f) {
                    change = 1.0f;
                }
            }
            obj->unit.aiming_change = (uint8_t)(int32_t)(change * 255.0f);
        }
        rate = speed_scale * tag->looking_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * tag->looking_acceleration_maximum * 0.0011111111f;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&obj->unit.looking_vector.i = *(real_vector3d *)&obj->unit.desired_looking_vector.i;
            UnitView(unit_index).clamp_direction_to_aim_or_look_bounds((real_vector3d *)&obj->unit.looking_vector, 0);
            *(real_point3d *)&obj->unit.looking_velocity.i = *global_origin3d_pointer;
        } else if ((uint8_t)obj->unit.looking_bounds_valid != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            halo::objects::object_get_orientation(&basis.forward, unit_index, &basis.up);
            halo::math::vector3d_cross_product(basis.left, basis.forward, basis.up);
            basis.position = *zero_vector;
            halo::math::vector3d_rotate_toward_bounded((real_vector3d *)&obj->unit.looking_vector, (real_vector3d *)&obj->unit.looking_velocity,
                (real *)&obj->unit.looking_bounds, rate, acceleration, *((real_vector3d *)&obj->unit.desired_looking_vector), &basis);
        } else {
            halo::math::vector3d_rotate_toward_with_acceleration((real_vector3d *)&obj->unit.looking_vector, *((real_vector3d *)&obj->unit.desired_looking_vector),
                *((real_vector3d *)&obj->unit.looking_velocity), rate, acceleration);
        }

        if (!unit_updates_suppressed) {
            uint8_t throwing = (uint8_t)((obj->unit.control_flags >> 13) & 1);

            switch ((int8_t)(uint8_t)obj->unit.throwing_grenade_state) {
            case _unit_throwing_grenade_state_none:
                if (throwing) {
                    UnitView(unit_index).begin_throw_grenade(0);
                }
                break;
            case _unit_throwing_grenade_state_begin:
                if (obj->base.animation_frame >= 2) {
                    UnitView(unit_index).throw_grenade_move_to_hand();
                }
                break;
            case _unit_throwing_grenade_state_in_hand:
                (obj->unit.throwing_grenade_counter)++;
                if ((uint8_t)obj->unit.animation_state != animation_state_value(unit_animation_state_id::throwing_grenade)) {
                    UnitView(unit_index).release_thrown_grenade(1);
                }
                break;
            case _unit_throwing_grenade_state_released:
                if ((uint8_t)obj->unit.animation_state != animation_state_value(unit_animation_state_id::throwing_grenade) && !throwing) {
                    obj->unit.throwing_grenade_state = _unit_throwing_grenade_state_none;
                }
                break;
            default:
                break;
            }
        }
        if (obj->unit.current_weapon_index != -1 && !unit_updates_suppressed) {
            uint32_t control = 0;
            float trigger = obj->unit.primary_trigger;
            unit_object *unit_now;
            datum_index weapon = k_datum_index_none;

            if (obj->unit.current_weapon_index == obj->unit.desired_weapon_index) {
                uint8_t flashing = (uint8_t)(obj->unit.persistent_control_ticks > 0 && test_flag(obj->unit.persistent_control_flags, units::unit_control_flag::primary_trigger));

                if (valid_team_player && test_flag(obj->unit.control_flags, units::unit_control_flag::integrated_light)) {
                    control = 1;
                }
                if (test_flag(obj->unit.control_flags, units::unit_control_flag::primary_trigger)) {
                    control |= 2;
                }
                if (test_flag(obj->unit.control_flags, units::unit_control_flag::secondary_trigger)) {
                    control |= 4;
                }
                if (test_flag(halo::objects::tag_as<Unit>(*(datum_index *)obj)->unit_flags, tags::unit_tag_flag::integrated_light_cntrls_weapon)) {
                    halo::items::weapon_set_ready_timer(UnitView(unit_index).get_weapon_object_index(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.current_weapon_index), obj->unit.integrated_light_power);
                }
                if (test_flag(obj->unit.control_flags, units::unit_control_flag::reload)) {
                    control |= 8;
                }
                if (::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(obj)) && !flashing) {
                    control |= 0x10;
                }
                if (obj->base.type == _object_type_biped && (int8_t)halo::units::biped_data_of(obj)->melee_ticks > 0) {
                    control |= 0x10;
                }
                if ((uint8_t)obj->unit.zoom_level != 0xff) {
                    control |= 0x40;
                }
            } else {
                control = 0x20;
            }
            unit_now = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
            if (unit_now->unit.current_weapon_index != -1) {
                weapon = unit_now->unit.weapons[unit_now->unit.current_weapon_index];
            }
            halo::items::weapon_set_control_flags(weapon, (uint16_t)control, trigger);
        }
    }

    if (!test_flag(tag->unit_flags, tags::unit_tag_flag::simple_creature)) {
        int16_t seat;

        if (test_flag(obj->unit.animation_state_flags, units::unit_animation_state_flag::aiming_enabled)) {
            UnitView(unit_index).update_look_delta_controls();
            obj->unit.animation_controls_smoothed[0] = obj->unit.animation_controls[0] * k_look_blend_new + obj->unit.animation_controls_smoothed[0] * k_look_blend_old;
            obj->unit.animation_controls_smoothed[1] = obj->unit.animation_controls[1] * k_look_blend_new + obj->unit.animation_controls_smoothed[1] * k_look_blend_old;
            obj->unit.animation_controls_smoothed[2] = obj->unit.animation_controls[2] * k_look_blend_new + obj->unit.animation_controls_smoothed[2] * k_look_blend_old;
        }
        for (seat = 0; seat < (int32_t)tag->powered_seats.count; seat++) {
            UnitPoweredSeat *powered = &halo::objects::block_element<UnitPoweredSeat>(tag->powered_seats, seat);
            float *power = (float *)(reinterpret_cast<uint8_t *>(obj) + 0x338 + seat * 4);
            uint8_t occupied;

            if (seat == 0) {
                occupied = (uint8_t)(obj->unit.driver_unit_index != k_datum_index_none || test_flag(obj->unit.flags, units::unit_flag::unattended));
            } else {
                occupied = (uint8_t)(obj->unit.gunner_unit_index != k_datum_index_none &&
                    obj->unit.gunner_unit_index != obj->unit.driver_unit_index);
            }
            if (!test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen) && occupied) {
                if (*power != 1.0f) {
                    *power += 1.0f / (powered->driver_powerup_time * 30.0f);
                    if (*power > 1.0f) {
                        *power = 1.0f;
                    }
                }
            } else if (*power != 0.0f) {
                *power -= 1.0f / (powered->driver_powerdown_time * 30.0f);
                if (*power < 0.0f) {
                    *power = 0.0f;
                }
            }
        }
    }
    if (obj->unit.delayed_damage_ticks > 0 && --obj->unit.delayed_damage_ticks == 0) {
        halo::ai::actor_react_to_threat_event(unit_index, obj->unit.delayed_damage_responsible_object, *(uint16_t *)&obj->unit.delayed_damage_category,
            obj->unit.delayed_damage_amount, 0, 1);
        obj->unit.delayed_damage_category = 0;
        obj->unit.delayed_damage_responsible_object = k_datum_index_none;
        *(int32_t *)&obj->unit.delayed_damage_amount = 0;
    }
    if (!unit_updates_suppressed) {
        UnitView(unit_index).melee_lunge_damage_tick();
        if (!unit_updates_suppressed) {
            UnitView(unit_index).update_animation_timers();
            if (!unit_updates_suppressed && (over_budget || obj->unit.controlling_player != k_datum_index_none)) {
                UnitView(unit_index).calculate_luminosity();
            }
        }
    }
    if ((uint8_t)obj->unit.flaming_ticks != 0) {
        if (unit_updates_suppressed) {
            return 1;
        }
        if (--obj->unit.flaming_ticks == 0) {
            UnitView(unit_index).update_autoaim_interaction();
            if (unit_updates_suppressed) {
                return 1;
            }
        }
    } else if (unit_updates_suppressed) {
        return 1;
    }
    {
        float step = -obj->unit.mouth_aperture;

        if (step < -0.1f) {
            step = -0.1f;
        } else if (step > 0.1f) {
            step = 0.1f;
        }
        obj->unit.mouth_aperture += step;
    }
    {
        uint8_t toggle = 0;
        uint32_t flags = obj->unit.flags;
        uint32_t button;

        if (test_flag(flags, units::unit_flag::desired_integrated_light_on)) {
            if (!test_flag(flags, units::unit_flag::integrated_light_on)) {
                toggle = 1;
            }
            obj->unit.flags = flags & ~halo::to_bits(units::unit_flag::desired_integrated_light_on);
        }
        flags = obj->unit.flags;
        if (test_flag(flags, units::unit_flag::desired_integrated_light_off)) {
            if (test_flag(flags, units::unit_flag::integrated_light_on)) {
                toggle = 1;
            }
            obj->unit.flags = flags & ~halo::to_bits(units::unit_flag::desired_integrated_light_off);
        }
        button = obj->unit.control_flags & 0x10;
        if (button != 0 || !(obj->unit.integrated_light_energy > 0.0f) || toggle) {
            if (!valid_team_player) {
                flags = obj->unit.flags;
                if (test_flag(flags, units::unit_flag::integrated_night_vision_on)) {
                    obj->unit.flags = flags & ~halo::to_bits(units::unit_flag::integrated_night_vision_on);
                }
                flags = obj->unit.flags;
                if (test_flag(flags, units::unit_flag::integrated_light_on)) {
                    obj->unit.flags = (flags & 0xfff7ffff) | 0x10;
                }
            } else {
                uint8_t toggle_light = 1;

                if (UnitView(unit_index).current_weapon_has_flag()) {
                    if (button != 0) {
                        GlobalsFirstPersonInterface *effects = halo::objects::block_elements<GlobalsFirstPersonInterface>(global_globals->first_person_interface);
                        datum_index effect = (test_flag(obj->unit.flags, units::unit_flag::integrated_night_vision_on))
                            ? halo::objects::tag_handle(effects->night_vision_off_effect) : halo::objects::tag_handle(effects->night_vision_on_effect);

                        if (effect != k_datum_index_none) {
                            halo::effects::effect_new_on_object(unit_index, effect, unit_index, -1, 0.0f, 0.0f, 0, 0);
                        }
                        obj->unit.flags ^= 0x4000000;
                    }
                    if (test_flag(obj->unit.control_flags, units::unit_control_flag::integrated_light)) {
                        toggle_light = 0;
                    }
                }
                if (toggle_light && (test_flag(obj->unit.flags, units::unit_flag::integrated_light_on) || obj->unit.integrated_light_energy > 0.2f) &&
                    obj->base.parent_object == k_datum_index_none) {
                    halo::effects::effect_new_on_object(unit_index, halo::objects::tag_handle(tag->integrated_light_toggle), unit_index, -1, 0.0f, 0.0f, 0, 0);
                    obj->unit.flags ^= 0x80000;
                }
            }
        }
        flags = obj->unit.flags;
        if (test_flag(flags, units::unit_flag::integrated_light_on)) {
            if (!test_flag(tag->unit_flags, tags::unit_tag_flag::integrated_light_lasts_forever)) {
                obj->unit.integrated_light_energy -= 0.00027777778f;
            }
            if (obj->base.parent_object != k_datum_index_none || test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) {
                obj->unit.flags = flags & ~halo::to_bits(units::unit_flag::integrated_light_on);
            }
            if (obj->unit.integrated_light_power != 1.0f) {
                obj->unit.integrated_light_power += 0.16666667f;
                if (obj->unit.integrated_light_power > 1.0f) {
                    obj->unit.integrated_light_power = 1.0f;
                }
            }
        } else {
            if (obj->unit.integrated_light_energy < 1.0f) {
                obj->unit.integrated_light_energy += 0.0011111111f;
            }
            if (obj->unit.integrated_light_power != 0.0f) {
                obj->unit.integrated_light_power -= 0.041666668f;
                if (obj->unit.integrated_light_power < 0.0f) {
                    obj->unit.integrated_light_power = 0.0f;
                }
            }
        }
    }
    if (UnitView(unit_index).current_weapon_has_flag()) {
        if (test_flag(obj->unit.flags, units::unit_flag::integrated_night_vision_on)) {
            if (obj->unit.integrated_night_vision_power != 1.0f) {
                obj->unit.integrated_night_vision_power += 0.083333336f;
                if (obj->unit.integrated_night_vision_power > 1.0f) {
                    obj->unit.integrated_night_vision_power = 1.0f;
                }
            }
        } else if (obj->unit.integrated_night_vision_power != 0.0f) {
            obj->unit.integrated_night_vision_power -= 0.041666668f;
            if (obj->unit.integrated_night_vision_power < 0.0f) {
                obj->unit.integrated_night_vision_power = 0.0f;
            }
        }
    }

    return 1;
}

}
