/**
 * Scoreboard entries, ranking, winner queries and end-of-game result text shared by all game engines.
 */

#include "tags.h"
#include "halo/core/tag_groups.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <stdint.h>
#include "objects.h"
#include "units.h"
#include "networking.h"

#include "halo/game/game1_scoreboard.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern data_array *player_data;
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern game_variant game_engine_variant;
extern wchar_t empty_string;
extern wchar_t missing_string_text[];
extern void qsort(void *base, uint32_t count, uint32_t size,
    uint32_t (*compare)(const void *, const void *));
extern int32_t game_engine_bucket_scores[16];
extern int32_t game_engine_bucket_scores_extra[16];
extern float game_engine_end_game_timer;
extern uint8_t multiplayer_sound_enabled[];
extern Globals *global_globals;
extern int32_t multiplayer_sound_queue_count;
extern multiplayer_sound_request multiplayer_sound_queue[k_maximum_queued_multiplayer_sounds];
extern uint8_t shared_hud_text_draw_state;
extern void *ui_root_widget;
extern void *ui_widget_history;
extern uint8_t ui_pause_depth;
extern uint8_t controls_input_capture_buffer[0xa0 * 4];
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern int32_t players_active_count(void);
extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_handle);
extern real *default_color_a;
extern real *default_color_b;
extern int32_t game_engine_unknown_aa00;
extern int16_t light_count_enabled;
extern float game_engine_nameplate_fade_opacity_array[1];
}

namespace halo::game::engine1 {

/**
 * File-local helper of Scoreboard: multiplayer game text string.
 */
wchar_t *Scoreboard::multiplayer_game_text_string(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\multiplayer_game_text");

    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, index));
}

/**
 * Builds the localized end-of-game result text (e.g. who is leading or tied) by comparing player or team
 * scores through the active game engine's callbacks.
 *
 * Original register convention: none; param_1 (player) and param_2 (out, wchar_t[0x50]) are stack args.
 *
 * @address 0x45cf30
 */
