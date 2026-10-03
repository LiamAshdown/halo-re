#include "halo/game/game2_engines.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/ai/api.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &race_used_locations = halo::link::ref<int32_t [8]>(halo::game::vars().race_used_locations);
static auto &race_used_location_count = halo::link::ref<int32_t>(halo::game::vars().race_used_location_count);
static auto &race_vehicle_counts = halo::link::ref<uint32_t [4]>(halo::game::vars().race_vehicle_counts);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &game_engine_bucket_scores = halo::link::ref<int32_t [16]>(halo::game::vars().game_engine_bucket_scores);
static auto &ctf_team_captured_flags_mask = halo::link::ref<uint32_t []>(halo::game::vars().ctf_team_captured_flags_mask);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_engine_teams_enabled_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_teams_enabled_flag);
static auto &game_engine_bucket_scores_extra = halo::link::ref<int32_t [16]>(halo::game::vars().game_engine_bucket_scores_extra);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &ctf_globals_live = halo::link::ref<ctf_globals>(halo::game::vars().ctf_globals_live);
static auto &ctf_globals_network = halo::link::ref<ctf_globals>(halo::game::vars().ctf_globals_network);
static auto &game_engine_state_value = halo::link::ref<int32_t>(halo::game::vars().game_engine_state_value);

namespace halo::game {

/**
 * 0x46d5d0 (EAX index): the vehicle tag for the index-th race vehicle, by the variant's vehicle set (low
 * nibble; the multiplayer information's vehicle references are 0x10 bytes, tag index at +0x0c). Set 8 is
 * custom: per-type limits in 3-bit fields, consumed through race_vehicle_counts.
 */
datum_index RaceEngine::race_pick_vehicle_tag(int32_t index)
{
    uint32_t vehicle_set = game_engine_variant.red_vehicle_set;
    uint8_t *information = (uint8_t *)global_globals->multiplayer_information.pointer;
    uint8_t *vehicles = *(uint8_t **)(information + 0x24);
    datum_index tag = halo::k_dword_none;

#define VEHICLE(k) (*(datum_index *)(vehicles + (k) * 0x10 + 0x0c))
    switch (vehicle_set & 0xf) {
    case 0:
        if (index == 0) {
            return VEHICLE(0);
        }
        if (index == 1) {
            return VEHICLE(2);
        }
        return index < 6 ? VEHICLE(1) : halo::k_dword_none;
    case 2:
        return index < 4 ? VEHICLE(0) : halo::k_dword_none;
    case 3:
        return index < 8 ? VEHICLE(1) : halo::k_dword_none;
    case 4:
        return index < 4 ? VEHICLE(2) : halo::k_dword_none;
    case 5:
        return index < 4 ? VEHICLE(5) : halo::k_dword_none;
    case 8:
        if (race_vehicle_counts[0] < ((vehicle_set >> 4) & 7)) {
            tag = VEHICLE(0);
            race_vehicle_counts[0]++;
            if (tag != halo::k_dword_none) {
                return tag;
            }
        }
        if (race_vehicle_counts[1] < ((vehicle_set >> 7) & 7)) {
            tag = VEHICLE(1);
            race_vehicle_counts[1]++;
            if (tag != halo::k_dword_none) {
                return tag;
            }
        }
        if (race_vehicle_counts[3] < ((vehicle_set >> 13) & 7)) {
            tag = VEHICLE(5);
            race_vehicle_counts[3]++;
            if (tag != halo::k_dword_none) {
                return tag;
            }
        }
        if (race_vehicle_counts[2] < ((vehicle_set >> 10) & 7)) {
            tag = VEHICLE(2);
            race_vehicle_counts[2]++;
        }
        return tag;
    default:
        return halo::k_dword_none;
    }
#undef VEHICLE
}

/**
 * 0x46d6f0 (EAX player): while fewer than 8 have been placed, gives the player a vehicle at the nearest unused
 * type-4 starting location (from its unit's origin, if it has one), facing along the location's facing; the
 * vehicle's +0x5b0 remembers the location. The binary does not check object_new's result; neither does this.
 */
void RaceEngine::race_spawn_next_vehicle(datum_index player_index)
{
    datum_index unit_index = halo::game::player_at(player_index)->unit;
    uint8_t *unit = 0;
    int32_t count = race_used_location_count;
    int32_t location_index;
    uint8_t *location;
    datum_index tag;
    object_placement_data placement;
    datum_index vehicle;
    float facing;

    if (unit_index != halo::k_dword_none) {
        unit = (uint8_t *)halo::game::object_at(unit_index);
    }
    if (count >= 8) {
        return;
    }
    location_index = halo::game::game_engine_find_nearest_unused_type4_location(race_used_locations, count,
        unit != 0 ? (real_point3d *)(unit + 0x5c) : (real_point3d *)0);
    if (location_index == -1) {
        return;
    }
    race_used_locations[count] = location_index;
    race_used_location_count = count + 1;
    location = (uint8_t *)halo::scenario::globals().scenario->netgame_flags.pointer + location_index * 0x94;
    tag = race_pick_vehicle_tag(count);
    if (tag == halo::k_dword_none) {
        return;
    }
    halo::objects::object_placement_data_initialize(&placement, tag, halo::k_dword_none);
    placement.position = *(real_point3d *)location;
    facing = *(float *)(location + 0x0c);
    placement.forward.i = (float)halo::libm::cos(facing);
    placement.forward.j = (float)halo::libm::sin(facing);
    placement.forward.k = 0.0f;
    vehicle = halo::objects::object_new(&placement);
    *(int16_t *)((uint8_t *)halo::game::object_at(vehicle) + 0x5b0) = (int16_t)location_index;
}

/**
 * The race engine's allow_grenade_counts slot, used as a spawn hook: as the server, for a player that has not
 * died yet (+0xae) while fewer vehicles were placed than there are players, places the next race vehicle
 * (0x46d6f0, with its tag picker 0x46d5d0; both exist only for this and are statics here). Returns 1.
 *
 * @address 0x46e980
 */
uint8_t RaceEngine::allow_grenade_counts(datum_index player_index)
{
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host && halo::game::player_at(player_index)->deaths == 0 &&
        race_used_location_count < player_data->actual_count) {
        race_spawn_next_vehicle(player_index);
    }
    return 1;
}

