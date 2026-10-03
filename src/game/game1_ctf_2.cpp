/**
 * Capture-the-flag game engine: flag objects, scoring, round resets and score text.
 */

#include "tags.h"
#include "halo/networking/game_mode.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "items.h"

#include "halo/game/game1_ctf.hpp"
#include "halo/math/api.hpp"
#include "halo/items/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern ctf_globals ctf_globals_live;
extern game_variant game_engine_variant;
extern int32_t ctf_team_flag_touch_count[2];
extern int32_t ctf_touch_counts_network[3];
extern uint8_t ctf_active_team;
extern int32_t ctf_flag_auto_return_ticks;
extern uint8_t custom_waypoints[];
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern datum_index ctf_team_flag_object[2];
extern uint8_t ctf_team_return_credit_active[2];
extern int32_t ctf_team_return_credit_ticks[2];
extern int32_t ctf_notify_throttle_tick;
extern int32_t ctf_neutral_flag_id;
extern uint32_t ctf_team_captured_flags_mask[];
extern int32_t game_engine_state_value;
extern float game_engine_end_game_timer;
extern int32_t ctf_flag_capture_limit_006b0ea0;
extern data_array *player_data;
extern game_engine_definition *current_game_engine;
}

namespace halo::game::engine1 {

/**
 * Picks a random flag index other than the excluded one.
 *
 * @address 0x46dfe0
 */
int32_t Ctf::pick_random_flag(int32_t exclude_flag_index)
{
    int16_t active_count = 0;
    int32_t i;
    int32_t pick;
    int32_t flag_count;
    ScenarioNetgameFlags *flags;

    for (i = 0; i < 0x20; i++) {
        if ((ctf_globals_live.flag_id_mask & (1u << (i & 0x1f))) != 0) {
            active_count++;
        }
    }
    if (exclude_flag_index != -1) {
        active_count--;
    }

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    pick = (int16_t)(((halo::math::globals().random_seed_global >> 0x10) *
                      (uint32_t)(int32_t)(int16_t)active_count) >> 0x10);

    flag_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    if (flag_count < 1) {
        return -1;
    }
    flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;

    for (i = 0; i < flag_count; i++) {
        if (flags[i].type == 3 && flags[i].usage_id != exclude_flag_index) {
            if (pick == 0) {
                return flags[i].usage_id;
            }
            pick--;
        }
    }
    return -1;
}

/**
 * File-local helper of Ctf: skip unchanged message.
 */
void Ctf::skip_unchanged_message(message_delta_decode_state *state)
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
 * File-local helper of Ctf: read changed.
 */
uint8_t Ctf::read_changed(void **context, void *changed_base, void *destination)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    int32_t bits = halo::networking::message_delta_read_changed_subfields(state, (uint8_t *)(context + 1), (int32_t)changed_base, (int32_t)destination);

    state->bits_read += bits;
    if (bits != 0) {
        state->changed = 1;
        return 1;
    }
    skip_unchanged_message(state);
    return 0;
}

/**
 * Decodes the capture-the-flag part of a player profile update message.
 *
 * @address 0x469d10
 */
void Ctf::profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    uint8_t team;

    if (state->incremental == 0) {
        ctf_touch_counts_network[0] = 0;
        ctf_touch_counts_network[1] = 0;
        ctf_touch_counts_network[2] = 0;
        changed = halo::networking::message_delta_decode_compound_field(context, ctf_touch_counts_network);
    } else {
        int32_t local[3];

        local[0] = ctf_touch_counts_network[0];
        local[1] = ctf_touch_counts_network[1];
        local[2] = ctf_touch_counts_network[2];
        changed = read_changed(context, ctf_touch_counts_network, local);
        ctf_touch_counts_network[0] = local[0];
        ctf_touch_counts_network[1] = local[1];
        ctf_touch_counts_network[2] = local[2];
    }
    team = (uint8_t)ctf_touch_counts_network[2];
    if (changed != 1) {
        return;
    }
    if (game_engine_variant.engine.ctf.single_flag_time > 0 && ctf_active_team != team) {
        memset(custom_waypoints, 0, 0x80);
    }
    ctf_team_flag_touch_count[0] = ctf_touch_counts_network[0];
    ctf_team_flag_touch_count[1] = ctf_touch_counts_network[1];
    ctf_active_team = team;
    ctf_flag_auto_return_ticks = *(int32_t *)context[0x11];
}

/**
 * Broadcasts the capture-the-flag profile state to a machine after profiles changed.
 *
 * @address 0x469bf0
 */