void Scoreboard::build_end_game_result_text(datum_index player_handle, wchar_t *out)
{
    wchar_t *lives_text = &empty_string;
    wchar_t lives_buffer[0x80];

    if (0 < game_engine_variant.lives_per_round) {
        player *p = halo::game::player_at(player_handle);
        int32_t lives_left = game_engine_variant.lives_per_round - (int32_t)(int16_t)p->deaths;

        if (lives_left == 0) {
            lives_text = multiplayer_game_text_string(0x34);
        } else if (lives_left == 1) {
            lives_text = multiplayer_game_text_string(0x35);
        } else {
            halo::text::string_format_wide_va_bounded(0x80, reinterpret_cast<uint16_t *>(lives_buffer), reinterpret_cast<const uint16_t *>(multiplayer_game_text_string(0x36)), lives_left);
            lives_buffer[0x7f] = 0;
            lives_text = lives_buffer;
        }
    }

    if (game_engine_state_value == _game_engine_state_ending) {
        int32_t result = 0;
        uint8_t teams = 0;

        if (current_game_engine != 0) {
            if (current_game_engine->is_winner == 0) {
                result = (int32_t)halo::game::game_engine_is_object_winning(player_handle);
            } else {
                result = ((int32_t (*)(datum_index))current_game_engine->is_winner)(player_handle);
            }
            teams = (uint8_t)game_engine_variant.teams;
        }

        if (result == -1) {
            datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\multiplayer_game_text");
            wchar_t *text = &empty_string;

            if (tag_id != k_datum_index_none) {
                int32_t *tag_data = (int32_t *)halo::cache::globals().tag_instances[tag_id & halo::k_datum_slot_mask].data;
                text = missing_string_text;
                if (0x37 < *tag_data) {
                    uint8_t *entry = (uint8_t *)tag_data[1];
                    uint32_t len = *(uint32_t *)(entry + 0x44c);
                    if (0 < (int32_t)len) {
                        wchar_t *string_data = *(wchar_t **)(entry + 0x458);
                        *(uint16_t *)((uint8_t *)string_data + ((len & 0xfffffffe) - 2)) = 0;
                        wcsncpy(out, string_data, 0x50);
                        goto done;
                    }
                }
            }
            wcsncpy(out, text, 0x50);
        } else if (result == 0) {
            wcsncpy(out, multiplayer_game_text_string(teams ? 0x38 : 0x39), 0x50);
        } else if (result == 1) {
            wcsncpy(out, multiplayer_game_text_string(teams ? 0x3a : 0x3b), 0x50);
        }
    } else if (current_game_engine == 0 || game_engine_variant.teams == 0) {
        scoreboard_entry entry;
        wchar_t header[0x80];
        wchar_t *fmt;

        halo::game::game_engine_get_player_scoreboard_entry(player_handle, &entry);
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(player_handle, header);

        fmt = multiplayer_game_text_string((entry.place & 0x80000000) != 0 ? 0x3f : 0x40);
        halo::text::string_format_wide_va_bounded(0x50, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(fmt), halo::game::game_engine_get_default_multiplayer_string(&entry), header, lives_text);
    } else {
        wchar_t team0_text[14];
        wchar_t team1_text[14];
        int32_t score0, score1;

        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(0, team0_text);
        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(1, team1_text);
        score0 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        score1 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(1);

        if (score0 > score1) {
            halo::text::string_format_wide_va_bounded(0x50, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(multiplayer_game_text_string(0x3c)), team0_text, team1_text, lives_text);
        } else if (score0 < score1) {
            halo::text::string_format_wide_va_bounded(0x50, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(multiplayer_game_text_string(0x3d)), team1_text, team0_text, lives_text);
        } else {
            halo::text::string_format_wide_va_bounded(0x50, reinterpret_cast<uint16_t *>(out), reinterpret_cast<const uint16_t *>(multiplayer_game_text_string(0x3e)), team1_text, lives_text);
        }
    }

done:
    out[0x4f] = 0;
}

/**
 * Builds a HUD/kill-feed message's text, preferring a game-variant-supplied override before falling back to
 * the default builder.
 *
 * Original register convention: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size.
 *
 * @address 0x460890
 */
uint8_t Scoreboard::build_message_text(wchar_t *out, uint32_t buffer_size, datum_index subject, uint32_t param_1, uint32_t message_type)
{
    char handled = 0;

    if (current_game_engine->build_message_text != 0) {
        handled = ((char (*)(uint32_t, uint32_t, datum_index, wchar_t *, uint32_t))current_game_engine->build_message_text)
            (param_1, message_type, subject, out, buffer_size);
    }
    if (handled == 0) {
        return halo::game::game_engine_build_kill_feed_message_text(param_1, out, message_type, subject, buffer_size);
    }
    return (uint8_t)handled;
}

/**
 * Builds the sort key of a player scoreboard entry from its score.
 *
 * Original register convention: ECX -> player_index, EDX -> score.
 *
 * @address 0x45cc30
 */
uint32_t Scoreboard::build_scoreboard_sort_key(uint32_t player_index, int32_t score)
{
    player *p;
    uint32_t flags;

    flags = 0;
    p = halo::game::player_at(player_index);
    if (score < -1000) {
        score = -1000;
    }
    if (p->deaths < game_engine_variant.lives_per_round) {
        flags = 0x40000000;
    }
    if (p->marked_for_deletion == 0) {
        flags = flags | 0x20000000;
    }
    return flags | (uint32_t)(score + 1000);
}

/**
 * Compares dwords 2..5 of adjacent entries... place... bit 0x80000000 set on a tie.
 *
 * Original register convention: AL -> invert_low_stat, stack -> (out_entries, mode).
 *
 * @address 0x45cc90
 */
