/**
 * Kill attribution and the kill feed messages that are broadcast and displayed for it.
 */

#include "tags.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>
#include "cache.h"
#include <wchar.h>
#include <string.h>

#include "halo/game/game1_kill_feed.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/ai/api.hpp"

static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &game_engine_attribute_enabled = halo::link::ref<uint8_t>(halo::game::vars().game_engine_attribute_enabled);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &team_pair_data = halo::link::ref<team_pair_globals *>(halo::ai::vars().team_pair_data);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);

namespace halo::game::engine1 {

/**
 * Applies a received kill streak message to the local state.
 *
 * Original register convention: EAX -> envelope.
 *
 * @address 0x479b40
 */
uint8_t KillFeed::apply_kill_streak_message(int32_t **envelope)
{
    struct { int32_t machine_id; uint8_t unknown_04[4]; int16_t slot; int16_t amount; } decoded;

    if (**envelope != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)envelope);
        return 0;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)envelope, &decoded) == 0) {
        return 0;
    }

    {
        uint32_t player_handle = halo::k_dword_none;
        if (decoded.machine_id != 0) {
            player_handle = *(uint32_t *)(*(uint8_t **)&machine_table->handles + decoded.machine_id * 4);
        }
        halo::game::player_add_kill_streak(decoded.slot, decoded.amount, player_handle);
    }
    return 0;
}

/**
 * Determines kill/assist credit for a player's death by scanning the victim's recent-damager history and team
 * relationships before invoking game_engine_on_player_death with the resolved killer.
 *
 * Original register convention: EAX -> victim_unit, stack -> killer, death_object, killer_team, credit_kills.
 *
 * @address 0x46ff00
 */
