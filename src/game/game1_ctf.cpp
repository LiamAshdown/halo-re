/**
 * Capture-the-flag game engine: flag objects, scoring, round resets and score text.
 */

#include "tags.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#include <wchar.h>
#include <string.h>
#include "items.h"
#include <stdint.h>

#include "halo/game/game1_ctf.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/libm.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/ai/api.hpp"

static auto &ctf_globals_live = halo::link::ref<ctf_globals>(halo::game::vars().ctf_globals_live);
static auto &ctf_globals_network = halo::link::ref<ctf_globals>(halo::game::vars().ctf_globals_network);
static auto &ctf_neutral_flag_id = halo::link::ref<int32_t>(halo::game::vars().ctf_neutral_flag_id);
static auto &shared_hud_text_draw_state = halo::link::ref<uint8_t>(halo::game::vars().shared_hud_text_draw_state);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &ctf_team_flag_touch_count = halo::link::ref<int32_t [2]>(halo::game::vars().ctf_team_flag_touch_count);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &ctf_flag_auto_return_ticks = halo::link::ref<int32_t>(halo::game::vars().ctf_flag_auto_return_ticks);
static auto &missing_string_text = halo::link::ref<uint16_t []>(halo::ui::vars().missing_string_text);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);
static auto &game_engine_ctf_reset_ticks = halo::link::ref<int32_t>(halo::game::vars().game_engine_ctf_reset_ticks);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &custom_waypoints = halo::link::ref<custom_waypoint [k_maximum_custom_waypoints]>(halo::game::vars().custom_waypoints);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &ctf_team_flag_stand_position = halo::link::ref<real_point3d *[2]>(halo::game::vars().ctf_team_flag_stand_position);
static auto &ctf_team_flag_object = halo::link::ref<datum_index [2]>(halo::game::vars().ctf_team_flag_object);
static auto &ctf_flag_capture_limit_006b0ea0 = halo::link::ref<int32_t>(halo::game::vars().ctf_flag_capture_limit_006b0ea0);
static auto &ctf_active_team = halo::link::ref<uint8_t>(halo::game::vars().ctf_active_team);
static auto &ctf_single_flag_mode = halo::link::ref<uint8_t>(halo::game::vars().ctf_single_flag_mode);
static auto &ctf_touch_counts_network = halo::link::ref<int32_t [3]>(halo::game::vars().ctf_touch_counts_network);
static auto &network_single_flag_force_reset_value = halo::link::ref<uint8_t>(halo::game::vars().network_single_flag_force_reset_value);
static auto &ctf_team_captured_flags_mask = halo::link::ref<uint32_t []>(halo::game::vars().ctf_team_captured_flags_mask);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &ctf_notify_throttle_tick = halo::link::ref<int32_t>(halo::game::vars().ctf_notify_throttle_tick);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &ctf_team_return_credit_active = halo::link::ref<uint8_t [2]>(halo::game::vars().ctf_team_return_credit_active);
static auto &ctf_team_return_credit_ticks = halo::link::ref<int32_t [2]>(halo::game::vars().ctf_team_return_credit_ticks);

