/**
 * Entry functions for the game module: one function per original symbol, forwarding to the
 * C++ classes in namespace halo::game::engine1.
 */

#include "tags.h"
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
#include "crt.h"
#include "win32.h"
#include "interface.h"

#include "halo/game/game1_ctf.hpp"
#include "halo/game/game1_king.hpp"
#include "halo/game/game1_koth.hpp"
#include "halo/game/game1_oddball.hpp"
#include "halo/game/game1_kill_feed.hpp"
#include "halo/game/game1_scoreboard.hpp"
#include "halo/game/game1_clock.hpp"
#include "halo/game/game1_variants.hpp"
#include "halo/game/game1_lifecycle.hpp"
#include "halo/game/game1_notifications.hpp"
#include "halo/game/game1_local_control.hpp"
#include "halo/game/game1_spawn.hpp"
#include "halo/game/game1_cleanup.hpp"
#include "halo/game/game1_engine_behavior.hpp"
#include "halo/game/api.hpp"

namespace halo::game {

void game_engine_ctf_assign_flag_ids(void)
{
    halo::game::engine1::Ctf::assign_flag_ids();
}

void game_engine_ctf_broadcast_state(void *request_fields, int32_t machine_index)
{
    halo::game::engine1::Ctf::broadcast_state(request_fields, machine_index);
}

uint8_t game_engine_ctf_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::ctf)->build_message_text(recipient, message_type, subject, text, count);
}

wchar_t *game_engine_ctf_build_player_text(datum_index player, wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::ctf)->build_player_text(player, buffer);
}

wchar_t *game_engine_ctf_build_score_header_text(wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::ctf)->build_score_header_text(buffer);
}

wchar_t *game_engine_ctf_build_team_score_text(int32_t team, wchar_t *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::ctf)->build_team_score_text(team, buffer);
}

datum_index game_engine_ctf_create_flag_object(real_point3d *position, uint16_t name_index)
{
    return halo::game::engine1::Ctf::create_flag_object(position, name_index);
}

int32_t game_engine_ctf_get_score(datum_index player, int32_t team_mode)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::ctf)->get_score(player, team_mode);
}

int32_t game_engine_ctf_get_team_score(int32_t team)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::ctf)->get_team_score(team);
}

int32_t game_engine_ctf_initialize_flags(void)
{
    return halo::game::engine1::Ctf::initialize_flags();
}

uint8_t game_engine_ctf_initialize_for_new_game(void)
{
    return halo::game::engine1::Ctf::initialize_for_new_game();
}

uint8_t game_engine_ctf_is_flag_eligible_for_capture(uint32_t team, int32_t flag_id)
{
    return halo::game::engine1::Ctf::is_flag_eligible_for_capture(team, flag_id);
}

void game_engine_ctf_notify_both_teams(int32_t team)
{
    halo::game::engine1::Ctf::notify_both_teams(team);
}

void game_engine_ctf_notify_flag_carried_throttled(int32_t target_player)
{
    halo::game::engine1::Ctf::notify_flag_carried_throttled(target_player);
}

void game_engine_ctf_object_expired(datum_index object_index)
{
    halo::game::engine1::Ctf::object_expired(object_index);
}

void game_engine_ctf_on_flag_captured(uint32_t flag_index)
{
    halo::game::engine1::Ctf::on_flag_captured(flag_index);
}

int32_t game_engine_ctf_pick_random_flag(int32_t exclude_flag_index)
{
    return halo::game::engine1::Ctf::pick_random_flag(exclude_flag_index);
}

void game_engine_ctf_player_drop_flag(uint32_t player_index, datum_index flag_object_index)
{
    halo::game::engine1::Ctf::player_drop_flag(player_index, flag_object_index);
}

uint8_t game_engine_ctf_player_flag_tick(uint32_t flag_handle, uint32_t player_index)
{
    return halo::game::engine1::Ctf::player_flag_tick(flag_handle, player_index);
}

