// actor_target_relationship_think  (Ghidra: actor_target_relationship_think, renamed)
// address 0x41abd0, size 3351 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase2/results/ai_02.json -- large per-target-data state machine (prop.kind
// states 0-5) that calls dodge/aim updates (actor_target_data_refresh,
// actor_target_update_tracking_speed), target-data release (actor_target_data_release), squad
// communication events (ai_communication_broadcast with ids 8/0xf/0x19), and posture escalation
// (actor_start_search_timer/actor_queue_velocity_search_from_prop). Walks actor.first_prop and, per prop, ages its perception and
// engagement counters, advances prop.kind through a small state machine, and emits dialogue and
// backup-request events; finishes by refreshing actor.unknown_54 and the per-actor "recheck due"
// cache at actor+0x4e (unknown_4d[1]).
// register convention: actor_index is a genuine stack parameter. Confirmed with objdump: every
// call site does `push esi` (the actor index) immediately before `call 0x41abd0`, and this
// function's own prologue loads it back from [esp+0xc4], i.e. [esp0+4] right after the return
// address -- the ordinary cdecl single-stack-argument shape, matching what Ghidra already shows.
// blam-cc: stack -> actor_index
//
// Two places where Ghidra's decompilation is provably wrong, found by reading the disassembly
// (bin/halo.exe) directly because the pseudocode looked suspicious on inspection:
//
// 1. Inside prop.kind case 1, the inner priority/urgency dispatch (0x41b16f-0x41b18b) failed
//    Ghidra's jump-table recovery ("Too many branches") and was rendered as an indirect CALL
//    through a function pointer followed by a bare `return;`. It is not a call: the instruction
//    is a local `jmp DWORD PTR [eax*4+0x41b950]` into five short blocks (0x41b192-0x41b1b5)
//    that all reconverge at 0x41b1bb, add the selected rate into target.unknown_2c, compare the
//    running sum against 1.0f, and then fall into the exact same shared tail every other
//    prop.kind case uses (0x41b45c, Ghidra's LAB_0041b468) -- it never returns early. The
//    fabricated `return` would have silently dropped the rest of the state machine (the
//    communication broadcasts, the backup-request scan) for every prop that takes this path.
//    Rewritten below as a switch driven by a 4x4 table of case selectors read directly out of
//    bin/halo.exe at 0x00655898 (rows = actor_target_get_priority_class's result 0..3, columns
//    = target.unknown_30 0..3); the five underlying rates are Actor tag fields, and the two
//    constant cases were read out of the binary at 0x00672ac0 (0.0f) and 0x00672ac4 (1.0f).
//    UNSURE: target.unknown_30 is not bounds-checked against 0..3 before this lookup in the
//    original binary either; if it can exceed 3 in practice, the real game reads further into
//    adjacent .rdata and would behave differently from the 4x4 table modeled here. Every other
//    read of target.unknown_30 in this function only ever compares it against 0, which is
//    consistent with it staying small, but that is not a proof.
//
// 2. The "aim/target refresh is due" timer just above the case-1 dispatch (0x41af7a-0x41aff6)
//    is plain 16-bit integer arithmetic in the disassembly -- two arithmetic SAR shifts and a
//    signed 16-bit compare against actor+0x4e, all on a WORD-sized stack slot -- but Ghidra's
//    `local_ac` is typed `float` and dressed up with `(float)(uint)uVar10`, `local_ac = 0.0`
//    and a `local_ac._0_2_` reinterpret. That is a stack-slot aliasing artifact: the same slot
//    (esp+0x14) is reused a little further down, in the unrelated case-1 jump-table code, to
//    hold a genuine float (`fld DWORD PTR [esp+0x14]` at 0x41ad06), and Ghidra merged the two
//    unrelated lifetimes into one wrongly-typed local. Rewritten below as plain int16_t
//    arithmetic per the disassembly.
//
// UNSURE: types/ai.h declares prop.unknown_2c as a datum_index, but this function's
// disassembly does `fadd`/`fst` on it (0x41b1bb/0x41b1be) -- it is a float accumulator here.
// Accessed through a float pointer cast rather than changing the header.
// UNSURE: self->needs_new_path (actor+0x4c) gates the perception rescan
// (actor_target_scan_potential_targets) at the top of this function. That field's name and
// evidence come from a movement-side writer (actor_update_path_if_needed @0x4017b0); its use
// here to gate a *perception* rescan is not otherwise explained. Used exactly as the header
// names it.
// UNSURE: actor_target_get_priority_class (0x41be10), actor_target_update_active_flag, actor_notify_target_engaged,
// actor_start_search_timer, actor_scan_backup_and_panic_reaction, actor_allocate_paired_prop, actor_unlink_prop and teams_are_enemies are not rewritten in
// this batch; each is declared and called with exactly the arguments Ghidra recovers at this
// call site (a register-passed argument Ghidra dropped is not guessed), with one exception:
// datum_delete's real two-argument signature is already established elsewhere in this module,
// so a genuinely argument-less call would not compile. It is called with prop_data and the
// current target_prop_index, which best fits the surrounding cleanup sequence (unlink the pair
// link, replace references, unlink the firing-position node, delete the datum) but is not
// directly evidenced by this call site's own decompilation.
// UNSURE: a dead store (Ghidra's local_88, the actor's tag-data pointer, written once and never
// read again) is dropped; it has no observable effect.
// UNSURE: actor+0xb8, read in prop.kind case 4/5 only when actor.mode==4, falls inside
// actor.mode_data (the per-mode union at actor+0x9c..0x11f) at relative offset 0x1c; accessed
// as a raw offset since types/ai.h does not further break down mode_data's per-mode shape.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *encounter_data;  // 0x008802c8
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void); // 0x4019f0

extern void actor_target_scan_potential_targets(datum_index actor_index); // 0x41d7e0
extern void actor_danger_update_reaction(datum_index actor_index);        // 0x41eda0

// blam-cc: EAX -> actor_index, ECX -> target_prop_index (objdump: `mov eax,esi` / `mov ecx,
// [esp+0x2c]` immediately before `call 0x41be10`)
extern uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index); // 0x41be10, not yet rewritten

extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force, char allow_reassign); // 0x41c4b0
extern void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index, void *scratch); // 0x41c8f0

extern uint8_t actor_target_update_active_flag(void); // 0x41fc60, not yet rewritten (phase2 name: unit_update_active_combat_flag); UNSURE: no visible args at this call site

extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50

