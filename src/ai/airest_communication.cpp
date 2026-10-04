#include "halo/game/constants.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/ai/airest_communication.hpp"

#include <stdint.h>
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/ai_constants.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"

static auto &global_structure_bsp = halo::link::ref<uint8_t *>(halo::ai::vars().global_structure_bsp);
static auto &global_structure_bsp_typed = reinterpret_cast<ScenarioStructureBSP *&>(global_structure_bsp);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
static auto &team_pair_data = halo::link::ref<team_pair_globals *>(halo::ai::vars().team_pair_data);
static auto &conversation_index_lookup = halo::link::ref<int16_t []>(halo::ai::vars().conversation_index_lookup);
static auto &ai_communication_lines = halo::link::ref<uint8_t []>(halo::ai::vars().ai_communication_lines);
static auto &ai_communication_direction_table = halo::link::ref<float []>(halo::ai::vars().ai_communication_direction_table);
static auto &ai_communication_class_priority = halo::link::ref<int16_t []>(halo::ai::vars().ai_communication_class_priority);
static auto &ai_communication_class_tail_seconds = halo::link::ref<float []>(halo::ai::vars().ai_communication_class_tail_seconds);
static auto &ai_communication_class_follow_up = halo::link::ref<int16_t []>(halo::ai::vars().ai_communication_class_follow_up);
static auto &ai_communication_class_look_marker = halo::link::ref<int16_t []>(halo::ai::vars().ai_communication_class_look_marker);
static auto &ai_communication_class_no_actor_class = halo::link::ref<int16_t []>(halo::ai::vars().ai_communication_class_no_actor_class);
static auto &ai_communication_selector_delay_seconds = halo::link::ref<float []>(halo::ai::vars().ai_communication_selector_delay_seconds);
static auto &communication_line_history = halo::link::ref<ai_line_history *>(halo::ai::vars().communication_line_base);
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &communication_line_count = halo::link::ref<int16_t>(halo::ai::vars().communication_line_count);
static auto &conversation_line_count = halo::link::ref<int16_t>(halo::ai::vars().conversation_line_count);
static auto &conversation_line_history = halo::link::ref<ai_line_history *>(halo::ai::vars().conversation_line_base);
static auto &ai_communication_event_definitions = halo::link::ref<ai_communication_event_definition []>(halo::ai::vars().ai_communication_event_definitions);
static inline uint8_t *ai_communication_event_definition_bytes()
{
    return reinterpret_cast<uint8_t *>(ai_communication_event_definitions);
}
static auto &ai_communication_class_repeat_delay = halo::link::ref<float []>(halo::ai::vars().ai_communication_class_repeat_delay);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &ai_communication_line_table = halo::link::ref<real []>(halo::ai::vars().ai_communication_line_table);
static auto &ai_conversation_line_table = halo::link::ref<real []>(halo::ai::vars().ai_conversation_line_table);

namespace halo::ai {

/**
 * Behaviour of ai broadcast communication event, moved unchanged from the original free function.
 *
 * @address 0x429fc0
 */
void AiCommunication::broadcast_communication_event(int16_t gate, real_point3d *point, int32_t source_object, int16_t event_type, int16_t unused)
{
    bsp_leaf_reference location;
    actor_iterator_state iterator;
    actor *a;
    int32_t leaf;

    (void)unused;
    leaf = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, point);
    location.leaf_index = leaf;
    location.cluster_index = leaf == -1 ? -1 :
        (int16_t)halo::ai::reflexive_data<ScenarioStructureBSPLeaf>(global_structure_bsp_typed->leaves)[leaf & 0x7fffffff].cluster;
    if (halo::ai::globals().state->actors_valid) {
        iterator.filter_array = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = k_datum_index_none;
        iterator.next_actor_index = -1;
    }
    for (a = halo::ai::actor_iterator_next(&iterator); a != 0; a = halo::ai::actor_iterator_next(&iterator)) {
        datum_index actor_index = iterator.actor_index;
        actor_firing_positions block;

        if (a->combat_status >= 7) {
            continue;
        }
        halo::ai::actor_get_firing_positions(actor_index, &block, point);
        if ((int16_t)halo::ai::actor_target_hearing_check(&location, 0, actor_index, &block, gate, point) < 2) {
            continue;
        }
        if (event_type == 0) {
            halo::ai::actor_queue_point_reaction_dialogue(point, actor_index);
        } else if (event_type == 1) {
            halo::ai::actor_react_to_registered_danger(point, actor_index, source_object);
        } else if (event_type == 2) {
            halo::ai::actor_react_to_flee_point(actor_index, source_object, point);
        }
    }
}

namespace {

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

}

/**
 * Behaviour of ai communication broadcast, moved unchanged from the original free function.
 *
 * @address 0x42d340
 */
