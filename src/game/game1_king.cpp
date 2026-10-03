/**
 * King-of-the-hill game engine entry points: scores, round resets and score text.
 */

#include "tags.h"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#include "networking.h"

#include "halo/game/game1_king.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"

extern "C" {
extern data_array *player_data;
extern int32_t king_bucket_credit_ticks[16];
extern wchar_t empty_string;
extern uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode);
extern wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank);
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest);
extern uint16_t missing_string_text[];
extern int32_t king_team_hill_seconds_network[16];
extern int16_t game_engine_recent_location_count;
extern int16_t game_engine_recent_location_table[];
extern int32_t king_starting_location_type;
extern int32_t king_hill_move_ticks_006b1068;
extern int32_t king_hill_index_006b1058;
extern int32_t king_hill_state_globals;
extern void game_engine_koth_build_hill_boundary(void);
extern void game_engine_koth_reset_hill_marker_history(void);
extern int16_t network_game_mode;
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern int32_t king_bucket_last_credit_tick[16];
extern uint8_t message_delta_decode_compound_field(void **context, void *destination);
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset);
extern int32_t king_hill_broadcast_overrun_value;
extern uint32_t players_get_active_by_index(int32_t index);
extern void qr2_buffer_add(void *buffer, const char *value);
extern void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest);
extern void qr2_buffer_add_int(void *buffer, int32_t value);
extern uint8_t king_hill_player_in_hill[16];
extern int32_t king_hill_state_006b1054;
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern game_variant game_engine_variant;
extern int32_t game_engine_state_value;
extern int32_t king_starting_location_count;
extern real_point3d king_hill_boundary_center;
extern int32_t game_engine_pick_random_recent_location(int32_t exclude_value, int32_t fallback);
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name,
    float height_offset, datum_index player_filter, int16_t team_filter);
extern void game_engine_koth_update_hill_occupancy_state(void);
}

namespace halo::game::engine1 {

/**
 * File-local helper of King: game text.
 */
const uint16_t *King::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : halo::text::text_string_list_get_string(tag_id, index);
}

/**
 * File-local helper of King: place text.
 */
const uint16_t *King::place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}

/**
 * Builds the text of a king-of-the-hill event message for a recipient.
 *
 * @address 0x46b000
 */
uint8_t King::build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    uint8_t *player = (uint8_t *)halo::memory::datum_get(subject, player_data);
    int32_t seconds;

    switch (message_type) {
    case 0x22:
        if (halo::memory::datum_get(recipient, player_data) == 0 || player == 0) {
            return 0;
        }
        {
            const uint16_t *place = place_text(recipient);

            seconds = king_bucket_credit_ticks[((struct player *)player)->team] / 30;
            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x9b), place, seconds);
        }
        return 1;
    case 0x21:
    case 0x20:
        if (player == 0) {
            return 0;
        }
        seconds = king_bucket_credit_ticks[((struct player *)player)->team] / 30;
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x21 ? 0x9c : 0x9d), player + 4, seconds);
        return 1;
    default:
        return 0;
    }
}

/**
 * Builds the per-player score text shown for king of the hill.
 *
 * @address 0x46b6e0
 */
wchar_t *King::build_player_text(datum_index player, wchar_t *buffer)
{
    game_time_format_minutes_seconds((uint32_t)((int32_t)*(int16_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0xc4)), 0x100, buffer);
    return buffer;
}

/**
 * File-local helper of King: multiplayer text.
 */
uint16_t *King::multiplayer_text(int16_t index)
{
    datum_index list = halo::cache::tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    if (list == 0xffffffff) {
        return (uint16_t *)L"";
    }
    {
        uint8_t *strings = (uint8_t *)halo::cache::globals().tag_instances[list & 0xffff].data;

        if (*(int32_t *)strings > index) {
            uint8_t *element = *(uint8_t **)(strings + 4) + index * 0x14;
            uint32_t size = *(uint32_t *)element;

            if ((int32_t)size > 0) {
                uint16_t *text = *(uint16_t **)(element + 0xc);

                text[(size >> 1) - 1] = 0;
                return text;
            }
        }
        return missing_string_text;
    }
}