extern void actor_notify_target_engaged(void);                 // 0x4220c0, not yet rewritten (phase2 name: actor_notify_target_engaged); UNSURE: no visible args at this call site
extern void actor_start_search_timer(void);                 // 0x422130, not yet rewritten (phase2 name: actor_start_search_timer); UNSURE: no visible args at this call site
extern void actor_queue_velocity_search_from_prop(uint32_t actor_index); // 0x4221b0, not yet rewritten (phase2 name: actor_clear_search_queue)
extern void actor_scan_backup_and_panic_reaction(uint32_t actor_index); // 0x423220, not yet rewritten (phase2 name: actor_update_search_target_for_unit)
extern uint8_t actor_is_burst_pending(void); // 0x428180, not yet rewritten (phase2 name: actor_is_ranged_burst_active); UNSURE: no visible args at this call site
extern uint8_t actor_check_burst_length_exceeded(void); // 0x4281b0, not yet rewritten (phase2 name: actor_should_end_burst); UNSURE: no visible args at this call site

// blam-cc: stack -> danger_type, danger_unknown_282; EBX -> danger_source (objdump:
// `lea ebx,[esi+0x2b0]` immediately before `call 0x4234f0`)
extern void actor_notify_squad_of_threat_direction(int16_t danger_type, int16_t danger_unknown_282, real_point3d *danger_source); // 0x4234f0, not yet rewritten (phase2 name: actor_notify_grenade_or_threat_direction)

extern uint32_t actor_target_data_release(datum_index target_prop_index, uint32_t actor_index, uint8_t *out_conflict_flag); // 0x41b980
extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0

extern datum_index actor_allocate_paired_prop(uint32_t actor_index, datum_index prop_index); // 0x43e910, not yet rewritten (phase2 name: actor_firing_position_node_new)
extern void actor_replace_object_reference(uint32_t actor_index);             // 0x428470, not yet rewritten in this batch
extern void actor_unlink_prop(void); // 0x43ea20, UNSURE signature, not yet rewritten (phase2 name: actor_firing_position_node_unlink)
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index,
                                       datum_index object_a, int32_t param_d,
                                       datum_index object_b, datum_index object_c,
                                       uint32_t *param_g);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.

// blam-cc: ECX -> team_a, EDX -> team_b
extern int8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, game module (teams_are_enemies), not yet rewritten

// Case selector for the inner priority/urgency dispatch in prop.kind case 1: read directly out
// of bin/halo.exe at 0x00655898 (int16[4][4], only the low byte of each entry is ever nonzero).
// Row = actor_target_get_priority_class's result (0..3). Column = target.unknown_30 (0..3;
// column 0 is unreachable from here since the caller only takes this path when
// target.unknown_30 != 0, but is included verbatim for fidelity). See file header UNSURE note.
static const uint8_t k_relationship_recheck_case[4][4] = {
    { 0, 1, 3, 0 },
    { 1, 2, 3, 0 },
    { 2, 3, 4, 0 },
    { 3, 4, 4, 1 },
};

