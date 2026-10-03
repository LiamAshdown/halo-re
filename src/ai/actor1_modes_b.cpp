#include "halo/ai/actor_modes.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace c_actor_mode_charge_enter {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void *actor_get_actor_definition(datum_index actor_index);
}
}

extern "C" void actor_mode_charge_enter(datum_index actor_index);

/**
 * actor_mode_charge_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_charge_enter.c.txt.
 *
 * @address 0x401d50
 */
void halo::ai::charge_mode::enter()
{
    using namespace c_actor_mode_charge_enter;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.charge.stage == 4 &&
        *(int16_t *)((uint8_t *)actor_get_actor_definition(actor_index) + 0x156) == 3 &&
        ((struct actor *)act)->special_fire_strafe_cooldown > 0) {
        ((struct actor *)act)->special_fire_strafe_cooldown -= 1;
    }
}

extern "C" void actor_mode_charge_enter(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).enter();
}

#undef ACTOR

namespace c_actor_mode_charge_process {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode,
                                                    actor_combat_consideration *consideration);
extern void *actor_get_threat_weapon_definition(int32_t actor_index);
extern void *actor_get_actor_definition(datum_index actor_index);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius);
extern void actor_movement_actions_cancel(datum_index actor_index);
extern uint8_t actor_movement_action_is_complete(datum_index actor_index);
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin,
    real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc,
    real_vector3d *out_direction, real *max_speed_override, real *out_speed,
    real *out_time_of_flight, real *out_range, real *out_half_gravity_term,
    real *out_horizontal_speed);
}
}

extern "C" uint8_t actor_mode_charge_process(datum_index actor_index);

/**
 * actor_mode_charge_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_charge_process.c.txt.
 *
 * @address 0x401da0
 */