int32_t Scoreboard::build_sorted_player_list(uint8_t invert_low_stat, scoreboard_entry out_entries[16], int32_t mode)
{
    data_iterator iterator;
    player *p;
    int32_t count;
    scoreboard_entry *entry;
    uint8_t negate;
    int32_t i;

    negate = (mode == 4) ? (invert_low_stat == 0) : invert_low_stat;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    count = 0;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    entry = out_entries;
    if (p != (player *)0) {
        while (p != (player *)0) {
            if (count < 16) {
                entry->player = iterator.index;
                count = count + 1;
                entry = entry + 1;
            }
            p = (player *)halo::memory::data_iterator_next(&iterator);
        }
    }

    for (i = 0; i < count; i = i + 1) {
        entry = &out_entries[i];
        p = (player *)((uint8_t *)player_data->data + ((uint32_t)entry->player & halo::k_datum_slot_mask) * sizeof(player));

        switch (mode) {
        case 0: {
            int32_t score = 0;
            entry->key_0 = 0;
            if (current_game_engine->get_score != (void *)0) {
                score = ((int32_t (*)(datum_index, int32_t))current_game_engine->get_score)(
                    entry->player, 0);
                entry->key_0 = (int32_t)halo::game::game_engine_build_scoreboard_sort_key(entry->player, score);
            }
            entry->key_1 = p->kills;
            entry->key_2 = p->deaths;
            entry->key_3 = p->assists;
            break;
        }
        case 1: {
            entry->single_sort_key = 0;
            if (current_game_engine->get_score != (void *)0) {
                int32_t score = ((int32_t (*)(datum_index, int32_t))current_game_engine->get_score)(
                    entry->player, 0);
                entry->single_sort_key = (int32_t)halo::game::game_engine_build_scoreboard_sort_key(entry->player, score);
            }
            break;
        }
        case 2:
            entry->single_sort_key = p->kills;
            break;
        case 3:
            entry->single_sort_key = p->assists;
            break;
        case 4:
            entry->single_sort_key = p->deaths;
            break;
        default:
            break;
        }

        if (negate) {
            entry->single_sort_key = -entry->single_sort_key;
        }
    }

    qsort(out_entries, (uint32_t)count, sizeof(scoreboard_entry),
        (mode == 0) ? (uint32_t (*)(const void *, const void *))(void *)halo::game::scoreboard_entry_compare
                    : (uint32_t (*)(const void *, const void *))(void *)halo::game::scoreboard_entry_compare_by_unknown_04);

    for (i = 0; i < count; i = i + 1) {
        entry = &out_entries[i];
        if (i == 0) {
            entry->place = i;
        } else {
            scoreboard_entry *prev = &out_entries[i - 1];
            if (prev->key_0 != entry->key_0 || prev->key_1 != entry->key_1 ||
                prev->key_2 != entry->key_2 || prev->key_3 != entry->key_3) {
                entry->place = i;
            } else {
                int32_t tied_place = prev->place | (int32_t)0x80000000u;
                prev->place = tied_place;
                entry->place = tied_place;
            }
        }
    }

    return count;
}

/**
 * Aggregates per-bucket (team/hill/flag) score values each tick and, once any bucket reaches its target,
 * declares a winner, arms the end-of-round countdown, plays the win announcement, and broadcasts a UI-
 * close/chat-reset message.
 *
 * @address 0x46db70
 */
