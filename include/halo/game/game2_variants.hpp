#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Default option blocks of the built-in multiplayer game types, one factory function per type.
 */
class VariantDefaults {
public:
    static game_variant * assault(game_variant *variant_options);
    static game_variant * classic_accumulation(game_variant *variant_options);
    static game_variant * classic_crazy_king(game_variant *variant_options);
    static game_variant * classic_ctf(game_variant *variant_options);
    static game_variant * classic_ctf_pro(game_variant *variant_options);
    static game_variant * classic_elimination(game_variant *out);
    static game_variant * classic_endurance(game_variant *out);
    static game_variant * classic_invasion(game_variant *variant_options);
    static game_variant * classic_iron_ctf(game_variant *variant_options);
    static game_variant * classic_juggernaut(game_variant *variant_options);
    static game_variant * classic_king(game_variant *variant_options);
    static game_variant * classic_king_pro(game_variant *variant_options);
    static game_variant * classic_oddball(game_variant *out);
    static game_variant * classic_phantoms(game_variant *out);
    static game_variant * classic_race(game_variant *variant_options);
    static game_variant * classic_rally(game_variant *variant_options);
    static game_variant * classic_reverse_tag(game_variant *variant_options);
    static game_variant * classic_rockets(game_variant *out);
    static game_variant * classic_slayer(game_variant *out);
    static game_variant * classic_slayer_pro(game_variant *out);
    static game_variant * classic_snipers(game_variant *out);
    static game_variant * classic_stalker(game_variant *variant_options);
    static game_variant * classic_team_king(game_variant *variant_options);
    static game_variant * classic_team_oddball(game_variant *out);
    static game_variant * classic_team_race(game_variant *variant_options);
    static game_variant * classic_team_rally(game_variant *variant_options);
    static game_variant * classic_team_slayer(game_variant *out);
    static game_variant * crazy_king(game_variant *variant_options);
    static game_variant * juggernaut(game_variant *variant_options);
    static game_variant * king(game_variant *variant_options);
    static game_variant * oddball(game_variant *variant_options);
    static game_variant * race(game_variant *variant_options);
    static game_variant * slayer(game_variant *variant_options);
    static game_variant * stalker(game_variant *variant_options);
    static game_variant * team_king(game_variant *variant_options);
    static game_variant * team_oddball(game_variant *variant_options);
    static game_variant * team_race(game_variant *variant_options);
    static game_variant * team_slayer(game_variant *variant_options);
};

/**
 * Game-variant bookkeeping: default selection, history, sanitising of option blocks.
 */
class GameVariantRules {
public:
    static void set_variant_by_name(const char *name);
    static void sync_variant_defaults(void);
    static uint32_t variant_add_to_history(char *name, game_variant *options, char *path);
    static uint32_t variant_option_default_by_index(uint32_t selector);
    static void variant_sanitize_options(game_variant *variant);
};

struct VariantDefaultsEntry {
    game_variant *(*fill)(game_variant *out);
};

/**
 * Table of every built-in variant-defaults factory, in the order the original image defined them.
 */
const VariantDefaultsEntry *variant_defaults_table(int32_t *out_count);

}
