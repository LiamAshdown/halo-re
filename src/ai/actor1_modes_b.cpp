#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_modes.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"

namespace c_actor_mode_charge_enter {

}


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
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.charge.stage == 4 &&
        *(int16_t *)((uint8_t *)halo::ai::actor_get_actor_definition(actor_index) + 0x156) == 3 &&
        act->special_fire_strafe_cooldown > 0) {
        act->special_fire_strafe_cooldown -= 1;
    }
}

namespace halo::ai {
void actor_mode_charge_enter(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).enter();
}
}


namespace c_actor_mode_charge_process {
extern "C" {

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

}
}


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
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    uint8_t *variant = TAG_DATA(act->actor_variant_tag);
    ActorVariant *definition = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);
    actor_mode_charge_data *md = &act->mode_data.charge;
    prop *target = 0;
    uint32_t actor_flags = actor_tag->flags;
    int16_t kind;
    float threshold;
    int32_t now;

    if (act->target_unit_index == k_datum_index_none) {
        md->close_in = 0;
    } else {
        target = halo::ai::prop_at(act->target_unit_index);
        kind = md->stage;
        if (static_cast<datum_index>(act->stuck_projectile_index) != k_datum_index_none || kind == 5 || kind == 4) {
            md->close_in = 1;
        } else if (kind == 2 || kind == 3) {

            float range = 3.4028235e38f;
            uint8_t check_range = 1;
            uint8_t use_retreat_range = act->berserking;

            if (!halo::ai::actor_has_unshielded_threat_weapon(actor_index)) {
                use_retreat_range = 1;
            }
            if (md->strike_started || md->jump_started || md->jump_solved) {
                check_range = 0;
            } else if (act->berserking || !halo::ai::actor_has_unshielded_threat_weapon(actor_index)) {
                range = use_retreat_range ? ((ActorVariant *)variant)->berserk_melee_abort_range : ((ActorVariant *)variant)->melee_abort_range;
            }
            if (act->charge_disallowed) {
                float limit = (0.0f > actor_tag->melee_fudge_factor ? 0.0f : actor_tag->melee_fudge_factor) + 0.8f;

                if (range > limit) {
                    range = limit;
                }
            }
            if (check_range && range < target->distance) {
                md->done = 1;
            } else {
                act->last_melee_time = halo::game::globals().game_time->game_time;
                md->close_in = 1;
                if (check_range) {
                    if (md->stage == 2) {
                        if (actor_tag->melee_leap_range[1] == 0.0f || actor_tag->melee_leap_chance == 0.0f) {
                            md->leap_allowed = 0;
                        } else if (target->flying || target->engaged_ticks > 0) {
                            md->leap_allowed = 1;
                        }
                        if (md->leap_allowed && (target->engaged_ticks > 0 ||
                                        actor_tag->melee_leap_range[0] * 1.5f < target->distance)) {
                            md->stage = 3;
                        }
                    } else if (target->distance < actor_tag->melee_leap_range[0]) {
                        md->stage = 2;
                        md->leap_allowed = 1;
                    }
                }
            }
        } else {

            kind = (int16_t)((actor_flags & 0x20000) && act->combat_status >= 5 && !act->berserking);
            md->stage = kind;
            if (kind == 1) {
                int16_t target_kind = ((struct actor *)target)->original_squad_index;
                uint8_t weak = (uint8_t)((target_kind == 0 || target_kind == 1) && (int8_t)(uint8_t)target->aiming_at_actor_class <= 2);

                md->target_weak = weak;
                md->close_in = (uint8_t)!(weak && (actor_flags & 0x40000));
                if (weak) {
                    md->weak_target_ticks += 1;
                }
                md->stand = 0;
                if (!weak && (int8_t)target->closing_speed_class <= 1) {
                    md->stand = 1;
                } else if (actor_tag->stalking_max_distance > 0.0f &&
                           !(target->distance < actor_tag->stalking_max_distance)) {
                    md->stand = 1;
                }
            } else if (!halo::ai::actor_has_unshielded_threat_weapon(actor_index) || act->in_water) {
                md->close_in = 1;
            } else {
                float range_lo;
                float range_hi;
                uint8_t *weapon;

                if (act->berserking) {
                    range_hi = definition->berserk_firing_ranges[1];
                    range_lo = definition->berserk_firing_ranges[0];
                } else {
                    range_hi = definition->desired_combat_range[1];
                    range_lo = definition->desired_combat_range[0];
                }
                weapon = (uint8_t *)halo::ai::actor_get_threat_weapon_definition(actor_index);
                if (weapon != 0 && *(float *)(weapon + 0x40c) > 0.0f && !(range_lo > *(float *)(weapon + 0x40c))) {
                    range_lo = *(float *)(weapon + 0x40c);
                }
                if (md->close_in) {
                    if (range_lo > target->distance) {
                        md->close_in = 0;
                    }
                } else if (target->distance > range_hi) {
                    md->close_in = 1;
                }
                if (target->distance > 0.7f && ((struct actor *)target)->original_squad_index != 0 &&
                    target->obstruction != 1) {
                    md->close_in = 1;
                }
            }
        }
    }

    if (md->strike_started) {
        datum_index unit_index = act->unit_index;

        md->strike_finished = (uint8_t)!(unit_index != k_datum_index_none && halo::units::unit_is_in_busy_animation_state(unit_index));
    } else if (!md->jump_solved && (md->stage == 2 || md->stage == 3) && target != 0) {
        real_vector3d direction;
        float along = 0.0f;
        float lead_ticks = 0.0f;
        uint8_t strike = 0;
        uint8_t *unit = 0;
        uint8_t have_along = 0;

        if (target->distance < 0.8f) {
            direction = *(real_vector3d *)((uint8_t *)target + 0xe0);
            strike = 1;
        } else {
            real_vector3d *velocity = (real_vector3d *)((uint8_t *)target + 0xd4);
            real_vector3d *facing = (real_vector3d *)((uint8_t *)target + 0xe0);
            float speed = halo::math::vector3d_length(*velocity);
            float factor = 0.0f;
            real_point3d lead;

            unit = (uint8_t *)halo::ai::object_at(act->unit_index);
            if (speed > 0.0f) {
                factor = ((velocity->k * facing->k + velocity->j * facing->j + velocity->i * facing->i) / speed + 1.0f) * 0.5f;
            }
            lead_ticks = (float)md->lead_ticks;
            halo::math::point3d_add_scaled(lead, *velocity, target->last_known_position, lead_ticks * factor);
            direction.i = lead.x - act->body_position.x;
            direction.j = lead.y - act->body_position.y;
            direction.k = lead.z - act->body_position.z;
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
            if (md->stage == 3 && !md->jump_started) {
                if (along < actor_tag->melee_leap_range[0] && target->engaged_ticks == 0 && !target->flying) {
                    md->done = 1;
                    act->last_melee_time = -1;
                } else if (along < actor_tag->melee_leap_range[1]) {
                    real_vector3d leap;
                    real half_gravity;
                    real horizontal_speed;

                    if (halo::ai::projectile_solve_ballistic_arc(&target->last_known_position, &act->body_position,
                                                       actor_tag->melee_leap_velocity, 1.0f, &actor_tag->melee_leap_ballistic,
                                                       0, &leap, 0, 0, 0, 0, &half_gravity, &horizontal_speed)) {
                        if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                            leap = *(real_vector3d *)&act->facing.i;
                            if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                                leap = *halo::math::globals().global_forward3d_pointer;
                            }
                        }
                        md->jump_direction.i = leap.i;
                        md->jump_direction.j = leap.j;
                        md->jump_solved = 1;
                        md->jump_horizontal_speed = horizontal_speed;
                        md->jump_vertical_speed = half_gravity;
                    }
                }
            } else if (md->suicide_charge) {
                if (along < actor_tag->melee_fudge_factor) {
                    strike = 1;
                } else if (along < actor_tag->suicide_sensing_dist) {
                    float closing = (velocity->j - ((unit_object *)unit)->base.velocity.j) * direction.j +
                                    (velocity->i - ((unit_object *)unit)->base.velocity.i) * direction.i +
                                    (velocity->k - ((unit_object *)unit)->base.velocity.k) * direction.k;

                    if (closing > 0.023333333f) {
                        strike = 1;
                    }
                }
            } else {
                if (md->stage == 3 && md->jump_started) {
                    along -= (direction.j * ((unit_object *)unit)->base.velocity.j + direction.k * ((unit_object *)unit)->base.velocity.k +
                              direction.i * ((unit_object *)unit)->base.velocity.i) * lead_ticks;
                }
                if (along < actor_tag->melee_fudge_factor + md->strike_range_extra) {
                    strike = 1;
                }
            }
        }
        (void)have_along;

        if (md->jump_solved || (strike && !md->suicide_charge)) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (halo::math::vector2d_normalize_with_length(flat) > 0.0f &&
                flat.j * act->facing.j + flat.i * act->facing.i < (md->jump_started ? 0.0f : 0.8660254f)) {
                md->jump_solved = 0;
                md->turning_to_face = 1;
                strike = 0;
                goto strike_done;
            }
        }
        if (strike) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (halo::math::vector2d_normalize_with_length(flat) == 0.0f) {
                flat.i = act->facing.i;
                flat.j = act->facing.j;
            }
            if (halo::units::unit_try_ready_weapon(act->unit_index, 0, &flat)) {
                halo::ai::ai_communication_broadcast(0x2b, act->unit_index, ((struct actor *)target)->unit_index, 3, -1, -1, 0);
                md->strike_started = 1;
            }
        }
    strike_done:;
    }

    now = halo::game::globals().game_time->game_time;
    kind = md->stage;
    if ((kind == 2 || kind == 3) && !md->strike_started && !md->jump_solved) {
        if (md->jump_started) {
            if (md->stage_ticks > 15) {
                md->done = 1;
            }
        } else if (actor_tag->melee_charge_time > 0.0f &&
                   !((float)md->charge_start_time + actor_tag->melee_charge_time * 30.0f > (float)now)) {
            md->done = 1;
        }
    }
    if (kind == 4 || kind == 5) {
        act->last_vehicle_charge_time = now;
    }
    threshold = halo::ai::actor_get_consideration_wait_threshold(actor_index, md->stage, (actor_combat_consideration *)md);
    md->wait_threshold = threshold;
    if (!act->swarm && act->needs_new_path) {
        md->approach_failed = 0;
        if (!md->strike_started && !md->jump_started && !md->jump_solved && md->close_in) {
            float radius = md->stage == 3 ? 4.0f : 1.5f;

            if (!(radius > threshold)) {
                radius = threshold;
            }
            if (halo::ai::actor_movement_set_destination_near_target(act->target_unit_index, actor_index, radius)) {
                halo::ai::actor_movement_actions_cancel(actor_index);
                goto approach_done;
            }
            md->approach_failed = 1;
            md->close_in = 0;
        }
        halo::ai::actor_movement_action_stop(actor_index);
    approach_done:
        if (act->target_combat_status >= 7) {
            datum_index target_index = act->target_unit_index;
            uint8_t far_away = (uint8_t)(((struct prop *)PROP(target_index))->distance > md->wait_threshold);
            uint8_t engaged = 0;
            int16_t current = md->stage;

            if (!((current == 2 || current == 3) && (md->jump_started || md->jump_solved || md->strike_started)) && far_away) {
                if (md->approach_failed || !halo::ai::actor_movement_action_is_complete(actor_index) ||
                    act->path_remaining_distance > md->wait_threshold) {
                    engaged = 1;
                }
            }
            halo::ai::actor_target_mark_engaged(target_index, actor_index, engaged);
        }
    }

    kind = md->stage;
    if (kind == 2 || kind == 3) {
        return (uint8_t)(md->done || md->strike_finished || md->approach_failed);
    }
    if (kind == 4 || kind == 5) {
        return md->approach_failed;
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_mode_charge_process(datum_index actor_index)
{
    return halo::ai::charge_mode(actor_index).process();
}
}