void Scoreboard::check_bucket_scores_and_end_round(void)
{
    int32_t bucket;

    for (bucket = 0; bucket < 16; bucket++) {
        data_iterator iter;
        player *p;
        int32_t count = 0;
        int32_t aggregate = 0;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        p = (player *)halo::memory::data_iterator_next(&iter);
        while (p != (player *)0) {
            if (p->team == bucket && p->marked_for_deletion == 0) {
                int32_t value = *(int16_t *)((uint8_t *)p + 0xc6);
                if (game_engine_variant.engine.race.team_scoring == 0) {
                    if (count == 0 || value < aggregate) {
                        aggregate = value;
                    }
                } else if (game_engine_variant.engine.race.team_scoring == 1) {
                    if (count == 0 || aggregate <= value) {
                        aggregate = value;
                    }
                } else {
                    aggregate += value;
                }
                count++;
            }
            p = (player *)halo::memory::data_iterator_next(&iter);
        }

        game_engine_bucket_scores[bucket] = aggregate;
        if (game_engine_variant.engine.race.team_scoring == 2) {
            game_engine_bucket_scores[bucket] = game_engine_bucket_scores_extra[bucket] + aggregate;
        }
    }

    halo::game::game_engine_player_profile_cache_sync_all(1, (void *)halo::k_dword_none);

    for (bucket = 0; bucket < 16; bucket++) {
        if (game_engine_bucket_scores[bucket] >= game_engine_variant.score_limit &&
            halo::networking::globals().game_mode == 2 && game_engine_state_value == 0) {
            halo::networking::globals().server->game_over = 1;
            game_engine_state_value = _game_engine_state_ending;
            game_engine_end_game_timer = 7.0f;

            if (multiplayer_sound_enabled[1] == 0) {
                GlobalsMultiplayerInformation *mp_info =
                    (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
                if (mp_info != (GlobalsMultiplayerInformation *)0 && (int32_t)mp_info->sounds.count > 1) {
                    uint8_t *sound1 = (uint8_t *)mp_info->sounds.pointer + 0x10;
                    if ((int32_t)mp_info->sounds.pointer != -0x10 && *(int32_t *)(sound1 + 0xc) != -1) {
                        halo::sound::sound_start_unspatialized(*(datum_index *)(sound1 + 0xc), 1.0f);
                    }
                }
            } else {
                int32_t duration = halo::game::game_engine_get_multiplayer_sound_duration_ticks(1);
                if (multiplayer_sound_queue_count < k_maximum_queued_multiplayer_sounds) {
                    multiplayer_sound_request *slot = &multiplayer_sound_queue[multiplayer_sound_queue_count];
                    slot->player = (datum_index)halo::k_dword_none;
                    slot->sound_index = 1;
                    multiplayer_sound_queue_count++;
                    slot->remaining_ticks = duration + 5;
                    slot->broadcast = 0;
                }
                if (multiplayer_sound_queue_count == 1) {
                    GlobalsMultiplayerInformation *mp_info =
                        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
                    if (mp_info != (GlobalsMultiplayerInformation *)0 && (int32_t)mp_info->sounds.count > 1) {
                        uint8_t *sound1 = (uint8_t *)mp_info->sounds.pointer + 0x10;
                        if ((int32_t)mp_info->sounds.pointer != -0x10 && *(int32_t *)(sound1 + 0xc) != -1) {
                            halo::sound::sound_start_unspatialized(*(datum_index *)(sound1 + 0xc), 1.0f);
                        }
                    }
                }
            }

            if (ui_root_widget != (void *)0) {
                halo::interface::widget_close((widget_instance *)ui_root_widget);
            }
            if (ui_widget_history != (void *)0) {
                halo::interface::widget_pool_list_free_all((widget_history_node **)&ui_widget_history);
            }
            ui_pause_depth = 0;
            if (halo::interface::globals().controls_capture_row != -1) {
                int32_t i;
                halo::interface::globals().controls_input_capture_flags &= 0xf7;
                for (i = 0; i < 0xa0; i++) {
                    ((uint32_t *)controls_input_capture_buffer)[i] = 0;
                }
                halo::interface::globals().controls_capture_row = -1;
            }

            {
                uint8_t payload = 1;
                uint8_t *payload_ptr = &payload;
                int32_t encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::end_game), 0, (void **)&payload_ptr, 0, 1, 0);
                if (encoded_bits > 0) {
                    halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
                }
            }
        }
    }
}

/**
 * Compares one object score against every other tracked object (via the game engine score callback) to compute
 * whether it is winning, tied, and how many objects tie with it.
 *
 * Original register convention: stack -> subject, team_mode.
 *
 * @address 0x463480
 */