uint8_t halo::ai::charge_mode::process()
{
    using namespace c_actor_mode_charge_process;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
    uint8_t *definition = (uint8_t *)actor_get_actor_definition(actor_index);
    uint8_t *md = act + 0x9c;
    uint8_t *target = 0;
    uint32_t actor_flags = *(uint32_t *)actor_tag;
    int16_t kind;
    float threshold;
    int32_t now;

    if (((actor *)act)->target_unit_index == k_datum_index_none) {
        md[0x28] = 0;
    } else {
        target = PROP(((actor *)act)->target_unit_index);
        kind = *(int16_t *)(md + 0x4);
        if (*(datum_index *)&((struct actor *)act)->stuck_projectile_index != k_datum_index_none || kind == 5 || kind == 4) {
            md[0x28] = 1;
        } else if (kind == 2 || kind == 3) {

            float range = 3.4028235e38f;
            uint8_t check_range = 1;
            uint8_t use_retreat_range = act[0x378];

            if (!actor_has_unshielded_threat_weapon(actor_index)) {
                use_retreat_range = 1;
            }
            if (md[0x6] || md[0xb] || md[0xc]) {
                check_range = 0;
            } else if (act[0x378] || !actor_has_unshielded_threat_weapon(actor_index)) {
                range = use_retreat_range ? ((ActorVariant *)variant)->berserk_melee_abort_range : ((ActorVariant *)variant)->melee_abort_range;
            }
            if (act[0x1cb]) {
                float limit = (0.0f > ((Actor *)actor_tag)->melee_fudge_factor ? 0.0f : ((Actor *)actor_tag)->melee_fudge_factor) + 0.8f;

                if (range > limit) {
                    range = limit;
                }
            }
            if (check_range && range < *(float *)(target + 0x11c)) {
                md[0x8] = 1;
            } else {
                *(int32_t *)&((struct actor *)act)->last_melee_time = game_time->game_time;
                md[0x28] = 1;
                if (check_range) {
                    if (*(int16_t *)(md + 0x4) == 2) {
                        if (*(float *)(actor_tag + 0x388) == 0.0f || ((Actor *)actor_tag)->melee_leap_chance == 0.0f) {
                            md[0xa] = 0;
                        } else if (target[0x130] || *(int16_t *)(target + 0x9c) > 0) {
                            md[0xa] = 1;
                        }
                        if (md[0xa] && (*(int16_t *)(target + 0x9c) > 0 ||
                                        *(float *)(actor_tag + 0x384) * 1.5f < *(float *)(target + 0x11c))) {
                            *(int16_t *)(md + 0x4) = 3;
                        }
                    } else if (*(float *)(target + 0x11c) < *(float *)(actor_tag + 0x384)) {
                        *(int16_t *)(md + 0x4) = 2;
                        md[0xa] = 1;
                    }
                }
            }
        } else {

            kind = (int16_t)((actor_flags & 0x20000) && ((struct actor *)act)->combat_status >= 5 && !act[0x378]);
            *(int16_t *)(md + 0x4) = kind;
            if (kind == 1) {
                int16_t target_kind = *(int16_t *)(target + 0x38);
                uint8_t weak = (uint8_t)((target_kind == 0 || target_kind == 1) && (int8_t)target[0x122] <= 2);

                md[0x24] = weak;
                md[0x28] = (uint8_t)!(weak && (actor_flags & 0x40000));
                if (weak) {
                    *(int16_t *)(md + 0x26) += 1;
                }
                md[0x25] = 0;
                if (!weak && (int8_t)target[0x124] <= 1) {
                    md[0x25] = 1;
                } else if (((Actor *)actor_tag)->stalking_max_distance > 0.0f &&
                           !(*(float *)(target + 0x11c) < ((Actor *)actor_tag)->stalking_max_distance)) {
                    md[0x25] = 1;
                }
            } else if (!actor_has_unshielded_threat_weapon(actor_index) || act[0x15d]) {
                md[0x28] = 1;
            } else {
                float range_lo;
                float range_hi;
                uint8_t *weapon;

                if (act[0x378]) {
                    range_hi = *(float *)(definition + 0x16c);
                    range_lo = *(float *)(definition + 0x168);
                } else {
                    range_hi = *(float *)(definition + 0xa0);
                    range_lo = *(float *)(definition + 0x9c);
                }
                weapon = (uint8_t *)actor_get_threat_weapon_definition(actor_index);
                if (weapon != 0 && *(float *)(weapon + 0x40c) > 0.0f && !(range_lo > *(float *)(weapon + 0x40c))) {
                    range_lo = *(float *)(weapon + 0x40c);
                }
                if (md[0x28]) {
                    if (range_lo > *(float *)(target + 0x11c)) {
                        md[0x28] = 0;
                    }
                } else if (*(float *)(target + 0x11c) > range_hi) {
                    md[0x28] = 1;
                }
                if (*(float *)(target + 0x11c) > 0.7f && *(int16_t *)(target + 0x38) != 0 &&
                    *(int16_t *)(target + 0x38) != 1) {
                    md[0x28] = 1;
                }
            }
        }
    }

    if (md[0x6]) {
        datum_index unit_index = ((actor *)act)->unit_index;

        md[0x7] = (uint8_t)!(unit_index != k_datum_index_none && halo::units::unit_is_in_busy_animation_state(unit_index));
    } else if (!md[0xc] && (*(int16_t *)(md + 0x4) == 2 || *(int16_t *)(md + 0x4) == 3) && target != 0) {
        real_vector3d direction;
        float along = 0.0f;
        float lead_ticks = 0.0f;
        uint8_t strike = 0;
        uint8_t *unit = 0;
        uint8_t have_along = 0;

        if (*(float *)(target + 0x11c) < 0.8f) {
            direction = *(real_vector3d *)(target + 0xe0);
            strike = 1;
        } else {
            real_vector3d *velocity = (real_vector3d *)(target + 0xd4);
            real_vector3d *facing = (real_vector3d *)(target + 0xe0);
            float speed = halo::math::vector3d_length(*velocity);
            float factor = 0.0f;
            real_point3d lead;

            unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[((actor *)act)->unit_index & 0xffff].data;
            if (speed > 0.0f) {
                factor = ((velocity->k * facing->k + velocity->j * facing->j + velocity->i * facing->i) / speed + 1.0f) * 0.5f;
            }
            lead_ticks = (float)*(int16_t *)(md + 0x32);
            halo::math::point3d_add_scaled(lead, *velocity, *(real_point3d *)(target + 0xbc), lead_ticks * factor);
            direction.i = lead.x - ((actor *)act)->body_position.x;
            direction.j = lead.y - ((actor *)act)->body_position.y;
            direction.k = lead.z - ((actor *)act)->body_position.z;
            if (direction.j * facing->j + direction.k * facing->k + direction.i * facing->i < 0.0f) {
                along = 0.0f;
                direction = *facing;
            } else {
                along = halo::math::vector3d_normalize_with_length(direction);
                if (along == 0.0f) {
                    direction = *facing;
                }
            }
            have_along = 1;
            if (*(int16_t *)(md + 0x4) == 3 && !md[0xb]) {
                if (along < *(float *)(actor_tag + 0x384) && *(int16_t *)(target + 0x9c) == 0 && !target[0x130]) {
                    md[0x8] = 1;
                    *(int32_t *)&((struct actor *)act)->last_melee_time = -1;
                } else if (along < *(float *)(actor_tag + 0x388)) {
                    real_vector3d leap;
                    real half_gravity;
                    real horizontal_speed;

                    if (projectile_solve_ballistic_arc((real_point3d *)(target + 0xbc), (real_point3d *)(act + 0x12c),
                                                       ((Actor *)actor_tag)->melee_leap_velocity, 1.0f, (real *)(actor_tag + 0x394),
                                                       0, &leap, 0, 0, 0, 0, &half_gravity, &horizontal_speed)) {
                        if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                            leap = *(real_vector3d *)&((actor *)act)->facing.i;
                            if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                                leap = *halo::math::globals().global_forward3d_pointer;
                            }
                        }
                        *(float *)(md + 0x14) = leap.i;
                        *(float *)(md + 0x18) = leap.j;
                        md[0xc] = 1;
                        *(float *)(md + 0x1c) = horizontal_speed;
                        *(float *)(md + 0x20) = half_gravity;
                    }
                }
            } else if (md[0x30]) {
                if (along < ((Actor *)actor_tag)->melee_fudge_factor) {
                    strike = 1;
                } else if (along < ((Actor *)actor_tag)->suicide_sensing_dist) {
                    float closing = (velocity->j - ((unit_object *)unit)->base.velocity.j) * direction.j +
                                    (velocity->i - ((unit_object *)unit)->base.velocity.i) * direction.i +
                                    (velocity->k - ((unit_object *)unit)->base.velocity.k) * direction.k;

                    if (closing > 0.023333333f) {
                        strike = 1;
                    }
                }
            } else {
                if (*(int16_t *)(md + 0x4) == 3 && md[0xb]) {
                    along -= (direction.j * ((unit_object *)unit)->base.velocity.j + direction.k * ((unit_object *)unit)->base.velocity.k +
                              direction.i * ((unit_object *)unit)->base.velocity.i) * lead_ticks;
                }
                if (along < ((Actor *)actor_tag)->melee_fudge_factor + *(float *)(md + 0x34)) {
                    strike = 1;
                }
            }
        }
        (void)have_along;

        if (md[0xc] || (strike && !md[0x30])) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (halo::math::vector2d_normalize_with_length(flat) > 0.0f &&
                flat.j * ((actor *)act)->facing.j + flat.i * ((actor *)act)->facing.i < (md[0xb] ? 0.0f : 0.8660254f)) {
                md[0xc] = 0;
                md[0x9] = 1;
                strike = 0;
                goto strike_done;
            }
        }
        if (strike) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (halo::math::vector2d_normalize_with_length(flat) == 0.0f) {
                flat.i = ((actor *)act)->facing.i;
                flat.j = ((actor *)act)->facing.j;
            }
            if (halo::units::unit_try_ready_weapon(((actor *)act)->unit_index, 0, &flat)) {
                ai_communication_broadcast(0x2b, ((actor *)act)->unit_index, *(datum_index *)(target + 0x18), 3, -1, -1, 0);
                md[0x6] = 1;
            }
        }
    strike_done:;
    }

    now = game_time->game_time;
    kind = *(int16_t *)(md + 0x4);
    if ((kind == 2 || kind == 3) && !md[0x6] && !md[0xc]) {
        if (md[0xb]) {
            if (*(int16_t *)(md + 0xe) > 15) {
                md[0x8] = 1;
            }
        } else if (((Actor *)actor_tag)->melee_charge_time > 0.0f &&
                   !((float)*(int32_t *)(md + 0x0) + ((Actor *)actor_tag)->melee_charge_time * 30.0f > (float)now)) {
            md[0x8] = 1;
        }
    }
    if (kind == 4 || kind == 5) {
        *(int32_t *)&((struct actor *)act)->last_vehicle_charge_time = now;
    }
    threshold = actor_get_consideration_wait_threshold(actor_index, *(int16_t *)(md + 0x4), (actor_combat_consideration *)md);
    *(float *)(md + 0x2c) = threshold;
    if (!act[0x6] && act[0x4c]) {
        md[0x29] = 0;
        if (!md[0x6] && !md[0xb] && !md[0xc] && md[0x28]) {
            float radius = *(int16_t *)(md + 0x4) == 3 ? 4.0f : 1.5f;

            if (!(radius > threshold)) {
                radius = threshold;
            }
            if (actor_movement_set_destination_near_target(((actor *)act)->target_unit_index, actor_index, radius)) {
                actor_movement_actions_cancel(actor_index);
                goto approach_done;
            }
            md[0x29] = 1;
            md[0x28] = 0;
        }
        actor_movement_action_stop(actor_index);
    approach_done:
        if (((actor *)act)->target_combat_status >= 7) {
            datum_index target_index = ((actor *)act)->target_unit_index;
            uint8_t far_away = (uint8_t)(*(float *)(PROP(target_index) + 0x11c) > *(float *)(md + 0x2c));
            uint8_t engaged = 0;
            int16_t current = *(int16_t *)(md + 0x4);

            if (!((current == 2 || current == 3) && (md[0xb] || md[0xc] || md[0x6])) && far_away) {
                if (md[0x29] || !actor_movement_action_is_complete(actor_index) ||
                    ((struct actor *)act)->path_remaining_distance > *(float *)(md + 0x2c)) {
                    engaged = 1;
                }
            }
            actor_target_mark_engaged(target_index, actor_index, engaged);
        }
    }

    kind = *(int16_t *)(md + 0x4);
    if (kind == 2 || kind == 3) {
        return (uint8_t)(md[0x8] || md[0x7] || md[0x29]);
    }
    if (kind == 4 || kind == 5) {
        return md[0x29];
    }
    return 0;
}

