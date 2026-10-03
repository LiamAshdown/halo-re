#include "halo/ai/airest_conversation.hpp"

#include <stdint.h>
#include <string.h>

extern "C" {
extern Scenario *global_scenario;
extern datum_index ai_conversation_new(int16_t conversation_definition_index, uint8_t allow_eviction);
extern int8_t ai_conversation_resolve_participants(datum_index instance_index, uint8_t *out_flag);
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b);
extern data_array *ai_conversation_data;
extern data_array *actor_data;
extern float ticks_per_second;
extern int32_t __ftol(double x);
extern void * data_iterator_next(data_iterator *iterator);
extern data_array *object_data;
extern game_time_globals *game_time;
extern int32_t ai_communication_quiet_until_tick;
extern void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale);
extern int32_t sound_impulse_time(datum_index sound_tag_handle);
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_communication_hold_tick, int16_t *dialogue_index, int32_t *chain_value);
extern int32_t unit_commit_speech(uint32_t unit_index, const void *source, int16_t mode);
extern ai_globals *ai_globals_ptr;
extern datum_index datum_new(data_array *array);
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array);
extern data_array *encounter_data;
extern datum_index *object_name_list;
extern uint32_t random_seed_global;
extern double sqrt(double x);
extern actor *actor_iterator_next(actor_iterator_state *iterator);
extern void ai_reference_actor_iterator_new(uint32_t reference, ai_reference_actor_iterator *iterator);
extern void *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator);
extern float ai_communication_rate_player_proximity(uint8_t require_line_of_sight, datum_index *out_player_object_index, float *out_distance, datum_index object_index);
extern int8_t teams_are_enemies(int16_t a, int16_t b);
extern data_array *prop_data;
extern data_array *player_data;
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern int32_t ai_conversation_get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index);
extern uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point);
extern int8_t ai_conversation_resolve_participant(int16_t participant_index, uint8_t *out_resolved, uint8_t *out_wants_alternate, uint8_t *out_blocked_by_player, float *inout_minimum_distance, datum_index conversation_index);
extern void datum_delete(data_array *array, datum_index handle);
extern uint8_t ai_conversation_activate_next_participant(datum_index instance_handle);
extern uint8_t ai_conversation_current_line_is_ready(datum_index instance_handle);
}