void actor_target_relationship_think(datum_index actor_index)
{
    actor *self;
    Actor *definition;
    uint32_t reaction_ticks;       // Ghidra local_84: largest per-tick "recheck due" value seen this pass
    uint8_t danger_reacted;        // Ghidra local_95
    datum_index best_prop;         // Ghidra local_80
    float best_prop_distance;      // Ghidra local_7c
    datum_index cursor;            // Ghidra local_90: next prop to visit
    datum_index target_prop_index; // Ghidra local_94 / uVar12: the prop this iteration processes
    prop *target;                  // Ghidra iVar14, as a pointer
    datum_index next_prop_index;   // Ghidra uVar3
    int32_t new_kind;              // Ghidra local_9c
    uint8_t cooldown_expired;      // Ghidra local_96
    uint8_t refresh_needed;        // Ghidra bVar16
    uint8_t need_aim_refresh;      // Ghidra local_8c (only ever read as a single byte)
    uint8_t released;              // Ghidra local_a5
    uint8_t had_conflict;          // Ghidra local_9d
    uint8_t scratch1[56];          // Ghidra local_70
    uint8_t scratch2[56];          // Ghidra local_38
    int16_t danger_type;
    int16_t timer;                 // the corrected int16 "aim refresh due" scratch (Ghidra's local_ac)
    actor *owner;
    struct { int16_t team; int16_t object_type; char is_enemy; } payload; // Ghidra local_78/local_76/local_74

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    reaction_ticks = 1;
    danger_reacted = 0;
    best_prop = (datum_index)k_datum_index_none;
    best_prop_distance = 3.402823e+38f;

    if (self->keep_unit_alive != 0) goto skip_danger_response;

    if (self->needs_new_path != 0) { // UNSURE, see file header
        actor_target_scan_potential_targets(actor_index);
    }
    actor_danger_update_reaction(actor_index);

    danger_type = self->danger_type;
    if (danger_type < 1) goto skip_danger_response;

    {
        uint8_t should_react;

        if (self->unknown_28a == 0 && self->danger_unknown_282 == 0) {
            should_react = 0;
            if (self->danger_unknown_284 > 0 && self->danger_unknown_286 != 0) {
                if (self->unknown_88 == -1 || self->unknown_88 > 0x3b) {
                    self->danger_unknown_284 -= 1;
                    should_react = (uint8_t)(self->danger_unknown_284 == 0);
                } else {
                    self->danger_unknown_284 = 0;
                    should_react = 1;
                }
            }
        } else {
            self->unknown_287[0] = 1;
            should_react = (uint8_t)(self->danger_unknown_284 > 0);
            self->danger_unknown_284 = 0;
        }

        if (should_react) {
            if (danger_type == 1) {
                self->unknown_287[0] = 1;
            } else {
                float threshold = -1.0f;
                if (danger_type == 2) {
                    threshold = definition->notice_projectile_chance;
                } else if (danger_type == 3) {
                    threshold = definition->notice_vehicle_chance;
                }
                if (threshold > 0.0f && random_real() < threshold) {
                    self->unknown_287[0] = 1;
                }
            }

            if (self->unknown_287[0] != 0) {
                if (self->unknown_28a == 0) {
                    if (self->danger_unknown_282 == 0 && danger_type != 3 && danger_type != 1) {
                        self->unknown_287[1] = (uint8_t)(random_real() < definition->dive_from_grenade_chance);
                    } else {
                        self->unknown_287[1] = 1;
                    }
                } else {
                    self->unknown_287[1] = 0;
                }
                actor_notify_squad_of_threat_direction(self->danger_type, self->danger_unknown_282, &self->flee_from_point);
            }
        }
    }

    if (self->danger_unknown_284 == 0) {
        if (self->vocalization_line == 0xc) {
            if (self->vocalization_variant > 5) {
                self->vocalization_variant = 5;
            }
        }
        if (self->unknown_28a != 0) {
            self->unknown_287[0] = 1;
            self->unknown_287[1] = 0;
        }
    }

skip_danger_response:
    cursor = self->first_prop;

restart:
    target_prop_index = cursor;

    if (cursor == (datum_index)k_datum_index_none) {
        datum_index result = (datum_index)k_datum_index_none;

        if (self->target_unit_index == (datum_index)k_datum_index_none) {
            result = best_prop;
        } else {
            prop *cur = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
            if (cur->kind < 4 || cur->kind > 5) {
                result = best_prop;
            }
        }

        if (self->target_combat_status > 5) {
            self->unknown_274[0] = 1;
        }
        if (self->target_combat_status < 10) {
            if (self->unknown_1c8 == 0) {
                if (self->unknown_278 != -1) {
                    self->unknown_278 += 1;
                }
            } else {
                self->unknown_278 = -1;
            }
        } else {
            self->unknown_278 = 0;
        }

        *(int16_t *)&self->unknown_4d[1] = (int16_t)reaction_ticks;
        self->unknown_54 = result;
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (cursor & 0xffff) * sizeof(prop));
    next_prop_index = target->next_in_actor;
    cursor = next_prop_index; // matches the disassembly: [esp+0x30] is advanced unconditionally
                               // right here, before any of the per-tick processing below; the
                               // several places Ghidra shows `local_90 = uVar3` later are
                               // redundant restores of the same value already stored here.

    new_kind = -1;
    cooldown_expired = 0;
    refresh_needed = 0;
    need_aim_refresh = 0;
    released = 0;
    had_conflict = 0;

    if (target->unknown_68 > 0) {
        target->unknown_68 -= 1;
        if (target->unknown_68 == 0) {
            target->unknown_66 = -1;
        }
    }
    if (target->seen_state != -1) {
        target->seen_state += 1;
        if (target->seen_state > 0x2c) {
            target->seen = 0;
        }
    }
    if (target->unknown_b0 != -1) {
        target->unknown_b0 += 1;
        if (target->unknown_b0 > 0x3b) {
            target->unknown_b8 = 0;
            target->unknown_b4 = -1;
        }
    }
    if (target->is_vault == 0) {
        target->unknown_76 = 0;
    } else {
        target->unknown_76 += 1;
    }
    if (target->unknown_4c > 0) {
        target->unknown_4c -= 1;
    }
    if (target->unknown_6a > 0 && target->unknown_126 == 0) {
        target->unknown_6a -= 1;
    }
    if (target->unknown_9c > 0 && target->unknown_9c < 0x7fff) {
        target->unknown_9c += 1;
    }
    if (target->unknown_a8 > 0) {
        target->unknown_a8 -= 1;
        if (target->unknown_a8 == 0) {
            target->unknown_a6 -= 1;
            if (target->unknown_a6 > 0) {
                target->unknown_a8 = 0x2ee;
            }
        }
    }
    if (target->unknown_32 < 2) {
        target->unknown_78 = 0;
    } else if (target->unknown_78 < 0x7fff) {
        target->unknown_78 += 1;
    }

    if (self->keep_unit_alive == 0) {
        *(int16_t *)&target->unknown_26[0] += 1;
        timer = *(int16_t *)&target->unknown_26[0];
        if (target->is_unit == 0) {
            timer = (int16_t)(timer >> 3);
        }
        if (target->unknown_121 > 2) {
            timer = (int16_t)(timer >> 1);
        }
        if (danger_reacted == 0 && timer >= *(int16_t *)&self->unknown_4d[1]) {
            need_aim_refresh = 1;
            refresh_needed = 1;
            timer = 0;
            *(int16_t *)&target->unknown_26[0] = 0;
            danger_reacted = 1;
        }
        if ((int16_t)reaction_ticks < timer) {
            reaction_ticks = (uint16_t)timer;
        }

        if (target->kind < 0 || target->kind > 1 || target->pair_index == (datum_index)k_datum_index_none) {
            if (self->swarm == 0) {
                uint8_t important =
                    (uint8_t)((self->target_unit_index == target_prop_index) ||
                              (self->unknown_54 == target_prop_index) ||
                              (self->unknown_3ac == target_prop_index) ||
                              (self->unknown_1d0 == target_prop_index) ||
                              (self->vocalization_line != 0 && *(int16_t *)&self->vocalization_unknown_54c == 1 &&
                               self->vocalization_unknown_550 == target_prop_index) ||
                              (self->unknown_55c != 0 && self->unknown_56c == 1 &&
                               *(uint32_t *)&self->unknown_56e[2] == target_prop_index) ||
                              (self->unknown_55e[1] != 0 && *(int16_t *)&self->unknown_56e[14] == 1 &&
                               *(uint32_t *)&self->unknown_56e[18] == target_prop_index));
                target->unknown_63 = important;
                if (target->kind > 3 && target->kind < 6) {
                    prop *pair = (prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop));
                    pair->unknown_63 = important;
                }
            } else {
                target->unknown_63 = 0;
            }
        }

        if ((target->unknown_63 != 0 && (target->kind < 0 || target->kind > 1)) || refresh_needed) {
            actor_target_data_refresh(actor_index, target_prop_index, scratch1, 0, need_aim_refresh);
        }
        if (need_aim_refresh != 0) {
            actor_target_update_tracking_speed(actor_index, target_prop_index, scratch1);
        }
    } else {
        target->unknown_63 = 0;
        *(int16_t *)&target->unknown_26[0] = 0;
    }

    switch (target->kind) {
    case 0:
        if (target->unknown_30 > 0) {
            new_kind = 1;
            *(float *)&target->unknown_2c = 0.0f; // UNSURE, see file header
            goto case1_dispatch;
        }
        break;

    case 1:
    case1_dispatch:
        if (target->unknown_30 != 0) {
            int priority_class = actor_target_get_priority_class(actor_index, target_prop_index);
            int danger = target->unknown_30 & 3;
            float rate;

            switch (k_relationship_recheck_case[priority_class & 3][danger]) {
            case 0: rate = 0.0f; break;
            case 1: rate = definition->inverse_non_combat_perception_time; break;
            case 2: rate = definition->inverse_guard_perception_time; break;
            case 3: rate = definition->inverse_combat_perception_time; break;
            default: rate = 1.0f; break; // case 4
            }

            *(float *)&target->unknown_2c += rate;
            if (*(float *)&target->unknown_2c >= 1.0f) {
                new_kind = 3;
            }
            goto apply_new_kind;
        }
        *(float *)&target->unknown_2c = 0.0f; // UNSURE, see file header
        new_kind = 0;
        goto apply_new_kind;

    case 2:
        if (target->unknown_30 < 1) {
            if (target->unknown_4c != 0) {
                float dx = target->last_known_position.x - target->unknown_80.x;
                float dy = target->last_known_position.y - target->unknown_80.y;
                if (dx * dx + dy * dy <= 1.0f) {
                    break;
                }
            }
            owner = (target->owner_actor_index == (datum_index)k_datum_index_none)
                        ? (actor *)0
                        : (actor *)((uint8_t *)actor_data->data + (target->owner_actor_index & 0xffff) * sizeof(actor));
            if (target->is_unit != 0 && target->is_vault == 0 &&
                (target->is_parented != 0 ||
                 ((owner == (actor *)0 || (owner->active != 0 && owner->keep_unit_alive == 0)) &&
                  target->distance * target->distance <= 1600.0f))) {
                actor_target_data_refresh(actor_index, target_prop_index, scratch2, 0, 0);
                actor_target_get_relationship_object(target_prop_index);
                actor_allocate_paired_prop(actor_index, target_prop_index);
            }
        replace_and_idle:
            actor_replace_object_reference(actor_index);
            new_kind = 0;
        } else {
            new_kind = 3;
        }
        goto apply_new_kind;

    case 3:
        if (target->unknown_30 == 0) {
            owner = (target->owner_actor_index == (datum_index)k_datum_index_none)
                        ? (actor *)0
                        : (actor *)((uint8_t *)actor_data->data + (target->owner_actor_index & 0xffff) * sizeof(actor));
            if (target->is_unit == 0 || target->is_vault != 0 ||
                (target->is_parented == 0 &&
                 ((owner != (actor *)0 && (owner->active == 0 || owner->keep_unit_alive != 0)) ||
                  target->distance * target->distance > 1600.0f))) {
                goto replace_and_idle;
            }
            new_kind = 2;
            goto apply_new_kind;
        }
        break;

    case 4:
    case 5: {
        int penalty;

        if (target->kind == 4 &&
            (target->unknown_32 > 1 ||
             (self->unknown_60c == 1 && self->unknown_610 == target_prop_index &&
              game_time->game_time % 3 == 0))) {
            char nearly_dead = self->unknown_162[0];
            int16_t threshold = (int16_t)((nearly_dead != 0) ? 300 : 45);
            target->unknown_3c += 1;
            if (target->unknown_3c >= threshold) {
                new_kind = 5;
            }
        }

        if (target_prop_index == self->unknown_3ac ||
            (self->mode == 4 && *(uint32_t *)&self->mode_data[0x1c] == target_prop_index)) { // UNSURE, see file header
            penalty = 0;
        } else if (target_prop_index == self->target_unit_index) {
            penalty = (target->noticed_c != 0) ? 1 : 0;
        } else if (target_prop_index == self->unknown_54) {
            penalty = (self->unknown_6e < 4) ? 1 : 6;
        } else {
            penalty = 10;
        }
        target->unknown_3a = (int16_t)(target->unknown_3a - penalty);
        if (target->unknown_3a < 0) {
            cooldown_expired = 1;
        }
        if (new_kind != -1) {
            goto apply_new_kind;
        }
        goto check_cooldown;
    }
    }