void AiCommunication::broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data)
{
    int32_t now = halo::game::globals().game_time->game_time;
    int16_t event = (int16_t)event_code;
    unit_object *unit = 0;
    unit_object *other = 0;
    actor *unit_actor = 0;
    actor *other_actor = 0;
    encounter *unit_encounter = 0;
    datum_index unit_actor_index = k_datum_index_none;
    datum_index other_actor_index = k_datum_index_none;
    datum_index unit_encounter_index = k_datum_index_none;
    datum_index cached_speaker = k_datum_index_none;
    datum_index cached_team_speaker = k_datum_index_none;
    uint32_t unit_team = halo::k_dword_none;
    uint32_t other_team_or_class = halo::k_dword_none;
    uint32_t unit_class = 0;
    uint32_t other_class = 0;
    uint8_t may_search = 1;
    uint8_t may_search_team = 1;
    uint8_t unit_capability[2] = {0, 0};
    uint8_t other_capability[2] = {0, 0};
    uint8_t gates[6];
    uint8_t buckets[16];
    uint8_t recent[2][16];
    int16_t recent_ticks[2][16];
    broadcast_candidate candidates[16];
    int16_t count = 0;
    float total = 0.0f;
    uint8_t any_broadcast = 0;
    datum_index look_object = 4;
    int32_t side;
    int32_t row_index;
    ai_communication_line_definition *row;

    if ((int16_t)reason == -1) {
        reason = 0;
    }
    if ((int16_t)object_b == -1) {
        object_b = 0;
    }

    if (unit_index != k_datum_index_none) {
        unit = (unit_object *)halo::ai::object_bytes(unit_index);
        unit_team = (unit_team & 0xffff0000u) | static_cast<uint16_t>(unit->base.owner_team);
        unit_actor_index = unit->unit.actor_index;
        unit_class = broadcast_team_class((int16_t)unit_team);
        if (unit_actor_index != k_datum_index_none) {
            unit_actor = halo::ai::actor_at(unit_actor_index);
            unit_class = (unit_class & 0xffff0000u) |
                         actor_type_procs[unit_actor->type]->flags;
            unit_encounter_index = unit_actor->encounter_index;
            if ((int8_t)unit_actor->tally.group_c_total > 0) {
                unit_capability[1] = 1;
                unit_capability[0] = 1;
            } else if ((int8_t)unit_actor->tally.group_a_total > 0) {
                unit_capability[1] = 0;
                unit_capability[0] = 1;
            }
            if (unit_encounter_index != k_datum_index_none) {
                unit_encounter = halo::ai::encounter_at(unit_encounter_index);
            }
        } else if (unit->unit.controlling_player != k_datum_index_none) {
            unit_class = 1;
        }
    }
    if (object_a != k_datum_index_none) {
        other = (unit_object *)halo::ai::object_bytes(object_a);
        other_actor_index = other->unit.actor_index;
        other_team_or_class = static_cast<uint16_t>(other->base.owner_team);
        other_class = broadcast_team_class((int16_t)other_team_or_class);
        if (other_actor_index != k_datum_index_none) {
            other_actor = halo::ai::actor_at(other_actor_index);
            other_class = (other_class & 0xffff0000u) |
                          actor_type_procs[other_actor->type]->flags;
            if ((int8_t)other_actor->tally.group_c_total > 0) {
                other_capability[0] = 1;
                other_capability[1] = 1;
            } else if ((int8_t)other_actor->tally.group_a_total > 0) {
                other_capability[0] = 1;
                other_capability[1] = 0;
            }
        } else if (other->unit.controlling_player != k_datum_index_none) {
            other_class = 1;
        }
    }

    if (unit != 0 && other != 0) {
        int16_t team_u = (int16_t)unit_team;
        int16_t team_o = (int16_t)other_team_or_class;

        if (team_u != team_o && team_u >= 0 && team_u < 10 && team_o >= 0 && team_o < 10) {
            int32_t bit = team_u * 10 + team_o;

            if (team_pair_data->secondary_bits[bit >> 5] & (1u << (bit & 0x1f))) {
                uint8_t react = 0;
                uint8_t hostile = 0;

                if (event == 0) {
                    if ((int16_t)reason == 3) {
                        hostile = 1;
                        react = 1;
                        reason = 4;
                    } else {
                        if (unit_encounter != 0) {
                            int32_t since = (int32_t)unit_encounter->ticks_since_engaged;

                            hostile = (uint8_t)!(unit_encounter->unknown_46 == 0 && since != -1 && since < halo::ai::k_encounter_hostile_memory_ticks);
                        }
                        cached_speaker = halo::ai::ai_communication_select_speaker_by_team(0, unit_index, object_a, 18.0f, 0, 6,
                                                                                 halo::k_dword_none, halo::k_dword_none, -1, 0,
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
                        uint8_t status = (uint8_t)halo::game::team_pair_override_adjust_counter(team_o, team_u, hostile != 0,
                                                                                    &mark);

                        if (mark) {
                            halo::ai::ai_mark_recognized_objects_for_reaction(team_o, team_u, status);
                        }
                    }
                }
                if (halo::game::teams_are_enemies(team_o, team_u)) {
                    reason = 4;
                }
            }
        }
    }

    if (unit_actor == 0) {
        memset(gates, 1, sizeof(gates));
    } else if (unit_encounter == 0) {
        uint8_t alerted = unit_actor->target_alive;
        int32_t since = unit_actor->ticks_since_engaged;
        int16_t grade = unit_actor->combat_status;

        gates[0] = (uint8_t)(unit_actor->ever_had_target[0] == 0);
        gates[1] = (uint8_t)(alerted == 0 && since != -1);
        gates[2] = (uint8_t)!(unit_actor->target_unit_index != k_datum_index_none && since != -1 && since < 0xb4);
        gates[3] = (uint8_t)(grade < 3 && !(since != -1 && since < 0x4b) && (alerted != 0 || grade > 0));
        gates[4] = (uint8_t)(grade < 6);
        gates[5] = (uint8_t)(unit_actor->target_combat_status >= 10 && alerted != 0);
    } else {
        int32_t since = (int32_t)unit_encounter->ticks_since_engaged;
        uint8_t engaged = unit_encounter->has_live_target;
        int16_t grade = unit_actor->combat_status;

        gates[0] = (uint8_t)(unit_actor->ever_had_target[0] == 0);
        gates[1] = (uint8_t)(since != -1 && engaged == 0);
        gates[2] = (uint8_t)(!(since != -1 && since < 0xb4) && engaged != 0);
        gates[3] = (uint8_t)(grade < 3 && !(since != -1 && since < 0x4b) && (engaged != 0 || grade > 0));
        gates[4] = (uint8_t)(grade < 6 && !(since != -1 && since < 0x4b));
        gates[5] = (uint8_t)(unit_encounter->engaged != 0 && engaged != 0);
    }

    memset(buckets, 0, sizeof(buckets));
    if ((int16_t)reason != -1 && (uint16_t)reason < 16) {
        buckets[(int16_t)reason] = 1;
        if ((int16_t)reason == 4) {
            buckets[3] = 1;
        }
    }

    memset(recent, 0, sizeof(recent));
    memset(recent_ticks, 0, sizeof(recent_ticks));
    for (side = 0; side < 2; side++) {
        int32_t *ticks = &halo::ai::globals().state->loudest_line_tick[0][0] + 2 + side;
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
                    columns[4] * halo::game::k_ticks_per_second_f > (float)recent_ticks[side][2 + r]) {
                    flag = 0;
                }
            }
            recent_ticks[side][2 + r] = value;
            recent[side][2 + r] = flag;
        }
    }

    if (halo::ai::globals().state->dialogue_triggers_enabled == 0) {
        return;
    }
    row_index = conversation_index_lookup[event];
    if ((int16_t)row_index == -1) {
        return;
    }
    row = (ai_communication_line_definition *)(ai_communication_lines + (int16_t)row_index * 0x28);
    if (row->event_id != event) {
        return;
    }

    for (; row->event_id == event; ++row, row_index++) {
        int16_t line_class = row->class_index;
        uint32_t class_word;
        uint32_t class_priority;
        int16_t selector = row->participant_selector;
        datum_index speaker_unit = k_datum_index_none;
        datum_index speaker_actor = k_datum_index_none;
        actor *speaker = 0;
        uint8_t *capability = 0;
        datum_index addressed = k_datum_index_none;
        datum_index target = k_datum_index_none;
        uint8_t no_actor_speaker = 0;
        int16_t look_kind = 0;
        int16_t look_marker = 0;
        int32_t delay = 0;
        uint32_t recent_value = 0;
        float recency = 1.0f;
        float weight = 1.0f;
        float proximity;
        float check_factor = 1.0f;
        float animation_factor = 1.0f;
        int32_t chain = -1;
        int16_t dialogue_index;
        int32_t check_result = 0;
        int16_t follow_up;
        uint32_t lipsync;
        uint8_t reject;
        float score;

        other_team_or_class = (other_team_or_class & 0xffff0000u) | (uint16_t)line_class;
        class_word = other_team_or_class;
        if (row->required_kind != -1 && !buckets[row->required_kind]) {
            continue;
        }
        if (halo::game::globals().game_time->game_time < halo::ai::globals().communication_quiet_until_tick && line_class < 6 && !(row->flags & 0x40)) {
            continue;
        }
        if (row->relationship_gate != -1 && !gates[row->relationship_gate]) {
            continue;
        }
        if (row->source_type_mask != halo::k_word_none &&
            (unit_index == k_datum_index_none || !(uint16_t)(row->source_type_mask & unit_class))) {
            continue;
        }
        if (row->target_type_mask != halo::k_word_none &&
            (object_a == k_datum_index_none || !(uint16_t)(row->target_type_mask & other_class))) {
            continue;
        }
        if (row->required_seat != -1 && row->required_seat != (int16_t)object_b) {
            continue;
        }
        class_priority = (uint16_t)ai_communication_class_priority[line_class];

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
                    uint8_t flags = (uint8_t)(broadcast_line_flags(row->flags) | 0x10);

                    if (unit_encounter_index != k_datum_index_none) {
                        cached_speaker = halo::ai::ai_communication_select_speaker_in_reference(10.0f, event, class_word,
                            class_priority, static_cast<uint16_t>(row->table_arg_a), row->table_arg_b, flags,
                            unit_encounter_index & halo::k_slot_mask, unit_index, object_a);
                    } else {
                        cached_speaker = halo::ai::ai_communication_select_speaker_by_team(1, unit_index, object_a, 10.0f, event,
                            class_word, class_priority, static_cast<uint16_t>(row->table_arg_a), row->table_arg_b, flags,
                            (int16_t)unit_team);
                    }
                    may_search = 0;
                }
                found = cached_speaker;
            } else {
                if (may_search_team) {
                    cached_team_speaker = halo::ai::ai_communication_select_speaker_by_team(2, unit_index, object_a, 12.0f, event,
                        class_word, class_priority, static_cast<uint16_t>(row->table_arg_a), row->table_arg_b,
                        broadcast_line_flags(row->flags), (int16_t)unit_team);
                    may_search_team = 0;
                }
                found = cached_team_speaker;
            }
            speaker_actor = found;
            if (found != k_datum_index_none) {
                speaker = halo::ai::actor_at(found);
                speaker_unit = speaker->unit_index;
            }
            break;
        }
        default:
            break;
        }

        reject = 0;
        if (speaker_unit == k_datum_index_none) {
            reject = 1;
        } else {
            unit_object *object = (unit_object *)halo::ai::object_bytes(speaker_unit);

            if ((static_cast<uint8_t>(object->base.vitality_flags) & 4) || ((struct object *)object)->type == 1) {
                reject = 1;
            } else if (object->unit.controlling_player == k_datum_index_none &&
                       object->unit.actor_index == k_datum_index_none) {
                if (row->flags & 8) {
                    no_actor_speaker = 1;
                } else {
                    reject = 1;
                }
            }
        }
        if (speaker != 0) {
            if (speaker->awareness_level == 0) {
                continue;
            }
            if (speaker->mode == halo::ai::actor_mode::obey && speaker->mode_data.obey.allow_communication == 0) {
                continue;
            }
        }
        if (reject) {
            continue;
        }
        if (no_actor_speaker) {
            target = halo::ai::ai_select_communication_target(speaker_unit, addressed, row->table_arg_a,
                                                    (int16_t)object_b, &weight);
            if (speaker_unit == unit_index) {
                may_search = 0;
                cached_speaker = target;
            }
            if (target == -1) {
                continue;
            }
        }
        if (row->capability_index != -1 && capability != 0 && !capability[row->capability_index]) {
            continue;
        }

        if (no_actor_speaker) {
            class_word = (class_word & 0xffff0000u) |
                         (uint16_t)ai_communication_class_no_actor_class[(int16_t)line_class];
            proximity = 2.0f;
        } else {
            uint8_t near;

            proximity = halo::ai::ai_communication_rate_player_proximity(1, 0, 0, speaker_unit);
            if (proximity == 0.0f) {
                continue;
            }
            near = (uint8_t)!(proximity >= 2.0f);
            if (speaker_actor != k_datum_index_none) {
                uint16_t type_flags = actor_type_procs[halo::ai::actor_at(speaker_actor)->type]->flags;
                int16_t type_side = (type_flags & 2) ? 0 : ((type_flags & 4) ? 1 : -1);

                if (type_side != -1) {
                    int32_t index = (line_class + type_side * 8) * 2 + near;

                    if ((&recent[0][0])[index]) {
                        continue;
                    }
                    recent_value = (uint16_t)(&recent_ticks[0][0])[index];
                    if ((int16_t)class_word < 7) {
                        ai_line_history *history = &communication_line_history[(int16_t)row_index * 2 + type_side];

                        if (history->last_tick != -1) {
                            recency = (float)(now - history->last_tick) * 0.0011111111f;
                            if (!(recency >= 0.0f)) {
                                recency = 0.0f;
                            } else if (!(recency <= 1.0f)) {
                                recency = 1.0f;
                            }
                        }
                        if (history->cooldown_until_tick != -1) {
                            int32_t wait = history->cooldown_until_tick - now;

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

        delay = (int32_t)(ai_communication_selector_delay_seconds[selector] * halo::game::k_ticks_per_second_f);
        if ((uint16_t)unit_class == 1 && !no_actor_speaker) {
            delay += 30;
        }
        {
            float tail = (row->flags & 4) ? 0.0f
                                         : ai_communication_class_tail_seconds[(int16_t)class_word] * halo::game::k_ticks_per_second_f;

            delay += recent_value;
            lipsync = (uint16_t)(int32_t)tail + recent_value;
        }

        bool look_found = false;

        switch (row->look_target_selector) {
        case 1:
        case 2:
        case 3: {
            datum_index looked = row->look_target_selector == 1 ? unit_index
                               : (row->look_target_selector == 2 ? speaker_unit : addressed);

            if (looked != k_datum_index_none) {
                look_kind = 1;
                look_object = looked;
                look_found = true;
            }
            break;
        }
        case 4:
            if (unit_actor_index != k_datum_index_none) {
                actor *a = halo::ai::actor_at(unit_actor_index);

                if (a->danger_type > 0) {
                    look_kind = 2;
                    look_object = a->danger_object_index;
                    look_found = true;
                }
            }
            break;
        default:
            break;
        }
        if (look_found) {
            look_marker = row->look_marker_selector;
            if (look_marker == -1 || look_marker == 1) {
                look_marker = ai_communication_class_look_marker[(int16_t)class_word];
            }
        }
        follow_up = row->fallback_order;
        if (follow_up == -1 || follow_up == 1) {
            follow_up = ai_communication_class_follow_up[(int16_t)class_word];
        }

        dialogue_index = row->table_arg_a;
        if (!no_actor_speaker) {
            uint32_t unused = 0;

            check_result = halo::units::unit_animation_change_priority_check(speaker_unit, (uint8_t)(row->flags & 1),
                                                                (int16_t)class_priority, 1, &unused,
                                                                &dialogue_index, &chain);
            if ((int16_t)check_result == 1) {
                check_factor = 0.3f;
            }
            if ((int16_t)check_result == 0) {
                continue;
            }
            if (row->table_arg_b != -1 &&
                halo::units::unit_scripted_action_animation_exists(speaker_unit, row->table_arg_b)) {
                if (speaker_actor == k_datum_index_none) {
                    animation_factor = 2.0f;
                } else {
                    int16_t mode = halo::ai::actor_at(speaker_actor)->mode;

                    if (actor_mode_definitions[mode].combat_grade != 2 /* 0x42e3cb */ &&
                        speaker->awareness_level != 1) {
                        animation_factor = 2.0f;
                    }
                }
            }
        }

        score = animation_factor * row->probability * check_factor * proximity * weight * recency;
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
            c->animation = row->table_arg_b;
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
            c->broadcast = (uint8_t)((row->flags >> 1) & 1);
            if (c->broadcast) {
                any_broadcast = 1;
            }
            total += score;
            count++;
        }
    }

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
        ai_communication_target_result header;
        int16_t tag_value;

        if (count > 1) {
            float pick;
            float sum = 0.0f;
            int16_t i = 0;

            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            pick = (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f * total;
            while (i < count - 1) {
                sum += candidates[i].score;
                if (!(sum < pick)) {
                    break;
                }
                i++;
            }
            chosen = &candidates[i];
        }

        memset(&header, 0, sizeof(header));
        header.target = chosen->other_object;
        header.event = event;
        header.row = chosen->row;
        header.object_b = (int16_t)object_b;
        header.valid = 1;
        header.look_marker = chosen->look_marker;
        header.look_kind = chosen->look_kind;
        header.look_object = chosen->look_object;
        tag_value = (int16_t)object_c;
        header.tag_value = tag_value == -1 ? 0 : tag_value;
        if (extra_data != 0) {
            header.extra_data[0] = extra_data[0];
            header.extra_data[1] = extra_data[1];
        }

        if (chosen->no_actor_speaker) {
            halo::ai::ai_propagate_communication_reaction(chosen->speaker_unit, reinterpret_cast<ai_communication_order *>(&header));
            halo::ai::ai_communication_play_event_line(chosen->speaker_unit, chosen->dialogue_index, 1, chosen->target,
                                             reinterpret_cast<uint32_t *>(&header));
            return;
        }

        {
            unit_speech speech;
            datum_index speaker_unit = chosen->speaker_unit;
            datum_index other_object = chosen->other_object;

            memset(&speech, 0, sizeof(speech));
            speech.priority = chosen->priority;
            speech.scream_type = chosen->dialogue_index;
            speech.sound_tag = (datum_index)chosen->chain;
            speech.delay_ticks = chosen->delay_ticks;
            speech.lipsync_ticks = chosen->lipsync_ticks;
            speech.tail_ticks = 0x18;
            halo::ai::speech_target(speech) = header;
            halo::units::unit_commit_speech(speaker_unit, &speech, chosen->check_result);

            if ((uint16_t)chosen->animation != halo::k_word_none) {
                unit_object *object = (unit_object *)halo::ai::object_bytes(speaker_unit);
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

                    halo::objects::object_get_node_local_transform(speaker_unit, ai_marker_name_a, &marker, 1);
                    from = marker.node_transform.position;
                    halo::objects::object_get_node_local_transform(other_object, ai_marker_name_a, &marker, 1);
                    to = marker.node_transform.position;
                    dx = to.x - from.x;
                    dy = to.y - from.y;
                    length = (float)halo::libm::sqrt(dy * dy + dx * dx);
                    if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
                        float inverse = 1.0f / length;

                        direction.i = inverse * dx;
                        direction.j = dy * inverse;
                    }
                }
                halo::units::unit_try_start_scripted_action_animation(speaker_unit, (int16_t)(uint16_t)chosen->animation,
                                                         &direction);
            }
            if (chosen->speaker_actor != k_datum_index_none) {
                halo::ai::actor_issue_order_or_vocalize(k_datum_index_none, chosen->speaker_actor, other_object, 9,
                                              (int16_t)(uint16_t)chosen->follow_up);
            }
            halo::ai::ai_communication_record_line_played(speaker_unit, chosen->priority, chosen->row, -1);
        }
    }
}