uint32_t Scoreboard::compare_score_to_others(uint32_t subject, int32_t team_mode)
{
    uint8_t all_tied = 1;
    uint8_t any_tied = 0;
    int32_t compared_count = 1;
    uint16_t higher_count = 0;
    uint16_t flags;
    uint32_t seen_teams = 0;

    if (current_game_engine->get_score != 0) {
        int32_t subject_score = ((int32_t (*)(uint32_t, int32_t))current_game_engine->get_score)(subject, team_mode);
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)halo::k_dword_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            player *entry = (player *)element;
            uint8_t skip;

            if (team_mode == 1) {
                player *subject_player = halo::game::player_at(subject);
                skip = (entry->team == subject_player->team);
            } else {
                skip = (subject == halo::k_dword_none);
            }

            if (!skip) {
                if (team_mode == 1) {
                    uint32_t team_bit = 1u << (entry->team & 0x1f);
                    if ((team_bit & seen_teams) != 0) {
                        goto next;
                    }
                    seen_teams = seen_teams | team_bit;
                }
                {
                    int32_t other_score = ((int32_t (*)(uint32_t, int32_t))current_game_engine->get_score)
                        (halo::k_dword_none, team_mode);
                    compared_count = compared_count + 1;
                    if (other_score == subject_score) {
                        any_tied = 1;
                    } else {
                        all_tied = 0;
                        if (subject_score < other_score) {
                            higher_count = higher_count + 1;
                        }
                    }
                }
            }
        next:
            element = halo::memory::data_iterator_next(&iter);
        }
    }

    flags = (team_mode != 1) ? 0 : 0xffff;
    if (any_tied) {
        flags = (flags & 8) | 1;
    } else {
        flags = flags & 8;
    }
    if (all_tied && any_tied) {
        flags = flags | 2;
    }
    if (compared_count == 2) {
        flags = flags | 4;
    }
    return ((uint32_t)higher_count << 16) | flags;
}

/**
 * Returns whether a team has at least one eligible player.
 *
 * @address 0x45c9e0
 */
uint8_t Scoreboard::find_first_eligible_player_on_team(int32_t team)
{
    data_iterator iterator;
    player *p;
    datum_index reread_unit;
    int16_t reread_deaths;
    uint8_t skip;

    if (halo::game::players_active_count() < 2) {
        return 1;
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    if (p == (player *)0) {
        return 0;
    }

    do {
        if (p->marked_for_deletion != 0) {
            skip = 1;
        } else if (p->unit == k_datum_index_none) {
            skip = (halo::game::game_engine_player_has_respawn_priority(iterator.index) != 0) ||
                   (0 < game_engine_variant.lives_per_round &&
                    (reread_unit = p->unit,
                     reread_unit == k_datum_index_none) &&
                    (reread_deaths = p->deaths,
                     game_engine_variant.lives_per_round <= reread_deaths));
        } else {
            skip = 0;
        }

        if (!skip) {
            break;
        }

        p = (player *)halo::memory::data_iterator_next(&iterator);
        if (p == (player *)0) {
            return 0;
        }
    } while (1);

    return p->team == team;
}

/**
 * Server-side helper that scans all players comparing names, likely for a console/RCON player-lookup command.
 *
 * Original register convention: EBX -> source_name.
 *
 * @address 0x473430
 */
void Scoreboard::find_player_by_name(char *source_name)
{
    if (halo::networking::globals().game_mode == 2) {
        wchar_t name[1024];
        data_iterator iter;
        void *element;

        halo::text::string_convert_ascii_to_unicode((uint16_t *)name, 0x800, source_name);

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = halo::memory::data_iterator_next(&iter);
        while (element != 0) {
            wcscmp((wchar_t *)((uint8_t *)element + 4), name);
            element = halo::memory::data_iterator_next(&iter);
        }
    }
}

/**
 * Returns the player that currently holds the target object.
 *
 * Original register convention: EBX -> target_object.
 *
 * @address 0x468b50
 */
datum_index Scoreboard::find_player_holding_object(datum_index target_object)
{
    data_iterator iter;
    player *p;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)halo::memory::data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->unit != (datum_index)halo::k_dword_none) {
            object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint32_t)p->unit & halo::k_datum_slot_mask].data;
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            int32_t i;
            for (i = 0; i < k_maximum_weapons_per_unit; i++) {
                if (unit->weapons[i] == target_object) {
                    return iter.index;
                }
            }
        }
        p = (player *)halo::memory::data_iterator_next(&iter);
    }
    return (datum_index)halo::k_dword_none;
}