void Ctf::profiles_updated(int32_t mode, int32_t machine_index)
{
    int32_t ticks = ctf_flag_auto_return_ticks;
    void *extra[1];
    void *items[2];
    int32_t bits;

    extra[0] = &ticks;
    if (mode == 0) {
        items[0] = ctf_touch_counts_network;
        bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::ctf_profiles_updated), (int32_t)extra, items, 0, 1, 0);
    } else {
        int32_t live[3];
        void *baseline[1];

        live[0] = ctf_team_flag_touch_count[0];
        live[1] = ctf_team_flag_touch_count[1];
        live[2] = ctf_active_team;
        items[0] = live;
        items[1] = (void *)ticks;
        baseline[0] = ctf_touch_counts_network;
        bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::ctf_profiles_updated), (int32_t)extra, items,
            (int32_t)baseline, 1, 0);
        ctf_touch_counts_network[0] = live[0];
        ctf_touch_counts_network[1] = live[1];
        ctf_touch_counts_network[2] = live[2];
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
 * Capture-the-flag reset-objects callback: puts every flag back to its stand.
 *
 * @address 0x46a010
 */
void Ctf::reset_objects(void)
{
    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    if (ctf_team_flag_object[0] != halo::k_dword_none) {
        halo::game::game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[0]);
    }
    if (ctf_team_flag_object[1] != halo::k_dword_none) {
        halo::game::game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[1]);
    }
    ctf_flag_auto_return_ticks = game_engine_variant.engine.ctf.single_flag_time;
    ctf_team_flag_touch_count[0] = 0;
    ctf_team_flag_touch_count[1] = 0;
    ctf_team_return_credit_active[0] = 0;
    ctf_team_return_credit_active[1] = 0;
    ctf_team_return_credit_ticks[0] = 0;
    ctf_team_return_credit_ticks[1] = 0;
    ctf_notify_throttle_tick = 0;
    memset(custom_waypoints, 0, 0x80);
}

/**
 * Re-initializes/returns all Capture-the-Flag flags mid-round in team play, resetting their state and
 * reassigning per-team flags similarly to the full round-start initializer.
 *
 * @address 0x46efe0
 */
void Ctf::return_all_flags(void)
{
    int32_t lowest_usage_id = 0x20;
    int32_t flag_count;
    ScenarioNetgameFlags *flags;
    int32_t i;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }

    halo::game::game_engine_ctf_assign_flag_ids();
    {
        uint32_t *raw = (uint32_t *)&ctf_globals_live;
        for (i = 0; i < (int32_t)(sizeof(ctf_globals_live) / 4); i++) raw[i] = 0;
    }

    flag_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;

    for (i = 0; i < flag_count; i++) {
        int32_t usage_id;
        if (flags[i].type != 3 || flags[i].usage_id >= 0x20) {
            continue;
        }
        usage_id = flags[i].usage_id;
        if (usage_id < lowest_usage_id) {
            lowest_usage_id = usage_id;
        }
        ctf_globals_live.flag_id_mask |= 1u << (usage_id & 0x1f);
        halo::game::custom_waypoint_register((datum_index)0, (int16_t)0, (real_point3d *)0, "flag_blue", 0.0f,
            (datum_index)halo::k_dword_none, (int16_t)halo::k_dword_none);
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        ctf_neutral_flag_id = halo::game::game_engine_ctf_pick_random_flag(-1);
        return;
    }
    if (game_engine_variant.engine.race.race_type == 0) {
        for (i = 0; i < 16; i++) {
            ctf_globals_live.team_flag_id[i] = lowest_usage_id;
        }
        return;
    }
    for (i = 0; i < 16; i++) {
        ctf_globals_live.team_flag_id[i] = -1;
    }
}

/**
 * Processes a flag being scored for a team: validates eligibility, updates per-team flag bookkeeping, and
 * completes the capture once all required flags (in multi-flag mode) or the single flag (in neutral mode) are
 * in.
 *
 * Original register convention: stack -> team, EAX -> scenario_flag_index.
 *
 * @address 0x46e080
 */
void Ctf::score_flag(uint32_t team, int32_t scenario_flag_index)
{
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;
    int16_t usage_id = flags[scenario_flag_index].usage_id;
    uint32_t idx = team & 0xffff;

    if (halo::game::game_engine_ctf_is_flag_eligible_for_capture(team, usage_id) == 0) {
        return;
    }

    halo::game::game_engine_queue_multiplayer_sound(0x1a, team, 1);
    if (ctf_globals_live.team_flag_id[idx] == -1) {
        ctf_globals_live.team_flag_id[idx] = usage_id;
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        halo::game::game_engine_ctf_on_flag_captured(team);
        ctf_neutral_flag_id = halo::game::game_engine_ctf_pick_random_flag(ctf_neutral_flag_id);
        return;
    }
    if (ctf_team_captured_flags_mask[idx] == ctf_globals_live.flag_id_mask) {
        halo::game::game_engine_ctf_on_flag_captured(team);
        return;
    }
    ctf_team_captured_flags_mask[idx] |= 1u << (usage_id & 0x1f);
}