/**
 * Behaviour of ai communication gate line played, moved unchanged from the original free function.
 *
 * @address 0x42e970
 */
void AiCommunication::gate_line_played(int16_t event_id, ai_communication_record *record, datum_index object_index)
{
    switch (event_id) {
        case 0: case 1: case 2: case 7: case 10:
            break;
        default:
            if (record->silenced == 0) {
                halo::ai::ai_communication_record_line_played(object_index, event_id,
                    record->line_row, -1);
            }
            break;
    }
}

/**
 * Behaviour of ai communication initialize, moved unchanged from the original free function.
 *
 * @address 0x42cf20
 */
void AiCommunication::initialize()
{
    ai_communication_line_definition *line = reinterpret_cast<ai_communication_line_definition *>(ai_communication_lines);
    ai_communication_event_definition *event_row = reinterpret_cast<ai_communication_event_definition *>(ai_communication_event_definition_bytes());
    int16_t conversation_index;
    int16_t position;
    int16_t next_conversation_index;
    uint8_t *dest;
    uint8_t *header;
    int32_t i;

    communication_line_count = 0;
    do {
        line++;
        communication_line_count = communication_line_count + 1;
    } while (line->event_id != -1);

    if (communication_line_history == 0) {
        int32_t allocation_size = (int32_t)communication_line_count * 0x10;
        communication_line_history = (ai_line_history *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
        halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + allocation_size;
        halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&allocation_size, 4);
    }

    conversation_line_count = 0;
    do {
        event_row++;
        conversation_line_count = conversation_line_count + 1;
    } while (event_row->event_id != -1);

    if (conversation_line_history == 0) {
        int32_t allocation_size = (int32_t)conversation_line_count * 0x10;
        conversation_line_history = (ai_line_history *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
        halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + allocation_size;
        halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&allocation_size, 4);
    }

    conversation_index = 0;
    for (;;) {
        conversation_index_lookup[conversation_index] = -1;
        line = reinterpret_cast<ai_communication_line_definition *>(ai_communication_lines);
        position = 0;
        next_conversation_index = 0;
        for (;;) {
            if (next_conversation_index == conversation_index) {
                conversation_index_lookup[conversation_index] = position;
                break;
            }
            next_conversation_index = line[1].event_id;
            line++;
            position = position + 1;
            if (next_conversation_index == -1) {
                break;
            }
        }
        conversation_index = conversation_index + 1;
        if (0x38 < conversation_index) {
            break;
        }
    }

    dest = halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor;
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x358;
    {
        int32_t allocation_size = 0x358; // matches this allocation's own byte count exactly
        halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&allocation_size, 4);
    }

    header = dest;
    for (i = 0; i < 0x38; i++) {
        header[i] = 0;
    }
    strncpy((char *)dest, halo::ai::k_ai_conversation_data_name, 0x1f);
    halo::ai::globals().conversation_data = (data_array *)dest;
    halo::ai::globals().conversation_data->maximum_count = 8;
    halo::ai::globals().conversation_data->size = 0x64;
    halo::ai::globals().conversation_data->valid = 0;
    halo::ai::globals().conversation_data->signature = k_data_array_signature;
    halo::ai::globals().conversation_data->data = dest + 0x38;
}