#undef PROP
#undef TAG_DATA

namespace c_actor_mode_charge_tick {
}


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
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.charge.stage == 3 && act->mode_data.charge.jump_started && !act->mode_data.charge.strike_started && !act->airborne) {
        act->mode_data.charge.stage_ticks += 1;
    }
}

namespace halo::ai {
void actor_mode_charge_tick(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).tick();
}
}


namespace c_actor_mode_charge_update {
extern "C" {


}
}


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
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    uint32_t actor_flags = actor_tag->flags;
    int16_t kind = act->mode_data.charge.stage;

    act->flee_source.code = 2;
    act->look_posture = 4;
    if ((kind == 2 || kind == 3) && act->mode_data.charge.turning_to_face && !act->moving && !act->movement_action_complete) {
        act->flee_reason = 4;
    } else if (act->combat_status >= 5 && kind != 1) {
        act->flee_reason = 7;
    } else {
        act->flee_reason = 5;
    }
    if (act->mode_data.charge.stage == 1) {
        act->crouch_decision[0] = (uint8_t)(act->mode_data.charge.stand == 0);
        act->crouch_decision[1] = (uint8_t)(act->mode_data.charge.stand == 0);
    } else if (!act->crouch_hold && (actor_flags & 0x10000)) {
        act->crouch_decision[0] = act->crouch_active;
        act->crouch_decision[1] = act->crouch_active;
    } else {
        act->crouch_decision[0] = 0;
        act->crouch_decision[1] = 0;
    }
    if (act->mode_data.charge.jump_solved) {
        act->jump_requested = 1;
        act->jump_is_leap = (uint8_t)(act->mode_data.charge.jump_horizontal_speed * 0.7f > act->mode_data.charge.jump_vertical_speed);
        act->jump_parameters_valid = 1;
        act->jump_facing.i = act->mode_data.charge.jump_direction.i;
        act->jump_facing.j = act->mode_data.charge.jump_direction.j;
        act->jump_horizontal_velocity = act->mode_data.charge.jump_horizontal_speed;
        act->jump_vertical_velocity = act->mode_data.charge.jump_vertical_speed;
        act->mode_data.charge.jump_started = 1;
        act->mode_data.charge.jump_solved = 0;
        act->mode_data.charge.stage_start_time = halo::game::globals().game_time->game_time;
        act->mode_data.charge.stage_ticks = 0;
    }
    if (actor_flags & 0x100000) {
        if (act->berserking || act->mode_data.charge.stage == 2 || act->mode_data.charge.stage == 3) {
            act->crouch_hold = (uint8_t)(act->mode_data.charge.close_in && !act->crouch_decision[1]);
        }
    }
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 0;
    act->unknown_42a = 1;
    act->wants_to_fire = (uint8_t)(act->mode_data.charge.stage != 1);
}