extern "C" uint8_t actor_mode_charge_process(datum_index actor_index)
{
    return halo::ai::charge_mode(actor_index).process();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_mode_charge_tick {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_charge_tick(datum_index actor_index);

/**
 * actor_mode_charge_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_charge_tick.c.txt.
 *
 * @address 0x402aa0
 */
void halo::ai::charge_mode::tick()
{
    using namespace c_actor_mode_charge_tick;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.charge.stage == 3 && act[0xa7] && !act[0xa2] && !act[0x15c]) {
        ((struct actor *)act)->mode_data.charge.stage_ticks += 1;
    }
}

extern "C" void actor_mode_charge_tick(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).tick();
}

#undef ACTOR

namespace c_actor_mode_charge_update {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)

extern game_time_globals *game_time;
}
}

extern "C" void actor_mode_charge_update(datum_index actor_index);

/**
 * actor_mode_charge_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_charge_update.c.txt.
 *
 * @address 0x402af0
 */
void halo::ai::charge_mode::update()
{
    using namespace c_actor_mode_charge_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint32_t actor_flags = *(uint32_t *)actor_tag;
    int16_t kind = ((struct actor *)act)->mode_data.charge.stage;

    ((actor *)act)->flee_source.code = 2;
    ((struct actor *)act)->look_posture = 4;
    if ((kind == 2 || kind == 3) && act[0xa5] && !act[0x504] && !act[0x4a8]) {
        ((actor *)act)->flee_reason = 4;
    } else if (((struct actor *)act)->combat_status >= 5 && kind != 1) {
        ((actor *)act)->flee_reason = 7;
    } else {
        ((actor *)act)->flee_reason = 5;
    }
    if (((struct actor *)act)->mode_data.charge.stage == 1) {
        act[0x426] = (uint8_t)(act[0xc1] == 0);
        act[0x427] = (uint8_t)(act[0xc1] == 0);
    } else if (!act[0x428] && (actor_flags & 0x10000)) {
        act[0x426] = act[0x358];
        act[0x427] = act[0x358];
    } else {
        act[0x426] = 0;
        act[0x427] = 0;
    }
    if (act[0xa8]) {
        act[0x440] = 1;
        act[0x441] = (uint8_t)(*(float *)(act + 0xb8) * 0.7f > *(float *)(act + 0xbc));
        act[0x442] = 1;
        *(int32_t *)&((struct actor *)act)->jump_facing.i = *(int32_t *)(act + 0xb0);
        *(int32_t *)&((struct actor *)act)->jump_facing.j = *(int32_t *)(act + 0xb4);
        *(int32_t *)&((struct actor *)act)->jump_horizontal_velocity = *(int32_t *)(act + 0xb8);
        *(int32_t *)&((struct actor *)act)->jump_vertical_velocity = *(int32_t *)(act + 0xbc);
        act[0xa7] = 1;
        act[0xa8] = 0;
        ((struct actor *)act)->mode_data.charge.stage_start_time = game_time->game_time;
        ((struct actor *)act)->mode_data.charge.stage_ticks = 0;
    }
    if (actor_flags & 0x100000) {
        if (act[0x378] || ((struct actor *)act)->mode_data.charge.stage == 2 || ((struct actor *)act)->mode_data.charge.stage == 3) {
            act[0x428] = (uint8_t)(act[0xc4] && !act[0x427]);
        }
    }
    act[0x424] = 0;
    act[0x425] = 0;
    act[0x42a] = 1;
    act[0x454] = (uint8_t)(((struct actor *)act)->mode_data.charge.stage != 1);
}

extern "C" void actor_mode_charge_update(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).update();
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_mode_fight_tick {
extern "C" {
extern data_array *actor_data;

extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
}
}

extern "C" void actor_mode_fight_tick(uint32_t actor_index);

/**
 * actor_mode_fight_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_fight_tick.c.txt.
 *
 * @address 0x403540
 */