/**
 * Behaviour of ai communication line fade multiplier, moved unchanged from the original free function.
 *
 * @address 0x42f8c0
 */
int16_t AiCommunication::line_fade_multiplier(uint32_t unit_index, int16_t priority, int16_t extra_delay, uint8_t follow_fallback, uint8_t apply_fade, float *volume, int32_t *chain_value, int16_t *dialogue_index, int16_t line_class)
{
    uint32_t last_spoke = (uint32_t)dialogue_index;
    int16_t status;

    status = (int16_t)halo::units::unit_animation_change_priority_check(unit_index, follow_fallback, priority, 1, &last_spoke,
        dialogue_index, chain_value);
    if (status == 1) {
        *volume = *volume * 0.3f;
    }
    if (apply_fade && line_class < 5 && last_spoke != halo::k_dword_none) {
        int32_t elapsed = halo::game::globals().game_time->game_time - (int32_t)last_spoke;
        int16_t limit;

        if (elapsed < 0) {
            elapsed = 0;
        }
        limit = (int16_t)(int32_t)(ai_communication_class_repeat_delay[line_class * 10] * halo::game::k_ticks_per_second_f + (float)(int32_t)extra_delay);
        if ((int16_t)elapsed <= limit) {
            *volume = 0.0f;
            return 0;
        }
        if ((int32_t)(int16_t)elapsed < (int32_t)limit + 0x3c) {
            *volume = (float)((int32_t)(int16_t)elapsed - (int32_t)limit) * *volume * (1.0f / 60.0f);
        }
    }
    return status;
}

namespace {

}

/**
 * Behaviour of ai communication play event line, moved unchanged from the original free function.
 *
 * @address 0x42eee0
 */
void AiCommunication::play_event_line(datum_index object_index, int16_t event_id, uint8_t force, datum_index explicit_speaker_actor_index, uint32_t *event_record)
{
    ai_communication_event_definition *row = reinterpret_cast<ai_communication_event_definition *>(ai_communication_event_definition_bytes());
    const ai_communication_target_result *record = reinterpret_cast<const ai_communication_target_result *>(event_record);
    int32_t row_index = 0;

    if (!halo::ai::globals().state->dialogue_triggers_enabled || event_id == -1) {
        return;
    }
    for (; row->event_id != -1; row++, row_index++) {
        unit_object *object;
        datum_index object_actor;
        int16_t class_index;
        int16_t priority;
        datum_index speaker_unit;
        unit_object *speaker;
        int16_t dialogue_index;
        int32_t chain = -1;
        int16_t delay;
        uint32_t unused_out = 0;
        int32_t status;

        if (row->event_id != event_id) {
            continue;
        }
        object = (unit_object *)halo::ai::object_bytes(object_index);
        object_actor = object->unit.actor_index;
        class_index = row->class_index;
        priority = ai_communication_class_priority[class_index];
        if (row->required_kind != -1 && row->required_kind != record->object_b) {
            continue;
        }
        if (halo::game::globals().game_time->game_time < halo::ai::globals().communication_quiet_until_tick && !(row->flags & 1)) {
            continue;
        }
        if (explicit_speaker_actor_index != k_datum_index_none) {
            speaker_unit = halo::ai::actor_at(explicit_speaker_actor_index)->unit_index;
        } else {
            int16_t mode = row->selection;
            datum_index found;

            if (mode == 3) {
                speaker_unit = record->target;
                if (halo::objects::object_try_and_get(speaker_unit, 3) == 0) {
                    continue;
                }
            } else if (mode == 2 || mode == 4) {
                struct actor *actor = object_actor != k_datum_index_none ? halo::ai::actor_at(object_actor) : 0;

                if (mode == 2 && actor != 0 && actor->encounter_index != k_datum_index_none) {
                    found = halo::ai::ai_communication_select_speaker_in_reference(9.0f, -1, (uint16_t)class_index,
                        (uint16_t)priority, (uint16_t)row->line_id, row->seat_filter, 0,
                        actor->encounter_index & halo::k_slot_mask, object_index, k_datum_index_none);
                } else {
                    found = halo::ai::ai_communication_select_speaker_by_team(mode == 2 ? 1 : 2, object_index, k_datum_index_none,
                        9.0f, -1, (uint16_t)class_index, (uint16_t)priority, (uint16_t)row->line_id,
                        row->seat_filter, 0, ((struct object *)object)->owner_team);
                }
                if (found == k_datum_index_none) {
                    continue;
                }
                speaker_unit = halo::ai::actor_at(found)->unit_index;
            } else {
                continue;
            }
        }
        if (speaker_unit == k_datum_index_none) {
            continue;
        }
        speaker = (unit_object *)halo::ai::object_bytes(speaker_unit);
        if (speaker->unit.controlling_player != k_datum_index_none) {
            continue;
        }
        if (!force) {
            float probability = row->probability;

            if (!(probability > 0.0f) || !(halo::math::random_real() < probability)) {
                continue;
            }
        }
        if (row->predicate != 0 && !row->predicate(object_index, event_record, speaker->unit.actor_index)) {
            continue;
        }
        dialogue_index = row->line_id;
        delay = (int16_t)(int32_t)(row->delay_seconds * halo::game::k_ticks_per_second_f);
        status = halo::units::unit_animation_change_priority_check(speaker_unit, 0, priority, 1, &unused_out, &dialogue_index, &chain);
        if ((int16_t)status <= 0) {
            continue;
        }

        {
            unit_speech speech;

            memset(&speech, 0, sizeof(speech));
            speech.priority = priority;
            speech.scream_type = dialogue_index;
            speech.sound_tag = (datum_index)chain;
            speech.delay_ticks = delay;
            speech.lipsync_ticks = (int16_t)(int32_t)(ai_communication_class_tail_seconds[class_index] * halo::game::k_ticks_per_second_f);
            speech.tail_ticks = 0x18;
            {
                ai_communication_target_result &target = halo::ai::speech_target(speech);

                target.target = object_index;
                target.event = -1;
                target.row = -1;
                target.object_b = -1;
                target.valid = 1;
            }
            halo::units::unit_commit_speech(speaker_unit, &speech, (int16_t)status);
            halo::ai::ai_communication_record_line_played(speaker_unit, priority, -1, (int16_t)row_index);
            halo::ai::actor_issue_order_or_vocalize(k_datum_index_none, speaker->unit.actor_index, object_index, 8,
                                          (int16_t)(uint16_t)ai_communication_class_follow_up[class_index]);
        }
        return;
    }
}


