// actor_target_update_tracking_speed  (Ghidra: actor_target_update_tracking_speed; named per
//   actor_target_data_acquire.c, which already declares and calls this address by this name)
// address 0x41c8f0, size 3753 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/ai.h actor.unknown_1d4/unknown_1d6/awareness_level/encounter_index/
//   unknown_3a0/active_unit_index/vitality_wait_time and prop.kind/is_unit/is_parented/
//   is_vault/engaged/relationship_object_index/distance/last_known_position/ground_position/
//   unknown_20/unknown_30..0x38/unknown_66/unknown_76/unknown_fc..0x10c/unknown_120..0x136
//   (the 0x120..0x137 byte run this function is one of the three "heavy readers"
//   ai_types_notes.md cites for prop); types/objects.h object.type/vitality_flags
//   (_object_type_biped, _object_health_frozen_bit); types/units.h unit_data.flags (0x204),
//   unit_data.aiming_vector (0x23c), unit_data.unknown_37c, unit_data.unknown_420,
//   unit_data.actor_index/swarm_actor_index (0x1f4/0x1f8); types/tags.h Actor.melee_fudge_factor
//   (0x37c) and the "suicidal_melee_attack" flags bit (bit 27, per ActorFlags' documented
//   order); encounter.unknown_40/41/42/44/45/58 (established by actor_target_scan_potential_
//   targets.c's "is_vault" gate and by this function's own tail drop-logic cross-check against
//   actor.unknown_3a0). Already-rewritten callees this batch reuses verbatim: their exact
//   register conventions were confirmed here by disassembling every call site (objdump -d -M
//   intel bin/halo.exe over 0x41c8f0..0x41d799) rather than trusting Ghidra's rendering, because
//   Ghidra's own "type propagation not settling" warning turned out to also drop or corrupt
//   several call arguments (see UNSURE notes below for the ones that mattered).
// register convention: all three parameters are genuine stack parameters (confirmed at
//   0x41c8f3/0x41c96e/0x41caee: [esp+0x38]/[esp+0x4c]/[esp+0x50] relative to the post-prologue
//   frame, i.e. the three words right after the return address).
//   // blam-cc: stack -> actor_index, target_prop_index, scratch
//
// This rewrite corrects several real errors an earlier pass made by transcribing Ghidra's
// pseudocode literally instead of checking the disassembly:
//   - the actor_danger_register_stationary_object call (gated by prop.unknown_136) was dropped
//     and replaced with a second, wrong call to actor_target_mark_engaged; disassembly at
//     0x41d46f..0x41d489 shows EAX <- scratch (the "reference" register argument of the real
//     function) and the second stack argument is prop.relationship_object_index, not
//     prop.unknown_20 as Ghidra's decompile claims.
//   - actor_danger_register_point's object_index argument is prop.object_index (0x18), not
//     prop.relationship_object_index; disassembly at 0x41d4cd confirms `mov edx,[ebp+0x18]`
//     immediately before the call, with EAX/EDX never pushed (they are the real function's
//     register arguments, invisible in Ghidra's zero-visible-arg rendering).
//   - both actor_target_mark_engaged calls (the "0x96-tick" and "vitality" gates) pass this
//     function's own target_prop_index in EAX (confirmed at 0x41d532/0x41d55a: `mov eax,
//     [esp+0x4c]`, the same stack slot used everywhere else in this function for
//     target_prop_index), not the literal 0 an earlier pass left them with.
//   - the vocalization gate compared prop.unknown_123's *new* value against itself; disassembly
//     at 0x41ca69/0x41ca79/0x41cbde shows the OLD (pre-reclassification) speed bucket is saved
//     to a stack temporary before the speed/closing-rate reclassification and compared against
//     the *new* bucket afterward (a rising-edge check), which needed a separate local.
//   - prop.unknown_130's gate tested `unit_obj->type == 0` (object_type biped), not a
//     nonexistent "unit->weapon_index"; disassembly shows `cmp WORD PTR [edi+0xb4],0` where edi
//     is the raw object pointer (object+0xb4 is object.type per types/objects.h), not any
//     unit_data field.
//   - the final unconditional field write matches Ghidra's `*(iVar21+100)=1`, i.e.
//     prop.combat_dirty (offset 0x64 = 100 decimal), not prop.unknown_b8.
//   - the owner-processing block's three fields (reachable/owner_not_fully_aware/owner_stalled)
//     are prop.unknown_12d/0x12b/0x12c respectively (Ghidra literally writes offsets 0x12d,
//     299=0x12b and 300=0x12c); an earlier pass invented a nonexistent "unknown_2ed_local"
//     field and additionally misrouted one of the three writes to prop.noticed_a (0xb9).
//   - actor_check_burst_length_exceeded (owner "burst" check) takes the *owner's* actor index in EAX, confirmed by
//     disassembling 0x4281b0 itself (`and eax,0xffff; imul eax,eax,0x724; ...`) and tracing EAX
//     back to `mov eax,[ebp+0x1c]` (prop.owner_actor_index) at the call site with no
//     intervening EAX write.
//
// UNSURE, still substantially (Ghidra's own "Type propagation algorithm not settling" marker
// covers this whole function): actor_evaluate_engagement_reachability (called twice, computing prop.unknown_38) is not
// yet rewritten anywhere in this module; its established extern (actor_check_weapon_pickup_
// reachable.c) models it as 4 stack arguments, but disassembling 0x42b270 itself shows it reads
// EAX and ECX (both 16-bit, compared against 0xffff) at entry before any stack argument -- the
// two call sites here load EAX from an Actor-tag word at actor_def+0x28 and ECX from
// prop.unknown_100 moments before the call, which the existing 4-stack-arg extern cannot
// express. Reused as-is (matching Ghidra's visible-argument order) rather than guessing a new,
// equally unverified convention; flagged for the hook-verification pass. actor_target_hearing_check's "gate"
// (EBX) and prop.unknown_130's source word (tag_data+0x2f4) are both raw offsets into a tag
// whose concrete type is not established (same caveat their sibling files already carry).
// The prop+0x34/0x36 conflict (types/ai.h declares one int32_t unknown_34, but this function
// writes 0x34 and 0x36 as two independent int16 halves) is reached through raw offsets with a
// note in place, as ai.h cannot be redefined.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT; declared locally because -I types shadows <math.h>
static float sqrtf_(float x) { return (float)sqrt((double)x); }

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern data_array *object_data;      // 0x008603b0
extern data_array *encounter_data;   // 0x008802c8
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, EAX -> actor_index
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index,
    uint8_t mark_engaged); // 0x41fa80, EAX, EBX, stack
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, EAX -> actor_index, EDI -> target_prop_index
extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX -> handle, ESI -> array
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50, stack args
extern float actor_compute_target_priority_weight(datum_index prop_index, datum_index actor_index); // 0x414590, EAX -> prop_index, ECX -> actor_index
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
    actor_vocalization_context *context); // 0x4142d0, EAX -> actor_index, stack -> line, variant, context
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
    void *target_ref, int16_t gate, real_point3d *listener_position); // 0x41c030, EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI -> listener_position, stack -> record, stance