namespace halo::game::engine1 {

/**
 * Assigns each type-3 scenario starting location (used for CTF-style flag stands) a unique slot id in the
 * range 0-31, resolving any duplicates.
 *
 * @address 0x46d800
 */
void Ctf::assign_flag_ids(void)
{
    int32_t flag_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;
    uint32_t used_mask = 0;
    int32_t i;

    for (i = 0; i < flag_count; i++) {
        int16_t usage_id;
        if (flags[i].type != 3) {
            continue;
        }
        usage_id = flags[i].usage_id;
        if (usage_id < 0 || usage_id >= 0x20) {
            continue;
        }
        {
            uint32_t bit = 1u << (usage_id & 0x1f);
            if ((used_mask & bit) == 0) {
                used_mask |= bit;
            } else {
                int32_t free_id;
                for (free_id = 0; free_id < 0x20; free_id++) {
                    if ((used_mask & (1u << (free_id & 0x1f))) == 0) {
                        used_mask |= (1u << (free_id & 0x1f));
                        break;
                    }
                }
                flags[i].usage_id = (int16_t)free_id;
            }
        }
    }
}

/**
 * Requests or broadcasts the current Capture-the-Flag state (active flags, per-team assignments, and captured
 * bitmasks) over the network.
 *
 * @address 0x46ec10
 */
void Ctf::broadcast_state(void *request_fields, int32_t machine_index)
{
    int32_t encoded_bits;

    if (request_fields == (void *)0) {
        void *field = &ctf_globals_network;
        encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::ctf_state), 0, &field, 0, 1, 0);
    } else {
        void *fields0 = &ctf_globals_live;
        void *fields1 = &ctf_globals_network;
        int32_t i;

        encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::ctf_state), 0, (void **)&fields0, (uint32_t)&fields1, 1, 0);

        ctf_globals_network.neutral_flag_id = ctf_neutral_flag_id;
        for (i = 0; i < 16; i++) {
            ctf_globals_network.bucket_scores[i] = ctf_globals_live.bucket_scores[i];
        }
        for (i = 0; i < 16; i++) {
            ctf_globals_network.team_flag_id[i] = ctf_globals_live.team_flag_id[i];
        }
        for (i = 0; i < 16; i++) {
            ctf_globals_network.team_captured_flags_mask[i] = ctf_globals_live.team_captured_flags_mask[i];
        }
        ctf_globals_network.flag_id_mask = ctf_globals_live.flag_id_mask;
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

/**
 * File-local helper of Ctf: game text.
 */
const uint16_t *Ctf::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return tag_id == halo::k_dword_none ? (const uint16_t *)&empty_string : halo::text::text_string_list_get_string(tag_id, index);
}

/**
 * File-local helper of Ctf: place text.
 */
const uint16_t *Ctf::place_text(datum_index recipient)
{
    return (const uint16_t *)halo::game::game_engine_get_multiplayer_text_list(halo::game::game_engine_compare_score_to_others(recipient, 1));
}

/**
 * Builds the text of a capture-the-flag event message for a recipient.
 *
 * @address 0x469300
 */
uint8_t Ctf::build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    static const int16_t copy_string[11] = { 0x90, 0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99 };

    switch (message_type) {
    case 0x20:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x8c), ctf_team_flag_touch_count[0], ctf_team_flag_touch_count[1]);
        return 1;
    case 0x21:
    case 0x22:
    case 0x23: {
        ::player *player = (::player *)halo::memory::datum_get(recipient, player_data);
        int32_t team;

        if (player == 0) {
            return 0;
        }
        team = player->team;
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text((int16_t)(0x8d + message_type - 0x21)),
            ctf_team_flag_touch_count[team], ctf_team_flag_touch_count[(team + 1) % 2]);
        return 1;
    }
    case 0x24:
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)&empty_string);
        return 1;
    case 0x30:
    case 0x31: {
        wchar_t time[0x20];

        halo::game::game_time_format_minutes_seconds((uint32_t)ctf_flag_auto_return_ticks, 0x20, time);
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)L"%s (%s)", game_text(message_type == 0x30 ? 0x98 : 0x99),
            time);
        return 1;
    }
    default:
        if (message_type >= 0x25 && message_type <= 0x2f) {
            wcsncpy(text, (const wchar_t *)game_text(copy_string[message_type - 0x25]), count);
            return 1;
        }
        return 0;
    }
}

/**
 * Builds the per-player score text shown for capture the flag.
 *
 * @address 0x4699f0
 */
wchar_t *Ctf::build_player_text(datum_index player, wchar_t *buffer)
{
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", (int32_t)halo::game::player_at(player)->objective_score);
    return buffer;
}

/**
 * File-local helper of Ctf: multiplayer text.
 */
uint16_t *Ctf::multiplayer_text(int16_t index)
{
    datum_index list = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    if (list == halo::k_dword_none) {
        return (uint16_t *)L"";
    }
    {
        uint8_t *strings = (uint8_t *)halo::game::tag_data_at(list);

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
 * Builds the scoreboard header text for capture the flag.
 *
 * @address 0x469a30
 */
wchar_t *Ctf::build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text(0x9a));
    return buffer;
}

