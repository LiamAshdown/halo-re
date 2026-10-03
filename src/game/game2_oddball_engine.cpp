#include "halo/game/game2_engines.hpp"
#include "halo/text/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern game_variant game_engine_variant;
extern int32_t king_alt_team_score[16];
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest);
extern data_array *player_data;
extern int32_t king_alt_player_score[];
extern int16_t network_game_mode;
extern int32_t king_alt_score_target;
extern int32_t king_alt_team_scores_network[16];
extern int32_t oddball_ball_timers_006b11cc[16];
extern int32_t king_hill_occupant_last_tick[16];
extern uint32_t king_hill_occupant_table[16];
extern void game_engine_koth_relocate_hill_marker(int32_t ball_index);
extern uint8_t game_engine_is_inactive(void);
extern void game_engine_koth_alt_scorer_tick(uint32_t player_index);
extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast);
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern uint8_t message_delta_decode_compound_field(void **context, void *destination);
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags, int32_t changed_offset, int32_t destination_offset);
extern int32_t king_alt_team_scores_network2[16];
extern int32_t king_alt_player_scores_network[16];
extern int32_t king_alt_scores_network_tail[16];
extern uint32_t players_get_active_by_index(int32_t index);
extern void qr2_buffer_add(void *buffer, const char *value);
extern void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest);
extern void qr2_buffer_add_int(void *buffer, int32_t value);
extern uint8_t custom_waypoints[];
extern game_time_globals *game_time;
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern int16_t hud_waypoint_arrow_find(const char *name);
}

namespace halo::game {

/**
 * The team score, as L"%d" when the variant's +0x8c is 2, else as minutes:seconds; returns the buffer.
 *
 * @address 0x46d020
 */
wchar_t * OddballEngine::build_team_score_text(int32_t team, wchar_t *buffer)
{
    int32_t score = king_alt_team_score[team];

    if (game_engine_variant.engine.oddball.ball_type == 2) {
        halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);
    } else {
        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);
    }
    return buffer;
}

/**
 * Team_mode 1: the team score of the player's team; otherwise the player score.
 *
 * @address 0x46cea0
 */
int32_t OddballEngine::get_score(datum_index player, int32_t team_mode)
{
    if (team_mode == 1) {
        return king_alt_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)];
    }
    return king_alt_player_score[player & 0xffff];
}

/**
 * The team score.
 *
 * @address 0x46cee0
 */
int32_t OddballEngine::get_team_score(int32_t team)
{
    return king_alt_team_score[team];
}

/**
 * Zeroes the 0x51 dwords from 0x006b1148 and from 0x0087a680; the score target becomes the variant score
 * limit, times 0x708 unless variant +0x8c is 2; the 16 hill occupants and their last ticks become -1. As the
 * server with variant +0x8c 1 or 2: each of the first variant +0x90 timers (0x006b11cc) is cleared and the
 * hill marker relocated; otherwise they get 0x1c2, 0x384, ... Returns 1.
 *
 * @address 0x46c080
 */
uint8_t OddballEngine::initialize_for_new_game(void)
{
    int32_t mode = game_engine_variant.engine.oddball.ball_type;
    int32_t target;
    int32_t i;

    memset(&king_alt_score_target, 0, 0x51 * 4);
    memset(king_alt_team_scores_network, 0, 0x51 * 4);
    target = game_engine_variant.score_limit;
    if (mode != 2) {
        target *= 0x708;
    }
    king_alt_score_target = target;
    for (i = 0; i < 0x10; i++) {
        king_hill_occupant_table[i] = 0xffffffff;
        king_hill_occupant_last_tick[i] = -1;
    }
    if (network_game_mode == 2) {
        if (mode > 0 && mode <= 2) {
            for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
                oddball_ball_timers_006b11cc[i] = 0;
                game_engine_koth_relocate_hill_marker(i);
            }
        } else {
            int32_t delay = 0;

            for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
                delay += 0x1c2;
                oddball_ball_timers_006b11cc[i] = delay;
            }
        }
    }
    return 1;
}