namespace halo::ai {
void actor_mode_charge_update(datum_index actor_index)
{
    halo::ai::charge_mode(actor_index).update();
}
}


namespace c_actor_mode_fight_tick {
}


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
    struct actor *actor = halo::ai::actor_at(actor_index);
    int16_t countdown = *(int16_t *)&actor->mode_data;

    if (countdown <= 0 || actor->movement_completed == 0) {
        return;
    }
    countdown = (int16_t)(countdown - 1);
    *(int16_t *)&actor->mode_data = countdown;
    if (countdown == 0 && *(uint16_t *)&actor->firing_position_index != halo::k_word_none && actor->firing_position_without_path == 0) {
        halo::ai::actor_push_recognition_entry(actor_index, actor->firing_position_index, 0);
    }
}

namespace halo::ai {
void actor_mode_fight_tick(uint32_t actor_index)
{
    halo::ai::fight_mode(actor_index).tick();
}
}

namespace c_actor_mode_fight_update {
}


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
    struct actor *actor = halo::ai::actor_at(actor_index);

    actor->crouch_decision[0] = actor->crouch_active;
    actor->flee_reason = 5;
    actor->flee_source.code = 2;
    actor->look_posture = 4;
    actor->crouch_decision[1] = 0;
    actor->crouch_hold = 0;
    actor->unknown_424[0] = 0;
    actor->unknown_424[1] = 0;
    if (actor->vehicle_driving_type != 4 && actor->combat_status >= 5) {
        actor->wants_to_fire = 1;
        actor->flee_reason = 7;
    }
}

