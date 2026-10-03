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

extern "C" {
extern uint8_t *object_network_id_table;
extern uint8_t event9_target;
extern int32_t network_role_0071c2d4;
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern data_array *object_data;
extern data_array *player_data;
extern uint8_t unit_updates_suppressed;
extern uint8_t *ai_update_stagger;
extern game_engine_definition *current_game_engine;
extern uint8_t weapon_bottomless_clip;
extern Globals *global_globals;
extern char *s_stand;
extern real_point3d *global_origin3d_pointer;
extern real_point3d *global_zero_vector3d_pointer;
extern void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay);
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const void *color, const void *tint_source);
extern uint8_t game_engine_is_valid_team_player(uint32_t identifier);
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger);
extern void weapon_set_ready_timer(datum_index item_index, real value);
extern datum_index sound_start_unspatialized(datum_index definition_index, float scale);
}

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
    encoded_len = message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 9, 0, items, 0, 1, 0);
    if (0 < encoded_len) {
        network_session_broadcast_to_flagged(encoded_len, network_server, 1, network_message_scratch, 1, 0, 0, 3);
    }
    return;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot((t))].data)
#define LOOK_BLEND_NEW 0.3f
#define LOOK_BLEND_OLD 0.7f
/**
 * Engine function unit_update.
 *
 * @address 0x5625b0
 */
