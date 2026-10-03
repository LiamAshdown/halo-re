/**
 * Entry functions for the game module slice converted in wave 2 (game engines, variants, lifecycle): one
 * function per original symbol, forwarding to the C++ classes in namespace halo::game.
 */

#include "halo/game/game2_engines.hpp"
#include "halo/game/game2_variants.hpp"
#include "halo/game/game2_engine_players.hpp"
#include "halo/game/game2_engine_placement.hpp"
#include "halo/game/game2_engine_hud.hpp"
#include "halo/game/game2_engine_match.hpp"
#include "halo/game/game2_game_lifecycle.hpp"
#include "halo/game/api.hpp"

namespace halo::game {

/**
 * C entry point for halo::game::SlayerEngine::build_message_text; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46f610
 */
uint8_t game_engine_slayer_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    return halo::game::slayer_engine.build_message_text(recipient, message_type, subject, text, count);
}

/**
 * C entry point for halo::game::SlayerEngine::build_player_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f9e0
 */
wchar_t * game_engine_slayer_build_player_text(datum_index player, wchar_t *buffer)
{
    return halo::game::slayer_engine.build_player_text(player, buffer);
}

/**
 * C entry point for halo::game::SlayerEngine::build_team_score_text; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46fa10
 */
wchar_t * game_engine_slayer_build_team_score_text(int32_t team, wchar_t *buffer)
{
    return halo::game::slayer_engine.build_team_score_text(team, buffer);
}

/**
 * C entry point for halo::game::SlayerEngine::get_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f980
 */
int32_t game_engine_slayer_get_score(datum_index player, int32_t team_mode)
{
    return halo::game::slayer_engine.get_score(player, team_mode);
}

/**
 * C entry point for halo::game::SlayerEngine::get_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f9c0
 */
int32_t game_engine_slayer_get_team_score(int32_t team)
{
    return halo::game::slayer_engine.get_team_score(team);
}

/**
 * C entry point for halo::game::SlayerEngine::initialize_for_new_game; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46f380
 */
uint8_t game_engine_slayer_initialize_for_new_game(void)
{
    return halo::game::slayer_engine.initialize_for_new_game();
}

/**
 * C entry point for halo::game::SlayerEngine::player_killed; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f580
 */
void game_engine_slayer_player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    halo::game::slayer_engine.player_killed(killer, death_object, victim, is_suicide);
}

/**
 * C entry point for halo::game::SlayerEngine::player_new_life; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f3c0
 */
void game_engine_slayer_player_new_life(datum_index player_index)
{
    halo::game::slayer_engine.player_new_life(player_index);
}

/**
 * C entry point for halo::game::SlayerEngine::player_round_reset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46fc70
 */
void game_engine_slayer_player_round_reset(datum_index player_index)
{
    halo::game::slayer_engine.player_round_reset(player_index);
}

/**
 * C entry point for halo::game::SlayerEngine::profile_post_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46fb20
 */
void game_engine_slayer_profile_post_update(void **context)
{
    halo::game::slayer_engine.profile_post_update(context);
}

/**
 * C entry point for halo::game::SlayerEngine::profiles_updated; forwards to the C++ implementation unchanged.
 *
 * @address 0x46fa40
 */
void game_engine_slayer_profiles_updated(int32_t mode, int32_t machine_index)
{
    halo::game::slayer_engine.profiles_updated(mode, machine_index);
}

/**
 * C entry point for halo::game::SlayerEngine::query_player_score; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46fd30
 */
uint8_t game_engine_slayer_query_player_score(int32_t key, int32_t index, void *buffer)
{
    return halo::game::slayer_engine.query_player_score(key, index, buffer);
}

/**
 * C entry point for halo::game::SlayerEngine::query_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46fdb0
 */
uint8_t game_engine_slayer_query_team_score(int32_t key, int32_t team, void *buffer)
{
    return halo::game::slayer_engine.query_team_score(key, team, buffer);
}

/**
 * C entry point for halo::game::SlayerEngine::reset_objects; forwards to the C++ implementation unchanged.
 *
 * @address 0x46fde0
 */
void game_engine_slayer_reset_objects(void)
{
    halo::game::slayer_engine.reset_objects();
}

/**
 * C entry point for halo::game::SlayerEngine::reset_round; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f420
 */
void game_engine_slayer_reset_round(void)
{
    halo::game::slayer_engine.reset_round();
}

/**
 * C entry point for halo::game::SlayerEngine::unknown_84; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f9d0
 */
uint8_t game_engine_slayer_unknown_84(int32_t kind)
{
    return halo::game::slayer_engine.unknown_84(kind);
}

/**
 * C entry point for halo::game::SlayerEngine::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x46f7f0
 */
void game_engine_slayer_update(datum_index player_index)
{
    halo::game::slayer_engine.update(player_index);
}

/**
 * C entry point for halo::game::OddballEngine::build_team_score_text; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46d020
 */
wchar_t * game_engine_oddball_build_team_score_text(int32_t team, wchar_t *buffer)
{
    return halo::game::oddball_engine.build_team_score_text(team, buffer);
}

