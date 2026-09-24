// actor_squad_action_execute  (Ghidra: actor_squad_action_execute, already named)
// address 0x405520, size 4338 bytes
// name confidence: 0.5   rewrite confidence: 0.2
// evidence: types/ai.h actor.unit_index (0x18)/active_unit_index (0x158)/unknown_504/
//   unknown_50a; types/tags.h Scenario.command_lists (TagReflexive at 0x438, pointer at
//   0x43c), ScenarioCommandList.commands/points (TagReflexive at +0x30/+0x3c, pointer at
//   +0x34/+0x40), and ScenarioCommand's already-declared field layout (atom_type,
//   atom_modifier, parameter1, parameter2, point_1, point_2, animation, script, recording,
//   command, object_name) -- every one of psVar12[N]'s offsets in the original matches this
//   struct exactly. ScenarioCommandPoint (already declared, size 0x14) matches the
//   `pfVar15[0x10] + n*0x14` indexing. types/objects.h object.velocity (0x68)/forward
//   (0x74)/parent_object (0x11c) match the "moving forward" dot-product test in case 10.
//
// This is by a wide margin the largest and least-understood function in this session's
// range (27 cases over one scripted action-list entry). Given the scope, several things are
// deliberately left close to the Ghidra decompilation rather than fully re-derived:
//
// UNSURE, broadly:
//  - `state` (param_4) is the per-actor squad-action-machine record also read/written by
//    actor_squad_action_is_complete (0x4066d0) and actor_squad_action_list_process
//    (0x406e30), both elsewhere in this session. Its offsets are used the same way in all
//    three files but are not backed by a types/ai.h struct (TYPES-GAP: no shared typedef,
//    because the three functions only clearly agree on byte 0, the current action index).
//  - `aim_state` (the register-inherited in_EAX pointer) is assumed to be actor.mode_data,
//    since every offset used against it (up to +0x54) fits inside mode_data's 0xbc bytes and
//    this function only ever runs while a mode's action list is active. Not proven.
//  - check_object_index is compared against actor.unit_index throughout to decide "is this
//    entry being evaluated for the actor's own controlled unit or some other object" --
//    plausible but not proven.
//  - Several out-of-range callees (FUN_00401020, actor_begin_vocalization, actor_get_body_axis_vector's forwarded
//    float, actor_find_prop_for_object/_ecd0/_ecf0, recorded_animation_find_by_name/_44a930, unit_animation_change_priority_check/_60f20,
//    unit_set_grenade_type_and_count_delta, unit_get_forward_vector_or_marker_normal) return or consume values through the x87 FPU stack or
//    hidden output pointers that Ghidra could not show as explicit arguments; declared here
//    with the most plausible signature for each call site, individually noted below.
//  - Cases 0 and 0x16 call `__ftol()` (float-to-int truncation) with no visible float
//    operand in the decompilation; the most consistent reading, matching every other
//    duration field in this module, is entry->parameter1 (or a callee's returned float)
//    times 30 ticks/second, but this is a guess.
//  - actor+0x504/0x50a (unknown_504/unknown_50a) and several `object+0x9e/0x106/0x2f4`-style
//    reads are used as raw offsets; they are not decoded further here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern double fcos(double x); // FCOS
extern double fsin(double x); // FSIN

// TYPES (folded into types/ai.h by the review pass): mirrors actor_axis_request from actor_get_body_axis_vector.c (case 0x16 below
// only needs the axis selector and the result vector).

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c

extern float FUN_00401020(void);                                // 0x401020, not yet rewritten: a distance/score helper returning a float
extern real random_real_range(real min, real max);           // 0x401050
extern real vector2d_normalize_with_length(real_vector2d *v);  // 0x4018e0
extern real vector3d_normalize_with_length(real_vector3d *v);  // 0x401990
extern int32_t random_int_range(int32_t exclusive_max);         // 0x405320
extern void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request); // 0x405390, this session
extern uint8_t actor_play_first_valid_vocalization(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_play_first_valid_vocalization at 0x40e260
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_begin_vocalization(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_begin_vocalization at 0x4142d0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, this module,
                                                    // blam-cc: EAX -> destination, stack -> the other three
extern void actor_movement_actions_cancel(void);                                 // 0x417a30
extern void actor_fill_unit_position_context(void *position);                                       // 0x4296c0, not yet rewritten
extern void ai_communication_target_result_reset(void);                                                 // 0x42d310, not yet rewritten
extern datum_index actor_find_prop_for_object(datum_index object_index);                                        // 0x43ea80, not yet rewritten
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator); // 0x43ecd0, rewritten as src/ai/actor_prop_iterator_init.c; blam-cc: EAX -> actor_index, stack -> iterator
extern prop *actor_prop_iterator_next(actor_prop_iterator *iterator);                      // 0x43ecf0, rewritten as src/ai/actor_prop_iterator_next.c; blam-cc: EDX -> iterator
extern int32_t recorded_animation_find_by_name(void);                                              // 0x449f80, not yet rewritten
extern int32_t FUN_0044a930(int32_t x);                                         // 0x44a930, not yet rewritten
extern char hs_call_script_by_name(void);                                       // 0x48a2d0
extern void * data_iterator_next(data_iterator *iterator);                              // 0x4d05d0
extern void object_reset_velocity_and_wake(uint32_t object_index);              // 0x4f5160
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                                real_vector3d *up, real_point3d *position); // 0x4f51c0,
                                                // blam-cc: stack -> object_index, forward, up; EDI -> position
extern void object_get_position(void);                                          // 0x4f6900, writes through a register-inherited pointer
extern void *object_try_and_get(int32_t kind);                                  // 0x4f6ec0
extern int32_t object_iterator_next(void *iterator);                            // 0x4f6f20
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern int16_t unit_animation_change_priority_check(int32_t a, int32_t b, int32_t c, void *d, void *e); // 0x560d00, not yet rewritten
extern void unit_commit_speech(void);                                                 // 0x560f20, not yet rewritten
extern void unit_get_primary_eye_marker_position(void);                                                 // 0x568f50, not yet rewritten
extern void unit_get_forward_vector_or_marker_normal(void);                                                 // 0x569720, not yet rewritten
extern void unit_set_grenade_type_and_count_delta(int32_t a);                                            // 0x56d160, not yet rewritten
extern char unit_start_user_animation(uint32_t unit_or_object_index, uint32_t animation_selector); // 0x5702a0
extern void _qsort(void *base, int32_t count, int32_t size, int32_t (*cmp)(const void *, const void *)); // 0x623410
extern int32_t float_compare_ascending(const void *a, const void *b); // 0x405360, this session (skipped: library qsort comparator)
extern const uint32_t DAT_0065512c; // 0x0065512c, a vocalization category table passed straight through to actor_play_first_valid_vocalization
extern int32_t object_lookup_table_get(void); // 0x4f73c0, not yet rewritten (redeclared with its real return type; see case 0x19)