/**
 * Builds the scoreboard header text for king of the hill.
 *
 * @address 0x46b720
 */
wchar_t *King::build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text(0x9e));
    return buffer;
}

/**
 * Builds the team score text shown for king of the hill.
 *
 * @address 0x46b7a0
 */
wchar_t *King::build_team_score_text(int32_t team, wchar_t *buffer)
{
    game_time_format_minutes_seconds((uint32_t)(king_bucket_credit_ticks[team]), 0x100, buffer);
    return buffer;
}

/**
 * Returns the king-of-the-hill score of a player or team.
 *
 * @address 0x46b200
 */
int32_t King::get_score(datum_index player, int32_t team_mode)
{
    uint8_t *p = ((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200);

    if (team_mode != 0) {
        return king_bucket_credit_ticks[*(int32_t *)(p + 0x20)];
    }
    return *(int16_t *)(p + 0xc4);
}

/**
 * Returns the king-of-the-hill score of a team.
 *
 * @address 0x46b240
 */
int32_t King::get_team_score(int32_t team)
{
    return king_bucket_credit_ticks[team];
}

/**
 * King-of-the-hill initialize-for-new-game callback; returns false to abort the start.
 *
 * @address 0x46a510
 */
uint8_t King::initialize_for_new_game(void)
{
    int16_t count = 0;
    int16_t i;

    memset(king_bucket_credit_ticks, 0, 0x6b * 4);
    memset(king_team_hill_seconds_network, 0, 0x6b * 4);
    game_engine_recent_location_count = 0;
    for (i = 0; i < *(int32_t *)&halo::scenario::globals().scenario->netgame_flags.count; i++) {
        uint8_t *location = (uint8_t *)halo::scenario::globals().scenario->netgame_flags.pointer + i * 0x94;
        int16_t k;

        if (*(int16_t *)(location + 0x10) != 8) {
            continue;
        }
        for (k = 0; k < count; k++) {
            if (game_engine_recent_location_table[k] == *(int16_t *)(location + 0x12)) {
                break;
            }
        }
        if (k == count) {
            game_engine_recent_location_table[count] = *(int16_t *)(location + 0x12);
            count++;
        }
    }
    if (*(int32_t *)&halo::scenario::globals().scenario->netgame_flags.count > 0) {
        game_engine_recent_location_count = count;
    }
    king_starting_location_type = 0;
    king_hill_move_ticks_006b1068 = 0x708;
    king_hill_index_006b1058 = -1;
    king_hill_state_globals = 0;
    game_engine_koth_build_hill_boundary();
    game_engine_koth_reset_hill_marker_history();
    return 1;
}

/**
 * King-of-the-hill player-new-life callback.
 *
 * @address 0x46a5e0
 */
void King::player_new_life(datum_index player_index)
{
    if (network_game_mode == 2 && (current_game_engine == 0 || game_engine_teams_enabled_flag == 0)) {
        int32_t team = *(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20);

        king_bucket_credit_ticks[team] = 0;
        king_bucket_last_credit_tick[team] = 0;
    }
}

/**
 * Resets the king-of-the-hill state of a player at a round reset.
 *
 * @address 0x46ba40
 */
void King::player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)halo::memory::datum_get(player_index, player_data);

    if (player != 0) {
        *(int16_t *)&((struct player *)player)->objective_time = 0;
    }
}

/**
 * File-local helper of King: skip unchanged message.
 */
void King::skip_unchanged_message(message_delta_decode_state *state)
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
 * File-local helper of King: read changed.
 */
uint8_t King::read_changed(void **context, void *changed_base, void *destination)
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
 * Decodes the king-of-the-hill part of a player profile update message.
 *
 * @address 0x46b920
 */
