#include "halo/game/game2_variants.hpp"
#include "halo/game/variant_flags.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/api.hpp"


namespace halo::game {

/**
 * Fills the option block with the default settings of the built-in assault game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467270
 */
game_variant * VariantDefaults::assault(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 5;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = 900;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.ctf.assault = 1;
    defaults.engine.ctf.flag_must_reset = 0;
    defaults.engine.ctf.flag_at_home_to_score = 0;
    defaults.engine.ctf.single_flag_time = halo::game::seconds_to_ticks(120);
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic accumulation game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464600
 */
game_variant * VariantDefaults::classic_accumulation(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 2;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 5;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 1;
    defaults.blue_vehicle_set = 1;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.unknown_7d = 0;
    defaults.engine.oddball.trait_with_ball = 0;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 1;
    defaults.engine.oddball.ball_count = 0x10;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic crazy king game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464aa0
 */
game_variant * VariantDefaults::classic_crazy_king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.king.moving_hill = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic ctf game type, zeroing it first,
 * and returns it.
 *
 * @address 0x464c40
 */
game_variant * VariantDefaults::classic_ctf(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.ctf.assault = 0;
    defaults.engine.ctf.unknown_7d = 0;
    defaults.engine.ctf.flag_must_reset = 0;
    defaults.engine.ctf.flag_at_home_to_score = 0;
    defaults.engine.ctf.single_flag_time = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic ctf pro game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464d30
 */
game_variant * VariantDefaults::classic_ctf_pro(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::k_ticks_per_fifteen_seconds;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.ctf.assault = 0;
    defaults.engine.ctf.unknown_7d = 0;
    defaults.engine.ctf.flag_must_reset = 0;
    defaults.engine.ctf.flag_at_home_to_score = 1;
    defaults.engine.ctf.single_flag_time = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic elimination game type, zeroing it
 * first, and returns it.
 *
 * @address 0x463e00
 */
game_variant * VariantDefaults::classic_elimination(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    out->lives_per_round = 1;
    out->suicide_penalty = 300;
    out->health = 1.0f;
    out->score_limit = 0x19;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic endurance game type, zeroing it
 * first, and returns it.
 *
 * @address 0x463fc0
 */
game_variant * VariantDefaults::classic_endurance(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    out->odd_man_out = 1;
    out->respawn_time_growth = 300;
    out->suicide_penalty = 300;
    out->lives_per_round = 5;
    out->health = 1.0f;
    out->score_limit = 10;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic invasion game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464e20
 */
game_variant * VariantDefaults::classic_invasion(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 5;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.ctf.assault = 1;
    defaults.engine.ctf.unknown_7d = 0;
    defaults.engine.ctf.flag_must_reset = 0;
    defaults.engine.ctf.flag_at_home_to_score = 0;
    defaults.engine.ctf.single_flag_time = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic iron ctf game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464f10
 */
game_variant * VariantDefaults::classic_iron_ctf(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::k_ticks_per_fifteen_seconds;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 2.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x408;
    defaults.blue_vehicle_set = 0x408;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.ctf.assault = 0;
    defaults.engine.ctf.unknown_7d = 0;
    defaults.engine.ctf.flag_must_reset = 1;
    defaults.engine.ctf.flag_at_home_to_score = 0;
    defaults.engine.ctf.single_flag_time = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic juggernaut game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464700
 */
game_variant * VariantDefaults::classic_juggernaut(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 10;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.unknown_7d = 0;
    defaults.engine.oddball.trait_with_ball = 2;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 2;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic king game type, zeroing it first,
 * and returns it.
 *
 * @address 0x464900
 */
game_variant * VariantDefaults::classic_king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.king.moving_hill = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic king pro game type, zeroing it
 * first, and returns it.
 *
 * @address 0x4649d0
 */
game_variant * VariantDefaults::classic_king_pro(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::k_ticks_per_fifteen_seconds;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.king.moving_hill = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic oddball game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464340
 */
game_variant * VariantDefaults::classic_oddball(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_oddball;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    out->objective_indicator = 1;
    out->respawn_time = halo::game::seconds_to_ticks(5);
    out->suicide_penalty = halo::game::seconds_to_ticks(5);
    out->health = 1.0f;
    out->score_limit = 2;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 1;
    out->blue_vehicle_set = 1;
    out->friendly_fire = 1;
    out->engine.oddball.unknown_7d = 1;
    out->engine.oddball.ball_count = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic phantoms game type, zeroing it
 * first, and returns it.
 *
 * @address 0x463ee0
 */
game_variant * VariantDefaults::classic_phantoms(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::invisible_players | halo::game::game_variant_flags::object_placement_filter);
    out->objective_indicator = 1;
    out->respawn_time = halo::game::seconds_to_ticks(5);
    out->suicide_penalty = halo::game::seconds_to_ticks(5);
    out->health = 1.0f;
    out->score_limit = 10;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->engine.slayer.death_bonus = 1;
    out->engine.slayer.kill_penalty = 1;
    out->engine.slayer.kill_in_order = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic race game type, zeroing it first,
 * and returns it.
 *
 * @address 0x465000
 */
game_variant * VariantDefaults::classic_race(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.race.race_type = 0;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic rally game type, zeroing it first,
 * and returns it.
 *
 * @address 0x4650e0
 */
game_variant * VariantDefaults::classic_rally(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 0xf;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.race.race_type = 2;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic reverse tag game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464510
 */
game_variant * VariantDefaults::classic_reverse_tag(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 1;
    defaults.blue_vehicle_set = 1;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.unknown_7d = 1;
    defaults.engine.oddball.speed_with_ball = 0;
    defaults.engine.oddball.trait_with_ball = 0;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 1;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic rockets game type, zeroing it
 * first, and returns it.
 *
 * @address 0x4640a0
 */
game_variant * VariantDefaults::classic_rockets(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    out->objective_indicator = 1;
    out->suicide_penalty = 300;
    out->health = 1.0f;
    out->score_limit = 0x19;
    out->weapon_set = 6;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic slayer game type, zeroing it first,
 * and returns it.
 *
 * @address 0x463c40
 */
game_variant * VariantDefaults::classic_slayer(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    out->suicide_penalty = 300;
    out->health = 1.0f;
    out->score_limit = 0x0f;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic slayer pro game type, zeroing it
 * first, and returns it.
 *
 * @address 0x463d20
 */
game_variant * VariantDefaults::classic_slayer_pro(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    out->suicide_penalty = halo::game::k_ticks_per_fifteen_seconds;
    out->health = 1.0f;
    out->score_limit = 0x19;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->engine.slayer.death_bonus = 1;
    out->engine.slayer.kill_penalty = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic snipers game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464180
 */
game_variant * VariantDefaults::classic_snipers(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    out->objective_indicator = 1;
    out->respawn_time_growth = halo::game::seconds_to_ticks(5);
    out->suicide_penalty = 300;
    out->health = 1.0f;
    out->score_limit = 0x0f;
    out->weapon_set = 4;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic stalker game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464800
 */
game_variant * VariantDefaults::classic_stalker(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 0;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 10;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.unknown_7d = 0;
    defaults.engine.oddball.speed_with_ball = 2;
    defaults.engine.oddball.trait_with_ball = 1;
    defaults.engine.oddball.trait_without_ball = 3;
    defaults.engine.oddball.ball_type = 2;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic team king game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464b70
 */
game_variant * VariantDefaults::classic_team_king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.king.moving_hill = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic team oddball game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464430
 */
game_variant * VariantDefaults::classic_team_oddball(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_oddball;
    out->teams = 1;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::loadout_override | halo::game::game_variant_flags::object_placement_filter);
    out->objective_indicator = 1;
    out->respawn_time = 300;
    out->health = 1.0f;
    out->score_limit = 2;
    out->suicide_penalty = halo::game::seconds_to_ticks(5);
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 1;
    out->blue_vehicle_set = 1;
    out->friendly_fire = 1;
    out->engine.oddball.ball_count = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in classic team race game type, zeroing it
 * first, and returns it.
 *
 * @address 0x4651c0
 */
game_variant * VariantDefaults::classic_team_race(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.race.race_type = 0;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic team rally game type, zeroing it
 * first, and returns it.
 *
 * @address 0x4652a0
 */
game_variant * VariantDefaults::classic_team_rally(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 5;
    defaults.weapon_set = 0xb;
    defaults.red_vehicle_set = 0x42;
    defaults.blue_vehicle_set = 0x42;
    defaults.vehicle_respawn_time = 0;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 0;
    defaults.engine.race.race_type = 2;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in classic team slayer game type, zeroing it
 * first, and returns it.
 *
 * @address 0x464260
 */
game_variant * VariantDefaults::classic_team_slayer(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->teams = 1;
    out->flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::object_placement_filter);
    out->respawn_time = 300;
    out->suicide_penalty = 300;
    out->health = 1.0f;
    out->score_limit = 0x32;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 0x42;
    out->blue_vehicle_set = 0x42;
    out->friendly_fire = 1;
    out->variant_flags = 1;
    return out;
}

/**
 * Fills the option block with the default settings of the built-in crazy king game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467370
 */
game_variant * VariantDefaults::crazy_king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.king.moving_hill = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in juggernaut game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467540
 */
game_variant * VariantDefaults::juggernaut(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 0xf;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.speed_with_ball = 1;
    defaults.engine.oddball.trait_with_ball = 2;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 2;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in king game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467650
 */
game_variant * VariantDefaults::king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.king.moving_hill = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in oddball game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467730
 */
game_variant * VariantDefaults::oddball(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.speed_with_ball = 1;
    defaults.engine.oddball.trait_with_ball = 0;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 0;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in race game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467830
 */
game_variant * VariantDefaults::race(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = 900;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.race.race_type = 0;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in slayer game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467920
 */
game_variant * VariantDefaults::slayer(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_slayer;
    defaults.teams = 0;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 0;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 0x19;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.slayer.death_bonus = 1;
    defaults.engine.slayer.kill_penalty = 1;
    defaults.engine.slayer.kill_in_order = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in stalker game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467450
 */
game_variant * VariantDefaults::stalker(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_ctf;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::hide_radar_blips | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.ctf.assault = 0;
    defaults.engine.ctf.flag_must_reset = 0;
    defaults.engine.ctf.flag_at_home_to_score = 1;
    defaults.engine.ctf.single_flag_time = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in team king game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467a10
 */
game_variant * VariantDefaults::team_king(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_king;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.king.moving_hill = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in team oddball game type, zeroing it first,
 * and returns it.
 *
 * @address 0x467af0
 */
game_variant * VariantDefaults::team_oddball(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_oddball;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = halo::game::seconds_to_ticks(5);
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 2;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.speed_with_ball = 1;
    defaults.engine.oddball.trait_with_ball = 0;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 0;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in team race game type, zeroing it first, and
 * returns it.
 *
 * @address 0x467c00
 */
game_variant * VariantDefaults::team_race(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_race;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0;
    defaults.suicide_penalty = 300;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 3;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = 900;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.race.race_type = 0;
    defaults.engine.race.team_scoring = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

/**
 * Fills the option block with the default settings of the built-in team slayer game type, zeroing it first,
 * and returns it.
 *
 * @address 0x467cf0
 */
game_variant * VariantDefaults::team_slayer(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = _game_engine_slayer;
    defaults.teams = 1;
    defaults.flags = halo::to_bits(halo::game::game_variant_flags::players_on_radar | halo::game::game_variant_flags::friend_indicators | halo::game::game_variant_flags::hide_radar_blips | halo::game::game_variant_flags::slayer_default);
    defaults.objective_indicator = 0;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 300;
    defaults.suicide_penalty = halo::game::seconds_to_ticks(5);
    defaults.lives_per_round = 0;
    defaults.health = 1.0f;
    defaults.score_limit = 0x32;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.blue_vehicle_set = halo::game::k_default_vehicle_set;
    defaults.vehicle_respawn_time = halo::game::k_ticks_per_minute;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 300;
    defaults.team_autobalance = 1;
    defaults.time_limit = 36000;
    defaults.engine.slayer.death_bonus = 1;
    defaults.engine.slayer.kill_penalty = 1;
    defaults.engine.slayer.kill_in_order = 0;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

}