/**
 * A ui\multiplayer_game_text string, or the empty string without the tag.
 */
const uint16_t * RaceEngine::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return tag_id == halo::k_dword_none ? (const uint16_t *)&empty_string : halo::text::text_string_list_get_string(tag_id, index);
}

/**
 * The recipient's place text: 0x4633f0 of game_engine_compare_score_to_others(recipient, 1).
 */
const uint16_t * RaceEngine::place_text(datum_index recipient)
{
    return (const uint16_t *)halo::game::game_engine_get_multiplayer_text_list(halo::game::game_engine_compare_score_to_others(recipient, 1));
}

/**
 * The kill-feed text override (called by chimera__kill_feed as (recipient, type, subject, text, count));
 * returns whether it built a text. (size is the dispatch head; the arms run to 0x46e939, tables 0x46e93c /
 * 0x46e960.) 0x23 copies string 0xa7; 0x24 / 0x25 format 0xa8 / 0xa9 with the subject's name; 0x20 formats
 * 0xaa with its laps (+0xc6) and lap time (+0xc4 / 30 s);.
 *
 * @address 0x46e480
 */
uint8_t RaceEngine::build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    ::player *player;

    if (message_type == 0x23) {
        wcsncpy(text, (const wchar_t *)game_text(0xa7), count);
        return 1;
    }
    if (message_type < 0x16 || message_type > 0x26 || (message_type > 0x16 && message_type < 0x20)) {
        return 0;
    }
    player = (::player *)halo::memory::datum_get(subject, player_data);
    if (player == 0) {
        return 0;
    }
    switch (message_type) {
    case 0x24:
    case 0x25:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x24 ? 0xa8 : 0xa9), player->name);
        return 1;
    case 0x20:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xaa), (int32_t)player->objective_time_words.race_laps,
            (double)((float)player->objective_time_words.low * 0.033333335f));
        return 1;
    case 0x21:
    case 0x22:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x21 ? 0xab : 0xac), player->name,
            (int32_t)player->objective_time_words.race_laps);
        return 1;
    case 0x26:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xad),
            (double)((float)player->objective_score * 0.033333335f));
        return 1;
    default:
        if (halo::memory::datum_get(recipient, player_data) == 0) {
            return 0;
        }
        if (game_engine_variant.engine.race.race_type == 2) {
            if (player->objective_time_words.race_laps == 1) {
                const uint16_t *format = game_text(0xae);

                halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));
            } else {
                const uint16_t *format = game_text(0xaf);

                halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), (int32_t)player->objective_time_words.race_laps);
            }
            return 1;
        }
        if (player->objective_time_words.race_laps + 1 > game_engine_variant.score_limit) {
            const uint16_t *format = game_text(0xb0);

            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));
        } else {
            const uint16_t *format = game_text(0xb1);

            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), player->objective_time_words.race_laps + 1,
                game_engine_variant.score_limit);
        }
        return 1;
    }
}