namespace halo::ai {
void actor_mode_fight_update(uint32_t actor_index)
{
    halo::ai::fight_mode(actor_index).update();
}
}

namespace c_actor_mode_flee_enter {

}


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
    actor *act = halo::ai::actor_at(actor_index);
    int16_t kind = act->mode_data.flee.panic;

    act->mode_data.flee.ticks_in_mode = 0;
    if (kind > 0) {
        act->search_firing_positions = 0;
    }
    if (act->mode_data.flee.countdown_02 == 0 && act->unit_index != k_datum_index_none && kind >= 9 && kind <= 12) {
        halo::units::unit_initialize_random_turn_angle(act->unit_index);
    }
}

namespace halo::ai {
void actor_mode_flee_enter(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).enter();
}
}


namespace c_actor_mode_flee_exit {
#define ACTOR(h) ((uint8_t *)halo::ai::globals().actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)

}


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
    datum_index unit_index = ((struct actor *)ACTOR(actor_index))->unit_index;

    if (unit_index != k_datum_index_none) {
        unit_object *obj = (unit_object *)halo::ai::object_at(unit_index);

        obj->unit.flags &= ~0x2000000u;
    }
}

namespace halo::ai {
void actor_mode_flee_exit(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).exit();
}
}

#undef ACTOR

namespace c_actor_mode_flee_get_look_weights {
extern "C" {


extern const float *hud_text_message_normal_color;
extern const float *actor_mode_default_look_weights;
}
}


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
    const float *source = halo::ai::actor_at(actor_index)->mode_data.wait.countdown_0c > 0 ? hud_text_message_normal_color
                                                                        : actor_mode_default_look_weights;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

namespace halo::ai {
void actor_mode_flee_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::flee_mode(actor_index).get_look_weights(out_weights);
}
}


namespace c_actor_mode_flee_movement_cancelled {
}


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
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    ((actor_mode_flee_data *)mode_data)->destination = -1;
    ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
}

namespace halo::ai {
void actor_mode_flee_movement_cancelled(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).movement_cancelled();
}
}