void halo::ai::fight_mode::tick()
{
    using namespace c_actor_mode_fight_tick;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    int16_t countdown = *(int16_t *)&((struct actor *)actor)->mode_data;

    if (countdown <= 0 || actor[0x484] == 0) {
        return;
    }
    countdown = (int16_t)(countdown - 1);
    *(int16_t *)&((struct actor *)actor)->mode_data = countdown;
    if (countdown == 0 && *(uint16_t *)&((struct actor *)actor)->firing_position_index != 0xffff && actor[0x3ba] == 0) {
        actor_push_recognition_entry(actor_index, ((struct actor *)actor)->firing_position_index, 0);
    }
}

extern "C" void actor_mode_fight_tick(uint32_t actor_index)
{
    halo::ai::fight_mode(actor_index).tick();
}

namespace c_actor_mode_fight_update {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" void actor_mode_fight_update(uint32_t actor_index);

/**
 * actor_mode_fight_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_fight_update.c.txt.
 *
 * @address 0x4035b0
 */
void halo::ai::fight_mode::update()
{
    using namespace c_actor_mode_fight_update;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    actor[0x426] = actor[0x358];
    ((struct actor *)actor)->flee_reason = 5;
    ((struct actor *)actor)->flee_source.code = 2;
    ((struct actor *)actor)->look_posture = 4;
    actor[0x427] = 0;
    actor[0x428] = 0;
    actor[0x424] = 0;
    actor[0x425] = 0;
    if (((struct actor *)actor)->vehicle_driving_type != 4 && ((struct actor *)actor)->combat_status >= 5) {
        actor[0x454] = 1;
        ((struct actor *)actor)->flee_reason = 7;
    }
}

extern "C" void actor_mode_fight_update(uint32_t actor_index)
{
    halo::ai::fight_mode(actor_index).update();
}

namespace c_actor_mode_flee_enter {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

}
}

extern "C" void actor_mode_flee_enter(datum_index actor_index);

/**
 * actor_mode_flee_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_enter.c.txt.
 *
 * @address 0x403740
 */
void halo::ai::flee_mode::enter()
{
    using namespace c_actor_mode_flee_enter;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    int16_t kind = ((struct actor *)act)->mode_data.flee.panic;

    ((struct actor *)act)->mode_data.flee.ticks_in_mode = 0;
    if (kind > 0) {
        act[0x98] = 0;
    }
    if (((struct actor *)act)->mode_data.flee.countdown_02 == 0 && ((actor *)act)->unit_index != k_datum_index_none && kind >= 9 && kind <= 12) {
        halo::units::unit_initialize_random_turn_angle(((actor *)act)->unit_index);
    }
}

extern "C" void actor_mode_flee_enter(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).enter();
}

#undef ACTOR

namespace c_actor_mode_flee_exit {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

}
}

extern "C" void actor_mode_flee_exit(datum_index actor_index);

/**
 * actor_mode_flee_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_exit.c.txt.
 *
 * @address 0x4037a0
 */
void halo::ai::flee_mode::exit()
{
    using namespace c_actor_mode_flee_exit;
    datum_index actor_index = datum;
    datum_index unit_index = *(datum_index *)(ACTOR(actor_index) + 0x18);

    if (unit_index != k_datum_index_none) {
        uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[unit_index & 0xffff].data;

        *(uint32_t *)(obj + 0x204) &= ~0x2000000u;
    }
}

extern "C" void actor_mode_flee_exit(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).exit();
}

#undef ACTOR

namespace c_actor_mode_flee_get_look_weights {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *hud_text_message_normal_color;
extern const float *actor_mode_default_look_weights;
}
}

extern "C" void actor_mode_flee_get_look_weights(datum_index actor_index, float *out_weights);

/**
 * actor_mode_flee_get_look_weights: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_get_look_weights.c.txt.
 *
 * @address 0x403d50
 */
void halo::ai::flee_mode::get_look_weights(float *out_weights)
{
    using namespace c_actor_mode_flee_get_look_weights;
    datum_index actor_index = datum;
    const float *source = *(int16_t *)(ACTOR(actor_index) + 0xa8) > 0 ? hud_text_message_normal_color
                                                                        : actor_mode_default_look_weights;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

extern "C" void actor_mode_flee_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::flee_mode(actor_index).get_look_weights(out_weights);
}

#undef ACTOR

namespace c_actor_mode_flee_movement_cancelled {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_flee_movement_cancelled(datum_index actor_index);

/**
 * actor_mode_flee_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_movement_cancelled.c.txt.
 *
 * @address 0x403d20
 */
void halo::ai::flee_mode::movement_cancelled()
{
    using namespace c_actor_mode_flee_movement_cancelled;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    ((actor_mode_flee_data *)mode_data)->destination = -1;
    ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
}

extern "C" void actor_mode_flee_movement_cancelled(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).movement_cancelled();
}

#undef ACTOR

namespace c_actor_mode_flee_process {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern uint8_t actor_is_target_within_engagement_range(uint32_t actor_index);
extern void actor_update_target_combat_status(datum_index actor_index);
extern void actor_update_awareness_level(datum_index actor_index);
extern uint8_t actor_check_weapon_pickup_reachable(uint32_t actor_index, uint8_t *record);
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order);
}
}

extern "C" uint8_t actor_mode_flee_process(datum_index actor_index);

/**
 * actor_mode_flee_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_process.c.txt.
 *
 * @address 0x4037f0
 */