/**
 * Formats the player's short +0xc6 as L"%d" (0x006607a0) into the buffer and returns it.
 *
 * @address 0x46eab0
 */
wchar_t * RaceEngine::build_player_text(datum_index player, wchar_t *buffer)
{
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", (int32_t)*(int16_t *)((uint8_t *)halo::game::player_at(player) + 0xc6));
    return buffer;
}

/**
 * Copies string 0xb2 (the variant dword +0x7c is 2) or 0x19 of ui\multiplayer_game_text into the buffer and
 * returns it.
 */
uint16_t * RaceEngine::multiplayer_text(int16_t index)
{
    datum_index list = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return list == halo::k_dword_none ? (uint16_t *)L"" : halo::text::text_string_list_get_string(list, index);
}

/**
 * Copies string 0xb2 (the variant dword +0x7c is 2) or 0x19 of ui\multiplayer_game_text into the buffer and
 * returns it.
 *
 * @address 0x46eaf0
 */
wchar_t * RaceEngine::build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text((int16_t)(game_engine_variant.engine.race.race_type == 2 ? 0xb2 : 0x19)));
    return buffer;
}

/**
 * Formats the team's bucket score as L"%d" (0x006607a0) into the buffer and returns it.
 *
 * @address 0x46eb50
 */
wchar_t * RaceEngine::build_team_score_text(int32_t team, wchar_t *buffer)
{
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", game_engine_bucket_scores[team]);
    return buffer;
}

/**
 * Team_mode 1: the bucket score of the player's team; otherwise the player's short +0xc6 times 0x21 plus the
 * number of bits set in its team's captured-flags mask (0x006b12d4).
 *
 * @address 0x46ea00
 */
int32_t RaceEngine::get_score(datum_index player, int32_t team_mode)
{
    ::player *p = halo::game::player_at(player);
    uint32_t mask;
    int32_t bits = 0;
    int32_t i;

    if (team_mode == 1) {
        return game_engine_bucket_scores[((::player *)p)->team];
    }
    mask = ctf_team_captured_flags_mask[((::player *)p)->team];
    for (i = 0; i < 0x20; i++) {
        if ((mask & (1u << i)) != 0) {
            bits++;
        }
    }
    return ((struct player *)p)->objective_time_words.race_laps * 0x21 + bits;
}

/**
 * The team's bucket score.
 *
 * @address 0x46eaa0
 */
int32_t RaceEngine::get_team_score(int32_t team)
{
    return game_engine_bucket_scores[team];
}

/**
 * With teams: when exactly one team still has scoring capacity (0x46e250 for teams 0 and 1), whether the
 * player's team is that one; when neither has, -1; when both have, game_engine_is_object_winning. Without
 * teams game_engine_is_object_winning.
 *
 * @address 0x46eb80
 */
uint32_t RaceEngine::is_winner(datum_index player)
{
    uint8_t capacity[2];

    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        return halo::game::game_engine_is_object_winning(player);
    }
    capacity[0] = halo::game::game_engine_team_has_scoring_capacity(0);
    capacity[1] = halo::game::game_engine_team_has_scoring_capacity(1);
    if (capacity[0] != capacity[1]) {
        return (uint32_t)(capacity[halo::game::player_at(player)->team] != 0);
    }
    if (capacity[0] == 0) {
        return halo::k_dword_none;
    }
    return halo::game::game_engine_is_object_winning(player);
}

