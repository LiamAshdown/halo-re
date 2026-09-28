// actor_update_look_target  (Ghidra: actor_update_look_target, renamed)
// address 0x415480, size 3896 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (REWRITTEN from objdump 0x415480..0x4163b7)
// Per-tick look / aim / facing selection for one actor. Works on three direction caches: A (+0x5a4, facing),
//   B (+0x5b0, aiming) and C (+0x5bc, looking), which are published at the end to +0x6fc / +0x708 / +0x714
//   (actor_apply_queued_look_to_unit hands those to the unit's control block). Sources, in order:
//   - the flee / reason point F (+0x3e8 kind with record +0x3ec, or reason 7 = the kind-2 threat point while
//     +0x60c > 0 and +0x5f2 == 2), gated by the aim cone (definition +0x12c cos) and the side flags +0x58d/+0x58e;
//   - the vocalization point V (record +0x54c, priority +0x546 while +0x548 ticks run), switch on priority 2..8;
//   - the idle look search (actor_get_idle_facing_range, actor_resolve_look_target, wait timers +0x560/+0x564/
//     +0x568, records +0x56c/+0x57c) and the look randomizer;
//   - the body-turn check (cone against +0x174, lane via definition +0x134 cos and the side cosines
//     definition +0xb4/+0xb8 or +0xbc/+0xc0 when +0x6a == 3), the vertical facing flatten (not flying), the
//     facing-change hold (+0x58f/+0x590/+0x598, definition +0x330).
//   Finally flags +0x6d0 bit 0x20 = +0x591 and the aiming speed word +0x6f8 (0 for a flee look, look mode 4, or
//   vocalization kinds 3/6/10/11/12, else 1).
//   The draft swapped the two sources (it drove the first block with the vocalization priority and the switch
//   with the reason kind), sent the vocalization point into the flee point, and tested local copies where the
//   binary reads the actor's side flags -- so actor look / aim vectors came from the wrong source: a10 crewmen
//   stared upward.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include <stddef.h>
#include <string.h>
#include "objects.h"
#include "units.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254, stride 0x38

extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0, EAX
extern uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out,
    datum_index actor_index); // 0x4146c0, EAX reason, EDI out, stack actor
extern uint8_t point3d_within_horizontal_cone(const real_point3d *to_point, const real_point3d *reference,
    real min_cos_threshold); // 0x414910, EAX, EDX, stack
extern uint8_t actor_point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis,
    float min_cos_threshold, float side_thresholds[2]); // 0x414990, EAX, ECX, EDX, stack x2
extern uint8_t actor_resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table,
    uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback); // 0x414d00, EAX, stack x5
extern void actor_look_randomize_direction(datum_index actor_index, float *deviation_table, real_vector3d *base_direction); // 0x414f50
extern float *actor_get_idle_facing_range(datum_index actor_index); // 0x4150f0, EAX
extern int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table); // 0x415150, EAX, stack, EDI
extern uint8_t actor_reset_queued_look_vector(datum_index actor_index); // 0x417ae0
extern void actor_update_facing_change_timer(datum_index actor_index); // 0x423670, EAX
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern double cos(double x);
extern double fabs(double x);

#define ULT_V3(p) (*(real_point3d *)(p))

static uint8_t ult_cone(real_point3d *point, uint8_t *reference, float cos_threshold)
{
    return point3d_within_horizontal_cone(point, (real_point3d *)reference, cos_threshold);
}

static uint8_t ult_lane(real_point3d *point, uint8_t *forward, uint8_t *axis, float cos_threshold, float *side)
{
    return actor_point_in_directional_lane(point, (real_point3d *)forward, (real_point3d *)axis, cos_threshold, side);
}