namespace halo::ai {

/**
 * Returns 1 if the conversation is now running.
 *
 * @address 0x4307c0
 */
uint8_t ConversationDefinitionView::activate(uint8_t allow_eviction)
{
    int16_t conversation_definition_index = handle;
    datum_index instance;
    uint8_t out_flag;
    int8_t active;

    if (conversation_definition_index < 0) {
        return 0;
    }
    if (conversation_definition_index < global_scenario->ai_conversations.count) {
        instance = ai_conversation_new(conversation_definition_index, allow_eviction);
        if (instance != (datum_index)k_datum_index_none) {
            out_flag = 0;
            active = ai_conversation_resolve_participants(instance, &out_flag);
            if (active != 0 || out_flag != 0) {
                return 1;
            }
            ai_conversation_stop(instance, 1, 0);
        }
    }
    return 0;
}

/**
 * FISTP-based float-to-int truncation Activates the participant referenced by the instance's current line
 * (ai_conversation unknown_48), once that participant has already been resolved (its bit set in
 * participant_mask). Records the participant's actor/unit on the instance, resolves the line's addressee
 * (if any) to a unit as well, picks up the line's sound-variant TagID and delay (converted to ticks), and
 * clears the per-line completion flags.
 *
 * @address 0x431d10
 */
uint8_t ConversationView::activate_next_participant()
{
    datum_index instance_handle = handle;
    ai_conversation *instance = &((ai_conversation *)ai_conversation_data->data)[instance_handle & 0xffff];
    ScenarioAIConversation *definition =
        &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
    ScenarioAIConversationLine *line =
        &((ScenarioAIConversationLine *)definition->lines.pointer)[instance->line_index];
    int16_t participant_index = line->participant;

    if (participant_index < 0 || participant_index >= definition->participants.count ||
        (instance->participant_mask & (1u << (participant_index & 0x1f))) == 0) {
        return 0;
    }

    participant_index = line->participant; // reloaded, matches the original's redundant re-read
    {
        ScenarioAIConversationParticipant *participants =
            (ScenarioAIConversationParticipant *)definition->participants.pointer;
        datum_index participant_actor_handle = instance->participant_actor[participant_index];

        instance->speaker_participant_index = participant_index;

        if (participant_actor_handle == (datum_index)k_datum_index_none) {
            instance->speaker_actor_index = (uint32_t)k_datum_index_none;
            instance->speaker_unit_index = (uint32_t)k_datum_index_none;
            instance->addressee_unit_index = (uint32_t)k_datum_index_none;
            instance->speaker_disembodied = 1;
        } else {
            actor *participant_actor = &((actor *)actor_data->data)[participant_actor_handle & 0xffff];
            int16_t selection_type;

            instance->speaker_actor_index = participant_actor_handle;
            instance->speaker_unit_index = participant_actor->unit_index;
            instance->addressee_unit_index = (uint32_t)k_datum_index_none;

            if (line->addressee == 1) {
                instance->addressee_unit_index = instance->player_unit_index;
            } else if (line->addressee == 2 && line->addressee_participant >= 0 &&
                       line->addressee_participant < definition->participants.count) {
                datum_index addressee_actor_handle = instance->participant_actor[line->addressee_participant];
                if (addressee_actor_handle != (datum_index)k_datum_index_none) {
                    instance->addressee_unit_index = ((actor *)actor_data->data)[addressee_actor_handle & 0xffff].unit_index;
                }
            }

            selection_type = participants[participant_index].selection_type;
            instance->speaker_disembodied = (selection_type == 6 || selection_type == 7) ? 1 : 0;
        }

        {
            TagDependency *variants = &line->variant_1;
            int16_t variant_selector = ((int16_t *)&instance->unknown_18)[participant_index];
            instance->sound_index = *(uint32_t *)&variants[variant_selector].tag_id;
        }

        instance->line_delay_ticks = (int16_t)__ftol((double)(line->line_delay_time * ticks_per_second));
        instance->line_flags = line->flags;
        instance->line_finished = 0;
        instance->line_spoken = 0;
        instance->line_started = 0;
    }

    return 1;
}

/**
 * Stops the instance if object_index was referenced anywhere in it.
 *
 * @address 0x430d30
 */
void Conversations::clear_object_references(datum_index object_index, uint8_t force_full_scan)
{
    data_iterator iterator;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    uint8_t referenced;
    int16_t i;
    int32_t participant_count;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
        referenced = 0;

        if (instance->speaker_unit_index == (int32_t)object_index) {
            instance->line_finished = 1;
            instance->speaker_unit_index = -1;
            referenced = 1;
        }
        if (instance->addressee_unit_index == (int32_t)object_index) {
            instance->addressee_unit_index = -1;
            referenced = 1;
        }
        if (instance->player_unit_index == object_index) {
            instance->player_unit_index = (datum_index)k_datum_index_none;
            referenced = 1;
        }

        if (force_full_scan != 0 || (definition->flags & 1) != 0) {
            participant_count = definition->participants.count;
            for (i = 0; i < participant_count; i++) {
                if ((instance->participant_mask & (1u << (i & 0x1f))) != 0 &&
                    instance->participant_actor[i] != (datum_index)k_datum_index_none) {
                    actor *a = &((actor *)actor_data->data)[instance->participant_actor[i] & 0xffff];
                    if (a->unit_index == object_index) {
                        referenced = 1;
                    }
                    if (force_full_scan != 0) {
                        if (a->mode == 12 && *(int32_t *)(a->mode_data.raw + 0x0c) == (int32_t)object_index) {
                            *(int32_t *)(a->mode_data.raw + 0x0c) = -1;
                        }
                        if (a->conversation_participant == object_index) {
                            a->conversation_participant = (datum_index)k_datum_index_none;
                        }
                    }
                }
            }
            if (referenced) {
                ai_conversation_stop(iterator.index, 0, 0);
                return;
            }
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai conversation clear participant, moved unchanged from the original free function.
 *
 * @address 0x430c70
 */
void Conversations::clear_participant(datum_index actor_index)
{
    data_iterator iterator;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    int16_t i;
    int32_t participant_count;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
        participant_count = definition->participants.count;

        for (i = 0; i < participant_count; i++) {
            if (instance->participant_actor[i] == actor_index) {
                if ((definition->flags & 1) != 0) {
                    ai_conversation_stop(iterator.index, 0, 0);
                    break;
                }
                instance->participant_mask &= ~(1u << (i & 0x1f));
                instance->participant_actor[i] = (datum_index)k_datum_index_none;
                if (instance->speaker_participant_index == i) {
                    instance->line_finished = 1;
                }
            }
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai conversation current line is ready, moved unchanged from the original free function.
 *
 * @address 0x431e70
 */
uint8_t ConversationView::current_line_is_ready()
{
    datum_index instance_handle = handle;
    uint8_t *inst = (uint8_t *)ai_conversation_data->data + (instance_handle & 0xffff) * 0x64;
    uint8_t *definition = *(uint8_t **)((uint8_t *)global_scenario + 0x46c) + *(int16_t *)(inst + 0x2) * 0x74;

    if (inst[0x63]) {
        return inst[0x63];
    }
    if (!inst[0x61]) {
        uint8_t blocked = 0;
        datum_index sound = *(datum_index *)(inst + 0x5c);

        if (sound != k_datum_index_none) {
            uint16_t flags = *(uint16_t *)(inst + 0x4e);

            if (flags & 0x30) {
                int16_t i;

                for (i = 0; (int32_t)i < *(int32_t *)(definition + 0x50); i++) {
                    datum_index actor_index = *(datum_index *)(inst + 0x28 + i * 4);
                    uint8_t *a;

                    if (actor_index == k_datum_index_none) {
                        continue;
                    }
                    if (!(flags & 0x20) && !((flags & 0x10) && actor_index == *(datum_index *)(inst + 0x50))) {
                        continue;
                    }
                    a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
                    if (((actor *)a)->mode == 0xc && *(datum_index *)(a + 0xa8) != k_datum_index_none &&
                        !a[0xa1] && !a[0xa0]) {
                        blocked = 1;
                    }
                }
            }
            if (game_time->game_time < ai_communication_quiet_until_tick || blocked) {
                return inst[0x63];
            }
            if (*(datum_index *)(inst + 0x54) != k_datum_index_none && !inst[0x60]) {
                int16_t dialogue_index = -1;
                int32_t chain_value = (int32_t)sound;
                int16_t result = (int16_t)unit_animation_change_priority_check(*(datum_index *)(inst + 0x54), 0, 6, 1, 0,
                    &dialogue_index, &chain_value);

                if (result == 1) {
                    return inst[0x63];
                }
                if (result > 0) {
                    uint8_t speech[0x30];

                    memset(speech, 0, sizeof(speech));
                    *(int16_t *)(speech + 0x0) = 6;
                    *(int16_t *)(speech + 0x2) = -1;
                    *(datum_index *)(speech + 0x4) = sound;
                    *(datum_index *)(speech + 0x10) = *(datum_index *)(inst + 0x58);
                    *(int16_t *)(speech + 0x14) = -1;
                    *(int16_t *)(speech + 0x18) = -1;
                    *(int16_t *)(speech + 0x16) = -1;
                    *(int16_t *)(speech + 0x1c) = 1;
                    *(int16_t *)(speech + 0x1e) = 1;
                    *(datum_index *)(speech + 0x20) = *(datum_index *)(inst + 0x54);
                    *(int16_t *)(speech + 0x24) = 0;
                    unit_commit_speech(*(datum_index *)(inst + 0x54), speech, result);
                }
            } else {
                sound_impulse_start(k_datum_index_none, sound, 1.0f);
            }
        }
        inst[0x61] = 1;
        inst[0x5] = 1;
    }
    if (!inst[0x61]) {
        return inst[0x63];
    }
    if (!inst[0x62]) {
        uint8_t done;

        if (*(datum_index *)(inst + 0x54) == k_datum_index_none) {
            done = !(*(datum_index *)(inst + 0x5c) != k_datum_index_none &&
                     sound_impulse_time(*(datum_index *)(inst + 0x5c)) != 0);
        } else {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[*(datum_index *)(inst + 0x54) & 0xffff].data;

            done = ((unit_object *)unit)->unit.current_speech.priority != 6;
        }
        inst[0x62] = done;
        if (!done) {
            return inst[0x63];
        }
    }
    if (*(int16_t *)(inst + 0x4c) > 0) {
        *(int16_t *)(inst + 0x4c) = (int16_t)(*(int16_t *)(inst + 0x4c) - 1);
        return inst[0x63];
    }
    inst[0x63] = 1;
    if (*(uint16_t *)(inst + 0x4e) & 0x8) {
        if (!inst[0x8]) {
            inst[0x8] = 1;
            inst[0x9] = 0;
        }
        if (inst[0x9]) {
            inst[0x8] = 0;
            return inst[0x63];
        }
        inst[0x63] = 0;
    }
    return inst[0x63];
}

/**
 * Behaviour of ai conversation get run to player range, moved unchanged from the original free function.
 *
 * @address 0x402cf0
 */
int32_t Conversations::get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index)
{
    ai_conversation *conv = &((ai_conversation *)ai_conversation_data->data)[conversation_index & 0xffff];
    ScenarioAIConversation *conversations = (ScenarioAIConversation *)global_scenario->ai_conversations.pointer;
    ScenarioAIConversation *def = &conversations[conv->definition_index];
    float distance = def->run_to_player_dist;

    out->conversation_index = 0;
    out->unknown_04 = 0;
    out->run_to_player_dist = 0.0f;
    out->player_unit_index = 0;
    out->unknown_10 = 0;

    out->conversation_index = conversation_index;
    out->run_to_player_dist = distance;
    if (distance == 0.0f) {
        out->player_unit_index = -1;
        out->unknown_10 = 0xffffffff;
        return 1;
    }
    out->player_unit_index = conv->player_unit_index;
    out->unknown_10 = 0xffffffff;
    return 1;
}

/**
 * If none are live, checks the recent-stop event ring for the most recently stopped instance of this
 * definition and reports its outcome as 5 (aborted) or 6/7 (finished, flagged/plain). Returns 0 if nothing
 * at all is found.
 *
 * @address 0x430830
 */
int32_t ConversationDefinitionView::get_status()
{
    int16_t conversation_definition_index = handle;
    data_iterator iterator;
    ai_conversation *instance;
    int32_t best;
    int32_t status;
    int16_t i;
    int16_t best_index;
    int32_t best_tick;
    ai_conversation_event *event;

    best = 0;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            if (instance->active == 0) {
                status = 1;
            } else if (instance->started == 0) {
                status = 2;
            } else {
                status = (instance->waiting_for_advance != 0) + 3;
            }
            if ((uint16_t)best <= (uint16_t)status) {
                best = status;
            }
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
    if ((int16_t)best != 0) {
        return best;
    }

    best_tick = -1;
    best_index = -1;
    if (0 < ai_globals_ptr->conversation_event_count) {
        for (i = 0; i < ai_globals_ptr->conversation_event_count; i++) {
            event = &ai_globals_ptr->conversation_events[i];
            if (event->definition_index == conversation_definition_index && best_tick < event->tick) {
                best_tick = event->tick;
                best_index = i;
            }
        }
        if (best_index != -1) {
            event = &ai_globals_ptr->conversation_events[best_index];
            if (event->reason_a == 0) {
                return 7 - (event->reason_b != 0);
            }
            return 5;
        }
    }
    return -1;
}

/**
 * Returns the line index of the running instance of the conversation definition, or 999 when no instance is running (hs
 * ai_conversation_line).
 *
 * @address 0x430960
 */
int16_t ConversationDefinitionView::get_line_index()
{
    int16_t conversation_definition_index = handle;
    data_iterator iterator;
    ai_conversation *instance;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            return instance->line_index;
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
    return 999;
}

/**
 * Behaviour of ai conversation mark all, moved unchanged from the original free function.
 *
 * @address 0x430a20
 */
void ConversationDefinitionView::mark_all()
{
    int16_t conversation_definition_index = handle;
    data_iterator iterator;
    ai_conversation *instance;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            instance->advance = 1;
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

/**
 * If the pool is full and allow_eviction is set, picks the instance with the lowest priority (ties broken
 * by the oldest start_tick), stops it, and re-allocates at that same slot. On success, initializes the new
 * instance's definition index, resets its line cursor to -1, records allow_eviction as its own priority,
 * and stamps its start_tick to the current tick.
 *
 * @address 0x431590
 */
datum_index ConversationDefinitionView::create(uint8_t allow_eviction)
{
    int16_t conversation_definition_index = handle;
    datum_index handle;
    data_iterator iterator;
    ai_conversation *candidate;
    uint8_t best_priority;
    int32_t best_tick;
    datum_index best_handle;
    ai_conversation *instance;

    handle = datum_new(ai_conversation_data);
    if (handle == (datum_index)k_datum_index_none) {
        if (allow_eviction != 0) {
            best_priority = 1;
            best_tick = 0x7fffffff;
            best_handle = (datum_index)k_datum_index_none;

            iterator.data = ai_conversation_data;
            iterator.next_index = 0;
            iterator.index = (datum_index)k_datum_index_none;
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            candidate = (ai_conversation *)data_iterator_next(&iterator);
            while (candidate != 0) {
                if (candidate->priority < best_priority || candidate->start_tick < best_tick) {
                    best_tick = candidate->start_tick;
                    best_priority = candidate->priority;
                    best_handle = iterator.index;
                }
                candidate = (ai_conversation *)data_iterator_next(&iterator);
            }

            if (best_handle != (datum_index)k_datum_index_none) {
                ai_conversation_stop(best_handle, 0, 0);
                handle = datum_new_at_index_with_salt(best_handle, ai_conversation_data);
            }
        }
    }

    if (handle != (datum_index)k_datum_index_none) {
        instance = &((ai_conversation *)ai_conversation_data->data)[handle & 0xffff];
        instance->definition_index = conversation_definition_index;
        instance->line_index = -1;
        instance->priority = allow_eviction;
        instance->start_tick = game_time->game_time;
    }
    return handle;
}

/**
 * Returns 1 when a participant was resolved.
 *
 * @address 0x431680
 */
int8_t Conversations::resolve_participant(int16_t participant_index, uint8_t *out_resolved, uint8_t *out_wants_alternate, uint8_t *out_blocked_by_player, float *inout_minimum_distance, datum_index conversation_index)
{
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ScenarioAIConversationParticipant *participant;
    int16_t selection_type;
    datum_index best_actor;
    uint32_t best_variant;
    float best_score;
    float best_distance;
    int8_t resolved;
    uint8_t blocked;
    uint8_t use_named_object;
    uint8_t use_reference;
    uint8_t is_alternate_kind;
    datum_index named_object;
    ai_reference_actor_iterator reference_iterator;
    actor_iterator_state actor_iterator;
    actor *candidate;
    datum_index candidate_index;
    datum_index player_object_index;
    float player_distance;
    float player_score;
    float score;
    int16_t slot;
    int32_t i;
    float positions[24];
    uint16_t resolved_count;
    uint16_t variant_candidates[8];
    int16_t variant_candidate_count;
    uint32_t zero_variant_slot;
    uint8_t have_zero_variant;
    int16_t candidate_variant;
    int16_t participant_variant;
    uint32_t chosen_variant;
    object *player_object;
    float dx, dy, dz, nearest;

    instance = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                   (conversation_index & 0xffff) * k_ai_conversation_size);
    definition = (ScenarioAIConversation *)((uint8_t *)(uintptr_t)global_scenario->ai_conversations.pointer +
                                            (int32_t)instance->definition_index * 0x74);
    participant = (ScenarioAIConversationParticipant *)
        ((uint8_t *)(uintptr_t)definition->participants.pointer + (int32_t)participant_index * 0x54);
    selection_type = (int16_t)participant->selection_type;

    best_actor = (datum_index)k_datum_index_none;
    best_variant = (uint32_t)k_datum_index_none;
    best_distance = 3.4028235e+38f;
    blocked = 0;
    resolved = 0;

    if (selection_type == 1) {
        best_variant = (best_variant & 0xffff0000u);
        resolved = 1;
        goto commit;
    }

    is_alternate_kind = (uint8_t)(selection_type == 6 || selection_type == 7);
    use_named_object = 0;
    use_reference = 0;
    named_object = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    resolved_count = 0;
    for (i = 0; i < definition->participants.count; i++) {
        datum_index other = instance->participant_actor[i];
        if (other != (datum_index)k_datum_index_none) {
            actor *o = (actor *)((uint8_t *)actor_data->data + (other & 0xffff) * k_actor_size);
            positions[resolved_count * 3 + 0] = o->body_position.x;
            positions[resolved_count * 3 + 1] = o->body_position.y;
            positions[resolved_count * 3 + 2] = o->body_position.z;
            resolved_count = (uint16_t)(resolved_count + 1);
        }
    }

    if ((int16_t)participant->use_this_object == -1) {
        if (*(int32_t *)&((struct ScenarioAIConversationParticipant *)participant)->encounter_index == -1) {
            if (ai_globals_ptr->actors_valid) {
                actor_iterator.filter_array = encounter_data;
                actor_iterator.next_index = 0;
                actor_iterator.cursor = -1;
                actor_iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
                actor_iterator.encounterless_done = 0;
                actor_iterator.active = 1;
                actor_iterator.actor_index = -1;
                actor_iterator.next_actor_index = -1;
            }
        } else {
            ai_reference_actor_iterator_new(
                ((struct ScenarioAIConversationParticipant *)participant)->encounter_index, &reference_iterator);
            use_reference = 1;
        }
    } else if ((int16_t)participant->use_this_object < 0 ||
               0x1ff < (int16_t)participant->use_this_object) {
        named_object = (datum_index)k_datum_index_none;
        use_named_object = 1;
    } else {
        named_object = object_name_list[(int16_t)participant->use_this_object];
        use_named_object = 1;
    }

    for (;;) {
        player_score = 0.0f;
        player_distance = 3.4028235e+38f;

        if (use_named_object) {
            object *obj = 0;
            void *element = 0;
            if (named_object != (datum_index)k_datum_index_none) {
                int16_t index = (int16_t)named_object;
                if (index >= 0 && index < object_data->maximum_count) {
                    int32_t byte_offset = (int32_t)object_data->size * (int32_t)index;
                    int16_t identifier = *(int16_t *)((uint8_t *)object_data->data + byte_offset);
                    int16_t salt = (int16_t)((uint32_t)named_object >> 16);
                    if (identifier != 0 && (salt == 0 || identifier == salt)) {
                        element = (uint8_t *)object_data->data + byte_offset;
                    }
                }
            }
            if (element != 0 && ((1 << (*((uint8_t *)element + 3) & 0x1f)) & 3u) != 0) {
                obj = *(object **)((uint8_t *)element + 8);
            }
            candidate = 0;
            candidate_index = (datum_index)k_datum_index_none;
            if (obj != 0) {
                datum_index a = *(datum_index *)((uint8_t *)obj + 0x1f4);
                if (a != (datum_index)k_datum_index_none) {
                    candidate = (actor *)((uint8_t *)actor_data->data + (a & 0xffff) * k_actor_size);
                    candidate_index = a;
                }
            }
            named_object = (datum_index)k_datum_index_none;
        } else if (use_reference) {
            candidate = (actor *)ai_reference_actor_iterator_next(&reference_iterator);
            candidate_index = reference_iterator.actor_index;
        } else {
            candidate = actor_iterator_next(&actor_iterator);
            candidate_index = (datum_index)actor_iterator.actor_index;
        }

        if (candidate == 0) {
            break;
        }

        if (candidate->unit_index == (datum_index)k_datum_index_none ||
            candidate->type != (int16_t)participant->actor_type) {
            continue;
        }

        slot = 0;
        for (i = 0; i < definition->participants.count; i++) {
            if (candidate_index == instance->participant_actor[i]) {
                break;
            }
            slot = (int16_t)(slot + 1);
            if (slot >= definition->participants.count) {
                break;
            }
        }
        if (definition->participants.count <= slot) {
            player_score = ai_communication_rate_player_proximity(
                (uint8_t)(resolved_count == 0), &player_object_index, &player_distance,
                candidate->unit_index);
            if (player_object_index == (datum_index)k_datum_index_none) {
                if (!is_alternate_kind) {
                    blocked = 1;
                    continue;
                }
                player_object = 0;
                score = 0.0f;
            } else {
                player_object = ((object_header *)object_data->data)
                                    [player_object_index & 0xffff].data;
                score = player_score;
            }

            switch ((int16_t)participant->selection_type) {
            case 0:
            case 6:
                if (player_object != 0 &&
                    teams_are_enemies(((struct object *)player_object)->owner_team,
                                      candidate->team) != 0) {
                    continue;
                }
                break;
            case 2:
                if (player_object == 0 ||
                    player_object->parent_object == (datum_index)k_datum_index_none ||
                    candidate->active_unit_index != player_object->parent_object) {
                    continue;
                }
                if (candidate->vehicle_gunner != 0) {
                    score = score + 1.0f;
                }
                break;
            case 3:
                if (candidate->active_unit_index != (datum_index)k_datum_index_none) {
                    continue;
                }
                break;
            case 4:
            case 7:
                if (candidate->counts_toward_encounter != 0) {
                    score = score + 1.5f;
                }
                break;
            default:
                break;
            }

            if (resolved_count == 0 && !is_alternate_kind && player_score < 2.0f &&
                definition->run_to_player_dist == 0.0f) {
                blocked = 1;
                continue;
            }

            if (0 < (int16_t)resolved_count) {
                nearest = 3.4028235e+38f;
                for (i = 0; i < (int32_t)resolved_count; i++) {
                    dx = positions[i * 3 + 0] - candidate->body_position.x;
                    dy = positions[i * 3 + 1] - candidate->body_position.y;
                    dz = positions[i * 3 + 2] - candidate->body_position.z;
                    dx = dz * dz + dy * dy + dx * dx;
                    if (dx < nearest) {
                        nearest = dx;
                    }
                }
                if (nearest < 20.25f) {
                    score = (1.0f - ((float)sqrt((double)nearest) - 1.5f) * 0.33333334f) + score;
                }
            }

            candidate_variant = *(int16_t *)((uint8_t *)((object_header *)object_data->data)
                                                 [candidate->unit_index & 0xffff].data + 0xbe);
            variant_candidate_count = 0;
            zero_variant_slot = (uint32_t)k_datum_index_none;
            have_zero_variant = 0;
            chosen_variant = 0;
            for (i = 0; i < 6; i++) {
                participant_variant = (int16_t)participant->variant_numbers[i];
                if (participant_variant != -1) {
                    if (participant_variant == candidate_variant) {
                        score = score + 0.7f;
                        chosen_variant = (uint32_t)i;
                        goto have_variant;
                    }
                    if (participant_variant == 0) {
                        have_zero_variant = 1;
                        zero_variant_slot = (uint32_t)i;
                    } else if (candidate_variant < 100 && participant_variant < 100) {
                        variant_candidates[variant_candidate_count] = (uint16_t)i;
                        variant_candidate_count = (int16_t)(variant_candidate_count + 1);
                    }
                }
            }
            if (have_zero_variant) {
                score = score + 0.7f;
                chosen_variant = zero_variant_slot;
            } else {
                if (variant_candidate_count < 1) {
                    continue;
                }
                chosen_variant = variant_candidates[0];
                if (variant_candidate_count != 1) {
                    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                    chosen_variant = (uint32_t)variant_candidates
                        [(int16_t)(((int32_t)variant_candidate_count *
                                    (int32_t)(random_seed_global >> 0x10)) >> 0x10)];
                }
            }
have_variant:
            if (best_score < score) {
                best_actor = candidate_index;
                best_score = score;
                best_distance = player_distance;
                resolved = 1;
                best_variant = chosen_variant;
            }
        }
    }

    if (resolved != 0) {
        goto commit;
    }
    if ((participant->flags & 2) == 0) {
        goto report;
    }
    if (out_wants_alternate != 0) {
        *out_wants_alternate = 1;
    }
    goto report;

commit:
    instance->participant_mask = instance->participant_mask | (1u << ((uint8_t)participant_index & 0x1f));
    instance->participant_actor[participant_index] = best_actor;
    *(int16_t *)((uint8_t *)instance + 0x18 + participant_index * 2) = (int16_t)best_variant;
    if (out_resolved != 0 && best_actor != (datum_index)k_datum_index_none) {
        *out_resolved = 1;
    }

report:
    if (blocked && out_blocked_by_player != 0) {
        *out_blocked_by_player = 1;
    }
    if (inout_minimum_distance != 0 && best_distance < *inout_minimum_distance) {
        *inout_minimum_distance = best_distance;
    }
    return resolved;
}

/**
 * When it can run, each participant's unit is stamped with its chosen dialogue variant, optionally renamed
 * into the scenario object-name table, and optionally switched into the conversation mode. Returns 1 when
 * the conversation is ready; *out_keep_trying reports the definition's keep_trying_to_play bit.
 *
 * @address 0x430fc0
 */
uint8_t ConversationView::resolve_participants(uint8_t *out_keep_trying)
{
    datum_index conversation_index = handle;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ScenarioAIConversationParticipant *participants;
    int16_t *variant_slots;
    uint32_t blocked_mask;
    uint8_t any_resolved;
    uint8_t wants_alternate;
    uint8_t alternate_resolved;
    uint8_t ready;
    uint8_t keep_trying;
    uint8_t blocked_this_slot;
    float minimum_distance;
    int32_t i;
    int16_t index;
    uint16_t participant_flags;
    uint32_t bit;
    uint8_t found_looking;
    data_iterator iterator;
    void *player;
    float nearest;
    float best_player_distance;
    datum_index player_unit;
    datum_index prop_index;
    prop *p;
    int32_t j;
    datum_index actor_handle;
    datum_index unit_index;
    object *unit_object;
    int16_t object_name;
    int16_t variant;
    uint16_t definition_flags;

    instance = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                   (conversation_index & 0xffff) * k_ai_conversation_size);
    definition = (ScenarioAIConversation *)((uint8_t *)(uintptr_t)
                                                global_scenario->ai_conversations.pointer +
                                            (int32_t)instance->definition_index * 0x74);
    participants = (ScenarioAIConversationParticipant *)(uintptr_t)definition->participants.pointer;
    variant_slots = (int16_t *)((uint8_t *)instance + 0x18);

    instance->participant_mask = 0;
    for (i = 0; i < 8; i++) {
        instance->participant_actor[i] = (datum_index)k_datum_index_none;
    }
    for (i = 0; i < 8; i++) {
        variant_slots[i] = -1;
    }

    blocked_mask = 0;
    wants_alternate = 0;
    alternate_resolved = 0;
    any_resolved = 0;
    ready = 1;
    keep_trying = 0;
    minimum_distance = 3.4028235e+38f;

    if (0 < (int32_t)definition->participants.count) {
        index = 0;
        i = 0;
        do {
            if ((participants[i].flags & 4) == 0) {
                blocked_this_slot = 0;
                ai_conversation_resolve_participant(index, &any_resolved, &wants_alternate,
                                                    &blocked_this_slot, &minimum_distance,
                                                    conversation_index);
                if (blocked_this_slot == 0) {
                    blocked_mask = blocked_mask & ~(1u << ((uint8_t)i & 0x1f));
                } else {
                    blocked_mask = blocked_mask | (1u << ((uint8_t)i & 0x1f));
                }
            }
            index = (int16_t)(index + 1);
            i = (int32_t)index;
        } while (i < (int32_t)definition->participants.count);

        if (wants_alternate != 0 && 0 < (int32_t)definition->participants.count) {
            index = 0;
            i = 0;
            do {
                bit = 1u << ((uint8_t)i & 0x1f);
                if ((bit & instance->participant_mask) == 0 && (participants[i].flags & 4) != 0) {
                    blocked_this_slot = 0;
                    if (ai_conversation_resolve_participant(index, &any_resolved, 0,
                                                            &blocked_this_slot, &minimum_distance,
                                                            conversation_index) == 0) {
                        if (blocked_this_slot == 0) {
                            blocked_mask = blocked_mask & ~bit;
                        } else {
                            blocked_mask = blocked_mask | bit;
                        }
                    } else {
                        alternate_resolved = 1;
                    }
                }
                index = (int16_t)(index + 1);
                i = (int32_t)index;
            } while (i < (int32_t)definition->participants.count);
        }
    }

    index = 0;
    if (0 < (int32_t)definition->participants.count) {
        i = 0;
        do {
            participant_flags = participants[i].flags;
            if ((participant_flags & 1) == 0 &&
                (instance->participant_mask & (1u << ((uint8_t)i & 0x1f))) == 0 &&
                ((participant_flags & 2) == 0 || alternate_resolved == 0) &&
                ((participant_flags & 4) == 0 || wants_alternate != 0)) {
                ready = 0;
                if ((blocked_mask & (1u << ((uint8_t)index & 0x1f))) == 0) {
                    goto clear_wait;
                }
                goto blocked_but_trying;
            }
            index = (int16_t)(index + 1);
            i = (int32_t)index;
        } while (i < (int32_t)definition->participants.count);
    }

    if ((definition->flags & 0x40) != 0 && 0.0f < definition->trigger_distance &&
        any_resolved != 0 && definition->trigger_distance < minimum_distance) {
blocked_but_trying:
        ready = 0;
        keep_trying = 1;
    }

clear_wait:
    instance->player_unit_index = -1;
    if (ready == 0) {
        goto check_keep_trying;
    }

    if ((definition->flags & 0x10) == 0) {
        goto check_looking;
    }

    if (any_resolved == 0) {
        ready = 0;
        goto check_keep_trying;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    best_player_distance = 3.4028235e+38f;
    player = data_iterator_next(&iterator);
    while (player != 0) {
        player_unit = ((struct player *)player)->unit;
        if (player_unit != (datum_index)k_datum_index_none) {
            nearest = 3.4028235e+38f;
            for (j = 0; j < (int32_t)definition->participants.count; j++) {
                if (instance->participant_actor[j] != (datum_index)k_datum_index_none) {
                    prop_index = actor_find_prop_for_object(player_unit, instance->participant_actor[j]);
                    if (prop_index != (datum_index)k_datum_index_none) {
                        p = (prop *)((uint8_t *)prop_data->data +
                                     (prop_index & 0xffff) * k_prop_size);
                        if (1 < p->state && p->state < 4 && p->distance < nearest) {
                            nearest = p->distance;
                        }
                    }
                }
            }
            if (nearest < best_player_distance) {
                best_player_distance = nearest;
                instance->player_unit_index = (int32_t)player_unit;
            }
        }
        player = data_iterator_next(&iterator);
    }
    if (instance->player_unit_index != -1) {
        goto check_looking;
    }
    definition_flags = definition->flags;
    goto not_ready_check_retry;

check_looking:
    if ((definition->flags & 0x80) == 0 || any_resolved == 0) {
        goto apply;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    found_looking = 0;
    player = data_iterator_next(&iterator);
    while (player != 0) {
        if (found_looking != 0) {
            goto apply;
        }
        if (((struct player *)player)->unit != (datum_index)k_datum_index_none) {
            for (j = 0; j < (int32_t)definition->participants.count; j++) {
                if (instance->participant_actor[j] != (datum_index)k_datum_index_none &&
                    unit_point_within_look_cone(0.5235988f, ((struct player *)player)->unit,
                        (real_point3d *)((uint8_t *)actor_data->data +
                            (instance->participant_actor[j] & 0xffff) * k_actor_size + 0x120)) != 0) {
                    found_looking = 1;
                    break;
                }
            }
        }
        player = data_iterator_next(&iterator);
    }
    if (found_looking != 0) {
        goto apply;
    }
    definition_flags = definition->flags;

not_ready_check_retry:
    ready = 0;
    if ((definition_flags & 0x40) == 0) {
        goto check_keep_trying;
    }
    goto report_keep_trying;

check_keep_trying:
    if (keep_trying == 0) {
        goto report_not_trying;
    }

report_keep_trying:
    if ((definition->flags & 0x40) != 0) {
        *out_keep_trying = 1;
        return ready;
    }

report_not_trying:
    *out_keep_trying = 0;
    return ready;

apply:
    for (i = 0; i < (int32_t)definition->participants.count; i++) {
        if ((instance->participant_mask & (1u << ((uint8_t)i & 0x1f))) == 0) {
            continue;
        }
        actor_handle = instance->participant_actor[i];
        if (actor_handle == (datum_index)k_datum_index_none) {
            continue;
        }
        unit_index = ((actor *)((uint8_t *)actor_data->data +
                                (actor_handle & 0xffff) * k_actor_size))->unit_index;
        unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;

        object_name = (int16_t)participants[i].set_new_name;
        if (object_name != -1 && object_name >= 0 &&
            (int32_t)object_name < (int32_t)global_scenario->object_names.count) {
            object_name_list[object_name] = unit_index;
        }
        if ((definition->flags & 0x20) != 0) {
            ai_conversation_range_lookup mode_data;

            if (ai_conversation_get_run_to_player_range(&mode_data, conversation_index) != 0) {
                actor_set_mode(instance->participant_actor[i], _actor_mode_conversation, &mode_data);
            }
        }
        variant = (int16_t)participants[i].variant_numbers[variant_slots[i]];
        if (*(int16_t *)((uint8_t *)unit_object + 0xbe) != variant) {
            *(int16_t *)((uint8_t *)unit_object + 0xbe) = variant;
            *(uint32_t *)((uint8_t *)unit_object + 0x204) =
                *(uint32_t *)((uint8_t *)unit_object + 0x204) & 0xfffffeffu;
        }
    }
    instance->active = 1;
    return ready;
}

/**
 * Behaviour of ai conversation stop, moved unchanged from the original free function.
 *
 * @address 0x430ea0
 */
void ConversationView::stop(uint8_t reason_a, uint8_t reason_b)
{
    datum_index instance_handle = handle;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ai_conversation_event *event;
    int16_t cursor;
    int16_t new_count;
    int16_t i;
    int32_t participant_count;

    if (instance_handle == (datum_index)k_datum_index_none) {
        return;
    }
    instance = &((ai_conversation *)ai_conversation_data->data)[instance_handle & 0xffff];
    definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];

    cursor = ai_globals_ptr->conversation_event_cursor;
    ai_globals_ptr->conversation_event_cursor = (cursor + 1) & 0xf;
    new_count = cursor + 1;
    if (new_count < ai_globals_ptr->conversation_event_count) {
        new_count = ai_globals_ptr->conversation_event_count;
    }
    ai_globals_ptr->conversation_event_count = new_count;

    event = &ai_globals_ptr->conversation_events[cursor];
    event->definition_index = instance->definition_index;
    event->reason_a = reason_a;
    event->reason_b = reason_b;
    event->tick = game_time->game_time;

    participant_count = definition->participants.count;
    for (i = 0; i < participant_count; i++) {
        if ((instance->participant_mask & (1u << (i & 0x1f))) != 0 &&
            instance->participant_actor[i] != (datum_index)k_datum_index_none) {
            actor *a = &((actor *)actor_data->data)[instance->participant_actor[i] & 0xffff];
            a->conversation_index = (datum_index)k_datum_index_none;
            a->conversation_participant = (datum_index)k_datum_index_none;
            if (a->mode == 12) {
                *(int32_t *)a->mode_data.raw = -1;
            }
        }
    }

    datum_delete(ai_conversation_data, instance_handle);
}

/**
 * Behaviour of ai conversation stop all, moved unchanged from the original free function.
 *
 * @address 0x4309c0
 */
void ConversationDefinitionView::stop_all()
{
    int16_t conversation_definition_index = handle;
    data_iterator iterator;
    ai_conversation *instance;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            ai_conversation_stop(iterator.index, 0, 0);
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai conversation update, moved unchanged from the original free function.
 *
 * @address 0x430a70
 */
void Conversations::update()
{
    int32_t now = game_time->game_time;
    data_iterator iterator;
    uint8_t *inst;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (inst = (uint8_t *)data_iterator_next(&iterator); inst != 0; inst = (uint8_t *)data_iterator_next(&iterator)) {
        datum_index handle = iterator.index;
        uint8_t *definition = *(uint8_t **)((uint8_t *)global_scenario + 0x46c) + *(int16_t *)(inst + 0x2) * 0x74;
        int32_t line_count = *(int32_t *)(definition + 0x5c);

        if (!inst[0x6]) {
            uint8_t ok = 1;

            if ((now - *(int32_t *)(inst + 0xc)) % 0x1e == 0) {
                ai_conversation_resolve_participants(handle, &ok);
            }
            if (!inst[0x6]) {
                if (!ok) {
                    ai_conversation_stop(handle, 1, 0);
                }
                if (!inst[0x6]) {
                    goto finished_check;
                }
            }
        }
        if (!inst[0x7]) {
            int16_t line = *(int16_t *)(inst + 0x48);
            uint8_t pending = (line >= 0 && (int32_t)line < line_count);

            for (;;) {
                if (pending && !ai_conversation_current_line_is_ready(handle)) {
                    break;
                }
                *(int16_t *)(inst + 0x48) = (int16_t)(*(int16_t *)(inst + 0x48) + 1);
                if ((int32_t)*(int16_t *)(inst + 0x48) >= line_count) {
                    inst[0x7] = 1;
                    break;
                }
                pending = ai_conversation_activate_next_participant(handle);
            }
        }
finished_check:
        if (inst[0x7]) {
            ai_conversation_stop(handle, 0, 1);
            continue;
        }
        if (inst[0x6]) {
            int16_t i;

            for (i = 0; (int32_t)i < *(int32_t *)(definition + 0x50); i++) {
                datum_index actor_index = *(datum_index *)(inst + 0x28 + i * 4);
                uint8_t *a;
                datum_index unit;
                uint16_t flags = *(uint16_t *)(inst + 0x4e);

                if (!(*(uint32_t *)(inst + 0x14) & (1u << i)) || actor_index == k_datum_index_none) {
                    continue;
                }
                a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
                unit = ((actor *)a)->unit_index;
                ((actor *)a)->conversation_index = handle;
                ((actor *)a)->conversation_participant = k_datum_index_none;
                if (unit == *(datum_index *)(inst + 0x54)) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x58);
                } else if (unit == *(datum_index *)(inst + 0x58) && (flags & 1)) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x54);
                } else if (flags & 2) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x54);
                } else if (flags & 4) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x58);
                }
            }
        }
    }
}

}