/**
 * As the server: for a valid player when the variant dword +0x80 is 2, adds its short +0xc6 to the extra
 * bucket score of its team; then checks the bucket scores for the end of the round (tail call).
 *
 * @address 0x46db00
 */
void RaceEngine::player_changed_object(datum_index player_index)
{
    ::player *player;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    player = (::player *)halo::memory::datum_get(player_index, player_data);
    if (player != 0 && game_engine_variant.engine.race.team_scoring == 2) {
        game_engine_bucket_scores_extra[player->team] += player->objective_time_words.race_laps;
    }
    halo::game::game_engine_check_bucket_scores_and_end_round();
}

/**
 * Stamps the player's +0x88 with the game tick, clears its captured-flags mask (0x006b12d4) and, as the
 * server, checks the bucket scores for the end of the round (tail call).
 *
 * @address 0x46dab0
 */
void RaceEngine::player_new_life(datum_index player)
{
    halo::game::player_at(player)->slayer_target = game_time->game_time;
    ctf_team_captured_flags_mask[player & halo::k_datum_slot_mask] = 0;
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_check_bucket_scores_and_end_round();
    }
}

/**
 * As the server, for a live player (index in range, salt 0 or matching): with variant +0x80 == 2 its word
 * +0xc6 is added to the extra bucket score of its team (or, when the flag argument equals the team, of team
 * flag != 1); its words +0xc4/+0xc6/+0xc8 are cleared, +0x88 takes the game tick and its 0x6b12d4 entry is
 * cleared. Every server call ends with game_engine_check_bucket_scores_and_end_round.
 *
 * @address 0x46ee60
 */
void RaceEngine::player_round_reset(datum_index player_index, uint8_t team_flag)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    if (player_index != halo::k_dword_none && index >= 0 && index < player_data->maximum_count) {
        uint8_t *player = (uint8_t *)player_data->data + index * player_data->size;
        int16_t player_salt = *(int16_t *)player;

        if (player_salt != 0 && (salt == 0 || player_salt == salt)) {
            if (game_engine_variant.engine.race.team_scoring == 2) {
                uint32_t team = *(uint32_t *)&((struct player *)player)->team;

                if ((uint32_t)team_flag == team) {
                    team = team_flag != 1;
                }
                game_engine_bucket_scores_extra[team] += ((struct player *)player)->objective_time_words.race_laps;
            }
            ((struct player *)player)->objective_time_words.low = 0;
            ((struct player *)player)->objective_time_words.race_laps = 0;
            ((struct player *)player)->objective_score = 0;
            ((struct player *)player)->slayer_target = game_time->game_time;
            ctf_team_captured_flags_mask[player_index & halo::k_datum_slot_mask] = 0;
        }
    }
    halo::game::game_engine_check_bucket_scores_and_end_round();
}

/**
 * The inline tail every decoder shares with message_delta_decode_compound_field: nothing changed, so the
 * stream cursor moves past this message's bits when the target is inside the stream.
 */
void RaceEngine::skip_unchanged_message(message_delta_decode_state *state)
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
uint8_t RaceEngine::read_changed(void **context, void *changed_base, void *destination)
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

namespace {

/** Copies the replicated part of the ctf state (everything before unknown_c8) from source to destination. */
void copy_replicated(ctf_globals &destination, const ctf_globals &source)
{
    memcpy(destination.bucket_scores, source.bucket_scores, sizeof(source.bucket_scores));
    memcpy(destination.team_flag_id, source.team_flag_id, sizeof(source.team_flag_id));
    memcpy(destination.team_captured_flags_mask, source.team_captured_flags_mask, sizeof(source.team_captured_flags_mask));
    destination.flag_id_mask = source.flag_id_mask;
    destination.neutral_flag_id = source.neutral_flag_id;
}

}

/**
 * The profile_post_update decoder (called as (context, ECX) by 0x466e60): a baseline message decodes the
 * replicated copy with message_delta_decode_compound_field; an incremental one reads the changed subfields
 * straight into the live ctf globals (0x6b1290) and copies the bucket scores (+0x88), the 16 dwords at +0x04,
 * the 16 at +0x44, the mask (+0x00) and the neutral flag id (+0x84) to the replicated copy;.
 *
 * @address 0x46ed30
 */
