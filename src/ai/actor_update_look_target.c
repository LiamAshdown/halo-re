// actor_update_look_target  (Ghidra: actor_update_look_target, renamed)
// address 0x415480, size 3896 bytes
// name confidence: 0.3   rewrite confidence: 0.25 (LEAST VERIFIED function in this module --
// read the UNSURE block below before trusting any single branch)
// evidence: ai_02.json's evidence for this address: "Reads actor look-mode field at +0x6dc,
// branches on unit posture/vehicle seat (+0x6a==3) selecting different unit orientation
// fields (0xbc/0xb4 etc.), and calls the look-direction helpers 00414910/00414990/00414d00/
// 00414f50/004150f0/00415150 plus obstacle/vector helpers" -- the central per-tick
// look-target orchestrator. It owns actor.position_cache_a/_b/_c (0x5a4/0x5b0/0x5bc: despite
// the name suggesting a read-only movement cache elsewhere, this function is one of their
// writers) and, at the very end, snapshots them into snapshot_facing / snapshot_unknown_708 /
// snapshot_unknown_714 (0x6fc/0x708/0x714) and updates actor.flags bit 0x20.
// register convention: actor_index is a genuine stack parameter (objdump -d -M intel:
// `mov ebp,[esp+0x58]` after 4 register pushes).
// blam-cc: stack -> actor_index
//
// UNSURE (read first): this function calls the four register-argument helpers
// actor_resolve_flee_source_point (0x4146c0), point3d_within_horizontal_cone (0x414910, a math-module
// single-cone test, not rewritten in this module), actor_point_in_directional_lane
// (0x414990) and actor_resolve_look_target (0x414d00) roughly twenty times between them.
// Ghidra shows only each call's visible stack argument and drops every implicit register
// argument. This rewrite is a deliberately literal, line-for-line translation of Ghidra's
// own control flow (same gotos, same branch order, same local-variable roles) rather than a
// restructuring, specifically to avoid introducing behavior changes while the register
// arguments below are reconstructed from a sample of call sites, not every one:
//   - actor_resolve_flee_source_point: objdump-verified at the 0x628-reason-2 call and the
//     vocalization_unknown_3ec call (both target the same output local, `flee_result`, via
//     EDI) and at the two look-wait-timer calls (0x56c-reason / 0x57c-reason, which target a
//     second local, `candidate`, via EDI -- confirmed by what Ghidra's own source reads
//     immediately afterward in each case). The vocalization_unknown_54c-reason call
//     (mid-function, feeding `priority`) is assumed to share the flee_result target by
//     analogy; not independently sampled.
//   - point3d_within_horizontal_cone / actor_point_in_directional_lane: objdump-verified at one call each
//     (to_point = &flee_result, reference/forward = &position_cache_b, cone_axis =
//     &position_cache_a); assumed identical at every other call to either function in this
//     file, since they always read the same two source locals in Ghidra's own code
//     immediately around each call.
//   - actor_resolve_look_target: objdump-verified in full (this is the same call site
//     documented in actor_resolve_look_target.c's own header); preferred_direction is
//     &position_cache_a or &position_cache_b depending on a flag pair this rewrite could not
//     fully separate from `priority` -- modeled here as "&position_cache_a when side_flag_a
//     and reason_kind == 1, else &position_cache_b", flagged UNSURE at that line specifically.
// Every other field access in this file is a direct, named replacement for Ghidra's raw
// offset with no reinterpretation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include <stddef.h> // size_t, for the pointer-to-uint32 cast below

// actor_flee_source_reason now lives in types/ai.h (folded from this file).

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern datum_index actor_get_threat_weapon_object_index(void); // 0x4282c0
extern uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out, datum_index actor_index); // 0x4146c0, this module
extern uint8_t point3d_within_horizontal_cone(real_point3d *to_point, real_point3d *reference, float min_cos_threshold); // 0x414910, not this module (math)
extern uint8_t actor_point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis,
                                                float min_cos_threshold, float side_thresholds[2]); // 0x414990, this module
extern uint8_t actor_resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table,
                                          uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback); // 0x414d00, this module
