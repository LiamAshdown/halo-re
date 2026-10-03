#include "halo/game/game2_engines.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern data_array *player_data;
extern game_variant game_engine_variant;
extern uint8_t game_engine_teams_enabled_flag;
extern int32_t slayer_team_score[16];
extern int32_t slayer_player_score[16];
extern int32_t slayer_unknown_0087a4a0[16];
extern int32_t slayer_unknown_0087a4e0[16];
extern wchar_t empty_string;
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index);
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...);
extern uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode);
extern wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank);
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...);
extern int16_t network_game_mode;
extern void game_engine_animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player);
extern void game_engine_player_select_random_target(datum_index player_or_all);
extern game_engine_definition *current_game_engine;
extern uint8_t message_delta_decode_compound_field(void **context, void *destination);
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags, int32_t changed_offset, int32_t destination_offset);
extern uint8_t network_message_scratch[0x7ff8];
extern network_server_globals *network_server;
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused);
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
extern uint32_t players_get_active_by_index(int32_t index);
extern void qr2_buffer_add_int(void *buffer, int32_t value);
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern data_array *object_data;
extern int32_t game_engine_state_value;
extern uint8_t custom_waypoints[];
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name, float height_offset, datum_index player_filter, int16_t team_filter);
extern uint8_t game_engine_player_respawn_priority_gate(uint32_t player_index);
extern void game_engine_begin_end_game_sequence(void);
}

namespace halo::game {

/**
 * A ui\multiplayer_game_text string, or the empty string without the tag.
 */
const uint16_t * SlayerEngine::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

/**
 * A live player of the given handle (index in range, salt 0 or matching), else 0.
 */
uint8_t * SlayerEngine::player_if_valid(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *player;

    if (handle == 0xffffffff || index < 0 || index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return 0;
    }
    player = (uint8_t *)player_data->data + index * *(int16_t *)((uint8_t *)player_data + 0x22);
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt)) {
        return 0;
    }
    return player;
}

/**
 * The kill-feed text override (called by chimera__kill_feed as (recipient, type, subject, text, count)). Type
 * 0x16 for a live recipient: with teams string 0xb5 with the recipient's place text (0x4633f0 of
 * game_engine_compare_score_to_others(recipient, 1)), its own score, its team's score and the score limit;
 * without, string 0xb6 with the place, its team slot's score and the limit.
 *
 * @address 0x46f610
 */
uint8_t SlayerEngine::build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    if (message_type == 0x16) {
        uint8_t *player = player_if_valid(recipient);
        const uint16_t *place;
        int32_t team;

        if (player == 0) {
            return 0;
        }
        place = (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
        team = *(int32_t *)(((uint8_t *)player_data->data + ((recipient) & 0xffff) * 0x200) + 0x20);
        if (game_engine_teams_enabled_flag != 0) {
            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb5), place, slayer_player_score[recipient & 0xffff],
                slayer_team_score[team], game_engine_variant.score_limit);
        } else {
            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb6), place, slayer_team_score[team],
                game_engine_variant.score_limit);
        }
        return 1;
    }
    if (message_type == 0x20) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(subject, player_data);

        if (player == 0) {
            return 0;
        }
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb4), player + 4);
        return 1;
    }
    return 0;
}

/**
 * Formats the player score as L"%d" (0x006607a0) into the buffer and returns it.
 *
 * @address 0x46f9e0
 */
wchar_t * SlayerEngine::build_player_text(datum_index player, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", slayer_player_score[player & 0xffff]);
    return buffer;
}

/**
 * Formats the team score as L"%d" (0x006607a0) into the buffer and returns it.
 *
 * @address 0x46fa10
 */
wchar_t * SlayerEngine::build_team_score_text(int32_t team, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", slayer_team_score[team]);
    return buffer;
}

/**
 * Team_mode 1: the team score of the player's team; otherwise the player score.
 *
 * @address 0x46f980
 */
int32_t SlayerEngine::get_score(datum_index player, int32_t team_mode)
{
    if (team_mode == 1) {
        return slayer_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)];
    }
    return slayer_player_score[player & 0xffff];
}

/**
 * The team score.
 *
 * @address 0x46f9c0
 */
int32_t SlayerEngine::get_team_score(int32_t team)
{
    return slayer_team_score[team];
}

/**
 * Zeroes the slayer team and player scores and the two arrays at 0x0087a4a0 / 0x0087a4e0; returns 1.
 *
 * @address 0x46f380
 */
uint8_t SlayerEngine::initialize_for_new_game(void)
{
    memset(slayer_team_score, 0, sizeof(slayer_team_score));
    memset(slayer_player_score, 0, sizeof(slayer_player_score));
    memset(slayer_unknown_0087a4a0, 0, sizeof(slayer_unknown_0087a4a0));
    memset(slayer_unknown_0087a4e0, 0, sizeof(slayer_unknown_0087a4e0));
    return 1;
}