tail:
    if (target->combat_dirty != 0 && target->kind > 1 && target->kind < 4) {
        if (target->unknown_129 != 0) {
            actor_scan_backup_and_panic_reaction(actor_index);
            target->unknown_129 = 0;
        }
        if (target->unknown_12a != 0 || (released != 0 && target->unknown_32 > 0)) {
            actor_notify_target_engaged();
            target->unknown_12a = 0;
        }
        if (self->unknown_377 == 0 && target->is_unit == 0 && target->is_parented != 0 &&
            target->unknown_32 > 1 && target->unknown_122 < 3 && target->distance < 7.0f) {
            self->unknown_377 = 1;
            ai_communication_broadcast(0x19, self->unit_index, target->object_index, 2, (uint32_t)-1, (uint32_t)-1, 0);
            actor_notify_target_engaged();
        }
        if (self->unit_index != (datum_index)k_datum_index_none && target->is_vault == 0 &&
            target->unknown_61 != 0 && target->unknown_62 != 0) {
            float dist_threshold;
            payload.object_type = target->object_type;
            payload.team = self->team;
            payload.is_enemy = (char)teams_are_enemies(target->object_type, self->team);
            if (payload.is_enemy == 0) {
                dist_threshold = (target->unknown_122 < 3) ? 10.0f : 3.0f;
            } else {
                dist_threshold = 15.0f;
            }
            if ((payload.is_enemy != 0 && target->seen != 0) || target->distance < dist_threshold) {
                ai_communication_broadcast(8, self->unit_index, target->object_index,
                                            (int32_t)((payload.is_enemy != 0 ? 2 : 0) + 2),
                                            (uint32_t)-1, 1, (uint32_t *)&payload); // 7th argument: extra_data, a pointer as in the definition
            }
        }

        if (self->awareness_level < 3) {
            if (target->is_vault != 0) {
                if (target->is_unit != 0) goto clear_search_and_continue;
                actor_start_search_timer();
                goto after_posture;
            }
            if (target->is_unit != 0) {
            clear_search_and_continue:
                actor_queue_velocity_search_from_prop(actor_index);
                goto after_posture;
            }
        } else {
        after_posture:
            if (target->is_unit != 0) goto restart;
        }

        if (target->is_vault == 0 && target->is_parented == 0) {
            if (self->target_unit_index == (datum_index)k_datum_index_none ||
                (self->unknown_278 != -1 && self->unknown_278 < 0xb4)) {
                if (self->encounter_index == (datum_index)k_datum_index_none) goto restart;
                {
                    encounter *enc = (encounter *)((uint8_t *)encounter_data->data + (self->encounter_index & 0xffff) * sizeof(encounter));
                    if (enc->unknown_50 != (datum_index)k_datum_index_none &&
                        (enc->unknown_50 < 0xb4 || enc->unknown_44 == 0)) {
                        goto restart;
                    }
                }
            }
            if (self->unit_index != (datum_index)k_datum_index_none) {
                if (self->awareness_level < 3) {
                    if (target->unknown_12c != 0) {
                        ai_communication_broadcast(0xf, target->object_index, self->unit_index, 2, (uint32_t)-1, 2, 0);
                    }
                } else {
                    char busy = (char)actor_is_burst_pending();
                    if (busy != 0) {
                        char should_end = (char)actor_check_burst_length_exceeded();
                        if (should_end == 0 && target->unknown_12b != 0 && target->unknown_32 > 1) {
                            ai_communication_broadcast(0xf, self->unit_index, target->object_index, 2, (uint32_t)-1, 2, 0);
                        }
                    }
                }
            }
        }
        goto restart;
    }
    if (target->kind > 3 && target->kind < 6 && target->distance < best_prop_distance) {
        best_prop = target_prop_index;
        best_prop_distance = target->distance;
    }
    goto restart;