extern uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index); // 0x41be10, EAX -> actor_index, ECX -> target_prop_index
extern uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index,
    datum_index object_index, uint8_t unknown_byte); // 0x41ea60, EAX -> reference, stack -> actor_index, object_index, unknown_byte
extern uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index,
    float radius, float distance, char accept_flag, uint8_t unknown_byte); // 0x41ec90, EAX -> actor_index, EDX -> source_object_index, stack -> radius, distance, accept_flag, unknown_byte
extern uint8_t actor_check_burst_length_exceeded(uint32_t actor_index); // 0x4281b0, EAX -> actor_index (disassembly-confirmed); not yet rewritten
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX -> actor_index; only AL is used here
// SIGNATURE-CONFLICT: src/ai/actor_rate_potential_target.c models 0x428370 as returning a
// bool+float x87 pair (types/ai.h bool_float_return); this call site reads only AL.
    // models this as returning a bool+float x87 pair, but only AL is used at this call site.

// UNSURE: see file header. Ghidra shows this call with 4 visible arguments; disassembly shows
// the callee also reads EAX/ECX (both 16-bit) at entry before those arguments are visible on
// the stack, so the true convention is likely a register+stack mix this declaration does not
// capture. Reused verbatim from actor_check_weapon_pickup_reachable.c ("not yet rewritten").
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying); // 0x42b270, AX, CX, ESI, EDI, stack

// UNSURE: actor_dispatch_look_handler_by_posture is also called (with an incompatible 3-argument shape) elsewhere in this
// module under the name actor_dispatch_look_handler_by_posture. Disassembling 0x41bb30 itself
// (`test bx,bx; je ...; cmp bx,1; jne <fallback>`) shows BX is a genuine register argument
// tested at entry, gating whether the function runs its main body or takes a fallback path;
// at all three call sites here it is loaded from prop.unknown_38 immediately before the call.
// The 6 stack arguments below were confirmed identical, slot for slot, across all three call
// sites via a full esp-offset trace of the disassembly.
extern int16_t actor_dispatch_look_handler_by_posture(int16_t kind, uint32_t actor_index, void *scratch, void *out_record,
    uint8_t rate_flag, uint8_t urgent_flag,
    uint16_t priority_class); // 0x41bb30, EBX -> kind, stack -> actor_index, scratch, out_record, rate_flag, urgent_flag, priority_class