/**
 * Returns the default multiplayer string of a scoreboard entry.
 *
 * Original register convention: ECX -> tag_id, DX -> index.
 *
 * @address 0x45ce90
 */
wchar_t *Scoreboard::get_default_multiplayer_string(const scoreboard_entry *entry)
{
    datum_index tag_id;
    int32_t place = entry->place & 0x7f;
    int32_t index = (place > 0xf) ? 0xf : place;

    tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\multiplayer_game_text");
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)(index + 0x24)));
}

/**
 * Returns the string-list data for the ui\multiplayer_game_text tag (or a built-in fallback list) used for
 * multiplayer HUD/game text strings.
 *
 * Original register convention: stack -> rank.
 *
 * @address 0x4633f0
 */
wchar_t *Scoreboard::get_multiplayer_text_list(uint32_t rank)
{
    int16_t place = (int16_t)(rank >> 16);
    int32_t index;
    datum_index tag_id;

    if ((rank & 4) != 0 && (rank & 1) != 0) {
        index = 0x23;
    } else if ((rank & 4) != 0 && place == 0) {
        index = 0x21;
    } else if ((rank & 4) != 0 && place == 1) {
        index = 0x22;
    } else if ((rank & 2) != 0) {
        index = 0x20;
    } else {
        index = place;
        if ((rank & 1) != 0) {
            index += 0x10;
        }
    }
    tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\multiplayer_game_text");
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, (int16_t)(index + 0x66)));
}

/**
 * Returns a 3-float RGB color for a player: either a resolved custom palette color or one of two fixed default
 * colors.
 *
 * Original register convention: EAX -> out_rgb, ECX -> color_index. Returns a pointer to three floats.
 *
 * @address 0x463290
 */
real *Scoreboard::get_player_color(uint32_t player_index, real *out_rgb)
{
    player *p = halo::game::player_at(player_index);
    real scratch[3];
    real *color;

    if (game_engine_variant.teams == 0) {
        color = halo::saved_games::player_color_get_rgb(scratch, (int32_t)*(int16_t *)&p->color_index);
    } else if (p->team == 0) {
        color = default_color_a;
    } else {
        color = default_color_b;
    }
    out_rgb[0] = color[0];
    out_rgb[1] = color[1];
    out_rgb[2] = color[2];
    return out_rgb;
}

/**
 * Looks up and copies out a single player's scoreboard entry from the sorted scoreboard list.
 *
 * Original register convention: EAX -> player, EBX -> out.
 *
 * @address 0x45cee0
 */
void Scoreboard::get_player_scoreboard_entry(datum_index player, scoreboard_entry *out)
{
    scoreboard_entry entries[16];
    int i;

    halo::game::game_engine_build_sorted_player_list(0, entries, 0);

    i = 0;
    if (entries[0].player != player) {
        do {
            i = i + 1;
        } while (entries[i].player != player);
    }

    *out = entries[i];
}

/**
 * Computes a player's rank/placement number within the sorted scoreboard list, accounting for tied entries.
 *
 * Original register convention: EDI -> player, EAX -> mode, stack -> invert_low_stat.
 *
 * @address 0x45d440
 */