/**
 * C entry point for halo::game::OddballEngine::get_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46cea0
 */
int32_t game_engine_oddball_get_score(datum_index player, int32_t team_mode)
{
    return halo::game::oddball_engine.get_score(player, team_mode);
}

/**
 * C entry point for halo::game::OddballEngine::get_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46cee0
 */
int32_t game_engine_oddball_get_team_score(int32_t team)
{
    return halo::game::oddball_engine.get_team_score(team);
}

/**
 * C entry point for halo::game::OddballEngine::initialize_for_new_game; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46c080
 */
uint8_t game_engine_oddball_initialize_for_new_game(void)
{
    return halo::game::oddball_engine.initialize_for_new_game();
}

/**
 * C entry point for halo::game::OddballEngine::player_killed; forwards to the C++ implementation unchanged.
 *
 * @address 0x46c940
 */
void game_engine_oddball_player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    halo::game::oddball_engine.player_killed(killer, death_object, victim, is_suicide);
}

/**
 * C entry point for halo::game::OddballEngine::player_new_life; forwards to the C++ implementation unchanged.
 *
 * @address 0x46c150
 */
void game_engine_oddball_player_new_life(datum_index player_index)
{
    halo::game::oddball_engine.player_new_life(player_index);
}

/**
 * C entry point for halo::game::OddballEngine::player_round_reset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46d310
 */
void game_engine_oddball_player_round_reset(datum_index player_index)
{
    halo::game::oddball_engine.player_round_reset(player_index);
}

/**
 * C entry point for halo::game::OddballEngine::profile_post_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46d1d0
 */
void game_engine_oddball_profile_post_update(void **context)
{
    halo::game::oddball_engine.profile_post_update(context);
}

/**
 * C entry point for halo::game::OddballEngine::query_player_score; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46d370
 */
uint8_t game_engine_oddball_query_player_score(int32_t key, int32_t index, void *buffer)
{
    return halo::game::oddball_engine.query_player_score(key, index, buffer);
}

/**
 * C entry point for halo::game::OddballEngine::query_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46d420
 */
uint8_t game_engine_oddball_query_team_score(int32_t key, int32_t team, void *buffer)
{
    return halo::game::oddball_engine.query_team_score(key, team, buffer);
}

/**
 * C entry point for halo::game::OddballEngine::reset_objects; forwards to the C++ implementation unchanged.
 *
 * @address 0x46d450
 */
void game_engine_oddball_reset_objects(void)
{
    halo::game::oddball_engine.reset_objects();
}

/**
 * C entry point for halo::game::OddballEngine::time_scale_override; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46cf20
 */
uint8_t game_engine_oddball_time_scale_override(uint32_t player, int32_t value)
{
    return halo::game::oddball_engine.time_scale_override(player, value);
}

/**
 * C entry point for halo::game::OddballEngine::unknown_48; forwards to the C++ implementation unchanged.
 *
 * @address 0x46c750
 */
void game_engine_oddball_unknown_48(void)
{
    halo::game::oddball_engine.unknown_48();
}

/**
 * C entry point for halo::game::OddballEngine::unknown_84; forwards to the C++ implementation unchanged.
 *
 * @address 0x46cf00
 */
uint8_t game_engine_oddball_unknown_84(int32_t kind)
{
    return halo::game::oddball_engine.unknown_84(kind);
}

/**
 * C entry point for halo::game::RaceEngine::allow_grenade_counts; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46e980
 */
uint8_t game_engine_race_allow_grenade_counts(datum_index player_index)
{
    return halo::game::race_engine.allow_grenade_counts(player_index);
}

/**
 * C entry point for halo::game::RaceEngine::build_message_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x46e480
 */
uint8_t game_engine_race_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    return halo::game::race_engine.build_message_text(recipient, message_type, subject, text, count);
}

/**
 * C entry point for halo::game::RaceEngine::build_player_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x46eab0
 */
wchar_t * game_engine_race_build_player_text(datum_index player, wchar_t *buffer)
{
    return halo::game::race_engine.build_player_text(player, buffer);
}

/**
 * C entry point for halo::game::RaceEngine::build_score_header_text; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46eaf0
 */
wchar_t * game_engine_race_build_score_header_text(wchar_t *buffer)
{
    return halo::game::race_engine.build_score_header_text(buffer);
}

/**
 * C entry point for halo::game::RaceEngine::build_team_score_text; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46eb50
 */
wchar_t * game_engine_race_build_team_score_text(int32_t team, wchar_t *buffer)
{
    return halo::game::race_engine.build_team_score_text(team, buffer);
}

/**
 * C entry point for halo::game::RaceEngine::get_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46ea00
 */
int32_t game_engine_race_get_score(datum_index player, int32_t team_mode)
{
    return halo::game::race_engine.get_score(player, team_mode);
}

/**
 * C entry point for halo::game::RaceEngine::get_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46eaa0
 */
int32_t game_engine_race_get_team_score(int32_t team)
{
    return halo::game::race_engine.get_team_score(team);
}

/**
 * C entry point for halo::game::RaceEngine::is_winner; forwards to the C++ implementation unchanged.
 *
 * @address 0x46eb80
 */