// Executes the squad's current scripted action-list entry (command list command_list_index,
// entry index state[0]) for this actor, branching on the entry's atom type. aim_state is the per-mode
// aim/look sub-record (see header UNSURE note); this is now an explicit parameter, matching
// the sibling functions actor_squad_action_is_complete/actor_squad_action_reset_entry and
// their shared driver actor_squad_action_list_process, all of which receive it the same way
// rather than deriving it internally. See the file
// header for the scope of what is and is not named here; local variable names below match
// the Ghidra decompilation (psVar12/pfVar15/entry, iVar8, local_*) so this can be diffed
// against the #if 0 block.
char actor_squad_action_execute(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *actor_base = (uint8_t *)a;
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    int32_t command_count = (int32_t)list->commands.count;
    int32_t current_action_index = state[0];
    ScenarioCommand *entry;
    char result = 0;

    if (command_count <= current_action_index) {
        return 0;
    }
    entry = (ScenarioCommand *)((uint8_t *)list->commands.pointer + current_action_index * sizeof(ScenarioCommand));

    switch (entry->atom_type) {
    case 0: {
        // UNSURE: see header note on the two __ftol-with-no-visible-operand cases.
        int32_t ticks = (int32_t)(entry->parameter1 * 30.0f);
        state[2] = (uint8_t)ticks;
        state[3] = (uint8_t)(ticks >> 8);
        return 1;
    }
    case 1:
    case 2: {
        int16_t point_index = (int16_t)entry->point_1;
        if (aim_state != 0 && point_index >= 0 && point_index < (int32_t)list->points.count) {
            ScenarioCommandPoint *point = &((ScenarioCommandPoint *)list->points.pointer)[point_index];
            char moved;

            state[2] = 0;
            state[3] = 0;
            aim_state[4] = 1;
            aim_state[5] = (entry->atom_modifier == 1);
            *(real_point3d *)(aim_state + 8) = *(real_point3d *)&point->position;
            *(uint32_t *)(aim_state + 0x14) = point->surface_index;
            // 0x405672: EAX = aim_state + 8 (the point just copied there), then push ebp
            // (actor_index) / ecx (surface_index) / -1. Ghidra hides the EAX argument and
            // the earlier draft had the destination and the actor index the wrong way round.
            moved = (char)actor_movement_set_destination_point((real_point3d *)(aim_state + 8), actor_index,
                                                               (int32_t)point->surface_index, 0xffffffff);
            if (moved != 0) {
                if (aim_state[5] != 0) {
                    actor_movement_actions_cancel();
                }
                if (entry->atom_type == 2 && (int16_t)entry->point_2 >= 0 &&
                    (int16_t)entry->point_2 < (int32_t)list->points.count) {
                    ScenarioCommandPoint *point2 = &((ScenarioCommandPoint *)list->points.pointer)[entry->point_2];

                    aim_state[0x18] = 1;
                    *(real_point3d *)(aim_state + 0x1c) = *(real_point3d *)&point2->position;
                    return moved;
                }
            }
        }
        break;
    }
    case 3: {
        real_point3d *out_point;
        real_point3d other_position;
        // REVIEW FIX: the original reads actor+0x12c (actor.body_position), not actor+0x174
        // (actor.position), and it has an else branch. When the action is aimed at some
        // other object, the heading below is measured from THAT object position, which the
        // earlier rewrite dropped entirely.
        if (check_object_index == a->unit_index) {
            *(real_point3d *)(state + 0x18) = a->body_position;
            out_point = (real_point3d *)(state + 0x18);
        } else {
            // UNSURE: Ghidra shows `pfVar9 = (float *)object_get_position();`, i.e. a bare
            // call whose return value it treats as the point pointer. The out-pointer is a
            // register argument; a local is used here so the read below is well defined.
            object_get_position();
            out_point = &other_position;
        }
        {
            int16_t heading = (int16_t)entry->point_1;
            uint8_t moved_to_point = 0;
            real_vector3d delta;

            if (heading < 0 || heading >= (int32_t)list->points.count) {
                float angle = entry->parameter2;
                if (angle < 0.0f || angle >= 360.0f) {
                    return 0;
                }
                state[0x14] = 0;
                state[0x15] = 0;
                state[0x16] = 0;
                state[0x17] = 0;
                *(float *)(state + 0xc) = (float)fcos(angle * 0.017453292);
                *(float *)(state + 0x10) = (float)fsin(angle * 0.017453292);
            } else {
                ScenarioCommandPoint *point = &((ScenarioCommandPoint *)list->points.pointer)[heading];

                delta.i = point->position.x - out_point->x;
                delta.j = point->position.y - out_point->y;
                delta.k = point->position.z - out_point->z;
                *(float *)(state + 0xc) = delta.i;
                *(float *)(state + 0x10) = delta.j;
                *(float *)(state + 0x14) = delta.k;
                if (vector3d_normalize_with_length(&delta) <= 0.0f) {
                    return 0;
                }
                moved_to_point = 1;
            }
            (void)moved_to_point;

            {
                int16_t direction = (int16_t)entry->atom_modifier;
                if (direction < 0 || direction > 3) {
                    state[8] = 0xff;
                    state[9] = 0xff;
                } else {
                    *(int16_t *)(state + 8) = direction;
                }
            }
            if (check_object_index == a->unit_index) {
                actor_movement_action_stop(actor_index);
            }
            state[5] = (state[5] & 0xfd) | 1;
            return 1;
        }
    }
    case 4:
    case 0x17:
    case 0x18:
    case 0x19: {
        int16_t chosen_point = -1;
        float chosen_delay = -1.0f; // stands in for the original's -NAN "unset" sentinel
        int32_t chosen_object = -1;
        uint8_t have_delay = 0;

        if (aim_state == 0) {
            return 0;
        }
        if (entry->atom_type == 4) {
            int16_t p = (int16_t)entry->point_1;
            if (p >= 0 && p < (int32_t)list->points.count) {
                chosen_delay = entry->parameter1;
                have_delay = 1;
                chosen_point = p;
            }
        } else if (entry->atom_type == 0x17) {
            if ((int16_t)entry->point_1 >= 0 && (int16_t)entry->point_1 < (int32_t)list->points.count) {
                int16_t p2 = (int16_t)entry->point_2;
                if (p2 >= 0 && p2 < (int32_t)list->points.count) {
                    chosen_point = (int16_t)random_int_range(p2 + 1);
                    if (entry->parameter1 == 0.0f && entry->parameter2 == 0.0f) {
                        Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
                        chosen_delay = random_real_range(*(float *)((uint8_t *)actor_def + 0xec), *(float *)((uint8_t *)actor_def + 0xf0));
                    } else {
                        chosen_delay = random_real_range(entry->parameter1, entry->parameter2);
                    }
                    have_delay = 1;
                }
            }
        } else if (entry->atom_type == 0x18) {
            // Pick the nearest perceived prop of kind 2..3 that is parented; if there is none,
            // fall back to the nearest player. Three things an earlier draft of this branch got
            // wrong, all of them visible in the Ghidra listing in the #if 0 block below:
            //  * the value the first loop selects is the ITERATOR's current prop datum index
            //    (Ghidra's `local_e8`, the first dword of the record handed to
            //    actor_prop_iterator_init), stored bit for bit into the same stack slot the
            //    delay uses (`local_d8`). The draft stored a POINTER into the object slot and
            //    compared it against an Actor tag pointer, which nothing in the original does.
            //  * the distance test is against a running minimum the compiler kept in a
            //    register -- Ghidra models it only as `extraout_ST0`, re-read after every
            //    iterator call -- not against 0.0, which the draft compared to.
            //  * the fallback player scan keeps its best score in a stack slot of its own
            //    (`local_e8`, reused) and writes the winner into the OBJECT slot (`local_d4`).
            //    The draft made the best score a field of the iterator record itself and read
            //    the player record as if it were a prop.
            actor_prop_iterator prop_iterator;
            prop *p;
            datum_index chosen_prop = (datum_index)k_datum_index_none;
            float best_distance = 3.4028235e+38f; // UNSURE: the register-held running minimum
            uint8_t chose_a_prop = 0;

            actor_prop_iterator_init(actor_index, &prop_iterator);
            p = actor_prop_iterator_next(&prop_iterator);
            if (p != 0) {
                do {
                    if (p->kind > 1 && p->kind < 4 && p->is_parented != 0 &&
                        p->distance < best_distance) {
                        best_distance = p->distance;
                        chosen_prop = prop_iterator.current;
                        chose_a_prop = 1;
                    }
                    p = actor_prop_iterator_next(&prop_iterator);
                } while (p != 0);
                if (chose_a_prop) {
                    // The original stores the datum index's BITS into the float delay slot and
                    // then tests that slot against 0xffffffff, so the bit pattern is what
                    // matters here, not a numeric conversion.
                    memcpy(&chosen_delay, &chosen_prop, sizeof(chosen_delay));
                    have_delay = 1;
                    goto have_choice;
                }
            }
            {
                // The original builds this iterator inline: {data, next_index = 0,
                // index = none, signature = data ^ 0x69746572}, the same four dwords
                // actor_iterator_new.c writes. data_iterator_next only reads the first three.
                struct {
                    data_array *data;
                    int32_t next_index;
                    datum_index index;
                    uint32_t signature;
                } player_iterator;
                void *record;
                float best_score = 3.4028235e+38f;

                player_iterator.data = player_data;
                player_iterator.next_index = 0;
                player_iterator.index = (datum_index)k_datum_index_none;
                player_iterator.signature = (uint32_t)(uintptr_t)player_data ^ 0x69746572;

                record = data_iterator_next((data_iterator *)&player_iterator);
                while (record != 0) {
                    // UNSURE offset: player record + 0x34, the player's controlled unit
                    // object index (the only field this loop reads).
                    datum_index unit_index = *(datum_index *)((uint8_t *)record + 0x34);
                    if (unit_index != (datum_index)k_datum_index_none) {
                        float score;

                        unit_get_primary_eye_marker_position();
                        score = FUN_00401020();
                        if (score < best_score) {
                            chosen_object = (int32_t)unit_index;
                            best_score = score;
                        }
                    }
                    record = data_iterator_next((data_iterator *)&player_iterator);
                }
            }
        } else if (entry->atom_type == 0x19 && (int16_t)entry->object_name >= 0 &&
                   (int16_t)entry->object_name < *(int32_t *)((uint8_t *)global_scenario + 0x204)) {
            int32_t resolved = object_lookup_table_get();
            if (object_try_and_get(3) != 0) {
                chosen_delay = (float)actor_find_prop_for_object((uint32_t)resolved);
                chosen_object = resolved;
                have_delay = 1;
            }
        }

    have_choice:
        {
            float threshold = have_delay ? entry->parameter1 : 0.0f;
            (void)threshold;
        }
        if (0.0f < entry->parameter1 &&
            (have_delay || chosen_object != -1 || (chosen_point >= 0 && chosen_point < (int32_t)list->points.count))) {
            uint32_t sub_kind = 1;
            uint8_t point_record[16];
            real_point3d chosen_position;

            if (entry->atom_modifier == 1) sub_kind = 5;
            else if (entry->atom_modifier == 2) sub_kind = 2;
            else if (entry->atom_modifier == 4) sub_kind = 7;
            else if (entry->atom_modifier == 3) sub_kind = 8;

            memset(point_record, 0, sizeof(point_record));
            if (!have_delay) {
                *(int16_t *)point_record = 3;
                if (chosen_object == -1) {
                    chosen_position = *(real_point3d *)&((ScenarioCommandPoint *)list->points.pointer)[chosen_point].position;
                    *(real_point3d *)(point_record + 4) = chosen_position;
                } else {
                    unit_get_primary_eye_marker_position();
                }
            } else {
                *(int16_t *)point_record = 1;
                *(float *)(point_record + 4) = chosen_delay;
            }
            {
                int32_t ticks = (int32_t)actor_begin_vocalization(0xd, sub_kind, point_record);
                state[2] = (uint8_t)ticks;
                state[3] = (uint8_t)(ticks >> 8);
            }
            return 1;
        }
        break;
    }
    case 5:
        if (aim_state != 0 && entry->atom_modifier >= 0 && entry->atom_modifier < 4) {
            *(int16_t *)aim_state = entry->atom_modifier;
            return 1;
        }
        break;
    case 6:
        if (aim_state != 0) {
            *aim_state = (entry->atom_modifier == 1);
            return 1;
        }
        break;
    case 7:
        if (aim_state != 0 && (int16_t)entry->point_1 >= 0 && (int16_t)entry->point_1 < (int32_t)list->points.count) {
            ScenarioCommandPoint *point = &((ScenarioCommandPoint *)list->points.pointer)[entry->point_1];

            aim_state[0x36] = 1;
            *(real_point3d *)(aim_state + 0x38) = *(real_point3d *)&point->position;
            *(float *)(aim_state + 0x44) = entry->parameter1;
            return 1;
        }
        break;
    case 8:
        if (aim_state != 0 && a->facing_unknown_180.i != -1.0f /* UNSURE: real gate is int16 at +0x180 */ &&
            (int16_t)entry->point_1 >= 0 && (int16_t)entry->point_1 < (int32_t)list->points.count) {
            ScenarioCommandPoint *point = &((ScenarioCommandPoint *)list->points.pointer)[entry->point_1];

            unit_set_grenade_type_and_count_delta(1);
            aim_state[0x49] = 0;
            aim_state[0x48] = 0;
            *(real_point3d *)(aim_state + 0x4c) = *(real_point3d *)&point->position;
            *(int16_t *)(aim_state + 0x4a) = 0;
            if (entry->atom_modifier >= 0 && entry->atom_modifier < 3) {
                *(int16_t *)(aim_state + 0x4a) = entry->atom_modifier;
            }
            state[2] = 0x3c;
            state[3] = 0;
            return 1;
        }
        break;
    case 9:
        if (check_object_index == a->unit_index) {
            struct { uint32_t magic_a; uint32_t magic_b; float best; uint32_t pad; int32_t index; } it;
            int32_t node;
            float samples[32];
            int16_t sample_count = 0;
            int16_t category = -1;

            it.magic_a = 0xffffffff;
            it.magic_b = 0x86868686;
            it.best = 2.8026e-45f;
            it.pad = 0;
            it.index = -1;
            node = object_iterator_next(&it);
            while (node != 0) {
                float prior_best = it.best;
                float distance;

                object_get_position();
                distance = FUN_00401020();
                if (entry->parameter1 == 0.0f || distance < entry->parameter1 * entry->parameter1) {
                    samples[sample_count * 2] = distance;
                    samples[sample_count * 2 + 1] = prior_best;
                    sample_count++;
                    if (sample_count > 0xf) break;
                }
                node = object_iterator_next(&it);
            }
            if (sample_count > 1) {
                _qsort(samples, sample_count, 8, float_compare_ascending);
            }
            if (entry->atom_modifier >= 0 && entry->atom_modifier < 5) {
                category = entry->atom_modifier;
            }
            {
                int16_t i;
                for (i = 0; i < sample_count; i++) {
                    if (actor_play_first_valid_vocalization(actor_index, &DAT_0065512c, &category, 0)) { // UNSURE: category passed by value originally
                        state[4] |= 4;
                        return 1;
                    }
                }
                if (sample_count > 0) {
                    return 0;
                }
            }
        }
        break;
    case 10:
        if (check_object_index == a->unit_index && a->active_unit_index != (datum_index)k_datum_index_none) {
            return 0;
        }
        state[5] = (state[5] & 0xe7) | 4;
        if (check_object_index == a->unit_index) {
            uint8_t take;

            if (a->unknown_504 != 0) {
                take = a->unknown_50a == 0;
            } else {
                take = 0;
            }
            if (!take) {
                take = 0;
            }
            state[2] = 0x3c;
            state[3] = 0;
            *(uint16_t *)(state + 8) = (take - 1) & 10;
            return 1;
        } else {
            object *obj = ((object_header *)object_data->data)[check_object_index & 0xffff].data;
            uint8_t take = 0;

            if (obj->parent_object == (datum_index)k_datum_index_none) {
                float dot = obj->forward.i * obj->velocity.i + obj->velocity.j * obj->forward.j + obj->velocity.k * obj->forward.k;
                take = dot > 0.06666667f;
            }
            state[2] = 0x3c;
            state[3] = 0;
            *(uint16_t *)(state + 8) = (take - 1) & 10;
            return 1;
        }
    case 0xb:
        if (check_object_index != a->unit_index || a->active_unit_index == (datum_index)k_datum_index_none) {
            state[5] = (state[5] & 0xf7) | 0x14;
            state[8] = 0;
            state[9] = 0;
            *(float *)(state + 0xc) = entry->parameter1;
            *(float *)(state + 0x10) = entry->parameter2;
            state[2] = 0x3c;
            state[3] = 0;
            return 1;
        }
        break;
    case 0xc:
        if ((int16_t)entry->script >= 0 && (int16_t)entry->script < *(int32_t *)((uint8_t *)global_scenario + 0x450)) {
            return hs_call_script_by_name();
        }
        break;
    case 0xd: {
        // UNSURE: local_f4's byte layout in the original is a decompiler artifact of a
        // 4-byte scratch reused as both a bool-ish flag and the animation selector; kept as
        // two plain locals here.
        uint8_t use_alternate = 0;
        uint8_t looping = 0;
        uint32_t animation_selector = 1;
        char started;

        if (entry->point_2 == 0xffff) {
            return 0;
        }
        switch (entry->atom_modifier) {
        case 1:
            break;
        case 2:
            looping = 1;
            use_alternate = 1;
            break;
        case 3:
            animation_selector = 0;
            break;
        case 4:
            animation_selector = 0;
            use_alternate = 1;
            break;
        case 5:
            animation_selector = 0;
            looping = 1;
            use_alternate = 1;
            break;
        default:
            break;
        }
        started = unit_start_user_animation(check_object_index, animation_selector);
        if (started == 0) {
            return 0;
        }
        {
            uint32_t *obj = (uint32_t *)object_try_and_get(1);
            if (obj != 0) {
                uint8_t *flags = (uint8_t *)obj + 0x4cc;
                uint32_t v = *(uint32_t *)flags;
                if (use_alternate) v |= 4; else v &= ~4u;
                *(uint32_t *)flags = v;
                if (!looping) {
                    *(uint32_t *)flags &= ~8u;
                    return 1;
                }
                *(uint32_t *)flags |= 8;
                return 1;
            }
        }
        // falls through when object_try_and_get returned 0, matching the original's
        // fallthrough into case 0x13 below
    }
    case 0x13:
        return 1;
    case 0xe:
        if ((int16_t)entry->recording >= 0 && (int16_t)entry->recording < *(int32_t *)((uint8_t *)global_scenario + 0x45c) &&
            recorded_animation_find_by_name() != -1) {
            return (char)FUN_0044a930(0);
        }
        break;
    case 0xf:
        if (aim_state == 0) {
            return 0;
        }
        aim_state[0x30] = 0;
        switch (entry->atom_modifier) {
        case 0: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 0; *(int16_t *)(aim_state + 0x34) = 0x2a; return (char)aim_state[0x30];
        case 1: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 4; *(int16_t *)(aim_state + 0x34) = 0x29; return (char)aim_state[0x30];
        case 2: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 5; *(int16_t *)(aim_state + 0x34) = 0x29; return (char)aim_state[0x30];
        case 3: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 6; *(int16_t *)(aim_state + 0x34) = -1; return (char)aim_state[0x30];
        case 4: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 7; *(int16_t *)(aim_state + 0x34) = -1; return (char)aim_state[0x30];
        case 5: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 8; *(int16_t *)(aim_state + 0x34) = 0x2c; return (char)aim_state[0x30];
        case 6: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 9; *(int16_t *)(aim_state + 0x34) = 0x2c; return (char)aim_state[0x30];
        case 7: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 10; *(int16_t *)(aim_state + 0x34) = 0x2c; return (char)aim_state[0x30];
        case 8: aim_state[0x30] = 1; *(int16_t *)(aim_state + 0x32) = 11; *(int16_t *)(aim_state + 0x34) = 0x2c; return (char)aim_state[0x30];
        case 9: *(int16_t *)(aim_state + 0x34) = 0x26; break;
        case 10: *(int16_t *)(aim_state + 0x34) = 0x27; break;
        default:
            return (char)aim_state[0x30];
        }
        *(int16_t *)(aim_state + 0x32) = -1;
        aim_state[0x30] = 1;
        return (char)aim_state[0x30];
    case 0x10: {
        uint16_t recording = (uint16_t)entry->atom_modifier;
        float duration = -1.0f;
        int16_t placed = unit_animation_change_priority_check(6, 1, 0, &recording, &duration);

        if (placed > 0) {
            uint8_t block[48];
            memset(block, 0, sizeof(block));
            *(float *)(block + 0x18) = duration;      // local_ac
            *(int16_t *)(block + 2) = recording;       // local_b0._2_2_
            *(int16_t *)block = 6;                      // local_b0._0_2_
            ai_communication_target_result_reset();
            unit_commit_speech();
            return 1;
        }
        break;
    }
    case 0x11:
        if (entry->atom_modifier != 0) {
            state[4] &= ~1;
            return 1;
        }
        state[4] |= 1;
        return 1;
    case 0x12:
        if (check_object_index == a->unit_index) {
            *(actor_base + 0x9e) = (entry->atom_modifier == 0); // UNSURE: raw mode_data-adjacent offset
            return 1;
        }
        break;
    case 0x14:
        if ((int16_t)entry->command >= 0 && (int16_t)entry->command < command_count &&
            entry->command != current_action_index) {
            return 1;
        }
        break;
    case 0x15: {
        object *obj = ((object_header *)object_data->data)[check_object_index & 0xffff].data;
        uint8_t *flags = (uint8_t *)obj + 0x106;
        if (entry->atom_modifier != 1) {
            *flags |= 0x20;
        } else {
            *flags |= 0x40;
        }
        return 1;
    }
    case 0x16: {
        int16_t direction = entry->atom_modifier;
        // UNSURE: see header note on the two __ftol-with-no-visible-operand cases; this one
        // follows actor_get_body_axis_vector, whose selected axis is not threaded through
        // here because the real request record is unknown.
        int32_t ticks;
        actor_axis_request dummy;

        if (direction < 0 || direction > 3) {
            state[8] = 0;
            state[9] = 0;
        } else {
            *(int16_t *)(state + 8) = direction;
        }
        dummy.axis = direction;
        actor_get_body_axis_vector(actor_index, check_object_index, &dummy);
        ticks = (int32_t)(dummy.result.i * 30.0f);
        state[2] = (uint8_t)ticks;
        state[3] = (uint8_t)(ticks >> 8);
        state[5] |= 3;
        return 1;
    }
    case 0x1a:
        if (aim_state != 0 && entry->parameter1 > 0.0f) {
            aim_state[0x28] = 1;
            *(float *)(aim_state + 0x2c) = entry->parameter1;
            return 1;
        }
        break;
    case 0x1b: {
        int16_t p1 = (int16_t)entry->point_1;
        real_point3d from_point;
        real_point3d to_point;
        real_vector3d delta;

        if (p1 < 0 || p1 >= (int32_t)list->points.count) {
            return 0;
        }
        from_point = *(real_point3d *)&((ScenarioCommandPoint *)list->points.pointer)[p1].position;
        unit_get_forward_vector_or_marker_normal();
        {
            int16_t p2 = (int16_t)entry->point_2;
            if (p2 >= 0 && p2 < (int32_t)list->points.count) {
                Actor *actor_def_unused;
                uint32_t *obj = (uint32_t *)object_try_and_get(1);
                uint8_t *variant_data = 0;

                to_point = *(real_point3d *)&((ScenarioCommandPoint *)list->points.pointer)[p2].position;
                if (obj != 0) {
                    variant_data = (uint8_t *)tag_instances[*obj & 0xffff].data;
                }
                delta.i = to_point.x - from_point.x;
                delta.j = to_point.y - from_point.y;
                delta.k = to_point.z - from_point.z;
                if (variant_data == 0 || (*(uint8_t *)(variant_data + 0x2f4) & 0x44) == 0) {
                    delta.k = 0.0f;
                    if (vector2d_normalize_with_length((real_vector2d *)&delta) == 0.0f) {
                        unit_get_forward_vector_or_marker_normal();
                    }
                } else {
                    if (vector3d_normalize_with_length(&delta) == 0.0f) {
                        unit_get_forward_vector_or_marker_normal();
                    }
                }
                (void)actor_def_unused;
            }
        }
        // 0x4065c0..0x4065cf: push 0 (up), push &[esp+0x20] (the normalized delta, the same
        // slot handed to unit_get_forward_vector_or_marker_normal just above), push esi (object index), with EDI still
        // pointing at from_point. The earlier draft passed from_point as the forward vector
        // and dropped both the up vector and the EDI position argument.
        object_set_position_and_orientation(check_object_index, &delta, (real_vector3d *)0, &from_point);
        object_reset_velocity_and_wake(check_object_index);
        object_recalculate_bounding_radius_recursive(check_object_index);
        if (check_object_index == a->unit_index) {
            actor_fill_unit_position_context(actor_base + 0x120);
            actor_movement_action_stop(actor_index);
        }
        return 1;
    }
    default:
        break;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x405520):

char actor_squad_action_execute(uint param_1,uint param_2,short param_3,byte *param_4)

{
  byte *pbVar1;
  int iVar2;
  byte bVar3;
  bool bVar4;
  char cVar5;
  undefined2 uVar6;
  short sVar7;
  int in_EAX;
  int iVar8;
  float *pfVar9;
  uint *puVar10;
  uint uVar11;
  short *psVar12;
  int iVar13;
  ushort uVar14;
  float *pfVar15;
  float fVar16;
  undefined4 uVar17;
  undefined4 *puVar18;
  bool bVar19;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float10 fVar20;
  char local_f7;
  int local_f4;
  float *local_f0;
  float local_ec;
  float local_e8;
  float local_e4;
  float local_e0;
  undefined4 local_dc;
  float local_d8;
  int local_d4;
  undefined2 local_d0 [2];
  float local_cc;
  undefined4 local_c8;
  undefined4 local_c4;
  uint local_c0;
  undefined2 local_bc;
  undefined4 local_b8;
  uint local_b4;
  undefined4 local_b0;
  float local_ac;
  float local_80 [32];
  
  iVar13 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_e8 = *(float *)((*(uint *)(iVar13 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar8 = *(int *)((*(uint *)(iVar13 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_d8 = *(float *)(param_3 * 0x60 + 0x30 + *(int *)(global_scenario + 0x43c));
  pfVar15 = (float *)(param_3 * 0x60 + *(int *)(global_scenario + 0x43c));
  local_ec = (float)(uint)*param_4;
  local_f7 = '\0';
  if ((int)local_d8 <= (int)local_ec) {
    return '\0';
  }
  psVar12 = (short *)((int)local_ec * 0x20 + (int)pfVar15[0xd]);
  local_f0 = pfVar15;
  switch(*psVar12) {
  case 0:
    uVar6 = __ftol();
    *(undefined2 *)(param_4 + 2) = uVar6;
    return '\x01';
  case 1:
  case 2:
    if (((in_EAX != 0) && (sVar7 = psVar12[6], -1 < sVar7)) && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      param_4[2] = 0;
      param_4[3] = 0;
      *(undefined1 *)(in_EAX + 4) = 1;
      *(bool *)(in_EAX + 5) = psVar12[1] == 1;
      *(undefined4 *)(in_EAX + 8) = *puVar18;
      *(undefined4 *)(in_EAX + 0xc) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x10) = puVar18[2];
      uVar17 = puVar18[3];
      *(undefined4 *)(in_EAX + 0x14) = uVar17;
      local_f7 = actor_movement_set_destination_point(param_1,uVar17,0xffffffff);
      if (local_f7 != '\0') {
        if (*(char *)(in_EAX + 5) != '\0') {
          actor_movement_action_cancel();
        }
        if (((*psVar12 == 2) && (-1 < psVar12[7])) &&
           (iVar8 = (int)psVar12[7], iVar8 < (int)pfVar15[0xf])) {
          fVar16 = pfVar15[0x10];
          *(undefined1 *)(in_EAX + 0x18) = 1;
          iVar13 = (int)fVar16 + iVar8 * 0x14;
          *(undefined4 *)(in_EAX + 0x1c) = *(undefined4 *)((int)fVar16 + iVar8 * 0x14);
          *(undefined4 *)(in_EAX + 0x20) = *(undefined4 *)(iVar13 + 4);
          *(undefined4 *)(in_EAX + 0x24) = *(undefined4 *)(iVar13 + 8);
          return local_f7;
        }
      }
    }
    break;
  case 3:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      pfVar9 = (float *)(param_4 + 0x18);
      *pfVar9 = *(float *)(iVar13 + 300);
      *(undefined4 *)(param_4 + 0x1c) = *(undefined4 *)(iVar13 + 0x130);
      *(undefined4 *)(param_4 + 0x20) = *(undefined4 *)(iVar13 + 0x134);
      local_f0 = pfVar9;
    }
    else {
      pfVar9 = (float *)object_get_position();
    }
    if ((psVar12[6] < 0) || (iVar8 = (int)psVar12[6], (int)pfVar15[0xf] <= iVar8)) {
      if (*(float *)(psVar12 + 4) < 0.0) {
        return '\0';
      }
      if (360.0 <= *(float *)(psVar12 + 4)) {
        return '\0';
      }
      fVar16 = *(float *)(psVar12 + 4);
      param_4[0x14] = 0;
      param_4[0x15] = 0;
      param_4[0x16] = 0;
      param_4[0x17] = 0;
      fVar20 = (float10)fcos((float10)fVar16 * (float10)0.017453292);
      *(float *)(param_4 + 0xc) = (float)fVar20;
      fVar20 = (float10)fsin((float10)fVar16 * (float10)0.017453292);
      *(float *)(param_4 + 0x10) = (float)fVar20;
    }
    else {
      iVar2 = (int)pfVar15[0x10] + iVar8 * 0x14;
      *(float *)(param_4 + 0xc) = *(float *)((int)pfVar15[0x10] + iVar8 * 0x14) - *pfVar9;
      *(float *)(param_4 + 0x10) = *(float *)(iVar2 + 4) - pfVar9[1];
      *(float *)(param_4 + 0x14) = *(float *)(iVar2 + 8) - pfVar9[2];
      fVar20 = (float10)vector3d_normalize_with_length();
      if (fVar20 <= (float10)0.0) {
        return '\0';
      }
    }
    sVar7 = psVar12[1];
    if ((sVar7 < 0) || (3 < sVar7)) {
      param_4[8] = 0xff;
      param_4[9] = 0xff;
    }
    else {
      *(short *)(param_4 + 8) = sVar7;
    }
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      actor_movement_action_stop();
    }
    param_4[5] = param_4[5] & 0xfd | 1;
    return '\x01';
  case 4:
  case 0x17:
  case 0x18:
  case 0x19:
    if (in_EAX == 0) {
      return '\0';
    }
    local_f4._0_2_ = -1;
    local_d8 = -NAN;
    local_d4 = -1;
    local_ec = *(float *)(psVar12 + 2);
    sVar7 = *psVar12;
    if (sVar7 == 4) {
      sVar7 = psVar12[6];
      if ((-1 < sVar7) && ((int)sVar7 < (int)pfVar15[0xf])) {
        local_ec = *(float *)(psVar12 + 2);
        local_f4._0_2_ = sVar7;
      }
    }
    else if (sVar7 == 0x17) {
      if ((-1 < psVar12[6]) && ((int)psVar12[6] < (int)pfVar15[0xf])) {
        uVar14 = psVar12[7];
        if ((-1 < (short)uVar14) && ((int)(short)uVar14 < (int)pfVar15[0xf])) {
          local_f4._0_2_ = random_int_range(uVar14 + 1);
          if ((*(float *)(psVar12 + 2) == 0.0) && (*(float *)(psVar12 + 4) == 0.0)) {
            local_ec = random_real_range(*(float *)((int)local_e8 + 0xec),
                                         *(float *)((int)local_e8 + 0xf0));
          }
          else {
            local_ec = random_real_range(*(float *)(psVar12 + 2),*(float *)(psVar12 + 4));
          }
        }
      }
    }
    else if (sVar7 == 0x18) {
      FUN_0043ecd0(&local_e8);
      iVar8 = FUN_0043ecf0();
      fVar16 = local_d8;
      fVar20 = extraout_ST0;
      if (iVar8 != 0) {
        do {
          if (((1 < *(short *)(iVar8 + 0x24)) && (*(short *)(iVar8 + 0x24) < 4)) &&
             ((*(char *)(iVar8 + 0x12e) != '\0' && ((float10)*(float *)(iVar8 + 0x11c) < fVar20))))
          {
            fVar16 = local_e8;
          }
          iVar8 = FUN_0043ecf0();
          fVar20 = extraout_ST0_00;
        } while (iVar8 != 0);
        local_d8 = fVar16;
        if (fVar16 != -NAN) goto LAB_00405961;
      }
      local_c0 = DAT_0087a480;
      local_b4 = DAT_0087a480 ^ 0x69746572;
      local_e8 = 3.4028235e+38;
      local_bc = 0;
      local_b8 = 0xffffffff;
      iVar8 = data_iterator_next();
      while (iVar8 != 0) {
        if (*(int *)(iVar8 + 0x34) != -1) {
          FUN_00568f50();
          fVar20 = (float10)FUN_00401020();
          pfVar15 = local_f0;
          if (fVar20 < (float10)local_e8) {
            local_d4 = *(int *)(iVar8 + 0x34);
            local_e8 = (float)fVar20;
          }
        }
        iVar8 = data_iterator_next();
      }
    }
    else if (((sVar7 == 0x19) && (-1 < psVar12[0xc])) &&
            ((int)psVar12[0xc] < *(int *)(global_scenario + 0x204))) {
      iVar8 = FUN_004f73c0();
      iVar13 = object_try_and_get(3);
      if (iVar13 != 0) {
        local_d8 = (float)FUN_0043ea80(iVar8);
        local_d4 = iVar8;
      }
    }
LAB_00405961:
    if ((0.0 < local_ec) &&
       (((local_d8 != -NAN || (local_d4 != -1)) ||
        ((-1 < (short)local_f4 && ((int)(short)local_f4 < (int)pfVar15[0xf])))))) {
      sVar7 = psVar12[1];
      uVar17 = 1;
      if (sVar7 == 1) {
        uVar17 = 5;
      }
      else if (sVar7 == 2) {
        uVar17 = 2;
      }
      else if (sVar7 == 4) {
        uVar17 = 7;
      }
      else if (sVar7 == 3) {
        uVar17 = 8;
      }
      if (local_d8 == -NAN) {
        local_d0[0] = 3;
        if (local_d4 == -1) {
          local_cc = *(float *)((int)pfVar15[0x10] + (short)local_f4 * 0x14);
          iVar8 = (int)pfVar15[0x10] + (short)local_f4 * 0x14;
          local_c8 = *(undefined4 *)(iVar8 + 4);
          local_c4 = *(undefined4 *)(iVar8 + 8);
        }
        else {
          FUN_00568f50();
        }
      }
      else {
        local_d0[0] = 1;
        local_cc = local_d8;
      }
      FUN_004142d0(0xd,uVar17,local_d0);
      uVar6 = __ftol();
      *(undefined2 *)(param_4 + 2) = uVar6;
      return '\x01';
    }
    break;
  case 5:
    if (((in_EAX != 0) && (sVar7 = psVar12[1], -1 < sVar7)) && (sVar7 < 4)) {
      *(short *)(in_EAX + 2) = sVar7;
      return '\x01';
    }
    break;
  case 6:
    if (in_EAX != 0) {
      *(bool *)in_EAX = psVar12[1] == 1;
      return '\x01';
    }
    break;
  case 7:
    if (((in_EAX != 0) && (sVar7 = psVar12[6], -1 < sVar7)) && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      *(undefined1 *)(in_EAX + 0x36) = 1;
      *(undefined4 *)(in_EAX + 0x38) = *puVar18;
      *(undefined4 *)(in_EAX + 0x3c) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x40) = puVar18[2];
      *(undefined4 *)(in_EAX + 0x44) = *(undefined4 *)(psVar12 + 2);
      return '\x01';
    }
    break;
  case 8:
    if ((((in_EAX != 0) && (*(short *)(iVar8 + 0x180) != -1)) && (sVar7 = psVar12[6], -1 < sVar7))
       && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      FUN_0056d160(1);
      *(undefined1 *)(in_EAX + 0x49) = 0;
      *(undefined1 *)(in_EAX + 0x48) = 0;
      *(undefined4 *)(in_EAX + 0x4c) = *puVar18;
      *(undefined4 *)(in_EAX + 0x50) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x54) = puVar18[2];
      *(undefined2 *)(in_EAX + 0x4a) = 0;
      sVar7 = psVar12[1];
      if ((-1 < sVar7) && (sVar7 < 3)) {
        *(short *)(in_EAX + 0x4a) = sVar7;
      }
      param_4[2] = 0x3c;
      param_4[3] = 0;
      return '\x01';
    }
    break;
  case 9:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      uVar14 = 0;
      local_f0 = (float *)0xffffffff;
      local_dc = 0x86868686;
      local_e8 = 2.8026e-45;
      local_e4 = (float)(((uint)local_e4 >> 8 & 0xff) << 8);
      local_e0 = -NAN;
      iVar8 = object_iterator_next(&local_e8);
      if (iVar8 != 0) {
        do {
          fVar16 = local_e0;
          object_get_position();
          fVar20 = (float10)FUN_00401020();
          if ((*(float *)(psVar12 + 2) == 0.0) ||
             (fVar20 < (float10)*(float *)(psVar12 + 2) * (float10)*(float *)(psVar12 + 2))) {
            iVar8 = (int)(short)uVar14;
            uVar14 = uVar14 + 1;
            local_80[iVar8 * 2] = (float)fVar20;
            local_80[iVar8 * 2 + 1] = fVar16;
            if (0xf < uVar14) break;
          }
          iVar8 = object_iterator_next(&local_e8);
        } while (iVar8 != 0);
        if (1 < (short)uVar14) {
          _qsort(local_80,(int)(short)uVar14,8,float_compare_ascending);
        }
      }
      sVar7 = psVar12[1];
      if ((-1 < sVar7) && (sVar7 < 5)) {
        local_f0 = (float *)(int)sVar7;
      }
      pfVar15 = local_f0;
      sVar7 = 0;
      if (0 < (short)uVar14) {
        do {
          cVar5 = FUN_0040e260(param_1,&DAT_0065512c,pfVar15,0);
          if (cVar5 != '\0') {
            param_4[4] = param_4[4] | 4;
            return '\x01';
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < (short)uVar14);
        return '\0';
      }
    }
    break;
  case 10:
    if ((param_2 == *(uint *)(iVar13 + 0x18)) && (*(int *)(iVar13 + 0x158) != -1)) {
      return '\0';
    }
    param_4[5] = param_4[5] & 0xe7 | 4;
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      if (*(char *)(iVar13 + 0x504) != '\0') {
        bVar19 = *(short *)(iVar13 + 0x50a) == 0;
LAB_00405c91:
        if (bVar19) {
          bVar3 = 1;
          goto LAB_00405c99;
        }
      }
    }
    else {
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
      if (*(int *)(iVar8 + 0x11c) == -1) {
        bVar19 = 0.06666667 <
                 *(float *)(iVar8 + 0x74) * *(float *)(iVar8 + 0x68) +
                 *(float *)(iVar8 + 0x6c) * *(float *)(iVar8 + 0x78) +
                 *(float *)(iVar8 + 0x70) * *(float *)(iVar8 + 0x7c);
        goto LAB_00405c91;
      }
    }
    bVar3 = 0;
LAB_00405c99:
    param_4[2] = 0x3c;
    param_4[3] = 0;
    *(ushort *)(param_4 + 8) = bVar3 - 1 & 10;
    return '\x01';
  case 0xb:
    if ((param_2 != *(uint *)(iVar13 + 0x18)) || (*(int *)(iVar13 + 0x158) == -1)) {
      param_4[5] = param_4[5] & 0xf7 | 0x14;
      param_4[8] = 0;
      param_4[9] = 0;
      *(undefined4 *)(param_4 + 0xc) = *(undefined4 *)(psVar12 + 2);
      *(undefined4 *)(param_4 + 0x10) = *(undefined4 *)(psVar12 + 4);
      param_4[2] = 0x3c;
      param_4[3] = 0;
      return '\x01';
    }
    break;
  case 0xc:
    if ((-1 < psVar12[9]) && ((int)psVar12[9] < *(int *)(global_scenario + 0x450))) {
      cVar5 = hs_call_script_by_name();
      return cVar5;
    }
    break;
  case 0xd:
    if (psVar12[8] == -1) {
      return '\0';
    }
    bVar19 = false;
    bVar4 = false;
    local_f4._1_3_ = (uint3)((uint)iVar8 >> 8);
    local_f4 = CONCAT31(local_f4._1_3_,1);
    switch(psVar12[1]) {
    case 1:
      break;
    case 2:
      goto switchD_00406262_caseD_2;
    case 3:
      local_f4 = (uint)local_f4._1_3_ << 8;
      goto switchD_00406262_default;
    case 4:
      local_f4 = (uint)local_f4._1_3_ << 8;
      break;
    case 5:
      local_f4 = (uint)local_f4._1_3_ << 8;
switchD_00406262_caseD_2:
      bVar4 = true;
      break;
    default:
      goto switchD_00406262_default;
    }
    bVar19 = true;
switchD_00406262_default:
    cVar5 = unit_start_user_animation(param_2,local_f4);
    if (cVar5 == '\0') {
      return '\0';
    }
    iVar8 = object_try_and_get(1);
    if (iVar8 != 0) {
      if (bVar19) {
        uVar11 = *(uint *)(iVar8 + 0x4cc) | 4;
      }
      else {
        uVar11 = *(uint *)(iVar8 + 0x4cc) & 0xfffffffb;
      }
      *(uint *)(iVar8 + 0x4cc) = uVar11;
      if (!bVar4) {
        *(uint *)(iVar8 + 0x4cc) = *(uint *)(iVar8 + 0x4cc) & 0xfffffff7;
        return '\x01';
      }
      *(uint *)(iVar8 + 0x4cc) = *(uint *)(iVar8 + 0x4cc) | 8;
      return '\x01';
    }
  case 0x13:
switchD_004055d3_caseD_13:
    local_f7 = '\x01';
    break;
  case 0xe:
    if (((-1 < psVar12[10]) && ((int)psVar12[10] < *(int *)(global_scenario + 0x45c))) &&
       (sVar7 = FUN_00449f80(), sVar7 != -1)) {
      cVar5 = FUN_0044a930(0);
      return cVar5;
    }
    break;
  case 0xf:
    if (in_EAX == 0) {
      return '\0';
    }
    *(undefined1 *)(in_EAX + 0x30) = 0;
    switch(psVar12[1]) {
    case 0:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 0;
      *(undefined2 *)(in_EAX + 0x34) = 0x2a;
      return *(char *)(in_EAX + 0x30);
    case 1:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 4;
      *(undefined2 *)(in_EAX + 0x34) = 0x29;
      return *(char *)(in_EAX + 0x30);
    case 2:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 5;
      *(undefined2 *)(in_EAX + 0x34) = 0x29;
      return *(char *)(in_EAX + 0x30);
    case 3:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 6;
      *(undefined2 *)(in_EAX + 0x34) = 0xffff;
      return *(char *)(in_EAX + 0x30);
    case 4:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 7;
      *(undefined2 *)(in_EAX + 0x34) = 0xffff;
      return *(char *)(in_EAX + 0x30);
    case 5:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 8;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 6:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 9;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 7:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 10;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 8:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 0xb;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 9:
      *(undefined2 *)(in_EAX + 0x34) = 0x26;
      break;
    case 10:
      *(undefined2 *)(in_EAX + 0x34) = 0x27;
      break;
    default:
      goto switchD_00405e1c_default;
    }
    *(undefined2 *)(in_EAX + 0x32) = 0xffff;
    *(undefined1 *)(in_EAX + 0x30) = 1;
switchD_00405e1c_default:
    return *(char *)(in_EAX + 0x30);
  case 0x10:
    local_f0 = (float *)(uint)(ushort)psVar12[1];
    local_e8 = -NAN;
    sVar7 = FUN_00560d00(6,1,0,&local_f0,&local_e8);
    if (0 < sVar7) {
      puVar18 = &local_b0;
      for (iVar8 = 0xc; iVar8 != 0; iVar8 = iVar8 + -1) {
        *puVar18 = 0;
        puVar18 = puVar18 + 1;
      }
      local_ac = local_e8;
      local_b0._2_2_ = local_f0._0_2_;
      local_b0._0_2_ = 6;
      FUN_0042d310();
      FUN_00560f20();
      return '\x01';
    }
    break;
  case 0x11:
    if (psVar12[1] != 0) {
      param_4[4] = param_4[4] & 0xfe;
      return '\x01';
    }
    param_4[4] = param_4[4] | 1;
    return '\x01';
  case 0x12:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      *(bool *)(iVar13 + 0x9e) = psVar12[1] == 0;
      return '\x01';
    }
    break;
  case 0x14:
    sVar7 = psVar12[0xb];
    if (((-1 < sVar7) && ((int)sVar7 < (int)local_d8)) && ((float)(int)sVar7 != local_ec)) {
      return '\x01';
    }
    break;
  case 0x15:
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    if (psVar12[1] != 1) {
      pbVar1 = (byte *)(iVar8 + 0x106);
      *pbVar1 = *pbVar1 | 0x20;
      return '\x01';
    }
    pbVar1 = (byte *)(iVar8 + 0x106);
    *pbVar1 = *pbVar1 | 0x40;
    return '\x01';
  case 0x16:
    sVar7 = psVar12[1];
    if ((sVar7 < 0) || (3 < sVar7)) {
      param_4[8] = 0;
      param_4[9] = 0;
    }
    else {
      *(short *)(param_4 + 8) = sVar7;
    }
    FUN_00405390();
    uVar6 = __ftol();
    *(undefined2 *)(param_4 + 2) = uVar6;
    param_4[5] = param_4[5] | 3;
    return '\x01';
  case 0x1a:
    if ((in_EAX != 0) && (0.0 < *(float *)(psVar12 + 2))) {
      *(undefined1 *)(in_EAX + 0x28) = 1;
      *(undefined4 *)(in_EAX + 0x2c) = *(undefined4 *)(psVar12 + 2);
      return '\x01';
    }
    break;
  case 0x1b:
    sVar7 = psVar12[6];
    if (sVar7 < 0) {
      return '\0';
    }
    if ((int)pfVar15[0xf] <= (int)sVar7) {
      return '\0';
    }
    pfVar9 = (float *)((int)pfVar15[0x10] + sVar7 * 0x14);
    FUN_00569720();
    sVar7 = psVar12[7];
    if ((-1 < sVar7) && ((int)sVar7 < (int)pfVar15[0xf])) {
      pfVar15 = (float *)((int)pfVar15[0x10] + sVar7 * 0x14);
      puVar10 = (uint *)object_try_and_get(1);
      iVar8 = 0;
      if (puVar10 != (uint *)0x0) {
        iVar8 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      }
      local_e8 = *pfVar15 - *pfVar9;
      local_e4 = pfVar15[1] - pfVar9[1];
      local_e0 = pfVar15[2] - pfVar9[2];
      if ((iVar8 == 0) || ((*(byte *)(iVar8 + 0x2f4) & 0x44) == 0)) {
        local_e0 = 0.0;
        fVar20 = (float10)vector2d_normalize_with_length();
      }
      else {
        fVar20 = (float10)vector3d_normalize_with_length();
      }
      if ((float10)0.0 == fVar20) {
        FUN_00569720();
      }
    }
    object_set_position_and_orientation(param_2,&local_e8,0);
    object_reset_velocity_and_wake(param_2);
    object_recalculate_bounding_radius_recursive(param_2);
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      FUN_004296c0(iVar13 + 0x120);
      actor_movement_action_stop();
    }
    goto switchD_004055d3_caseD_13;
  }
  return local_f7;
}

#endif