apply_new_kind:
    switch (new_kind) {
    case 0:
    case 4:
    case 5:
        target->unknown_b8 = 0;
        target->unknown_b4 = -1;
        break;
    case 2:
        target->unknown_4c = (uint16_t)((target->unknown_32 < 2) ? 10 : 60);
        break;
    case 3:
        released = (uint8_t)actor_target_data_release(target_prop_index, actor_index, &had_conflict);
        cursor = target->next_in_actor; // re-read: the release call may have changed the list
        break;
    }
    target->kind = (int16_t)new_kind;
    target->engaged = actor_target_update_active_flag();
    target->desirability = actor_rate_potential_target(actor_index, target_prop_index);

check_cooldown:
    if (cooldown_expired == 0) {
        // matches Ghidra's `break;` out of the outer switch(prop.kind): fall to the shared
        // trailing communication / best-candidate code below.
        goto tail;
    }

    {
        prop *pair = (prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop));
        pair->pair_index = (datum_index)k_datum_index_none;
    }
    actor_replace_object_reference(actor_index);
    actor_unlink_prop();
    datum_delete(prop_data, target_prop_index); // UNSURE, see file header
    goto restart;
}

#if 0
Original Ghidra decompilation (0x41abd0), full output via `python tools/pack.py 0x41abd0`:

void FUN_0041abd0(uint param_1)