void King::profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    uint8_t moved;
    int32_t i;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, king_team_hill_seconds_network);
        moved = 1;
    } else {
        int32_t previous = king_hill_broadcast_overrun_value;

        memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);
        king_starting_location_type = previous;
        changed = read_changed(context, king_team_hill_seconds_network, king_bucket_credit_ticks);
        memcpy(king_team_hill_seconds_network, king_bucket_credit_ticks, 16 * 4);
        moved = king_starting_location_type != previous;
        king_hill_broadcast_overrun_value = king_starting_location_type;
    }
    if (changed != 1) {
        return;
    }
    memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);
    king_starting_location_type = king_hill_broadcast_overrun_value;
    for (i = 0; i < 16; i++) {
        king_bucket_credit_ticks[i] = king_team_hill_seconds_network[i] * 30;
    }
    if (moved == 1) {
        game_engine_koth_build_hill_boundary();
    }
}

/**
 * Answers a scoreboard query keyed by a player index with king-of-the-hill values.
 *
 * @address 0x46ba90
 */
uint8_t King::query_player_score(int32_t key, int32_t index, void *buffer)
{
    uint32_t handle = players_get_active_by_index(index);
    uint8_t *player = (uint8_t *)halo::memory::datum_get(handle, player_data);
    char text[0x100];

    if (player == 0 || key != 0x16) {
        return 0;
    }
    game_time_format_minutes_seconds_ascii((uint32_t)(*(int16_t *)&((struct player *)player)->objective_time), 0x100, text);
    qr2_buffer_add(buffer, text);
    return 1;
}

/**
 * Answers a scoreboard query keyed by a team index with king-of-the-hill values.
 *
 * @address 0x46bb40
 */
uint8_t King::query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, king_bucket_credit_ticks[team]);
    return 1;
}

/**
 * King-of-the-hill reset-objects callback.
 *
 * @address 0x46bb70
 */
void King::reset_objects(void)
{
    int32_t i;

    if (network_game_mode != 2) {
        return;
    }
    for (i = 0; i < 0x10; i++) {
        king_bucket_credit_ticks[i] = 0;
        king_bucket_last_credit_tick[i] = 0;
        king_hill_player_in_hill[i] = 0;
    }
    king_starting_location_type = 0;
    king_hill_move_ticks_006b1068 = 0x708;
    king_hill_index_006b1058 = -1;
    king_hill_state_globals = 0;
    king_hill_state_006b1054 = 0;
    game_engine_koth_build_hill_boundary();
}

/**
 * King-of-the-hill reset-round callback.
 *
 * @address 0x46a630
 */
void King::reset_round(void)
{
    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

    (void)teams;
    game_engine_queue_multiplayer_sound(teams ? 0x20 : 0x24, 0xffffffff, 0);
}

/**
 * King-of-the-hill engine definition slot +0x48.
 *
 * @address 0x46aee0
 */
void King::unknown_48(void)
{
    if ((current_game_engine == 0 || game_engine_state_value == 0) && network_game_mode == 2 &&
        game_engine_variant.engine.king.moving_hill != 0 && --king_hill_move_ticks_006b1068 == 0) {
        king_hill_move_ticks_006b1068 = 0x708;
        king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);
        game_engine_koth_build_hill_boundary();
        game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);
        while (king_starting_location_count == 0 && king_starting_location_type != 0) {
            king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);
            game_engine_koth_build_hill_boundary();
            game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);
        }
    }
    if (king_starting_location_count > 0) {
        real_point3d position = king_hill_boundary_center;

        custom_waypoint_register(0xffffffff, 0, &position, "crown_blue", 0.0f, 0xffffffff, -1);
    } else {
        halo::main::console_print_error_va(0, "FAILED TO FIND HILL");
    }
    if (network_game_mode == 2) {
        game_engine_koth_update_hill_occupancy_state();
    }
}

/**
 * Waypoint filter callback of the king-of-the-hill engine for a player.
 *
 * @address 0x46b7d0
 */
uint8_t King::waypoint_filter(datum_index player)
{
    return (uint8_t)(king_hill_player_in_hill[player & 0xffff] == 0);
}

}