uint32_t game_engine_race_is_winner(datum_index player)
{
    return halo::game::race_engine.is_winner(player);
}

/**
 * C entry point for halo::game::RaceEngine::player_changed_object; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46db00
 */
void game_engine_race_player_changed_object(datum_index player_index)
{
    halo::game::race_engine.player_changed_object(player_index);
}

/**
 * C entry point for halo::game::RaceEngine::player_new_life; forwards to the C++ implementation unchanged.
 *
 * @address 0x46dab0
 */
void game_engine_race_player_new_life(datum_index player)
{
    halo::game::race_engine.player_new_life(player);
}

/**
 * C entry point for halo::game::RaceEngine::player_round_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x46ee60
 */
void game_engine_race_player_round_reset(datum_index player_index, uint8_t team_flag)
{
    halo::game::race_engine.player_round_reset(player_index, team_flag);
}

/**
 * C entry point for halo::game::RaceEngine::profile_post_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x46ed30
 */
void game_engine_race_profile_post_update(void **context)
{
    halo::game::race_engine.profile_post_update(context);
}

/**
 * C entry point for halo::game::RaceEngine::query_player_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46ef30
 */
uint8_t game_engine_race_query_player_score(int32_t key, int32_t index, void *buffer)
{
    return halo::game::race_engine.query_player_score(key, index, buffer);
}

/**
 * C entry point for halo::game::RaceEngine::query_team_score; forwards to the C++ implementation unchanged.
 *
 * @address 0x46efb0
 */
uint8_t game_engine_race_query_team_score(int32_t key, int32_t team, void *buffer)
{
    return halo::game::race_engine.query_team_score(key, team, buffer);
}

/**
 * C entry point for halo::game::RaceEngine::unknown_48; forwards to the C++ implementation unchanged.
 *
 * @address 0x46e400
 */
void game_engine_race_unknown_48(void)
{
    halo::game::race_engine.unknown_48();
}

/**
 * C entry point for halo::game::RaceEngine::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x46e160
 */
void game_engine_race_update(datum_index player_index)
{
    halo::game::race_engine.update(player_index);
}

/**
 * C entry point for halo::game::RaceEngine::waypoint_filter; forwards to the C++ implementation unchanged.
 *
 * @address 0x46e9d0
 */
uint8_t game_engine_race_waypoint_filter(datum_index player, int32_t team)
{
    return halo::game::race_engine.waypoint_filter(player, team);
}

/**
 * C entry point for halo::game::VariantDefaults::assault; forwards to the C++ implementation unchanged.
 *
 * @address 0x467270
 */