/**
 * Optionally reports the winning player's unit object index and its distance.
 *
 * @address 0x4303f0
 */
float AiCommunication::rate_player_proximity(uint8_t require_line_of_sight, datum_index *out_player_object_index, float *out_distance, datum_index object_index)
{
    object_marker self_marker;
    object_marker player_marker;
    real_point3d self_position;
    real_point3d player_position;
    real_vector3d to_self;
    data_iterator iterator;
    void *player;
    datum_index best_object_index;
    float best_score;
    float best_distance;
    uint8_t saw_any_player;
    uint8_t line_of_sight_clear;
    float dx, dy, dz, distance_squared, distance, score, facing;
    uint32_t walk, previous;
    int16_t self_cluster, player_cluster;
    object *player_object;
    uint8_t trace_scratch[96];

    best_object_index = (datum_index)k_datum_index_none;
    best_score = 0.0f;
    best_distance = 3.4028235e+38f;
    saw_any_player = 0;

    halo::objects::object_get_node_local_transform(object_index, ai_marker_name_a, &self_marker, 1);
    self_position = self_marker.node_transform.position;

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    for (player = halo::memory::data_iterator_next(&iterator); player != 0; player = halo::memory::data_iterator_next(&iterator)) {
            if (((struct player *)player)->unit != (datum_index)k_datum_index_none) {
                datum_index player_unit = ((struct player *)player)->unit;

                saw_any_player = 1;
                halo::objects::object_get_node_local_transform(player_unit, ai_marker_name_a, &player_marker, 1);
                player_position = player_marker.node_transform.position;
                dx = self_position.x - player_position.x;
                dy = self_position.y - player_position.y;
                dz = self_position.z - player_position.z;
                distance_squared = dz * dz + dy * dy + dx * dx;
                if (distance_squared < 900.0f) {
                    line_of_sight_clear = 0;
                    if (require_line_of_sight != 0) {
                        previous = (uint32_t)k_datum_index_none;
                        if (object_index != (datum_index)k_datum_index_none) {
                            walk = (uint32_t)object_index;
                            do {
                                previous = walk;
                                walk = (uint32_t)((object_header *)halo::objects::globals().object_data->data)
                                           [previous & halo::k_slot_mask].data->parent_object;
                            } while (walk != (uint32_t)k_datum_index_none);
                        }
                        self_cluster = ((object_header *)halo::objects::globals().object_data->data)
                                                        [previous & halo::k_slot_mask].data->location_cluster_index;
                        walk = (uint32_t)player_unit;
                        previous = (uint32_t)k_datum_index_none;
                        while (walk != (uint32_t)k_datum_index_none) {
                            previous = walk;
                            walk = (uint32_t)((object_header *)halo::objects::globals().object_data->data)
                                       [previous & halo::k_slot_mask].data->parent_object;
                        }
                        player_cluster = ((object_header *)halo::objects::globals().object_data->data)
                                                          [previous & halo::k_slot_mask].data->location_cluster_index;
                        if (self_cluster != -1 && player_cluster != -1) {
                        if ((halo::ai::cluster_visibility_row(global_structure_bsp_typed, self_cluster)[(int32_t)player_cluster >> 5] &
                             (1u << ((uint8_t)player_cluster & 0x1f))) == 0) {
                            continue;
                        }
                        }
                        to_self.i = dx;
                        to_self.j = dy;
                        to_self.k = dz;
                        line_of_sight_clear =
                            (distance_squared < 9.0f ||
                             halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::back_face | halo::collision_test_flag::double_sided | halo::collision_test_flag::structure_bsp), &player_position, &to_self,
                                                   (uint32_t)k_datum_index_none, (collision_result *)trace_scratch) == 0)
                                ? 1 : 0;
                    }

                    distance = (float)halo::libm::sqrt((double)distance_squared);
                    score = 1.0f;
                    if (distance < 15.0f) {
                        if (3.0f <= distance) {
                            score = (15.0f - distance) * 0.083333336f + 1.0f;
                        } else {
                            score = 2.0f;
                        }
                        if (line_of_sight_clear) {
                            score = score + 0.5f;
                        }
                        if (0.0001f < distance) {
                            player_object = ((object_header *)halo::objects::globals().object_data->data)
                                                [player_unit & halo::k_slot_mask].data;
                            facing = (halo::units::unit_data_of(player_object)->aiming_vector.k * dz +
                                      halo::units::unit_data_of(player_object)->aiming_vector.j * dy +
                                      halo::units::unit_data_of(player_object)->aiming_vector.i * dx) / distance;
                            if (0.70710677f < facing) {
                                score = (0.7f - (1.0f - facing) * 3.4142134f * 0.35f) + score;
                            }
                        }
                    }
                    if (best_score < score) {
                        best_object_index = player_unit;
                        best_score = score;
                        best_distance = distance;
                    }
                }
            }
    }
    if (!saw_any_player) {
        best_score = 1.0f;
    }
    if (out_distance != 0) {
        *out_distance = best_distance;
    }
    if (out_player_object_index != 0) {
        *out_player_object_index = best_object_index;
    }
    return best_score;
}


/**
 * The actor is rejected outright when it is not at least "alerted" (awareness_level < 2), when it controls
 * no unit, when neither subject object is inside `radius` of its aim origin, when a player-proximity gate
 * (flags bit 1) scores zero, when flags bit 2 demands object_a share this actor's active_unit_index, or
 * when the line has been spoken too recently. The score is then raised by how close each subject is
 * (through that subject's prop record) and by 5.0 when the seat filter matches.
 *
 * @address 0x42fb90
 */