void game_engine_ctf_player_round_reset(datum_index player_index)
{
    halo::game::engine1::Ctf::player_round_reset(player_index);
}

void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team)
{
    halo::game::engine1::Ctf::player_touch_flag(player_index, team);
}

uint8_t game_engine_ctf_point_within_team_flag_radius(float radius, int32_t team, real_point3d *point)
{
    return halo::game::engine1::Ctf::point_within_team_flag_radius(radius, team, point);
}

void game_engine_ctf_profile_post_update(void **context)
{
    halo::game::engine1::Ctf::profile_post_update(context);
}

void game_engine_ctf_profiles_updated(int32_t mode, int32_t machine_index)
{
    halo::game::engine1::Ctf::profiles_updated(mode, machine_index);
}

uint8_t game_engine_ctf_query_player_score(int32_t key, int32_t index, void *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::ctf)->query_player_score(key, index, buffer);
}

uint8_t game_engine_ctf_query_team_score(int32_t key, int32_t team, void *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::ctf)->query_team_score(key, team, buffer);
}

void game_engine_ctf_reset_objects(void)
{
    halo::game::engine1::Ctf::reset_objects();
}

void game_engine_ctf_reset_round(void)
{
    halo::game::engine1::Ctf::reset_round();
}

void game_engine_ctf_reset_team_return_credit(uint32_t object_index)
{
    halo::game::engine1::Ctf::reset_team_return_credit(object_index);
}

void game_engine_ctf_respawn_team_flag(int32_t team, real_point3d *forwarded_position, uint16_t forwarded_name_index)
{
    halo::game::engine1::Ctf::respawn_team_flag(team, forwarded_position, forwarded_name_index);
}

void game_engine_ctf_return_all_flags(void)
{
    halo::game::engine1::Ctf::return_all_flags();
}

void game_engine_ctf_score_flag(uint32_t team, int32_t scenario_flag_index)
{
    halo::game::engine1::Ctf::score_flag(team, scenario_flag_index);
}

uint8_t game_engine_ctf_unit_is_flag_holder(player *p)
{
    return halo::game::engine1::Ctf::unit_is_flag_holder(p);
}

uint8_t game_engine_ctf_unit_weapon_must_be_readied(datum_index unit_handle)
{
    return halo::game::engine1::Ctf::unit_weapon_must_be_readied(unit_handle);
}

void game_engine_ctf_unknown_48(void)
{
    halo::game::engine1::Ctf::unknown_48();
}

uint8_t game_engine_ctf_unknown_60(datum_index unit_index, datum_index item_index)
{
    return halo::game::engine1::Ctf::unknown_60(unit_index, item_index);
}

float game_engine_ctf_unknown_70(datum_index player_index, real_point3d *position)
{
    return halo::game::engine1::Ctf::unknown_70(player_index, position);
}

uint8_t game_engine_ctf_unknown_84(int32_t kind)
{
    return halo::game::engine1::Ctf::unknown_84(kind);
}

void game_engine_ctf_update(datum_index player_index)
{
    halo::game::engine1::Ctf::update(player_index);
}

uint8_t game_engine_king_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::king)->build_message_text(recipient, message_type, subject, text, count);
}

wchar_t *game_engine_king_build_player_text(datum_index player, wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::king)->build_player_text(player, buffer);
}

wchar_t *game_engine_king_build_score_header_text(wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::king)->build_score_header_text(buffer);
}

wchar_t *game_engine_king_build_team_score_text(int32_t team, wchar_t *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::king)->build_team_score_text(team, buffer);
}

int32_t game_engine_king_get_score(datum_index player, int32_t team_mode)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::king)->get_score(player, team_mode);
}

int32_t game_engine_king_get_team_score(int32_t team)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::king)->get_team_score(team);
}

uint8_t game_engine_king_initialize_for_new_game(void)
{
    return halo::game::engine1::King::initialize_for_new_game();
}

void game_engine_king_player_new_life(datum_index player_index)
{
    halo::game::engine1::King::player_new_life(player_index);
}