/**
 * Capture-the-flag engine definition slot +0x48.
 *
 * @address 0x469180
 */
void Ctf::unknown_48(void)
{
    int32_t limit = ctf_flag_capture_limit_006b0ea0;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    if ((ctf_team_flag_touch_count[0] >= limit || ctf_team_flag_touch_count[1] >= limit) && game_engine_state_value == 0) {
        halo::networking::globals().server->game_over = 1;
        game_engine_state_value = 1;
        game_engine_end_game_timer = 7.0f;
        halo::game::game_engine_queue_multiplayer_sound(1, halo::k_dword_none, 0);
        halo::interface::widget_close_all();
        halo::game::game_engine_send_end_game_notification(1);
    }
    if (ctf_team_return_credit_active[0] != 0) {
        int32_t ticks = ctf_team_return_credit_ticks[0];

        if (ticks > 0x258) {
            halo::game::game_engine_queue_multiplayer_sound(8, halo::k_dword_none, 1);
            ticks = 0;
        }
        ctf_team_return_credit_ticks[0] = ticks + 1;
    }
    if (ctf_team_return_credit_active[1] != 0) {
        int32_t ticks = ctf_team_return_credit_ticks[1];

        if (ticks > 0x258) {
            halo::game::game_engine_queue_multiplayer_sound(0xb, halo::k_dword_none, 1);
            ticks = 0;
        }
        ctf_team_return_credit_ticks[1] = ticks + 1;
    }
}

/**
 * Capture-the-flag engine definition slot +0x60: a unit and item query.
 *
 * @address 0x469270
 */
uint8_t Ctf::unknown_60(datum_index unit_index, datum_index item_index)
{
    datum_index player = halo::game::player_index_from_unit_index(unit_index);
    uint8_t *weapon;

    if (player == halo::k_dword_none || item_index == halo::k_dword_none || halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return 1;
    }
    weapon = (uint8_t *)halo::objects::object_try_and_get(item_index, 4);
    if (weapon != 0 && (uint8_t)halo::items::weapon_must_be_readied(item_index) != 0 && (weapon[0x22c] & 0x40) == 0 &&
        ((struct weapon_object *)weapon)->base.owner_team == halo::game::player_at(player)->team) {
        return 0;
    }
    return 1;
}

/**
 * Capture-the-flag per-player update callback: lets a unit carrying the enemy flag touch its own flag and drop
 * the carried one.
 *
 * @address 0x468a20
 */
void Ctf::update(datum_index player_index)
{
    ::player *player = halo::game::player_at(player_index);
    datum_index unit_index;
    uint8_t *unit;
    int16_t weapon_slot;
    datum_index weapon;
    int32_t team;

    if (halo::game::unit_has_must_be_readied_weapon(player_index) != 0) {
        halo::game::unit_reset_gauge_if_flagged(player_index);
    }
    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    unit_index = ((struct player *)player)->unit;
    if (unit_index == halo::k_dword_none) {
        return;
    }
    unit = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit_index & halo::k_datum_slot_mask) * 12 + 8);
    weapon_slot = ((unit_object *)unit)->unit.current_weapon_index;
    if (weapon_slot == -1) {
        return;
    }
    weapon = *(datum_index *)(unit + 0x2f8 + weapon_slot * 4);
    if (weapon == halo::k_dword_none) {
        return;
    }
    if (current_game_engine != 0 && game_engine_state_value != 0) {
        return;
    }
    if (halo::items::weapon_must_be_readied(weapon) == 0) {
        return;
    }
    team = ((struct player *)player)->team;
    if (halo::game::game_engine_ctf_point_within_team_flag_radius(1.0f, team, (real_point3d *)(unit + 0x5c)) == 0) {
        return;
    }
    if (game_engine_variant.engine.ctf.flag_at_home_to_score != 0 && game_engine_variant.engine.ctf.single_flag_time == 0) {
        uint8_t *flag = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (ctf_team_flag_object[team] & halo::k_datum_slot_mask) * 12 + 8);

        if (((*(uint32_t *)(flag + 0x22c) >> 6) & 1) != 0) {
            halo::game::game_engine_ctf_notify_flag_carried_throttled((int32_t)player_index);
            return;
        }
    }
    halo::game::game_engine_ctf_player_touch_flag(player_index, team);
    halo::game::game_engine_ctf_player_drop_flag(player_index, weapon);
}

}