uint8_t halo::ai::flee_mode::process()
{
    using namespace c_actor_mode_flee_process;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *mode_data = act + 0x9c;
    int16_t kind;

    if (!act[0x6]) {
        kind = ((actor_mode_flee_data *)mode_data)->panic;
        if (kind >= 9 && kind <= 12) {
            ((actor_mode_flee_data *)mode_data)->countdown_180 = 180;
        }
        if (((actor_mode_flee_data *)mode_data)->countdown_02 > 0) {
            ((actor_mode_flee_data *)mode_data)->destination = -1;
        } else if (((actor_mode_flee_data *)mode_data)->destination == -1) {
            ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
        } else if (((actor *)act)->firing_position_index == -1) {
            ((actor_mode_flee_data *)mode_data)->destination = -1;
            ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
        } else if (actor_is_target_within_engagement_range(actor_index)) {
            if (((actor_mode_flee_data *)mode_data)->countdown_180 != 0) {
                ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
            } else {
                ((actor_mode_flee_data *)mode_data)->destination = ((actor *)act)->firing_position_index;
                mode_data[0xa] = act[0x3ba];
                mode_data[0xf] = 1;
                ((actor_mode_flee_data *)mode_data)->movement_cancelled = 0;
                if (((actor_mode_flee_data *)mode_data)->reference != k_datum_index_none) {
                    uint8_t *source = PROP(((actor_mode_flee_data *)mode_data)->reference);
                    int16_t a = *(int16_t *)(source + 0x34);
                    int16_t b = *(int16_t *)(source + 0x36);

                    *(int16_t *)(source + 0x32) = 0;
                    *(int16_t *)(source + 0x30) = a > b ? a : b;
                    *(int16_t *)(source + 0x38) = 2;
                    source[0x74] = 0;
                    actor_update_target_combat_status(actor_index);
                    actor_update_awareness_level(actor_index);
                }
            }
        }
        switch (((actor_mode_flee_data *)mode_data)->panic) {
        case 9:
        case 10:
            if (*(datum_index *)&((struct actor *)act)->stuck_projectile_index == k_datum_index_none) {
                mode_data[0xf] = 1;
            }
            break;
        case 11:
            if (!act[0x1b4]) {
                mode_data[0xf] = 1;
            }
            break;
        case 12:
            if (!act[0x1b5]) {
                mode_data[0xf] = 1;
            }
            break;
        default:
            break;
        }
        if (act[0x4c] && !mode_data[0xf]) {
            if (((actor_mode_flee_data *)mode_data)->destination != -1 && ((actor_mode_flee_data *)mode_data)->countdown_180 == 0 &&
                actor_check_weapon_pickup_reachable(actor_index, mode_data)) {
                ((actor_mode_flee_data *)mode_data)->destination = -1;
                ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
            }
            if (act[0x160]) {
                ((actor_mode_flee_data *)mode_data)->movement_cancelled = 0;
                mode_data[0xe] = 1;
                *(int32_t *)&((struct actor *)act)->last_flee_abort_time = game_time->game_time;
            } else if (((actor_mode_flee_data *)mode_data)->movement_cancelled) {
                actor_check_melee_target_reachable(actor_index, (int16_t *)mode_data);
                if (((actor_mode_flee_data *)mode_data)->destination == -1) {
                    mode_data[0xe] = 1;
                    *(int32_t *)&((struct actor *)act)->last_flee_abort_time = game_time->game_time;
                }
            }
        }
    }

    kind = ((actor_mode_flee_data *)mode_data)->panic;
    if (kind >= 9 && kind <= 12 && ((actor *)act)->unit_index != k_datum_index_none) {
        uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[((actor *)act)->unit_index & 0xffff].data;

        if (((unit_object *)unit)->unit.current_speech.priority <= 0) {
            mode_data[0x10] = 0;
        }
    }
    if (kind > 0 && ((actor_mode_flee_data *)mode_data)->destination != -1) {
        datum_index unit_index;
        int32_t now;
        uint8_t announced;

        if (mode_data[0xe]) {
            return 1;
        }
        unit_index = ((actor *)act)->unit_index;
        if (unit_index != k_datum_index_none) {
            announced = mode_data[0x10];
            now = game_time->game_time;
            if (!announced || *(int32_t *)(mode_data + 0x14) + 60 >= now) {
                if (kind == 11 || kind == 12) {
                    halo::units::unit_dispatch_reaction_animation(unit_index, 2);
                } else if (kind == 9 || kind == 10) {
                    halo::units::unit_dispatch_reaction_animation(unit_index, 1);
                } else {
                    datum_index source_object = k_datum_index_none;

                    if (((actor_mode_flee_data *)mode_data)->reference != k_datum_index_none) {
                        source_object = *(datum_index *)(PROP(((actor_mode_flee_data *)mode_data)->reference) + 0x18);
                    }
                    if (!announced) {
                        ai_communication_broadcast(0x1f + (kind == 8), unit_index, source_object, -1, -1, 4, 0);
                        mode_data[0x10] = 1;
                    } else {
                        ai_communication_broadcast(0x21, unit_index, source_object, -1, -1, -1, 0);
                    }
                }
                *(int32_t *)(mode_data + 0x14) = now;
            }
        }
    }
    return (uint8_t)(mode_data[0xe] || mode_data[0xf]);
}

extern "C" uint8_t actor_mode_flee_process(datum_index actor_index)
{
    return halo::ai::flee_mode(actor_index).process();
}

#undef ACTOR
#undef PROP

namespace c_actor_mode_flee_replace_reference {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_flee_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference);

/**
 * actor_mode_flee_replace_reference: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_replace_reference.c.txt.
 *
 * @address 0x404300
 */
void halo::ai::flee_mode::replace_reference(datum_index old_reference, datum_index new_reference)
{
    using namespace c_actor_mode_flee_replace_reference;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.flee.reference == old_reference) {
        ((struct actor *)act)->mode_data.flee.reference = new_reference;
    }
}

extern "C" void actor_mode_flee_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::flee_mode(actor_index).replace_reference(old_reference, new_reference);
}

#undef ACTOR

namespace c_actor_mode_flee_tick {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern game_time_globals *game_time;
}
}

extern "C" void actor_mode_flee_tick(datum_index actor_index);

/**
 * actor_mode_flee_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_tick.c.txt.
 *
 * @address 0x403af0
 */
void halo::ai::flee_mode::tick()
{
    using namespace c_actor_mode_flee_tick;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    ((struct actor *)act)->mode_data.flee.ticks_in_mode += 1;
    if (((struct actor *)act)->mode_data.flee.countdown_180 > 0) {
        ((struct actor *)act)->mode_data.flee.countdown_180 -= 1;
    }
    if (((struct actor *)act)->mode_data.flee.countdown_02 > 0) {
        ((struct actor *)act)->mode_data.flee.countdown_02 -= 1;
        if (((struct actor *)act)->mode_data.flee.countdown_02 == 0 && ((actor *)act)->unit_index != k_datum_index_none &&
            ((struct actor *)act)->mode_data.flee.panic >= 9 && ((struct actor *)act)->mode_data.flee.panic <= 12) {
            halo::units::unit_initialize_random_turn_angle(((actor *)act)->unit_index);
        }
    }
    if (((struct actor *)act)->mode_data.flee.panic > 0) {
        *(int32_t *)&((struct actor *)act)->panic_cooldown_time = game_time->game_time + 750;
    }
}

extern "C" void actor_mode_flee_tick(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).tick();
}

#undef ACTOR