/**
 * Builds the team score text shown for capture the flag.
 *
 * @address 0x469ab0
 */
wchar_t *Ctf::build_team_score_text(int32_t team, wchar_t *buffer)
{
    halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", ctf_team_flag_touch_count[team]);
    return buffer;
}

/**
 * Creates the flag object for a netgame flag at the given position and name index.
 *
 * Original register convention: EAX -> position, stack -> name_index.
 *
 * @address 0x468360
 */
datum_index Ctf::create_flag_object(real_point3d *position, uint16_t name_index)
{
    object_placement_data placement;
    datum_index flag_tag;
    uint32_t role;
    datum_index new_object;
    object_header *hdr;
    uint8_t header_flags;
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;

    flag_tag = (datum_index)(uint32_t)((mp_info->flag.tag_id.id << 16) | mp_info->flag.tag_id.index);

    halo::objects::object_placement_data_initialize(&placement, flag_tag, (datum_index)halo::k_dword_none);
    placement.position = *position;
    placement.owner_team = (int16_t)name_index;

    role = 3;
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        int16_t object_type = *(int16_t *)halo::game::tag_data_at((uint32_t)placement.definition_tag);
        if (object_type_definitions[object_type]->network_delta_message_type != -1) {
            role = 0;
        }
    }

    new_object = halo::objects::object_new_with_datum_role_control(&placement, role);

    hdr = (object_header *)halo::objects::globals().object_data->data + ((uint32_t)new_object & halo::k_datum_slot_mask);
    header_flags = hdr->flags;
    hdr->flags = header_flags & ~_object_header_in_pvs_pass_bit;
    if ((header_flags & _object_header_active_bit) == 0) {
        halo::objects::object_mark_pending_delete((uint32_t)new_object);
    }

    return new_object;
}

/**
 * Returns the capture-the-flag score of a player or team.
 *
 * @address 0x469990
 */
int32_t Ctf::get_score(datum_index player, int32_t team_mode)
{
    ::player *p = halo::game::player_at(player);

    if (team_mode != 0) {
        return ctf_team_flag_touch_count[p->team];
    }
    return p->objective_score;
}

/**
 * Returns the capture-the-flag score of a team.
 *
 * @address 0x4699d0
 */
int32_t Ctf::get_team_score(int32_t team)
{
    return ctf_team_flag_touch_count[team];
}

/**
 * Creates the flag objects of the scenario for a new capture-the-flag game.
 *
 * @address 0x46d890
 */
int32_t Ctf::initialize_flags(void)
{
    int32_t lowest_usage_id = 0x20;
    int32_t flag_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;
    int32_t i;

    halo::game::game_engine_ctf_assign_flag_ids();

    {
        memset(&ctf_globals_live, 0, sizeof(ctf_globals_live));
    }
    {
        memset(&ctf_globals_network, 0, sizeof(ctf_globals_network));
    }
    game_engine_ctf_reset_ticks = 0x1e;

    for (i = 0; i < flag_count; i++) {
        ScenarioNetgameFlags *flag = &flags[i];
        int32_t usage_id;

        if (flag->type != 3 || flag->usage_id >= 0x20) {
            continue;
        }
        usage_id = flag->usage_id;
        if (usage_id < lowest_usage_id) {
            lowest_usage_id = usage_id;
        }
        ctf_globals_live.flag_id_mask |= 1u << (usage_id & 0x1f);

        {
            custom_waypoint *w = &custom_waypoints[usage_id];
            w->owner = (datum_index)halo::k_dword_none;
            w->icon = halo::interface::hud_waypoint_arrow_find("flag_blue");
            w->active = 1;
            w->position.x = flag->position.x;
            w->position.y = flag->position.y;
            w->position.z = flag->position.z;
            w->player = (datum_index)halo::k_dword_none;
            w->team = (int16_t)halo::k_word_none;
            w->position.z += 0.63f;
        }
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        ctf_neutral_flag_id = halo::game::game_engine_ctf_pick_random_flag(-1);
        return 1;
    }
    if (game_engine_variant.engine.race.race_type != 0) {
        for (i = 0; i < 16; i++) {
            ctf_globals_live.team_flag_id[i] = -1;
        }
        return -0xff;
    }
    for (i = 0; i < 16; i++) {
        ctf_globals_live.team_flag_id[i] = lowest_usage_id;
    }
    return 1;
}