namespace c_actor_mode_flee_process {
extern "C" {

#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

}
}


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
    actor *act = halo::ai::actor_at(actor_index);
    actor_mode_flee_data *mode_data = &act->mode_data.flee;
    int16_t kind;

    if (!act->swarm) {
        kind = mode_data->panic;
        if (kind >= 9 && kind <= 12) {
            mode_data->countdown_180 = 180;
        }
        if (mode_data->countdown_02 > 0) {
            mode_data->destination = -1;
        } else if (mode_data->destination == -1) {
            mode_data->movement_cancelled = 1;
        } else if (act->firing_position_index == -1) {
            mode_data->destination = -1;
            mode_data->movement_cancelled = 1;
        } else if (halo::ai::actor_is_target_within_engagement_range(actor_index)) {
            if (mode_data->countdown_180 != 0) {
                mode_data->movement_cancelled = 1;
            } else {
                mode_data->destination = act->firing_position_index;
                mode_data->destination_without_path = act->firing_position_without_path;
                mode_data->finished = 1;
                mode_data->movement_cancelled = 0;
                if (mode_data->reference != k_datum_index_none) {
                    prop *source = halo::ai::prop_at(mode_data->reference);
                    int16_t a = source->auditory_perception;
                    int16_t b = source->ambient_perception;

                    source->visual_perception = 0;
                    source->perception_level = a > b ? a : b;
                    source->obstruction = 2;
                    source->seen = 0;
                    halo::ai::actor_update_target_combat_status(actor_index);
                    halo::ai::actor_update_awareness_level(actor_index);
                }
            }
        }
        switch (mode_data->panic) {
        case 9:
        case 10:
            if (static_cast<datum_index>(act->stuck_projectile_index) == k_datum_index_none) {
                mode_data->finished = 1;
            }
            break;
        case 11:
            if (!act->unknown_1b4[0]) {
                mode_data->finished = 1;
            }
            break;
        case 12:
            if (!act->unknown_1b4[1]) {
                mode_data->finished = 1;
            }
            break;
        default:
            break;
        }
        if (act->needs_new_path && !mode_data->finished) {
            if (mode_data->destination != -1 && mode_data->countdown_180 == 0 &&
                halo::ai::actor_check_weapon_pickup_reachable(actor_index, mode_data)) {
                mode_data->destination = -1;
                mode_data->movement_cancelled = 1;
            }
            if (act->order_committed) {
                mode_data->movement_cancelled = 0;
                mode_data->engage = 1;
                act->last_flee_abort_time = static_cast<datum_index>(halo::game::globals().game_time->game_time);
            } else if (mode_data->movement_cancelled) {
                halo::ai::actor_check_melee_target_reachable(actor_index, mode_data);
                if (mode_data->destination == -1) {
                    mode_data->engage = 1;
                    act->last_flee_abort_time = static_cast<datum_index>(halo::game::globals().game_time->game_time);
                }
            }
        }
    }

    kind = mode_data->panic;
    if (kind >= 9 && kind <= 12 && act->unit_index != k_datum_index_none) {
        uint8_t *unit = (uint8_t *)halo::ai::object_at(act->unit_index);

        if (((unit_object *)unit)->unit.current_speech.priority <= 0) {
            mode_data->announced = 0;
        }
    }
    if (kind > 0 && mode_data->destination != -1) {
        datum_index unit_index;
        int32_t now;
        uint8_t announced;

        if (mode_data->engage) {
            return 1;
        }
        unit_index = act->unit_index;
        if (unit_index != k_datum_index_none) {
            announced = mode_data->announced;
            now = halo::game::globals().game_time->game_time;
            if (!announced || mode_data->announce_time + 60 >= now) {
                if (kind == 11 || kind == 12) {
                    halo::units::unit_dispatch_reaction_animation(unit_index, 2);
                } else if (kind == 9 || kind == 10) {
                    halo::units::unit_dispatch_reaction_animation(unit_index, 1);
                } else {
                    datum_index source_object = k_datum_index_none;

                    if (mode_data->reference != k_datum_index_none) {
                        source_object = ((struct prop *)PROP(mode_data->reference))->object_index;
                    }
                    if (!announced) {
                        halo::ai::ai_communication_broadcast(0x1f + (kind == 8), unit_index, source_object, -1, -1, 4, 0);
                        mode_data->announced = 1;
                    } else {
                        halo::ai::ai_communication_broadcast(0x21, unit_index, source_object, -1, -1, -1, 0);
                    }
                }
                mode_data->announce_time = now;
            }
        }
    }
    return (uint8_t)(mode_data->engage || mode_data->finished);
}

namespace halo::ai {
uint8_t actor_mode_flee_process(datum_index actor_index)
{
    return halo::ai::flee_mode(actor_index).process();
}
}

#undef PROP

namespace c_actor_mode_flee_replace_reference {
}


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
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.flee.reference == old_reference) {
        act->mode_data.flee.reference = new_reference;
    }
}

namespace halo::ai {
void actor_mode_flee_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::flee_mode(actor_index).replace_reference(old_reference, new_reference);
}
}


namespace c_actor_mode_flee_tick {
extern "C" {


}
}


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
    actor *act = halo::ai::actor_at(actor_index);

    act->mode_data.flee.ticks_in_mode += 1;
    if (act->mode_data.flee.countdown_180 > 0) {
        act->mode_data.flee.countdown_180 -= 1;
    }
    if (act->mode_data.flee.countdown_02 > 0) {
        act->mode_data.flee.countdown_02 -= 1;
        if (act->mode_data.flee.countdown_02 == 0 && act->unit_index != k_datum_index_none &&
            act->mode_data.flee.panic >= 9 && act->mode_data.flee.panic <= 12) {
            halo::units::unit_initialize_random_turn_angle(act->unit_index);
        }
    }
    if (act->mode_data.flee.panic > 0) {
        act->panic_cooldown_time = static_cast<datum_index>(halo::game::globals().game_time->game_time + 750);
    }
}

namespace halo::ai {
void actor_mode_flee_tick(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).tick();
}
}


