// actor_mode_charge_process  (not a Ghidra function: actor mode table 0x65524c, mode "charge" slot +0x14)
// address 0x401da0, size 2878 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x401da0..0x4028de (no C existed: a mode entered in game would have hit a trap).
//   Charging (returns 1 when over). Mode data +0x04 is the charge kind: 0 shoot-while-closing, 1 rush, 2 / 3 melee
//   (3 = leap), 4 / 5 hold. The kind is chosen and gated against the target prop (+0x270): props out of weapon
//   range stop the charge (+0x28 keep closing / +0xa4 give up / +0xa6 leap allowed). Melee kinds aim at the
//   target's lead point (prop +0xbc along its velocity +0xd4, lead factor +0xce); a leap within the tag's leap
//   range (+0x384..+0x388) solves a ballistic arc (0x4beb30) and stores the leap (+0xb0..+0xbc, +0xa8); a strike in
//   range readies the melee (0x569a20) and says so (event 0x2b, +0xa2). Timeouts: leap +0xaa > 15, strike tag +0x380
//   seconds. The approach walks to the target within max(4 or 1.5, the consideration wait +0xc8) (0x417910) and at
//   morale 7+ the target is marked engaged when close (0x41fa80).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale); // 0x401930, EAX, ECX, stack
extern real vector3d_length(real_vector3d *v); // 0x401960, EAX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode,
                                                    actor_combat_consideration *consideration); // 0x4028e0, EAX, CX, EDI
extern void *actor_get_threat_weapon_definition(int32_t actor_index); // 0x40f970, EAX
extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius); // 0x417910, EAX, stack
extern void actor_movement_actions_cancel(datum_index actor_index); // 0x417a30, EAX
extern uint8_t actor_movement_action_is_complete(datum_index actor_index); // 0x41a960, EAX
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged); // 0x41fa80, EAX, EBX, stack
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX
extern uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin,
    real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc,
    real_vector3d *out_direction, real *max_speed_override, real *out_speed,
    real *out_time_of_flight, real *out_range, real *out_half_gravity_term,
    real *out_horizontal_speed); // 0x4beb30, EAX, ECX, ESI, EDI, stack
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX
extern uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const real_vector2d *direction); // 0x569a20, EDI, stack

uint8_t actor_mode_charge_process(datum_index actor_index)
{
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
            // 0x402034: melee kinds -- the strike range and the approach
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
            // kinds 0 / 1
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

    // 0x4021f2
    if (md[0x6]) {
        datum_index unit_index = ((actor *)act)->unit_index;

        md[0x7] = (uint8_t)!(unit_index != k_datum_index_none && unit_is_in_busy_animation_state(unit_index));
    } else if (!md[0xc] && (*(int16_t *)(md + 0x4) == 2 || *(int16_t *)(md + 0x4) == 3) && target != 0) {
        real_vector3d direction;     // S+0x28
        float along = 0.0f;          // the FPU value from 0x4023c1 on
        float lead_ticks = 0.0f;     // S+0x20
        uint8_t strike = 0;          // S+0x11
        uint8_t *unit = 0;           // S+0x1c
        uint8_t have_along = 0;

        if (*(float *)(target + 0x11c) < 0.8f) {
            direction = *(real_vector3d *)(target + 0xe0);
            strike = 1;
        } else {
            real_vector3d *velocity = (real_vector3d *)(target + 0xd4);
            real_vector3d *facing = (real_vector3d *)(target + 0xe0);
            float speed = vector3d_length(velocity);
            float factor = 0.0f;
            real_point3d lead;

            unit = (uint8_t *)((object_header *)object_data->data)[((actor *)act)->unit_index & 0xffff].data;
            if (speed > 0.0f) {
                factor = ((velocity->k * facing->k + velocity->j * facing->j + velocity->i * facing->i) / speed + 1.0f) * 0.5f;
            }
            lead_ticks = (float)*(int16_t *)(md + 0x32);
            point3d_add_scaled(&lead, velocity, (real_point3d *)(target + 0xbc), lead_ticks * factor);
            direction.i = lead.x - ((actor *)act)->body_position.x;
            direction.j = lead.y - ((actor *)act)->body_position.y;
            direction.k = lead.z - ((actor *)act)->body_position.z;
            if (direction.j * facing->j + direction.k * facing->k + direction.i * facing->i < 0.0f) {
                along = 0.0f;
                direction = *facing;
            } else {
                along = vector3d_normalize_with_length(&direction);
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
                        if (vector2d_normalize_with_length((real_vector2d *)&leap) == 0.0f) {
                            leap = *(real_vector3d *)&((actor *)act)->facing.i;
                            if (vector2d_normalize_with_length((real_vector2d *)&leap) == 0.0f) {
                                leap = *global_forward3d_pointer;
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
        // 0x4025ab: a leap facing the wrong way is abandoned (+0xa5)
        if (md[0xc] || (strike && !md[0x30])) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (vector2d_normalize_with_length(&flat) > 0.0f &&
                flat.j * ((actor *)act)->facing.j + flat.i * ((actor *)act)->facing.i < (md[0xb] ? 0.0f : 0.8660254f)) {
                md[0xc] = 0;
                md[0x9] = 1;
                strike = 0; // 0x402629 goes straight to 0x4026a7
                goto strike_done;
            }
        }
        if (strike) {
            real_vector2d flat;

            flat.i = direction.i;
            flat.j = direction.j;
            if (vector2d_normalize_with_length(&flat) == 0.0f) {
                flat.i = ((actor *)act)->facing.i;
                flat.j = ((actor *)act)->facing.j;
            }
            if (unit_try_ready_weapon(((actor *)act)->unit_index, 0, &flat)) {
                ai_communication_broadcast(0x2b, ((actor *)act)->unit_index, *(datum_index *)(target + 0x18), 3, -1, -1, 0);
                md[0x6] = 1;
            }
        }
    strike_done:;
    }

    // 0x4026a7
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