/**
 * File-local helper of Ctf: distance squared.
 */
float Ctf::distance_squared(const real_point3d *a, const real_point3d *b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;

    return dz * dz + dy * dy + dx * dx;
}

/**
 * Capture-the-flag initialize-for-new-game callback: resets the flag state and creates the flags; returns
 * false to abort the start.
 *
 * @address 0x4684a0
 */
uint8_t Ctf::initialize_for_new_game(void)
{
    int32_t team;
    int16_t count;
    int16_t i;

    memset(ctf_team_flag_stand_position, 0, 0xd * 4);
    ctf_touch_counts_network[0] = 0;
    ctf_touch_counts_network[1] = 0;
    ctf_touch_counts_network[2] = 0;
    ctf_team_flag_object[0] = halo::k_dword_none;
    ctf_team_flag_object[1] = halo::k_dword_none;
    game_engine_ctf_reset_ticks = 0x3c;
    for (team = 0; team < 2; team++) {
        int32_t index = -1;
        int32_t slot;

        halo::game::game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 0, (int16_t)team, 1, &index);
        ctf_team_flag_touch_count[team] = 0;
        slot = game_engine_variant.engine.ctf.assault != 0 ? (team + 1) % 2 : team;
        ctf_team_flag_stand_position[slot] = 0;
        if (index != -1) {
            ctf_team_flag_stand_position[slot] = (real_point3d *)((uint8_t *)halo::scenario::globals().scenario->netgame_flags.pointer + index * 0x94);
        }
    }
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        if (game_engine_variant.engine.ctf.single_flag_time > 0) {
            int32_t active;

            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            active = (int16_t)((((halo::math::globals().random_seed_global >> 16) << 1) & 0xffffffff) >> 16);
            if (ctf_team_flag_stand_position[active] != 0) {
                datum_index flag = halo::game::game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[active], (uint16_t)active);

                if (flag != halo::k_dword_none) {
                    ctf_team_flag_object[active] = flag;
                }
            }
            ctf_active_team = (uint8_t)active;
            halo::game::game_engine_broadcast_kill_feed_to_team(0x2f, active % 2, 1);
            halo::game::game_engine_broadcast_kill_feed_to_team(0x2e, (active + 1) % 2, 1);
            ctf_flag_auto_return_ticks = game_engine_variant.engine.ctf.single_flag_time;
        } else {
            for (team = 0; team < 2; team++) {
                if (ctf_team_flag_stand_position[team] != 0) {
                    datum_index flag = halo::game::game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[team], (uint16_t)team);

                    if (flag != halo::k_dword_none) {
                        ctf_team_flag_object[team] = flag;
                    }
                }
            }
        }
    } else if (game_engine_variant.engine.ctf.single_flag_time > 0) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    }
    ctf_flag_capture_limit_006b0ea0 = game_engine_variant.score_limit;
    count = *(int16_t *)&halo::scenario::globals().scenario->player_starting_locations.count;
    for (i = 0; i < count; i++) {
        ScenarioPlayerStartingLocation *start = nullptr;
        int16_t type;
        uint8_t belongs;

        if (i >= 0 && i < *(int32_t *)&halo::scenario::globals().scenario->player_starting_locations.count) {
            start = (ScenarioPlayerStartingLocation *)halo::scenario::globals().scenario->player_starting_locations.pointer + i;
        }
        type = (int16_t)start->team_index;
        if (type != 0 && type != 1) {
            continue;
        }
        if (current_game_engine != 0) {
            int32_t k;

            belongs = 0;
            for (k = 0; k < 4; k++) {
                int16_t game_type = (&start->type_0)[k];

                if (game_type == 1 || game_type == 12) {
                    belongs = 1;
                }
            }
        } else {
            belongs = start->type_0 == 0 && start->type_1 == 0 && start->type_2 == 0 && start->type_3 == 0;
        }
        if (belongs) {
            float own = distance_squared((real_point3d *)start, ctf_team_flag_stand_position[type % 2]);
            float other = distance_squared((real_point3d *)start, ctf_team_flag_stand_position[(type + 1) % 2]);

            if (game_engine_variant.engine.ctf.assault != 0 ? own < other : own > other) {
                start->team_index = 3;
            }
        }
    }
    ctf_single_flag_mode = network_single_flag_force_reset_value;
    return 1;
}