game_variant * game_engine_variant_defaults_assault(game_variant *variant_options)
{
    return halo::game::VariantDefaults::assault(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_accumulation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464600
 */
game_variant * game_engine_variant_defaults_classic_accumulation(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_accumulation(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_crazy_king; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464aa0
 */
game_variant * game_engine_variant_defaults_classic_crazy_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_crazy_king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_ctf; forwards to the C++ implementation unchanged.
 *
 * @address 0x464c40
 */
game_variant * game_engine_variant_defaults_classic_ctf(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_ctf(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_ctf_pro; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464d30
 */
game_variant * game_engine_variant_defaults_classic_ctf_pro(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_ctf_pro(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_elimination; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463e00
 */
game_variant * game_engine_variant_defaults_classic_elimination(game_variant *out)
{
    return halo::game::VariantDefaults::classic_elimination(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_endurance; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463fc0
 */
game_variant * game_engine_variant_defaults_classic_endurance(game_variant *out)
{
    return halo::game::VariantDefaults::classic_endurance(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_invasion; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464e20
 */
game_variant * game_engine_variant_defaults_classic_invasion(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_invasion(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_iron_ctf; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464f10
 */
game_variant * game_engine_variant_defaults_classic_iron_ctf(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_iron_ctf(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_juggernaut; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464700
 */
game_variant * game_engine_variant_defaults_classic_juggernaut(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_juggernaut(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_king; forwards to the C++ implementation unchanged.
 *
 * @address 0x464900
 */
game_variant * game_engine_variant_defaults_classic_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_king_pro; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4649d0
 */
game_variant * game_engine_variant_defaults_classic_king_pro(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_king_pro(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_oddball; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464340
 */
game_variant * game_engine_variant_defaults_classic_oddball(game_variant *out)
{
    return halo::game::VariantDefaults::classic_oddball(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_phantoms; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463ee0
 */
game_variant * game_engine_variant_defaults_classic_phantoms(game_variant *out)
{
    return halo::game::VariantDefaults::classic_phantoms(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_race; forwards to the C++ implementation unchanged.
 *
 * @address 0x465000
 */
game_variant * game_engine_variant_defaults_classic_race(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_race(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_rally; forwards to the C++ implementation unchanged.
 *
 * @address 0x4650e0
 */
game_variant * game_engine_variant_defaults_classic_rally(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_rally(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_reverse_tag; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464510
 */
game_variant * game_engine_variant_defaults_classic_reverse_tag(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_reverse_tag(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_rockets; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4640a0
 */
game_variant * game_engine_variant_defaults_classic_rockets(game_variant *out)
{
    return halo::game::VariantDefaults::classic_rockets(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_slayer; forwards to the C++ implementation unchanged.
 *
 * @address 0x463c40
 */
game_variant * game_engine_variant_defaults_classic_slayer(game_variant *out)
{
    return halo::game::VariantDefaults::classic_slayer(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_slayer_pro; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463d20
 */
game_variant * game_engine_variant_defaults_classic_slayer_pro(game_variant *out)
{
    return halo::game::VariantDefaults::classic_slayer_pro(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_snipers; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464180
 */
game_variant * game_engine_variant_defaults_classic_snipers(game_variant *out)
{
    return halo::game::VariantDefaults::classic_snipers(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_stalker; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464800
 */
game_variant * game_engine_variant_defaults_classic_stalker(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_stalker(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_team_king; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464b70
 */
game_variant * game_engine_variant_defaults_classic_team_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_team_king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_team_oddball; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464430
 */
game_variant * game_engine_variant_defaults_classic_team_oddball(game_variant *out)
{
    return halo::game::VariantDefaults::classic_team_oddball(out);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_team_race; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4651c0
 */
game_variant * game_engine_variant_defaults_classic_team_race(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_team_race(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_team_rally; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4652a0
 */
game_variant * game_engine_variant_defaults_classic_team_rally(game_variant *variant_options)
{
    return halo::game::VariantDefaults::classic_team_rally(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::classic_team_slayer; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x464260
 */
game_variant * game_engine_variant_defaults_classic_team_slayer(game_variant *out)
{
    return halo::game::VariantDefaults::classic_team_slayer(out);
}

/**
 * C entry point for halo::game::VariantDefaults::crazy_king; forwards to the C++ implementation unchanged.
 *
 * @address 0x467370
 */
game_variant * game_engine_variant_defaults_crazy_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::crazy_king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::juggernaut; forwards to the C++ implementation unchanged.
 *
 * @address 0x467540
 */
game_variant * game_engine_variant_defaults_juggernaut(game_variant *variant_options)
{
    return halo::game::VariantDefaults::juggernaut(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::king; forwards to the C++ implementation unchanged.
 *
 * @address 0x467650
 */
game_variant * game_engine_variant_defaults_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::oddball; forwards to the C++ implementation unchanged.
 *
 * @address 0x467730
 */
game_variant * game_engine_variant_defaults_oddball(game_variant *variant_options)
{
    return halo::game::VariantDefaults::oddball(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::race; forwards to the C++ implementation unchanged.
 *
 * @address 0x467830
 */
game_variant * game_engine_variant_defaults_race(game_variant *variant_options)
{
    return halo::game::VariantDefaults::race(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::slayer; forwards to the C++ implementation unchanged.
 *
 * @address 0x467920
 */
game_variant * game_engine_variant_defaults_slayer(game_variant *variant_options)
{
    return halo::game::VariantDefaults::slayer(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::stalker; forwards to the C++ implementation unchanged.
 *
 * @address 0x467450
 */
game_variant * game_engine_variant_defaults_stalker(game_variant *variant_options)
{
    return halo::game::VariantDefaults::stalker(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::team_king; forwards to the C++ implementation unchanged.
 *
 * @address 0x467a10
 */
game_variant * game_engine_variant_defaults_team_king(game_variant *variant_options)
{
    return halo::game::VariantDefaults::team_king(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::team_oddball; forwards to the C++ implementation unchanged.
 *
 * @address 0x467af0
 */
game_variant * game_engine_variant_defaults_team_oddball(game_variant *variant_options)
{
    return halo::game::VariantDefaults::team_oddball(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::team_race; forwards to the C++ implementation unchanged.
 *
 * @address 0x467c00
 */
game_variant * game_engine_variant_defaults_team_race(game_variant *variant_options)
{
    return halo::game::VariantDefaults::team_race(variant_options);
}

/**
 * C entry point for halo::game::VariantDefaults::team_slayer; forwards to the C++ implementation unchanged.
 *
 * @address 0x467cf0
 */
game_variant * game_engine_variant_defaults_team_slayer(game_variant *variant_options)
{
    return halo::game::VariantDefaults::team_slayer(variant_options);
}

/**
 * C entry point for halo::game::GameVariantRules::set_variant_by_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45b920
 */
void game_engine_set_variant_by_name(const char *name)
{
    halo::game::GameVariantRules::set_variant_by_name(name);
}

/**
 * C entry point for halo::game::GameVariantRules::sync_variant_defaults; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45fc80
 */
void game_engine_sync_variant_defaults(void)
{
    halo::game::GameVariantRules::sync_variant_defaults();
}

/**
 * C entry point for halo::game::GameVariantRules::variant_add_to_history; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463980
 */
uint32_t game_engine_variant_add_to_history(char *name, game_variant *options, char *path)
{
    return halo::game::GameVariantRules::variant_add_to_history(name, options, path);
}

/**
 * C entry point for halo::game::GameVariantRules::variant_option_default_by_index; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x465380
 */
uint32_t game_variant_option_default_by_index(uint32_t selector)
{
    return halo::game::GameVariantRules::variant_option_default_by_index(selector);
}

/**
 * C entry point for halo::game::GameVariantRules::variant_sanitize_options; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x466730
 */
void game_variant_sanitize_options(game_variant *variant)
{
    halo::game::GameVariantRules::variant_sanitize_options(variant);
}

/**
 * C entry point for halo::game::EnginePlayers::player_changed_object; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45c570
 */
void game_engine_player_changed_object(uint32_t param)
{
    halo::game::EnginePlayers::player_changed_object(param);
}

/**
 * C entry point for halo::game::EnginePlayers::player_has_respawn_priority; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x460e40
 */
uint8_t game_engine_player_has_respawn_priority(uint32_t player_index)
{
    return halo::game::EnginePlayers::player_has_respawn_priority(player_index);
}

/**
 * C entry point for halo::game::EnginePlayers::player_is_eliminated; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x460f30
 */
uint8_t game_engine_player_is_eliminated(uint32_t player_index)
{
    return halo::game::EnginePlayers::player_is_eliminated(player_index);
}

/**
 * C entry point for halo::game::EnginePlayers::player_new_life; forwards to the C++ implementation unchanged.
 *
 * @address 0x45c440
 */
void game_engine_player_new_life(uint32_t player_handle)
{
    halo::game::EnginePlayers::player_new_life(player_handle);
}

/**
 * C entry point for halo::game::EnginePlayers::player_ready_to_respawn; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x460f70
 */
uint8_t game_engine_player_ready_to_respawn(uint32_t player_index)
{
    return halo::game::EnginePlayers::player_ready_to_respawn(player_index);
}

/**
 * C entry point for halo::game::EnginePlayers::player_respawn_priority_gate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x463100
 */
uint8_t game_engine_player_respawn_priority_gate(uint32_t player_index)
{
    return halo::game::EnginePlayers::player_respawn_priority_gate(player_index);
}

/**
 * C entry point for halo::game::EnginePlayers::player_round_reset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x463620
 */
void game_engine_player_round_reset(int32_t player_handle, int32_t callback_argument)
{
    halo::game::EnginePlayers::player_round_reset(player_handle, callback_argument);
}

/**
 * C entry point for halo::game::EnginePlayers::player_select_random_target; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46f1a0
 */
void game_engine_player_select_random_target(datum_index player_or_all)
{
    halo::game::EnginePlayers::player_select_random_target(player_or_all);
}

/**
 * C entry point for halo::game::EnginePlayers::players_ready_for_bsp_switch; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x45c750
 */
uint8_t game_engine_players_ready_for_bsp_switch(void)
{
    return halo::game::EnginePlayers::players_ready_for_bsp_switch();
}

/**
 * C entry point for halo::game::EnginePlayers::players_ready_for_bsp_switch_strict; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x45c830
 */
uint8_t game_engine_players_ready_for_bsp_switch_strict(void)
{
    return halo::game::EnginePlayers::players_ready_for_bsp_switch_strict();
}

/**
 * C entry point for halo::game::EnginePlayers::reset_all_players; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45b8b0
 */
void game_engine_reset_all_players(void)
{
    halo::game::EnginePlayers::reset_all_players();
}

/**
 * C entry point for halo::game::EnginePlayers::reset_player_look_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x470de0
 */
void game_engine_reset_player_look_state(void)
{
    halo::game::EnginePlayers::reset_player_look_state();
}

/**
 * C entry point for halo::game::EnginePlayers::reset_player_profile_stats; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x468150
 */
void game_engine_reset_player_profile_stats(void)
{
    halo::game::EnginePlayers::reset_player_profile_stats();
}

/**
 * C entry point for halo::game::EnginePlayers::reset_respawns_and_cleanup_bipeds; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x467e60
 */
void game_engine_reset_respawns_and_cleanup_bipeds(void)
{
    halo::game::EnginePlayers::reset_respawns_and_cleanup_bipeds();
}

/**
 * C entry point for halo::game::EnginePlayers::resolve_player_team; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4611b0
 */
void game_engine_resolve_player_team(uint32_t player_index)
{
    halo::game::EnginePlayers::resolve_player_team(player_index);
}

/**
 * C entry point for halo::game::EnginePlayers::reattach_player_unit_unused; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x475270
 */
void game_engine_reattach_player_unit_unused(uint32_t player_index, uint32_t target_object, void *local_offset)
{
    halo::game::EnginePlayers::reattach_player_unit_unused(player_index, target_object, local_offset);
}

/**
 * C entry point for halo::game::EnginePlayers::pick_random_recent_location; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46a1b0
 */
int32_t game_engine_pick_random_recent_location(int32_t exclude_value, int32_t fallback)
{
    return halo::game::EnginePlayers::pick_random_recent_location(exclude_value, fallback);
}

/**
 * C entry point for halo::game::EnginePlayers::reset_all_unit_grenade_counts; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x467de0
 */
void game_engine_reset_all_unit_grenade_counts(void)
{
    halo::game::EnginePlayers::reset_all_unit_grenade_counts();
}

/**
 * C entry point for halo::game::EnginePlayers::team_has_scoring_capacity; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46e250
 */
uint8_t game_engine_team_has_scoring_capacity(int32_t team)
{
    return halo::game::EnginePlayers::team_has_scoring_capacity(team);
}

/**
 * C entry point for halo::game::EnginePlayers::scores_tracked_individually; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4635e0
 */
uint8_t game_engine_scores_tracked_individually(void)
{
    return halo::game::EnginePlayers::scores_tracked_individually();
}

/**
 * C entry point for halo::game::EnginePlayerSync::players_update_client; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x474590
 */
void game_engine_players_update_client(void)
{
    halo::game::EnginePlayerSync::players_update_client();
}

/**
 * C entry point for halo::game::EnginePlayerSync::players_update_server; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4740a0
 */
void game_engine_players_update_server(void)
{
    halo::game::EnginePlayerSync::players_update_server();
}

/**
 * C entry point for halo::game::EnginePlayerSync::server_update_player_positions; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x476760
 */
void game_engine_server_update_player_positions(void)
{
    halo::game::EnginePlayerSync::server_update_player_positions();
}

/**
 * C entry point for halo::game::EnginePlayerSync::send_player_profile_update; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x467010
 */
void game_engine_send_player_profile_update(void *has_payload, void *profile_tail, int32_t target)
{
    halo::game::EnginePlayerSync::send_player_profile_update(has_payload, profile_tail, target);
}

/**
 * C entry point for halo::game::EnginePlayerSync::send_unit_weapon_loadout; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x477a80
 */
void game_engine_send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle, int32_t value, int32_t machine_index)
{
    halo::game::EnginePlayerSync::send_unit_weapon_loadout(unit_index, player_handle, value, machine_index);
}

/**
 * C entry point for halo::game::EnginePlayerSync::update_local_player_control; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x471ae0
 */
void game_engine_update_local_player_control(int16_t local_player_index, real delta_time, int32_t ticks_this_frame)
{
    halo::game::EnginePlayerSync::update_local_player_control(local_player_index, delta_time, ticks_this_frame);
}

/**
 * C entry point for halo::game::EnginePlayerSync::update_local_player_look; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x472160
 */
void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta)
{
    halo::game::EnginePlayerSync::update_local_player_look(local_player_index, yaw_delta, pitch_delta);
}

/**
 * C entry point for halo::game::EnginePlayerSync::player_profile_cache_add; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x466c60
 */
void game_engine_player_profile_cache_add(datum_index player_handle)
{
    halo::game::EnginePlayerSync::player_profile_cache_add(player_handle);
}

/**
 * C entry point for halo::game::EnginePlayerSync::player_profile_cache_find; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x466e80
 */
int32_t game_engine_player_profile_cache_find(datum_index player_handle)
{
    return halo::game::EnginePlayerSync::player_profile_cache_find(player_handle);
}

/**
 * C entry point for halo::game::EnginePlayerSync::player_profile_cache_sync_all; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x466cb0
 */
void game_engine_player_profile_cache_sync_all(int32_t commit, void *callback_extra_arg)
{
    halo::game::EnginePlayerSync::player_profile_cache_sync_all(commit, callback_extra_arg);
}

/**
 * C entry point for halo::game::EnginePlayerSync::spawn_player_starting_loadout; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4611f0
 */
void game_engine_spawn_player_starting_loadout(uint32_t starting_equipment_index, int32_t *frag_count, int32_t *plasma_count)
{
    halo::game::EnginePlayerSync::spawn_player_starting_loadout(starting_equipment_index, frag_count, plasma_count);
}

/**
 * C entry point for halo::game::EnginePlacement::rate_location_ally_bonus; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x461c60
 */
float game_engine_rate_location_ally_bonus(uint32_t self_index, real_point3d *point)
{
    return halo::game::EnginePlacement::rate_location_ally_bonus(self_index, point);
}

/**
 * C entry point for halo::game::EnginePlacement::rate_location_crowding; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x461ad0
 */
float game_engine_rate_location_crowding(uint32_t self_index, real_point3d *point)
{
    return halo::game::EnginePlacement::rate_location_crowding(self_index, point);
}

/**
 * C entry point for halo::game::EnginePlacement::rate_player_starting_location; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x461d90
 */
real game_engine_rate_player_starting_location(ScenarioPlayerStartingLocation *location, datum_index player_handle)
{
    return halo::game::EnginePlacement::rate_player_starting_location(location, player_handle);
}

/**
 * C entry point for halo::game::EnginePlacement::remap_placement_by_type; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4630b0
 */
uint32_t game_engine_remap_placement_by_type(uint32_t handle)
{
    return halo::game::EnginePlacement::remap_placement_by_type(handle);
}

/**
 * C entry point for halo::game::EnginePlacement::resolve_multiplayer_placement; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x462c30
 */
uint32_t game_engine_resolve_multiplayer_placement(uint32_t handle)
{
    return halo::game::EnginePlacement::resolve_multiplayer_placement(handle);
}

/**
 * C entry point for halo::game::EnginePlacement::resolve_netgame_flag_role; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x462df0
 */
int32_t game_engine_resolve_netgame_flag_role(uint32_t handle)
{
    return halo::game::EnginePlacement::resolve_netgame_flag_role(handle);
}

/**
 * C entry point for halo::game::EnginePlacement::scan_netgame_flags_noop; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4637c0
 */
void game_engine_scan_netgame_flags_noop(int16_t needle)
{
    halo::game::EnginePlacement::scan_netgame_flags_noop(needle);
}

/**
 * C entry point for halo::game::EnginePlacement::spawn_or_replay_netgame_equipment; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x45f8f0
 */
void game_engine_spawn_or_replay_netgame_equipment(int32_t *message)
{
    halo::game::EnginePlacement::spawn_or_replay_netgame_equipment(message);
}

/**
 * C entry point for halo::game::EnginePlacement::update_netgame_equipment; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45f9f0
 */
void game_engine_update_netgame_equipment(char force_respawn)
{
    halo::game::EnginePlacement::update_netgame_equipment(force_respawn);
}

/**
 * C entry point for halo::game::EnginePlacement::update_teleporter; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x461630
 */
void game_engine_update_teleporter(uint32_t player_index)
{
    halo::game::EnginePlacement::update_teleporter(player_index);
}

/**
 * C entry point for halo::game::EnginePlacement::validate_scenario_placements_noop; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x463810
 */
void game_engine_validate_scenario_placements_noop(void)
{
    halo::game::EnginePlacement::validate_scenario_placements_noop();
}

/**
 * C entry point for halo::game::EnginePlacement::touch_multiplayer_predicted_resources; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x466890
 */
void game_engine_touch_multiplayer_predicted_resources(void)
{
    halo::game::EnginePlacement::touch_multiplayer_predicted_resources();
}

/**
 * C entry point for halo::game::EnginePlacement::update_item_scale_and_pickup; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x45f560
 */
void game_engine_update_item_scale_and_pickup(void)
{
    halo::game::EnginePlacement::update_item_scale_and_pickup();
}

/**
 * C entry point for halo::game::EnginePlacement::reset_round_objects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x468260
 */
void game_engine_reset_round_objects(void)
{
    halo::game::EnginePlacement::reset_round_objects();
}

/**
 * C entry point for halo::game::EnginePlacement::reset_vehicles_or_race_cleanup; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4681a0
 */
void game_engine_reset_vehicles_or_race_cleanup(void)
{
    halo::game::EnginePlacement::reset_vehicles_or_race_cleanup();
}

/**
 * C entry point for halo::game::EnginePlacement::pack_object_flags_or_passthrough; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x462bd0
 */
uint32_t game_engine_pack_object_flags_or_passthrough(uint32_t input)
{
    return halo::game::EnginePlacement::pack_object_flags_or_passthrough(input);
}

/**
 * C entry point for halo::game::EngineHud::rasterize_in_game_score; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x465690
 */
void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity)
{
    halo::game::EngineHud::rasterize_in_game_score(subject_player, opacity);
}

/**
 * C entry point for halo::game::EngineHud::rasterize_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x462a80
 */


/**
 * C entry point for halo::game::EngineHud::post_rasterize_post_game; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45d700
 */
void game_engine_post_rasterize_post_game(void)
{
    halo::game::EngineHud::post_rasterize_post_game();
}

/**
 * C entry point for halo::game::EngineHud::pick_hud_hint; forwards to the C++ implementation unchanged.
 *
 * @address 0x463150
 */
uint8_t game_engine_pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text)
{
    return halo::game::EngineHud::pick_hud_hint(player_index, maximum_length, out_text);
}

/**
 * C entry point for halo::game::EngineHud::play_multiplayer_sound; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46bd00
 */
void game_engine_play_multiplayer_sound(int32_t sound_index, datum_index recipient_player, uint8_t broadcast)
{
    halo::game::EngineHud::play_multiplayer_sound(sound_index, recipient_player, broadcast);
}

/**
 * C entry point for halo::game::EngineHud::queue_multiplayer_sound; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46be40
 */
void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast)
{
    halo::game::EngineHud::queue_multiplayer_sound(sound_index, player, broadcast);
}

/**
 * C entry point for halo::game::EngineHud::queue_status_sound_message; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x46bbd0
 */
void game_engine_queue_status_sound_message(int32_t sound_index, datum_index recipient_player)
{
    halo::game::EngineHud::queue_status_sound_message(sound_index, recipient_player);
}

/**
 * C entry point for halo::game::EngineHud::update_custom_waypoint_navpoints; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x462a90
 */
void game_engine_update_custom_waypoint_navpoints(int16_t local_player_slot)
{
    halo::game::EngineHud::update_custom_waypoint_navpoints(local_player_slot);
}

/**
 * C entry point for halo::game::EngineMatch::on_player_death; forwards to the C++ implementation unchanged.
 *
 * @address 0x460200
 */
void game_engine_on_player_death(datum_index killer, datum_index death_object, datum_index victim, char is_suicide)
{
    halo::game::EngineMatch::on_player_death(killer, death_object, victim, is_suicide);
}

/**
 * C entry point for halo::game::EngineMatch::tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x45ff30
 */
void game_engine_tick(void)
{
    halo::game::EngineMatch::tick();
}

/**
 * C entry point for halo::game::EngineMatch::unload; forwards to the C++ implementation unchanged.
 *
 * @address 0x45c330
 */
void __cdecl game_engine_unload(void)
{
    return halo::game::EngineMatch::unload();
}

/**
 * C entry point for halo::game::EngineMatch::send_end_game_notification; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4671d0
 */
void game_engine_send_end_game_notification(uint32_t reason)
{
    halo::game::EngineMatch::send_end_game_notification(reason);
}

/**
 * C entry point for halo::game::EngineMatch::send_round_reset_message; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4682c0
 */
void game_engine_send_round_reset_message(void)
{
    halo::game::EngineMatch::send_round_reset_message();
}

/**
 * C entry point for halo::game::EngineMatch::send_team_allegiance_message; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4704d0
 */
void game_engine_send_team_allegiance_message(char broadcast)
{
    halo::game::EngineMatch::send_team_allegiance_message(broadcast);
}

/**
 * C entry point for halo::game::EngineMatch::team_close_game_check; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x470790
 */
uint8_t game_engine_team_close_game_check(int32_t side, int32_t filter_value)
{
    return halo::game::EngineMatch::team_close_game_check(side, filter_value);
}

/**
 * C entry point for halo::game::EngineMatch::team_is_leading; forwards to the C++ implementation unchanged.
 *
 * @address 0x470720
 */
uint8_t game_engine_team_is_leading(int32_t filter_value)
{
    return halo::game::EngineMatch::team_is_leading(filter_value);
}

/**
 * C entry point for halo::game::EngineMatch::update_end_game_sequence; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45fdf0
 */
void game_engine_update_end_game_sequence(float delta_time)
{
    halo::game::EngineMatch::update_end_game_sequence(delta_time);
}

/**
 * C entry point for halo::game::EngineMatch::update_lead_change_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x470810
 */
void game_engine_update_lead_change_state(void **envelope, uint8_t *message)
{
    halo::game::EngineMatch::update_lead_change_state(envelope, message);
}

/**
 * C entry point for halo::game::GameLifecycle::get_player_starting_location; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x477640
 */
ScenarioPlayerStartingLocation * game_get_player_starting_location(int16_t index)
{
    return halo::game::GameLifecycle::get_player_starting_location(index);
}

/**
 * C entry point for halo::game::GameLifecycle::initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x45a9c0
 */
void game_initialize(void)
{
    halo::game::GameLifecycle::initialize();
}

/**
 * C entry point for halo::game::GameLifecycle::no_player_is_dead; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x45bbe0
 */
uint32_t game_no_player_is_dead(void)
{
    return halo::game::GameLifecycle::no_player_is_dead();
}

/**
 * C entry point for halo::game::GameLifecycle::safe_to_pause; forwards to the C++ implementation unchanged.
 *
 * @address 0x45b9e0
 */
uint32_t game_safe_to_pause(void)
{
    return halo::game::GameLifecycle::safe_to_pause();
}

/**
 * C entry point for halo::game::GameLifecycle::safe_to_save; forwards to the C++ implementation unchanged.
 *
 * @address 0x45ba50
 */
uint8_t game_safe_to_save(void)
{
    return halo::game::GameLifecycle::safe_to_save();
}

/**
 * C entry point for halo::game::GameLifecycle::set_local_player; forwards to the C++ implementation unchanged.
 *
 * @address 0x474d50
 */
void game_set_local_player(datum_index player_handle, int16_t local_player_index)
{
    halo::game::GameLifecycle::set_local_player(player_handle, local_player_index);
}

/**
 * C entry point for halo::game::GameLifecycle::simulate_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x45b780
 */
void game_simulate_tick(uint32_t predict_pass)
{
    halo::game::GameLifecycle::simulate_tick(predict_pass);
}

/**
 * C entry point for halo::game::GameLifecycle::start_new_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x45b050
 */
void game_start_new_map(void)
{
    halo::game::GameLifecycle::start_new_map();
}

/**
 * C entry point for halo::game::GameLifecycle::stop_current_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x45b370
 */
void game_stop_current_map(void)
{
    halo::game::GameLifecycle::stop_current_map();
}

/**
 * C entry point for halo::game::GameLifecycle::time_format_minutes_seconds; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x466530
 */
void game_time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest)
{
    halo::game::GameLifecycle::time_format_minutes_seconds(ticks, count, dest);
}

/**
 * C entry point for halo::game::GameLifecycle::time_format_minutes_seconds_ascii; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x466600
 */
void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest)
{
    halo::game::GameLifecycle::time_format_minutes_seconds_ascii(ticks, count, dest);
}

/**
 * C entry point for halo::game::GameLifecycle::unload_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x45afb0
 */
void game_unload_map(void)
{
    halo::game::GameLifecycle::unload_map();
}

}