{
  undefined2 uVar1;
  undefined2 uVar2;
  uint uVar3;
  float fVar4;
  uint uVar5;
  undefined1 uVar6;
  char cVar7;
  short sVar8;
  short sVar9;
  ushort uVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  bool bVar16;
  float10 fVar17;
  float fVar18;
  float local_ac;
  char local_a5;
  char local_9d;
  undefined4 local_9c;
  char local_96;
  char local_95;
  uint local_94;
  uint local_90;
  uint local_8c;
  int local_88;
  uint local_84;
  uint local_80;
  float local_7c;
  undefined2 local_78;
  undefined2 local_76;
  char local_74;
  undefined1 local_70 [56];
  undefined1 local_38 [56];

  iVar14 = *(int *)(DAT_00880360 + 0x34);
  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar15 = iVar14 + iVar13;
  iVar11 = *(int *)((*(uint *)(iVar14 + 0x58 + iVar13) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_84 = 1;
  local_95 = '\0';
  local_80 = 0xffffffff;
  local_7c = 3.4028235e+38;
  local_88 = iVar11;
  if (*(char *)(iVar14 + 0x13 + iVar13) != '\0') goto LAB_0041ae08;
  if (*(char *)(iVar15 + 0x4c) != '\0') {
    FUN_0041d7e0(param_1);
  }
  FUN_0041eda0(param_1);
  sVar9 = *(short *)(iVar15 + 0x280);
  if (sVar9 < 1) goto LAB_0041ae08;
  if ((*(char *)(iVar15 + 0x28a) == '\0') && (*(short *)(iVar15 + 0x282) == 0)) {
    if ((0 < *(short *)(iVar15 + 0x284)) && (*(char *)(iVar15 + 0x286) != '\0')) {
      if ((*(int *)(iVar15 + 0x88) == -1) || (0x3b < *(int *)(iVar15 + 0x88))) {
        sVar8 = *(short *)(iVar15 + 0x284) + -1;
        bVar16 = sVar8 == 0;
        *(short *)(iVar15 + 0x284) = sVar8;
        goto LAB_0041ace2;
      }
      *(undefined2 *)(iVar15 + 0x284) = 0;
      goto LAB_0041acea;
    }
  }
  else {
    *(undefined1 *)(iVar15 + 0x287) = 1;
    bVar16 = 0 < *(short *)(iVar15 + 0x284);
    *(undefined2 *)(iVar15 + 0x284) = 0;
LAB_0041ace2:
    if (bVar16) {
LAB_0041acea:
      if (sVar9 == 1) {
LAB_0041ad27:
        *(undefined1 *)(iVar15 + 0x287) = 1;
      }
      else if (sVar9 == 2) {
        local_ac = *(float *)(iVar11 + 0x50);
LAB_0041ad06:
        if ((0.0 < local_ac) && (fVar18 = random_real(), fVar18 < local_ac)) goto LAB_0041ad27;
      }
      else if (sVar9 == 3) {
        local_ac = *(float *)(iVar11 + 0x54);
        goto LAB_0041ad06;
      }
      if (*(char *)(iVar15 + 0x287) != '\0') {
        if (*(char *)(iVar15 + 0x28a) == '\0') {
          if (((*(short *)(iVar15 + 0x282) == 0) && (*(short *)(iVar15 + 0x280) != 3)) &&
             (*(short *)(iVar15 + 0x280) != 1)) {
            fVar18 = random_real();
            if (*(float *)(iVar11 + 0x88) <= fVar18) {
              *(undefined1 *)(iVar15 + 0x288) = 0;
            }
            else {
              *(undefined1 *)(iVar15 + 0x288) = 1;
            }
          }
          else {
            *(undefined1 *)(iVar15 + 0x288) = 1;
          }
        }
        else {
          *(undefined1 *)(iVar15 + 0x288) = 0;
        }
        FUN_004234f0(*(undefined2 *)(iVar15 + 0x280),*(undefined2 *)(iVar15 + 0x282));
      }
    }
  }
  if (*(short *)(iVar15 + 0x284) == 0) {
    if (*(short *)(iVar15 + 0x544) == 0xc) {
      sVar9 = *(short *)(iVar15 + 0x546);
      if (5 < sVar9) {
        sVar9 = 5;
      }
      *(short *)(iVar15 + 0x546) = sVar9;
    }
    if (*(char *)(iVar15 + 0x28a) != '\0') {
      *(undefined1 *)(iVar15 + 0x287) = 1;
      *(undefined1 *)(iVar15 + 0x288) = 0;
    }
  }
LAB_0041ae08:
  local_90 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x50 + iVar13);
LAB_0041ae20:
  uVar12 = local_90;
  local_94 = local_90;
  if (local_90 == 0xffffffff) {
    if (((*(uint *)(iVar15 + 0x270) == 0xffffffff) ||
        (sVar9 = *(short *)((*(uint *)(iVar15 + 0x270) & 0xffff) * 0x138 +
                            *(int *)(DAT_008802c0 + 0x34) + 0x24), sVar9 < 4)) ||
       (uVar12 = 0xffffffff, 5 < sVar9)) {
      uVar12 = local_80;
    }
    if (5 < *(short *)(iVar15 + 0x268)) {
      *(undefined1 *)(iVar15 + 0x274) = 1;
    }
    if (*(short *)(iVar15 + 0x268) < 10) {
      if (*(char *)(iVar15 + 0x1c8) == '\0') {
        if (*(int *)(iVar15 + 0x278) != -1) {
          *(int *)(iVar15 + 0x278) = *(int *)(iVar15 + 0x278) + 1;
        }
      }
      else {
        *(undefined4 *)(iVar15 + 0x278) = 0xffffffff;
      }
    }
    else {
      *(undefined4 *)(iVar15 + 0x278) = 0;
    }
    *(short *)(iVar15 + 0x4e) = (short)local_84;
    *(uint *)(iVar15 + 0x54) = uVar12;
    return;
  }
  iVar14 = (local_90 & 0xffff) * 0x138;
  uVar3 = *(uint *)(iVar14 + 8 + *(int *)(DAT_008802c0 + 0x34));
  iVar14 = iVar14 + *(int *)(DAT_008802c0 + 0x34);
  local_9c = 0xffffffff;
  local_96 = '\0';
  bVar16 = false;
  uVar5 = local_8c >> 8;
  local_8c = local_8c & 0xffffff00;
  local_a5 = '\0';
  local_9d = '\0';
  if ((0 < *(short *)(iVar14 + 0x68)) &&
     (sVar9 = *(short *)(iVar14 + 0x68) + -1, *(short *)(iVar14 + 0x68) = sVar9, sVar9 == 0)) {
    *(undefined2 *)(iVar14 + 0x66) = 0xffff;
  }
  if ((*(short *)(iVar14 + 0x6c) != -1) &&
     (sVar9 = *(short *)(iVar14 + 0x6c) + 1, *(short *)(iVar14 + 0x6c) = sVar9, 0x2c < sVar9)) {
    *(undefined1 *)(iVar14 + 0x74) = 0;
  }
  if ((*(short *)(iVar14 + 0xb0) != -1) &&
     (sVar9 = *(short *)(iVar14 + 0xb0) + 1, *(short *)(iVar14 + 0xb0) = sVar9, 0x3b < sVar9)) {
    *(undefined1 *)(iVar14 + 0xb8) = 0;
    *(undefined4 *)(iVar14 + 0xb4) = 0xffffffff;
  }
  if (*(char *)(iVar14 + 0x127) == '\0') {
    *(undefined2 *)(iVar14 + 0x76) = 0;
  }
  else {
    *(short *)(iVar14 + 0x76) = *(short *)(iVar14 + 0x76) + 1;
  }
  if (0 < *(short *)(iVar14 + 0x4c)) {
    *(short *)(iVar14 + 0x4c) = *(short *)(iVar14 + 0x4c) + -1;
  }
  if ((0 < *(short *)(iVar14 + 0x6a)) && (*(char *)(iVar14 + 0x126) == '\0')) {
    *(short *)(iVar14 + 0x6a) = *(short *)(iVar14 + 0x6a) + -1;
  }
  sVar9 = *(short *)(iVar14 + 0x9c);
  if ((0 < sVar9) && (sVar9 < 0x7fff)) {
    *(short *)(iVar14 + 0x9c) = sVar9 + 1;
  }
  if (((0 < *(short *)(iVar14 + 0xa8)) &&
      (sVar9 = *(short *)(iVar14 + 0xa8) + -1, *(short *)(iVar14 + 0xa8) = sVar9, sVar9 == 0)) &&
     (*(short *)(iVar14 + 0xa6) = *(short *)(iVar14 + 0xa6) + -1, 0 < *(short *)(iVar14 + 0xa6))) {
    *(undefined2 *)(iVar14 + 0xa8) = 0x2ee;
  }
  if (*(short *)(iVar14 + 0x32) < 2) {
    *(undefined2 *)(iVar14 + 0x78) = 0;
  }
  else if (*(short *)(iVar14 + 0x78) < 0x7fff) {
    *(short *)(iVar14 + 0x78) = *(short *)(iVar14 + 0x78) + 1;
  }
  if (*(char *)(iVar15 + 0x13) == '\0') {
    *(short *)(iVar14 + 0x26) = *(short *)(iVar14 + 0x26) + 1;
    uVar10 = *(ushort *)(iVar14 + 0x26);
    if (*(char *)(iVar14 + 0x60) == '\0') {
      uVar10 = (short)uVar10 >> 3;
    }
    if ('\x02' < *(char *)(iVar14 + 0x121)) {
      uVar10 = (short)uVar10 >> 1;
    }
    local_ac = (float)(uint)uVar10;
    if ((local_95 == '\0') && (*(short *)(iVar15 + 0x4e) <= (short)uVar10)) {
      local_8c = CONCAT31((int3)uVar5,1);
      bVar16 = true;
      local_ac = 0.0;
      *(undefined2 *)(iVar14 + 0x26) = 0;
      local_95 = '\x01';
    }
    if ((short)local_84 < local_ac._0_2_) {
      local_84 = (uint)local_ac;
    }
    sVar9 = *(short *)(iVar14 + 0x24);
    if (((sVar9 < 0) || (1 < sVar9)) || (*(int *)(iVar14 + 0xc) == -1)) {
      if (*(char *)(iVar15 + 6) == '\0') {
        if (((((((*(uint *)(iVar15 + 0x270) == local_90) || (*(uint *)(iVar15 + 0x54) == local_90))
               || (*(uint *)(iVar15 + 0x3ac) == local_90)) ||
              (*(uint *)(iVar15 + 0x1d0) == local_90)) ||
             (((*(short *)(iVar15 + 0x544) != 0 && (*(short *)(iVar15 + 0x54c) == 1)) &&
              (*(uint *)(iVar15 + 0x550) == local_90)))) ||
            (((*(char *)(iVar15 + 0x55c) != '\0' && (*(short *)(iVar15 + 0x56c) == 1)) &&
             (*(uint *)(iVar15 + 0x570) == local_90)))) ||
           (((*(char *)(iVar15 + 0x55f) != '\0' && (*(short *)(iVar15 + 0x57c) == 1)) &&
            (*(uint *)(iVar15 + 0x580) == local_90)))) {
          uVar6 = 1;
        }
        else {
          uVar6 = 0;
        }
        *(undefined1 *)(iVar14 + 99) = uVar6;
        if ((3 < sVar9) && (sVar9 < 6)) {
          *(undefined1 *)
           ((*(uint *)(iVar14 + 0xc) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34) + 99) = uVar6
          ;
        }
      }
      else {
        *(undefined1 *)(iVar14 + 99) = 0;
      }
    }
    if (((*(char *)(iVar14 + 99) != '\0') &&
        ((*(short *)(iVar14 + 0x24) < 0 || (1 < *(short *)(iVar14 + 0x24))))) ||
       (local_90 = uVar3, bVar16)) {
      local_90 = uVar3;
      FUN_0041c4b0(param_1,uVar12,local_70,0,local_8c);
    }
    if ((char)local_8c != '\0') {
      FUN_0041c8f0(param_1,local_94,local_70);
    }
  }
  else {
    *(undefined1 *)(iVar14 + 99) = 0;
    *(undefined2 *)(iVar14 + 0x26) = 0;
    local_90 = uVar3;
  }
  uVar12 = local_94;
  switch(*(short *)(iVar14 + 0x24)) {
  case 0:
    if (0 < *(short *)(iVar14 + 0x30)) {
      local_9c = 1;
      *(undefined4 *)(iVar14 + 0x2c) = 0;
      goto switchD_0041b13b_caseD_1;
    }
    break;
  case 1:
switchD_0041b13b_caseD_1:
    sVar9 = *(short *)(iVar14 + 0x30);
    if (sVar9 != 0) {
      sVar8 = FUN_0041be10();
                    /* WARNING: Could not recover jumptable at 0x0041b18b. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      (*(code *)(&PTR_LAB_0041b950)[*(short *)(&DAT_00655898 + ((int)sVar9 + sVar8 * 4) * 2)])();
      return;
    }
    *(undefined4 *)(iVar14 + 0x2c) = 0;
    local_9c = 0;
    goto LAB_0041b468;
  case 2:
    if (*(short *)(iVar14 + 0x30) < 1) {
      if ((*(short *)(iVar14 + 0x4c) != 0) &&
         (fVar18 = *(float *)(iVar14 + 0xbc) - *(float *)(iVar14 + 0x80),
         fVar4 = *(float *)(iVar14 + 0xc0) - *(float *)(iVar14 + 0x84),
         fVar18 * fVar18 + fVar4 * fVar4 <= 1.0)) break;
      if (*(uint *)(iVar14 + 0x1c) == 0xffffffff) {
        iVar11 = 0;
      }
      else {
        iVar11 = (*(uint *)(iVar14 + 0x1c) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      }
      if (((*(char *)(iVar14 + 0x60) != '\0') && (*(char *)(iVar14 + 0x127) == '\0')) &&
         ((*(char *)(iVar14 + 0x12e) != '\0' ||
          (((iVar11 == 0 || ((*(char *)(iVar11 + 8) != '\0' && (*(char *)(iVar11 + 0x13) == '\0'))))
           && (*(float *)(iVar14 + 0x11c) * *(float *)(iVar14 + 0x11c) <= 1600.0)))))) {
        FUN_0041c4b0(param_1,local_94,local_38,0,0);
        actor_target_get_relationship_object();
        FUN_0043e910(param_1,uVar12);
      }
LAB_0041b378:
      actor_replace_object_reference(param_1);
      local_9c = 0;
    }
    else {
      local_9c = 3;
    }
LAB_0041b468:
    switch((short)local_9c) {
    case 0:
    case 4:
    case 5:
      *(undefined1 *)(iVar14 + 0xb8) = 0;
      *(undefined4 *)(iVar14 + 0xb4) = 0xffffffff;
      break;
    case 2:
      *(ushort *)(iVar14 + 0x4c) = ((*(short *)(iVar14 + 0x32) < 2) - 1 & 0x32) + 10;
      break;
    case 3:
      local_a5 = FUN_0041b980(param_1,&local_9d);
      local_90 = *(uint *)(iVar14 + 8);
    }
    uVar12 = local_94;
    *(short *)(iVar14 + 0x24) = (short)local_9c;
    uVar6 = FUN_0041fc60();
    *(undefined1 *)(iVar14 + 0xa4) = uVar6;
    fVar17 = (float10)actor_rate_potential_target(param_1,uVar12);
    *(float *)(iVar14 + 0x50) = (float)fVar17;
LAB_0041b4ed:
    if (local_96 == '\0') break;
    *(undefined4 *)
     ((*(uint *)(iVar14 + 0xc) & 0xffff) * 0x138 + 0xc + *(int *)(DAT_008802c0 + 0x34)) = 0xffffffff
    ;
    actor_replace_object_reference(param_1);
    FUN_0043ea20();
    datum_delete();
    goto LAB_0041ae20;
  case 3:
    if (*(short *)(iVar14 + 0x30) == 0) {
      if (*(uint *)(iVar14 + 0x1c) == 0xffffffff) {
        iVar11 = 0;
      }
      else {
        iVar11 = (*(uint *)(iVar14 + 0x1c) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      }
      if (((*(char *)(iVar14 + 0x60) == '\0') || (*(char *)(iVar14 + 0x127) != '\0')) ||
         ((*(char *)(iVar14 + 0x12e) == '\0' &&
          (((iVar11 != 0 && ((*(char *)(iVar11 + 8) == '\0' || (*(char *)(iVar11 + 0x13) != '\0'))))
           || (1600.0 < *(float *)(iVar14 + 0x11c) * *(float *)(iVar14 + 0x11c)))))))
      goto LAB_0041b378;
      local_9c = 2;
      goto LAB_0041b468;
    }
    break;
  case 4:
  case 5:
    if (((*(short *)(iVar14 + 0x24) == 4) &&
        ((cVar7 = *(char *)(iVar15 + 0x162), 1 < *(short *)(iVar14 + 0x32) ||
         (((*(short *)(iVar15 + 0x60c) == 1 && (*(uint *)(iVar15 + 0x610) == local_94)) &&
          (*(int *)(DAT_006f1d6c + 0xc) % 3 == 0)))))) &&
       (*(short *)(iVar14 + 0x3c) = *(short *)(iVar14 + 0x3c) + 1,
       (short)((-(ushort)(cVar7 != '\0') & 0xff) + 0x2d) <= *(short *)(iVar14 + 0x3c))) {
      local_9c = 5;
    }
    if ((local_94 == *(uint *)(iVar15 + 0x3ac)) ||
       ((*(short *)(iVar15 + 0x6c) == 4 && (*(uint *)(iVar15 + 0xb8) == local_94)))) {
      uVar10 = 0;
    }
    else if (local_94 == *(uint *)(iVar15 + 0x270)) {
      uVar10 = (ushort)(*(char *)(iVar14 + 0xbb) != '\0');
    }
    else if (local_94 == *(uint *)(iVar15 + 0x54)) {
      uVar10 = ((*(short *)(iVar15 + 0x6e) < 4) - 1 & 5) + 1;
    }
    else {
      uVar10 = 10;
    }
    *(short *)(iVar14 + 0x3a) = *(short *)(iVar14 + 0x3a) - uVar10;
    if (*(short *)(iVar14 + 0x3a) < 0) {
      local_96 = '\x01';
    }
    if ((short)local_9c != -1) goto LAB_0041b468;
    goto LAB_0041b4ed;
  }
  if (((*(char *)(iVar14 + 100) != '\0') && (1 < *(short *)(iVar14 + 0x24))) &&
     (*(short *)(iVar14 + 0x24) < 4)) {
    if (*(char *)(iVar14 + 0x129) != '\0') {
      FUN_00423220(param_1);
      *(undefined1 *)(iVar14 + 0x129) = 0;
    }
    if ((*(char *)(iVar14 + 0x12a) != '\0') ||
       ((local_a5 != '\0' && (0 < *(short *)(iVar14 + 0x32))))) {
      FUN_004220c0();
      *(undefined1 *)(iVar14 + 0x12a) = 0;
    }
    if (((((*(char *)(iVar15 + 0x377) == '\0') && (*(char *)(iVar14 + 0x60) == '\0')) &&
         (*(char *)(iVar14 + 0x12e) != '\0')) &&
        ((1 < *(short *)(iVar14 + 0x32) && (*(char *)(iVar14 + 0x122) < '\x03')))) &&
       (*(float *)(iVar14 + 0x11c) < 7.0)) {
      *(undefined1 *)(iVar15 + 0x377) = 1;
      ai_communication_broadcast
                (0x19,*(undefined4 *)(iVar15 + 0x18),*(undefined4 *)(iVar14 + 0x18),2,0xffffffff,
                 0xffffffff,0);
      FUN_004220c0();
    }
    iVar11 = *(int *)(iVar15 + 0x18);
    if (((iVar11 != -1) && (*(char *)(iVar14 + 0x127) == '\0')) &&
       ((*(char *)(iVar14 + 0x61) != '\0' && (*(char *)(iVar14 + 0x62) != '\0')))) {
      uVar1 = *(undefined2 *)(iVar14 + 0x12);
      uVar2 = *(undefined2 *)(iVar15 + 0x3e);
      cVar7 = FUN_0045bd50();
      if (cVar7 == '\0') {
        if (*(char *)(iVar14 + 0x122) < '\x03') {
          fVar18 = 10.0;
        }
        else {
          fVar18 = 3.0;
        }
      }
      else {
        fVar18 = 15.0;
      }
      if (((cVar7 != '\0') && (*(char *)(iVar14 + 0x74) != '\0')) ||
         (*(float *)(iVar14 + 0x11c) < fVar18)) {
        local_78 = uVar2;
        local_76 = uVar1;
        local_74 = cVar7;
        ai_communication_broadcast
                  (8,iVar11,*(undefined4 *)(iVar14 + 0x18),(cVar7 != '\0') * '\x02' + '\x02',
                   0xffffffff,1,&local_78);
      }
    }
    if (*(short *)(iVar15 + 0x6a) < 3) {
      if (*(char *)(iVar14 + 0x127) != '\0') {
        if (*(char *)(iVar14 + 0x60) != '\0') goto LAB_0041b721;
        FUN_00422130();
        goto LAB_0041b735;
      }
      if (*(char *)(iVar14 + 0x60) != '\0') {
LAB_0041b721:
        FUN_004221b0(param_1);
        goto LAB_0041b735;
      }
    }
    else {
LAB_0041b735:
      if (*(char *)(iVar14 + 0x60) != '\0') goto LAB_0041ae20;
    }
    if ((*(char *)(iVar14 + 0x127) == '\0') && (*(char *)(iVar14 + 0x12e) == '\0')) {
      if ((*(int *)(iVar15 + 0x270) == -1) ||
         ((*(int *)(iVar15 + 0x278) != -1 && (*(int *)(iVar15 + 0x278) < 0xb4)))) {
        if (*(uint *)(iVar15 + 0x34) == 0xffffffff) goto LAB_0041ae20;
        iVar13 = (*(uint *)(iVar15 + 0x34) & 0xffff) * 0x6c;
        iVar11 = *(int *)(iVar13 + 0x50 + *(int *)(DAT_008802c8 + 0x34));
        if ((iVar11 != -1) &&
           ((iVar11 < 0xb4 || (*(char *)(iVar13 + *(int *)(DAT_008802c8 + 0x34) + 0x44) == '\0'))))
        goto LAB_0041ae20;
      }
      iVar11 = *(int *)(iVar15 + 0x18);
      if (iVar11 != -1) {
        if (*(short *)(iVar15 + 0x6a) < 3) {
          if (*(char *)(iVar14 + 300) != '\0') {
            ai_communication_broadcast(0xf,*(undefined4 *)(iVar14 + 0x18),iVar11,2,0xffffffff,2,0);
          }
        }
        else {
          cVar7 = FUN_00428180();
          if ((((cVar7 != '\0') && (cVar7 = FUN_004281b0(), cVar7 == '\0')) &&
              (*(char *)(iVar14 + 299) != '\0')) && (1 < *(short *)(iVar14 + 0x32))) {
            ai_communication_broadcast(0xf,iVar11,*(undefined4 *)(iVar14 + 0x18),2,0xffffffff,2,0);
          }
        }
      }
    }
    goto LAB_0041ae20;
  }
  if (((3 < *(short *)(iVar14 + 0x24)) && (*(short *)(iVar14 + 0x24) < 6)) &&
     (*(float *)(iVar14 + 0x11c) < local_7c)) {
    local_80 = local_94;
    local_7c = *(float *)(iVar14 + 0x11c);
  }
  goto LAB_0041ae20;
}
#endif