namespace c_actor_mode_flee_update {
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

}


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
    actor *act = halo::ai::actor_at(actor_index);
    int16_t panic = act->mode_data.flee.panic;
    datum_index target = act->target_unit_index;
    int16_t destination;

    if (panic > 0) {
        act->flee_reason = 6;
        act->flee_source.code = 0;
        act->unknown_455[1] = 1;
    } else if (target != k_datum_index_none && ((struct prop *)PROP(target))->visual_perception > 0) {
        act->flee_reason = 7;
        act->flee_source.code = 2;
        act->wants_to_fire = 1;
    } else if (act->mode_data.flee.reference != k_datum_index_none) {
        act->flee_reason = 3;
        act->flee_source.code = 1;
        act->flee_source.payload.handle = act->mode_data.flee.reference;
    } else {
        act->flee_reason = 0;
    }
    act->look_posture = 4;
    act->crouch_hold = (uint8_t)(act->mode_data.flee.panic > 0);
    act->cowering = (uint8_t)(act->mode_data.flee.panic >= 9 && act->mode_data.flee.panic <= 12);
    act->crouch_decision[0] = 1;
    act->crouch_decision[1] = 0;
    act->unknown_424[0] = 1;
    act->unknown_424[1] = 0;

    destination = act->mode_data.flee.destination;
    if (destination == -1) {
        halo::ai::actor_movement_action_stop(actor_index);
        return;
    }
    if (!act->needs_new_path) {
        return;
    }
    if (halo::ai::actor_movement_set_destination_firing_position(actor_index, destination, 0)) {
        act->firing_position_index = act->mode_data.flee.destination;
        act->firing_position_without_path = act->mode_data.flee.destination_without_path;
        return;
    }
    if (act->firing_position_index != -1) {
        halo::ai::actor_push_recognition_entry(actor_index, act->firing_position_index, 0);
        halo::ai::actor_movement_action_stop(actor_index);
        act->firing_position_index = -1;
    }
    act->mode_data.flee.destination = -1;
    act->mode_data.flee.movement_cancelled = 1;
}

namespace halo::ai {
void actor_mode_flee_update(datum_index actor_index)
{
    halo::ai::flee_mode(actor_index).update();
}
}

#undef PROP

namespace c_actor_mode_guard_enter {

}


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
    actor *act = halo::ai::actor_at(actor_index);

    halo::ai::actor_target_reset_seen_flags(actor_index);
    act->search_firing_positions = 0;
    if (act->mode_data.guard.ambush_retreat) {
        halo::ai::actor_target_reset_shot_counters(actor_index);
    }
}

namespace halo::ai {
void actor_mode_guard_enter(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).enter();
}
}


namespace c_actor_mode_guard_exit {
}


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
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.guard.command_pending) {
        act->post_combat_action = 0;
        act->post_combat_prop_index = static_cast<datum_index>(-1);
    }
}

namespace halo::ai {
void actor_mode_guard_exit(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).exit();
}
}


namespace c_actor_mode_guard_get_look_weights {
extern "C" {


extern const float *actor_mode_guard_look_weights_idle;
extern const float *actor_mode_guard_look_weights_a6;
extern const float *actor_mode_guard_look_weights_a5;
extern const float *actor_mode_guard_look_weights_ambush;
}
}


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
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;
    const float *source;

    if (!mode_data->guard.ambush_active) {
        source = actor_mode_guard_look_weights_idle;
    } else if (mode_data->guard.ambush_retreat) {
        source = actor_mode_guard_look_weights_a6;
    } else if (mode_data->guard.ambush_triggered) {
        source = actor_mode_guard_look_weights_a5;
    } else {
        source = actor_mode_guard_look_weights_ambush;
    }
    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

namespace halo::ai {
void actor_mode_guard_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::guard_mode(actor_index).get_look_weights(out_weights);
}
}


namespace c_actor_mode_guard_movement_cancelled {
}


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
    actor *act = halo::ai::actor_at(actor_index);
    int16_t kind;

    if (act->mode_data.guard.ambush_active && act->mode_data.guard.stage == 3) {
        act->mode_data.guard.ambush_active = 0;
        act->mode_data.guard.countdown_0c = 0;
        act->mode_data.guard.ambush_retreat = 0;
    }
    kind = act->mode_data.guard.stage;
    if (kind == 3 || (kind == 1 && act->order_committed == 0)) {
        act->mode_data.guard.stage = 0;
        act->mode_data.guard.firing_position = -1;
        act->mode_data.guard.reselect = 1;
    }
}

namespace halo::ai {
void actor_mode_guard_movement_cancelled(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).movement_cancelled();
}
}


namespace c_actor_mode_guard_replace_reference {
}


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
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    if (((actor_mode_guard_data *)mode_data)->guard_target == old_reference) {
        ((actor_mode_guard_data *)mode_data)->guard_target = new_reference;
    }
    if (((actor_mode_guard_data *)mode_data)->hold_reference == old_reference) {
        ((actor_mode_guard_data *)mode_data)->hold_reference = new_reference;
        if (new_reference == k_datum_index_none) {
            mode_data->guard.watch_pending = 0;
        }
    }
}

