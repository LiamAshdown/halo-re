#include "halo/game/game2_variants.hpp"
#include "halo/game/constants.hpp"
#include "halo/text/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &game_engine_active_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_active_variant);
static auto &network_server = halo::link::ref<uint8_t *>(halo::networking::vars().network_server);
static auto &game_variant_history = halo::link::ref<game_variant_history_entry *>(halo::game::vars().game_variant_history);
static auto &game_variant_history_count = halo::link::ref<uint32_t>(halo::game::vars().game_variant_history_count);
static auto &game_variant_history_capacity = halo::link::ref<uint32_t>(halo::game::vars().game_variant_history_capacity);

namespace halo::game {

/**
 * Looks up a game variant by name and installs it as the active variant, propagating the change to the network
 * layer if it is hosting and the engine type actually changed.
 *
 * @address 0x45b920
 */
void GameVariantRules::set_variant_by_name(const char *name)
{
    game_variant looked_up;

    if (halo::game::game_engine_get_variant_by_name(name, &looked_up) != 0) {
        game_engine_active_variant = looked_up;
        if (network_server != (void *)0 &&
            ((network_server_globals *)network_server)->session.variant.game_engine_index != looked_up.game_engine_index) {
            ((network_server_globals *)network_server)->session.variant = looked_up;
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
 * Adds a named custom game variant (with either supplied or default option data) to the dynamically-grown
 * recent/custom variant cache.
 *
 * @address 0x463980
 */
uint32_t GameVariantRules::variant_add_to_history(char *name, game_variant *options, char *path)
{
    game_variant temp;
    void *name_copy;

    if (halo::game::game_engine_is_map_and_variant_valid(0, 0) == 0) {
        return 0;
    }

    if (path == 0) {
        int32_t length = (int32_t)wcslen((const wchar_t *)options);
        name_copy = GlobalAlloc(0, (uint32_t)length + 1);
        path = reinterpret_cast<char *>(halo::text::string_convert_unicode_to_ascii(static_cast<uint8_t *>(name_copy), reinterpret_cast<uint16_t *>(options), length + 1));
        ((char *)name_copy)[length] = 0;
    } else {
        size_t length = strlen(path);
        name_copy = GlobalAlloc(0, (uint32_t)(length + 1));
        memcpy(name_copy, path, length + 1);
    }

    if (options == 0) {
        if (halo::game::game_engine_get_variant_by_name(name, &temp) == 0) {
            GlobalFree(name_copy);
            return 0;
        }
    } else {
        memcpy(&temp, options, sizeof(game_variant));
    }

    if (name_copy != 0) {
        game_variant_history_entry *entry;

        if (game_variant_history_capacity <= game_variant_history_count) {
            uint32_t bytes;
            game_variant_history_capacity = game_variant_history_capacity + 4;
            bytes = game_variant_history_capacity * sizeof(game_variant_history_entry);
            if (game_variant_history == 0) {
                game_variant_history = (game_variant_history_entry *)GlobalAlloc(0, bytes);
            } else if (bytes == 0) {
                GlobalFree(game_variant_history);
                game_variant_history = 0;
            } else {
                game_variant_history = (game_variant_history_entry *)GlobalReAlloc(game_variant_history, bytes, 2);
            }
        }

        entry = &game_variant_history[game_variant_history_count];
        game_variant_history_count = game_variant_history_count + 1;

        entry->options = temp;
        entry->name = (char *)name_copy;

        {
            size_t name_length = strlen(name);
            char *path_copy = (char *)GlobalAlloc(0, (uint32_t)(name_length + 1));
            entry->path = path_copy;
            memcpy(path_copy, name, name_length + 1);
        }
        return 1;
    }
    return 0;
}

/**
 * Returns one of eight hardcoded 32-bit constants selected by `selector` (0..7), or 0x249248 for any other
 * value. UNSURE: see header for what these values actually mean.
 *
 * @address 0x465380
 */
uint32_t GameVariantRules::variant_option_default_by_index(uint32_t selector)
{
    switch (selector) {
    case 0: return halo::game::k_default_vehicle_set;
    case 1: return 0x1;
    case 2: return 0x42;
    case 3: return 0x203;
    case 4: return 0x1004;
    case 5: return 0x8005;
    case 6: return 0x40006;
    case 7: return 0x200007;
    default: return 0x249248;
    }
}

/**
 * Clamps and normalizes every numeric/boolean field of a game-variant options block in place, then re-
 * normalizes the CTF-specific fields (0x7c..0x80) if the engine is CTF, or a subset of them if it is Slayer.
 *
 * @address 0x466730
 */
void GameVariantRules::variant_sanitize_options(game_variant *variant)
{
    int32_t engine_index;

    engine_index = variant->game_engine_index;
    if (engine_index < _game_engine_ctf) {
        engine_index = _game_engine_ctf;
    } else if (engine_index > _game_engine_race) {
        engine_index = _game_engine_race;
    }
    variant->game_engine_index = engine_index;

    variant->teams = (variant->teams != 0);
    variant->odd_man_out = (variant->odd_man_out != 0);

    if (variant->respawn_time_growth < 0) variant->respawn_time_growth = 0;
    if (variant->respawn_time < 0) variant->respawn_time = 0;
    if (variant->suicide_penalty < 0) variant->suicide_penalty = 0;
    if (variant->lives_per_round < 0) variant->lives_per_round = 0;

    if (variant->health < 0.25f) {
        variant->health = 0.25f;
    } else if (variant->health > 4.0f) {
        variant->health = 4.0f;
    }

    if (variant->weapon_set < 0) {
        variant->weapon_set = 0;
    } else if (variant->weapon_set > 0xd) {
        variant->weapon_set = 0xd;
    }

    {
        uint32_t low_nibble = variant->red_vehicle_set & 0xf;
        if (low_nibble > 8) {
            low_nibble = 8;
        }
        variant->red_vehicle_set = (variant->red_vehicle_set & ~(uint32_t)0xf) | low_nibble;
    }

    if (engine_index == _game_engine_ctf) {
        variant->engine.ctf.assault = (variant->engine.ctf.assault != 0);
        variant->engine.ctf.unknown_7d = (variant->engine.ctf.unknown_7d != 0);
        variant->engine.ctf.flag_must_reset = (variant->engine.ctf.flag_must_reset != 0);
        variant->engine.ctf.flag_at_home_to_score = (variant->engine.ctf.flag_at_home_to_score != 0);
        variant->teams = 1;
        if (variant->engine.ctf.single_flag_time < 0) {
            variant->engine.ctf.single_flag_time = 0;
        }
    } else if (engine_index == _game_engine_slayer) {
        variant->engine.slayer.death_bonus = (variant->engine.slayer.death_bonus != 0);
        variant->engine.slayer.kill_penalty = (variant->engine.slayer.kill_penalty != 0);
        variant->engine.slayer.kill_in_order = (variant->engine.slayer.kill_in_order != 0);
    }
}

}