namespace c_actor_mode_flee_update {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context);
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
}
}

extern "C" void actor_mode_flee_update(datum_index actor_index);

/**
 * actor_mode_flee_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_flee_update.c.txt.
 *
 * @address 0x403b90
 */
void halo::ai::flee_mode::update()
{
    using namespace c_actor_mode_flee_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    int16_t panic = ((struct actor *)act)->mode_data.flee.panic;
    datum_index target = ((actor *)act)->target_unit_index;
    int16_t destination;

    if (panic > 0) {
        ((actor *)act)->flee_reason = 6;
        ((actor *)act)->flee_source.code = 0;
        act[0x456] = 1;
    } else if (target != k_datum_index_none && *(int16_t *)(PROP(target) + 0x32) > 0) {
        ((actor *)act)->flee_reason = 7;
        ((actor *)act)->flee_source.code = 2;
        act[0x454] = 1;
    } else if (((struct actor *)act)->mode_data.flee.reference != k_datum_index_none) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 1;
        *(datum_index *)(act + 0x3f0) = ((struct actor *)act)->mode_data.flee.reference;
    } else {
        ((actor *)act)->flee_reason = 0;
    }
    ((struct actor *)act)->look_posture = 4;
    act[0x428] = (uint8_t)(((struct actor *)act)->mode_data.flee.panic > 0);
    act[0x429] = (uint8_t)(((struct actor *)act)->mode_data.flee.panic >= 9 && ((struct actor *)act)->mode_data.flee.panic <= 12);
    act[0x426] = 1;
    act[0x427] = 0;
    act[0x424] = 1;
    act[0x425] = 0;

    destination = ((struct actor *)act)->mode_data.flee.destination;
    if (destination == -1) {
        actor_movement_action_stop(actor_index);
        return;
    }
    if (!act[0x4c]) {
        return;
    }
    if (actor_movement_set_destination_firing_position(actor_index, destination, 0)) {
        ((actor *)act)->firing_position_index = ((struct actor *)act)->mode_data.flee.destination;
        act[0x3ba] = act[0xa6];
        return;
    }
    if (((actor *)act)->firing_position_index != -1) {
        actor_push_recognition_entry(actor_index, ((actor *)act)->firing_position_index, 0);
        actor_movement_action_stop(actor_index);
        ((actor *)act)->firing_position_index = -1;
    }
    ((struct actor *)act)->mode_data.flee.destination = -1;
    act[0xa2] = 1;
}

extern "C" void actor_mode_flee_update(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).update();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_mode_guard_enter {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void actor_target_reset_seen_flags(datum_index actor_index);
extern void actor_target_reset_shot_counters(datum_index actor_index);
}
}

extern "C" void actor_mode_guard_enter(datum_index actor_index);

/**
 * actor_mode_guard_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_enter.c.txt.
 *
 * @address 0x404820
 */
void halo::ai::guard_mode::enter()
{
    using namespace c_actor_mode_guard_enter;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    actor_target_reset_seen_flags(actor_index);
    act[0x98] = 0;
    if (act[0xa6]) {
        actor_target_reset_shot_counters(actor_index);
    }
}

extern "C" void actor_mode_guard_enter(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).enter();
}

#undef ACTOR

namespace c_actor_mode_guard_exit {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_guard_exit(datum_index actor_index);

/**
 * actor_mode_guard_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_exit.c.txt.
 *
 * @address 0x404870
 */
void halo::ai::guard_mode::exit()
{
    using namespace c_actor_mode_guard_exit;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (act[0xa1]) {
        ((struct actor *)act)->post_combat_action = 0;
        *(int32_t *)&((struct actor *)act)->post_combat_prop_index = -1;
    }
}

extern "C" void actor_mode_guard_exit(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).exit();
}

#undef ACTOR

namespace c_actor_mode_guard_get_look_weights {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *actor_mode_guard_look_weights_idle;
extern const float *actor_mode_guard_look_weights_a6;
extern const float *actor_mode_guard_look_weights_a5;
extern const float *actor_mode_guard_look_weights_ambush;
}
}

extern "C" void actor_mode_guard_get_look_weights(datum_index actor_index, float *out_weights);

/**
 * actor_mode_guard_get_look_weights: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_get_look_weights.c.txt.
 *
 * @address 0x4051c0
 */
void halo::ai::guard_mode::get_look_weights(float *out_weights)
{
    using namespace c_actor_mode_guard_get_look_weights;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;
    const float *source;

    if (!mode_data[0x8]) {
        source = actor_mode_guard_look_weights_idle;
    } else if (mode_data[0xa]) {
        source = actor_mode_guard_look_weights_a6;
    } else if (mode_data[0x9]) {
        source = actor_mode_guard_look_weights_a5;
    } else {
        source = actor_mode_guard_look_weights_ambush;
    }
    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

extern "C" void actor_mode_guard_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::guard_mode(actor_index).get_look_weights(out_weights);
}

#undef ACTOR

namespace c_actor_mode_guard_movement_cancelled {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_guard_movement_cancelled(datum_index actor_index);

/**
 * actor_mode_guard_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_movement_cancelled.c.txt.
 *
 * @address 0x405100
 */
void halo::ai::guard_mode::movement_cancelled()
{
    using namespace c_actor_mode_guard_movement_cancelled;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    int16_t kind;

    if (act[0xa4] && ((struct actor *)act)->mode_data.guard.stage == 3) {
        act[0xa4] = 0;
        ((struct actor *)act)->mode_data.guard.countdown_0c = 0;
        act[0xa6] = 0;
    }
    kind = ((struct actor *)act)->mode_data.guard.stage;
    if (kind == 3 || (kind == 1 && act[0x160] == 0)) {
        ((struct actor *)act)->mode_data.guard.stage = 0;
        ((struct actor *)act)->mode_data.guard.firing_position = -1;
        act[0xaa] = 1;
    }
}

extern "C" void actor_mode_guard_movement_cancelled(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).movement_cancelled();
}

#undef ACTOR

namespace c_actor_mode_guard_replace_reference {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_guard_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference);

/**
 * actor_mode_guard_replace_reference: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_replace_reference.c.txt.
 *
 * @address 0x405270
 */
