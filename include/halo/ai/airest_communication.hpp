#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Actor-to-actor and actor-to-player spoken communication: speaker selection and rating, line gating,
 * broadcast of communication events and reaction propagation.
 */
class AiCommunication {
public:
    static void broadcast_communication_event(int16_t gate, real_point3d *point, int32_t source_object, int16_t event_type, int16_t unused);
    static void broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
    static void gate_line_played(int16_t event_id, ai_communication_record *record, datum_index object_index);
    static void initialize();
    static int16_t line_fade_multiplier(uint32_t unit_index, int16_t priority, int16_t extra_delay, uint8_t follow_fallback, uint8_t apply_fade, float *volume, int32_t *chain_value, int16_t *dialogue_index, int16_t line_class);
    static void play_event_line(datum_index object_index, int16_t event_id, uint8_t force, datum_index explicit_speaker_actor_index, uint32_t *event_record);
    static float rate_player_proximity(uint8_t require_line_of_sight, datum_index *out_player_object_index, float *out_distance, datum_index object_index);
    static float rate_speaker(datum_index actor_index, datum_index object_b, real_point3d *position_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, real_point3d *position_a, datum_index object_a);
    static void record_line_played(datum_index object_index, int16_t tier, int16_t communication_line_id, int16_t conversation_line_id);
    static void reset();
    static datum_index select_speaker_by_team(int16_t match_mode, datum_index object_a, datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team);
    static datum_index select_speaker_in_reference(float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, uint32_t reference, datum_index object_a, datum_index object_b);
    static void target_result_reset(ai_communication_target_result *record);
    static void dispatch_queued_order(ai_queued_order *order, datum_index prop_index, datum_index actor_index);
    static void propagate_communication_reaction(datum_index object_index, ai_communication_order *order);
    static int32_t select_communication_target(uint32_t param_a, uint32_t param_b, int16_t line_id, int16_t sub_id, float *out_weight);
};

/**
 * Interface of the dialogue condition predicates referenced by the dialogue tables. Each concrete
 * condition is a stateless object in static storage.
 */
class DialogueCondition {
public:
    virtual uint8_t test(datum_index object_index, uint32_t param_2, datum_index actor_index) const = 0;

protected:
    ~DialogueCondition() = default;
};

/**
 * Returns the dialogue condition registered at a table position (0 to 7), in the order of the original
 * predicate addresses.
 */
const DialogueCondition &dialogue_condition(int index);

}