/**
 * Determines whether a given flag id is the one currently eligible to be captured for a particular team/slot,
 * accounting for the active CTF sub-mode.
 *
 * Original register convention: ECX -> team, EDI -> flag_id.
 *
 * @address 0x46df30
 */
uint8_t Ctf::is_flag_eligible_for_capture(uint32_t team, int32_t flag_id)
{
    uint32_t team_idx = team & 0xffff;
    uint32_t uncaptured_mask = ~ctf_team_captured_flags_mask[team_idx] & ctf_globals_live.flag_id_mask;
    player *p;

    if (flag_id > 0x1f) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + team_idx * sizeof(player));
    if (((struct player *)p)->objective_time_words.race_laps >= game_engine_variant.score_limit) {
        return 0;
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        return ctf_neutral_flag_id == flag_id;
    }
    if (ctf_team_captured_flags_mask[team_idx] == ctf_globals_live.flag_id_mask) {
        return flag_id == ctf_globals_live.team_flag_id[team_idx];
    }
    if ((uncaptured_mask & (1u << (flag_id & 0x1f))) != 0) {
        if (game_engine_variant.engine.race.race_type == 0) {
            int32_t i;
            for (i = 0; i != flag_id; i++) {
                if ((uncaptured_mask & (1u << (i & 0x1f))) != 0) {
                    return 0;
                }
                if (i > 0x1f) {
                    return 1;
                }
            }
        }
        return 1;
    }
    return 0;
}

/**
 * Invokes a per-team update/notify routine for both the current team and its opposing team.
 *
 * Original register convention: EAX -> team.
 *
 * @address 0x468460
 */
void Ctf::notify_both_teams(int32_t team)
{
    halo::game::game_engine_broadcast_kill_feed_to_team(0x2f, team % 2, 1);
    halo::game::game_engine_broadcast_kill_feed_to_team(0x2e, (team + 1) % 2, 1);
}

/**
 * Throttles a periodic flag-related update/notify call to at most once every four seconds.
 *
 * Original register convention: EDI -> target_player.
 *
 * @address 0x4689e0
 */
void Ctf::notify_flag_carried_throttled(int32_t target_player)
{
    if (ctf_notify_throttle_tick < game_time->game_time) {
        halo::game::game_engine_queue_multiplayer_sound(0x1c, (datum_index)target_player, 1);
        ctf_notify_throttle_tick = game_time->game_time + 0x78;
    }
}

/**
 * Called when a flag object expires so the engine can respawn it.
 *
 * @address 0x469960
 */
void Ctf::object_expired(datum_index object_index)
{
    uint8_t *object = (uint8_t *)halo::game::object_at(object_index);

    *(int32_t *)&((struct object *)object)->owner_linkage = -1;
}

/**
 * Part of the profile block.
 *
 * @address 0x46dde0
 */