namespace halo::ai {
void actor_mode_guard_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::guard_mode(actor_index).replace_reference(old_reference, new_reference);
}
}


namespace c_actor_mode_guard_target_cleared {
}


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
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.guard.stage == 2) {
        act->mode_data.guard.guard_point_surface = -1;
    }
}

namespace halo::ai {
void actor_mode_guard_target_cleared(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).target_cleared();
}
}


namespace c_actor_mode_guard_tick {

}


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
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t ambush_over;

    if (!act->keep_unit_alive && act->movement_completed && act->mode_data.guard.countdown_00 > 0) {
        act->mode_data.guard.countdown_00 -= 1;
        if (act->mode_data.guard.countdown_00 == 0 && !act->order_committed && !act->swarm) {
            if (act->mode_data.guard.command_pending) {
                halo::ai::actor_report_command_status(actor_index);
                act->post_combat_action = 0;
                act->post_combat_prop_index = static_cast<datum_index>(-1);
                act->mode_data.guard.command_pending = 0;
                act->mode_data.guard.attack_point = 0;
                act->mode_data.guard.guard_target = -1;
            }
            act->mode_data.guard.reselect = 1;
        }
    }
    if (act->mode_data.guard.countdown_02 > 0 && (!act->mode_data.guard.follow_movement || act->movement_completed)) {
        act->mode_data.guard.countdown_02 -= 1;
        if (act->mode_data.guard.countdown_02 == 0) {
            act->mode_data.guard.guard_target = -1;
        }
    }
    if (!act->mode_data.guard.settled || !act->movement_completed) {
        return;
    }
    if (act->mode_data.guard.ambush_retreat) {
        act->mode_data.guard.ambush_retreat = (uint8_t)(act->retreat_timer > 0);
        ambush_over = (uint8_t)(act->mode_data.guard.ambush_retreat == 0);
    } else {
        if (act->mode_data.guard.countdown_0c <= 0) {
            return;
        }
        act->mode_data.guard.countdown_0c -= 1;
        ambush_over = (uint8_t)(act->mode_data.guard.countdown_0c == 0);
    }
    if (!ambush_over) {
        return;
    }
    halo::ai::actor_set_units_active(actor_index, 0);
    act->mode_data.guard.ambush_active = 0;
    act->mode_data.guard.ambush_triggered = 0;
    act->mode_data.guard.ambush_retreat = 0;
    act->mode_data.guard.countdown_0c = 0;
    if (act->combat_status >= 2 && act->unit_index != k_datum_index_none) {
        halo::ai::ai_communication_broadcast(0x23, act->unit_index, halo::ai::actor_get_target_prop_object_index(actor_index),
                                   -1, -1, -1, 0);
    }
    if (act->mode_data.guard.stage == 3) {
        halo::ai::actor_push_recognition_entry(actor_index, act->mode_data.guard.firing_position, 0);
        act->mode_data.guard.firing_position = -1;
    }
    act->firing_position_index = -1;
    if (act->order_committed) {
        act->mode_data.guard.stage = 1;
        return;
    }
    act->mode_data.guard.stage = 0;
    act->mode_data.guard.reselect = 1;
}

namespace halo::ai {
void actor_mode_guard_tick(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).tick();
}
}