void KillFeed::attribute_player_death(datum_index victim_unit, datum_index killer, datum_index death_object, int32_t killer_team, char credit_kills)
{
    datum_index victim;
    player *victim_player;
    int32_t victim_team;
    uint8_t friendly_or_no_killer;
    int32_t current_tick;
    int32_t assist_window_start;
    object *victim_object;
    unit_recent_damage *recent_damage;
    unit_recent_damage compacted[4];
    int32_t compacted_count;
    int32_t i;
    int16_t best_damage_index;
    int16_t killer_match_index;
    int16_t index;
    float best_damage;
    float assist_damage_threshold;
    uint32_t death_flags;

    if (!game_engine_attribute_enabled) {
        return;
    }
    victim = halo::game::player_index_from_unit_index(victim_unit);
    if (victim == k_datum_index_none) {
        return;
    }
    victim_player = halo::game::player_at(victim);
    victim_team = victim_player->team;
    if (killer == victim) {
        victim_player->suicides = victim_player->suicides + 1;
    }
    victim_player->deaths = victim_player->deaths + 1;
    victim_player->killing_spree_count = 0;
    victim_player->last_kill_tick = -1;
    victim_player->multikill_count = 0;

    if (killer == k_datum_index_none) {
        friendly_or_no_killer = 1;
    } else {
        friendly_or_no_killer = !halo::game::teams_are_enemies((int16_t)killer_team, (int16_t)victim_team);
    }

    current_tick = game_time->game_time;
    assist_window_start = current_tick - 0xb4;
    victim_object = *(object **)((uint8_t *)halo::objects::globals().object_data->data +
        (uint32_t)(uint16_t)victim_unit * halo::objects::globals().object_data->size + 8);
    recent_damage = (unit_recent_damage *)((uint8_t *)victim_object + 0x430);

    compacted_count = 0;
    for (i = 0; i < 4; i++) {
        compacted[i].tick = 0;
        compacted[i].damage = 0.0f;
        compacted[i].responsible_unit = k_datum_index_none;
        compacted[i].responsible_player = k_datum_index_none;
    }
    for (i = 0; i < 4; i++) {
        datum_index responsible = recent_damage[i].responsible_player;
        int16_t responsible_index = (int16_t)responsible;
        int16_t responsible_salt = (int16_t)((uint32_t)responsible >> 16);

        if (responsible != k_datum_index_none && responsible_index >= 0 &&
            responsible_index < player_data->maximum_count) {
            int16_t slot_identifier = *(int16_t *)((uint8_t *)player_data->data +
                (int32_t)player_data->size * (int32_t)responsible_index);

            if (slot_identifier != 0 && (responsible_salt == 0 || slot_identifier == responsible_salt)) {
                compacted[compacted_count] = recent_damage[i];
                compacted_count = compacted_count + 1;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        recent_damage[i] = compacted[i];
    }

    best_damage = -3.4028235e+38f;
    best_damage_index = -1;
    killer_match_index = -1;
    for (index = 0; index < 4; index++) {
        int16_t chosen = index;
        uint8_t take_as_primary = 0;

        if (recent_damage[index].responsible_player == killer) {
            take_as_primary = 1;
        } else if (killer_match_index == index) {
            chosen = killer_match_index;
            take_as_primary = 1;
        }

        if (take_as_primary) {
            killer_match_index = chosen;
            if (assist_window_start < recent_damage[index].tick && best_damage < recent_damage[index].damage) {
                best_damage = recent_damage[index].damage;
                best_damage_index = index;
            }
        } else if (friendly_or_no_killer) {
            int16_t entry_team = (int16_t)recent_damage[index].responsible_player;

            uint8_t is_enemy_team;

            if (current_game_engine == 0) {
                if (victim_team < 0 || victim_team > 9 || entry_team < 0 || entry_team > 9) {
                    killer_match_index = chosen;
                    if (assist_window_start < recent_damage[index].tick &&
                        best_damage < recent_damage[index].damage) {
                        best_damage = recent_damage[index].damage;
                        best_damage_index = index;
                    }
                    continue;
                }
                {
                    int32_t bit_index = entry_team + victim_team * 10;

                    is_enemy_team = (uint8_t)(1 - ((team_pair_data->enemy_bits[bit_index >> 5] &
                        (1u << (bit_index & 0x1f))) != 0));
                }
            } else {
                is_enemy_team = (int16_t)victim_team != entry_team;
            }
            if (!is_enemy_team) {
                killer_match_index = chosen;
                if (assist_window_start < recent_damage[index].tick &&
                    best_damage < recent_damage[index].damage) {
                    best_damage = recent_damage[index].damage;
                    best_damage_index = index;
                }
            }
        }
    }

    death_flags = (uint32_t)assist_window_start & 0xffffff00u;
    if ((best_damage_index == -1 && killer_match_index == -1) || credit_kills != 1) {
        assist_damage_threshold = 0.0f;
    } else {
        killer = recent_damage[best_damage_index].responsible_player;
        assist_damage_threshold = recent_damage[best_damage_index].damage * 0.4f;
    }

    if (killer != k_datum_index_none && credit_kills == 1) {
        player *killer_player = halo::game::player_at(killer);
        int16_t killer_player_team = killer_player->team;
        uint8_t is_friendly;

        if (current_game_engine == 0 && victim_team >= 0 && victim_team < 10 &&
            killer_player_team >= 0 && killer_player_team < 10) {
            int32_t bit_index = killer_player_team + victim_team * 10;

            is_friendly = (uint8_t)((team_pair_data->enemy_bits[bit_index >> 5] &
                (1u << (bit_index & 0x1f))) != 0);
        } else if (current_game_engine != 0) {
            is_friendly = (int16_t)victim_team == killer_player_team;
        } else {
            is_friendly = 0;
        }

        if (is_friendly) {
            killer_player->betrayals = killer_player->betrayals + 1;
            death_flags = ((uint32_t)assist_window_start & 0xffffff00u) | 1u;
            if (killer != victim) {
                killer_player->betrayal_penalty_count = killer_player->betrayal_penalty_count + 1;
                halo::game::player_advance_multikill_medal(killer);
            }
        } else {
            killer_player->kills = killer_player->kills + 1;
            killer_player->killing_spree_count = killer_player->killing_spree_count + 1;
            if (killer_player->last_kill_tick < current_tick - 0x78) {
                killer_player->multikill_count = 1;
            } else {
                killer_player->multikill_count = killer_player->multikill_count + 1;
            }
            killer_player->last_kill_tick = (int16_t)game_time->game_time;
        }
    }

    if (assist_damage_threshold > 0.0f && credit_kills == 1) {
        for (index = 0; index < 4; index++) {
            datum_index assist_candidate = recent_damage[index].responsible_player;

            if ((index == killer_match_index || recent_damage[index].damage < assist_damage_threshold) &&
                assist_candidate != k_datum_index_none && assist_candidate != killer) {
                player *candidate_player = halo::game::player_at(assist_candidate);
                int16_t candidate_team = candidate_player->team;
                uint8_t is_friendly;

                if (current_game_engine == 0 && victim_team >= 0 && victim_team < 10 &&
                    candidate_team >= 0 && candidate_team < 10) {
                    int32_t bit_index = candidate_team + victim_team * 10;

                    is_friendly = (uint8_t)((team_pair_data->enemy_bits[bit_index >> 5] &
                        (1u << (bit_index & 0x1f))) != 0);
                } else if (current_game_engine != 0) {
                    is_friendly = (int16_t)victim_team == candidate_team;
                } else {
                    is_friendly = 1;
                }

                if (!is_friendly) {
                    candidate_player->assists = candidate_player->assists + 1;
                }
            }
        }
    }

    halo::game::game_engine_on_player_death(killer, death_object, victim, (char)(death_flags & 0xff));
}

/**
 * Broadcasts one of several kill-feed message variants to each recipient depending on their team relationship
 * to the source player.
 *
 * Original register convention: stack -> source_player, no_source_message, message_a, message_b, subject.
 *
 * @address 0x460c10
 */
void KillFeed::broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        player *p = (player *)element;
        int32_t message = no_source_message;

        if (source_player != (uint32_t)halo::k_dword_none) {
            int16_t their_team = p->team;
            int16_t source_team = (halo::game::player_at(source_player))->team;

            message = message_b;

            if (current_game_engine == 0) {
                if (0 <= source_team && source_team < 10 && 0 <= their_team && their_team < 10) {
                    int32_t pair = their_team + source_team * 10;
                    char enemies = (team_pair_data->enemy_bits[pair >> 5] >>
                        (pair & 0x1f) & 1) != 0;
                    if (enemies) {
                        message = message_a;
                    }
                }

            } else if (their_team == source_team) {
                message = message_a;
            }
        }

        if (message != -1) {
            halo::game::chimera__kill_feed(iter.index, (int32_t)iter.index, (uint32_t)message, subject, (char)broadcast);
        }
        element = halo::memory::data_iterator_next(&iter);
    }
}