void game_engine_king_player_round_reset(datum_index player_index)
{
    halo::game::engine1::King::player_round_reset(player_index);
}

void game_engine_king_profile_post_update(void **context)
{
    halo::game::engine1::King::profile_post_update(context);
}

uint8_t game_engine_king_query_player_score(int32_t key, int32_t index, void *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::king)->query_player_score(key, index, buffer);
}

uint8_t game_engine_king_query_team_score(int32_t key, int32_t team, void *buffer)
{
    return halo::game::engine1::engine_scoring(halo::game::engine1::EngineId::king)->query_team_score(key, team, buffer);
}

void game_engine_king_reset_objects(void)
{
    halo::game::engine1::King::reset_objects();
}

void game_engine_king_reset_round(void)
{
    halo::game::engine1::King::reset_round();
}

void game_engine_king_unknown_48(void)
{
    halo::game::engine1::King::unknown_48();
}

uint8_t game_engine_king_waypoint_filter(datum_index player)
{
    return halo::game::engine1::King::waypoint_filter(player);
}

void game_engine_animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player)
{
    halo::game::engine1::Koth::animate_hill_pulse_icons(fading_player, growing_player);
}

void game_engine_koth_alt_scorer_tick(uint32_t player_index)
{
    halo::game::engine1::Koth::alt_scorer_tick(player_index);
}

void game_engine_koth_ball_idle_tick(uint32_t object_handle, object *obj)
{
    halo::game::engine1::Koth::ball_idle_tick(object_handle, obj);
}

void game_engine_koth_broadcast_hill_times(int32_t mode, int32_t machine_index)
{
    halo::game::engine1::Koth::broadcast_hill_times(mode, machine_index);
}

void game_engine_koth_broadcast_team_scores(int32_t mode, int32_t machine_index)
{
    halo::game::engine1::Koth::broadcast_team_scores(mode, machine_index);
}

void game_engine_koth_build_hill_boundary(void)
{
    halo::game::engine1::Koth::build_hill_boundary();
}

void game_engine_koth_build_hill_boundary_fence(void)
{
    halo::game::engine1::Koth::build_hill_boundary_fence();
}

uint32_t game_engine_koth_dispatch_player_scoring(uint32_t player_index)
{
    return halo::game::engine1::Koth::dispatch_player_scoring(player_index);
}

void game_engine_koth_find_marker_position(real_point3d *out_position, int16_t type_filter)
{
    halo::game::engine1::Koth::find_marker_position(out_position, type_filter);
}

uint8_t game_engine_koth_player_eligible_to_score(uint32_t object_handle, uint32_t player_index)
{
    return halo::game::engine1::Koth::player_eligible_to_score(object_handle, player_index);
}

uint8_t game_engine_koth_player_in_hill_bounds(uint32_t player_index)
{
    return halo::game::engine1::Koth::player_in_hill_bounds(player_index);
}

void game_engine_koth_player_tick(uint32_t player_index)
{
    halo::game::engine1::Koth::player_tick(player_index);
}

void game_engine_koth_relocate_hill_marker(int32_t ball_index)
{
    halo::game::engine1::Koth::relocate_hill_marker(ball_index);
}

void game_engine_koth_relocate_object_hill(uint32_t object_index)
{
    halo::game::engine1::Koth::relocate_object_hill(object_index);
}

void game_engine_koth_reset_hill_marker_history(void)
{
    halo::game::engine1::Koth::reset_hill_marker_history();
}

void game_engine_koth_submit_hill_marker_geometry(uint32_t tag_handle_as_uint, uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4, uint32_t param_5, float *vertex_source)
{
    halo::game::engine1::Koth::submit_hill_marker_geometry(tag_handle_as_uint, position_override, orientation_override, param_4, param_5, vertex_source);
}

void game_engine_koth_update_hill_occupancy_state(void)
{
    halo::game::engine1::Koth::update_hill_occupancy_state();
}

void game_engine_koth_update_occupant_table(uint32_t index)
{
    halo::game::engine1::Koth::update_occupant_table(index);
}