/**
 * 0x46c8e0 (ESI player): whether the player holds one of the variant +0x90 balls.
 */
uint8_t OddballEngine::oddball_is_carrier(datum_index player_index)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
        if (king_hill_occupant_table[i] == player_index) {
            return 1;
        }
    }
    return 0;
}

/**
 * 0x46c910: whether some ball has run out its timer and has no carrier.
 */
uint8_t OddballEngine::oddball_any_ball_free(void)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
        if (oddball_ball_timers_006b11cc[i] == 0 && king_hill_occupant_table[i] == 0xffffffff) {
            return 1;
        }
    }
    return 0;
}

/**
 * Fired by game_engine_on_player_death with (killer, death object, victim, suicide); with variant +0x8c in
 * 1..2 and as the server. For a real killer that is not a suicide: killing a carrier counts in the killer's
 * +0xc6, a carrier's kill in its +0xc8 (both then score through game_engine_koth_alt_scorer_tick only when
 * +0x8c is 2 and game_engine_is_inactive), otherwise the killer scores when a ball lies free;.
 *
 * @address 0x46c940
 */
void OddballEngine::player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    int32_t count;
    int32_t found = -1;
    int32_t i;

    (void)death_object;
    if (game_engine_variant.engine.oddball.ball_type <= 0 || game_engine_variant.engine.oddball.ball_type > 2 || network_game_mode != 2) {
        return;
    }
    count = game_engine_variant.engine.oddball.ball_count;
    if (killer != 0xffffffff && is_suicide == 0) {
        uint8_t *killer_player = ((uint8_t *)player_data->data + ((killer) & 0xffff) * 0x200);
        uint8_t score;

        if (oddball_is_carrier(victim) || oddball_is_carrier(killer)) {
            if (oddball_is_carrier(victim)) {
                (*(int16_t *)(killer_player + 0xc6))++;
            } else {
                (*(int16_t *)(killer_player + 0xc8))++;
            }
            score = game_engine_variant.engine.oddball.ball_type == 2 ? game_engine_is_inactive() : 0;
        } else {
            score = oddball_any_ball_free();
        }
        if (score != 0) {
            game_engine_koth_alt_scorer_tick(killer);
        }
        if (*(datum_index *)(killer_player + 0x34) != 0xffffffff) {
            for (i = 0; i < count; i++) {
                if (oddball_ball_timers_006b11cc[i] == 0 && found == -1 && king_hill_occupant_table[i] == 0xffffffff) {
                    found = i;
                }
                if (king_hill_occupant_table[i] == victim) {
                    found = i;
                    break;
                }
            }
            if (found != -1) {
                int32_t message = (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type <= 2) ? -1 : 0x23;

                game_engine_broadcast_kill_feed_by_relationship(killer, message, 0x24, 0x25, killer, 0);
                king_hill_occupant_table[found] = killer;
            }
        }
    }
    for (i = 0; i < count; i++) {
        if (king_hill_occupant_table[i] == victim) {
            king_hill_occupant_table[i] = 0xffffffff;
        }
    }
}

/**
 * As the server clears the player score and, without teams, the score of the player's team.
 *
 * @address 0x46c150
 */
void OddballEngine::player_new_life(datum_index player_index)
{
    if (network_game_mode != 2) {
        return;
    }
    king_alt_player_score[player_index & 0xffff] = 0;
    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        king_alt_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20)] = 0;
    }
}

/**
 * For a valid player handle: king_alt_player_score[player_index & 0xffff] = 0;.
 *
 * @address 0x46d310
 */
void OddballEngine::player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)halo::memory::datum_get(player_index, player_data);

    if (player != 0) {
        king_alt_player_score[player_index & 0xffff] = 0;
    }
}