void Ctf::on_flag_captured(uint32_t flag_index)
{
    player *p = halo::game::player_at(flag_index);
    int16_t elapsed = (int16_t)(game_time->game_time - p->slayer_target);
    uint8_t new_record = 0;

    ctf_team_captured_flags_mask[flag_index & halo::k_datum_slot_mask] = 0;
    halo::game::game_engine_queue_multiplayer_sound(0x2a, flag_index, 1);

    ((struct player *)p)->objective_time_words.low = elapsed;
    if (((struct player *)p)->objective_time_words.race_laps != 0) {
        if (elapsed <= ((struct player *)p)->objective_score) {
            new_record = 1;
            ((struct player *)p)->objective_score = elapsed;
        }
    } else {
        ((struct player *)p)->objective_score = elapsed;
    }

    ((struct player *)p)->objective_time_words.race_laps += 1;
    p->slayer_target = game_time->game_time;

    halo::game::game_engine_check_bucket_scores_and_end_round();

    if (game_engine_variant.engine.race.race_type == 2) {
        halo::game::game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x23, 0x24, 0x22, flag_index, 1);
    } else {
        halo::game::game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x20, 0x21, 0x22, flag_index, 1);
    }

    if (game_engine_variant.engine.race.race_type != 2 && new_record != 0) {
        data_iterator iter;
        void *element;
        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            uint32_t recipient = (flag_index == halo::k_dword_none) ? (uint32_t)iter.index : flag_index;
            halo::game::chimera__kill_feed((datum_index)recipient, 0x26, flag_index, 1, 0);
            element = halo::memory::data_iterator_next(&iter);
        }
    }
}

/**
 * Attaches/updates a flag-carry state on a player's unit, then resets the associated team's flag-return
 * credit.
 *
 * Original register convention: EAX -> player_index, stack -> flag_object_index.
 *
 * @address 0x4688b0
 */
void Ctf::player_drop_flag(uint32_t player_index, datum_index flag_object_index)
{
    player *p = halo::game::player_at(player_index);
    uint32_t unit_index = (uint32_t)p->unit;
    object *unit_obj = halo::game::object_at(unit_index);

    if (unit_obj->network_role == 0) {
        halo::units::unit_dispatch_scripted_event_1b(1, unit_index);
    }
    halo::units::unit_drop_current_weapon(unit_index, 1);
    halo::game::game_engine_ctf_reset_team_return_credit(flag_object_index);
}

/**
 * Per-tick per-player handling of objective (flag) pickup/carry/drop bookkeeping and associated event
 * notifications.
 *
 * Original register convention: stack -> flag_handle, player_index.
 *
 * @address 0x4697e0
 */
uint8_t Ctf::player_flag_tick(uint32_t flag_handle, uint32_t player_index)
{
    object *flag_obj = halo::game::object_at(flag_handle);
    int16_t team = ((struct object *)flag_obj)->owner_team;

    if (player_index != halo::k_dword_none && halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        player *p = halo::game::player_at(player_index);

        if ((int32_t)team == p->team) {
            if (game_engine_variant.engine.ctf.flag_must_reset == 0) {
                if ((((weapon_object *)flag_obj)->weapon.flags & _weapon_game_object_taken_bit) != 0) {
                    if (halo::game::game_engine_is_inactive() != 0) {
                        ctf_team_return_credit_active[team] = 0;
                        ctf_team_return_credit_ticks[team] = 0;
                        ((struct player *)p)->objective_time_words.race_laps += 1;
                        halo::game::game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index, 1);
                        halo::game::game_engine_queue_multiplayer_sound(p->team != 0 ? 9 : 0xc, halo::k_dword_none, 1);
                    }
                }
                halo::game::game_engine_ctf_reset_team_return_credit(flag_handle);
                return 0;
            }
            if ((((weapon_object *)flag_obj)->weapon.flags & _weapon_game_object_taken_bit) != 0) {
                halo::game::game_engine_ctf_notify_flag_carried_throttled((int32_t)player_index);
            }
            return 0;
        }

        if ((((weapon_object *)flag_obj)->weapon.flags & _weapon_game_object_taken_bit) == 0 &&
            (current_game_engine == 0 || game_engine_state_value == 0)) {
            ((struct player *)p)->objective_time_words.low += 1;
            if (game_engine_variant.engine.ctf.assault == 0) {
                halo::game::game_engine_queue_multiplayer_sound(p->team != 0 ? 8 : 0xb, halo::k_dword_none, 1);
                ctf_team_return_credit_active[team] = 1;
                ctf_team_return_credit_ticks[team] = 0;
                halo::game::game_engine_broadcast_kill_feed_by_relationship(player_index, halo::k_dword_none, 0x29, 0x26, player_index, 1);
            }
        }
        ((weapon_object *)flag_obj)->weapon.flags |= _weapon_game_object_taken_bit;
    }
    return 1;
}