uint8_t game_engine_oddball_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::oddball)->build_message_text(recipient, message_type, subject, text, count);
}

wchar_t *game_engine_oddball_build_player_text(datum_index player, wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::oddball)->build_player_text(player, buffer);
}

wchar_t *game_engine_oddball_build_score_header_text(wchar_t *buffer)
{
    return halo::game::engine1::engine_text(halo::game::engine1::EngineId::oddball)->build_score_header_text(buffer);
}

uint8_t game_engine_apply_kill_streak_message(int32_t **envelope)
{
    return halo::game::engine1::KillFeed::apply_kill_streak_message(envelope);
}

void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer, datum_index death_object, int32_t killer_team, char credit_kills)
{
    halo::game::engine1::KillFeed::attribute_player_death(victim_unit, killer, death_object, killer_team, credit_kills);
}

void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast)
{
    halo::game::engine1::KillFeed::broadcast_kill_feed_by_relationship(source_player, no_source_message, message_a, message_b, subject, broadcast);
}

void game_engine_broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t exclude_index, int32_t alternate_recipient, datum_index subject, char broadcast)
{
    halo::game::engine1::KillFeed::broadcast_kill_feed_gated(broadcast_enabled, exclude_index, alternate_recipient, subject, broadcast);
}

void game_engine_broadcast_kill_feed_or_direct(datum_index recipient_or_all, int32_t broadcast_enabled, char broadcast, int32_t hash_key, datum_index subject)
{
    halo::game::engine1::KillFeed::broadcast_kill_feed_or_direct(recipient_or_all, broadcast_enabled, broadcast, hash_key, subject);
}

void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast)
{
    halo::game::engine1::KillFeed::broadcast_kill_feed_to_team(message_type, team, broadcast);
}

uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type, datum_index subject, size_t buffer_size)
{
    return halo::game::engine1::KillFeed::build_kill_feed_message_text(recipient, out, message_type, subject, buffer_size);
}

void game_engine_handle_kill_feed_network_event(int32_t **message)
{
    halo::game::engine1::KillFeed::handle_kill_feed_network_event(message);
}

void game_engine_notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type, datum_index subject)
{
    halo::game::engine1::KillFeed::notify_kill_event(player_index, hash_key, message_type, subject);
}

void game_engine_build_end_game_result_text(datum_index player_handle, wchar_t *out)
{
    halo::game::engine1::Scoreboard::build_end_game_result_text(player_handle, out);
}

uint8_t game_engine_build_message_text(wchar_t *out, uint32_t buffer_size, datum_index subject, uint32_t param_1, uint32_t message_type)
{
    return halo::game::engine1::Scoreboard::build_message_text(out, buffer_size, subject, param_1, message_type);
}

uint32_t game_engine_build_scoreboard_sort_key(uint32_t player_index, int32_t score)
{
    return halo::game::engine1::Scoreboard::build_scoreboard_sort_key(player_index, score);
}

int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat, scoreboard_entry out_entries[16], int32_t mode)
{
    return halo::game::engine1::Scoreboard::build_sorted_player_list(invert_low_stat, out_entries, mode);
}

void game_engine_check_bucket_scores_and_end_round(void)
{
    halo::game::engine1::Scoreboard::check_bucket_scores_and_end_round();
}

uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode)
{
    return halo::game::engine1::Scoreboard::compare_score_to_others(subject, team_mode);
}

uint8_t game_engine_find_first_eligible_player_on_team(int32_t team)
{
    return halo::game::engine1::Scoreboard::find_first_eligible_player_on_team(team);
}

void game_engine_find_player_by_name(char *source_name)
{
    halo::game::engine1::Scoreboard::find_player_by_name(source_name);
}

datum_index game_engine_find_player_holding_object(datum_index target_object)
{
    return halo::game::engine1::Scoreboard::find_player_holding_object(target_object);
}

void game_engine_gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2], int32_t filter_value)
{
    halo::game::engine1::Scoreboard::gather_team_score_totals(out_count, out_score, filter_value);
}