/**
 * Conditionally broadcasts a kill-feed message id to every entry in the current data iteration.
 *
 * Original register convention: ESI -> broadcast_enabled, EBX -> broadcast, stack -> exclude_index,.
 *
 * @address 0x460db0
 */
void KillFeed::broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t exclude_index, int32_t alternate_recipient, datum_index subject, char broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        if ((int32_t)iter.index != exclude_index) {
            int32_t forwarded_param_1 = (alternate_recipient == -1) ? (int32_t)iter.index : alternate_recipient;
            if (broadcast_enabled != -1) {
                halo::game::chimera__kill_feed(iter.index, forwarded_param_1, broadcast_enabled, subject, broadcast);
            }
        }
        element = halo::memory::data_iterator_next(&iter);
    }
}

/**
 * Broadcasts a single kill-feed message id to every entry in the current data iteration.
 *
 * Original register convention: EAX -> recipient_or_all, ESI -> broadcast_enabled, EBX -> broadcast, stack ->
 * param_1,.
 *
 * @address 0x460d10
 */
void KillFeed::broadcast_kill_feed_or_direct(datum_index recipient_or_all, int32_t broadcast_enabled, char broadcast, int32_t hash_key, datum_index subject)
{
    int32_t forwarded_param_1 = (hash_key == -1) ? -1 : hash_key;

    if (recipient_or_all == (datum_index)halo::k_dword_none) {
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            if (broadcast_enabled != -1) {
                halo::game::chimera__kill_feed(iter.index, forwarded_param_1, broadcast_enabled, subject,
                    broadcast);
            }
            element = halo::memory::data_iterator_next(&iter);
        }
    } else if (broadcast_enabled != -1) {
        halo::game::chimera__kill_feed(recipient_or_all, hash_key, broadcast_enabled, subject, broadcast);
    }
}

/**
 * Broadcasts a kill-feed message to every entry in the current data iteration that belongs to a given
 * group/team id.
 *
 * Original register convention: ESI -> message_type, BL -> broadcast, stack -> team.
 *
 * @address 0x460ba0
 */
void KillFeed::broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        player *p = (player *)element;

        if (p->team == team && message_type != -1) {
            halo::game::chimera__kill_feed(iter.index, (int32_t)iter.index, (uint32_t)message_type, halo::k_dword_none, (char)broadcast);
        }
        element = halo::memory::data_iterator_next(&iter);
    }
}

/**
 * Formats the localized text for a specific kill-feed/HUD message type (kill, suicide, betrayal, etc.) into a
 * caller-supplied buffer.
 *
 * Original register convention: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size.
 *
 * @address 0x45e680
 */