/**
 * Resets the capture-the-flag state of a player at a round reset.
 *
 * @address 0x469f10
 */
void Ctf::player_round_reset(datum_index player_index)
{
    ::player *player = (::player *)halo::memory::datum_get(player_index, player_data);

    if (player != 0) {
        player->objective_score = 0;
    }
}

/**
 * Increments a player's/team's flag-touch counters and fires an associated medal/event notification.
 *
 * Original register convention: stack -> player_index, EAX -> team.
 *
 * @address 0x468910
 */
void Ctf::player_touch_flag(uint32_t player_index, int32_t team)
{
    player *p = halo::game::player_at(player_index);
    int16_t *touch_count = (int16_t *)((uint8_t *)p + 0xc8);

    ctf_team_flag_touch_count[team]++;
    (*touch_count)++;

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_player_profile_cache_sync_all(1, (void *)halo::k_dword_none);
    }
    halo::game::game_engine_queue_multiplayer_sound(p->team != 0 ? 0xa : 0xd, halo::k_dword_none, 1);
    halo::game::game_engine_broadcast_kill_feed_by_relationship(player_index, 0x21, 0x23, 0x22, player_index, 1);
}

/**
 * Tests whether a given point lies within a given radius of a per-team position (interpreted here as the
 * team's flag location).
 *
 * Original register convention: stack -> radius, EAX -> team, ECX -> point.
 *
 * @address 0x468990
 */
uint8_t Ctf::point_within_team_flag_radius(float radius, int32_t team, real_point3d *point)
{
    real_point3d *flag_position;
    float dx, dy, dz, distance_squared, radius_squared;

    if (point == (real_point3d *)0) {
        return 0;
    }
    flag_position = ctf_team_flag_stand_position[team];
    if (flag_position == (real_point3d *)0) {
        return 0;
    }

    dx = flag_position->x - point->x;
    dy = flag_position->y - point->y;
    dz = flag_position->z - point->z;
    distance_squared = dz * dz + dy * dy + dx * dx;
    radius_squared = radius * radius;

    return (radius_squared > distance_squared) ? 1 : 0;
}

/**
 * Answers a scoreboard query keyed by a player index with capture-the-flag values.
 *
 * @address 0x469f60
 */
uint8_t Ctf::query_player_score(int32_t key, int32_t index, void *buffer)
{
    ::player *player = (::player *)halo::memory::datum_get(halo::game::players_get_active_by_index(index), player_data);

    if (player == 0 || key != 0x16) {
        return 0;
    }
    qr2_buffer_add_int(buffer, player->objective_score);
    return 1;
}

/**
 * Answers a scoreboard query keyed by a team index with capture-the-flag values.
 *
 * @address 0x469fe0
 */
uint8_t Ctf::query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, ctf_team_flag_touch_count[team]);
    return 1;
}

/**
 * Capture-the-flag reset-round callback: returns all flags and clears the round bookkeeping.
 *
 * @address 0x468820
 */
void Ctf::reset_round(void)
{
    halo::game::game_engine_queue_multiplayer_sound(0x16, halo::k_dword_none, 0);
}

/**
 * Resets a team's flag-return credit tracking and, if a flag object exists for that team, clears its carrier
 * state and updates its flag bits.
 *
 * Original register convention: EAX -> object_index.
 *
 * @address 0x468840
 */
void Ctf::reset_team_return_credit(uint32_t object_index)
{
    object *obj = halo::game::object_at(object_index);
    int16_t team = ((object *)obj)->owner_team;

    ctf_team_return_credit_active[team] = 0;
    ctf_team_return_credit_ticks[team] = 0;

    if (ctf_team_flag_stand_position[team] != (real_point3d *)0) {
        halo::game::ctf_flag_object_clear_carrier(object_index, ctf_team_flag_stand_position[team]);
        ((weapon_object *)obj)->weapon.flags &= ~(uint32_t)_weapon_game_object_taken_bit;
        obj->flags |= _object_changed_bit;
    }
}

/**
 * Respawns a team's objective (flag) object if that team is configured to have one, recording the new object's
 * handle.
 *
 * Original register convention: ESI -> team, EAX -> forwarded_position, stack -> forwarded_name_index.
 *
 * @address 0x468430
 */
