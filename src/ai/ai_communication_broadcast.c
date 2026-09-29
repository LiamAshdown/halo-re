// ai_communication_broadcast  (Ghidra: already named)
// address 0x42d340, size 5624 bytes -- the largest function in the ai module (0x42d340..0x42e937, jump tables after).
// name confidence: 0.55   rewrite confidence: 0.85
// REWRITTEN from objdump 0x42d340..0x42e937 (the earlier draft's front half did not follow the binary and most of its
//   calls used invented operand lists). Stack: (event, unit, other unit, reason, kind, tag, extra[2]). Resolves both
//   units (team, class mask or actor type flags, actor, encounter, the two capability bytes), runs the team-pair
//   recognition for event 0 between related teams (0x4300d0 speaker search, 0x45bfc0 counter, 0x42ba80 marking,
//   0x45bd50 enemies -> reason 4), builds six combat gates from the speaker actor / encounter, the reason buckets and
//   the per-side recency table (ai_globals +0x14..+0x28 against the ten 0x655950 rows). Then every line row of the
//   event (0x8802e0 -> 0x655aa0, 0x28 each) is filtered (bucket, quiet period 0x725204, gate, class masks, kind),
//   given a speaker (selector 0 unit, 1 other, 2 encounter / team search, 4 team search), checked (live, not a
//   vehicle, a player or actor or line flag 8), rated by player proximity (0x4303f0) and line history (0x6f0c9c),
//   timed, given a look target and follow-up, checked for priority (0x560d00) and gesture (0x569470), and scored.
//   One of up to 16 is drawn by weight (broadcast lines only if any), then either a scripted object propagates and
//   plays it (0x42e9c0 / 0x42eee0) or the speaker commits it as speech (0x560f20, tail 24 ticks), gestures toward the
//   addressed object (0x569530), orders its follow-up (0x4302e0 line 9) and records it (0x42f9e0).
// blam-cc: stack -> event_code, unit_index, object_a, reason, object_b, object_c, extra_data

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include "fn_ai.h"
#include <string.h>

extern game_time_globals *game_time;   // 0x006f1d6c
extern data_array *object_data;        // 0x008603b0
extern data_array *actor_data;         // 0x00880360
extern data_array *encounter_data;     // 0x008802c8
extern uint8_t *actor_type_procs[]; // 0x006853b8, +0x4 the type flags word
extern uint8_t *team_pair_data; // 0x006b0b84, +0x94 the 10x10 team bit matrix
extern uint8_t *ai_globals_ptr;        // 0x00880354
extern int16_t conversation_index_lookup[]; // 0x008802e0, first line row per event, -1 none
extern uint8_t ai_communication_lines[];       // 0x00655aa0, 0x28-byte rows
extern float ai_communication_direction_table[]; // 0x00655950, 10 rows of 5 floats
extern int16_t ai_communication_class_priority[]; // 0x006558c4
extern float ai_communication_class_tail_seconds[]; // 0x006558d4
extern int16_t ai_communication_class_follow_up[]; // 0x006558f4
extern int16_t ai_communication_class_look_marker[]; // 0x00655904
extern int16_t ai_communication_class_no_actor_class[]; // 0x00655914
extern float ai_communication_selector_delay_seconds[]; // 0x00655a68, by participant selector
extern uint8_t *communication_line_base; // 0x006f0c9c, 8-byte records per (row, side)
extern uint32_t random_seed_global;    // 0x00719cd0
extern int32_t ai_communication_quiet_until_tick; // 0x00725204
// FIXED 2026-09-27 (static loop, scratchpad/equcheck.py): this file declared the table at 0x0065524c with the
// combat grade at +0xc, but the linker binds actor_mode_definitions to 0x00655254 (16 other files), so the
// read landed on process_proc's low word. Binary: [mode * 0x38 + 0x655258] == definitions[mode].combat_grade.
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254
extern char ai_marker_name_a[];      // 0x0066bfa0

extern double sqrt(double x);
extern double fabs(double x);

extern datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a,
    datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class,
    uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team); // 0x4300d0, stack + DI
extern datum_index ai_communication_select_speaker_in_reference(float radius, int16_t allow_unreachable,
    uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags,
    uint32_t reference, datum_index object_a, datum_index object_b); // 0x42ff80, stack + EAX, EDI, EBX
extern uint32_t team_pair_override_adjust_counter(int16_t index_a, int16_t index_b, int16_t delta_selector,
    uint8_t *out_flag); // 0x45bfc0, EAX + stack

extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX

extern float ai_communication_rate_player_proximity(uint8_t require_line_of_sight,
    datum_index *out_player_object_index, float *out_distance, datum_index object_index); // 0x4303f0, EBX
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command); // 0x569470, EAX, ECX


extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20, EAX, ECX, DX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum_markers); // 0x4f6080
extern uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command,
    const real_vector2d *direction); // 0x569530


#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define ACTOR_DATA(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

// one scored line, 0x38 bytes at [esp+0x15c + i * 0x38]
typedef struct broadcast_candidate {
    float score;                // +0x00
    uint8_t broadcast;          // +0x04 line flag 2
    uint8_t no_actor_speaker;   // +0x05
    int16_t dialogue_index;     // +0x06
    int16_t priority;           // +0x08 class priority (0x6558c4)
    int16_t animation;          // +0x0a line +0x6
    int16_t check_result;       // +0x0c unit_animation_change_priority_check
    int16_t delay_ticks;        // +0x0e
    int16_t lipsync_ticks;      // +0x10
    datum_index speaker_unit;   // +0x14
    datum_index speaker_actor;  // +0x18
    datum_index other_object;   // +0x1c
    datum_index target;         // +0x20 ai_select_communication_target
    int16_t follow_up;          // +0x24
    int16_t look_marker;        // +0x26
    int16_t look_kind;          // +0x28
    datum_index look_object;    // +0x2c
    int32_t chain;              // +0x30
    int16_t row;                // +0x34
} broadcast_candidate;

static uint8_t broadcast_line_flags(uint8_t line_flags)
{
    uint8_t flags = (uint8_t)((line_flags & 1) | 2);

    if (line_flags & 0x10) {
        flags |= 4;
    }
    if (line_flags & 0x20) {
        flags |= 8;
    }
    return flags;
}

static uint32_t broadcast_team_class(int16_t team)
{
    switch (team) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 4;
    case 4: return 0x38;
    case 5: return 0x40;
    default: return 0;
    }
}