uint8_t KillFeed::build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type, datum_index subject, size_t buffer_size)
{
    uint32_t adjusted_type = message_type;
    uint8_t ok = 1;

    if (current_game_engine != 0 && current_game_engine->unknown_84 != 0 &&
        ((char (*)(int32_t))current_game_engine->unknown_84)(1) != 0) {
        switch (message_type) {
            case 7: adjusted_type = 0x10; break;
            case 8: adjusted_type = 0x13; break;
            case 9: adjusted_type = 0x0f; break;
            case 10: adjusted_type = 0x0e; break;
            case 11: adjusted_type = 0x12; break;
            case 12: adjusted_type = 0x11; break;
            default: break;
        }
    }

    if (adjusted_type < 0x20) {
        switch (adjusted_type) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x06: case 0x0d: case 0x1c: {
            void *element = halo::memory::datum_get(subject, player_data);
            if (element == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject);
            }
            break;
        }
        case 0x04: case 0x05: {
            void *a = halo::memory::datum_get(subject, player_data);
            void *b = halo::memory::datum_get(subject, player_data);
            if (a == 0 || b == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject, subject);
            }
            break;
        }
        case 0x07: case 0x09: case 0x0a: case 0x0b: case 0x0c: {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
            wchar_t *text = (tag_id == k_datum_index_none) ? (wchar_t *)L""
                : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
            wcsncpy(out, text, buffer_size);

            break;
        }
        case 0x08: {
            void *element = halo::memory::datum_get(subject, player_data);
            if (element == 0) {
                ok = 0;
                break;
            }
            {
                datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                if (tag_id == k_datum_index_none) {
                    halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(L"%d"), subject);
                } else {
                    wchar_t *fmt = reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                    halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject);
                }
            }
            break;
        }
        case 0x0e: case 0x0f: case 0x10: case 0x11: case 0x12: {
            void *element = halo::memory::datum_get(subject, player_data);
            if (element == 0) {
                ok = 0;
                break;
            }
            if (current_game_engine != 0 && current_game_engine->get_score != 0) {
                ((int32_t (*)(datum_index))current_game_engine->get_score)(subject);
            }
            {
                datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject);
            }

            {
                static const uint8_t arm_sound[5] = { 0x10, 0x0f, 0x0e, 0x11, 0x12 };

                halo::game::game_engine_queue_multiplayer_sound(arm_sound[adjusted_type - 0x0e], halo::k_dword_none, 0);
            }
            break;
        }
        case 0x13: {
            void *a = halo::memory::datum_get(subject, player_data);
            void *b = halo::memory::datum_get(subject, player_data);
            if (a == 0 || b == 0) {
                ok = 0;
                break;
            }
            if (current_game_engine != 0 && current_game_engine->get_score != 0) {
                ((int32_t (*)(datum_index))current_game_engine->get_score)(subject);
            }
            {
                datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                    : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject, subject);
            }
            break;
        }
        case 0x17: case 0x18: case 0x1a: case 0x1b: {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
            if (tag_id != k_datum_index_none) {
                wchar_t *text = reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                wcsncpy(out, text, buffer_size);
            } else {
                wcsncpy(out, L"", buffer_size);
            }
            break;
        }
        case 0x19: {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
            if (tag_id == k_datum_index_none) {
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(L"%d"), subject);
            } else {
                wchar_t *fmt = reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject);
            }
            break;
        }
        case 0x1d: {
            uint8_t scratch[140];
            uint16_t binding_name[0x40];
            if (!halo::input::Bindings::get_last_used_binding((int16_t)subject, (control_binding_descriptor *)scratch)) {
                out[0] = 0;
            } else {
                halo::input::BindingNames::get_binding_display_name((control_binding_descriptor *)scratch, binding_name);
                {
                    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
                    wchar_t *fmt = (tag_id == k_datum_index_none) ? &empty_string
                        : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                    halo::text::string_format_wide_va_bounded(buffer_size, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), subject);
                }
            }
            break;
        }
        case 0x1e: {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
            wchar_t *prefix = (tag_id == k_datum_index_none) ? (wchar_t *)L""
                : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
            int32_t formatted_len;

            halo::game::game_time_format_minutes_seconds((uint32_t)subject, buffer_size, out);
            formatted_len = (int32_t)wcslen(out);
            wcsncat(out, prefix, buffer_size - formatted_len);
            break;
        }
        case 0x1f: {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
            if (tag_id != k_datum_index_none) {
                wchar_t *text = reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)adjusted_type));
                wcsncpy(out, text, buffer_size);
            } else {
                wcsncpy(out, L"", buffer_size);
            }
            break;
        }
        default:

            ok = 0;
            break;
        }
    } else {
        ok = 0;
    }

    out[buffer_size - 1] = 0;
    return ok;
}

/**
 * Handles an incoming or locally-generated kill-feed network event by resolving the killer and forwarding it
 * to the kill-feed message system.
 *
 * Original register convention: EAX -> message.
 *
 * @address 0x4609d0
 */
void KillFeed::handle_kill_feed_network_event(int32_t **message)
{
    int32_t decoded[3];

    if (**message == 0) {
        if (halo::networking::message_delta_decode_compound_field((void **)message, decoded) != 0) {
            datum_index killer = (datum_index)halo::k_dword_none;
            if (decoded[0] != 0) {
                killer = *(datum_index *)(*(uint8_t **)&machine_table->handles + decoded[0] * 4);
            }
            halo::game::chimera__kill_feed(local_player_globals->local_players[0], (int32_t)killer,
                                (uint32_t)decoded[1], (datum_index)decoded[2], 0);
            return;
        }
    } else {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
    }
}

}