/**
 * The inline tail every decoder shares with message_delta_decode_compound_field: nothing changed, so the
 * stream cursor moves past this message's bits when the target is inside the stream.
 */
void OddballEngine::skip_unchanged_message(message_delta_decode_state *state)
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
uint8_t OddballEngine::read_changed(void **context, void *changed_base, void *destination)
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
 * replicated copy with message_delta_decode_compound_field; an incremental one restores the 0x51 live dwords
 * from 0x6b1148 out of the replicated copy, reads the changed subfields into them and copies the player
 * scores, team scores and occupant table back.
 *
 * @address 0x46d1d0
 */
void OddballEngine::profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    int32_t i;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, king_alt_team_scores_network);
    } else {
        memcpy(&king_alt_score_target, king_alt_team_scores_network, 0x51 * 4);
        changed = read_changed(context, king_alt_team_scores_network, &king_alt_score_target);
        memcpy(king_alt_player_scores_network, king_alt_player_score, 16 * 4);
        memcpy(king_alt_team_scores_network2, king_alt_team_score, 16 * 4);
        memcpy(king_alt_scores_network_tail, king_hill_occupant_table, 16 * 4);
    }
    if (changed != 1) {
        return;
    }
    memcpy(king_alt_player_score, king_alt_player_scores_network, 16 * 4);
    memcpy(king_alt_team_score, king_alt_team_scores_network2, 16 * 4);
    memcpy(king_hill_occupant_table, king_alt_scores_network_tail, 16 * 4);
    if (game_engine_variant.engine.oddball.ball_type == 2) {
        return;
    }
    for (i = 0; i < 16; i++) {
        king_alt_team_score[i] *= 30;
        king_alt_player_score[i] *= 30;
    }
}

/**
 * GameSpy player query: for key 0x16 and an active player at the index, formats king_alt_player_score[handle &
 * 0xffff] as ASCII minutes:seconds (0x466600, 0x100 characters) and writes it into the report (0x615590);
 * returns 1, else 0.
 *
 * @address 0x46d370
 */
uint8_t OddballEngine::query_player_score(int32_t key, int32_t index, void *buffer)
{
    uint32_t handle = players_get_active_by_index(index);
    uint8_t *player = (uint8_t *)halo::memory::datum_get(handle, player_data);
    char text[0x100];

    if (player == 0 || key != 0x16) {
        return 0;
    }
    game_time_format_minutes_seconds_ascii((uint32_t)(king_alt_player_score[handle & 0xffff]), 0x100, text);
    qr2_buffer_add(buffer, text);
    return 1;
}

/**
 * GameSpy query report: for key 0x1d writes the team score (king_alt_team_score[team]) into the report
 * (0x616640) and returns 1; other keys 0.
 *
 * @address 0x46d420
 */
uint8_t OddballEngine::query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, king_alt_team_score[team]);
    return 1;
}

/**
 * As the server: zeroes the 16 team scores and the first 16 player scores, sets the 16 occupant entries and
 * last ticks to -1, then with variant +0x8c in 1..2 zeroes each of the first variant +0x90 ball timers and
 * relocates the marker once per ball, otherwise gives them cumulative 0x1c2 tick delays. Always zeroes the
 * first variant +0x90 custom waypoints.
 *
 * @address 0x46d450
 */
void OddballEngine::reset_objects(void)
{
    int32_t count = game_engine_variant.engine.oddball.ball_count;
    int32_t i;

    if (network_game_mode == 2) {
        for (i = 0; i < 16; i++) {
            king_alt_player_score[i] = 0;
            king_alt_team_score[i] = 0;
        }
        for (i = 0; i < 16; i++) {
            king_hill_occupant_table[i] = 0xffffffff;
            king_hill_occupant_last_tick[i] = -1;
        }
        if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type <= 2) {
            for (i = 0; i < count; i++) {
                oddball_ball_timers_006b11cc[i] = 0;
                game_engine_koth_relocate_hill_marker(i);
            }
            count = game_engine_variant.engine.oddball.ball_count;
        } else {
            int32_t delay = 0;

            for (i = 0; i < count; i++) {
                delay += 0x1c2;
                oddball_ball_timers_006b11cc[i] = delay;
            }
        }
    }
    for (i = 0; i < count; i++) {
        memset(custom_waypoints + (int16_t)i * 0x20, 0, 0x20);
    }
}

