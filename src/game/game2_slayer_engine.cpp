#include "halo/game/game2_engines.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &game_engine_teams_enabled_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_teams_enabled_flag);
static auto &slayer_team_score = halo::link::ref<int32_t [16]>(halo::game::vars().slayer_team_score);
static auto &slayer_player_score = halo::link::ref<int32_t [16]>(halo::game::vars().slayer_player_score);
static auto &slayer_unknown_0087a4a0 = halo::link::ref<int32_t [16]>(halo::game::vars().slayer_unknown_0087a4a0);
static auto &slayer_unknown_0087a4e0 = halo::link::ref<int32_t [16]>(halo::game::vars().slayer_unknown_0087a4e0);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &custom_waypoints = halo::link::ref<uint8_t []>(halo::game::vars().custom_waypoints);

namespace halo::game {

/**
 * A ui\multiplayer_game_text string, or the empty string without the tag.
 */
const uint16_t * SlayerEngine::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return tag_id == halo::k_dword_none ? (const uint16_t *)&empty_string : halo::text::text_string_list_get_string(tag_id, index);
}

/**
 * A live player of the given handle (index in range, salt 0 or matching), else 0.
 */
uint8_t * SlayerEngine::player_if_valid(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *player;

    if (handle == halo::k_dword_none || index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    player = (uint8_t *)player_data->data + index * player_data->size;
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
        place = (const uint16_t *)halo::game::game_engine_get_multiplayer_text_list(halo::game::game_engine_compare_score_to_others(recipient, 1));
        team = halo::game::player_at(recipient)->team;
        if (game_engine_teams_enabled_flag != 0) {
            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb5), place, slayer_player_score[recipient & halo::k_datum_slot_mask],
                slayer_team_score[team], game_engine_variant.score_limit);
        } else {
            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb6), place, slayer_team_score[team],
                game_engine_variant.score_limit);
        }
        return 1;
    }
    if (message_type == 0x20) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(subject, player_data);

        if (player == 0) {
            return 0;
        }
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb4), player + 4);
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
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", slayer_player_score[player & 0xffff]);
    return buffer;
}

/**
 * Formats the team score as L"%d" (0x006607a0) into the buffer and returns it.
 *
 * @address 0x46fa10
 */
wchar_t * SlayerEngine::build_team_score_text(int32_t team, wchar_t *buffer)
{
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", slayer_team_score[team]);
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
        return slayer_team_score[halo::game::player_at(player)->team];
    }
    return slayer_player_score[player & halo::k_datum_slot_mask];
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
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
        return;
    }
    slayer_team_score[halo::game::player_at(player_index)->team] += delta;
    slayer_player_score[player_index & halo::k_datum_slot_mask] += delta;
}

/**
 * Fired first by game_engine_on_player_death with (killer, death object, victim, suicide). Unless the victim
 * is marked for deletion or there is no killer: a suicide takes a point from the killer;.
 *
 * @address 0x46f580
 */
void SlayerEngine::player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    player *killer_player;

    (void)death_object;
    if (*((uint8_t *)halo::game::player_at(victim) + 0xd5) != 0 || killer == halo::k_dword_none) {
        return;
    }
    killer_player = halo::game::player_at(killer);
    if (is_suicide != 0) {
        add_score(killer, -1);
        return;
    }
    halo::game::game_engine_animate_hill_pulse_icons(killer, victim);
    if (game_engine_variant.engine.slayer.kill_in_order != 0 && halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        if (static_cast<datum_index>(killer_player->slayer_target) != victim) {
            return;
        }
        halo::game::game_engine_player_select_random_target(killer);
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
    ::player *player = halo::game::player_at(player_index);

    ((struct player *)player)->slayer_target = -1;
    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    slayer_player_score[player_index & halo::k_datum_slot_mask] = 0;
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
    ::player *player = (::player *)halo::memory::datum_get(player_index, player_data);
    data_iterator iterator;
    uint8_t *other;

    if (player == 0) {
        return;
    }
    player->slayer_target = -1;
    slayer_player_score[player_index & halo::k_datum_slot_mask] = 0;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = halo::k_dword_none;
    iterator.signature = (uint32_t)player_data ^ halo::game::k_iterator_signature_key;
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
    message_delta_decode_state *state = halo::networking::delta_context(context)->state;
    int32_t bits = halo::networking::message_delta_read_changed_subfields(state, halo::networking::delta_context(context)->changed, (int32_t)changed_base, (int32_t)destination);

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
    message_delta_decode_state *state = halo::networking::delta_context(context)->state;
    uint8_t changed;

    if (state->incremental == 0) {
        changed = halo::networking::message_delta_decode_compound_field(context, slayer_unknown_0087a4a0);
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
        bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::slayer_profiles_updated), 0, network_fields, 0, 1, 0);
    } else {
        items[0] = slayer_team_score;
        items[1] = 0;
        network_fields[0] = slayer_unknown_0087a4a0;
        bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::slayer_profiles_updated), 0, items, (int32_t)network_fields, 1, 0);
        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);
    }
    if (bits <= 0) {
        return;
    }
    if (machine_index == -1) {
        halo::networking::network_session_broadcast_to_flagged(bits, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    } else {
        halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, network_message_scratch, bits, 1, 0, 0, 3);
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
    uint32_t handle = halo::game::players_get_active_by_index(index);
    ::player *player = (::player *)halo::memory::datum_get(handle, player_data);

    if (player == 0 || key != 0x16) {
        return 0;
    }
    qr2_buffer_add_int(buffer, slayer_player_score[handle & halo::k_datum_slot_mask]);
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
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
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
    halo::game::game_engine_queue_multiplayer_sound(teams ? 0x23 : 0x15, halo::k_dword_none, 0);
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
    ::player *player = halo::game::player_at(player_index);
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
        if (target != halo::k_dword_none) {
            datum_index unit_index = halo::game::player_at(target)->unit;

            if (unit_index != halo::k_dword_none) {
                uint8_t *unit = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit_index & halo::k_datum_slot_mask) * 12 + 8);

                halo::game::custom_waypoint_register(halo::k_dword_none, (int16_t)player_index, (real_point3d *)(unit + 0xa0), "target_blue", 0.0f,
                    player_index, -1);
            }
        }
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
            if (((struct player *)player)->unit != halo::k_dword_none && *(datum_index *)&((struct player *)player)->slayer_target == halo::k_dword_none) {
                halo::game::game_engine_player_select_random_target(player_index);
            }
            target = *(datum_index *)&((struct player *)player)->slayer_target;
            if (target != halo::k_dword_none && halo::game::game_engine_player_respawn_priority_gate(target) != 0) {
                halo::game::game_engine_player_select_random_target(player_index);
            }
        }
    }
    if (slayer_team_score[((struct player *)player)->team] >= game_engine_variant.score_limit) {
        halo::game::game_engine_begin_end_game_sequence();
    }
}

}