wchar_t *game_engine_get_default_multiplayer_string(const scoreboard_entry *entry)
{
    return halo::game::engine1::Scoreboard::get_default_multiplayer_string(entry);
}

wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank)
{
    return halo::game::engine1::Scoreboard::get_multiplayer_text_list(rank);
}

real *game_engine_get_player_color(uint32_t player_index, real *out_rgb)
{
    return halo::game::engine1::Scoreboard::get_player_color(player_index, out_rgb);
}

void game_engine_get_player_scoreboard_entry(datum_index player, scoreboard_entry *out)
{
    halo::game::engine1::Scoreboard::get_player_scoreboard_entry(player, out);
}

int32_t game_engine_get_scoreboard_place(datum_index player, int32_t mode, uint8_t invert_low_stat)
{
    return halo::game::engine1::Scoreboard::get_scoreboard_place(player, mode, invert_low_stat);
}

uint32_t game_engine_is_object_winning(uint32_t handle)
{
    return halo::game::engine1::Scoreboard::is_object_winning(handle);
}

uint32_t game_engine_is_tracked_object_winner(int32_t team)
{
    return halo::game::engine1::Scoreboard::is_tracked_object_winner(team);
}

uint8_t game_engine_is_valid_team_player(uint32_t identifier)
{
    return halo::game::engine1::Scoreboard::is_valid_team_player(identifier);
}

uint8_t game_engine_local_player_score_is_nonpositive(datum_index player_handle)
{
    return halo::game::engine1::Scoreboard::local_player_score_is_nonpositive(player_handle);
}

void game_effects_update(real delta_time)
{
    halo::game::engine1::SimulationClock::effects_update(delta_time);
}

int32_t game_engine_accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder)
{
    return halo::game::engine1::SimulationClock::accumulate_simulation_ticks(elapsed_seconds, keep_remainder);
}

void game_engine_advance_simulation_ticks(float delta_time)
{
    halo::game::engine1::SimulationClock::advance_simulation_ticks(delta_time);
}

void game_engine_allocate_tick_record(void)
{
    halo::game::engine1::SimulationClock::allocate_tick_record();
}

int32_t game_engine_announce_time_remaining(void)
{
    return halo::game::engine1::SimulationClock::announce_time_remaining();
}

void game_engine_apply_catchup_speed_boost(void)
{
    halo::game::engine1::SimulationClock::apply_catchup_speed_boost();
}

float game_engine_compute_time_scale(int32_t param_a, int32_t param_b)
{
    return halo::game::engine1::SimulationClock::compute_time_scale(param_a, param_b);
}

int32_t game_engine_get_current_tick(void)
{
    return halo::game::engine1::SimulationClock::get_current_tick();
}

int32_t game_engine_get_time_remaining(void)
{
    return halo::game::engine1::SimulationClock::get_time_remaining();
}

float game_engine_get_time_scale(void)
{
    return halo::game::engine1::SimulationClock::get_time_scale();
}

void game_engine_init_tick_record_for_mode(void)
{
    halo::game::engine1::SimulationClock::init_tick_record_for_mode();
}

void game_engine_apply_current_custom_variant(void)
{
    halo::game::engine1::Variants::apply_current_custom_variant();
}

void game_engine_apply_player_profile_entry(void *event)
{
    halo::game::engine1::Variants::apply_player_profile_entry(event);
}

void game_engine_apply_variant(const game_variant *variant)
{
    halo::game::engine1::Variants::apply_variant(variant);
}

void game_engine_capture_player_profile(int32_t slot, int32_t commit)
{
    halo::game::engine1::Variants::capture_player_profile(slot, commit);
}

uint32_t game_engine_ensure_variant_history_has_entry(void)
{
    return halo::game::engine1::Variants::ensure_variant_history_has_entry();
}

void game_engine_free_custom_variant_cache(void)
{
    halo::game::engine1::Variants::free_custom_variant_cache();
}

uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out)
{
    return halo::game::engine1::Variants::get_variant_by_name(name, out);
}

void game_engine_invoke_profile_post_update_callback(uint32_t arg_ecx, uint32_t arg_edx)
{
    halo::game::engine1::Variants::invoke_profile_post_update_callback(arg_ecx, arg_edx);
}

uint32_t game_engine_is_map_and_variant_valid(const char *map_path, const char *variant_name)
{
    return halo::game::engine1::Variants::is_map_and_variant_valid(map_path, variant_name);
}

void game_engine_load_from_variant(const game_variant *variant)
{
    halo::game::engine1::Variants::load_from_variant(variant);
}

void game_dispose(void)
{
    halo::game::engine1::Lifecycle::dispose();
}

uint8_t game_engine_attach_players_to_new_bsp(void)
{
    return halo::game::engine1::Lifecycle::attach_players_to_new_bsp();
}

void game_engine_begin_end_game_sequence(void)
{
    halo::game::engine1::Lifecycle::begin_end_game_sequence();
}

void game_engine_end_game_sequence_stage1(void)
{
    halo::game::engine1::Lifecycle::end_game_sequence_stage1();
}

void game_engine_end_game_sequence_stage2(void)
{
    halo::game::engine1::Lifecycle::end_game_sequence_stage2();
}

void game_engine_end_game_sequence_stage3(void)
{
    halo::game::engine1::Lifecycle::end_game_sequence_stage3();
}

uint8_t game_engine_get_teams_enabled(void)
{
    return halo::game::engine1::Lifecycle::get_teams_enabled();
}

void game_engine_initialize_for_new_game(void)
{
    halo::game::engine1::Lifecycle::initialize_for_new_game();
}

uint8_t game_engine_is_inactive(void)
{
    return halo::game::engine1::Lifecycle::is_inactive();
}

void game_engine_maybe_render_post_game(void)
{
    halo::game::engine1::Lifecycle::maybe_render_post_game();
}

int32_t game_engine_multiplayer_ui_state_id(void)
{
    return halo::game::engine1::Lifecycle::multiplayer_ui_state_id();
}

void game_engine_apply_partial_round_reset_message(void *event)
{
    halo::game::engine1::Notifications::apply_partial_round_reset_message(event);
}

void game_engine_apply_player_grenade_counts(uint32_t player_index)
{
    halo::game::engine1::Notifications::apply_player_grenade_counts(player_index);
}

uint8_t game_engine_apply_player_interaction_message(void **envelope)
{
    return halo::game::engine1::Notifications::apply_player_interaction_message(envelope);
}

void game_engine_apply_player_join_message(void **envelope)
{
    halo::game::engine1::Notifications::apply_player_join_message(envelope);
}

void game_engine_apply_player_spawn_loadout_message(void **envelope)
{
    halo::game::engine1::Notifications::apply_player_spawn_loadout_message(envelope);
}

void game_engine_client_apply_team_assignment(void **envelope)
{
    halo::game::engine1::Notifications::client_apply_team_assignment(envelope);
}

void game_engine_dispatch_end_game_notification(void *event)
{
    halo::game::engine1::Notifications::dispatch_end_game_notification(event);
}

void game_engine_dispatch_item_pickup_event(int32_t machine_id, int32_t picked_tag, int32_t param_2)
{
    halo::game::engine1::Notifications::dispatch_item_pickup_event(machine_id, picked_tag, param_2);
}

int32_t game_engine_get_multiplayer_sound_duration_ticks(int32_t sound_index)
{
    return halo::game::engine1::Notifications::get_multiplayer_sound_duration_ticks(sound_index);
}

void game_engine_handle_sound_status_event(void *event)
{
    halo::game::engine1::Notifications::handle_sound_status_event(event);
}

void game_engine_multiplayer_sound_queue_tick(void)
{
    halo::game::engine1::Notifications::multiplayer_sound_queue_tick();
}

void game_engine_notify_item_expired(datum_index object_index)
{
    halo::game::engine1::Notifications::notify_item_expired(object_index);
}