void actor_update_look_target(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *definition = (uint8_t *)tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data;
    uint8_t *cache_a = a + 0x5a4;
    uint8_t *cache_b = a + 0x5b0;
    uint8_t *cache_c = a + 0x5bc;
    int16_t look_mode = ((struct actor *)a)->unknown_6dc;
    uint8_t flee_look = 0;                                  // [esp+0x19]
    uint8_t aim_speed_zero;

    if (look_mode == 1) {
        ULT_V3(cache_b) = ULT_V3(cache_a);
        ULT_V3(cache_c) = ULT_V3(cache_a);
    } else {
        uint8_t has_weapon;                                 // [esp+0x15]
        uint8_t side_a = a[0x58d];                          // [esp+0x14]
        uint8_t look_follows = 0;                           // [esp+0x13]
        uint8_t free_aim = 1;                               // [esp+0x12]
        uint8_t side_b = a[0x58e];                          // [esp+0x17]
        uint8_t claimed = 0;                                // [esp+0x16]
        uint8_t in_cone = 0;                                // [esp+0x18]
        uint8_t resolved = 0;                               // [esp+0x1b]
        uint8_t range_1, trust;
        float cos_aim = *(float *)(definition + 0x12c);     // [esp+0x1c]
        float cos_look = *(float *)(definition + 0x134);    // [esp+0x28]
        float side_cos[2];                                  // [esp+0x48]
        int16_t reason;                                     // [esp+0x20]
        int16_t priority = 0;                               // [esp+0x24]
        real_point3d flee_point;                            // [esp+0x50]
        real_point3d voc_point;                             // [esp+0x2c]
        uint8_t section_done;                               // BL at 0x415807
        float *range;
        actor_flee_source_reason kind2;

        if (a[0x161] != 0) {
            has_weapon = 1;
        } else if (look_mode == 0 || look_mode == 2) {
            has_weapon = actor_get_threat_weapon_object_index(actor_index) != k_datum_index_none;
        } else {
            has_weapon = 0;
        }
        look_follows = has_weapon;
        if (((actor *)a)->awareness_level == 3) {
            side_cos[0] = (float)cos((double)*(float *)(definition + 0xbc));
            side_cos[1] = (float)cos((double)*(float *)(definition + 0xc0));
        } else {
            side_cos[0] = (float)cos((double)*(float *)(definition + 0xb4));
            side_cos[1] = (float)cos((double)*(float *)(definition + 0xb8));
        }

        // the flee / reason source
        memset(&kind2, 0, sizeof(kind2));
        kind2.code = 2;
        if (((struct actor *)a)->unknown_60c > 0 && ((struct actor *)a)->unknown_5f2 == 2 && a[0x456] == 0 &&
            actor_resolve_flee_source_point(&kind2, (real_vector3d *)&flee_point, actor_index)) {
            reason = 7;
            flee_look = 1;
        } else {
            reason = (int16_t)*(uint16_t *)&((actor *)a)->vocalization_unknown_3e8;
            if (reason != 0 && reason != 1) {
                if (actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x3ec),
                        (real_vector3d *)&flee_point, actor_index)) {
                    flee_look = ((actor *)a)->vocalization_unknown_3ec == 2;
                } else {
                    reason = 0;
                }
            }
        }

        // the vocalization source
        if (((actor *)a)->vocalization_line >= 0 && ((actor *)a)->vocalization_state > 0 &&
            actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x54c), (real_vector3d *)&voc_point,
                actor_index)) {
            priority = (int16_t)*(uint16_t *)&((actor *)a)->vocalization_variant;
        }
        if (a[0x504] != 0 &&
            *(int16_t *)((uint8_t *)actor_mode_definitions + ((actor *)a)->mode * 0x38 + 4) == 2 && priority > 5) {
            priority = 5;
        }
        if (((actor *)a)->vocalization_state > 0) {
            ((actor *)a)->vocalization_state = (int16_t)(((actor *)a)->vocalization_state - 1);
            if (((actor *)a)->vocalization_state == 0) {
                ((actor *)a)->vocalization_line = 0;
                ((actor *)a)->vocalization_variant = 0;
            }
        }
        a[0x58c] = 0;

        // 0x4156f5: the reason point
        if (reason < 2) {
            section_done = resolved;
        } else {
            uint8_t commit = 0;

            if (reason >= 5 && (side_a || ult_cone(&flee_point, cache_a, cos_aim))) {
                commit = 1;
            } else if (reason >= 3 && side_b) {
                side_b = 0;
                side_a = 1;
                commit = 1;
            }
            if (commit) {
                ULT_V3(cache_b) = flee_point;
                free_aim = 0;
                look_follows = has_weapon;
                if (reason >= 7) {
                    claimed = 1;
                    if (has_weapon) {
                        ULT_V3(cache_c) = flee_point;
                    }
                }
            }
            if (reason == 2) {
                reason = side_a ? 5 : 0;
            }
            if (side_a) {
                ULT_V3(cache_a) = flee_point;
                a[0x591] = (uint8_t)(a[0x591] | (reason == 4));
                side_a = 0;
                side_b = 0;
            }
            section_done = ((a[0x58d] == 0 && a[0x58e] == 0) || reason >= 6) ? 1 : 0;
        }

        // 0x415807: the vocalization point
        if (priority >= 2 && priority <= 6) {
            in_cone = ult_cone(&voc_point, cache_a, cos_aim);
            if (section_done) {
                if (claimed) {
                    goto lane_or_take;
                }
                goto cone_gate;
            }
            if (claimed) {
                goto lane_or_take;
            }
            if (priority >= 6 && actor_reset_queued_look_vector(actor_index)) {
                goto take_all;
            }
            if (priority >= 5 && (side_b || a[0x58d] != 0)) {
                goto take_all;
            }
            if (priority >= 4) {
                if (in_cone) {
                    goto priority_gate;
                }
                if (!side_a || !free_aim) {
                    goto lane_or_take;
                }
                goto take_all;
            }
        cone_gate:
            if (!in_cone) {
                goto lane_or_take;
            }
        priority_gate:
            if (priority >= 5 || (priority >= 3 && free_aim)) {
                ULT_V3(cache_b) = voc_point;
                ULT_V3(cache_c) = voc_point;
                look_follows = has_weapon;
                goto voc_claim;
            }
        lane_or_take:
            if (has_weapon && ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                ULT_V3(cache_c) = voc_point;
                look_follows = 0;
            } else if (free_aim && in_cone) {
                ULT_V3(cache_b) = voc_point;
                ULT_V3(cache_c) = voc_point;
                look_follows = 1;
                free_aim = 0;
            }
            goto switch_done;
        take_all:
            if (!(a[0x591] != 0 && in_cone)) {
                ULT_V3(cache_a) = voc_point;
                a[0x591] = 0;
            }
            ULT_V3(cache_b) = voc_point;
            ULT_V3(cache_c) = voc_point;
            goto voc_face;
        } else if (priority == 7 || priority == 8) {
            resolved = in_cone = (uint8_t)(priority == 8);
            if (a[0x58d] == 0) {
                if (!ult_cone(&voc_point, cache_a, cos_aim)) {
                    if (!actor_reset_queued_look_vector(actor_index)) {
                        goto switch_done;
                    }
                    in_cone = 1;
                } else if (!resolved) {
                    goto voc_aim;
                }
            }
            ULT_V3(cache_a) = voc_point;
            a[0x591] = in_cone;
        voc_aim:
            ULT_V3(cache_b) = voc_point;
            ULT_V3(cache_c) = voc_point;
        voc_face:
            side_a = 0;
            look_follows = has_weapon;
        voc_claim:
            a[0x58c] = 1;
            claimed = 0;
            free_aim = 0;
        }
    switch_done:
        if (reason == 2 && free_aim && ult_cone(&flee_point, cache_a, cos_aim)) {
            ULT_V3(cache_b) = flee_point;
            if (look_follows) {
                ULT_V3(cache_c) = flee_point;
            }
            a[0x58c] = 0;
            free_aim = 0;
        }

        // 0x415b0d: the idle look search
        range = actor_get_idle_facing_range(actor_index);
        range_1 = range[1] > 0.0f;
        side_b = range[3] > 0.0f;
        in_cone = range[5] > 0.0f;
        if (((struct actor *)a)->unknown_3fc > 0 && !claimed && (free_aim || look_follows) &&
            (range_1 || side_b || in_cone)) {
            resolved = 0;
            claimed = 0;
            trust = (range_1 && side_a && reason == 1 && *(int32_t *)(a + 0x560) == 0) ? 1 : 0;
            if (*(int32_t *)(a + 0x560) > 0) {
                *(int32_t *)(a + 0x560) -= 1;
            }
            if (a[0x55c] != 0 && a[0x55d] != 0 && !free_aim) {
                a[0x55c] = 1;
                ((struct actor *)a)->unknown_564 = actor_look_get_wait_ticks(actor_index, 2, 1, range);
                ULT_V3(a + 0x570) = ULT_V3(cache_b);
                ((struct actor *)a)->unknown_56c = 4;
            }
            if (!(a[0x55c] != 0 && ((struct actor *)a)->unknown_564 != 0)) {
                uint8_t use_aiming;
                uint8_t force = 0;
                uint8_t *direction = 0;

                if (free_aim && side_b) {
                    use_aiming = 1;
                    force = (look_follows && in_cone) ? 1 : 0;
                    direction = cache_a;
                } else {
                    use_aiming = 0;
                    if (look_follows && in_cone) {
                        direction = cache_b;
                    }
                }
                if (direction != 0) {
                    a[0x55e] = actor_resolve_look_target((real_point3d *)direction, actor_index, range, trust,
                        use_aiming, force);
                    resolved = 1;
                }
            }
            if (a[0x55c] != 0) {
                ((struct actor *)a)->unknown_564 -= 1;
                if (actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x56c), (real_vector3d *)&voc_point,
                        actor_index)) {
                    if (free_aim) {
                        if (side_a && range_1 && a[0x99] != 0) {
                            trust = 1;
                            claimed = 1;
                            goto idle_take_ab;
                        }
                        if (trust) {
                            goto idle_take_ab;
                        }
                        if (!ult_cone(&voc_point, cache_a, cos_aim)) {
                            goto idle_reset;
                        }
                        ULT_V3(cache_b) = voc_point;
                        a[0x58c] = 1;
                        goto idle_timers;
                    idle_take_ab:
                        ULT_V3(cache_a) = voc_point;
                        ULT_V3(cache_b) = voc_point;
                        a[0x58c] = 1;
                        goto idle_timers;
                    }
                    if (!ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                        goto idle_reset;
                    }
                    ULT_V3(cache_c) = voc_point;
                    goto idle_timers;
                }
            }
        idle_reset:
            voc_point = ULT_V3(cache_b);
            a[0x55c] = 0;
            goto idle_follow;
        idle_timers:
            if (resolved && in_cone) {
                a[0x55f] = 1;
                *(int32_t *)(a + 0x568) = actor_look_get_wait_ticks(actor_index, 2, a[0x55e], range);
                memcpy(a + 0x57c, a + 0x56c, 16);
                if (trust) {
                    *(int32_t *)(a + 0x560) = actor_look_get_wait_ticks(actor_index, 0, a[0x55e], range);
                }
            }
        idle_follow:
            if (!free_aim) {
                goto clear_hold;
            }
            if (!((look_follows && in_cone) || (claimed && side_b))) {
                goto clear_hold;
            }
            if (*(int32_t *)(a + 0x568) == 0) {
                actor_look_randomize_direction(actor_index, range, (real_vector3d *)&voc_point);
            }
            *(int32_t *)(a + 0x568) -= 1;
            if (a[0x55f] == 0) {
                goto body_turn;
            }
            if (!actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x57c), (real_vector3d *)&flee_point,
                    actor_index)) {
                goto clear_hold;
            }
            if (claimed ? !ult_cone(&flee_point, cache_a, cos_aim)
                        : !ult_lane(&flee_point, cache_b, cache_a, cos_look, side_cos)) {
                goto clear_hold;
            }
            if (claimed) {
                ULT_V3(cache_b) = flee_point;
            }
            ULT_V3(cache_c) = flee_point;
            goto body_turn;
        }
        a[0x55c] = 0;
        a[0x55e] = 0;
    clear_hold:
        a[0x55f] = 0;
    body_turn:
        // 0x415fb6: turning the body toward the aim
        if (a[0x504] == 0 && a[0x505] == 0 && !unit_is_in_busy_animation_state(*(uint32_t *)&((actor *)a)->unit_index) &&
            ((actor *)a)->active_unit_index == k_datum_index_none) {
            if (ult_cone((real_point3d *)cache_b, cache_a, cos_aim) &&
                !ult_cone((real_point3d *)cache_b, a + 0x174, cos_aim)) {
                a[0x591] = 1;
            } else if (has_weapon) {
                if (ult_lane((real_point3d *)cache_c, cache_b, cache_a, cos_look, side_cos) &&
                    !ult_lane((real_point3d *)cache_c, cache_b, a + 0x174, cos_look, side_cos)) {
                    a[0x591] = 1;
                }
            }
        }
        if (!has_weapon) {
            ULT_V3(cache_c) = ULT_V3(cache_b);
        }
    }

    // 0x41609a: keep the facing horizontal unless flying
    if (a[0x99] == 0 && !(fabs((double)((actor *)a)->position_cache_a.z) < 9.999999747378752e-05)) {
        ((actor *)a)->position_cache_a.z = 0.0f;
        if (vector2d_normalize_with_length((real_vector2d *)cache_a) == 0.0f) {
            ULT_V3(cache_a) = ULT_V3(a + 0x174);
        }
    }
    if (a[0x58f] != 0) {
        if (a[0x590] == 0) {
            if (a[0x504] == 0 &&
                ((actor *)a)->facing_unknown_180.k * ((actor *)a)->position_cache_b.z + ((actor *)a)->facing_unknown_180.j * ((actor *)a)->position_cache_b.y +
                ((actor *)a)->facing_unknown_180.i * ((actor *)a)->position_cache_b.x > 0.9f) {
                ULT_V3(a + 0x598) = ULT_V3(cache_a);
                a[0x590] = 1;
            }
        } else if (*(float *)(definition + 0x330) > 0.0f) {
            float limit = (float)cos((double)*(float *)(definition + 0x330));
            uint8_t keep = 0;

            if (a[0x99] != 0) {
                keep = ((actor *)a)->position_cache_a.z * *(float *)(a + 0x5a0) + ((actor *)a)->position_cache_a.y * *(float *)(a + 0x59c) +
                       ((actor *)a)->position_cache_a.x * *(float *)(a + 0x598) > limit &&
                       *(float *)(a + 0x5a0) * ((actor *)a)->position_cache_b.z + *(float *)(a + 0x59c) * ((actor *)a)->position_cache_b.y +
                       ((actor *)a)->position_cache_b.x * *(float *)(a + 0x598) > limit;
            } else {
                real_vector2d aim2, face2, hold2;

                aim2.i = ((actor *)a)->position_cache_b.x;
                aim2.j = ((actor *)a)->position_cache_b.y;
                face2.i = ((actor *)a)->position_cache_a.x;
                face2.j = ((actor *)a)->position_cache_a.y;
                hold2.i = *(float *)(a + 0x598);
                hold2.j = *(float *)(a + 0x59c);
                if (vector2d_normalize_with_length(&face2) != 0.0f && vector2d_normalize_with_length(&aim2) != 0.0f &&
                    vector2d_normalize_with_length(&hold2) != 0.0f) {
                    keep = hold2.j * face2.j + hold2.i * face2.i > limit && aim2.j * hold2.j + aim2.i * hold2.i > limit;
                }
            }
            if (!keep) {
                a[0x590] = 0;
                actor_update_facing_change_timer(actor_index);
            }
        }
    } else {
        a[0x590] = 0;
    }

    ULT_V3(a + 0x6fc) = ULT_V3(cache_a);
    ULT_V3(a + 0x708) = ULT_V3(cache_b);
    ULT_V3(a + 0x714) = ULT_V3(cache_c);
    if (a[0x591] != 0) {
        ((actor *)a)->flags |= 0x20;
    } else {
        ((actor *)a)->flags &= ~0x20u;
    }

    aim_speed_zero = 1;
    if (!flee_look && ((struct actor *)a)->unknown_3fc != 4) {
        switch (((actor *)a)->vocalization_line) {
        case 3: case 6: case 10: case 11: case 12:
            break;
        default:
            aim_speed_zero = 0;
            break;
        }
    }
    *(int16_t *)(a + 0x6f8) = aim_speed_zero ? 0 : 1;
}