uint8_t UnitView::update()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *tag = TAG_DATA(*(datum_index *)obj);
    uint8_t over_budget = 0;
    uint8_t valid_team_player;
    uint8_t riding = 0;
    float speed_scale;
    float rate;
    float acceleration;
    real_vector3d previous_aim;
    real_point3d *zero_vector;

    valid_team_player = game_engine_is_valid_team_player(unit_index);
    if (!unit_updates_suppressed) {
        uint8_t *stagger = ai_update_stagger;

        (((unit_object *)obj)->unit.update_tick_counter)++;
        if (stagger[4] == 0 && ((unit_object *)obj)->unit.update_tick_counter > *(int16_t *)stagger) {
            stagger[4] = 1;
            over_budget = 1;
            ((unit_object *)obj)->unit.update_tick_counter = 0;
        } else if (*(int16_t *)(stagger + 2) <= ((unit_object *)obj)->unit.update_tick_counter) {
            *(int16_t *)(stagger + 2) = ((unit_object *)obj)->unit.update_tick_counter;
        }
    }
    if (test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::idle_turn_seeded)) {
        UnitView(unit_index).update_random_turn_angle((real_vector3d *)&((struct unit_object *)obj)->unit.desired_facing_vector);
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.throttle.i = *halo::math::globals().global_forward3d_pointer;
        ((unit_object *)obj)->unit.control_flags = 0;
    } else if (!test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unattended)) {
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *global_origin3d_pointer;
        ((unit_object *)obj)->unit.control_flags = 0;
    }

    if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::simple_creature)) {
        int32_t ticks = ((struct unit_object *)obj)->unit.persistent_control_ticks;

        if (ticks > 0) {
            uint32_t bits = ((struct unit_object *)obj)->unit.persistent_control_flags;
            uint32_t control = ((unit_object *)obj)->unit.control_flags | bits;

            if (test_flag(bits, units::unit_control_flag::primary_trigger)) {
                control = (ticks % 7 == 0) ? (control | 0x800) : (control & ~0x800u);
                ((unit_object *)obj)->unit.primary_trigger = 1.0f;
            } else {
                ((unit_object *)obj)->unit.primary_trigger = 0.0f;
            }
            ((unit_object *)obj)->unit.control_flags = control;
            ((struct unit_object *)obj)->unit.persistent_control_ticks = --ticks;
            if (ticks == 0) {
                ((struct unit_object *)obj)->unit.persistent_control_flags = 0;
            }
        }
        if (!test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_8000000)) {
            datum_index driver = ((unit_object *)obj)->unit.driver_unit_index;
            datum_index gunner = ((unit_object *)obj)->unit.gunner_unit_index;

            if (driver != k_datum_index_none && !test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
                uint8_t *d = OBJECT_DATA(driver);

                ((unit_object *)obj)->base.owner_team = ((struct object *)d)->owner_team;
                riding = 1;
                if (((struct unit_object *)d)->unit.controlling_player != k_datum_index_none || ((uint8_t)((struct unit_object *)d)->unit.animation_state != 0x1b && (uint8_t)((struct unit_object *)d)->unit.animation_state != 0x1a)) {
                    ((unit_object *)obj)->unit.control_flags |= ((struct unit_object *)d)->unit.control_flags & 0x3f;
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = ((struct unit_object *)d)->unit.desired_facing_vector;
                    *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *(real_point3d *)(d + 0x278);
                }
            }
            if (gunner != k_datum_index_none && !test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
                uint8_t *g = OBJECT_DATA(gunner);

                if (!riding) {
                    ((unit_object *)obj)->base.owner_team = ((struct object *)g)->owner_team;
                }
                if (((struct unit_object *)g)->unit.controlling_player != k_datum_index_none || ((uint8_t)((struct unit_object *)g)->unit.animation_state != 0x1b && (uint8_t)((struct unit_object *)g)->unit.animation_state != 0x1a)) {
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = ((struct unit_object *)g)->unit.desired_aiming_vector;
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = ((struct unit_object *)g)->unit.desired_aiming_vector;
                    ((unit_object *)obj)->unit.control_flags |= ((struct unit_object *)g)->unit.control_flags & 0x7c00;
                    ((unit_object *)obj)->unit.primary_trigger = ((struct unit_object *)g)->unit.primary_trigger;
                }
            }
            if (test_flag(((unit_object *)obj)->unit.control_flags, units::unit_control_flag::reload | units::unit_control_flag::primary_trigger | units::unit_control_flag::secondary_trigger | units::unit_control_flag::grenade | units::unit_control_flag::exchange_weapon)) {
                ((struct unit_object *)obj)->unit.weapon_control_idle_ticks = 0;
            } else if ((int8_t)(uint8_t)((struct unit_object *)obj)->unit.weapon_control_idle_ticks < 0x7f) {
                ((struct unit_object *)obj)->unit.weapon_control_idle_ticks++;
            }
        }
        if (!unit_updates_suppressed) {
            if (test_flag(((struct unit_object *)obj)->unit.flags, units::unit_flag::unknown_10)) {
                float step = 0.008333334f;

                if (current_game_engine != 0 && ((struct unit_object *)obj)->unit.active_camouflage_regrowth != 0 && ((struct unit_object *)obj)->unit.active_camouflage_regrowth == 1) {
                    datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)OBJECT_DATA(unit_index))->unit.current_weapon_index);

                    if (weapon != k_datum_index_none) {
                        uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));

                        if (*(float *)(weapon_tag + 0x4d0) != 0.0f) {
                            step = *(float *)(weapon_tag + 0x4d0);
                        }
                    }
                }
                ((struct unit_object *)obj)->unit.active_camouflage_power += step;
                if (((struct unit_object *)obj)->unit.active_camouflage_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.active_camouflage_power = 1.0f;
                    ((struct unit_object *)obj)->unit.active_camouflage_regrowth = 0;
                }
            } else {
                ((struct unit_object *)obj)->unit.active_camouflage_power -= 0.008333334f;
                if (((struct unit_object *)obj)->unit.active_camouflage_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.active_camouflage_power = 0.0f;
                }
            }
            if (test_flag(((struct unit_object *)obj)->unit.flags, units::unit_flag::unknown_20)) {
                ((struct unit_object *)obj)->unit.super_active_camouflage_power += 0.011111111f;
                if (((struct unit_object *)obj)->unit.super_active_camouflage_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.super_active_camouflage_power = 1.0f;
                }
            } else {
                ((struct unit_object *)obj)->unit.super_active_camouflage_power -= 0.011111111f;
                if (((struct unit_object *)obj)->unit.super_active_camouflage_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.super_active_camouflage_power = 0.0f;
                }
            }
            if (((struct unit_object *)obj)->unit.stun_ticks > 0 && --((struct unit_object *)obj)->unit.stun_ticks == 0) {
                *(int32_t *)&((struct unit_object *)obj)->unit.stun = 0;
            }
            if ((int8_t)(uint8_t)((struct unit_object *)obj)->unit.delayed_weapon_drop_ticks > 0 && --((struct unit_object *)obj)->unit.delayed_weapon_drop_ticks == 0) {
                UnitView(unit_index).drop_current_weapon(1);
                if (unit_updates_suppressed) {
                    goto controls;
                }
            }
            if (((struct unit_object *)obj)->unit.feign_death_ticks > 0 && test_flag(((struct object *)obj)->flags, objects::object_flag::at_rest) && --((struct unit_object *)obj)->unit.feign_death_ticks == 0) {
                if (((unit_object *)obj)->base.body_vitality > 0.0f) {
                    int16_t state = (int16_t)((~((uint8_t)((struct unit_object *)obj)->unit.animation_state_flags >> 3) & 1) | 0x22);

                    clear_flag(((unit_object *)obj)->base.vitality_flags, objects::vitality_flag::health_frozen);
                    UnitView(unit_index).refresh_targeting_flag_and_weapons(1);
                    UnitView(unit_index).set_or_test_seat_and_weapon_label(s_stand, 0, 1);
                    UnitView(unit_index).try_set_animation_state(state);
                    clear_flag(((unit_object *)obj)->unit.animation_state_flags, units::unit_animation_state_flag::unknown_4);
                    if (((unit_object *)obj)->base.type == 0) {
                        UnitView(unit_index).clear_ground_adjust_dirty();
                    }
                    UnitView(unit_index).dispatch_reaction_animation(5);
                } else {
                    UnitView(unit_index).release_transient_state(0);
                }
            }
        }
    }