/**
 * Without a value, false; when the player is among the first variant +0x90 hill occupants (0x006b120c),
 * whether the value is variant +0x84, else whether it is variant +0x88.
 *
 * @address 0x46cf20
 */
uint8_t OddballEngine::time_scale_override(uint32_t player, int32_t value)
{
    int32_t i;

    if (value == 0) {
        return 0;
    }
    for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
        if (king_hill_occupant_table[i] == player) {
            return (uint8_t)(value == game_engine_variant.engine.oddball.trait_with_ball);
        }
    }
    return (uint8_t)(value == game_engine_variant.engine.oddball.trait_without_ball);
}

/**
 * Also the race engine's +0xf8 slot. At tick 60 queues sound 0x21 with teams, 0x13 without. As the server each
 * running ball timer counts down and at zero queues sound 0 and respawns that ball (0x46bfe0). With variant
 * +0x8c in 1..2 each ball waypoint follows its carrier: cleared when uncarried, else owner = the carrier,
 * arrow "target_blue", visible, at the carrier unit's origin 0.63 higher, both filters -1.
 *
 * @address 0x46c750
 */
void OddballEngine::unknown_48(void)
{
    int32_t count;
    int32_t i;

    if (game_time->game_time == 0x3c) {
        uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

        game_engine_queue_multiplayer_sound(teams != 0 ? 0x21 : 0x13, 0xffffffff, 0);
    }
    count = game_engine_variant.engine.oddball.ball_count;
    if (network_game_mode == 2) {
        for (i = 0; i < count; i++) {
            if (oddball_ball_timers_006b11cc[i] > 0 && --oddball_ball_timers_006b11cc[i] == 0) {
                game_engine_queue_multiplayer_sound(0, 0xffffffff, 0);
                game_engine_koth_relocate_hill_marker(i);
            }
        }
    }
    if (game_engine_variant.engine.oddball.ball_type <= 0 || game_engine_variant.engine.oddball.ball_type > 2) {
        return;
    }
    for (i = 0; i < count; i++) {
        uint8_t *waypoint = custom_waypoints + (int16_t)i * 0x20;
        datum_index carrier = king_hill_occupant_table[i];
        datum_index unit_index;
        uint8_t *unit;

        if (carrier == 0xffffffff) {
            memset(waypoint, 0, 0x20);
            continue;
        }
        unit_index = *(datum_index *)(((uint8_t *)player_data->data + ((carrier) & 0xffff) * 0x200) + 0x34);
        if (unit_index == 0xffffffff) {
            continue;
        }
        unit = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit_index & 0xffff) * 12 + 8);
        *(datum_index *)(waypoint + 0x18) = carrier;
        *(int16_t *)(waypoint + 0x1c) = hud_waypoint_arrow_find("target_blue");
        waypoint[0x0c] = 1;
        *(real_point3d *)waypoint = *(real_point3d *)&((unit_object *)unit)->base.bounding_center.x;
        *(float *)(waypoint + 0x08) += 0.63f;
        *(int16_t *)(waypoint + 0x14) = -1;
        *(int32_t *)(waypoint + 0x10) = -1;
    }
}

/**
 * True for kind 1 when the variant's +0x8c is 2.
 *
 * @address 0x46cf00
 */
uint8_t OddballEngine::unknown_84(int32_t kind)
{
    return (uint8_t)(kind == 1 && game_engine_variant.engine.oddball.ball_type == 2);
}

}