void Ctf::respawn_team_flag(int32_t team, real_point3d *forwarded_position, uint16_t forwarded_name_index)
{
    if (ctf_team_flag_stand_position[team] != (real_point3d *)0) {
        datum_index new_flag = halo::game::game_engine_ctf_create_flag_object(forwarded_position, forwarded_name_index);
        if (new_flag != (datum_index)halo::k_dword_none) {
            ctf_team_flag_object[team] = new_flag;
        }
    }
}

/**
 * Checks whether a unit is currently a valid holder of its team's objective (flag) object by walking the
 * object/equipment reference chain.
 *
 * Original register convention: EAX -> player.
 *
 * @address 0x469780
 */
uint8_t Ctf::unit_is_flag_holder(player *p)
{
    datum_index flag_object;
    object *flag_obj;
    player *carrier;
    object *unit_obj;

    if (p == (player *)0) {
        return 0;
    }
    flag_object = ctf_team_flag_object[p->team];
    if (flag_object == (datum_index)halo::k_dword_none) {
        return 0;
    }
    flag_obj = halo::objects::object_try_and_get(flag_object, _object_mask_weapon);
    if (flag_obj == (object *)0) {
        return 0;
    }
    if (*(int32_t *)&((struct object *)flag_obj)->owner_linkage == -1) {
        return 0;
    }
    carrier = (player *)halo::memory::datum_get((datum_index)((struct object *)flag_obj)->owner_linkage, player_data);
    if (carrier == (player *)0) {
        return 0;
    }
    unit_obj = halo::objects::object_try_and_get(carrier->unit, _object_mask_unit);
    if (unit_obj == (object *)0 || *(int32_t *)&((struct object *)unit_obj)->parent_object == -1) {
        return 0;
    }
    return 1;
}

/**
 * Returns whether the weapon of the unit must stay readied (the unit carries a flag).
 *
 * Original register convention: ECX -> unit_handle.
 *
 * @address 0x466bc0
 */
uint8_t Ctf::unit_weapon_must_be_readied(datum_index unit_handle)
{
    object *unit_obj;
    Weapon *weapon_tag;

    if (current_game_engine == 0 || game_engine_variant.game_engine_index != _game_engine_ctf) {
        return 0;
    }

    unit_obj = halo::game::object_at(unit_handle);
    weapon_tag = (Weapon *)halo::game::tag_data_at(unit_obj->definition_tag);
    return (uint8_t)((weapon_tag->weapon_flags >> 3) & 1);
}

/**
 * Capture-the-flag engine definition slot +0x70: returns a float for a player and a position.
 *
 * @address 0x469ae0
 */
float Ctf::unknown_70(datum_index player_index, real_point3d *position)
{
    real_point3d *stand;
    float dx, dy, dz, distance_squared, weight;
    int32_t other_team;

    if (game_engine_variant.engine.ctf.assault == 0) {
        return 1.0f;
    }
    other_team = (halo::game::player_at(player_index)->team + 1) % 2;
    stand = ctf_team_flag_stand_position[other_team];
    dx = stand->x - position->x;
    dy = stand->y - position->y;
    dz = stand->z - position->z;
    distance_squared = dz * dz + dx * dx + dy * dy;
    if (distance_squared < 0.5f) {
        distance_squared = 0.5f;
    } else if (distance_squared > 10.0f) {
        distance_squared = 10.0f;
    }
    weight = 1.0f / distance_squared;
    if (game_time->game_time <= 0x1e) {
        return weight;
    }
    if (distance_squared > 1.0f) {
        weight = (float)halo::libm::pow((double)weight, (double)0.33f);
    }
    if (weight < 0.5f) {
        return 0.5f;
    }
    if (weight > 2.0f) {
        return 2.0f;
    }
    return weight;
}

/**
 * Capture-the-flag engine definition slot +0x84: true for kind 0.
 *
 * @address 0x4699e0
 */
uint8_t Ctf::unknown_84(int32_t kind)
{
    return (uint8_t)(kind == 0);
}

}
