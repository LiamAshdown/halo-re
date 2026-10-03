#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Kill attribution and the kill feed messages that are broadcast and displayed for it. Stateless service
 * class: every function is a static member and the state it acts on lives in the engine globals.
 */
class KillFeed {
public:
    static uint8_t apply_kill_streak_message(int32_t **envelope);
    static void attribute_player_death(datum_index victim_unit, datum_index killer, datum_index death_object, int32_t killer_team, char credit_kills);
    static void broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast);
    static void broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t exclude_index, int32_t alternate_recipient, datum_index subject, char broadcast);
    static void broadcast_kill_feed_or_direct(datum_index recipient_or_all, int32_t broadcast_enabled, char broadcast, int32_t hash_key, datum_index subject);
    static void broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast);
    static uint8_t build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type, datum_index subject, size_t buffer_size);
    static void handle_kill_feed_network_event(int32_t **message);
    static void notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type, datum_index subject);
};

}