void halo::ai::guard_mode::replace_reference(datum_index old_reference, datum_index new_reference)
{
    using namespace c_actor_mode_guard_replace_reference;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (((actor_mode_guard_data *)mode_data)->guard_target == old_reference) {
        ((actor_mode_guard_data *)mode_data)->guard_target = new_reference;
    }
    if (((actor_mode_guard_data *)mode_data)->hold_reference == old_reference) {
        ((actor_mode_guard_data *)mode_data)->hold_reference = new_reference;
        if (new_reference == k_datum_index_none) {
            mode_data[0xf] = 0;
        }
    }
}

extern "C" void actor_mode_guard_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::guard_mode(actor_index).replace_reference(old_reference, new_reference);
}

#undef ACTOR

namespace c_actor_mode_guard_target_cleared {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_guard_target_cleared(datum_index actor_index);

/**
 * actor_mode_guard_target_cleared: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_target_cleared.c.txt.
 *
 * @address 0x405180
 */
void halo::ai::guard_mode::target_cleared()
{
    using namespace c_actor_mode_guard_target_cleared;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.guard.stage == 2) {
        *(int32_t *)(act + 0xd0) = -1;
    }
}

extern "C" void actor_mode_guard_target_cleared(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).target_cleared();
}

#undef ACTOR

namespace c_actor_mode_guard_tick {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern int32_t actor_report_command_status(uint32_t actor_index);
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern datum_index actor_get_target_prop_object_index(datum_index actor_index);
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
}
}

extern "C" void actor_mode_guard_tick(datum_index actor_index);

/**
 * actor_mode_guard_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_tick.c.txt.
 *
 * @address 0x404b90
 */
void halo::ai::guard_mode::tick()
{
    using namespace c_actor_mode_guard_tick;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t ambush_over;

    if (!act[0x13] && act[0x484] && ((struct actor *)act)->mode_data.guard.countdown_00 > 0) {
        ((struct actor *)act)->mode_data.guard.countdown_00 -= 1;
        if (((struct actor *)act)->mode_data.guard.countdown_00 == 0 && !act[0x160] && !act[0x6]) {
            if (act[0xa1]) {
                actor_report_command_status(actor_index);
                ((struct actor *)act)->post_combat_action = 0;
                *(int32_t *)&((struct actor *)act)->post_combat_prop_index = -1;
                act[0xa1] = 0;
                act[0xa3] = 0;
                *(int32_t *)(act + 0xd8) = -1;
            }
            act[0xaa] = 1;
        }
    }
    if (((struct actor *)act)->mode_data.guard.countdown_02 > 0 && (!act[0xdc] || act[0x484])) {
        ((struct actor *)act)->mode_data.guard.countdown_02 -= 1;
        if (((struct actor *)act)->mode_data.guard.countdown_02 == 0) {
            *(int32_t *)(act + 0xd8) = -1;
        }
    }
    if (!act[0xa0] || !act[0x484]) {
        return;
    }
    if (act[0xa6]) {
        act[0xa6] = (uint8_t)(((struct actor *)act)->retreat_timer > 0);
        ambush_over = (uint8_t)(act[0xa6] == 0);
    } else {
        if (((struct actor *)act)->mode_data.guard.countdown_0c <= 0) {
            return;
        }
        ((struct actor *)act)->mode_data.guard.countdown_0c -= 1;
        ambush_over = (uint8_t)(((struct actor *)act)->mode_data.guard.countdown_0c == 0);
    }
    if (!ambush_over) {
        return;
    }
    actor_set_units_active(actor_index, 0);
    act[0xa4] = 0;
    act[0xa5] = 0;
    act[0xa6] = 0;
    ((struct actor *)act)->mode_data.guard.countdown_0c = 0;
    if (((struct actor *)act)->combat_status >= 2 && ((actor *)act)->unit_index != k_datum_index_none) {
        ai_communication_broadcast(0x23, ((actor *)act)->unit_index, actor_get_target_prop_object_index(actor_index),
                                   -1, -1, -1, 0);
    }
    if (((struct actor *)act)->mode_data.guard.stage == 3) {
        actor_push_recognition_entry(actor_index, ((struct actor *)act)->mode_data.guard.firing_position, 0);
        ((struct actor *)act)->mode_data.guard.firing_position = -1;
    }
    ((actor *)act)->firing_position_index = -1;
    if (act[0x160]) {
        ((struct actor *)act)->mode_data.guard.stage = 1;
        return;
    }
    ((struct actor *)act)->mode_data.guard.stage = 0;
    act[0xaa] = 1;
}

extern "C" void actor_mode_guard_tick(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).tick();
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_mode_guard_update {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra);
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context);
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
extern void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data);
extern int32_t actor_report_command_status(uint32_t actor_index);
}
}

extern "C" void actor_mode_guard_update(datum_index actor_index);

/**
 * actor_mode_guard_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_guard_update.c.txt.
 *
 * @address 0x404d60
 */