// blam-cc: stack -> actor_index, target_prop_index, scratch
// Refreshes one prop's (target-data record's) derived aim/tracking classification: how fast
// the tracked object is moving and turning relative to the actor's aim (buckets at +0x123/
// +0x124), lead-distance and cone-visibility buckets (+0x120..+0x122), the "same squad/
// platoon" flag (+0x134), reachability/engagement grading (+0x38, +0x30..+0x36), and re-derives
// the prop's owner_actor_index straight from the tracked unit's actor/swarm_actor reference.
void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index, void *scratch)
{
    actor *self;
    prop *p;
    Actor *actor_def;
    encounter *enc;
    object *unit_obj;
    unit_data *unit;
    uint8_t team_gate; // Ghidra's bVar7
    int32_t tick;
    real_vector3d velocity;
    float speed;
    float closing_rate;
    uint8_t old_speed_bucket;
    uint8_t reachable;          // prop.unknown_12d
    uint8_t owner_not_fully_aware; // prop.unknown_12b
    uint8_t owner_stalled;      // prop.unknown_12c
    uint8_t engage_flag;

    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    if (!self->active) {
        return;
    }

    actor_def = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    enc = (self->encounter_index == (datum_index)k_datum_index_none)
              ? (encounter *)0
              : &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
    p = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    unit_obj = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    tick = game_time->game_time;

    team_gate = 0;
    if ((enc != (encounter *)0 && enc->unknown_40 != 0) || self->awareness_level == 1) {
        team_gate = 1;
    }

    p->unknown_133 = (uint8_t)(unit->flags >> 10) & 1; // UNSURE: bit not named in units.h unit_flags
    if (p->is_unit) {
        p->unknown_134 = (uint8_t)(unit->flags >> 11) & 1; // UNSURE: bit not named
        if (self->unknown_1d4 == 1) {
            if (p->owner_actor_index != (datum_index)k_datum_index_none) {
                uint32_t team_ref = *(uint32_t *)&self->unknown_1d6[2]; // actor+0x1d8
                if (team_ref != 0xffffffff) {
                    actor *owner = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                    if (((owner->encounter_index ^ team_ref) & 0xffff) == 0) {
                        uint32_t team_kind = team_ref >> 0x1e;
                        uint8_t match = 1;
                        encounter *owner_enc = &((encounter *)encounter_data->data)[owner->encounter_index & 0xffff];
                        // UNSURE: neither offset lands on a named encounter field; 0x3a is the
                        // salt half of first_pursuit (a datum_index) and 0x3c falls across
                        // unknown_3c/unknown_3d. Reached raw, matching the disassembly exactly.
                        if (team_kind == 1) {
                            match = (self->unknown_1d6[4] /* actor+0x1da */ ==
                                     (uint8_t)*(int16_t *)((uint8_t *)owner_enc + 0x3a));
                        } else if (team_kind == 2) {
                            match = (self->unknown_1d6[4] ==
                                     (uint8_t)*(int16_t *)&((struct encounter *)owner_enc)->unknown_3c);
                        } else if (team_kind != 0) {
                            match = 0;
                        }
                        if (match) {
                            p->unknown_134 = 1;
                        }
                    }
                }
            }
        } else if (self->unknown_1d4 == 2 && p->is_parented) {
            p->unknown_134 = 1;
        }
    }

    // Speed classification (+0x123): magnitude of the tracked unit's root velocity, bucketed.
    // The pre-update bucket is saved first: the vocalization gate below fires on a *rising
    // edge* (old bucket low, new bucket high), confirmed at 0x41ca69/0x41cbde.
    old_speed_bucket = p->unknown_123;
    object_get_root_object_velocities(p->object_index, &velocity, (real_vector3d *)0);
    speed = sqrtf_(velocity.i * velocity.i + velocity.j * velocity.j + velocity.k * velocity.k);
    if (speed < 0.0033333334f) {
        p->unknown_123 = 0;
    } else if (speed < 0.016666668f) {
        p->unknown_123 = 1;
    } else if (speed < 0.033333335f) {
        p->unknown_123 = 2;
    } else {
        p->unknown_123 = 3;
    }

    // Closing-rate classification (+0x124): velocity projected onto the prop's stored normal
    // (unknown_e0), relative to the caller-owned scratch point at scratch+0x2c/0x30/0x34.
    closing_rate = -((velocity.i - *(float *)((uint8_t *)scratch + 0x2c)) * p->unknown_e0.x +
                      (velocity.j - *(float *)((uint8_t *)scratch + 0x30)) * p->unknown_e0.y +
                      (velocity.k - *(float *)((uint8_t *)scratch + 0x34)) * p->unknown_e0.z);
    if (closing_rate < -0.033333335f) {
        p->unknown_124 = 0;
    } else if (closing_rate < -0.016666668f) {
        p->unknown_124 = 1;
    } else if (closing_rate < -0.0033333334f) {
        p->unknown_124 = 2;
    } else if (closing_rate < 0.0033333334f) {
        p->unknown_124 = 3;
    } else if (closing_rate < 0.016666668f) {
        p->unknown_124 = 4;
    } else if (closing_rate < 0.033333335f) {
        p->unknown_124 = 5;
    } else {
        p->unknown_124 = 6;
    }

    if (p->kind > 1 && p->kind < 4 && old_speed_bucket < 2 && p->unknown_123 > 1) {
        actor_vocalization_context ctx;
        ctx.kind = 1;
        ctx.handle = target_prop_index;
        // UNSURE: which of these two literals is "line" and which is "variant" is not
        // independently confirmed at this call site (both are compile-time constants here).
        actor_begin_vocalization(actor_index, /*line=*/2, /*variant=*/1, &ctx);
    }

    // Lead-distance bucket (+0x121), from raw distance.
    if (p->distance < 1.0f) {
        p->unknown_121 = 0;
    } else if (p->distance < 6.0f) {
        p->unknown_121 = 1;
    } else if (p->distance < 10.0f) {
        p->unknown_121 = 2;
    } else if (p->distance < 30.0f) {
        p->unknown_121 = 3;
    } else {
        p->unknown_121 = 4;
    }

    // Cone-visibility bucket (+0x122), from how far off-axis the unit's aiming vector is from
    // the prop's stored normal, scaled by distance.
    {
        real_vector3d aim = unit->aiming_vector;
        float cos_angle = -(aim.i * p->unknown_e0.x + aim.k * p->unknown_e0.z + aim.j * p->unknown_e0.y);
        float lateral;

        if (cos_angle < 0.0f) {
            lateral = 3.4028235e+38f;
        } else if (cos_angle < 1.0f) {
            lateral = sqrtf_(1.0f - cos_angle * cos_angle) * p->distance;
        } else {
            lateral = 0.0f;
        }

        if (0.9925f < cos_angle || lateral < 0.5f) {
            p->unknown_122 = 0;
        } else if (0.9063f < cos_angle || lateral < 1.5f) {
            p->unknown_122 = 1;
        } else if (cos_angle <= 0.5f) {
            p->unknown_122 = (cos_angle <= 0.0f) ? 4 : 3;
        } else {
            p->unknown_122 = 2;
        }
    }

    p->unknown_12f = (p->unknown_66 == 1);

    if (p->kind < 4 || 5 < p->kind) {
        engage_flag = 0;

        {
            int16_t kind_flag = (!p->is_parented || !p->is_unit) ? 0 : 2;
            p->unknown_38 = (int16_t)actor_evaluate_engagement_reachability(
                *(int16_t *)((uint8_t *)scratch + 0x28), p->cluster_index, (real_point3d *)&p->unknown_104,
                (real_point3d *)scratch, kind_flag, 0, p->relationship_object_index,
                self->active_unit_index != (datum_index)k_datum_index_none); // 0x41cee5: AX = block +0x28, EDI = the block
        }
        p->unknown_120 = 2;

        if (unit_obj->type == _object_type_biped) {
            void *own_tag_data = tag_instances[unit_obj->definition_tag & 0xffff].data; // UNSURE: raw tag offset
            p->unknown_130 = (uint8_t)((*(uint32_t *)((uint8_t *)own_tag_data + 0x2f4)) >> 2) & 1;
        } else {
            p->unknown_130 = 0;
        }
        p->unknown_131 = (0.5f < unit->unknown_37c);
        p->unknown_132 = (uint8_t)(unit->flags >> 0x13) & 1;

        {
            uint8_t frozen = (unit_obj->vitality_flags & _object_health_frozen_bit) != 0;
            uint8_t stun_pending = frozen && unit->unknown_420 != 0;

            p->unknown_129 = (frozen && p->is_vault == 0) ? 1 : 0;
            p->is_vault = frozen;
            p->unknown_128 = stun_pending;

            if (p->unknown_129 != 0 && !p->is_unit && self->awareness_level < 3) {
                engage_flag = 1;
            }
            if (frozen) {
                p->unknown_6a = 0;
            }
        }

        {
            uint32_t new_owner = unit->swarm_actor_index;
            uint8_t owner_is_none = (new_owner == (uint32_t)k_datum_index_none);
            if (owner_is_none) {
                new_owner = unit->actor_index;
            }
            if (new_owner != (uint32_t)p->owner_actor_index) {
                p->has_parent = !owner_is_none;
                p->owner_actor_index = new_owner;
                if (p->pair_index != (datum_index)k_datum_index_none) {
                    prop *paired = &((prop *)prop_data->data)[p->pair_index & 0xffff];
                    paired->owner_actor_index = new_owner;
                    paired->has_parent = p->has_parent;
                }
            }

            if (new_owner == (uint32_t)k_datum_index_none) {
                reachable = (p->is_vault == 0);
                owner_not_fully_aware = 0;
                owner_stalled = 0;
            } else {
                actor *owner = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                owner_not_fully_aware = (owner->awareness_level < 3);
                owner_stalled = (owner->awareness_level == 3 && owner->alert_floor < owner->alert_level);
                reachable = actor_check_burst_length_exceeded(p->owner_actor_index);
                if (owner_stalled && p->unknown_12c == 0 && !p->is_unit && self->awareness_level < 3) {
                    engage_flag = 1;
                }
            }
            p->unknown_12d = reachable;
            p->unknown_12b = owner_not_fully_aware;
            p->unknown_12c = owner_stalled;
        }

        if (engage_flag) {
            int16_t result = 0;
            if (!team_gate) {
                uint8_t rate_flag = p->unknown_132 ? 2 : p->unknown_120;
                uint16_t priority_class = actor_target_get_priority_class(actor_index, target_prop_index);
                result = actor_dispatch_look_handler_by_posture(p->unknown_38, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                       rate_flag, 1, priority_class);
                if (result > 1) goto after_engage;
            }
            p->unknown_30 = result;
            p->unknown_32 = result;
            p->unknown_38 = 0;
        }
after_engage:
        if (!p->unknown_133) {
            uint8_t did_track = 0;
            if (!p->unknown_131) {
                if (team_gate) {
                    p->unknown_32 = 0;
                    p->unknown_12a = 0;
                    did_track = 1;
                }
            } else if (p->is_unit || (p->is_parented && 4.0f < p->distance)) {
                p->unknown_32 = 0;
                p->unknown_12a = 0;
                did_track = 1;
            }
            if (!did_track) {
                uint8_t use_urgent = 1;
                if (self->unknown_15e == 4 || self->type == 0xf) {
                    use_urgent = 0;
                } else if (!p->is_unit) {
                    use_urgent = 0;
                    if (self->awareness_level < 3 && (p->is_vault || p->unknown_12c != 0)) {
                        use_urgent = 1;
                    }
                } else if (2 <= p->kind && p->kind < 4) {
                    use_urgent = 0;
                }
                {
                    uint8_t rate_flag = p->unknown_132 ? 2 : p->unknown_120;
                    uint16_t priority_class = actor_target_get_priority_class(actor_index, target_prop_index);
                    int16_t result = actor_dispatch_look_handler_by_posture(p->unknown_38, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                                   rate_flag, use_urgent, priority_class);
                    p->unknown_12a = (p->unknown_32 == 0 && result > 0);
                    p->unknown_32 = result;
                    if (result != 0) {
                        p->unknown_90 = ((struct prop *)p)->unknown_104;
                        p->unknown_94 = ((struct prop *)p)->unknown_108;
                        p->unknown_98 = ((struct prop *)p)->unknown_10c;
                        p->unknown_8c = tick;
                    }
                }
            }
            // prop+0x34/0x36: types/ai.h declares one int32_t unknown_34, but this function
            // writes independent int16 halves at +0x34 and +0x36 (see file header).
            if (enc == (encounter *)0 || enc->unknown_41 == 0) {
                if (p->unknown_66 == 1 || p->unknown_66 == 2) {
                    *(int16_t *)&((struct prop *)p)->unknown_34 = 3;
                } else {
                    *(int16_t *)&((struct prop *)p)->unknown_34 =
                        actor_target_hearing_check((uint8_t *)p + 0xfc, p->unknown_38, actor_index,
                                                    scratch, /*gate=UNSURE-tag-word*/ 0, &p->last_known_position);
                }
            } else {
                *(int16_t *)&((struct prop *)p)->unknown_34 = 0;
            }
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            if (p->unknown_66 == 0) {
                *(int16_t *)((uint8_t *)p + 0x36) = 3;
            }
            if (p->unknown_132 != 0 && p->unknown_122 < 3 && p->unknown_121 < 3 &&
                (p->unknown_38 == 0 || p->unknown_38 == 1)) {
                int16_t v = *(int16_t *)((uint8_t *)p + 0x36);
                if (v < 2) v = 1;
                *(int16_t *)((uint8_t *)p + 0x36) = v;
            }
            {
                int16_t a = *(int16_t *)&((struct prop *)p)->unknown_34;
                int16_t b = *(int16_t *)((uint8_t *)p + 0x36);
                int16_t best = (a <= b) ? b : a;
                int16_t chosen = p->unknown_32;
                if (chosen <= best) {
                    chosen = best;
                }
                p->unknown_30 = chosen;
                if (chosen == 1 && 2 <= p->kind && p->kind < 4) {
                    p->unknown_30 = 2;
                }
            }
        } else {
            p->unknown_30 = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            *(int16_t *)&((struct prop *)p)->unknown_34 = 0;
            p->unknown_32 = 0;
        }

        if (p->unknown_30 != 0) {
            p->last_known_position = p->ground_position;
            p->unknown_7c = tick;
        }

        if (2 <= p->kind && p->kind < 4 &&
            (1 < p->unknown_32 ||
             (p->noticed_b != 0 && p->unknown_b4 != -1 &&
              ((actor *)datum_get(p->unknown_b4, actor_data)) != (actor *)0 &&
              9 < ((actor *)datum_get(p->unknown_b4, actor_data))->target_combat_status &&
              ((actor *)datum_get(p->unknown_b4, actor_data))->target_unit_index != (datum_index)k_datum_index_none &&
              ((actor *)datum_get(p->unknown_b4, actor_data))->unknown_454 != 0 &&
              (((prop *)prop_data->data)[((actor *)datum_get(p->unknown_b4, actor_data))->target_unit_index & 0xffff]).object_index == p->object_index))) {
            p->noticed_b = 1;
            p->unknown_b0 = 0;
        }
    } else {
        int16_t kind_flag = (!p->is_parented || !p->is_unit) ? 0 : 2;
        p->unknown_38 = (int16_t)actor_evaluate_engagement_reachability(
            *(int16_t *)((uint8_t *)scratch + 0x28), p->cluster_index, (real_point3d *)&p->unknown_104,
            (real_point3d *)scratch, kind_flag, 0, p->relationship_object_index,
            self->active_unit_index != (datum_index)k_datum_index_none); // 0x41ce26
        if (p->unknown_133 || team_gate) {
            p->unknown_30 = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            *(int16_t *)&((struct prop *)p)->unknown_34 = 0;
            p->unknown_32 = 0;
        } else {
            int16_t result = actor_dispatch_look_handler_by_posture(p->unknown_38, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                           p->unknown_120, 1, 2);
            p->unknown_32 = result;
            *(int16_t *)&((struct prop *)p)->unknown_34 = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            p->unknown_30 = result;
        }
    }

    if (p->unknown_136 != 0) {
        actor_danger_register_stationary_object((const float *)scratch, actor_index,
                                                 p->relationship_object_index, 1 < p->unknown_30);
    }

    // UNSURE: the byte at actor_def+0x2a3 is read here as a value compared against 0x1e; it
    // does not land cleanly on a named field of types/tags.h's Actor (the padding blocks
    // around ActorUnreachableDangerTrigger_t are not precise enough to be sure), so it is kept
    // as a raw offset rather than guessed at.
    if (0.0f < p->unknown_20 &&
        (p->is_vault || *((uint8_t *)actor_def + 0x2a3) == 0x1e)) {
        actor_danger_register_point(actor_index, p->object_index, p->unknown_20, p->distance,
                                     p->is_unit, 1 < p->unknown_30);
    }

    if (p->is_unit && 2 <= p->kind && p->kind < 4 &&
        ((actor_has_unshielded_threat_weapon(actor_index) != 0 && p->distance < self->vitality_wait_time) ||
         ((actor_def->flags & 0x08000000u) != 0 && p->distance < actor_def->melee_fudge_factor))) {
        // bit 27 = "suicidal_melee_attack" per ActorFlags' documented bit order
        actor_target_mark_engaged(target_prop_index, actor_index, 0); // FIXED: EBX = the actor (EDI)
    }

    if (p->unknown_a0 != -1 && p->unknown_a0 + 0x96 < tick) {
        actor_target_mark_engaged(target_prop_index, actor_index, 0); // FIXED: EBX = the actor (EDI)
    }

    if (p->unknown_126 != 0) {
        float dist_sq = p->distance * p->distance;
        actor *owner = (p->owner_actor_index == (datum_index)k_datum_index_none)
                           ? (actor *)0
                           : &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
        uint8_t drop = 0;

        if (p->is_parented) {
            drop = 0;
        } else if (owner != (actor *)0 && !(owner->active != 0 && owner->keep_unit_alive == 0)) {
            drop = 1;
        } else if (p->unknown_63 != 0) {
            drop = 0;
        } else if (1600.0f < dist_sq) {
            drop = 1;
        } else if (!p->is_vault) {
            if (!p->is_unit) {
                if (225.0f <= dist_sq) drop = 1;
            }
        } else {
            uint32_t enc_idx = self->encounter_index;
            uint8_t ok = 1;
            if (enc_idx != (uint32_t)k_datum_index_none) {
                encounter *e = &((encounter *)encounter_data->data)[enc_idx & 0xffff];
                int32_t gate = (e->unknown_58 <= self->unknown_3a0) ? self->unknown_3a0 : e->unknown_58;
                object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                unit_data *u2 = (unit_data *)((uint8_t *)ohdr->data + k_unit_data_offset);
                int32_t last_seen = u2->unknown_41c;
                ok = (gate == -1 || (last_seen != -1 && gate <= last_seen));
                if (!ok) {
                    drop = 1;
                } else if (e->unknown_45 == 0 && e->unknown_44 == 0 && e->unknown_42 == 0) {
                    if (225.0f <= dist_sq) drop = 1;
                }
            } else if (p->unknown_20 <= 0.0f) {
                if (!p->is_unit || p->unknown_76 < 0x97) {
                    int16_t grade = actor_get_current_mode_combat_grade(actor_index);
                    if (grade < 2) {
                        float threshold = 16.0f;
                        if (!p->is_unit && self->awareness_level < 3) threshold = 64.0f;
                        if (dist_sq >= threshold) drop = 1;
                    } else {
                        drop = 1;
                    }
                } else {
                    drop = 1;
                }
            }
        }

        if (drop) {
            p->unknown_6a = 0;
        }
    }
    p->unknown_126 = 0;

    p->engaged = actor_target_update_active_flag(actor_index, target_prop_index);
    p->desirability = actor_rate_potential_target(actor_index, target_prop_index);
    p->unknown_54 = actor_compute_target_priority_weight(target_prop_index, actor_index);
    p->combat_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x41c8f0) -- see out/phase4/ai_functions.md pack and