// blam-cc: stack -> event_code, unit_index, object_a, reason, object_b, object_c, extra_data
void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a,
                                 int32_t reason, datum_index object_b, datum_index object_c,
                                 uint32_t *extra_data)
{
    int32_t now = game_time->game_time;                 // [esp+0x9c]
    int16_t event = (int16_t)event_code;
    uint8_t *unit = 0;                                  // edx
    uint8_t *other = 0;                                 // esi
    uint8_t *unit_actor = 0;                            // [esp+0xc8] (ebp)
    uint8_t *other_actor = 0;                           // [esp+0xcc]
    uint8_t *unit_encounter = 0;                        // [esp+0x20]
    datum_index unit_actor_index = k_datum_index_none;  // [esp+0xa0]
    datum_index other_actor_index = k_datum_index_none; // [esp+0xd4]
    datum_index unit_encounter_index = k_datum_index_none; // [esp+0x90]
    datum_index cached_speaker = k_datum_index_none;    // [esp+0x80]
    datum_index cached_team_speaker = k_datum_index_none; // [esp+0xd0]
    uint32_t unit_team = 0xffffffff;                    // [esp+0x4c] (low word the team)
    uint32_t other_team_or_class = 0xffffffff;          // [esp+0x14]
    uint32_t unit_class = 0;                            // [esp+0x78]
    uint32_t other_class = 0;                           // [esp+0x84]
    uint8_t may_search = 1;                             // [esp+0x13]
    uint8_t may_search_team = 1;                        // [esp+0x37]
    uint8_t unit_capability[2] = {0, 0};                // [esp+0x3c]
    uint8_t other_capability[2] = {0, 0};               // [esp+0x38]
    uint8_t gates[6];                                   // [esp+0x18]
    uint8_t buckets[16];                                // [esp+0x94] (5 used)
    uint8_t recent[2][16];                              // [esp+0xa4]
    int16_t recent_ticks[2][16];                        // [esp+0xf0]
    broadcast_candidate candidates[16];                 // [esp+0x15c]
    int16_t count = 0;                                  // [esp+0x6c]
    float total = 0.0f;                                 // [esp+0x74]
    uint8_t any_broadcast = 0;                          // [esp+0x2b]
    datum_index look_object = 4;                       // [esp+0x5c], not reset per line (4 after the side loop)
    int32_t side;
    int32_t row_index;
    uint8_t *row;

    if ((int16_t)reason == -1) {
        reason = 0;
    }
    if ((int16_t)object_b == -1) {
        object_b = 0;
    }

    // 0x42d3e5: the speaking unit
    if (unit_index != k_datum_index_none) {
        unit = OBJECT_DATA(unit_index);
        unit_team = (unit_team & 0xffff0000u) | *(uint16_t *)&((unit_object *)unit)->base.owner_team;
        unit_actor_index = ((unit_object *)unit)->unit.actor_index;
        unit_class = broadcast_team_class((int16_t)unit_team);
        if (unit_actor_index != k_datum_index_none) {
            unit_actor = ACTOR_DATA(unit_actor_index);
            unit_class = (unit_class & 0xffff0000u) |
                         *(uint16_t *)(actor_type_procs[*(int16_t *)(unit_actor + 0x4)] + 0x4);
            unit_encounter_index = *(datum_index *)(unit_actor + 0x34);
            if (*(int8_t *)(unit_actor + 0x245) > 0) {
                unit_capability[1] = 1;
                unit_capability[0] = 1;
            } else if (*(int8_t *)(unit_actor + 0x200) > 0) {
                unit_capability[1] = 0;
                unit_capability[0] = 1;
            }
            if (unit_encounter_index != k_datum_index_none) {
                unit_encounter = (uint8_t *)encounter_data->data + (unit_encounter_index & 0xffff) * 0x6c;
            }
        } else if (((unit_object *)unit)->unit.controlling_player != k_datum_index_none) {
            unit_class = 1;
        }
    }
    // 0x42d505: the other unit
    if (object_a != k_datum_index_none) {
        other = OBJECT_DATA(object_a);
        other_actor_index = *(datum_index *)(other + 0x1f4);
        other_team_or_class = *(uint16_t *)(other + 0xb8);
        other_class = broadcast_team_class((int16_t)other_team_or_class);
        if (other_actor_index != k_datum_index_none) {
            other_actor = ACTOR_DATA(other_actor_index);
            other_class = (other_class & 0xffff0000u) |
                          *(uint16_t *)(actor_type_procs[*(int16_t *)(other_actor + 0x4)] + 0x4);
            if (*(int8_t *)(other_actor + 0x245) > 0) {
                other_capability[0] = 1;
                other_capability[1] = 1;
            } else if (*(int8_t *)(other_actor + 0x200) > 0) {
                other_capability[0] = 1;
                other_capability[1] = 0;
            }
        } else if (*(datum_index *)(other + 0x218) != k_datum_index_none) {
            other_class = 1;
        }
    }

    // 0x42d617: two units of different, related teams
    if (unit != 0 && other != 0) {
        int16_t team_u = (int16_t)unit_team;
        int16_t team_o = (int16_t)other_team_or_class;

        if (team_u != team_o && team_u >= 0 && team_u < 10 && team_o >= 0 && team_o < 10) {
            int32_t bit = team_u * 10 + team_o;

            if (*(uint32_t *)(team_pair_data + 0x94 + (bit >> 5) * 4) & (1u << (bit & 0x1f))) {
                uint8_t react = 0; // [esp+0x12]
                uint8_t hostile = 0; // bl

                if (event == 0) {
                    if ((int16_t)reason == 3) {
                        hostile = 1;
                        react = 1;
                        reason = 4;
                    } else {
                        if (unit_encounter != 0) {
                            int32_t since = *(int32_t *)(unit_encounter + 0x50);

                            hostile = (uint8_t)!(unit_encounter[0x46] == 0 && since != -1 && since < 0x10e);
                        }
                        cached_speaker = ai_communication_select_speaker_by_team(0, unit_index, object_a, 18.0f, 0, 6,
                                                                                 0xffffffff, 0xffffffff, -1, 0,
                                                                                 (int16_t)unit_team);
                        if (cached_speaker != k_datum_index_none) {
                            may_search = 0;
                            react = 1;
                        }
                        {
                            int32_t kind = (int16_t)object_b;

                            if (kind >= 3 && (kind <= 4 || kind == 9) && !hostile) {
                                react = 0;
                            }
                            if (kind == 3) {
                                hostile = 0;
                            } else if (hostile) {
                                reason = 4;
                            }
                        }
                    }
                    if (react) {
                        uint8_t mark = 0;
                        uint8_t status = (uint8_t)team_pair_override_adjust_counter(team_o, team_u, hostile != 0,
                                                                                    &mark);

                        if (mark) {
                            ai_mark_recognized_objects_for_reaction(team_o, team_u, status);
                        }
                    }
                }
                if (teams_are_enemies(team_o, team_u)) {
                    reason = 4;
                }
            }
        }
    }

    // 0x42d7c8: the speaker's combat gates
    if (unit_actor == 0) {
        memset(gates, 1, sizeof(gates));
    } else if (unit_encounter == 0) {
        uint8_t alerted = unit_actor[0x27c];
        int32_t since = *(int32_t *)(unit_actor + 0x278);
        int16_t grade = *(int16_t *)(unit_actor + 0x6e);

        gates[0] = (uint8_t)(unit_actor[0x274] == 0);
        gates[1] = (uint8_t)(alerted == 0 && since != -1);
        gates[2] = (uint8_t)!(*(datum_index *)(unit_actor + 0x270) != k_datum_index_none && since != -1 && since < 0xb4);
        gates[3] = (uint8_t)(grade < 3 && !(since != -1 && since < 0x4b) && (alerted != 0 || grade > 0));
        gates[4] = (uint8_t)(grade < 6);
        gates[5] = (uint8_t)(*(int16_t *)(unit_actor + 0x268) >= 10 && alerted != 0);
    } else {
        int32_t since = *(int32_t *)(unit_encounter + 0x50);
        uint8_t engaged = unit_encounter[0x44];
        int16_t grade = *(int16_t *)(unit_actor + 0x6e);

        gates[0] = (uint8_t)(unit_actor[0x274] == 0);
        gates[1] = (uint8_t)(since != -1 && engaged == 0);
        gates[2] = (uint8_t)(!(since != -1 && since < 0xb4) && engaged != 0);
        gates[3] = (uint8_t)(grade < 3 && !(since != -1 && since < 0x4b) && (engaged != 0 || grade > 0));
        gates[4] = (uint8_t)(grade < 6 && !(since != -1 && since < 0x4b));
        gates[5] = (uint8_t)(unit_encounter[0x45] != 0 && engaged != 0);
    }

    // 0x42d930: the reason buckets
    memset(buckets, 0, sizeof(buckets));
    if ((int16_t)reason != -1 && (uint16_t)reason < 16) {
        buckets[(int16_t)reason] = 1;
        if ((int16_t)reason == 4) {
            buckets[3] = 1;
        }
    }

    // 0x42d966: how recently each side spoke, against the ten direction rows
    memset(recent, 0, sizeof(recent));
    memset(recent_ticks, 0, sizeof(recent_ticks));
    for (side = 0; side < 2; side++) {
        int32_t *ticks = (int32_t *)(ai_globals_ptr + 0x1c) + side;
        int32_t gap_far = now - ticks[2];
        int32_t gap_mid = now - ticks[0];
        int32_t gap_near = now - ticks[-2];
        int32_t r;

        if (gap_far < 0) gap_far = 0;
        if (gap_mid < 0) gap_mid = 0;
        if (gap_near < 0) gap_near = 0;
        recent[side][0] = 1;
        recent[side][1] = 1;
        for (r = 0; r < 10; r++) {
            float *columns = ai_communication_direction_table + r * 5;
            uint8_t flag = 0;
            int16_t value = 0;

            if (columns[0] > 0.0f) {
                int16_t v = (int16_t)(int32_t)(columns[0] * 30.0f - (float)(int16_t)gap_near);

                if (v > 0) {
                    flag = 1;
                    value = v;
                }
            }
            if (columns[1] > 0.0f) {
                int16_t v = (int16_t)(int32_t)(columns[1] * 30.0f - (float)(int16_t)gap_mid);

                if (v > 0) {
                    flag = 1;
                    if (!(value > v)) {
                        value = v;
                    }
                }
            }
            {
                uint8_t check_last = 0;

                if (columns[3] > 0.0f) {
                    int16_t v = (int16_t)(int32_t)(columns[3] * 30.0f - (float)(int16_t)gap_far);

                    if (v > 0) {
                        flag = 1;
                        if (!(value > v)) {
                            value = v;
                        }
                        check_last = 1;
                    }
                }
                if ((check_last || flag) && columns[4] > 0.0f &&
                    columns[4] * 30.0f > (float)recent_ticks[side][2 + r]) {
                    flag = 0;
                }
            }
            recent_ticks[side][2 + r] = value;
            recent[side][2 + r] = flag;
        }
    }

    if (ai_globals_ptr[0x10] == 0) {
        return;
    }
    row_index = conversation_index_lookup[event];
    if ((int16_t)row_index == -1) {
        return;
    }
    row = ai_communication_lines + (int16_t)row_index * 0x28;
    if (*(int16_t *)row != event) {
        return;
    }

    // 0x42dc30: score every line of this event
    for (; *(int16_t *)row == event; row += 0x28, row_index++) {
        int16_t line_class = *(int16_t *)(row + 0x2);
        uint32_t class_word;           // [esp+0x14]
        uint32_t class_priority;       // [esp+0x54]
        int16_t selector = *(int16_t *)(row + 0x8);
        datum_index speaker_unit = k_datum_index_none;  // [esp+0x24] (ebx)
        datum_index speaker_actor = k_datum_index_none; // [esp+0x68]
        uint8_t *speaker = 0;                           // [esp+0x48] (ebp)
        uint8_t *capability = 0;                        // edi
        datum_index addressed = k_datum_index_none;     // [esp+0x60]
        datum_index target = k_datum_index_none;        // [esp+0x2c]
        uint8_t no_actor_speaker = 0;                   // [esp+0x12]
        int16_t look_kind = 0;                          // [esp+0x50]
        int16_t look_marker = 0;                        // [esp+0x70]
        int32_t delay = 0;                              // [esp+0x30]
        uint32_t recent_value = 0;                      // [esp+0x7c]
        float recency = 1.0f;                           // [esp+0x44]
        float weight = 1.0f;                            // [esp+0x88]
        float proximity;                                // [esp+0x58]
        float check_factor = 1.0f;                      // [esp+0x8c]
        float animation_factor = 1.0f;                  // [esp+0xc4]
        int32_t chain = -1;                             // [esp+0x64]
        int16_t dialogue_index;                         // [esp+0x7c]
        int32_t check_result = 0;                       // [esp+0x40]
        int16_t follow_up;
        uint32_t lipsync;
        uint8_t reject;
        float score;

        other_team_or_class = (other_team_or_class & 0xffff0000u) | (uint16_t)line_class;
        class_word = other_team_or_class;
        if (*(int16_t *)(row + 0x1c) != -1 && !buckets[*(int16_t *)(row + 0x1c)]) {
            continue;
        }
        if (game_time->game_time < ai_communication_quiet_until_tick && line_class < 6 && !(row[0x18] & 0x40)) {
            continue;
        }
        if (*(int16_t *)(row + 0x1e) != -1 && !gates[*(int16_t *)(row + 0x1e)]) {
            continue;
        }
        if (*(uint16_t *)(row + 0x20) != 0xffff &&
            (unit_index == k_datum_index_none || !(uint16_t)(*(uint16_t *)(row + 0x20) & unit_class))) {
            continue;
        }
        if (*(uint16_t *)(row + 0x22) != 0xffff &&
            (object_a == k_datum_index_none || !(uint16_t)(*(uint16_t *)(row + 0x22) & other_class))) {
            continue;
        }
        if (*(int16_t *)(row + 0x24) != -1 && *(int16_t *)(row + 0x24) != (int16_t)object_b) {
            continue;
        }
        class_priority = *(uint16_t *)((uint8_t *)ai_communication_class_priority + line_class * 2);

        // 0x42dd60: who speaks
        switch (selector) {
        case 0:
            speaker_actor = unit_actor_index;
            addressed = object_a;
            speaker_unit = unit_index;
            speaker = unit_actor;
            capability = unit_capability;
            break;
        case 1:
            speaker_actor = other_actor_index;
            addressed = unit_index;
            speaker_unit = object_a;
            speaker = other_actor;
            capability = other_capability;
            break;
        case 2:
        case 4: {
            datum_index found;

            addressed = object_a;
            if (selector == 2) {
                if (may_search) {
                    uint8_t flags = (uint8_t)(broadcast_line_flags(row[0x18]) | 0x10);

                    if (unit_encounter_index != k_datum_index_none) {
                        cached_speaker = ai_communication_select_speaker_in_reference(10.0f, event, class_word,
                            class_priority, *(uint16_t *)(row + 0x4), *(int16_t *)(row + 0x6), flags,
                            unit_encounter_index & 0xffff, unit_index, object_a);
                    } else {
                        cached_speaker = ai_communication_select_speaker_by_team(1, unit_index, object_a, 10.0f, event,
                            class_word, class_priority, *(uint16_t *)(row + 0x4), *(int16_t *)(row + 0x6), flags,
                            (int16_t)unit_team);
                    }
                    may_search = 0;
                }
                found = cached_speaker;
            } else {
                if (may_search_team) {
                    cached_team_speaker = ai_communication_select_speaker_by_team(2, unit_index, object_a, 12.0f, event,
                        class_word, class_priority, *(uint16_t *)(row + 0x4), *(int16_t *)(row + 0x6),
                        broadcast_line_flags(row[0x18]), (int16_t)unit_team);
                    may_search_team = 0;
                }
                found = cached_team_speaker;
            }
            speaker_actor = found;
            if (found != k_datum_index_none) {
                speaker = ACTOR_DATA(found);
                speaker_unit = *(datum_index *)(speaker + 0x18);
            }
            break;
        }
        default:
            break;
        }

        // 0x42df83: the speaker must be a live non-vehicle unit with a player or an actor
        reject = 0;
        if (speaker_unit == k_datum_index_none) {
            reject = 1;
        } else {
            uint8_t *object = OBJECT_DATA(speaker_unit);

            if ((object[0x106] & 4) || ((struct object *)object)->type == 1) {
                reject = 1;
            } else if (*(datum_index *)(object + 0x218) == k_datum_index_none &&
                       *(datum_index *)(object + 0x1f4) == k_datum_index_none) {
                if (row[0x18] & 8) {
                    no_actor_speaker = 1;
                } else {
                    reject = 1;
                }
            }
        }
        if (speaker != 0) {
            if (*(int16_t *)(speaker + 0x6a) == 0) {
                continue;
            }
            if (*(int16_t *)(speaker + 0x6c) == 0xb && speaker[0xa0] == 0) {
                continue;
            }
        }
        if (reject) {
            continue;
        }
        if (no_actor_speaker) {
            target = ai_select_communication_target(speaker_unit, addressed, *(int16_t *)(row + 0x4),
                                                    (int16_t)object_b, &weight);
            if (speaker_unit == unit_index) {
                may_search = 0;
                cached_speaker = target;
            }
            if (target == -1) {
                continue;
            }
        }
        if (*(int16_t *)(row + 0x1a) != -1 && capability != 0 && !capability[*(int16_t *)(row + 0x1a)]) {
            continue;
        }

        if (no_actor_speaker) {
            // 0x42e079
            class_word = (class_word & 0xffff0000u) |
                         (uint16_t)ai_communication_class_no_actor_class[(int16_t)line_class];
            proximity = 2.0f;
        } else {
            // 0x42e097: only near a player, and not again too soon
            uint8_t near;

            proximity = ai_communication_rate_player_proximity(1, 0, 0, speaker_unit);
            if (proximity == 0.0f) {
                continue;
            }
            near = (uint8_t)!(proximity >= 2.0f);
            if (speaker_actor != k_datum_index_none) {
                uint16_t type_flags = *(uint16_t *)(actor_type_procs[*(int16_t *)(ACTOR_DATA(speaker_actor) + 0x4)] + 0x4);
                int16_t type_side = (type_flags & 2) ? 0 : ((type_flags & 4) ? 1 : -1);

                if (type_side != -1) {
                    int32_t index = (line_class + type_side * 8) * 2 + near;

                    if (((uint8_t *)recent)[index]) {
                        continue;
                    }
                    recent_value = (uint16_t)((int16_t *)recent_ticks)[index];
                    if ((int16_t)class_word < 7) {
                        int32_t *history = (int32_t *)(communication_line_base +
                                                       ((int16_t)row_index * 2 + type_side) * 8);

                        if (history[0] != -1) {
                            recency = (float)(now - history[0]) * 0.0011111111f;
                            if (!(recency >= 0.0f)) {
                                recency = 0.0f;
                            } else if (!(recency <= 1.0f)) {
                                recency = 1.0f;
                            }
                        }
                        if (history[1] != -1) {
                            int32_t wait = history[1] - now;

                            if (near) {
                                wait += 30;
                            }
                            if (wait > 0) {
                                continue;
                            }
                        }
                    }
                }
            }
        }

        // 0x42e1e5: timing
        delay = (int32_t)(ai_communication_selector_delay_seconds[selector] * 30.0f);
        if ((uint16_t)unit_class == 1 && !no_actor_speaker) {
            delay += 30;
        }
        {
            float tail = (row[0x18] & 4) ? 0.0f
                                         : ai_communication_class_tail_seconds[(int16_t)class_word] * 30.0f;

            delay += recent_value;
            lipsync = (uint16_t)(int32_t)tail + recent_value;
        }

        // 0x42e24c: who looks at what
        switch (*(int16_t *)(row + 0xc)) {
        case 1:
        case 2:
        case 3: {
            datum_index looked = *(int16_t *)(row + 0xc) == 1 ? unit_index
                               : (*(int16_t *)(row + 0xc) == 2 ? speaker_unit : addressed);

            if (looked != k_datum_index_none) {
                look_kind = 1;
                look_object = looked;
                goto look_marker_default;
            }
            break;
        }
        case 4:
            if (unit_actor_index != k_datum_index_none) {
                uint8_t *a = ACTOR_DATA(unit_actor_index);

                if (((actor *)a)->danger_type > 0) {
                    look_kind = 2;
                    look_object = ((actor *)a)->danger_object_index;
                    goto look_marker_default;
                }
            }
            break;
        default:
            break;
        look_marker_default:
            look_marker = *(int16_t *)(row + 0xe);
            if (look_marker == -1 || look_marker == 1) {
                look_marker = ai_communication_class_look_marker[(int16_t)class_word];
            }
            break;
        }
        follow_up = *(int16_t *)(row + 0xa);
        if (follow_up == -1 || follow_up == 1) {
            follow_up = ai_communication_class_follow_up[(int16_t)class_word];
        }

        // 0x42e30c: can the speaker say it now
        dialogue_index = *(int16_t *)(row + 0x4);
        if (!no_actor_speaker) {
            uint32_t unused = 0;

            check_result = unit_animation_change_priority_check(speaker_unit, (uint8_t)(row[0x18] & 1),
                                                                (int16_t)class_priority, 1, &unused,
                                                                &dialogue_index, &chain);
            if ((int16_t)check_result == 1) {
                check_factor = 0.3f;
            }
            if ((int16_t)check_result == 0) {
                continue;
            }
            if (*(int16_t *)(row + 0x6) != -1 &&
                unit_scripted_action_animation_exists(speaker_unit, *(int16_t *)(row + 0x6))) {
                if (speaker_actor == k_datum_index_none) {
                    animation_factor = 2.0f;
                } else {
                    int16_t mode = *(int16_t *)(ACTOR_DATA(speaker_actor) + 0x6c);

                    if (actor_mode_definitions[mode].combat_grade != 2 /* 0x42e3cb */ &&
                        *(int16_t *)(speaker + 0x6a) != 1) {
                        animation_factor = 2.0f;
                    }
                }
            }
        }

        // 0x42e3eb
        score = animation_factor * *(float *)(row + 0x10) * check_factor * proximity * weight * recency;
        if (!(score > 0.0f)) {
            continue;
        }
        if (count >= 16) {
            break;
        }
        {
            broadcast_candidate *c = &candidates[count];

            c->row = (int16_t)row_index;
            c->no_actor_speaker = no_actor_speaker;
            c->score = score;
            c->speaker_unit = speaker_unit;
            c->speaker_actor = speaker_actor;
            c->animation = *(int16_t *)(row + 0x6);
            c->other_object = addressed;
            c->target = target;
            c->priority = (int16_t)class_priority;
            c->delay_ticks = (int16_t)delay;
            c->check_result = (int16_t)check_result;
            c->dialogue_index = dialogue_index;
            c->chain = chain;
            c->look_marker = look_marker;
            c->look_kind = look_kind;
            c->look_object = look_object;
            c->lipsync_ticks = (int16_t)lipsync;
            c->follow_up = follow_up;
            c->broadcast = (uint8_t)((row[0x18] >> 1) & 1);
            if (c->broadcast) {
                any_broadcast = 1;
            }
            total += score;
            count++;
        }
    }

    // 0x42e541: pick one
    if (count <= 0) {
        return;
    }
    if (any_broadcast) {
        int16_t i;

        total = 0.0f;
        for (i = 0; i < count; i++) {
            if (!candidates[i].broadcast) {
                candidates[i].score = 0.0f;
            }
            total += candidates[i].score;
        }
    }
    {
        broadcast_candidate *chosen = &candidates[0];
        uint8_t header[0x20];      // [esp+0xa4], the ai_communication_order
        int16_t tag_value;

        if (count > 1) {
            float pick;
            float sum = 0.0f;
            int16_t i = 0;

            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            pick = (float)(int32_t)(random_seed_global >> 16) * 1.5259022e-05f * total;
            while (i < count - 1) {
                sum += candidates[i].score;
                if (!(sum < pick)) {
                    break;
                }
                i++;
            }
            chosen = &candidates[i];
        }

        memset(header, 0, sizeof(header));
        *(datum_index *)(header + 0x00) = chosen->other_object;
        *(int16_t *)(header + 0x04) = event;
        *(int16_t *)(header + 0x06) = chosen->row;
        *(int16_t *)(header + 0x08) = (int16_t)object_b;
        header[0x0a] = 1;
        *(int16_t *)(header + 0x0c) = chosen->look_marker;
        *(int16_t *)(header + 0x0e) = chosen->look_kind;
        *(datum_index *)(header + 0x10) = chosen->look_object;
        tag_value = (int16_t)object_c;
        *(int16_t *)(header + 0x14) = tag_value == -1 ? 0 : tag_value;
        if (extra_data != 0) {
            *(uint32_t *)(header + 0x18) = extra_data[0];
            *(uint32_t *)(header + 0x1c) = extra_data[1];
        }

        if (chosen->no_actor_speaker) {
            // 0x42e6c3: a scripted object speaks; the history record is compiled but never reached (DL = 1)
            ai_propagate_communication_reaction(chosen->speaker_unit, (ai_communication_order *)header);
            ai_communication_play_event_line(chosen->speaker_unit, chosen->dialogue_index, 1, chosen->target,
                                             (uint32_t *)header);
            return;
        }

        // 0x42e734: queue the line on the speaker
        {
            uint8_t speech[0x30]; // [esp+0xf0], a unit_speech
            datum_index speaker_unit = chosen->speaker_unit;
            datum_index other_object = chosen->other_object;

            memset(speech, 0, sizeof(speech));
            *(int16_t *)(speech + 0x00) = chosen->priority;
            *(int16_t *)(speech + 0x02) = chosen->dialogue_index;
            *(int32_t *)(speech + 0x04) = chosen->chain;
            *(int16_t *)(speech + 0x08) = chosen->delay_ticks;
            *(int16_t *)(speech + 0x0a) = chosen->lipsync_ticks;
            *(int16_t *)(speech + 0x0c) = 0x18;
            memcpy(speech + 0x10, header, 0x20);
            unit_commit_speech(speaker_unit, (unit_speech *)speech, chosen->check_result);

            if ((uint16_t)chosen->animation != 0xffff) {
                // 0x42e7b3: gesture toward the addressed object, else along the speaker's facing
                uint8_t *object = OBJECT_DATA(speaker_unit);
                real_vector2d direction;

                direction.i = ((struct object *)object)->forward.i;
                direction.j = ((struct object *)object)->forward.j;
                if (other_object != k_datum_index_none) {
                    object_marker marker;
                    real_point3d from;
                    real_point3d to;
                    float dx;
                    float dy;
                    float length;

                    object_get_node_local_transform(speaker_unit, ai_marker_name_a, &marker, 1);
                    from = marker.node_transform.position;
                    object_get_node_local_transform(other_object, ai_marker_name_a, &marker, 1);
                    to = marker.node_transform.position;
                    dx = to.x - from.x;
                    dy = to.y - from.y;
                    length = (float)sqrt(dy * dy + dx * dx);
                    if (fabs(length) >= 9.999999747378752e-05) {
                        float inverse = 1.0f / length;

                        direction.i = inverse * dx;
                        direction.j = dy * inverse;
                    }
                }
                unit_try_start_scripted_action_animation(speaker_unit, (int16_t)(uint16_t)chosen->animation,
                                                         &direction);
            }
            if (chosen->speaker_actor != k_datum_index_none) {
                actor_issue_order_or_vocalize(k_datum_index_none, chosen->speaker_actor, other_object, 9,
                                              (int16_t)(uint16_t)chosen->follow_up);
            }
            ai_communication_record_line_played(speaker_unit, chosen->priority, chosen->row, -1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x42d340):

/* WARNING: Removing unreachable block (ram,0x0042e6ee) */

void ai_communication_broadcast
               (undefined4 param_1,uint param_2,uint param_3,short param_4,undefined4 param_5,
               ushort param_6,undefined4 *param_7)

{
  int *piVar1;
  short sVar2;
  float fVar3;
  undefined4 uVar4;
  undefined1 uVar5;
  char cVar6;
  byte bVar7;
  ushort uVar8;
  short sVar9;
  short sVar10;
  float fVar11;
  uint uVar12;
  int iVar13;
  byte bVar14;
  short sVar15;
  ushort uVar16;
  short sVar17;
  float *pfVar18;
  int iVar19;
  uint uVar20;
  int iVar21;
  int iVar22;
  float *pfVar23;
  short *psVar24;
  undefined4 *puVar25;
  ushort *puVar26;
  undefined2 *puVar27;
  undefined4 *puVar28;
  bool bVar29;
  float10 fVar30;
  byte local_4ca;
  char local_4c9;
  uint local_4c8;
  undefined4 local_4c4;
  undefined4 local_4c0;
  int *local_4bc;
  uint local_4b8;
  char local_4b1;
  uint local_4b0;
  undefined2 *local_4ac;
  char local_4a5;
  undefined2 local_4a4 [2];
  undefined2 local_4a0;
  float local_49c;
  float local_498;
  int local_494;
  undefined4 local_490;
  undefined1 *local_48c;
  uint local_488;
  float *local_484;
  uint local_480;
  uint local_47c;
  uint local_478;
  uint local_474;
  float local_470;
  undefined1 *local_46c;
  float local_468;
  uint local_464;
  int local_460;
  uint local_45c;
  uint local_458;
  float local_454;
  float local_450;
  uint local_44c;
  char local_448 [8];
  int local_440;
  uint local_43c;
  undefined4 local_438;
  undefined4 local_434;
  undefined4 local_430;
  undefined4 local_42c;
  float local_428;
  undefined4 local_424;
  undefined4 *local_420;
  undefined4 local_41c;
  float local_418;
  int local_414;
  int local_410;
  uint local_40c;
  uint local_408;
  float local_404;
  float local_400;
  undefined4 local_3fc;
  float local_3f8;
  float local_3f4;
  undefined4 local_3f0;
  ushort local_3ec [2];
  float local_3e8;
  undefined2 local_3e4;
  undefined2 local_3e2;
  undefined2 local_3e0;
  undefined4 local_3dc [20];
  float local_38c;
  float local_388;
  undefined4 local_384;
  float local_380;
  byte local_37c [16];
  uint auStack_36c [4];
  short asStack_35c [4];
  uint local_354 [2];
  undefined2 local_34c [422];

  iVar19 = 0;
  iVar22 = 0;
  local_440 = *(int *)(DAT_006f1d6c + 0xc);
  local_470 = 0.0;
  local_468 = 0.0;
  local_4b1 = '\0';
  local_44c = 0xffffffff;
  local_4bc = (int *)0x0;
  local_43c = 0xffffffff;
  local_414 = 0;
  local_408 = 0xffffffff;
  local_410 = 0;
  local_45c = 0xffffffff;
  local_40c = 0xffffffff;
  local_490 = 0xffffffff;
  local_4c8 = 0xffffffff;
  local_464 = 0;
  local_458 = 0;
  local_4c9 = '\x01';
  local_4a5 = '\x01';
  if (param_4 == -1) {
    param_4 = 0;
  }
  if ((short)param_5 == -1) {
    param_5 = 0;
  }
  local_4a4[0] = 0;
  local_4a0 = 0;
  if (param_2 != 0xffffffff) {
    iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    sVar15 = *(short *)(iVar19 + 0xb8);
    local_43c = *(uint *)(iVar19 + 500);
    local_490 = CONCAT22(0xffff,sVar15);
    local_464 = 0;
    if (sVar15 == 1) {
      local_464 = 1;
    }
    else if (sVar15 == 2) {
      local_464 = 2;
    }
    else if (sVar15 == 3) {
      local_464 = 4;
    }
    else if (sVar15 == 4) {
      local_464 = 0x38;
    }
    else if (sVar15 == 5) {
      local_464 = 0x40;
    }
    if (local_43c == 0xffffffff) {
      if (*(int *)(iVar19 + 0x218) != -1) {
        local_464 = 1;
      }
    }
    else {
      iVar13 = *(int *)(DAT_00880360 + 0x34);
      iVar21 = (local_43c & 0xffff) * 0x724;
      local_44c = *(uint *)(iVar21 + 0x34 + iVar13);
      local_414 = iVar21 + iVar13;
      local_464 = (uint)*(ushort *)((&PTR_PTR_006853b8)[*(short *)(iVar21 + 4 + iVar13)] + 4);
      if (*(char *)(local_414 + 0x245) < '\x01') {
        if ('\0' < *(char *)(local_414 + 0x200)) {
          local_4a0._1_1_ = 0;
          goto LAB_0042d4e2;
        }
      }
      else {
        local_4a0._1_1_ = 1;
LAB_0042d4e2:
        local_4a0 = CONCAT11(local_4a0._1_1_,1);
      }
      if (local_44c != 0xffffffff) {
        local_4bc = (int *)((local_44c & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34));
      }
    }
  }
  iVar13 = local_414;
  if (param_3 != 0xffffffff) {
    iVar22 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_3 & 0xffff) * 0xc);
    local_408 = *(uint *)(iVar22 + 500);
    uVar16 = *(ushort *)(iVar22 + 0xb8);
    local_4c8 = (uint)uVar16;
    local_458 = 0;
    if (uVar16 == 1) {
      local_458 = 1;
    }
    else if (uVar16 == 2) {
      local_458 = 2;
    }
    else if (uVar16 == 3) {
      local_458 = 4;
    }
    else if (uVar16 == 4) {
      local_458 = 0x38;
    }
    else if (uVar16 == 5) {
      local_458 = 0x40;
    }
    if (local_408 == 0xffffffff) {
      if (*(int *)(iVar22 + 0x218) != -1) {
        local_458 = 1;
      }
    }
    else {
      iVar21 = (local_408 & 0xffff) * 0x724;
      local_410 = iVar21 + *(int *)(DAT_00880360 + 0x34);
      local_458 = (uint)*(ushort *)
                         ((&PTR_PTR_006853b8)
                          [*(short *)(iVar21 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4);
      if (*(char *)(local_410 + 0x245) < '\x01') {
        if ('\0' < *(char *)(local_410 + 0x200)) {
          local_4a4[0] = 1;
        }
      }
      else {
        local_4a4[0] = 0x101;
      }
    }
  }
  sVar15 = (short)param_5;
  if ((iVar19 != 0) && (iVar22 != 0)) {
    sVar9 = (short)local_490;
    if ((sVar9 != (short)local_4c8) &&
       ((((-1 < sVar9 && (sVar9 < 10)) && (-1 < (short)local_4c8)) &&
        (((short)local_4c8 < 10 &&
         (iVar19 = (int)(short)local_4c8 + sVar9 * 10,
         (*(uint *)(DAT_006b0b84 + 0x94 + (iVar19 >> 5) * 4) & 1 << ((byte)iVar19 & 0x1f)) != 0)))))
       ) {
      bVar29 = false;
      local_4ca = 0;
      if ((short)param_1 == 0) {
        if (param_4 == 3) {
          bVar29 = true;
          local_4ca = 1;
LAB_0042d759:
          param_4 = 4;
        }
        else {
          if (local_4bc != (int *)0x0) {
            if (((*(char *)((int)local_4bc + 0x46) == '\0') &&
                (*(int *)((int)local_4bc + 0x50) != -1)) &&
               (*(int *)((int)local_4bc + 0x50) < 0x10e)) {
              bVar29 = false;
            }
            else {
              bVar29 = true;
            }
          }
          local_45c = FUN_004300d0(0,param_2,param_3,0x41900000,0,6,0xffffffff,0xffffffff,0xffffffff
                                   ,0);
          if (local_45c != 0xffffffff) {
            local_4c9 = '\0';
            local_4ca = 1;
          }
          if ((2 < sVar15) && (((sVar15 < 5 || (sVar15 == 9)) && (!bVar29)))) {
            local_4ca = 0;
          }
          if (sVar15 == 3) {
            bVar29 = false;
          }
          else if (bVar29) goto LAB_0042d759;
        }
        uVar4 = local_490;
        uVar20 = local_4c8;
        if (local_4ca != 0) {
          local_4ca = 0;
          uVar5 = FUN_0045bfc0(local_490,bVar29,&local_4ca);
          local_488 = CONCAT31(local_488._1_3_,uVar5);
          if (local_4ca != 0) {
            FUN_0042ba80(uVar20,uVar4,local_488);
          }
        }
      }
      cVar6 = FUN_0045bd50();
      if (cVar6 != '\0') {
        param_4 = 4;
      }
    }
  }
  if (iVar13 == 0) {
    local_4c4 = 2.3694278e-38;
    local_4c0 = (float)CONCAT22(local_4c0._2_2_,0x101);
  }
  else {
    if (local_4bc == (int *)0x0) {
      bVar29 = *(char *)(iVar13 + 0x274) == '\0';
      local_4c4._0_3_ = (uint3)bVar29;
      cVar6 = *(char *)(iVar13 + 0x27c);
      if (cVar6 == '\0') {
        local_4c4._0_2_ = CONCAT11(1,bVar29);
        local_4c4._0_3_ = (uint3)(ushort)local_4c4;
        if (*(int *)(iVar13 + 0x278) == -1) goto LAB_0042d812;
      }
      else {
LAB_0042d812:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xff;
      }
      if (((*(int *)(iVar13 + 0x270) == -1) || (*(int *)(iVar13 + 0x278) == -1)) ||
         (0xb3 < *(int *)(iVar13 + 0x278))) {
        local_4c4._0_3_ = CONCAT12(1,(ushort)local_4c4);
      }
      sVar9 = *(short *)(iVar13 + 0x6e);
      if (((sVar9 < 3) && ((*(int *)(iVar13 + 0x278) == -1 || (0x4a < *(int *)(iVar13 + 0x278)))))
         && ((cVar6 != '\0' || (0 < sVar9)))) {
        local_4c4 = (float)CONCAT13(1,(uint3)local_4c4);
      }
      else {
        local_4c4 = (float)(uint)(uint3)local_4c4;
      }
      local_4c0 = (float)CONCAT31(local_4c0._1_3_,sVar9 < 6);
      if (9 < *(short *)(iVar13 + 0x268)) {
LAB_0042d924:
        local_4c0._0_2_ = CONCAT11(1,(byte)local_4c0);
        if (cVar6 != '\0') goto LAB_0042d930;
      }
    }
    else {
      iVar19 = *(int *)((int)local_4bc + 0x50);
      bVar29 = *(char *)(iVar13 + 0x274) == '\0';
      local_4c4._0_3_ = (uint3)bVar29;
      if (iVar19 == -1) {
LAB_0042d8ae:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xff;
      }
      else {
        local_4c4._0_2_ = CONCAT11(1,bVar29);
        local_4c4._0_3_ = (uint3)(ushort)local_4c4;
        if (*(char *)((int)local_4bc + 0x44) != '\0') goto LAB_0042d8ae;
      }
      if ((iVar19 == -1) || (0xb3 < iVar19)) {
        local_4c4._0_3_ = CONCAT12(1,(ushort)local_4c4);
        if (*(char *)((int)local_4bc + 0x44) == '\0') goto LAB_0042d8cb;
      }
      else {
LAB_0042d8cb:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xffff;
      }
      sVar9 = *(short *)(iVar13 + 0x6e);
      if (((sVar9 < 3) && ((iVar19 == -1 || (0x4a < iVar19)))) &&
         ((*(char *)((int)local_4bc + 0x44) != '\0' || (0 < sVar9)))) {
        local_4c4 = (float)CONCAT13(1,(uint3)local_4c4);
      }
      else {
        local_4c4 = (float)(uint)(uint3)local_4c4;
      }
      if ((sVar9 < 6) && ((iVar19 == -1 || (0x4a < iVar19)))) {
        local_4c0 = (float)CONCAT31(local_4c0._1_3_,1);
      }
      else {
        local_4c0 = (float)((uint)local_4c0._1_3_ << 8);
      }
      if (*(char *)((int)local_4bc + 0x45) != '\0') {
        cVar6 = *(char *)((int)local_4bc + 0x44);
        goto LAB_0042d924;
      }
    }
    local_4c0._0_2_ = (ushort)(byte)local_4c0;
  }
LAB_0042d930:
  local_448[0] = '\0';
  local_448[1] = '\0';
  local_448[2] = '\0';
  local_448[3] = '\0';
  local_448[4] = 0;
  if ((param_4 != -1) && (local_448[param_4] = '\x01', param_4 == 4)) {
    local_448[3] = 1;
  }
  local_438 = 0.0;
  local_434 = 0;
  local_430 = 0;
  local_42c = 0;
  local_428 = 0.0;
  local_424 = 0;
  local_420 = (undefined4 *)0x0;
  local_41c = 0;
  puVar26 = local_3ec;
  for (iVar19 = 0x10; iVar19 != 0; iVar19 = iVar19 + -1) {
    puVar26[0] = 0;
    puVar26[1] = 0;
    puVar26 = puVar26 + 2;
  }
  local_484 = &local_3e8;
  local_48c = (undefined1 *)((int)&local_438 + 1);
  local_4bc = (int *)(DAT_00880354 + 0x1c);
  local_480 = 0;
  local_494 = 2;
  do {
    iVar13 = local_440;
    uVar20 = local_480;
    iVar19 = local_4bc[-2];
    iVar22 = *local_4bc;
    local_498 = 7.00649e-45;
    fVar11 = (float)(local_440 - local_4bc[2] & (local_440 - local_4bc[2] < 0) - 1);
    *(short *)((int)&local_49c + local_480) = SUB42(fVar11,0);
    local_450 = fVar11;
    uVar12 = iVar13 - iVar22;
    fVar11 = (float)(uVar12 & ((int)uVar12 < 0) - 1);
    *(short *)((int)&local_488 + uVar20) = SUB42(fVar11,0);
    pfVar18 = local_484;
    local_454 = fVar11;
    uVar12 = iVar13 - iVar19;
    uVar12 = uVar12 & ((int)uVar12 < 0) - 1;
    *(short *)((int)&local_4b0 + uVar20) = (short)uVar12;
    local_488 = uVar12;
    local_48c[-1] = 1;
    *local_48c = 1;
    pfVar23 = (float *)&DAT_00655954;
    local_46c = local_48c + 1;
    do {
      local_4ac = (undefined2 *)0x2;
      do {
        bVar29 = false;
        uVar16 = 0;
        if (0.0 < pfVar23[-1]) {
          local_4b0 = (uint)(short)local_488;
          uVar8 = __ftol();
          if (0 < (short)uVar8) {
            bVar29 = true;
            uVar16 = ((short)uVar8 < 0) - 1 & uVar8;
          }
        }
        if (0.0 < *pfVar23) {
          local_4b0 = (uint)local_454._0_2_;
          uVar8 = __ftol();
          if ((0 < (short)uVar8) && (bVar29 = true, (short)uVar16 <= (short)uVar8)) {
            uVar16 = uVar8;
          }
        }
        if (pfVar23[2] <= 0.0) {
LAB_0042db3b:
          if (bVar29) goto LAB_0042db3f;
        }
        else {
          local_4b0 = (uint)local_450._0_2_;
          uVar8 = __ftol();
          if ((short)uVar8 < 1) goto LAB_0042db3b;
          bVar29 = true;
          if ((short)uVar16 <= (short)uVar8) {
            uVar16 = uVar8;
          }
LAB_0042db3f:
          if ((0.0 < pfVar23[3]) &&
             (local_4b0 = (int)(short)*(ushort *)pfVar18,
             (float)(int)(short)*(ushort *)pfVar18 < pfVar23[3] * 30.0)) {
            bVar29 = false;
          }
        }
        *(ushort *)pfVar18 = uVar16;
        pfVar23 = pfVar23 + 5;
        pfVar18 = (float *)((int)pfVar18 + 2);
        *local_46c = bVar29;
        local_46c = local_46c + 1;
        local_4ac = (undefined2 *)((int)local_4ac + -1);
      } while (local_4ac != (undefined2 *)0x0);
      local_498 = (float)((int)local_498 + -1);
    } while (local_498 != 0.0);
    local_4bc = local_4bc + 1;
    local_494 = local_494 + -1;
    local_480 = local_480 + 2;
    local_484 = local_484 + 8;
    local_48c = local_48c + 0x10;
  } while (local_494 != 0);
  if (*(char *)(DAT_00880354 + 0x10) == '\0') {
    return;
  }
  sVar9 = (&DAT_008802e0)[(short)param_1];
  local_4bc = (int *)(int)sVar9;
  if (sVar9 == -1) {
    return;
  }
  psVar24 = (short *)(&DAT_00655aa0 + sVar9 * 0x28);
  if (*(short *)(&DAT_00655aa0 + sVar9 * 0x28) != (short)param_1) {
    return;
  }
  do {
    sVar9 = psVar24[1];
    local_4c8 = CONCAT22(local_4c8._2_2_,sVar9);
    if (((((((psVar24[0xe] != -1) && (local_448[psVar24[0xe]] == '\0')) ||
           ((*(int *)(DAT_006f1d6c + 0xc) < DAT_00725204 &&
            ((sVar9 < 6 && ((*(byte *)(psVar24 + 0xc) & 0x40) == 0)))))) ||
          ((psVar24[0xf] != -1 && (*(char *)((int)&local_4c4 + (int)psVar24[0xf]) == '\0')))) ||
         ((psVar24[0x10] != 0xffff &&
          ((param_2 == 0xffffffff || ((ushort)(psVar24[0x10] & (ushort)local_464) == 0)))))) ||
        ((psVar24[0x11] != 0xffff &&
         ((param_3 == 0xffffffff || ((ushort)(psVar24[0x11] & (ushort)local_458) == 0)))))) ||
       ((psVar24[0x12] != -1 && (psVar24[0x12] != sVar15)))) goto LAB_0042e524;
    local_488 = (uint)*(ushort *)(&DAT_006558c4 + sVar9 * 2);
    local_478 = (int)sVar9;
    uVar20 = 0xffffffff;
    iVar19 = 0;
    local_474 = 0xffffffff;
    local_4b8 = 0xffffffff;
    local_494 = 0;
    local_47c = 0xffffffff;
    local_48c = (undefined1 *)0x0;
    local_46c = (undefined1 *)0x0;
    local_4ac = (undefined2 *)0x0;
    local_4ca = 0;
    local_4b0 = 0xffffffff;
    local_460 = 0;
    local_498 = 1.0;
    local_454 = 1.0;
    puVar27 = (undefined2 *)0x0;
    switch(psVar24[4]) {
    case 0:
      puVar27 = &local_4a0;
      local_474 = local_43c;
      local_47c = param_3;
      uVar20 = param_2;
      iVar19 = local_414;
      break;
    case 1:
      puVar27 = local_4a4;
      local_474 = local_408;
      local_47c = param_2;
      uVar20 = param_3;
      iVar19 = local_410;
      break;
    case 2:
      local_47c = param_3;
      uVar12 = local_45c;
      if (local_4c9 != '\0') {
        uVar16 = psVar24[0xc];
        bVar14 = (byte)uVar16 & 1;
        bVar7 = bVar14 | 2;
        if ((uVar16 & 0x10) != 0) {
          bVar7 = bVar14 | 6;
        }
        if ((uVar16 & 0x20) != 0) {
          bVar7 = bVar7 | 8;
        }
        if (local_44c == 0xffffffff) {
          local_45c = FUN_004300d0(0,param_2,param_3,0x41900000,param_1,local_4c8,local_488,
                                   psVar24[2],psVar24[3],bVar7 | 0x10);
          local_4c9 = '\0';
          uVar12 = local_45c;
        }
        else {
          local_45c = FUN_0042ff80(0x41200000,param_1,local_4c8,local_488,psVar24[2],psVar24[3]);
          local_4c9 = '\0';
          uVar12 = local_45c;
          uVar20 = local_4b8;
        }
      }
      goto LAB_0042df55;
    default:
      goto switchD_0042dd60_caseD_3;
    case 4:
      local_47c = param_3;
      uVar12 = local_40c;
      if (local_4a5 != '\0') {
        uVar16 = psVar24[0xc];
        bVar14 = (byte)uVar16 & 1;
        bVar7 = bVar14 | 2;
        if ((uVar16 & 0x10) != 0) {
          bVar7 = bVar14 | 6;
        }
        if ((uVar16 & 0x20) != 0) {
          bVar7 = bVar7 | 8;
        }
        local_40c = FUN_004300d0(2,param_2,param_3,0x41400000,param_1,local_4c8,local_488,psVar24[2]
                                 ,psVar24[3],bVar7);
        local_4a5 = '\0';
        uVar12 = local_40c;
      }
LAB_0042df55:
      local_474 = uVar12;
      puVar27 = local_4ac;
      if (uVar12 != 0xffffffff) {
        iVar19 = (uVar12 & 0xffff) * 0x724;
        uVar20 = *(uint *)(iVar19 + 0x18 + *(int *)(DAT_00880360 + 0x34));
        iVar19 = iVar19 + *(int *)(DAT_00880360 + 0x34);
        break;
      }
      goto switchD_0042dd60_caseD_3;
    }
    local_494 = iVar19;
    local_4b8 = uVar20;
switchD_0042dd60_caseD_3:
    uVar12 = local_4b8;
    bVar29 = false;
    if (((uVar20 == 0xffffffff) ||
        (iVar22 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar20 & 0xffff) * 0xc),
        (*(byte *)(iVar22 + 0x106) & 4) != 0)) || (*(short *)(iVar22 + 0xb4) == 1)) {
LAB_0042dfd3:
      bVar29 = true;
    }
    else if ((*(int *)(iVar22 + 0x218) != -1) && (*(int *)(iVar22 + 500) == -1)) {
      if ((*(byte *)(psVar24 + 0xc) & 8) == 0) goto LAB_0042dfd3;
      local_4ca = 1;
    }
    bVar14 = local_4ca;
    if (((iVar19 == 0) ||
        ((*(short *)(iVar19 + 0x6a) != 0 &&
         ((*(short *)(local_494 + 0x6c) != 0xb || (*(char *)(local_494 + 0xa0) != '\0')))))) &&
       (!bVar29)) {
      if (local_4ca != 0) {
        local_4b0 = FUN_0042ec90(local_4b8,local_47c,psVar24[2],param_5,&local_454);
        if (uVar12 == param_2) {
          local_4c9 = '\0';
          local_45c = local_4b0;
        }
        if (local_4b0 == 0xffffffff) goto LAB_0042e524;
      }
      if (((psVar24[0xd] != -1) && (puVar27 != (undefined2 *)0x0)) &&
         (*(char *)((int)psVar24[0xd] + (int)puVar27) == '\0')) goto LAB_0042e524;
      if (bVar14 == 0) {
        fVar30 = (float10)FUN_004303f0(1,0,0);
        local_484 = (float *)(float)fVar30;
        if ((float)local_484 == 0.0) goto LAB_0042e524;
        if (local_474 != 0xffffffff) {
          sVar9 = -1;
          if ((*(ushort *)
                ((&PTR_PTR_006853b8)
                 [*(short *)((local_474 & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4)
              & 2) == 0) {
            if ((*(ushort *)
                  ((&PTR_PTR_006853b8)
                   [*(short *)((local_474 & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] +
                  4) & 4) != 0) {
              sVar9 = 1;
            }
          }
          else {
            sVar9 = 0;
          }
          if (sVar9 != -1) {
            iVar19 = (uint)((float)local_484 < 2.0) + (local_478 + sVar9 * 8) * 2;
            if (*(char *)((int)&local_438 + iVar19) != '\0') goto LAB_0042e524;
            local_460 = CONCAT22(local_460._2_2_,local_3ec[iVar19]);
            if ((short)local_4c8 < 7) {
              piVar1 = (int *)(DAT_006f0c9c + ((int)sVar9 + (short)local_4bc * 2) * 8);
              iVar19 = *piVar1;
              if (iVar19 != -1) {
                local_478 = local_440 - iVar19;
                local_498 = (float)(int)local_478 * 0.0011111111;
                if (0.0 <= local_498) {
                  if (1.0 < local_498) {
                    local_498 = 1.0;
                  }
                }
                else {
                  local_498 = 0.0;
                }
              }
              iVar19 = piVar1[1];
              if (iVar19 != -1) {
                iVar19 = iVar19 - local_440;
                if ((float)local_484 < 2.0) {
                  iVar19 = iVar19 + 0x1e;
                }
                if (0 < iVar19) goto LAB_0042e524;
              }
            }
          }
        }
      }
      else {
        local_4c8 = CONCAT22(local_4c8._2_2_,*(undefined2 *)(&DAT_00655914 + local_478 * 2));
        local_484 = (float *)&DAT_40000000;
      }
      local_4ac = (undefined2 *)__ftol();
      iVar19 = local_460;
      if (((short)local_464 == 1) && (local_4ca == 0)) {
        local_4ac = (undefined2 *)((int)local_4ac + 0x1e);
      }
      local_4ac = (undefined2 *)((int)local_4ac + local_460);
      sVar9 = __ftol();
      switch(psVar24[6]) {
      case 1:
        uVar20 = param_2;
        break;
      case 2:
        uVar20 = local_4b8;
        break;
      case 3:
        uVar20 = local_47c;
        break;
      case 4:
        if ((local_43c == 0xffffffff) ||
           (iVar22 = (local_43c & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34),
           *(short *)(iVar22 + 0x280) < 1)) goto switchD_0042e25c_default;
        local_48c = (undefined1 *)0x2;
        local_480 = *(uint *)(iVar22 + 0x28c);
        goto LAB_0042e2c7;
      default:
        goto switchD_0042e25c_default;
      }
      if (uVar20 != 0xffffffff) {
        local_48c = (undefined1 *)0x1;
        local_480 = uVar20;
LAB_0042e2c7:
        uVar16 = psVar24[7];
        local_46c = (undefined1 *)(uint)uVar16;
        if ((uVar16 == 0xffff) || (uVar16 == 1)) {
          local_46c = (undefined1 *)(uint)*(ushort *)(&DAT_00655904 + (short)local_4c8 * 2);
        }
      }
switchD_0042e25c_default:
      sVar17 = psVar24[5];
      if ((sVar17 == -1) || (sVar17 == 1)) {
        sVar17 = *(short *)(&DAT_006558f4 + (short)local_4c8 * 2);
      }
      sVar2 = psVar24[3];
      local_460 = CONCAT22(local_460._2_2_,psVar24[2]);
      local_478 = 0xffffffff;
      local_450 = 1.0;
      local_418 = 1.0;
      if (local_4ca == 0) {
        local_49c = (float)FUN_00560d00(local_488,1,&local_49c,&local_460,&local_478);
        sVar10 = SUB42(local_49c,0);
        if ((sVar10 != 0) && (sVar10 == 1)) {
          local_450 = 0.3;
        }
        if (sVar10 == 0) goto LAB_0042e524;
        if (((sVar2 != -1) && (cVar6 = FUN_00569470(), cVar6 != '\0')) &&
           ((local_474 == 0xffffffff ||
            ((*(short *)(&DAT_00655258 +
                        *(short *)((local_474 & 0xffff) * 0x724 + 0x6c +
                                  *(int *)(DAT_00880360 + 0x34)) * 0x38) != 2 &&
             (*(short *)(local_494 + 0x6a) != 1)))))) {
          local_418 = 2.0;
        }
      }
      fVar11 = local_418 * *(float *)(psVar24 + 8) * local_450 * (float)local_484 * local_454 *
               local_498;
      if (0.0 < fVar11) {
        if (0xf < SUB42(local_470,0)) break;
        iVar22 = (int)SUB42(local_470,0);
        iVar13 = iVar22 * 0x38;
        local_34c[iVar22 * 0x1c] = (short)local_4bc;
        local_37c[iVar13 + 1] = local_4ca;
        (&local_380)[iVar22 * 0xe] = fVar11;
        auStack_36c[iVar22 * 0xe] = local_4b8;
        auStack_36c[iVar22 * 0xe + 1] = local_474;
        *(short *)(local_37c + iVar13 + 6) = psVar24[3];
        auStack_36c[iVar22 * 0xe + 2] = local_47c;
        auStack_36c[iVar22 * 0xe + 3] = local_4b0;
        *(short *)(local_37c + iVar13 + 4) = (short)local_488;
        *(undefined2 *)(local_37c + iVar13 + 10) = local_4ac._0_2_;
        *(undefined2 *)(local_37c + iVar13 + 8) = local_49c._0_2_;
        *(undefined2 *)(local_37c + iVar13 + 2) = (undefined2)local_460;
        local_354[iVar22 * 0xe + 1] = local_478;
        asStack_35c[iVar22 * 0x1c + 1] = (short)local_46c;
        asStack_35c[iVar22 * 0x1c + 2] = (short)local_48c;
        local_354[iVar22 * 0xe] = local_480;
        bVar14 = *(byte *)(psVar24 + 0xc);
        *(short *)(local_37c + iVar13 + 0xc) = sVar9 + (short)iVar19;
        asStack_35c[iVar22 * 0x1c] = sVar17;
        bVar14 = bVar14 >> 1 & 1;
        local_37c[iVar13] = bVar14;
        if (bVar14 != 0) {
          local_4b1 = '\x01';
        }
        local_470 = (float)((int)local_470 + 1);
        local_468 = fVar11 + local_468;
      }
    }
LAB_0042e524:
    psVar24 = psVar24 + 0x14;
    local_4bc = (int *)((int)local_4bc + 1);
  } while (*psVar24 == (short)param_1);
  uVar4 = local_430;
  sVar9 = SUB42(local_470,0);
  if (sVar9 < 1) {
    return;
  }
  pfVar18 = &local_380;
  if ((local_4b1 != '\0') && (local_468 = 0.0, 0 < sVar9)) {
    uVar20 = (uint)local_470 & 0xffff;
    pfVar23 = pfVar18;
    do {
      if (*(char *)(pfVar23 + 1) == '\0') {
        *pfVar23 = 0.0;
      }
      uVar20 = uVar20 - 1;
      local_468 = local_468 + *pfVar23;
      pfVar23 = pfVar23 + 0xe;
    } while (uVar20 != 0);
  }
  if (1 < sVar9) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    sVar17 = 0;
    local_49c = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * local_468;
    fVar11 = 0.0;
    if (0 < sVar9 + -1) {
      iVar19 = 0;
      do {
        fVar11 = fVar11 + (&local_380)[iVar19 * 0xe];
        if (local_49c <= fVar11) break;
        sVar17 = sVar17 + 1;
        iVar19 = (int)sVar17;
      } while (iVar19 < sVar9 + -1);
    }
    pfVar18 = &local_380 + sVar17 * 0xe;
  }
  local_430 = CONCAT22(local_430._2_2_,sVar15);
  uVar16 = *(ushort *)(pfVar18 + 0xd);
  fVar11 = pfVar18[7];
  local_42c = CONCAT22(local_42c._2_2_,*(undefined2 *)((int)pfVar18 + 0x26));
  local_434 = CONCAT22(uVar16,(undefined2)local_434);
  local_428 = pfVar18[0xb];
  local_49c = (float)(uint)uVar16;
  local_42c = *(undefined4 *)((int)pfVar18 + 0x26);
  local_434 = CONCAT22(uVar16,(short)param_1);
  local_438 = fVar11;
  local_430._3_1_ = SUB41(uVar4,3);
  local_430._0_3_ = CONCAT12(1,sVar15);
  local_424 = CONCAT22(local_424._2_2_,(param_6 == 0xffff) - 1 & param_6);
  if (param_7 == (undefined4 *)0x0) {
    local_420 = param_7;
    local_41c = 0;
  }
  else {
    local_41c = param_7[1];
    local_420 = (undefined4 *)*param_7;
  }
  if (*(char *)((int)pfVar18 + 5) != '\0') {
    fVar11 = pfVar18[5];
    uVar16 = *(ushort *)((int)pfVar18 + 6);
    switch(*(undefined2 *)(pfVar18 + 2)) {
    case 0:
    case 1:
    case 2:
    case 7:
    case 10:
      break;
    default:
    }
    local_49c = (float)(uint)uVar16;
    FUN_0042e9c0(fVar11,&local_438);
    FUN_0042eee0(fVar11,(uint)uVar16,1,pfVar18[8],&local_438);
    return;
  }
  local_3ec[0] = *(ushort *)(pfVar18 + 2);
  local_3e4 = *(undefined2 *)((int)pfVar18 + 0xe);
  local_3e2 = *(undefined2 *)(pfVar18 + 4);
  local_3e0 = 0x18;
  local_488 = (uint)*(ushort *)(pfVar18 + 2);
  local_3ec[1] = *(undefined2 *)((int)pfVar18 + 6);
  local_3e8 = pfVar18[0xc];
  puVar25 = &local_438;
  puVar28 = local_3dc;
  for (iVar19 = 8; iVar19 != 0; iVar19 = iVar19 + -1) {
    *puVar28 = *puVar25;
    puVar25 = puVar25 + 1;
    puVar28 = puVar28 + 1;
  }
  fVar3 = pfVar18[5];
  FUN_00560f20();
  if (*(short *)((int)pfVar18 + 10) == -1) goto LAB_0042e8f7;
  iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)fVar3 & 0xffff) * 0xc);
  local_4c4 = *(float *)(iVar19 + 0x74);
  local_4c0 = *(float *)(iVar19 + 0x78);
  if (fVar11 != -NAN) {
    object_get_node_local_transform(fVar3,&DAT_0066bfa0,local_3ec,1);
    local_3f8 = local_38c;
    local_3f4 = local_388;
    local_3f0 = local_384;
    object_get_node_local_transform(fVar11,&DAT_0066bfa0,local_3ec,1);
    local_404 = local_38c;
    local_38c = local_38c - local_3f8;
    local_400 = local_388;
    local_388 = local_388 - local_3f4;
    local_3fc = local_384;
    local_470 = SQRT(local_38c * local_38c + local_388 * local_388);
    if (0.0001 <= ABS(local_470)) {
      local_4c4 = (1.0 / local_470) * local_38c;
      local_4c0 = local_388 * (1.0 / local_470);
      if (local_470 != 0.0) goto LAB_0042e8e2;
    }
    local_4c4 = *(float *)(iVar19 + 0x74);
    local_4c0 = *(float *)(iVar19 + 0x78);
  }
LAB_0042e8e2:
  FUN_00569530(fVar3,*(undefined2 *)((int)pfVar18 + 10),&local_4c4);
LAB_0042e8f7:
  if (pfVar18[6] != -NAN) {
    FUN_004302e0(9,*(undefined2 *)(pfVar18 + 9));
  }
  ai_communication_record_line_played(local_488,local_49c,0xffffffff);
  return;
}
#endif