void halo::ai::guard_mode::update()
{
    using namespace c_actor_mode_guard_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint32_t actor_flags = *(uint32_t *)actor_tag;

    if ((actor_flags & 0x40) && ((struct actor *)act)->combat_status == 0) {
        act[0x426] = 1;
        act[0x427] = 1;
    } else {
        act[0x427] = 0;
        if (act[0xa4]) {
            act[0x426] = act[0xa6] ? (uint8_t)((actor_flags >> 23) & 1) : 1;
        } else {
            act[0x426] = (uint8_t)((actor_flags & 0x80) && ((struct actor *)act)->combat_status > 0);
        }
    }
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;

    if (act[0x4c] && !act[0x6]) {
        uint8_t in_place = 0;

        switch (((struct actor *)act)->mode_data.guard.stage) {
        case 0:
        case 1:
            actor_movement_action_stop(actor_index);
            in_place = 1;
            break;
        case 2: {
            float distance_squared = halo::math::vector3d_distance_squared(*(real_point3d *)(act + 0xc4), *(real_point3d *)(act + 0x12c));
            float radius = *(float *)(act + 0xd4);

            if (distance_squared < radius * radius) {
                actor_movement_action_stop(actor_index);
            } else {
                actor_movement_set_destination_point((real_point3d *)(act + 0xc4), actor_index, *(int32_t *)(act + 0xd0), -1);
            }
            in_place = (uint8_t)(distance_squared < 9.0f);
            break;
        }
        case 3: {
            int16_t position = ((struct actor *)act)->mode_data.guard.firing_position;

            if (position != -1) {
                ((actor *)act)->firing_position_index = position;
                act[0x3ba] = 0;
                if (!actor_movement_set_destination_firing_position(actor_index, position, 0)) {
                    actor_push_recognition_entry(actor_index, ((struct actor *)act)->mode_data.guard.firing_position, 0);
                    ((actor *)act)->firing_position_index = -1;
                }
            }
            if (!act[0x4a8]) {
                in_place = 1;
            } else {
                in_place = (uint8_t)(halo::math::vector3d_distance_squared(*(real_point3d *)(act + 0x12c), *(real_point3d *)(act + 0x4ac)) < 9.0f);
            }
            break;
        }
        default:
            break;
        }
        act[0xa0] = 1;
        if (in_place) {
            if (act[0xab]) {
                uint8_t *watched = PROP(((struct actor *)act)->mode_data.guard.hold_reference);

                act[0xab] = 0;
                *(int32_t *)(act + 0xac) = -1;
                actor_record_perception_event(actor_index, 2, 600);
                ai_communication_broadcast(7, ((actor *)act)->unit_index, *(datum_index *)(watched + 0x18), -1, -1, 2, 0);
            }
            if (act[0xa1]) {
                actor_report_command_status(actor_index);
                if (((struct actor *)act)->post_combat_action == 9 && ((struct actor *)act)->mode_data.guard.stage == 2) {
                    act[0xa3] = 1;
                }
            }
        }
    }

    if (act[0xa3]) {
        ((actor *)act)->flee_reason = 7;
        ((actor *)act)->flee_source.code = 2;
        act[0x454] = 1;
        act[0x45d] = 1;
        *(float *)(act + 0x460) = halo::math::globals().global_up3d_pointer->i * 0.05f + *(float *)(act + 0xc4);
        *(float *)(act + 0x464) = halo::math::globals().global_up3d_pointer->j * 0.05f + *(float *)(act + 0xc8);
        *(float *)(act + 0x468) = halo::math::globals().global_up3d_pointer->k * 0.05f + *(float *)(act + 0xcc);
    } else if (((struct actor *)act)->mode_data.guard.guard_target != k_datum_index_none) {
        ((actor *)act)->flee_reason = 5;
        ((actor *)act)->flee_source.code = 1;
        *(datum_index *)(act + 0x3f0) = ((struct actor *)act)->mode_data.guard.guard_target;
    } else if (act[0xb0]) {
        ((actor *)act)->flee_source.code = 4;
        ((actor *)act)->flee_reason = act[0xb1] ? 5 : 3;
        *(real_point3d *)(act + 0x3f0) = *(real_point3d *)(act + 0xb4);
    } else if (((struct actor *)act)->combat_status > 0 && ((actor *)act)->target_unit_index != k_datum_index_none) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 1;
        *(datum_index *)(act + 0x3f0) = ((actor *)act)->target_unit_index;
    } else {
        ((actor *)act)->flee_reason = 0;
    }
    ((struct actor *)act)->look_posture = ((struct actor *)act)->combat_status >= 4 ? 4 : 2;
}

extern "C" void actor_mode_guard_update(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).update();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_mode_uncover_enter {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern data_array *prop_data;
}
}

extern "C" void actor_mode_uncover_enter(datum_index actor_index);

/**
 * actor_mode_uncover_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_uncover_enter.c.txt.
 *
 * @address 0x4081e0
 */
void halo::ai::uncover_mode::enter()
{
    using namespace c_actor_mode_uncover_enter;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    float lo = *(float *)(actor_tag + 0x33c);
    float hi = *(float *)(actor_tag + 0x340);
    float t;
    int32_t ticks;

    if (!act[0x9f]) {
        if (!(lo > *(float *)(actor_tag + 0x344))) {
            lo = *(float *)(actor_tag + 0x344);
        }
        if (!(hi > *(float *)(actor_tag + 0x348))) {
            hi = *(float *)(actor_tag + 0x348);
        }
    }
    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    t = (float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    ((struct actor *)act)->mode_data.uncover.duration_ticks = ticks;
    ((struct actor *)act)->mode_data.uncover.remaining_ticks = ticks;
    if (((struct actor *)act)->mode_data.uncover.stage == 0 && ((actor *)act)->target_unit_index != k_datum_index_none &&
        ((struct actor *)act)->combat_status < 3) {
        uint8_t *target = (uint8_t *)prop_data->data + (((actor *)act)->target_unit_index & 0xffff) * 0x138;

        ai_communication_broadcast(0x15, ((actor *)act)->unit_index, ((prop *)target)->object_index, -1, -1, -1, 0);
    }
}

extern "C" void actor_mode_uncover_enter(datum_index actor_index)
{
    halo::ai::uncover_mode(actor_index).enter();
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_mode_uncover_get_look_weights {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *actor_mode_uncover_look_weights_active;
extern const float *hud_text_message_hold_color;
}
}

extern "C" void actor_mode_uncover_get_look_weights(datum_index actor_index, float *out_weights);

/**
 * actor_mode_uncover_get_look_weights: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_uncover_get_look_weights.c.txt.
 *
 * @address 0x408840
 */
void halo::ai::uncover_mode::get_look_weights(float *out_weights)
{
    using namespace c_actor_mode_uncover_get_look_weights;
    datum_index actor_index = datum;
    const float *source = ACTOR(actor_index)[0x9c] ? actor_mode_uncover_look_weights_active
                                                    : hud_text_message_hold_color;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

extern "C" void actor_mode_uncover_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::uncover_mode(actor_index).get_look_weights(out_weights);
}

#undef ACTOR

namespace c_actor_mode_uncover_movement_cancelled {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
}
}

extern "C" void actor_mode_uncover_movement_cancelled(datum_index actor_index);

/**
 * actor_mode_uncover_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_uncover_movement_cancelled.c.txt.
 *
 * @address 0x408800
 */
void halo::ai::uncover_mode::movement_cancelled()
{
    using namespace c_actor_mode_uncover_movement_cancelled;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (((actor_mode_uncover_data *)mode_data)->stage == 1) {
        *(int16_t *)(mode_data + 0xa) = -1;
        mode_data[0x1] = 1;
    }
}

extern "C" void actor_mode_uncover_movement_cancelled(datum_index actor_index)
{
    halo::ai::uncover_mode(actor_index).movement_cancelled();
}

#undef ACTOR