namespace c_actor_mode_guard_update {

}


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
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    uint32_t actor_flags = actor_tag->flags;

    if ((actor_flags & 0x40) && act->combat_status == 0) {
        act->crouch_decision[0] = 1;
        act->crouch_decision[1] = 1;
    } else {
        act->crouch_decision[1] = 0;
        if (act->mode_data.guard.ambush_active) {
            act->crouch_decision[0] = act->mode_data.guard.ambush_retreat ? (uint8_t)((actor_flags >> 23) & 1) : 1;
        } else {
            act->crouch_decision[0] = (uint8_t)((actor_flags & 0x80) && act->combat_status > 0);
        }
    }
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 0;

    if (act->needs_new_path && !act->swarm) {
        uint8_t in_place = 0;

        switch (act->mode_data.guard.stage) {
        case 0:
        case 1:
            halo::ai::actor_movement_action_stop(actor_index);
            in_place = 1;
            break;
        case 2: {
            float distance_squared = halo::math::vector3d_distance_squared(act->mode_data.guard.guard_point, act->body_position);
            float radius = act->mode_data.guard.guard_radius;

            if (distance_squared < radius * radius) {
                halo::ai::actor_movement_action_stop(actor_index);
            } else {
                halo::ai::actor_movement_set_destination_point(&act->mode_data.guard.guard_point, actor_index, act->mode_data.guard.guard_point_surface, -1);
            }
            in_place = (uint8_t)(distance_squared < 9.0f);
            break;
        }
        case 3: {
            int16_t position = act->mode_data.guard.firing_position;

            if (position != -1) {
                act->firing_position_index = position;
                act->firing_position_without_path = 0;
                if (!halo::ai::actor_movement_set_destination_firing_position(actor_index, position, 0)) {
                    halo::ai::actor_push_recognition_entry(actor_index, act->mode_data.guard.firing_position, 0);
                    act->firing_position_index = -1;
                }
            }
            if (!act->movement_action_complete) {
                in_place = 1;
            } else {
                in_place = (uint8_t)(halo::math::vector3d_distance_squared(act->body_position, act->path_end_point) < 9.0f);
            }
            break;
        }
        default:
            break;
        }
        act->mode_data.guard.settled = 1;
        if (in_place) {
            if (act->mode_data.guard.watch_pending) {
                prop *watched = halo::ai::prop_at(act->mode_data.guard.hold_reference);

                act->mode_data.guard.watch_pending = 0;
                act->mode_data.guard.hold_reference = -1;
                halo::ai::actor_record_perception_event(actor_index, 2, 600);
                halo::ai::ai_communication_broadcast(7, act->unit_index, ((struct actor *)watched)->unit_index, -1, -1, 2, 0);
            }
            if (act->mode_data.guard.command_pending) {
                halo::ai::actor_report_command_status(actor_index);
                if (act->post_combat_action == 9 && act->mode_data.guard.stage == 2) {
                    act->mode_data.guard.attack_point = 1;
                }
            }
        }
    }

    if (act->mode_data.guard.attack_point) {
        act->flee_reason = 7;
        act->flee_source.code = 2;
        act->wants_to_fire = 1;
        act->forced_aim_valid = 1;
        act->forced_aim_point.x = halo::math::globals().global_up3d_pointer->i * 0.05f + act->mode_data.flee.target_position.y;
        act->forced_aim_point.y = halo::math::globals().global_up3d_pointer->j * 0.05f + act->mode_data.flee.target_position.z;
        act->forced_aim_point.z = halo::math::globals().global_up3d_pointer->k * 0.05f + act->mode_data.guard.guard_point.z;
    } else if (act->mode_data.guard.guard_target != k_datum_index_none) {
        act->flee_reason = 5;
        act->flee_source.code = 1;
        act->flee_source.payload.handle = act->mode_data.guard.guard_target;
    } else if (act->mode_data.guard.look_point_valid) {
        act->flee_source.code = 4;
        act->flee_reason = act->mode_data.guard.look_point_hostile ? 5 : 3;
        act->flee_source.payload.point = act->mode_data.guard.look_point;
    } else if (act->combat_status > 0 && act->target_unit_index != k_datum_index_none) {
        act->flee_reason = 3;
        act->flee_source.code = 1;
        act->flee_source.payload.handle = act->target_unit_index;
    } else {
        act->flee_reason = 0;
    }
    act->look_posture = act->combat_status >= 4 ? 4 : 2;
}

namespace halo::ai {
void actor_mode_guard_update(datum_index actor_index)
{
    halo::ai::guard_mode(actor_index).update();
}
}


namespace c_actor_mode_uncover_enter {

}


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
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    float lo = actor_tag->uncover_delay_time[0];
    float hi = actor_tag->uncover_delay_time[1];
    float t;
    int32_t ticks;

    if (!act->mode_data.uncover.unknown_03) {
        if (!(lo > actor_tag->target_search_time[0])) {
            lo = actor_tag->target_search_time[0];
        }
        if (!(hi > actor_tag->target_search_time[1])) {
            hi = actor_tag->target_search_time[1];
        }
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    t = (float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    act->mode_data.uncover.duration_ticks = ticks;
    act->mode_data.uncover.remaining_ticks = ticks;
    if (act->mode_data.uncover.stage == 0 && act->target_unit_index != k_datum_index_none &&
        act->combat_status < 3) {
        prop *target = halo::ai::prop_at(act->target_unit_index);

        halo::ai::ai_communication_broadcast(0x15, act->unit_index, target->object_index, -1, -1, -1, 0);
    }
}

namespace halo::ai {
void actor_mode_uncover_enter(datum_index actor_index)
{
    halo::ai::uncover_mode(actor_index).enter();
}
}


namespace c_actor_mode_uncover_get_look_weights {
extern "C" {


extern const float *actor_mode_uncover_look_weights_active;
extern const float *hud_text_message_hold_color;
}
}


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
    const float *source = halo::ai::actor_at(actor_index)->mode_data.wait.finished ? actor_mode_uncover_look_weights_active
                                                    : hud_text_message_hold_color;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}

namespace halo::ai {
void actor_mode_uncover_get_look_weights(datum_index actor_index, float *out_weights)
{
    halo::ai::uncover_mode(actor_index).get_look_weights(out_weights);
}
}


namespace c_actor_mode_uncover_movement_cancelled {
}


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
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    if (((actor_mode_uncover_data *)mode_data)->stage == 1) {
        mode_data->uncover.firing_position = -1;
        mode_data->uncover.done = 1;
    }
}

namespace halo::ai {
void actor_mode_uncover_movement_cancelled(datum_index actor_index)
{
    halo::ai::uncover_mode(actor_index).movement_cancelled();
}
}