float AiCommunication::rate_speaker(datum_index actor_index, datum_index object_b, real_point3d *position_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, real_point3d *position_a, datum_index object_a)
{
    actor *a;
    struct { float score; int32_t out_a; int32_t line_id; } scratch;
    uint8_t have_subject;
    uint8_t accept;
    uint8_t matched_a;
    uint8_t matched_b;
    float dx, dy, dz;
    float proximity;
    datum_index prop_index;
    prop *p;
    int16_t reach;
    int32_t reach_mode;

    a = halo::ai::actor_at(actor_index);

    have_subject = (object_a != (datum_index)k_datum_index_none ||
                    object_b != (datum_index)k_datum_index_none) ? 1 : 0;
    accept = (a->unit_index != (datum_index)k_datum_index_none) ? 1 : 0;
    scratch.score = 10.0f;
    scratch.out_a = -1;
    scratch.line_id = (int32_t)line_id;

    if (a->awareness_level < 2 || !accept) {
        return 0.0f;
    }

    if (have_subject) {
        if (object_a == (datum_index)k_datum_index_none ||
            ((dx = position_a->x - a->aim_origin.x,
              dy = position_a->y - a->aim_origin.y,
              dz = position_a->z - a->aim_origin.z),
             radius * radius <= dz * dz + dx * dx + dy * dy)) {
            if (object_b == (datum_index)k_datum_index_none ||
                ((dx = position_b->x - a->aim_origin.x,
                  dy = position_b->y - a->aim_origin.y,
                  dz = position_b->z - a->aim_origin.z),
                 radius * radius <= dx * dx + dy * dy + dz * dz)) {
                return 0.0f;
            }
        }
        accept = 1;
    }

    if ((flags & 2) != 0) {
        proximity = halo::ai::ai_communication_rate_player_proximity(0, 0, 0, a->unit_index);
        if (proximity == 0.0f) {
            return 0.0f;
        }
        scratch.score = proximity * 5.0f + 10.0f;
    }

    if ((flags & 4) != 0 && object_a != (datum_index)k_datum_index_none &&
        halo::ai::object_at(object_a)->parent_object !=
            a->active_unit_index) {
        return 0.0f;
    }

    if (seat_filter != -1 && halo::units::unit_scripted_action_animation_exists(a->unit_index, (int16_t)seat_filter) != 0 /* 0x42fd1d */) {
        scratch.score = scratch.score + 5.0f;
    }

    if ((int16_t)line_id != -1) {
        int32_t chain_value = -1;
        int16_t dialogue_index = (int16_t)line_id;

        if (halo::ai::ai_communication_line_fade_multiplier(a->unit_index, (int16_t)line_class, 0, (uint8_t)(flags & 1u), 1,
                &scratch.score, &chain_value, &dialogue_index, (int16_t)fade_limit) == 0) {
            return 0.0f;
        }
    }

    if (have_subject) {
        matched_a = 0;
        matched_b = 0;
        if (object_a != (datum_index)k_datum_index_none) {
            if (a->unit_index == object_a) {
                if ((flags & 8) == 0) {
                    accept = 0;
                } else {
                    matched_a = 1;
                }
            } else {
                prop_index = halo::ai::actor_find_or_create_shared_prop(object_a, actor_index, 1, 0);
                if (prop_index != (datum_index)k_datum_index_none) {
                    p = halo::ai::prop_at(prop_index);
                    if (p->distance <= radius) {
                        bool reachable = true;

                        reach_mode = 2;
                        if (p->state < 2 || 3 < p->state) {
                            if (p->enemy != 0) {
                                reachable = false;
                            } else if (allow_unreachable == 0 &&
                                p->auditory_perception < 2 &&
                                p->ambient_perception < 2) {
                                if (p->flashlight_on == 0) {
                                    reach_mode = (int32_t)static_cast<int8_t>(p->perception_range_class);
                                }
                                reach = halo::ai::actor_dispatch_look_handler_by_posture(p->obstruction,
                                                     actor_index, &a->aim_origin, &p->head_position,
                                                     (uint8_t)reach_mode, 1,
                                                     halo::ai::actor_target_get_priority_class(actor_index, prop_index));
                                if (reach < 2) {
                                    reachable = false;
                                }
                            }
                        }
                        if (reachable) {
                            scratch.score = (1.0f - p->distance / radius) * 10.0f + scratch.score;
                            matched_a = 1;
                        }
                    } else {
                        matched_a = 0;
                    }
                }
            }
        }
        if (object_b != (datum_index)k_datum_index_none) {
            if (a->unit_index == object_b) {
                if ((flags & 0x10) == 0) {
                    return 0.0f;
                }
                matched_b = 1;
            } else {
                prop_index = halo::ai::actor_find_prop_for_object(object_b, actor_index);
                if (prop_index != (datum_index)k_datum_index_none) {
                    p = halo::ai::prop_at(prop_index);
                    if (p->distance <= radius) {
                        if ((1 < p->state && p->state < 4) || p->enemy == 0) {
                            scratch.score = (1.0f - p->distance / radius) * 10.0f + scratch.score;
                            matched_b = 1;
                        }
                    } else {
                        matched_b = 0;
                    }
                }
            }
        }
        if (!accept) {
            return 0.0f;
        }
        accept = (uint8_t)(matched_b | matched_a);
    }

    if (accept) {
        return scratch.score;
    }
    return 0.0f;
}

/**
 * Behaviour of ai communication record line played, moved unchanged from the original free function.
 *
 * @address 0x42f9e0
 */
void AiCommunication::record_line_played(datum_index object_index, int16_t tier, int16_t communication_line_id, int16_t conversation_line_id)
{
    unit_object *obj;
    datum_index actor_index;
    int32_t current_tick;
    int32_t decay;
    int32_t stamp;
    int32_t category;
    ai_line_history *entry;
    int32_t *slot;

    obj = (unit_object *)halo::ai::object_at(object_index);
    actor_index = obj->unit.actor_index;

    current_tick = halo::game::globals().game_time->game_time;
    decay = obj->unit.speech_duration_ticks - 0x2d;
    if (decay < 0) {
        decay = 0;
    }
    stamp = decay + current_tick;
    obj->unit.communication_hold_tick = stamp;

    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    halo::ai::actor_recompute_grenade_eligibility(actor_index);
    category = halo::ai::actor_classify_communication_object_type(actor_index);
    if (category == -1) {
        return;
    }

    if (tier <= 5) {
        slot = &halo::ai::globals().state->loudest_line_tick[0][category];
        if (*slot <= stamp) {
            *slot = stamp;
        }
        if (tier >= 3) {
            slot = &halo::ai::globals().state->loudest_line_tick[1][category];
            if (*slot <= stamp) {
                *slot = stamp;
            }
        }
        if (tier >= 5) {
            slot = &halo::ai::globals().state->loudest_line_tick[2][category];
            if (*slot <= stamp) {
                *slot = stamp;
            }
        }
    }

    if (communication_line_id != -1) {
        float delay = ai_communication_line_table[communication_line_id * 0x28 / 4];

        entry = &communication_line_history[category + communication_line_id * 2];
        entry->last_tick = current_tick;
        if (delay > 0.0f) {
            entry->cooldown_until_tick = (int32_t)(delay * halo::game::k_ticks_per_second_f + (float)stamp);
        }
    }
    if (conversation_line_id != -1) {
        float delay = ai_conversation_line_table[conversation_line_id * 0x24 / 4];

        entry = &conversation_line_history[category + conversation_line_id * 2];
        entry->last_tick = current_tick;
        if (delay > 0.0f) {
            entry->cooldown_until_tick = (int32_t)(delay * halo::game::k_ticks_per_second_f + (float)stamp);
        }
    }
}


/**
 * Behaviour of ai communication reset, moved unchanged from the original free function.
 *
 * @address 0x42d230
 */
void AiCommunication::reset()
{
    int32_t i;
    int32_t entry_count;
    ai_line_history *entries;
    uint8_t *element;

    halo::ai::globals().state->dialogue_triggers_enabled = 1;
    for (i = 0; i < 3; i++) {
        halo::ai::globals().state->loudest_line_tick[i][0] = 0;
        halo::ai::globals().state->loudest_line_tick[i][1] = 0;
    }

    entries = communication_line_history;
    entry_count = communication_line_count * 2;
    for (i = 0; i < entry_count; i++) {
        entries[i].last_tick = -1;
        entries[i].cooldown_until_tick = -1;
    }

    entries = conversation_line_history;
    entry_count = conversation_line_count * 2;
    for (i = 0; i < entry_count; i++) {
        entries[i].last_tick = -1;
        entries[i].cooldown_until_tick = -1;
    }

    halo::ai::globals().state->conversation_event_count = 0;
    halo::ai::globals().state->conversation_event_cursor = 0;
    memset(halo::ai::globals().state->conversation_events, 0, sizeof(halo::ai::globals().state->conversation_events));

    halo::ai::globals().conversation_data->next_index = 0;
    halo::ai::globals().conversation_data->last_index = 0;
    halo::ai::globals().conversation_data->actual_count = 0;
    strncpy((char *)&halo::ai::globals().conversation_data->next_identifier, halo::ai::globals().conversation_data->name, 2);
    halo::ai::globals().conversation_data->next_identifier |= 0x8000;
    halo::ai::globals().conversation_data->valid = 1;

    for (i = 0; i < halo::ai::globals().conversation_data->maximum_count; i++) {
        element = (uint8_t *)halo::ai::globals().conversation_data->data + (int32_t)halo::ai::globals().conversation_data->size * i;
        *(int16_t *)element = 0;
    }
}


/**
 * Behaviour of ai communication select speaker by team, moved unchanged from the original free function.
 *
 * @address 0x4300d0
 */
