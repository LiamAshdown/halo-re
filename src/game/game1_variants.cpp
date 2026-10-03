/**
 * Game variant selection, loading, validation and player profile capture.
 */

#include "crt.h"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#include "win32.h"
#include <wchar.h>
#include "interface.h"

#include "halo/game/game1_variants.hpp"
#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

typedef game_variant *(*game_engine_variant_defaults_fn)(game_variant *out);
typedef void (*profile_post_update_proc)(uint32_t arg_edx, uint32_t arg_ecx);

extern "C" {
extern uint32_t game_variant_history_count;
extern int32_t game_variant_history_current;
extern game_variant_history_entry *game_variant_history;
extern char variant_defaults_source[0x40];
extern game_variant game_engine_pending_variant;
extern int32_t sv_friendly_fire_mode;
extern int32_t sv_timelimit_minutes;
extern player_profile player_profile_cache[16];
extern game_variant game_engine_variant;
extern data_array *player_data;
extern int32_t game_engine_player_profile_cache_find(datum_index player_handle);
extern int32_t *machine_table;
extern game_variant game_engine_active_variant;
extern uint8_t *network_server;
extern uint8_t game_variant_saved_default_valid;
extern game_variant game_variant_saved_default;
extern char network_build_string[];
extern uint32_t game_variant_history_capacity;
extern uint8_t playlist_profiles_need_defaults;
extern void playlist_profile_create_default_profiles_on_disk(void);
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count);
extern uint8_t saved_game_get_variant(int32_t slot, game_variant *out);
extern game_engine_definition *current_game_engine;
extern uint32_t game_engine_unknown_aa00;
extern int32_t game_engine_auto_team_counter;
extern float game_engine_end_game_timer;
extern float game_engine_post_game_fade;
extern game_engine_state game_engine_state_value;
extern float game_engine_nameplate_fade_opacity_array[];
extern uint8_t game_engine_dedicated_idle;
extern float game_engine_dedicated_idle_timer;
extern int32_t game_engine_round_reset_tick;
extern game_engine_definition *game_engine_definitions[7];
}