extern void actor_look_randomize_direction(datum_index actor_index, float *deviation_table, real_vector3d *base_direction); // 0x414f50, this module
extern float *actor_get_idle_facing_range(datum_index actor_index); // 0x4150f0, this module
extern int32_t actor_look_get_wait_ticks(int16_t mode, uint32_t flags, float *deviation_table); // 0x415150, this module
extern uint8_t actor_reset_queued_look_vector(datum_index actor_index); // 0x417ae0, this module
extern uint8_t actor_update_facing_change_timer(void); // 0x423670, not this module, UNSURE signature (called with no visible args)
extern uint8_t unit_is_in_busy_animation_state(datum_index actor_index); // 0x569c90, not yet rewritten
extern double cos(double x); // FCOS
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, vector in ECX

// blam-cc: stack -> actor_index
void actor_update_look_target(datum_index actor_index)
{
    actor *self;
    Actor *definition;
    int16_t look_mode;
    uint8_t flee_priority;         // bVar24

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    look_mode = self->unknown_6dc;
    flee_priority = 0;

    if (look_mode == 1) {
        self->position_cache_b = self->position_cache_a;
        self->position_cache_c = self->position_cache_a;
    } else {
        uint8_t has_weapon_or_forced;  // bVar23
        int8_t side_flag_a;            // local_48
        int8_t side_flag_b;            // local_45
        float aiming_cos_threshold;    // uVar2
        float looking_cos_threshold;   // uVar3
        uint8_t allow_flag;            // bVar8, init true
        uint8_t some_flag;             // bVar7, init false
        float side_thresholds[2];      // local_14 / local_10
        real_point3d flee_result;     // local_c / local_8 / local_4
        real_point3d candidate;       // local_30 / local_2c / local_28
        uint16_t reason_kind;          // uVar18
        uint32_t priority;             // local_38
        uint8_t rc;
        int16_t sw;                    // sVar14, re-used here for (short)priority
        uint8_t bVar6;                 // survives the switch into the tail below

        if (self->unknown_161 != 0) {
            has_weapon_or_forced = 1;
        } else if (look_mode == 0 || look_mode == 2) {
            has_weapon_or_forced = (actor_get_threat_weapon_object_index() != (datum_index)k_datum_index_none);
        } else {
            has_weapon_or_forced = 0;
        }

        side_flag_a = self->unknown_56e[31]; // 0x58d
        side_flag_b = self->unknown_56e[32]; // 0x58e
        aiming_cos_threshold = definition->cosine_maximum_aiming_deviation.yaw;
        looking_cos_threshold = definition->cosine_maximum_looking_deviation.yaw;
        allow_flag = 1;
        some_flag = 0;

        {
            float delta_r;
            if (self->awareness_level == 3) {
                side_thresholds[0] = (float)cos((double)definition->combat_look_delta_l);
                delta_r = definition->combat_look_delta_r;
            } else {
                side_thresholds[0] = (float)cos((double)definition->noncombat_look_delta_l);
                delta_r = definition->noncombat_look_delta_r;
            }
            side_thresholds[1] = (float)cos((double)delta_r);
        }

        if (self->unknown_60c < 1 || self->unknown_5f2 != 2 || self->unknown_455[1] != 0) {
            goto recompute_reason;
        } else {
            actor_flee_source_reason literal_reason = {2};
            rc = actor_resolve_flee_source_point(&literal_reason, (real_vector3d *)&flee_result, actor_index);
            if (rc == 0) goto recompute_reason;
            reason_kind = 7;
            flee_priority = 1;
            goto reason_kind_done;
        }
    recompute_reason:
        reason_kind = self->vocalization_unknown_3e8; // 0x3e8
        if (reason_kind != 0 && reason_kind != 1) {
            rc = actor_resolve_flee_source_point((actor_flee_source_reason *)&self->vocalization_unknown_3ec, (real_vector3d *)&flee_result, actor_index);
            if (rc == 0) {
                reason_kind = 0;
            } else {
                flee_priority = (self->vocalization_unknown_3ec == 2);
            }
        }
    reason_kind_done:

        priority = 0;
        if (self->vocalization_line >= 0 && self->vocalization_state > 0) {
            rc = actor_resolve_flee_source_point((actor_flee_source_reason *)&self->vocalization_unknown_54c, (real_vector3d *)&flee_result, actor_index);
            if (rc != 0) {
                priority = (uint16_t)self->vocalization_variant;
            }
        }

        if (self->unknown_504 != 0 && actor_mode_definitions[self->mode].combat_grade == 2 && priority >= 6) {
            priority = 5;
        }

        if (self->vocalization_state > 0) {
            self->vocalization_state = self->vocalization_state - 1;
            if (self->vocalization_state == 0) {
                self->vocalization_line = 0;
                self->vocalization_variant = 0;
            }
        }
        self->unknown_56e[30] = 0; // 0x58c

        {
            uint8_t bVar20; // a third flag, live into the switch below, then dead/overwritten after it

            if ((int16_t)priority < 2) {
                bVar20 = 0;
            } else {
                uint8_t cone_ok;
                if ((int16_t)priority < 5 ||
                    (side_flag_a == 0 && (cone_ok = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold), cone_ok == 0))) {
                    if (2 < (int16_t)priority && side_flag_b != 0) {
                        side_flag_b = 0;
                        side_flag_a = 1;
                        goto commit_flee_to_b;
                    }
                } else {
                commit_flee_to_b:
                    self->position_cache_b = flee_result;
                    allow_flag = 0;
                    if (6 < (int16_t)priority && has_weapon_or_forced) {
                        some_flag = 1;
                        self->position_cache_c = flee_result;
                    }
                }
                if (priority == 2) {
                    // Ghidra: uVar18 = -(ushort)(local_48 != '\0') & 5; -- 5 when side_flag_a set, 0 otherwise
                    priority = (side_flag_a != 0) ? 5u : 0u;
                }
                if (side_flag_a != 0) {
                    self->position_cache_a = flee_result;
                    side_flag_a = 0;
                    side_flag_b = 0;
                    self->unknown_591 = (uint8_t)(self->unknown_591 | (priority == 4));
                }
                if ((side_flag_a == 0 && side_flag_b == 0) || 5 < (int16_t)priority) {
                    bVar20 = 1;
                } else {
                    bVar20 = 0;
                }
            }

            {
                uint8_t cVar11 = 0;
                uint8_t cVar12;

                bVar6 = has_weapon_or_forced;

                sw = (int16_t)priority;
                switch (sw) {
                case 2: case 3: case 4: case 5: case 6:
                    cVar11 = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold);
                    if (!bVar20) {
                        if (some_flag) goto lane_test;
                        if ((sw < 6 || (cVar12 = actor_reset_queued_look_vector(actor_index), cVar12 == 0)) &&
                            (sw < 5 || (side_flag_b == 0 && side_flag_a == 0))) {
                            if (sw < 4) goto no_cone_check;
                            if (cVar11 != 0) goto cone_check_passed;
                            if (side_flag_a == 0 || !allow_flag) goto lane_test;
                        }
                        if (self->unknown_591 == 0 || cVar11 == 0) {
                            self->position_cache_a = candidate;
                            self->unknown_591 = 0;
                        }
                        self->position_cache_b = candidate;
                        self->position_cache_c = candidate;
                        goto after_switch_a84;
                    }
                    if (!some_flag) {
                    no_cone_check:
                        if (cVar11 == 0) goto lane_test;
                    cone_check_passed:
                        if (sw < 5 && (sw < 3 || !allow_flag)) goto lane_test;
                        self->position_cache_b = candidate;
                        self->position_cache_c = candidate;
                        goto after_switch_a91;
                    }
                lane_test:
                    if (!has_weapon_or_forced ||
                        (cVar12 = actor_point_in_directional_lane(&flee_result, &self->position_cache_b,
                                                                    &self->position_cache_a, looking_cos_threshold, side_thresholds),
                         cVar12 == 0)) {
                        if (!allow_flag || cVar11 == 0) break;
                        self->position_cache_b = candidate;
                        self->position_cache_c = candidate;
                        bVar6 = 1;
                        goto after_switch_a9d;
                    }
                    self->position_cache_c = candidate;
                    bVar6 = 0;
                    break;

                case 7: case 8: {
                    uint8_t is8 = (sw == 8);
                    if (self->unknown_56e[31] == 0) { // 0x58d, the ORIGINAL field, not the (possibly mutated) side_flag_a local
                        cVar11 = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold);
                        if (cVar11 == 0) {
                            cVar12 = actor_reset_queued_look_vector(actor_index);
                            if (cVar12 == 0) break;
                            is8 = 1;
                            goto commit_candidate_to_a;
                        }
                        if (is8) goto commit_candidate_to_a;
                    } else {
                    commit_candidate_to_a:
                        self->position_cache_a = candidate;
                        self->unknown_591 = is8;
                    }
                    self->position_cache_b = candidate;
                    self->position_cache_c = candidate;
                after_switch_a84:
                    side_flag_a = 0;
                after_switch_a91:
                    self->unknown_56e[30] = 1; // 0x58c
                    some_flag = 0;
                after_switch_a9d:
                    allow_flag = 0;
                    break;
                }
                default:
                    break;
                }

            if (priority == 2 && allow_flag) {
                cVar11 = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold);
                if (cVar11 != 0) {
                    self->position_cache_b = flee_result;
                    if (bVar6) {
                        self->position_cache_c = flee_result;
                    }
                    self->unknown_56e[30] = 0; // 0x58c
                    allow_flag = 0;
                }
            }
            }
        }

        {
            // idle_range[1]/[3]/[5] are deviation_table[0..5]'s odd (pitch) slots: mode 0's
            // pitch bound, mode 1's pitch bound, mode 2's pitch bound (see
            // actor_look_get_wait_ticks.c for the table layout).
            float *idle_range = actor_get_idle_facing_range(actor_index);
            uint8_t bVar25 = idle_range[1] <= 0.0f;
            uint8_t bVar26 = idle_range[3] <= 0.0f;
            uint8_t bVar20 = 0.0f < idle_range[5];
            uint8_t cVar11;
            uint8_t rc2;

            if ((self->unknown_3fc < 1 || some_flag || (!allow_flag && !bVar6)) ||
                (bVar25 && bVar26 && idle_range[5] <= 0.0f)) {
                self->unknown_55c = 0;
                self->unknown_55e[0] = 0; // 0x55e
            goto_look_end:
                self->unknown_55e[1] = 0; // 0x55f
            } else {
                uint8_t use_aiming_flag;    // local_30 reused as a byte flag in this section
                                             // (distinct from `candidate`, which local_30 also
                                             // names elsewhere in this function)
                uint8_t started_new = 0;    // bVar7, re-used in this second half
                uint8_t force_fallback_flag = 0; // bVar9

                // Ghidra: local_38 (priority) has its low byte cleared, or set to 1 and then
                // re-cleared if unknown_560 != 0 -- this directly modifies `priority`, which
                // is still live below (both as the switch selector's already-consumed value
                // and, from here on, only via its low byte).
                if (bVar25 || side_flag_a == 0 || priority != 1) {
                    priority &= 0xffffff00u;
                } else {
                    priority = (priority & 0xffffff00u) | 1u;
                    if ((*(int32_t *)&self->unknown_55e[2]) != 0) {
                        priority &= 0xffffff00u;
                    }
                }

                if ((*(int32_t *)&self->unknown_55e[2]) > 0) {
                    (*(int32_t *)&self->unknown_55e[2]) = (*(int32_t *)&self->unknown_55e[2]) - 1;
                }

                if (self->unknown_55c == 0) {
                arm_new_look:
                    use_aiming_flag = 1;
                    if (!allow_flag || bVar26) {
                        use_aiming_flag = 0;
                        if (bVar6 && bVar20) goto arm_look_shared;
                    } else {
                        if (bVar6 && bVar20) {
                            force_fallback_flag = 1;
                        } else {
                        arm_look_shared:
                            force_fallback_flag = 0;
                        }
                        // UNSURE: preferred_direction selection -- see file header. objdump
                        // shows EAX = &position_cache_a exactly when (bVar6 && bVar20), else
                        // &position_cache_b, matching the same pair just tested above.
                        {
                            real_point3d *preferred = (bVar6 && bVar20) ? &self->position_cache_a : &self->position_cache_b;
                            uint8_t committed = actor_resolve_look_target(preferred, actor_index, idle_range,
                                                                            (uint8_t)priority, use_aiming_flag, force_fallback_flag);
                            self->unknown_55e[0] = committed; // 0x55e
                        }
                        started_new = 1;
                    }
                } else {
                    if (self->unknown_55d != 0 && !allow_flag) {
                        self->unknown_55c = 1;
                        self->unknown_564 = actor_look_get_wait_ticks(2, 1, idle_range);
                        *(real_point3d *)&self->unknown_56e[2] = self->position_cache_b; // 0x570/0x574/0x578
                        self->unknown_56c = 4;
                    }
                    if (self->unknown_55c == 0 || self->unknown_564 == 0) goto arm_new_look;
                }

                if (self->unknown_55c == 0) {
                use_current:
                    candidate = self->position_cache_b;
                    self->unknown_55c = 0;
                } else {
                    self->unknown_564 = self->unknown_564 - 1;
                    rc2 = actor_resolve_flee_source_point((actor_flee_source_reason *)&self->vocalization_unknown_3ec, (real_vector3d *)&flee_result, actor_index);
                    if (rc2 == 0) goto use_current;
                    if (allow_flag) {
                        // side_flag_a=='\0' || bVar25 || !self->flying selects the
                        // "priority low byte == 0" shortcut; priority!=0 (or the else branch)
                        // falls through to the shared tail below.
                        if (side_flag_a == 0 || bVar25 || self->flying == 0) {
                            if ((uint8_t)priority == 0) {
                                cVar11 = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold);
                                if (cVar11 == 0) goto use_current;
                                self->position_cache_b = candidate;
                                self->unknown_56e[30] = 1; // 0x58c
                                goto look_committed;
                            }
                        } else {
                            priority = 1;
                            force_fallback_flag = 1;
                        }
                        self->position_cache_a = candidate;
                        self->position_cache_b = candidate;
                        self->unknown_56e[30] = 1; // 0x58c
                    } else {
                        cVar11 = actor_point_in_directional_lane(&flee_result, &self->position_cache_b,
                                                                   &self->position_cache_a, looking_cos_threshold, side_thresholds);
                        if (cVar11 == 0) goto use_current;
                        self->position_cache_c = candidate;
                    }
                look_committed:
                    if (started_new && bVar20) {
                        self->unknown_55e[1] = 1; // 0x55f
                        (*(int32_t *)self->unknown_568) = actor_look_get_wait_ticks(2, self->unknown_55e[0], idle_range);
                        // copies the whole 16-byte {code,pad,point} record at self+0x56c/0x570
                        // to self+0x57c/0x580 (four undefined4 copies in the original)
                        *(int16_t *)&self->unknown_56e[14] = self->unknown_56c;              // 0x57c = 0x56c
                        *(real_point3d *)&self->unknown_56e[18] = *(real_point3d *)&self->unknown_56e[2]; // 0x580.. = 0x570..
                        if ((uint8_t)priority != 0) {
                            (*(int32_t *)&self->unknown_55e[2]) = actor_look_get_wait_ticks(0, self->unknown_55e[0], idle_range);
                        }
                    }
                }

                if (!allow_flag || ((!bVar6 || !bVar20) && (!force_fallback_flag || bVar26))) {
                    goto goto_look_end;
                }
                if ((*(int32_t *)self->unknown_568) == 0) {
                    actor_look_randomize_direction(actor_index, idle_range, (real_vector3d *)&candidate);
                }
                (*(int32_t *)self->unknown_568) = (*(int32_t *)self->unknown_568) - 1;
                if (self->unknown_55e[1] != 0) { // 0x55f
                    rc2 = actor_resolve_flee_source_point((actor_flee_source_reason *)&self->vocalization_unknown_3ec, (real_vector3d *)&flee_result, actor_index);
                    if (rc2 != 0) {
                        if (force_fallback_flag) {
                            cVar11 = point3d_within_horizontal_cone(&flee_result, &self->position_cache_a, aiming_cos_threshold);
                        } else {
                            cVar11 = actor_point_in_directional_lane(&flee_result, &self->position_cache_b,
                                                                       &self->position_cache_a, looking_cos_threshold, side_thresholds);
                        }
                        if (cVar11 != 0) {
                            if (force_fallback_flag) {
                                self->position_cache_b = flee_result;
                            }
                            self->position_cache_c = flee_result;
                            goto look_scheduled;
                        }
                    }
                    goto goto_look_end;
                }
            look_scheduled:;
            }
        }
    }

    // UNSURE: this whole "stationary facing" refinement block was reconstructed from Ghidra's
    // source alone (no disassembly cross-check for the three vector2d_normalize_with_length
    // call targets); the read order below (position_cache_b.xy, then position_cache_a.xy,
    // then the unknown_594[1]/[2] pair) follows the order Ghidra's own locals are populated
    // in, but the exact ECX argument to each of the three calls was not independently
    // verified.
    if (self->flying == 0 && 0.0001f <= ((self->position_cache_a.z < 0.0f) ? -self->position_cache_a.z : self->position_cache_a.z)) {
        self->position_cache_a.z = 0.0f;
        if (vector2d_normalize_with_length((real_vector2d *)&self->position_cache_a) == 0.0f) {
            self->position_cache_a.x = self->facing.i;
            self->position_cache_a.y = self->facing.j;
            self->position_cache_a.z = self->facing.k;
        }
    }

    if (self->unknown_56e[33] == 0) { // 0x58f
        self->unknown_56e[34] = 0;    // 0x590
        goto stationary_check_done;
    }
    if (self->unknown_56e[34] == 0) { // 0x590
        if (self->unknown_504 == 0 &&
            0.9f < self->facing_unknown_180.i * self->position_cache_b.x +
                   self->facing_unknown_180.j * self->position_cache_b.y +
                   self->facing_unknown_180.k * self->position_cache_b.z) {
            self->unknown_594[1] = self->position_cache_a.x; // 0x598
            self->unknown_594[2] = self->position_cache_a.y; // 0x59c
            self->unknown_56e[34] = 1; // 0x590
            self->unknown_594[3] = self->position_cache_a.z; // 0x5a0
        }
        goto stationary_check_done;
    }
    if (definition->stationary_facing_angle <= 0.0f) goto stationary_check_done;
    {
        float cos_limit = (float)cos((double)definition->stationary_facing_angle);
        float dot_b;

        if (self->flying == 0) {
            real_vector2d vb, va, vr;
            vb.i = self->position_cache_b.x; vb.j = self->position_cache_b.y;
            va.i = self->position_cache_a.x; va.j = self->position_cache_a.y;
            vr.i = self->unknown_594[1]; vr.j = self->unknown_594[2]; // 0x598/0x59c

            if (vector2d_normalize_with_length(&vb) != 0.0f &&
                vector2d_normalize_with_length(&va) != 0.0f &&
                vector2d_normalize_with_length(&vr) != 0.0f &&
                cos_limit < vr.i * va.i + vr.j * va.j) {
                dot_b = vb.j * vr.j + vb.i * vr.i;
                if (cos_limit < dot_b) goto stationary_ok;
                goto stationary_reset;
            }
            goto stationary_reset;
        } else {
            if (cos_limit < self->position_cache_a.x * self->unknown_594[1] +
                             self->position_cache_a.y * self->unknown_594[2] +
                             self->position_cache_a.z * self->unknown_594[3]) {
                dot_b = self->unknown_594[2] * self->position_cache_b.y + self->unknown_594[3] * self->position_cache_b.z +
                        self->position_cache_b.x * self->unknown_594[1];
                if (cos_limit < dot_b) goto stationary_ok;
            }
            goto stationary_reset;
        }
    stationary_reset:
        self->unknown_56e[34] = 0; // 0x590
        actor_update_facing_change_timer();
    stationary_ok:;
    }
stationary_check_done:

    self->snapshot_facing.i = self->position_cache_a.x;
    self->snapshot_facing.j = self->position_cache_a.y;
    self->snapshot_facing.k = self->position_cache_a.z;
    self->snapshot_unknown_708.i = self->position_cache_b.x;
    self->snapshot_unknown_708.j = self->position_cache_b.y;
    self->snapshot_unknown_708.k = self->position_cache_b.z;
    self->snapshot_unknown_714.i = self->position_cache_c.x;
    self->snapshot_unknown_714.j = self->position_cache_c.y;
    self->snapshot_unknown_714.k = self->position_cache_c.z;

    if (self->unknown_591 == 0) {
        self->flags &= ~0x20u;
    } else {
        self->flags |= 0x20u;
    }

    if (!flee_priority && self->unknown_3fc != 4) {
        switch (self->vocalization_line) {
        case 3: case 6: case 10: case 11: case 12:
            break;
        default:
            *(int16_t *)&self->unknown_6ee[10] = 1; // 0x6f8
            return;
        }
    }
    *(int16_t *)&self->unknown_6ee[10] = 0; // 0x6f8
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