/**
 * 0x46f540 (EAX player, EDX delta): unless a client, adds delta to the player's team score and its own score.
 */
void SlayerEngine::add_score(datum_index player_index, int32_t delta)
{
    if (network_game_mode == 1) {
        return;
    }
    slayer_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20)] += delta;
    slayer_player_score[player_index & 0xffff] += delta;
}

/**
 * Fired first by game_engine_on_player_death with (killer, death object, victim, suicide). Unless the victim
 * is marked for deletion or there is no killer: a suicide takes a point from the killer;.
 *
 * @address 0x46f580
 */
void SlayerEngine::player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    uint8_t *killer_player;

    (void)death_object;
    if (*(((uint8_t *)player_data->data + ((victim) & 0xffff) * 0x200) + 0xd5) != 0 || killer == 0xffffffff) {
        return;
    }
    killer_player = ((uint8_t *)player_data->data + ((killer) & 0xffff) * 0x200);
    if (is_suicide != 0) {
        add_score(killer, -1);
        return;
    }
    game_engine_animate_hill_pulse_icons(killer, victim);
    if (game_engine_variant.engine.slayer.kill_in_order != 0 && network_game_mode == 2) {
        if (*(datum_index *)(killer_player + 0x88) != victim) {
            return;
        }
        game_engine_player_select_random_target(killer);
    }
    add_score(killer, 1);
}

/**
 * The player's +0x88 becomes -1; as the server its score is cleared and, without teams, the score of its team.
 *
 * @address 0x46f3c0
 */
void SlayerEngine::player_new_life(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);

    ((struct player *)player)->slayer_target = -1;
    if (network_game_mode != 2) {
        return;
    }
    slayer_player_score[player_index & 0xffff] = 0;
    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        slayer_team_score[((struct player *)player)->team] = 0;
    }
}

/**
 * For a valid player: its +0x88 becomes -1 and its score 0, and every player whose +0x88 names it is reset to
 * -1 too (a player data iterator).
 *
 * @address 0x46fc70
 */
void SlayerEngine::player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)halo::memory::datum_get(player_index, player_data);
    data_iterator iterator;
    uint8_t *other;

    if (player == 0) {
        return;
    }
    ((struct player *)player)->slayer_target = -1;
    slayer_player_score[player_index & 0xffff] = 0;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = 0xffffffff;
    iterator.signature = (uint32_t)player_data ^ 0x69746572;
    for (other = (uint8_t *)halo::memory::data_iterator_next(&iterator); other != 0; other = (uint8_t *)halo::memory::data_iterator_next(&iterator)) {
        if (*(datum_index *)(other + 0x88) == player_index) {
            *(int32_t *)(other + 0x88) = -1;
        }
    }
}

/**
 * The inline tail every decoder shares with message_delta_decode_compound_field: nothing changed, so the
 * stream cursor moves past this message's bits when the target is inside the stream.
 */
void SlayerEngine::skip_unchanged_message(message_delta_decode_state *state)
{
    bit_stream *stream = (bit_stream *)state->stream;
    int32_t delta = state->start_bit_offset;
    uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;

    if ((delta >= 0 || target <= stream->first_bit) &&
        (delta <= 0 || stream->first_bit <= target) &&
        ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
        stream->bit_cursor = target & 7;
        stream->byte_cursor = target >> 3;
    }
}

/**
 * Message_delta_read_changed_subfields plus the bookkeeping around it; returns whether anything changed.
 */
uint8_t SlayerEngine::read_changed(void **context, void *changed_base, void *destination)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    int32_t bits = message_delta_read_changed_subfields(state, (uint8_t *)(context + 1), (int32_t)changed_base, (int32_t)destination);

    state->bits_read += bits;
    if (bits != 0) {
        state->changed = 1;
        return 1;
    }
    skip_unchanged_message(state);
    return 0;
}

/**
 * The profile_post_update decoder (called as (context, ECX) by 0x466e60): a baseline message decodes the
 * replicated copy with message_delta_decode_compound_field; an incremental one reads the changed subfields
 * into the live team and player scores (0x6b13d8, 0x20 dwords) and copies them to the replicated copy; on a
 * change they come back from it.
 *
 * @address 0x46fb20
 */
void SlayerEngine::profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, slayer_unknown_0087a4a0);
    } else {
        changed = read_changed(context, slayer_unknown_0087a4a0, slayer_team_score);
        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);
    }
    if (changed == 1) {
        memcpy(slayer_team_score, slayer_unknown_0087a4a0, 0x20 * 4);
    }
}

/**
 * Mode 0 encodes a type 0x10 request for the replicated scores at 0x87a4a0; otherwise encodes type 0x10 from
 * the live team scores 0x6b13d8 against 0x87a4a0 and then copies the 0x20 live dwords (team and player scores)
 * over the replicated copy. A positive bit count is broadcast when the machine is -1, else sent to that
 * machine.
 *
 * @address 0x46fa40
 */