namespace halo::game::engine1 {

/**
 * Cycles to and loads the next entry of the custom game-variant cache into the currently active variant state.
 *
 * @address 0x463b90
 */
void Variants::apply_current_custom_variant(void)
{
    game_variant_history_entry *entry;
    int32_t mode;
    uint8_t mode_is_1;

    if (game_variant_history_count == 0) {
        return;
    }

    game_variant_history_current = game_variant_history_current + 1;
    if (game_variant_history_count <= (uint32_t)game_variant_history_current) {
        game_variant_history_current = 0;
    }

    entry = &game_variant_history[game_variant_history_current];
    strncpy(variant_defaults_source, entry->name, 0x3f);

    mode = sv_friendly_fire_mode;
    mode_is_1 = (mode == 1);
    variant_defaults_source[0x3f] = 0;
    game_engine_pending_variant = entry->options;

    if (mode_is_1) {
        game_engine_pending_variant.friendly_fire = 0;
    } else if (mode == 2) {
        game_engine_pending_variant.friendly_fire = 2;
    } else if (mode == 3) {
        game_engine_pending_variant.friendly_fire = 1;
    }

    if (sv_timelimit_minutes != -1) {
        if (sv_timelimit_minutes != 0) {
            game_engine_pending_variant.time_limit = sv_timelimit_minutes * 0x708;
            return;
        }
        game_engine_pending_variant.time_limit = 0;
    }
}

/**
 * Applies a received player profile entry.
 *
 * @address 0x466d00
 */
void Variants::apply_player_profile_entry(void *event)
{
    int32_t lookup_index;
    datum_index search_handle;
    int32_t slot;
    player_profile *profile;
    uint32_t *tail;
    uint8_t committed;
    player *p;

    lookup_index = **(int32_t **)((uint8_t *)event + 0x44);
    search_handle = (datum_index)halo::k_dword_none;
    if (lookup_index != 0) {
        search_handle = (datum_index)machine_table[lookup_index];
    }

    slot = halo::game::game_engine_player_profile_cache_find(search_handle);
    if (slot == -1) {
        halo::networking::message_delta_decode_compound_field_staged((void **)event);
        return;
    }

    profile = &player_profile_cache[slot];
    tail = (uint32_t *)&profile->kills;

    if (**(int32_t **)event == 0) {
        committed = halo::networking::message_delta_decode_compound_field((void **)event, tail);
    } else {
        uint32_t scratch[11];
        uint32_t i;
        for (i = 0; i < 10; i = i + 1) {
            scratch[i] = tail[i];
        }
        committed = halo::networking::message_delta_decode_compound_field_forced((void **)event, tail, (int32_t)(scratch + 1), 0);
        for (i = 0; i < 10; i = i + 1) {
            tail[i] = scratch[i + 1];
        }
    }

    if (committed != 1) {
        return;
    }

    p = (player *)halo::memory::datum_get(profile->player, player_data);
    if (p == (player *)0) {
        return;
    }

    p->kills = profile->kills;
    p->unknown_9e = profile->unknown_0a;
    p->unknown_a0 = profile->unknown_0c;
    p->assists = profile->assists;
    p->unknown_a6 = profile->unknown_12;
    p->unknown_a8 = profile->unknown_14;
    p->betrayals = profile->betrayals;
    p->deaths = profile->deaths;
    p->suicides = profile->suicides;
    p->objective_time = profile->objective_time;
    p->objective_score = profile->objective_score;
    p->slayer_target = profile->slayer_target;
    p->odd_man_out = profile->odd_man_out;
    p->speed = profile->speed;

    if (game_engine_variant.game_engine_index == _game_engine_king) {
        *(int16_t *)&p->objective_time = (int16_t)profile->objective_time * 0x1e;
    }
}

/**
 * Applies a game variant to the active game engine.
 *
 * Original register convention: EDX -> variant.
 *
 * @address 0x45b990
 */
void Variants::apply_variant(const game_variant *variant)
{
    if (variant != (const game_variant *)0) {
        game_engine_active_variant = *variant;
        if (network_server != (void *)0 &&
            *(int32_t *)((uint8_t *)network_server + 0x13c) != variant->game_engine_index) {
            *(game_variant *)((uint8_t *)network_server + 0x10c) = *variant;
            halo::networking::network_game_broadcast_player_set_changed(network_server);
        }
    } else {
        uint8_t *dst = (uint8_t *)&game_engine_active_variant;
        uint32_t i;
        for (i = 0; i < sizeof(game_variant); i = i + 1) {
            dst[i] = 0;
        }
    }
}

/**
 * Ensures the recent/custom game-variant cache has at least one entry, restoring a previously saved default
 * variant into it if it was empty.
 *
 * @address 0x463b20
 */
uint32_t Variants::ensure_variant_history_has_entry(void)
{
    halo::game::game_engine_free_custom_variant_cache();
    if (game_variant_history_count != 0) {
        return 1;
    }
    if (game_variant_saved_default_valid != 0) {
        game_variant temp = game_variant_saved_default;

        halo::game::game_engine_free_custom_variant_cache();
        halo::game::game_engine_variant_add_to_history(network_build_string, &temp, 0);
        if (game_variant_history_count != 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Releases the dynamically allocated recent/custom game-variant cache and resets its bookkeeping globals to
 * empty.
 *
 * @address 0x4638b0
 */
void Variants::free_custom_variant_cache(void)
{
    if (game_variant_history != 0) {
        uint32_t i;
        for (i = 0; i < game_variant_history_count; i++) {
            GlobalFree(game_variant_history[i].path);
            GlobalFree(game_variant_history[i].name);
        }
        GlobalFree(game_variant_history);
    }
    game_variant_history = 0;
    game_variant_history_capacity = 0;
    game_variant_history_count = 0;
    game_variant_history_current = -1;
}

/**
 * Looks a game variant up by name, filling in the defaults of built-in variants.
 *
 * Original register convention: EAX -> out_name, EDI -> max_chars. UNSURE identity, but it is what fills the.
 *
 * @address 0x4622d0
 */
uint8_t Variants::get_variant_by_name(const char *name, game_variant *out)
{
    static const struct { const char *name; game_engine_variant_defaults_fn fn; } k_builtin_variants[] = {
        {"classic_slayer", halo::game::game_engine_variant_defaults_classic_slayer},
        {"classic_slayer_pro", halo::game::game_engine_variant_defaults_classic_slayer_pro},
        {"classic_elimination", halo::game::game_engine_variant_defaults_classic_elimination},
        {"classic_phantoms", halo::game::game_engine_variant_defaults_classic_phantoms},
        {"classic_endurance", halo::game::game_engine_variant_defaults_classic_endurance},
        {"classic_rockets", halo::game::game_engine_variant_defaults_classic_rockets},
        {"classic_snipers", halo::game::game_engine_variant_defaults_classic_snipers},
        {"classic_team_slayer", halo::game::game_engine_variant_defaults_classic_team_slayer},
        {"classic_oddball", halo::game::game_engine_variant_defaults_classic_oddball},
        {"classic_team_oddball", halo::game::game_engine_variant_defaults_classic_team_oddball},
        {"classic_reverse_tag", halo::game::game_engine_variant_defaults_classic_reverse_tag},
        {"classic_accumulation", halo::game::game_engine_variant_defaults_classic_accumulation},
        {"classic_juggernaut", halo::game::game_engine_variant_defaults_classic_juggernaut},
        {"classic_stalker", halo::game::game_engine_variant_defaults_classic_stalker},
        {"classic_king", halo::game::game_engine_variant_defaults_classic_king},
        {"classic_king_pro", halo::game::game_engine_variant_defaults_classic_king_pro},
        {"classic_crazy_king", halo::game::game_engine_variant_defaults_classic_crazy_king},
        {"classic_team_king", halo::game::game_engine_variant_defaults_classic_team_king},
        {"classic_ctf", halo::game::game_engine_variant_defaults_classic_ctf},
        {"classic_ctf_pro", halo::game::game_engine_variant_defaults_classic_ctf_pro},
        {"classic_invasion", halo::game::game_engine_variant_defaults_classic_invasion},
        {"classic_iron_ctf", halo::game::game_engine_variant_defaults_classic_iron_ctf},
        {"classic_race", halo::game::game_engine_variant_defaults_classic_race},
        {"classic_rally", halo::game::game_engine_variant_defaults_classic_rally},
        {"classic_team_race", halo::game::game_engine_variant_defaults_classic_team_race},
        {"classic_team_rally", halo::game::game_engine_variant_defaults_classic_team_rally},
        {"team_slayer", halo::game::game_engine_variant_defaults_team_slayer},
        {"team_race", halo::game::game_engine_variant_defaults_team_race},
        {"team_oddball", halo::game::game_engine_variant_defaults_team_oddball},
        {"team_king", halo::game::game_engine_variant_defaults_team_king},
        {"slayer", halo::game::game_engine_variant_defaults_slayer},
        {"race", halo::game::game_engine_variant_defaults_race},
        {"oddball", halo::game::game_engine_variant_defaults_oddball},
        {"king", halo::game::game_engine_variant_defaults_king},
        {"juggernaut", halo::game::game_engine_variant_defaults_juggernaut},
    };
    game_variant staging;
    wchar_t requested_name_wide[24];
    uint8_t matched = 0;
    size_t i;

    for (i = 0; i < sizeof(k_builtin_variants) / sizeof(k_builtin_variants[0]); i++) {
        if (_stricmp(name, k_builtin_variants[i].name) == 0) {
            if (out == 0) {
                return 1;
            }
            k_builtin_variants[i].fn(&staging);
            matched = 1;
            break;
        }
    }

    if (matched == 0 && _stricmp(name, "ctf") == 0) {
        if (out == 0) {
            return 1;
        }
        halo::game::game_engine_variant_defaults_stalker(&staging);
        matched = 1;
    } else if (matched == 0 && _stricmp(name, "crazy_king") == 0) {
        if (out == 0) {
            return 1;
        }
        halo::game::game_engine_variant_defaults_crazy_king(&staging);
        matched = 1;
    } else if (matched == 0 && _stricmp(name, "assault") != 0) {
        int32_t slots[100];
        int32_t slot_count = 100;
        uint16_t slot_index;

        halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(requested_name_wide), 0x30, name);
        if (playlist_profiles_need_defaults == 1) {
            halo::saved_games::playlist_profile_create_default_profiles_on_disk();
            playlist_profiles_need_defaults = 0;
        }

        halo::saved_games::saved_game_enumerate_by_type(1, slots, 1, (uint16_t *)&slot_count);

        for (slot_index = 0; (int32_t)(uint32_t)slot_index < slot_count; slot_index++) {
            if (slots[slot_index] == -1) {
                halo::game::game_engine_apply_current_custom_variant();
                continue;
            }
            if (halo::saved_games::saved_game_get_variant(slots[slot_index], &staging) != 0 &&
                _wcsicmp((const wchar_t *)staging.name, requested_name_wide) == 0) {
                if (out == 0) {
                    return 1;
                }
                matched = 1;
                break;
            }
        }
        if (matched == 0) {
            return 0;
        }
    } else if (matched == 0) {
        if (out == 0) {
            return 1;
        }
        halo::game::game_engine_variant_defaults_assault(&staging);
    }

    memcpy(out, &staging, sizeof(game_variant));
    return 1;
}

/**
 * Invokes the profile post-update callback slot of the active game engine.
 *
 * Original register convention: ECX -> arg_ecx, EDX -> arg_edx.
 *
 * @address 0x466e60
 */
void Variants::invoke_profile_post_update_callback(uint32_t arg_ecx, uint32_t arg_edx)
{
    profile_post_update_proc callback = (profile_post_update_proc)current_game_engine->profile_post_update;
    if (callback != 0) {
        callback(arg_edx, arg_ecx);
    }
}

/**
 * Checks whether the currently loaded map is present/ enabled in the multiplayer maps table and, optionally,
 * that the active game variant is a recognized one.
 *
 * @address 0x463920
 */
uint32_t Variants::is_map_and_variant_valid(const char *map_path, const char *variant_name)
{
    int32_t map_index;

    const char *map_name = strrchr(map_path, '\\');
    map_name = map_name != (const char *)0 ? map_name + 1 : map_path;
    map_index = halo::interface::map_list_find_known_map_index((char *)map_name);

    if (map_index != -1 && -1 < map_index && map_index < halo::interface::globals().map_list_count &&
        halo::interface::globals().map_list[map_index].cache_file_exists != 0) {
        if (variant_name != 0) {
            return (uint32_t)halo::game::game_engine_get_variant_by_name(variant_name, 0);

        }
        return 1;
    }
    return 0;
}

/**
 * Loads the game engine state from a game variant.
 *
 * Original register convention: EBX -> variant.
 *
 * @address 0x45c2c0
 */
void Variants::load_from_variant(const game_variant *variant)
{
    uint32_t *src;
    uint32_t *dst;
    int32_t i;

    game_engine_unknown_aa00 = 0;
    game_engine_auto_team_counter = 0;
    game_engine_end_game_timer = 0.0f;
    game_engine_post_game_fade = 0.0f;
    game_engine_nameplate_fade_opacity_array[0] = 0.0f;
    game_engine_dedicated_idle = 0;
    game_engine_dedicated_idle_timer = 0.0f;
    game_engine_round_reset_tick = 0;
    game_engine_state_value = _game_engine_state_not_started;

    if (variant != (const game_variant *)0 && variant->game_engine_index != 0) {
        src = (uint32_t *)variant;
        dst = (uint32_t *)&game_engine_variant;
        for (i = 0x26; i != 0; i = i - 1) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
        }
        halo::game::game_variant_sanitize_options(&game_engine_variant);
        current_game_engine = game_engine_definitions[variant->game_engine_index];
    }
    halo::game::player_profile_cache_initialize();
}

}