void RaceEngine::profile_post_update(void **context)
{
    message_delta_decode_state *state = halo::networking::delta_context(context)->state;
    uint8_t changed;

    if (state->incremental == 0) {
        changed = halo::networking::message_delta_decode_compound_field(context, &ctf_globals_network);
    } else {
        changed = read_changed(context, &ctf_globals_network, &ctf_globals_live);
        copy_replicated(ctf_globals_network, ctf_globals_live);
    }
    if (changed != 1) {
        return;
    }
    copy_replicated(ctf_globals_live, ctf_globals_network);
}

/**
 * GameSpy player query: for key 0x16 and an active player at the index, writes its short +0xc6 into the report
 * (0x616640) and returns 1; else 0.
 *
 * @address 0x46ef30
 */
uint8_t RaceEngine::query_player_score(int32_t key, int32_t index, void *buffer)
{
    ::player *player = (::player *)halo::memory::datum_get(halo::game::players_get_active_by_index(index), player_data);

    if (player == 0 || key != 0x16) {
        return 0;
    }
    qr2_buffer_add_int(buffer, player->objective_time_words.race_laps);
    return 1;
}

/**
 * GameSpy query report: for key 0x1d writes the team score (game_engine_bucket_scores[team]) into the report
 * (0x616640) and returns 1; other keys 0.
 *
 * @address 0x46efb0
 */
uint8_t RaceEngine::query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, game_engine_bucket_scores[team]);
    return 1;
}

/**
 * At game tick 2 queues sound 0x22 (teams) or 0x14; with teams, a team without scoring capacity (0x46e250,
 * team 0 then 1) begins the end game sequence; then the catch-up speed boost (tail call).
 *
 * @address 0x46e400
 */
void RaceEngine::unknown_48(void)
{
    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

    if (game_time->game_time == 2) {
        halo::game::game_engine_queue_multiplayer_sound(teams ? 0x22 : 0x14, halo::k_dword_none, 0);
    }
    if (current_game_engine != 0 && game_engine_teams_enabled_flag != 0) {
        if (halo::game::game_engine_team_has_scoring_capacity(0) == 0) {
            halo::game::game_engine_begin_end_game_sequence();
        }
        if (halo::game::game_engine_team_has_scoring_capacity(1) == 0) {
            halo::game::game_engine_begin_end_game_sequence();
        }
    }
    halo::game::game_engine_apply_catchup_speed_boost();
}

/**
 * The player's +0x74 becomes 0x16 and +0x78 its own handle; with a unit, while the game has not ended and as
 * the server, looks for the race flag (scenario player starting location) near the unit -- within 2.5 of its
 * parent's origin (+0xa0) through 0x461080 when it rides something, else within 1.5 / 0.6 of its own origin
 * through 0x461180 -- and scores it with game_engine_ctf_score_flag.
 *
 * @address 0x46e160
 */
void RaceEngine::update(datum_index player_index)
{
    ::player *player = halo::game::player_at(player_index);
    datum_index unit_index;
    uint8_t *unit;
    datum_index parent_index;
    int32_t result;

    *(int32_t *)&((struct player *)player)->hud_message_index = 0x16;
    ((struct player *)player)->hud_message_player = player_index;
    unit_index = ((struct player *)player)->unit;
    if (unit_index == halo::k_dword_none) {
        return;
    }
    if (current_game_engine != 0 && game_engine_state_value != 0) {
        return;
    }
    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    unit = (uint8_t *)halo::game::object_at(unit_index);
    parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != halo::k_dword_none) {
        uint8_t *parent = (uint8_t *)halo::game::object_at(parent_index);

        result = -1;
        halo::game::game_engine_find_valid_starting_locations((real_point3d *)(parent + 0xa0), 2.5f, 0.0f, 3, -1, 1, &result);
    } else {
        result = halo::game::game_engine_find_one_valid_starting_location(-1, 3, (real_point3d *)(unit + 0xa0), 1.5f, 0.6f);
    }
    if (result != -1) {
        halo::game::game_engine_ctf_score_flag(player_index, result);
    }
}

}