void SlayerEngine::profiles_updated(int32_t mode, int32_t machine_index)
{
    void *items[2];
    void *network_fields[1];
    int32_t bits;

    if (mode == 0) {
        network_fields[0] = slayer_unknown_0087a4a0;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x10, 0, network_fields, 0, 1, 0);
    } else {
        items[0] = slayer_team_score;
        items[1] = 0;
        network_fields[0] = slayer_unknown_0087a4a0;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x10, 0, items, (int32_t)network_fields, 1, 0);
        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);
    }
    if (bits <= 0) {
        return;
    }
    if (machine_index == -1) {
        network_session_broadcast_to_flagged(bits, network_server, 1, network_message_scratch, 1, 0, 0, 3);
    } else {
        network_session_send_to_machine(machine_index, network_server, 1, network_message_scratch, bits, 1, 0, 0, 3);
    }
}

/**
 * GameSpy player query: for key 0x16 and an active player at the index, writes its slayer score into the
 * report (0x616640) and returns 1; else 0.
 *
 * @address 0x46fd30
 */
uint8_t SlayerEngine::query_player_score(int32_t key, int32_t index, void *buffer)
{
    uint32_t handle = players_get_active_by_index(index);
    uint8_t *player = (uint8_t *)halo::memory::datum_get(handle, player_data);

    if (player == 0 || key != 0x16) {
        return 0;
    }
    qr2_buffer_add_int(buffer, slayer_player_score[handle & 0xffff]);
    return 1;
}

/**
 * GameSpy query report: for key 0x1d writes the team score (slayer_team_score[team]) into the report
 * (0x616640) and returns 1; other keys 0.
 *
 * @address 0x46fdb0
 */
uint8_t SlayerEngine::query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, slayer_team_score[team]);
    return 1;
}

/**
 * As the server, zeroes the slayer team and player scores.
 *
 * @address 0x46fde0
 */
void SlayerEngine::reset_objects(void)
{
    if (network_game_mode == 2) {
        memset(slayer_team_score, 0, sizeof(slayer_team_score));
        memset(slayer_player_score, 0, sizeof(slayer_player_score));
    }
}

/**
 * Queues multiplayer sound 0x23 with teams, 0x15 without.
 *
 * @address 0x46f420
 */
void SlayerEngine::reset_round(void)
{
    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

    (void)teams;
    game_engine_queue_multiplayer_sound(teams ? 0x23 : 0x15, 0xffffffff, 0);
}

/**
 * True for kind 1.
 *
 * @address 0x46f9d0
 */
uint8_t SlayerEngine::unknown_84(int32_t kind)
{
    return (uint8_t)(kind == 1);
}

/**
 * With variant +0x7d the player's speed (+0x6c) above 1 decays by 1/9000 a tick down to 1; with +0x7c a speed
 * below 1 grows by 1/90000 up to 1. With +0x7e (targets) the player's waypoint slot is cleared and, when its
 * +0x88 target has a unit, re-registered "target_blue" over that unit for this player only; as the server a
 * player with a unit and no target, or whose target passes 0x463100, gets a new random target.
 *
 * @address 0x46f7f0
 */
void SlayerEngine::update(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);
    datum_index target;

    if (game_engine_variant.engine.slayer.kill_penalty != 0 && ((struct player *)player)->speed > 1.0f) {
        float speed = ((struct player *)player)->speed - 0.000111111112f;

        ((struct player *)player)->speed = speed > 1.0f ? speed : 1.0f;
    }
    if (game_engine_variant.engine.slayer.death_bonus != 0 && ((struct player *)player)->speed < 1.0f) {
        float speed = ((struct player *)player)->speed + 0.0000111111112f;

        ((struct player *)player)->speed = speed <= 1.0f ? speed : 1.0f;
    }
    if (game_engine_variant.engine.slayer.kill_in_order != 0) {
        memset(custom_waypoints + (int16_t)player_index * 0x20, 0, 0x20);
        target = *(datum_index *)&((struct player *)player)->slayer_target;
        if (target != 0xffffffff) {
            datum_index unit_index = *(datum_index *)(((uint8_t *)player_data->data + ((target) & 0xffff) * 0x200) + 0x34);

            if (unit_index != 0xffffffff) {
                uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);

                custom_waypoint_register(0xffffffff, (int16_t)player_index, (real_point3d *)(unit + 0xa0), "target_blue", 0.0f,
                    player_index, -1);
            }
        }
        if (network_game_mode == 2) {
            if (((struct player *)player)->unit != 0xffffffff && *(datum_index *)&((struct player *)player)->slayer_target == 0xffffffff) {
                game_engine_player_select_random_target(player_index);
            }
            target = *(datum_index *)&((struct player *)player)->slayer_target;
            if (target != 0xffffffff && game_engine_player_respawn_priority_gate(target) != 0) {
                game_engine_player_select_random_target(player_index);
            }
        }
    }
    if (slayer_team_score[((struct player *)player)->team] >= game_engine_variant.score_limit) {
        game_engine_begin_end_game_sequence();
    }
}

}