controls:
    if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::has_no_aiming)) {
        if (!test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen) && !unit_updates_suppressed) {
            if (test_flag(((unit_object *)obj)->base.vitality_flags, objects::vitality_flag::region_response_400)) {
                UnitView(unit_index).drop_current_weapon(1);
            } else if (((unit_object *)obj)->unit.desired_weapon_index != ((unit_object *)obj)->unit.current_weapon_index &&
                       !::halo::units::unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset))) {
                datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)OBJECT_DATA(unit_index))->unit.desired_weapon_index);

                if (weapon != k_datum_index_none && UnitView(unit_index).check_weapon_use_permission(weapon)) {
                    UnitView(unit_index).ready_desired_weapon(1);
                }
            }
            if ((uint8_t)((struct unit_object *)obj)->unit.desired_grenade_index != (uint8_t)((struct unit_object *)obj)->unit.current_grenade_index && !::halo::units::unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset))) {
                int16_t grenade = UnitView(unit_index).find_next_grenade_type_with_count((int16_t)(int8_t)(uint8_t)((struct unit_object *)obj)->unit.desired_grenade_index, 0);

                if (grenade != -1) {
                    ((struct unit_object *)obj)->unit.current_grenade_index = (uint8_t)grenade;
                }
            }
            if (weapon_bottomless_clip && ((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
                int32_t i;

                for (i = 0; i < 2; i++) {
                    if ((int8_t)obj[0x31e + i] <= 1) {
                        obj[0x31e + i] = 1;
                    }
                }
                if ((uint8_t)((struct unit_object *)obj)->unit.desired_grenade_index == 0xff) {
                    ((struct unit_object *)obj)->unit.desired_grenade_index = 0;
                }
            }
            if ((uint8_t)((struct unit_object *)obj)->unit.desired_zoom_level != (uint8_t)((struct unit_object *)obj)->unit.zoom_level) {
                ((struct unit_object *)obj)->unit.zoom_level = (uint8_t)((struct unit_object *)obj)->unit.desired_zoom_level;
                if ((uint8_t)((struct unit_object *)obj)->unit.zoom_level == 0xff) {
                    *(int32_t *)&((struct unit_object *)obj)->unit.integrated_night_vision_power = 0;
                }
                if (player_index_from_unit_index(unit_index) != k_datum_index_none &&
                    *(int16_t *)((uint8_t *)player_data->data +
                        halo::datum_slot(player_index_from_unit_index(unit_index)) * 0x200 + 2) != -1) {
                    datum_index weapon = UnitView(unit_index).get_weapon_object_index(((struct unit_object *)OBJECT_DATA(unit_index))->unit.current_weapon_index);

                    if (weapon != k_datum_index_none) {
                        uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
                        datum_index sound = ((uint8_t)((struct unit_object *)obj)->unit.zoom_level == 0xff) ? *(datum_index *)(weapon_tag + 0x4bc)
                                                                 : *(datum_index *)(weapon_tag + 0x4ac);
                        float fraction = 1.0f;

                        if ((uint8_t)((struct unit_object *)obj)->unit.zoom_level != 0xff && *(int16_t *)(weapon_tag + 0x3da) > 1) {
                            fraction = (float)(int8_t)(uint8_t)((struct unit_object *)obj)->unit.zoom_level / (float)(*(int16_t *)(weapon_tag + 0x3da) - 1);
                        }
                        if (sound != k_datum_index_none) {
                            halo::sound::sound_start_unspatialized(sound, fraction);
                        }
                    }
                }
            }
        }

        speed_scale = ((uint8_t)((struct unit_object *)obj)->unit.aiming_speed == 1) ? ((Unit *)tag)->casual_aiming_modifier : 1.0f;
        rate = speed_scale * ((Unit *)tag)->aiming_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * ((Unit *)tag)->aiming_acceleration_maximum * 0.0011111111f;
        previous_aim = *(real_vector3d *)&((unit_object *)obj)->unit.aiming_vector.i;
        zero_vector = global_zero_vector3d_pointer;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i;
            if (UnitView(unit_index).is_look_target_valid()) {
                UnitView(unit_index).clamp_direction_to_aim_or_look_bounds((real_vector3d *)&((struct unit_object *)obj)->unit.aiming_vector, 1);
            }
            *(real_point3d *)&((unit_object *)obj)->unit.aiming_velocity.i = *global_origin3d_pointer;
        } else if ((uint8_t)((struct unit_object *)obj)->unit.aiming_bounds_valid != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            halo::objects::object_get_orientation(&basis.forward, unit_index, &basis.up);
            halo::math::vector3d_cross_product(basis.left, basis.forward, basis.up);
            basis.position = *zero_vector;
            halo::math::vector3d_rotate_toward_bounded((real_vector3d *)&((struct unit_object *)obj)->unit.aiming_vector, (real_vector3d *)&((struct unit_object *)obj)->unit.aiming_velocity,
                (real *)&((struct unit_object *)obj)->unit.aiming_bounds, rate, acceleration, *((real_vector3d *)&((struct unit_object *)obj)->unit.desired_aiming_vector), &basis);
        } else {
            halo::math::vector3d_rotate_toward_with_acceleration((real_vector3d *)&((struct unit_object *)obj)->unit.aiming_vector, *((real_vector3d *)&((struct unit_object *)obj)->unit.desired_aiming_vector),
                *((real_vector3d *)&((struct unit_object *)obj)->unit.aiming_velocity), rate, acceleration);
        }
        {
            float change = 0.0f;

            if (((Unit *)tag)->aiming_velocity_maximum != 0.0f) {
                change = halo::math::vector3d_angle_between_4cd4f0(previous_aim, *((real_vector3d *)&((struct unit_object *)obj)->unit.aiming_vector)) /
                    (((Unit *)tag)->aiming_velocity_maximum * 0.033333335f);
                if (change < 0.0f) {
                    change = 0.0f;
                } else if (change > 1.0f) {
                    change = 1.0f;
                }
            }
            ((struct unit_object *)obj)->unit.aiming_change = (uint8_t)(int32_t)(change * 255.0f);
        }
        rate = speed_scale * ((Unit *)tag)->looking_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * ((Unit *)tag)->looking_acceleration_maximum * 0.0011111111f;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i;
            UnitView(unit_index).clamp_direction_to_aim_or_look_bounds((real_vector3d *)&((struct unit_object *)obj)->unit.looking_vector, 0);
            *(real_point3d *)&((unit_object *)obj)->unit.looking_velocity.i = *global_origin3d_pointer;
        } else if ((uint8_t)((struct unit_object *)obj)->unit.looking_bounds_valid != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            halo::objects::object_get_orientation(&basis.forward, unit_index, &basis.up);
            halo::math::vector3d_cross_product(basis.left, basis.forward, basis.up);
            basis.position = *zero_vector;
            halo::math::vector3d_rotate_toward_bounded((real_vector3d *)&((struct unit_object *)obj)->unit.looking_vector, (real_vector3d *)&((struct unit_object *)obj)->unit.looking_velocity,
                (real *)&((struct unit_object *)obj)->unit.looking_bounds, rate, acceleration, *((real_vector3d *)&((struct unit_object *)obj)->unit.desired_looking_vector), &basis);
        } else {
            halo::math::vector3d_rotate_toward_with_acceleration((real_vector3d *)&((struct unit_object *)obj)->unit.looking_vector, *((real_vector3d *)&((struct unit_object *)obj)->unit.desired_looking_vector),
                *((real_vector3d *)&((struct unit_object *)obj)->unit.looking_velocity), rate, acceleration);
        }

        if (!unit_updates_suppressed) {
            uint8_t throwing = (uint8_t)((((unit_object *)obj)->unit.control_flags >> 13) & 1);

            switch ((int8_t)(uint8_t)((struct unit_object *)obj)->unit.throwing_grenade_state) {
            case 0:
                if (throwing) {
                    UnitView(unit_index).begin_throw_grenade(0);
                }
                break;
            case 1:
                if (((unit_object *)obj)->base.animation_frame >= 2) {
                    UnitView(unit_index).throw_grenade_move_to_hand();
                }
                break;
            case 2:
                (((unit_object *)obj)->unit.throwing_grenade_counter)++;
                if ((uint8_t)((struct unit_object *)obj)->unit.animation_state != 0x21) {
                    UnitView(unit_index).release_thrown_grenade(1);
                }
                break;
            case 3:
                if ((uint8_t)((struct unit_object *)obj)->unit.animation_state != 0x21 && !throwing) {
                    ((struct unit_object *)obj)->unit.throwing_grenade_state = 0;
                }
                break;
            default:
                break;
            }
        }
        if (((unit_object *)obj)->unit.current_weapon_index != -1 && !unit_updates_suppressed) {
            uint32_t control = 0;
            float trigger = ((unit_object *)obj)->unit.primary_trigger;
            uint8_t *unit_now;
            datum_index weapon = k_datum_index_none;

            if (((unit_object *)obj)->unit.current_weapon_index == ((unit_object *)obj)->unit.desired_weapon_index) {
                uint8_t flashing = (uint8_t)(((struct unit_object *)obj)->unit.persistent_control_ticks > 0 && test_flag(((struct unit_object *)obj)->unit.persistent_control_flags, units::unit_control_flag::primary_trigger));

                if (valid_team_player && test_flag(((struct unit_object *)obj)->unit.control_flags, units::unit_control_flag::unknown_10)) {
                    control = 1;
                }
                if (test_flag(((unit_object *)obj)->unit.control_flags, units::unit_control_flag::primary_trigger)) {
                    control |= 2;
                }
                if (test_flag(((unit_object *)obj)->unit.control_flags, units::unit_control_flag::secondary_trigger)) {
                    control |= 4;
                }
                if (test_flag(((struct Unit *)TAG_DATA(*(datum_index *)obj))->unit_flags, tags::unit_tag_flag::integrated_light_cntrls_weapon)) {
                    halo::items::weapon_set_ready_timer(UnitView(unit_index).get_weapon_object_index(((struct unit_object *)OBJECT_DATA(unit_index))->unit.current_weapon_index), ((struct unit_object *)obj)->unit.integrated_light_power);
                }
                if (test_flag(((unit_object *)obj)->unit.control_flags, units::unit_control_flag::reload)) {
                    control |= 8;
                }
                if (::halo::units::unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset)) && !flashing) {
                    control |= 0x10;
                }
                if (((unit_object *)obj)->base.type == 0 && (int8_t)obj[0x505] > 0) {
                    control |= 0x10;
                }
                if ((uint8_t)((struct unit_object *)obj)->unit.zoom_level != 0xff) {
                    control |= 0x40;
                }
            } else {
                control = 0x20;
            }
            unit_now = OBJECT_DATA(unit_index);
            if (((struct unit_object *)unit_now)->unit.current_weapon_index != -1) {
                weapon = *(datum_index *)(unit_now + 0x2f8 + ((struct unit_object *)unit_now)->unit.current_weapon_index * 4);
            }
            halo::items::weapon_set_control_flags(weapon, (uint16_t)control, trigger);
        }
    }

    if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::simple_creature)) {
        int16_t seat;

        if (test_flag(((struct unit_object *)obj)->unit.animation_state_flags, units::unit_animation_state_flag::aiming_enabled)) {
            UnitView(unit_index).update_look_delta_controls();
            ((struct unit_object *)obj)->unit.animation_controls_smoothed[0] = ((struct unit_object *)obj)->unit.animation_controls[0] * LOOK_BLEND_NEW + ((struct unit_object *)obj)->unit.animation_controls_smoothed[0] * LOOK_BLEND_OLD;
            ((struct unit_object *)obj)->unit.animation_controls_smoothed[1] = ((struct unit_object *)obj)->unit.animation_controls[1] * LOOK_BLEND_NEW + ((struct unit_object *)obj)->unit.animation_controls_smoothed[1] * LOOK_BLEND_OLD;
            ((struct unit_object *)obj)->unit.animation_controls_smoothed[2] = ((struct unit_object *)obj)->unit.animation_controls[2] * LOOK_BLEND_NEW + ((struct unit_object *)obj)->unit.animation_controls_smoothed[2] * LOOK_BLEND_OLD;
        }
        for (seat = 0; seat < *(int32_t *)&((Unit *)tag)->powered_seats.count; seat++) {
            uint8_t *powered = *(uint8_t **)&((Unit *)tag)->powered_seats.pointer + seat * 0x44;
            float *power = (float *)(obj + 0x338 + seat * 4);
            uint8_t occupied;

            if (seat == 0) {
                occupied = (uint8_t)(((unit_object *)obj)->unit.driver_unit_index != k_datum_index_none || test_flag(((struct unit_object *)obj)->unit.flags, units::unit_flag::unattended));
            } else {
                occupied = (uint8_t)(((unit_object *)obj)->unit.gunner_unit_index != k_datum_index_none &&
                    ((unit_object *)obj)->unit.gunner_unit_index != ((unit_object *)obj)->unit.driver_unit_index);
            }
            if (!test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen) && occupied) {
                if (*power != 1.0f) {
                    *power += 1.0f / (*(float *)(powered + 4) * 30.0f);
                    if (*power > 1.0f) {
                        *power = 1.0f;
                    }
                }
            } else if (*power != 0.0f) {
                *power -= 1.0f / (*(float *)(powered + 8) * 30.0f);
                if (*power < 0.0f) {
                    *power = 0.0f;
                }
            }
        }
    }
    if (((struct unit_object *)obj)->unit.delayed_damage_ticks > 0 && --((struct unit_object *)obj)->unit.delayed_damage_ticks == 0) {
        actor_react_to_threat_event(unit_index, ((struct unit_object *)obj)->unit.delayed_damage_responsible_object, *(uint16_t *)&((struct unit_object *)obj)->unit.delayed_damage_category,
            ((struct unit_object *)obj)->unit.delayed_damage_amount, 0, 1);
        ((struct unit_object *)obj)->unit.delayed_damage_category = 0;
        ((struct unit_object *)obj)->unit.delayed_damage_responsible_object = k_datum_index_none;
        *(int32_t *)&((struct unit_object *)obj)->unit.delayed_damage_amount = 0;
    }
    if (!unit_updates_suppressed) {
        UnitView(unit_index).melee_lunge_damage_tick();
        if (!unit_updates_suppressed) {
            UnitView(unit_index).update_animation_timers();
            if (!unit_updates_suppressed && (over_budget || ((unit_object *)obj)->unit.controlling_player != k_datum_index_none)) {
                UnitView(unit_index).calculate_luminosity();
            }
        }
    }
    if ((uint8_t)((struct unit_object *)obj)->unit.flaming_ticks != 0) {
        if (unit_updates_suppressed) {
            goto done;
        }
        if (--((struct unit_object *)obj)->unit.flaming_ticks == 0) {
            UnitView(unit_index).update_autoaim_interaction();
            if (unit_updates_suppressed) {
                goto done;
            }
        }
    } else if (unit_updates_suppressed) {
        goto done;
    }
    {
        float step = -((unit_object *)obj)->unit.mouth_aperture;

        if (step < -0.1f) {
            step = -0.1f;
        } else if (step > 0.1f) {
            step = 0.1f;
        }
        ((unit_object *)obj)->unit.mouth_aperture += step;
    }
    {
        uint8_t toggle = 0;
        uint32_t flags = ((unit_object *)obj)->unit.flags;
        uint32_t button;

        if (test_flag(flags, units::unit_flag::unknown_10000000)) {
            if (!test_flag(flags, units::unit_flag::unknown_80000)) {
                toggle = 1;
            }
            ((unit_object *)obj)->unit.flags = flags & 0xefffffff;
        }
        flags = ((unit_object *)obj)->unit.flags;
        if (test_flag(flags, units::unit_flag::unknown_20000000)) {
            if (test_flag(flags, units::unit_flag::unknown_80000)) {
                toggle = 1;
            }
            ((unit_object *)obj)->unit.flags = flags & 0xdfffffff;
        }
        button = ((unit_object *)obj)->unit.control_flags & 0x10;
        if (button != 0 || !(((struct unit_object *)obj)->unit.integrated_light_energy > 0.0f) || toggle) {
            if (!valid_team_player) {
                flags = ((unit_object *)obj)->unit.flags;
                if (test_flag(flags, units::unit_flag::unknown_4000000)) {
                    ((unit_object *)obj)->unit.flags = flags & 0xfbffffff;
                }
                flags = ((unit_object *)obj)->unit.flags;
                if (test_flag(flags, units::unit_flag::unknown_80000)) {
                    ((unit_object *)obj)->unit.flags = (flags & 0xfff7ffff) | 0x10;
                }
            } else {
                uint8_t toggle_light = 1;

                if (UnitView(unit_index).current_weapon_has_flag()) {
                    if (button != 0) {
                        uint8_t *effects = (uint8_t *)global_globals->first_person_interface.pointer;
                        datum_index effect = (test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_4000000))
                            ? *(datum_index *)(effects + 0x64) : *(datum_index *)(effects + 0x54);

                        if (effect != k_datum_index_none) {
                            halo::effects::effect_new_on_object(unit_index, effect, unit_index, -1, 0.0f, 0.0f, 0, 0);
                        }
                        ((unit_object *)obj)->unit.flags ^= 0x4000000;
                    }
                    if (test_flag(((struct unit_object *)obj)->unit.control_flags, units::unit_control_flag::unknown_10)) {
                        toggle_light = 0;
                    }
                }
                if (toggle_light && (test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_80000) || ((struct unit_object *)obj)->unit.integrated_light_energy > 0.2f) &&
                    ((unit_object *)obj)->base.parent_object == k_datum_index_none) {
                    halo::effects::effect_new_on_object(unit_index, *(datum_index *)&((Unit *)tag)->integrated_light_toggle.tag_id, unit_index, -1, 0.0f, 0.0f, 0, 0);
                    ((unit_object *)obj)->unit.flags ^= 0x80000;
                }
            }
        }
        flags = ((unit_object *)obj)->unit.flags;
        if (test_flag(flags, units::unit_flag::unknown_80000)) {
            if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::integrated_light_lasts_forever)) {
                ((struct unit_object *)obj)->unit.integrated_light_energy -= 0.00027777778f;
            }
            if (((unit_object *)obj)->base.parent_object != k_datum_index_none || test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
                ((unit_object *)obj)->unit.flags = flags & 0xfff7ffff;
            }
            if (((struct unit_object *)obj)->unit.integrated_light_power != 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_power += 0.16666667f;
                if (((struct unit_object *)obj)->unit.integrated_light_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.integrated_light_power = 1.0f;
                }
            }
        } else {
            if (((struct unit_object *)obj)->unit.integrated_light_energy < 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_energy += 0.0011111111f;
            }
            if (((struct unit_object *)obj)->unit.integrated_light_power != 0.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_power -= 0.041666668f;
                if (((struct unit_object *)obj)->unit.integrated_light_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.integrated_light_power = 0.0f;
                }
            }
        }
    }
    if (UnitView(unit_index).current_weapon_has_flag()) {
        if (test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_4000000)) {
            if (((struct unit_object *)obj)->unit.integrated_night_vision_power != 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_night_vision_power += 0.083333336f;
                if (((struct unit_object *)obj)->unit.integrated_night_vision_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.integrated_night_vision_power = 1.0f;
                }
            }
        } else if (((struct unit_object *)obj)->unit.integrated_night_vision_power != 0.0f) {
            ((struct unit_object *)obj)->unit.integrated_night_vision_power -= 0.041666668f;
            if (((struct unit_object *)obj)->unit.integrated_night_vision_power < 0.0f) {
                ((struct unit_object *)obj)->unit.integrated_night_vision_power = 0.0f;
            }
        }
    }
done:
    return 1;
}
#undef OBJECT_DATA
#undef TAG_DATA
#undef LOOK_BLEND_NEW
#undef LOOK_BLEND_OLD

}