int32_t Scoreboard::get_scoreboard_place(datum_index player, int32_t mode, uint8_t invert_low_stat)
{
    scoreboard_entry entries[16];
    int count;
    int place;
    int i;

    count = halo::game::game_engine_build_sorted_player_list(invert_low_stat, entries, mode);
    place = 0;

    if (entries[0].player != player && 1 < count) {
        for (i = 1; i < count; i++) {
            if (entries[i - 1].single_sort_key != entries[i].single_sort_key) {
                place = place + 1;
            }
            if (entries[i].player == player) {
                return place;
            }
        }
    }

    return place;
}

/**
 * Determines whether a given object belongs to the currently leading/winning team.
 *
 * Original register convention: EAX -> handle.
 *
 * @address 0x463660
 */
uint32_t Scoreboard::is_object_winning(uint32_t handle)
{
    if (game_engine_variant.teams == 0) {
        scoreboard_entry entry;
        halo::game::game_engine_get_player_scoreboard_entry((datum_index)handle, &entry);
        if ((entry.place & 0x80000000) == 0 || (entry.place & 0x7fffffff) != 0) {
            return (entry.place & 0x7fffffff) == 0;
        }
    } else {
        int32_t team0_score = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        int32_t team1_score = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(1);
        uint8_t winning_team;

        if (halo::game::game_engine_players_ready_for_bsp_switch() == 0) {
            winning_team = (halo::game::game_engine_find_first_eligible_player_on_team(0) == 0);
        } else {
            if (team0_score == team1_score) {
                return halo::k_dword_none;
            }
            winning_team = team0_score <= team1_score;
        }
        if (winning_team != halo::k_dword_none) {
            player *p = halo::game::player_at(handle);
            return (uint32_t)(p->team == (int32_t)winning_team);
        }
    }
    return halo::k_dword_none;
}

/**
 * Returns whether a given tracked object should be treated as this round's winner, deferring to the active
 * variant's custom win-check callback when one exists.
 *
 * Original register convention: unaff_EBX -> team.
 *
 * @address 0x463730
 */
uint32_t Scoreboard::is_tracked_object_winner(int32_t team)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    for (element = halo::memory::data_iterator_next(&iter); element != 0; element = halo::memory::data_iterator_next(&iter)) {
        player *p = (player *)element;
        if (p->team == team) {
            break;
        }
    }
    if (element == 0) {
        return 0;
    }

    if (current_game_engine == 0) {
        return 0;
    }
    if (current_game_engine->is_winner == 0) {
        return halo::game::game_engine_is_object_winning(iter.index);
    }
    return ((uint32_t (*)(datum_index))current_game_engine->is_winner)((datum_index)halo::k_dword_none);
}

/**
 * Returns whether the identifier names a valid player of a team.
 *
 * @address 0x466b60
 */
uint8_t Scoreboard::is_valid_team_player(uint32_t identifier)
{
    uint32_t player_index;
    player *p;

    if (current_game_engine == 0) {
        return 1;
    }
    if ((game_engine_unknown_aa00 & 2) != 0) {
        return 0;
    }
    if (light_count_enabled < 1) {
        return 0;
    }
    if (light_count_enabled >= 2) {
        return 1;
    }

    player_index = halo::game::player_index_from_unit_index(identifier);
    if (player_index == halo::k_dword_none) {
        return 0;
    }

    p = halo::game::player_at(player_index);
    return p->local_player_index != -1;
}

/**
 * Returns whether the score of a local player is zero or below.
 *
 * Original register convention: EAX -> player_handle.
 *
 * @address 0x466340
 */
uint8_t Scoreboard::local_player_score_is_nonpositive(datum_index player_handle)
{
    player *p;
    int16_t local_player_index;

    if (current_game_engine == 0 || player_handle == (datum_index)halo::k_dword_none) {
        return 1;
    }

    p = halo::game::player_at(player_handle);
    local_player_index = p->local_player_index;
    if (local_player_index != -1) {
        if (game_engine_nameplate_fade_opacity_array[local_player_index] > 0.0f) {
            return 0;
        }
    }
    return 1;
}

}
