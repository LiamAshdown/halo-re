#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Non-owning handle to a conversation definition by scenario index. Members start, query and stop the
 * conversation instances created from it.
 */
class ConversationDefinitionView {
public:
    int16_t handle;
    explicit constexpr ConversationDefinitionView(int16_t h) : handle(h) {}

    uint8_t activate(uint8_t allow_eviction);
    int32_t get_status();
    int16_t get_line_index();
    void mark_all();
    datum_index create(uint8_t allow_eviction);
    void stop_all();
};

/**
 * Non-owning handle to a running conversation instance. Members advance, resolve participants of and
 * stop that instance.
 */
class ConversationView {
public:
    datum_index handle;
    explicit constexpr ConversationView(datum_index h) : handle(h) {}

    uint8_t activate_next_participant();
    uint8_t current_line_is_ready();
    uint8_t resolve_participants(uint8_t *out_keep_trying);
    void stop(uint8_t reason_a, uint8_t reason_b);
};

/**
 * Conversation-system level operations: per-tick update, participant resolution and object clean-up.
 */
class Conversations {
public:
    static void clear_object_references(datum_index object_index, uint8_t force_full_scan);
    static void clear_participant(datum_index actor_index);
    static int32_t get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index);
    static int8_t resolve_participant(int16_t participant_index, uint8_t *out_resolved, uint8_t *out_wants_alternate, uint8_t *out_blocked_by_player, float *inout_minimum_distance, datum_index conversation_index);
    static void update();
};

}