python tools/pack.py 0x41c8f0 for the full listing this rewrite is built from (Ghidra marks
the whole function "Type propagation algorithm not settling"). This rewrite was built by
cross-checking that decompile line by line against `objdump -d -M intel bin/halo.exe` over
0x41c8f0..0x41d799 (see scratchpad/disasm_41c8f0.txt in the working tree at rewrite time),
which is what caught the callee/argument/field errors described in the file header; the
decompile overall control flow and most memory-offset arithmetic checked out and is
preserved here as-is.

/* WARNING: Type propagation algorithm not settling */

void FUN_0041c8f0(uint param_1,uint param_2,int param_3)

{
  short sVar1;
  uint *puVar2;
  uint *puVar3;
  float fVar4;
  short sVar5;
  float fVar6;
  bool bVar7;
  uint3 uVar8;
  byte bVar9;
  char cVar10;
  undefined1 uVar11;
  undefined2 uVar12;
  short sVar13;
  short sVar14;
  int iVar15;
  int iVar16;
  undefined4 uVar17;
  int iVar18;
  int iVar19;
  uint uVar20;
  int iVar21;
  int iVar22;
  bool bVar23;
  bool bVar24;
  float10 fVar25;
  bool local_31;
  float local_30;
  undefined4 local_24;
  float local_10;
  float local_c;
  float local_8;

  iVar18 = DAT_00880360;
  iVar15 = (param_1 & 0xffff) * 0x724;
  iVar22 = *(int *)(DAT_00880360 + 0x34) + iVar15;
  if (*(char *)(iVar22 + 8) == '\0') {
    return;
  }
  puVar2 = *(uint **)((*(uint *)(iVar22 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(uint *)(iVar22 + 0x34) == 0xffffffff) {
    iVar16 = 0;
  }
  else {
    iVar16 = (*(uint *)(iVar22 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  }
  iVar21 = (param_2 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar21 + 0x18) & 0xffff) * 0xc)
  ;
  iVar19 = *(int *)(DAT_006f1d6c + 0xc);
  if (((iVar16 != 0) && (*(char *)(iVar16 + 0x40) != '\0')) ||
     (bVar7 = false, *(short *)(iVar22 + 0x6a) == 1)) {
    bVar7 = true;
  }
  *(byte *)(iVar21 + 0x133) = (byte)(puVar3[0x81] >> 10) & 1;
  if (*(char *)(iVar21 + 0x60) != '\0') {
    *(byte *)(iVar21 + 0x134) = (byte)(puVar3[0x81] >> 0xb) & 1;
    if (*(short *)(iVar22 + 0x1d4) == 1) {
      if (((*(uint *)(iVar21 + 0x1c) != 0xffffffff) &&
          (uVar20 = *(uint *)(iVar22 + 0x1d8), uVar20 != 0xffffffff)) &&
         (iVar18 = (*(uint *)(iVar21 + 0x1c) & 0xffff) * 0x724 + *(int *)(iVar18 + 0x34),
         ((*(uint *)(iVar18 + 0x34) ^ uVar20) & 0xffff) == 0)) {
        uVar20 = uVar20 >> 0x1e;
        if (uVar20 != 0) {
          if (uVar20 == 1) {
            bVar23 = (ushort)*(byte *)(iVar22 + 0x1da) == *(ushort *)(iVar18 + 0x3a);
          }
          else {
            if (uVar20 != 2) goto LAB_0041ca69;
            bVar23 = (ushort)*(byte *)(iVar22 + 0x1da) == *(ushort *)(iVar18 + 0x3c);
          }
          if (!bVar23) goto LAB_0041ca69;
        }
LAB_0041ca62:
        *(undefined1 *)(iVar21 + 0x134) = 1;
      }
    }
    else if ((*(short *)(iVar22 + 0x1d4) == 2) && (*(char *)(iVar21 + 0x12e) != '\0'))
    goto LAB_0041ca62;
  }
LAB_0041ca69:
  cVar10 = *(char *)(iVar21 + 0x123);
  local_24 = CONCAT22(local_24._2_2_,(short)cVar10);
  FUN_004f6aa0();
  fVar4 = SQRT(local_8 * local_8 + local_10 * local_10 + local_c * local_c);
  if (0.0033333334 <= fVar4) {
    if (0.016666668 <= fVar4) {
      if (0.033333335 <= fVar4) {
        *(undefined1 *)(iVar21 + 0x123) = 3;
      }
      else {
        *(undefined1 *)(iVar21 + 0x123) = 2;
      }
    }
    else {
      *(undefined1 *)(iVar21 + 0x123) = 1;
    }
  }
  else {
    *(undefined1 *)(iVar21 + 0x123) = 0;
  }
  fVar4 = -((local_10 - *(float *)(param_3 + 0x2c)) * *(float *)(iVar21 + 0xe0) +
           (local_c - *(float *)(param_3 + 0x30)) * *(float *)(iVar21 + 0xe4) +
           (local_8 - *(float *)(param_3 + 0x34)) * *(float *)(iVar21 + 0xe8));
  if (-0.033333335 <= fVar4) {
    if (-0.016666668 <= fVar4) {
      if (-0.0033333334 <= fVar4) {
        if (0.0033333334 <= fVar4) {
          if (0.016666668 <= fVar4) {
            if (0.033333335 <= fVar4) {
              *(undefined1 *)(iVar21 + 0x124) = 6;
            }
            else {
              *(undefined1 *)(iVar21 + 0x124) = 5;
            }
          }
          else {
            *(undefined1 *)(iVar21 + 0x124) = 4;
          }
        }
        else {
          *(undefined1 *)(iVar21 + 0x124) = 3;
        }
      }
      else {
        *(undefined1 *)(iVar21 + 0x124) = 2;
      }
    }
    else {
      *(undefined1 *)(iVar21 + 0x124) = 1;
    }
  }
  else {
    *(undefined1 *)(iVar21 + 0x124) = 0;
  }
  if ((((1 < *(short *)(iVar21 + 0x24)) && (*(short *)(iVar21 + 0x24) < 4)) && (cVar10 < 2)) &&
     ('\x01' < *(char *)(iVar21 + 0x123))) {
    local_10 = (float)CONCAT22(local_10._2_2_,1);
    local_c = (float)param_2;
    FUN_004142d0(2,1,&local_10);
  }
  if (1.0 <= *(float *)(iVar21 + 0x11c)) {
    if (6.0 <= *(float *)(iVar21 + 0x11c)) {
      if (10.0 <= *(float *)(iVar21 + 0x11c)) {
        if (30.0 <= *(float *)(iVar21 + 0x11c)) {
          *(undefined1 *)(iVar21 + 0x121) = 4;
        }
        else {
          *(undefined1 *)(iVar21 + 0x121) = 3;
        }
      }
      else {
        *(undefined1 *)(iVar21 + 0x121) = 2;
      }
    }
    else {
      *(undefined1 *)(iVar21 + 0x121) = 1;
    }
  }
  else {
    *(undefined1 *)(iVar21 + 0x121) = 0;
  }
  iVar18 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar21 + 0x18) & 0xffff) * 0xc);
  local_10 = *(float *)(iVar18 + 0x23c);
  local_c = *(float *)(iVar18 + 0x240);
  local_8 = *(float *)(iVar18 + 0x244);
  local_30 = -(local_10 * *(float *)(iVar21 + 0xe0) +
              local_8 * *(float *)(iVar21 + 0xe8) + local_c * *(float *)(iVar21 + 0xe4));
  if (0.0 <= local_30) {
    if (local_30 < 1.0) {
      fVar4 = SQRT(1.0 - local_30 * local_30) * *(float *)(iVar21 + 0x11c);
    }
    else {
      fVar4 = 0.0;
    }
  }
  else {
    fVar4 = 3.4028235e+38;
  }
  if ((0.9925 < local_30) || (fVar4 < 0.5)) {
    *(undefined1 *)(iVar21 + 0x122) = 0;
  }
  else if ((0.9063 < local_30) || (fVar4 < 1.5)) {
    *(undefined1 *)(iVar21 + 0x122) = 1;
  }
  else if (local_30 <= 0.5) {
    if (local_30 <= 0.0) {
      *(undefined1 *)(iVar21 + 0x122) = 4;
    }
    else {
      *(undefined1 *)(iVar21 + 0x122) = 3;
    }
  }
  else {
    *(undefined1 *)(iVar21 + 0x122) = 2;
  }
  *(bool *)(iVar21 + 0x12f) = *(short *)(iVar21 + 0x66) == 1;
  if ((*(short *)(iVar21 + 0x24) < 4) || (5 < *(short *)(iVar21 + 0x24))) {
    bVar23 = false;
    if ((*(char *)(iVar21 + 0x12e) == '\0') || (*(char *)(iVar21 + 0x60) == '\0')) {
      uVar17 = 0;
    }
    else {
      uVar17 = 2;
    }
    uVar12 = FUN_0042b270(uVar17,0,*(undefined4 *)(iVar21 + 0x110),
                          CONCAT31((int3)((uint)*(int *)(iVar22 + 0x158) >> 8),
                                   *(int *)(iVar22 + 0x158) != -1));
    *(undefined2 *)(iVar21 + 0x38) = uVar12;
    *(undefined1 *)(iVar21 + 0x120) = 2;
    if ((short)puVar3[0x2d] == 0) {
      *(byte *)(iVar21 + 0x130) =
           (byte)(*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) >> 2)
           & 1;
    }
    else {
      *(undefined1 *)(iVar21 + 0x130) = 0;
    }
    *(bool *)(iVar21 + 0x131) = 0.5 < (float)puVar3[0xdf];
    *(byte *)(iVar21 + 0x132) = (byte)(puVar3[0x81] >> 0x13) & 1;
    bVar9 = *(byte *)((int)puVar3 + 0x106) >> 2 & 1;
    if ((bVar9 == 0) || ((short)puVar3[0x108] != 0)) {
      uVar11 = 0;
    }
    else {
      uVar11 = 1;
    }
    if ((bVar9 == 0) || (*(char *)(iVar21 + 0x127) != '\0')) {
      cVar10 = '\0';
    }
    else {
      cVar10 = '\x01';
    }
    *(char *)(iVar21 + 0x129) = cVar10;
    *(byte *)(iVar21 + 0x127) = bVar9;
    *(undefined1 *)(iVar21 + 0x128) = uVar11;
    if (((cVar10 != '\0') && (*(char *)(iVar21 + 0x60) == '\0')) && (*(short *)(iVar22 + 0x6a) < 3))
    {
      bVar23 = true;
    }
    if (bVar9 != 0) {
      *(undefined2 *)(iVar21 + 0x6a) = 0;
    }
    uVar20 = puVar3[0x7e];
    bVar24 = uVar20 == 0xffffffff;
    if (bVar24) {
      uVar20 = puVar3[0x7d];
    }
    if (uVar20 != *(uint *)(iVar21 + 0x1c)) {
      *(bool *)(iVar21 + 0x14) = !bVar24;
      *(uint *)(iVar21 + 0x1c) = uVar20;
      if (*(uint *)(iVar21 + 0xc) != 0xffffffff) {
        iVar18 = (*(uint *)(iVar21 + 0xc) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
        *(uint *)(iVar18 + 0x1c) = uVar20;
        *(undefined1 *)(iVar18 + 0x14) = *(undefined1 *)(iVar21 + 0x14);
      }
    }
    if (uVar20 == 0xffffffff) {
      cVar10 = '\0';
      uVar11 = *(char *)(iVar21 + 0x127) == '\0';
      local_31 = false;
    }
    else {
      iVar18 = (*(uint *)(iVar21 + 0x1c) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      local_31 = *(short *)(iVar18 + 0x6a) < 3;
      if ((*(short *)(iVar18 + 0x6a) == 3) &&
         (*(short *)(iVar18 + 0x72) < *(short *)(iVar18 + 0x6e))) {
        cVar10 = '\x01';
      }
      else {
        cVar10 = '\0';
      }
      uVar11 = FUN_004281b0();
      if (((cVar10 != '\0') && (*(char *)(iVar21 + 300) == '\0')) &&
         ((*(char *)(iVar21 + 0x60) == '\0' && (*(short *)(iVar22 + 0x6a) < 3)))) {
        bVar23 = true;
      }
    }
    *(undefined1 *)(iVar21 + 0x12d) = uVar11;
    *(bool *)(iVar21 + 299) = local_31;
    *(char *)(iVar21 + 300) = cVar10;
    if (bVar23) {
      sVar14 = 0;
      if (!bVar7) {
        local_30._1_3_ = (uint3)((uint)local_30 >> 8);
        if (*(char *)(iVar21 + 0x132) == '\0') {
          local_30 = (float)CONCAT31(local_30._1_3_,*(undefined1 *)(iVar21 + 0x120));
        }
        else {
          local_30 = (float)CONCAT31(local_30._1_3_,2);
        }
        uVar17 = FUN_0041be10();
        sVar14 = FUN_0041bb30(param_1,param_3,iVar21 + 0x104,local_30,1,uVar17);
        if (1 < sVar14) goto LAB_0041d12a;
      }
      *(short *)(iVar21 + 0x30) = sVar14;
      *(short *)(iVar21 + 0x32) = sVar14;
      *(undefined2 *)(iVar21 + 0x24) = 0;
    }
LAB_0041d12a:
    if (*(char *)(iVar21 + 0x133) == '\0') {
      if (*(char *)(iVar21 + 0x131) == '\0') {
        if (!bVar7) goto LAB_0041d17b;
LAB_0041d1cb:
        *(undefined2 *)(iVar21 + 0x32) = 0;
        *(undefined1 *)(iVar21 + 0x12a) = 0;
      }
      else {
        if ((*(char *)(iVar21 + 0x60) != '\0') ||
           ((*(char *)(iVar21 + 0x12e) != '\0' && (4.0 < *(float *)(iVar21 + 0x11c)))))
        goto LAB_0041d1cb;
LAB_0041d17b:
        uVar8 = local_30._1_3_;
        local_30 = (float)CONCAT31(local_30._1_3_,1);
        if ((*(short *)(iVar22 + 0x15e) == 4) || (*(short *)(iVar22 + 4) == 0xf)) {
LAB_0041d1ef:
          local_30 = (float)((uint)local_30._1_3_ << 8);
        }
        else if (*(char *)(iVar21 + 0x60) == '\0') {
          local_30 = (float)((uint)local_30._1_3_ << 8);
          if ((*(short *)(iVar22 + 0x6a) < 3) &&
             ((*(char *)(iVar21 + 0x127) != '\0' || (*(char *)(iVar21 + 300) != '\0')))) {
            local_30 = (float)CONCAT31(uVar8,1);
          }
        }
        else if ((1 < *(short *)(iVar21 + 0x24)) && (*(short *)(iVar21 + 0x24) < 4))
        goto LAB_0041d1ef;
        if (*(char *)(iVar21 + 0x132) == '\0') {
          local_24 = CONCAT31(local_24._1_3_,*(undefined1 *)(iVar21 + 0x120));
        }
        else {
          local_24 = CONCAT31(local_24._1_3_,2);
        }
        uVar17 = FUN_0041be10();
        sVar14 = FUN_0041bb30(param_1,param_3,(undefined4 *)(iVar21 + 0x104),local_24,local_30,
                              uVar17);
        if ((*(short *)(iVar21 + 0x32) == 0) && (0 < sVar14)) {
          uVar11 = 1;
        }
        else {
          uVar11 = 0;
        }
        *(undefined1 *)(iVar21 + 0x12a) = uVar11;
        *(short *)(iVar21 + 0x32) = sVar14;
        if (sVar14 != 0) {
          *(undefined4 *)(iVar21 + 0x90) = *(undefined4 *)(iVar21 + 0x104);
          *(undefined4 *)(iVar21 + 0x94) = *(undefined4 *)(iVar21 + 0x108);
          *(undefined4 *)(iVar21 + 0x98) = *(undefined4 *)(iVar21 + 0x10c);
          *(int *)(iVar21 + 0x8c) = iVar19;
        }
      }
      if ((iVar16 == 0) || (*(char *)(iVar16 + 0x41) == '\0')) {
        if ((*(short *)(iVar21 + 0x66) == 1) || (*(short *)(iVar21 + 0x66) == 2)) {
          *(undefined2 *)(iVar21 + 0x34) = 3;
        }
        else {
          uVar12 = FUN_0041c030(iVar21 + 0xfc,*(undefined2 *)(iVar21 + 0x38));
          *(undefined2 *)(iVar21 + 0x34) = uVar12;
        }
      }
      else {
        *(undefined2 *)(iVar21 + 0x34) = 0;
      }
      *(undefined2 *)(iVar21 + 0x36) = 0;
      if (*(short *)(iVar21 + 0x66) == 0) {
        *(undefined2 *)(iVar21 + 0x36) = 3;
      }
      if ((((*(char *)(iVar21 + 0x132) != '\0') && (*(char *)(iVar21 + 0x122) < '\x03')) &&
          (*(char *)(iVar21 + 0x121) < '\x03')) &&
         ((*(short *)(iVar21 + 0x38) == 0 || (*(short *)(iVar21 + 0x38) == 1)))) {
        sVar14 = *(short *)(iVar21 + 0x36);
        if (sVar14 < 2) {
          sVar14 = 1;
        }
        *(short *)(iVar21 + 0x36) = sVar14;
      }
      sVar14 = *(short *)(iVar21 + 0x34);
      sVar1 = *(short *)(iVar21 + 0x36);
      sVar5 = sVar14;
      if (sVar14 <= sVar1) {
        sVar5 = sVar1;
      }
      sVar13 = *(short *)(iVar21 + 0x32);
      if ((*(short *)(iVar21 + 0x32) <= sVar5) && (sVar13 = sVar14, sVar14 <= sVar1)) {
        sVar13 = sVar1;
      }
      *(short *)(iVar21 + 0x30) = sVar13;
      if (((sVar13 == 1) && (1 < *(short *)(iVar21 + 0x24))) && (*(short *)(iVar21 + 0x24) < 4)) {
        *(undefined2 *)(iVar21 + 0x30) = 2;
      }
    }
    else {
      *(undefined2 *)(iVar21 + 0x30) = 0;
      *(undefined2 *)(iVar21 + 0x36) = 0;
      *(undefined2 *)(iVar21 + 0x34) = 0;
      *(undefined2 *)(iVar21 + 0x32) = 0;
    }
    if (*(short *)(iVar21 + 0x30) != 0) {
      *(undefined4 *)(iVar21 + 0x80) = *(undefined4 *)(iVar21 + 0xbc);
      *(undefined4 *)(iVar21 + 0x84) = *(undefined4 *)(iVar21 + 0xc0);
      *(undefined4 *)(iVar21 + 0x88) = *(undefined4 *)(iVar21 + 0xc4);
      *(int *)(iVar21 + 0x7c) = iVar19;
    }
    if (((1 < *(short *)(iVar21 + 0x24)) && (*(short *)(iVar21 + 0x24) < 4)) &&
       ((1 < *(short *)(iVar21 + 0x32) ||
        (((*(char *)(iVar21 + 0xb8) != '\0' && (*(int *)(iVar21 + 0xb4) != -1)) &&
         ((iVar18 = datum_get(), iVar18 != 0 &&
          ((((9 < *(short *)(iVar18 + 0x268) && (*(uint *)(iVar18 + 0x270) != 0xffffffff)) &&
            (*(char *)(iVar18 + 0x454) != '\0')) &&
           (*(int *)((*(uint *)(iVar18 + 0x270) & 0xffff) * 0x138 + 0x18 +
                    *(int *)(DAT_008802c0 + 0x34)) == *(int *)(iVar21 + 0x18))))))))))) {
      *(undefined1 *)(iVar21 + 0xb8) = 1;
      *(undefined2 *)(iVar21 + 0xb0) = 0;
    }
  }
  else {
    if ((*(char *)(iVar21 + 0x12e) == '\0') || (*(char *)(iVar21 + 0x60) == '\0')) {
      uVar17 = 0;
    }
    else {
      uVar17 = 2;
    }
    uVar12 = FUN_0042b270(uVar17,0,*(undefined4 *)(iVar21 + 0x110),
                          CONCAT31((int3)((uint)*(int *)(iVar22 + 0x158) >> 8),
                                   *(int *)(iVar22 + 0x158) != -1));
    *(undefined2 *)(iVar21 + 0x38) = uVar12;
    if ((*(char *)(iVar21 + 0x133) != '\0') || (bVar7)) {
      *(undefined2 *)(iVar21 + 0x30) = 0;
      *(undefined2 *)(iVar21 + 0x36) = 0;
      *(undefined2 *)(iVar21 + 0x34) = 0;
      *(undefined2 *)(iVar21 + 0x32) = 0;
    }
    else {
      uVar12 = FUN_0041bb30(param_1,param_3,iVar21 + 0x104,*(undefined1 *)(iVar21 + 0x120),1,2);
      *(undefined2 *)(iVar21 + 0x32) = uVar12;
      *(undefined2 *)(iVar21 + 0x34) = 0;
      *(undefined2 *)(iVar21 + 0x36) = 0;
      *(undefined2 *)(iVar21 + 0x30) = uVar12;
    }
  }
  if (*(char *)(iVar21 + 0x136) != '\0') {
    FUN_0041ea60(param_1,*(undefined4 *)(iVar21 + 0x20),1 < *(short *)(iVar21 + 0x30));
  }
  if ((0.0 < *(float *)(iVar21 + 0x20)) &&
     ((*(char *)(iVar21 + 0x127) != '\0' || (*(char *)((int)puVar2 + 0x2a3) == '\x1e')))) {
    FUN_0041ec90(*(undefined4 *)(iVar21 + 0x20),*(undefined4 *)(iVar21 + 0x11c),
                 *(undefined1 *)(iVar21 + 0x60),1 < *(short *)(iVar21 + 0x30));
  }
  if ((((*(char *)(iVar21 + 0x60) != '\0') && (1 < *(short *)(iVar21 + 0x24))) &&
      (*(short *)(iVar21 + 0x24) < 4)) &&
     (((cVar10 = FUN_00428370(), cVar10 != '\0' &&
       (*(float *)(iVar21 + 0x11c) < *(float *)(iVar22 + 0x608))) ||
      (((*puVar2 & 0x8000000) != 0 && (*(float *)(iVar21 + 0x11c) < (float)puVar2[0xdf])))))) {
    FUN_0041fa80(0);
  }
  if ((*(int *)(iVar21 + 0xa0) != -1) && (*(int *)(iVar21 + 0xa0) + 0x96 < iVar19)) {
    FUN_0041fa80(0);
  }
  if (*(char *)(iVar21 + 0x126) == '\0') goto LAB_0041d75f;
  fVar4 = *(float *)(iVar21 + 0x11c) * *(float *)(iVar21 + 0x11c);
  iVar18 = *(int *)(DAT_00880360 + 0x34);
  if (*(uint *)(iVar21 + 0x1c) == 0xffffffff) {
    iVar22 = 0;
  }
  else {
    iVar22 = (*(uint *)(iVar21 + 0x1c) & 0xffff) * 0x724 + iVar18;
  }
  if (*(char *)(iVar21 + 0x12e) == '\0') {
    if ((iVar22 == 0) || ((*(char *)(iVar22 + 8) != '\0' && (*(char *)(iVar22 + 0x13) == '\0')))) {
      if (*(char *)(iVar21 + 99) == '\0') {
        if (1600.0 < fVar4) goto LAB_0041d752;
        if (*(char *)(iVar21 + 0x127) == '\0') {
          if (*(char *)(iVar21 + 0x60) == '\0') {
joined_r0x0041d750:
            if (225.0 <= fVar4) goto LAB_0041d752;
          }
        }
        else {
          uVar20 = *(uint *)(iVar18 + 0x34 + iVar15);
          if (uVar20 != 0xffffffff) {
            iVar19 = (uVar20 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
            iVar22 = *(int *)(iVar18 + 0x3a0 + iVar15);
            iVar16 = *(int *)(iVar19 + 0x58);
            if (*(int *)(iVar19 + 0x58) <= iVar22) {
              iVar16 = iVar22;
            }
            if ((iVar16 == -1) ||
               ((iVar22 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                           (*(uint *)(iVar21 + 0x18) & 0xffff) * 0xc) + 0x41c),
                iVar22 != -1 && (iVar16 <= iVar22)))) {
              bVar7 = true;
            }
            else {
              bVar7 = false;
            }
            if (((*(char *)(iVar19 + 0x45) == '\0') && (*(char *)(iVar19 + 0x44) == '\0')) &&
               (*(char *)(iVar19 + 0x42) == '\0')) {
              bVar23 = true;
            }
            else {
              bVar23 = false;
            }
            if (!bVar7) goto LAB_0041d752;
            if (bVar23) goto joined_r0x0041d750;
          }
          if (*(float *)(iVar21 + 0x20) <= 0.0) {
            if (((*(char *)(iVar21 + 0x60) == '\0') || (*(short *)(iVar21 + 0x76) < 0x97)) &&
               (sVar14 = FUN_0040e760(), sVar14 < 2)) {
              fVar6 = 16.0;
              if ((*(char *)(iVar21 + 0x60) == '\0') && (*(short *)(iVar18 + 0x6a + iVar15) < 3)) {
                fVar6 = 64.0;
              }
              if (fVar4 < fVar6) goto LAB_0041d758;
            }
            goto LAB_0041d752;
          }
        }
      }
    }
    else {
LAB_0041d752:
      *(undefined2 *)(iVar21 + 0x6a) = 0;
    }
  }
LAB_0041d758:
  *(undefined1 *)(iVar21 + 0x126) = 0;
LAB_0041d75f:
  uVar11 = FUN_0041fc60();
  *(undefined1 *)(iVar21 + 0xa4) = uVar11;
  fVar25 = (float10)actor_rate_potential_target(param_1,param_2);
  *(float *)(iVar21 + 0x50) = (float)fVar25;
  fVar25 = (float10)FUN_00414590();
  *(float *)(iVar21 + 0x54) = (float)fVar25;
  *(undefined1 *)(iVar21 + 100) = 1;
  return;
}
#endif