datum_index AiCommunication::select_speaker_by_team(int16_t match_mode, datum_index object_a, datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team)
{
    object_marker marker_a;
    object_marker marker_b;
    real_point3d position;
    actor_iterator_state iterator;
    actor *a;
    datum_index best;
    float best_score;
    datum_index actor_index;
    int16_t other_team;
    uint8_t accept;
    int32_t pair;
    float score;

    best = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    if (object_a != (datum_index)k_datum_index_none) {
        halo::objects::object_get_node_local_transform(object_a, ai_marker_name_a, &marker_a, 1);
        position = marker_a.node_transform.position;
    }
    if (object_b != (datum_index)k_datum_index_none) {
        halo::objects::object_get_node_local_transform(object_a, ai_marker_name_a, &marker_b, 1);
        position = marker_b.node_transform.position;
    }

    if (halo::ai::globals().state->actors_valid) {
        iterator.filter_array = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.next_actor_index = -1;
    }

    a = halo::ai::actor_iterator_next(&iterator);
    if (a == 0) {
        return (datum_index)k_datum_index_none;
    }
    do {
        actor_index = (datum_index)iterator.actor_index;
        accept = 1;
        if (team != -1) {
            other_team = a->team;
            if (halo::game::globals().current_engine != 0) {
                accept = (uint8_t)(team != other_team);
            } else if (team >= 0 && team < 10 && other_team >= 0 && other_team < 10) {
                pair = (int32_t)other_team + (int32_t)team * 10;
                accept = (uint8_t)(((team_pair_data->enemy_bits[pair >> 5]) &
                                    (1u << ((uint8_t)pair & 0x1f))) == 0);
            }
            if (match_mode == 0) {
                accept = (uint8_t)(other_team == team);
            } else if (match_mode == 1) {
                accept = (uint8_t)(accept == 0);
            }
        }
        if (accept) {
            score = halo::ai::ai_communication_rate_speaker(actor_index, object_b, &position, radius,
                                                  allow_unreachable, fade_limit, line_class,
                                                  line_id, seat_filter, flags, &position,
                                                  object_a);
            if (best_score < score) {
                best_score = score;
                best = actor_index;
            }
        }
        a = halo::ai::actor_iterator_next(&iterator);
    } while (a != 0);

    return best;
}

/**
 * Behaviour of ai communication select speaker in reference, moved unchanged from the original free
 * function.
 *
 * @address 0x42ff80
 */
datum_index AiCommunication::select_speaker_in_reference(float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, uint32_t reference, datum_index object_a, datum_index object_b)
{
    object_marker marker;
    real_point3d position_a;
    real_point3d position_b;
    ai_reference_actor_iterator iterator;
    datum_index best;
    float best_score;
    datum_index actor_index;
    float score;

    best = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    if (reference == (uint32_t)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    if (object_a != (datum_index)k_datum_index_none) {
        halo::objects::object_get_node_local_transform(object_a, ai_marker_name_a, &marker, 1);
        position_a = marker.node_transform.position;
    }
    if (object_b != (datum_index)k_datum_index_none) {
        halo::objects::object_get_node_local_transform(object_b, ai_marker_name_a, &marker, 1);
        position_b = marker.node_transform.position;
    }

    halo::ai::ai_reference_actor_iterator_new(reference, &iterator);
    if (halo::ai::ai_reference_actor_iterator_next(&iterator) == 0) {
        return (datum_index)k_datum_index_none;
    }
    do {
        actor_index = iterator.actor_index;
        score = halo::ai::ai_communication_rate_speaker(actor_index, object_b, &position_b, radius,
                                              allow_unreachable, fade_limit, line_class, line_id,
                                              seat_filter, flags, &position_a, object_a);
        if (best_score < score) {
            best_score = score;
            best = actor_index;
        }
    } while (halo::ai::ai_reference_actor_iterator_next(&iterator) != 0);

    return best;
}

/**
 * Behaviour of ai communication target result reset, moved unchanged from the original free function.
 *
 * @address 0x42d310
 */
void AiCommunication::target_result_reset(ai_communication_target_result *record)
{
    memset(record, 0, sizeof(*record));
    record->target = (datum_index)k_datum_index_none;
    record->event = -1;
    record->row = -1;
    record->object_b = -1;
}

namespace {

class DialogueCondition_42f4f0 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f4f0, moved unchanged from the original free function.
 *
 * @address 0x42f4f0
 */
uint8_t DialogueCondition_42f4f0::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    datum_index prop_index;
    prop *p;

    if (actor_index == k_datum_index_none) {
        return 0;
    }
    prop_index = halo::ai::actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index == k_datum_index_none) {
        return 0;
    }
    p = halo::ai::prop_at(prop_index);
    if (p->distance > 5.0f) {
        return 1;
    }
    return (uint8_t)(p->obstruction != 0 && p->obstruction != 1);
}

}


namespace {

class DialogueCondition_42f560 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f560, moved unchanged from the original free function.
 *
 * @address 0x42f560
 */
uint8_t DialogueCondition_42f560::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    struct actor *actor;

    if (actor_index == k_datum_index_none) {
        return 0;
    }
    actor = halo::ai::actor_at(actor_index);
    if (actor->mode == halo::ai::actor_mode::uncover) {
        return (uint8_t)(actor->mode_data.uncover.stage == 1);
    }
    return (uint8_t)(actor->mode == halo::ai::actor_mode::search);
}

}


namespace {

class DialogueCondition_42f5b0 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f5b0, moved unchanged from the original free function.
 *
 * @address 0x42f5b0
 */
uint8_t DialogueCondition_42f5b0::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    datum_index own_actor;
    actor *a;
    actor *b;

    if (!halo::ai::actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    own_actor = halo::units::unit_data_of(halo::ai::object_at(object_index))->actor_index;
    if (own_actor == k_datum_index_none || actor_index == k_datum_index_none) {
        return 0;
    }
    a = halo::ai::actor_at(own_actor);
    b = halo::ai::actor_at(actor_index);
    return (uint8_t)(a->encounter_index != k_datum_index_none &&
        a->encounter_index == b->encounter_index &&
        a->platoon_index == b->platoon_index);
}

}


namespace {

class DialogueCondition_42f650 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f650, moved unchanged from the original free function.
 *
 * @address 0x42f650
 */
uint8_t DialogueCondition_42f650::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint8_t result = (uint8_t)(actor->combat_status >= 7);

    if (result && actor->mode == halo::ai::actor_mode::flee && actor->mode_data.flee.panic > 0) {
        result = 0;
    }
    return result;
}

}


namespace {

class DialogueCondition_42f690 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f690, moved unchanged from the original free function.
 *
 * @address 0x42f690
 */
uint8_t DialogueCondition_42f690::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    struct actor *actor;

    if (!halo::ai::actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    actor = halo::ai::actor_at(actor_index);
    if (actor->combat_status < 7) {
        return 0;
    }
    if (actor->mode == halo::ai::actor_mode::flee && actor->mode_data.flee.panic > 0) {
        return 0;
    }
    return 1;
}

}


namespace {

class DialogueCondition_42f6f0 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f6f0, moved unchanged from the original free function.
 *
 * @address 0x42f6f0
 */
uint8_t DialogueCondition_42f6f0::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    datum_index own_actor;
    datum_index a_target;
    datum_index b_target;

    if (!halo::ai::actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    own_actor = halo::units::unit_data_of(halo::ai::object_at(object_index))->actor_index;
    if (own_actor == k_datum_index_none || actor_index == k_datum_index_none) {
        return 0;
    }
    a_target = ((struct actor *)halo::ai::actor_bytes(own_actor))->target_unit_index;
    if (a_target == k_datum_index_none) {
        return 0;
    }
    b_target = ((struct actor *)halo::ai::actor_bytes(actor_index))->target_unit_index;
    if (b_target == k_datum_index_none) {
        return 0;
    }
    return (uint8_t)(((struct prop *)halo::ai::prop_bytes(a_target))->object_index == ((struct prop *)halo::ai::prop_bytes(b_target))->object_index);
}

}


namespace {

class DialogueCondition_42f7b0 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f7b0, moved unchanged from the original free function.
 *
 * @address 0x42f7b0
 */
uint8_t DialogueCondition_42f7b0::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    struct actor *actor;

    if (actor_index == k_datum_index_none) {
        return 0;
    }
    actor = halo::ai::actor_at(actor_index);
    return (uint8_t)(actor->awareness_level == 3 && actor->combat_status < 4);
}

}


namespace {

class DialogueCondition_42f7f0 final : public DialogueCondition {
public:
    uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const override;
};

/**
 * Behaviour of ai dialogue condition 42f7f0, moved unchanged from the original free function.
 *
 * @address 0x42f7f0
 */
uint8_t DialogueCondition_42f7f0::test(datum_index object_index, uint32_t param_2, datum_index actor_index) const
{
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->combat_status < 7) {
        return 0;
    }
    if (actor->mode == halo::ai::actor_mode::flee && actor->mode_data.flee.panic > 0) {
        return 0;
    }
    return (uint8_t)(actor->type == 0);
}

}


/**
 * Behaviour of ai dispatch queued order, moved unchanged from the original free function.
 *
 * @address 0x42f840
 */
void AiCommunication::dispatch_queued_order(ai_queued_order *order, datum_index prop_index, datum_index actor_index)
{
    int16_t count = order->target_count;
    datum_index target = order->object_a;
    int16_t variant = (int16_t)static_cast<uint16_t>(order->single_target);
    int16_t line = 9;

    if (count <= 0) {
        return;
    }
    if (count == 1) {
        prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];

        if (target == p->object_index) {
            line = 8;
        }
        halo::ai::actor_issue_order_or_vocalize(k_datum_index_none, actor_index, target, line, variant);
    } else if (count == 2) {
        halo::ai::actor_issue_multi_target_vocalization(line, actor_index, variant, target);
    }
}