#if 0
Original Ghidra decompilation (0x415480): run `python tools/pack.py 0x415480`.
The listing is ~420 lines for a 3896-byte body and is not duplicated here, following the same
convention actor_find_best_firing_position.c, actor_update_aim_wander.c and
actor_update_firing_state.c use for the other oversized functions in this module. What makes
it unusually hard to diff against:
  - Ghidra models the Actor tag data pointer as the float local fVar1 / local_20 and then
    indexes it with `*(undefined4 *)((int)fVar1 + 300)`, so every tag-field read looks like
    float arithmetic. The four reads that matter are Actor+0x12c and +0x134 (the yaw halves of
    cosine_maximum_aiming_deviation and cosine_maximum_looking_deviation) and Actor+0xb4/0xb8
    versus +0xbc/0xc0 (the noncombat and combat look deltas, selected on awareness_level == 3).
  - the frame reuses one 16-bit slot (Ghidra's sVar14) first for actor.unknown_6dc and later
    for the low half of the look priority, and one float slot (local_30) first as a 16-bit
    constant 2 and later as the x of a candidate point.
  - about twenty helper calls have their register arguments dropped; the file header lists
    which of them were checked against the disassembly and which were inferred by analogy.
The Opus module review re-derived only the prologue (the mode-1 copy of the three direction
vectors at 0x5a4 / 0x5b0 / 0x5bc, the has_weapon gate and the four tag reads above) and found
those correct; the rest of the body is unverified and this file keeps the lowest rewrite
confidence in the module.
#endif