void game_engine_notify_object_value_event(uint8_t value_byte, int32_t hash_key, int32_t machine_index, void *subject)
{
    halo::game::engine1::Notifications::notify_object_value_event(value_byte, hash_key, machine_index, subject);
}

void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t edi_key, uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key)
{
    halo::game::engine1::Notifications::notify_player_interaction(primary_key, edi_key, mode, interaction_type, interaction_seat, secondary_key);
}

uint8_t game_engine_notify_weapon_ready_state_change(datum_index unit_index, datum_index weapon_index)
{
    return halo::game::engine1::Notifications::notify_weapon_ready_state_change(unit_index, weapon_index);
}

void game_engine_build_local_player_control_input(int16_t local_player_index, real delta_time, player_control_input *out)
{
    halo::game::engine1::LocalControl::build_local_player_control_input(local_player_index, delta_time, out);
}

void game_engine_compute_local_player_look_vector(real_vector3d *out_forward, int16_t local_player_index)
{
    halo::game::engine1::LocalControl::compute_local_player_look_vector(out_forward, local_player_index);
}

void game_engine_compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index)
{
    halo::game::engine1::LocalControl::compute_look_angles_from_vector(facing, local_player_index);
}

void game_engine_digitize_control_input(player_control_input *input)
{
    halo::game::engine1::LocalControl::digitize_control_input(input);
}

real game_engine_get_max_look_pitch(int16_t local_player_index)
{
    return halo::game::engine1::LocalControl::get_max_look_pitch(local_player_index);
}

void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index)
{
    halo::game::engine1::LocalControl::init_player_look_state_from_object(unit, local_player_index);
}

void game_engine_build_visible_cluster_bitmask(uint32_t *out_bitmask, uint8_t local_players_only)
{
    halo::game::engine1::SpawnLocations::build_visible_cluster_bitmask(out_bitmask, local_players_only);
}

int16_t game_engine_collect_matching_waypoints(int32_t candidate, float *out_positions, uint8_t *out_slots, int16_t max_count)
{
    return halo::game::engine1::SpawnLocations::collect_matching_waypoints(candidate, out_positions, out_slots, max_count);
}

int32_t game_engine_find_nearest_unused_type4_location(int32_t *excluded_indices, int32_t excluded_count, real_point3d *reference_point)
{
    return halo::game::engine1::SpawnLocations::find_nearest_unused_type4_location(excluded_indices, excluded_count, reference_point);
}

int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin, float max_horizontal_dist, float max_height_delta)
{
    return halo::game::engine1::SpawnLocations::find_one_valid_starting_location(type, team, origin, max_horizontal_dist, max_height_delta);
}

int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results)
{
    return halo::game::engine1::SpawnLocations::find_valid_starting_locations(origin, max_horizontal_dist, max_height_delta, team, type, max_results, results);
}

uint8_t game_engine_location_blocked_by_vehicle(real_point3d *point)
{
    return halo::game::engine1::SpawnLocations::location_blocked_by_vehicle(point);
}

void game_engine_cleanup_dropped_objects(void)
{
    halo::game::engine1::ObjectCleanup::cleanup_dropped_objects();
}

void game_engine_cleanup_stray_items(void)
{
    halo::game::engine1::ObjectCleanup::cleanup_stray_items();
}

void game_engine_cleanup_stray_projectiles(void)
{
    halo::game::engine1::ObjectCleanup::cleanup_stray_projectiles();
}

void game_engine_clear_unit_shields_when_disabled(datum_index player_handle)
{
    halo::game::engine1::ObjectCleanup::clear_unit_shields_when_disabled(player_handle);
}

void game_engine_flag_local_player_units(void)
{
    halo::game::engine1::ObjectCleanup::flag_local_player_units();
}

uint8_t game_engine_object_flag_bit3_clear(int32_t handle)
{
    return halo::game::engine1::ObjectCleanup::object_flag_bit3_clear(handle);
}

}