/**
 * Behaviour of ai propagate communication reaction, moved unchanged from the original free function.
 *
 * @address 0x42e9c0
 */
void AiCommunication::propagate_communication_reaction(datum_index object_index, ai_communication_order *order)
{
    object *obj;
    int16_t object_team;
    object_marker marker;
    real_point3d position;
    bsp_leaf_reference *location;
    int16_t gate = 1;
    int16_t row = order->row;
    actor_iterator_state iterator;
    actor *a;

    if (order->order_type == 1) {
        halo::ai::ai_mark_recognized_objects_for_reaction((int16_t)(uint16_t)order->team_a, (int16_t)(uint16_t)order->team_b,
                                                order->status);
    }
    if (order->order_type == 0 && order->count <= 0) {
        return;
    }
    obj = halo::ai::object_at(object_index);
    object_team = obj->owner_team;
    location = reinterpret_cast<bsp_leaf_reference *>(&obj->location_leaf_index);
    halo::objects::object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1);
    position = marker.node_transform.position;
    if (row != -1 && reinterpret_cast<ai_communication_line_definition *>(ai_communication_lines)[row].class_index >= 4) {
        gate = 3;
    }
    if (obj->parent_object != k_datum_index_none) {
        location = reinterpret_cast<bsp_leaf_reference *>(&halo::ai::object_at(halo::objects::object_get_root_object_index(object_index))->location_leaf_index);
    }
    if (!halo::ai::globals().state->actors_valid) {
        return;
    }
    memset(&iterator, 0, sizeof(iterator));
    iterator.filter_array = halo::ai::globals().encounter_data;
    iterator.next_index = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
    iterator.encounterless_done = 0;
    iterator.active = 1;
    iterator.actor_index = k_datum_index_none;
    iterator.next_actor_index = -1;

    for (a = halo::ai::actor_iterator_next(&iterator); a != 0; a = halo::ai::actor_iterator_next(&iterator)) {
        uint8_t *ap = (uint8_t *)a;
        int16_t team = ((struct actor *)ap)->team;
        datum_index actor_index;
        datum_index prop_index;

        if (((struct actor *)ap)->unit_index == object_index) {
            continue;
        }
        if (halo::game::globals().current_engine != 0) {
            if (team != object_team) {
                continue;
            }
        } else {
            int32_t bit;

            if (team < 0 || team >= 10 || object_team < 0 || object_team >= 10) {
                continue;
            }
            bit = team * 10 + object_team;
            if (!(team_pair_data->enemy_bits[bit >> 5] & (1u << (bit & 0x1f)))) {
                continue;
            }
        }
        {
            float dx = position.x - ((struct actor *)ap)->aim_origin.x;
            float dy = position.y - ((struct actor *)ap)->aim_origin.y;
            float dz = position.z - ((struct actor *)ap)->aim_origin.z;

            if (!(dz * dz + dy * dy + dx * dx <= 900.0f)) {
                continue;
            }
        }
        actor_index = iterator.actor_index;
        prop_index = halo::ai::actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
        if (prop_index != k_datum_index_none) {
            prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
            actor_firing_positions firing;

            halo::ai::actor_get_firing_positions(actor_index, &firing, &position);
            if ((int16_t)halo::ai::actor_target_hearing_check(location, (int16_t)static_cast<uint16_t>(p->obstruction), actor_index,
                                           &firing, gate, &position) >= 2) {
                halo::ai::actor_dispatch_squad_order(prop_index, (const actor_squad_order_header *)order, actor_index);
                halo::ai::ai_dispatch_queued_order((ai_queued_order *)order, prop_index, actor_index);
            }
        }
    }
}


/**
 * Reports a recency-based weight (0..1) through out_weight when given.
 *
 * @address 0x42ec90
 */
int32_t AiCommunication::select_communication_target(uint32_t param_a, uint32_t param_b, int16_t line_id, int16_t sub_id, float *out_weight)
{
    ai_communication_event_definition *row;
    int32_t result;
    float weight;
    int32_t index;
    int16_t target_kind;
    int16_t candidate_a;
    int16_t candidate_b;
    uint32_t search_kind;
    unit_object *vehicle_obj;
    int16_t comm_kind;
    ai_line_history *timestamp_pair;
    int32_t now;

    result = -1;
    weight = 1.0f;

    if (halo::ai::globals().state->dialogue_triggers_enabled && line_id != -1) {
        index = 0;
        row = reinterpret_cast<ai_communication_event_definition *>(ai_communication_event_definition_bytes());
        do {
            if (row->event_id == line_id &&
                (row->required_kind == -1 || row->required_kind == sub_id)) {
                comm_kind = row->class_index;
                if ((halo::ai::globals().communication_quiet_until_tick <= halo::game::globals().game_time->game_time ||
                     (row->flags & 1) != 0) &&
                    0.0f < row->unknown_14) {
                    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                    if ((float)((uint32_t)halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f <
                        row->unknown_14) {
                        target_kind = row->selection;
                        if (target_kind == 2 || target_kind == 4) {
                            candidate_a = row->line_id;
                            candidate_b = row->seat_filter;
                            search_kind = (target_kind == 2) ? 1u : 2u;
                            result = halo::ai::ai_communication_select_speaker_by_team((int16_t)search_kind, param_a,
                                                   halo::k_dword_none, 9.0f, -1,
                                                   (uint32_t)(uint16_t)comm_kind,
                                                   (uint32_t)(uint16_t)ai_communication_class_priority[comm_kind],
                                                   (uint32_t)(uint16_t)candidate_a, candidate_b, 0,
                                                   halo::ai::object_at(param_a)->owner_team);
                        } else if (target_kind == 3) {
                            vehicle_obj = (unit_object *)halo::objects::object_try_and_get(param_b, 3);
                            result = -1;
                            if (vehicle_obj != 0) {
                                result = static_cast<int32_t>(vehicle_obj->unit.actor_index);
                            }
                        }

                        if (result != -1) {
                            int16_t comm_index = (int16_t)halo::ai::actor_classify_communication_object_type((datum_index)result);
                            if (comm_index != -1) {
                                now = halo::game::globals().game_time->game_time;
                                timestamp_pair = &conversation_line_history[comm_index + index * 2];
                                if (timestamp_pair->last_tick != -1) {
                                    weight = (float)(now - timestamp_pair->last_tick) * 0.0011111111f;
                                    if (0.0f <= weight) {
                                        if (1.0f < weight) {
                                            weight = 1.0f;
                                        }
                                    } else {
                                        weight = 0.0f;
                                    }
                                }
                                if (timestamp_pair->cooldown_until_tick != -1 && timestamp_pair->cooldown_until_tick != now &&
                                    -1 < timestamp_pair->cooldown_until_tick - now) {
                                    result = -1;
                                }
                            }
                        }
                    }
                }
                if (result != -1) {
                    break;
                }
            }
            index = index + 1;
            row++;
        } while (row->event_id != -1);
    }

    if (out_weight != 0) {
        *out_weight = weight;
    }
    return result;
}


namespace {

constexpr DialogueCondition_42f4f0 dialogue_condition_42f4f0;
constexpr DialogueCondition_42f560 dialogue_condition_42f560;
constexpr DialogueCondition_42f5b0 dialogue_condition_42f5b0;
constexpr DialogueCondition_42f650 dialogue_condition_42f650;
constexpr DialogueCondition_42f690 dialogue_condition_42f690;
constexpr DialogueCondition_42f6f0 dialogue_condition_42f6f0;
constexpr DialogueCondition_42f7b0 dialogue_condition_42f7b0;
constexpr DialogueCondition_42f7f0 dialogue_condition_42f7f0;

constexpr const DialogueCondition *k_dialogue_conditions[] = {
    &dialogue_condition_42f4f0,
    &dialogue_condition_42f560,
    &dialogue_condition_42f5b0,
    &dialogue_condition_42f650,
    &dialogue_condition_42f690,
    &dialogue_condition_42f6f0,
    &dialogue_condition_42f7b0,
    &dialogue_condition_42f7f0,
};

}

/**
 * Returns the dialogue condition registered at a table position.
 */
const DialogueCondition &dialogue_condition(int index)
{
    return *k_dialogue_conditions[index];
}

}
