#include "halo/ai/airest_system.hpp"
#include "halo/ai/airest_objects.hpp"
#include "halo/ai/airest_communication.hpp"
#include "halo/ai/airest_conversation.hpp"
#include "halo/ai/airest_encounters.hpp"
#include "halo/ai/airest_pathfind.hpp"
#include "halo/ai/airest_reference.hpp"
#include "halo/ai/airest_search.hpp"

extern "C" {

/**
 * C entry point for halo::ai::AiSystem::actors_initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x426710
 */
void actors_initialize()
{
    halo::ai::AiSystem::actors_initialize();
}

/**
 * C entry point for halo::ai::AiSystem::accumulate_repeated_event; forwards to the C++ implementation unchanged.
 *
 * @address 0x42c610
 */
void ai_accumulate_repeated_event(int32_t event_type, real_point3d *position, int16_t event_id, int16_t window_ticks)
{
    halo::ai::AiSystem::accumulate_repeated_event(event_type, position, event_id, window_ticks);
}

/**
 * C entry point for halo::ai::AiActorView::get_activity_stage; forwards to the C++ implementation unchanged.
 *
 * @address 0x435680
 */
int32_t ai_actor_get_activity_stage(datum_index actor_index)
{
    return halo::ai::AiActorView(actor_index).get_activity_stage();
}

/**
 * C entry point for halo::ai::AiActorView::link_to_unassigned_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x436940
 */
void ai_actor_link_to_unassigned_list(datum_index actor_index)
{
    halo::ai::AiActorView(actor_index).link_to_unassigned_list();
}

/**
 * C entry point for halo::ai::AiObjects::type_get_morale_grade; forwards to the C++ implementation unchanged.
 *
 * @address 0x434ed0
 */
uint32_t ai_actor_type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference)
{
    return halo::ai::AiObjects::type_get_morale_grade(actor_type_index, command_reference);
}

/**
 * C entry point for halo::ai::AiActorView::unlink_from_unassigned_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x436990
 */
void ai_actor_unlink_from_unassigned_list(datum_index actor_index)
{
    halo::ai::AiActorView(actor_index).unlink_from_unassigned_list();
}

/**
 * C entry point for halo::ai::AiSystem::alert_actors_in_grenade_radius; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a0e0
 */
void ai_alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate)
{
    halo::ai::AiSystem::alert_actors_in_grenade_radius(source_unit_index, stimulus, gate);
}

/**
 * C entry point for halo::ai::AiCommunication::broadcast_communication_event; forwards to the C++ implementation unchanged.
 *
 * @address 0x429fc0
 */
void ai_broadcast_communication_event(int16_t gate, real_point3d *point, int32_t source_object, int16_t event_type, int16_t unused)
{
    halo::ai::AiCommunication::broadcast_communication_event(gate, point, source_object, event_type, unused);
}

/**
 * C entry point for halo::ai::AiSystem::build_priority_target_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x42acd0
 */
void ai_build_priority_target_list(ai_priority_target_list *out_list)
{
    halo::ai::AiSystem::build_priority_target_list(out_list);
}

/**
 * C entry point for halo::ai::AiSystem::category_matches_wildcard; forwards to the C++ implementation unchanged.
 *
 * @address 0x433ba0
 */
void ai_category_matches_wildcard(int16_t category, int16_t other_category)
{
    halo::ai::AiSystem::category_matches_wildcard(category, other_category);
}

/**
 * C entry point for halo::ai::AiObjects::clear_object_references; forwards to the C++ implementation unchanged.
 *
 * @address 0x42c140
 */
void ai_clear_object_references(datum_index object_index)
{
    halo::ai::AiObjects::clear_object_references(object_index);
}

/**
 * C entry point for halo::ai::AiCommunication::broadcast; forwards to the C++ implementation unchanged.
 *
 * @address 0x42d340
 */
void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data)
{
    halo::ai::AiCommunication::broadcast(event_code, unit_index, object_a, reason, object_b, object_c, extra_data);
}

/**
 * C entry point for halo::ai::AiCommunication::gate_line_played; forwards to the C++ implementation unchanged.
 *
 * @address 0x42e970
 */
void ai_communication_gate_line_played(int16_t event_id, ai_communication_record *record, datum_index object_index)
{
    halo::ai::AiCommunication::gate_line_played(event_id, record, object_index);
}

/**
 * C entry point for halo::ai::AiCommunication::initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x42cf20
 */
void ai_communication_initialize()
{
    halo::ai::AiCommunication::initialize();
}

/**
 * C entry point for halo::ai::AiCommunication::line_fade_multiplier; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f8c0
 */
int16_t ai_communication_line_fade_multiplier(uint32_t unit_index, int16_t priority, int16_t extra_delay, uint8_t follow_fallback, uint8_t apply_fade, float *volume, int32_t *chain_value, int16_t *dialogue_index, int16_t line_class)
{
    return halo::ai::AiCommunication::line_fade_multiplier(unit_index, priority, extra_delay, follow_fallback, apply_fade, volume, chain_value, dialogue_index, line_class);
}

/**
 * C entry point for halo::ai::AiCommunication::play_event_line; forwards to the C++ implementation unchanged.
 *
 * @address 0x42eee0
 */
void ai_communication_play_event_line(datum_index object_index, int16_t event_id, uint8_t force, datum_index explicit_speaker_actor_index, uint32_t *event_record)
{
    halo::ai::AiCommunication::play_event_line(object_index, event_id, force, explicit_speaker_actor_index, event_record);
}

/**
 * C entry point for halo::ai::AiCommunication::rate_player_proximity; forwards to the C++ implementation unchanged.
 *
 * @address 0x4303f0
 */
float ai_communication_rate_player_proximity(uint8_t require_line_of_sight, datum_index *out_player_object_index, float *out_distance, datum_index object_index)
{
    return halo::ai::AiCommunication::rate_player_proximity(require_line_of_sight, out_player_object_index, out_distance, object_index);
}

/**
 * C entry point for halo::ai::AiCommunication::rate_speaker; forwards to the C++ implementation unchanged.
 *
 * @address 0x42fb90
 */
float ai_communication_rate_speaker(datum_index actor_index, datum_index object_b, real_point3d *position_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, real_point3d *position_a, datum_index object_a)
{
    return halo::ai::AiCommunication::rate_speaker(actor_index, object_b, position_b, radius, allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags, position_a, object_a);
}

/**
 * C entry point for halo::ai::AiCommunication::record_line_played; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f9e0
 */
void ai_communication_record_line_played(datum_index object_index, int16_t tier, int16_t communication_line_id, int16_t conversation_line_id)
{
    halo::ai::AiCommunication::record_line_played(object_index, tier, communication_line_id, conversation_line_id);
}

/**
 * C entry point for halo::ai::AiCommunication::reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x42d230
 */
void ai_communication_reset()
{
    halo::ai::AiCommunication::reset();
}

/**
 * C entry point for halo::ai::AiCommunication::select_speaker_by_team; forwards to the C++ implementation unchanged.
 *
 * @address 0x4300d0
 */
datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a, datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team)
{
    return halo::ai::AiCommunication::select_speaker_by_team(match_mode, object_a, object_b, radius, allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags, team);
}

/**
 * C entry point for halo::ai::AiCommunication::select_speaker_in_reference; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ff80
 */
datum_index ai_communication_select_speaker_in_reference(float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags, uint32_t reference, datum_index object_a, datum_index object_b)
{
    return halo::ai::AiCommunication::select_speaker_in_reference(radius, allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags, reference, object_a, object_b);
}

/**
 * C entry point for halo::ai::AiCommunication::target_result_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x42d310
 */
void ai_communication_target_result_reset(ai_communication_target_result *record)
{
    halo::ai::AiCommunication::target_result_reset(record);
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::activate; forwards to the C++ implementation unchanged.
 *
 * @address 0x4307c0
 */
uint8_t ai_conversation_activate(int16_t conversation_definition_index, uint8_t allow_eviction)
{
    return halo::ai::ConversationDefinitionView(conversation_definition_index).activate(allow_eviction);
}

/**
 * C entry point for halo::ai::ConversationView::activate_next_participant; forwards to the C++ implementation unchanged.
 *
 * @address 0x431d10
 */
uint8_t ai_conversation_activate_next_participant(datum_index instance_handle)
{
    return halo::ai::ConversationView(instance_handle).activate_next_participant();
}

/**
 * C entry point for halo::ai::Conversations::clear_object_references; forwards to the C++ implementation unchanged.
 *
 * @address 0x430d30
 */
void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan)
{
    halo::ai::Conversations::clear_object_references(object_index, force_full_scan);
}

/**
 * C entry point for halo::ai::Conversations::clear_participant; forwards to the C++ implementation unchanged.
 *
 * @address 0x430c70
 */
void ai_conversation_clear_participant(datum_index actor_index)
{
    halo::ai::Conversations::clear_participant(actor_index);
}

/**
 * C entry point for halo::ai::ConversationView::current_line_is_ready; forwards to the C++ implementation unchanged.
 *
 * @address 0x431e70
 */
uint8_t ai_conversation_current_line_is_ready(datum_index instance_handle)
{
    return halo::ai::ConversationView(instance_handle).current_line_is_ready();
}

/**
 * C entry point for halo::ai::Conversations::get_run_to_player_range; forwards to the C++ implementation unchanged.
 *
 * @address 0x402cf0
 */
int32_t ai_conversation_get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index)
{
    return halo::ai::Conversations::get_run_to_player_range(out, conversation_index);
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::get_status; forwards to the C++ implementation unchanged.
 *
 * @address 0x430830
 */
int32_t ai_conversation_get_status(int16_t conversation_definition_index)
{
    return halo::ai::ConversationDefinitionView(conversation_definition_index).get_status();
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::get_line_index; forwards to the C++ implementation unchanged.
 *
 * @address 0x430960
 */
int16_t ai_conversation_get_line_index(int16_t conversation_definition_index)
{
    return halo::ai::ConversationDefinitionView(conversation_definition_index).get_line_index();
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::mark_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x430a20
 */
void ai_conversation_mark_all(int16_t conversation_definition_index)
{
    halo::ai::ConversationDefinitionView(conversation_definition_index).mark_all();
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x431590
 */
datum_index ai_conversation_new(int16_t conversation_definition_index, uint8_t allow_eviction)
{
    return halo::ai::ConversationDefinitionView(conversation_definition_index).create(allow_eviction);
}

/**
 * C entry point for halo::ai::Conversations::resolve_participant; forwards to the C++ implementation unchanged.
 *
 * @address 0x431680
 */
int8_t ai_conversation_resolve_participant(int16_t participant_index, uint8_t *out_resolved, uint8_t *out_wants_alternate, uint8_t *out_blocked_by_player, float *inout_minimum_distance, datum_index conversation_index)
{
    return halo::ai::Conversations::resolve_participant(participant_index, out_resolved, out_wants_alternate, out_blocked_by_player, inout_minimum_distance, conversation_index);
}

/**
 * C entry point for halo::ai::ConversationView::resolve_participants; forwards to the C++ implementation unchanged.
 *
 * @address 0x430fc0
 */
uint8_t ai_conversation_resolve_participants(datum_index conversation_index, uint8_t *out_keep_trying)
{
    return halo::ai::ConversationView(conversation_index).resolve_participants(out_keep_trying);
}

/**
 * C entry point for halo::ai::ConversationView::stop; forwards to the C++ implementation unchanged.
 *
 * @address 0x430ea0
 */
void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b)
{
    halo::ai::ConversationView(instance_handle).stop(reason_a, reason_b);
}

/**
 * C entry point for halo::ai::ConversationDefinitionView::stop_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x4309c0
 */
void ai_conversation_stop_all(int16_t conversation_definition_index)
{
    halo::ai::ConversationDefinitionView(conversation_definition_index).stop_all();
}

/**
 * C entry point for halo::ai::Conversations::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x430a70
 */
void ai_conversation_update()
{
    halo::ai::Conversations::update();
}

/**
 * C entry point for halo::ai::AiSystem::count_actors_in_mode9_group; forwards to the C++ implementation unchanged.
 *
 * @address 0x433e20
 */
int16_t ai_count_actors_in_mode9_group(int32_t group_id)
{
    return halo::ai::AiSystem::count_actors_in_mode9_group(group_id);
}

/**
 * C entry point for the dialogue condition at table position 0; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f4f0
 */
uint8_t ai_dialogue_condition_42f4f0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(0).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 1; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f560
 */
uint8_t ai_dialogue_condition_42f560(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(1).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 2; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f5b0
 */
uint8_t ai_dialogue_condition_42f5b0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(2).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 3; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f650
 */
uint8_t ai_dialogue_condition_42f650(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(3).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 4; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f690
 */
uint8_t ai_dialogue_condition_42f690(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(4).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 5; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f6f0
 */
uint8_t ai_dialogue_condition_42f6f0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(5).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 6; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f7b0
 */
uint8_t ai_dialogue_condition_42f7b0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(6).test(object_index, param_2, actor_index);
}

/**
 * C entry point for the dialogue condition at table position 7; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f7f0
 */
uint8_t ai_dialogue_condition_42f7f0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    return halo::ai::dialogue_condition(7).test(object_index, param_2, actor_index);
}

/**
 * C entry point for halo::ai::AiCommunication::dispatch_queued_order; forwards to the C++ implementation unchanged.
 *
 * @address 0x42f840
 */
void ai_dispatch_queued_order(ai_queued_order *order, datum_index prop_index, datum_index actor_index)
{
    halo::ai::AiCommunication::dispatch_queued_order(order, prop_index, actor_index);
}

/**
 * C entry point for halo::ai::EncounterView::drift_zone_bias; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a9d0
 */
uint8_t ai_drift_zone_bias(datum_index encounter_index, int16_t squad_offset, float bias)
{
    return halo::ai::EncounterView(encounter_index).drift_zone_bias(squad_offset, bias);
}

/**
 * C entry point for halo::ai::EncounterView::record_recent_zone; forwards to the C++ implementation unchanged.
 *
 * @address 0x437820
 */
int32_t ai_encounter_record_recent_zone(datum_index encounter_index, int16_t zone_id)
{
    return halo::ai::EncounterView(encounter_index).record_recent_zone(zone_id);
}

/**
 * C entry point for halo::ai::EncounterView::stamp_team_from_unit; forwards to the C++ implementation unchanged.
 *
 * @address 0x436710
 */
void ai_encounter_stamp_team_from_unit(datum_index encounter_index, datum_index unit_index)
{
    halo::ai::EncounterView(encounter_index).stamp_team_from_unit(unit_index);
}

/**
 * C entry point for halo::ai::AiSystem::get_difficulty_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a950
 */
void ai_get_difficulty_request(int16_t request_code, uint8_t *out_flag_a, uint8_t *out_flag_b, float *out_value)
{
    halo::ai::AiSystem::get_difficulty_request(request_code, out_flag_a, out_flag_b, out_value);
}

/**
 * C entry point for halo::ai::AiSystem::group_bucket_find_or_add; forwards to the C++ implementation unchanged.
 *
 * @address 0x420de0
 */
int16_t ai_group_bucket_find_or_add(ai_group_bucket_entry *buckets, int32_t key, int16_t *count, int16_t capacity)
{
    return halo::ai::AiSystem::group_bucket_find_or_add(buckets, key, count, capacity);
}

/**
 * C entry point for halo::ai::AiSystem::initialize_for_new_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a7c0
 */
void ai_initialize_for_new_map()
{
    halo::ai::AiSystem::initialize_for_new_map();
}

/**
 * C entry point for halo::ai::AiSystem::insert_scored_candidate_pair; forwards to the C++ implementation unchanged.
 *
 * @address 0x4383f0
 */
uint8_t ai_insert_scored_candidate_pair(ai_scored_candidate *list, datum_index handle, float score, datum_index payload, datum_index key)
{
    return halo::ai::AiSystem::insert_scored_candidate_pair(list, handle, score, payload, key);
}

/**
 * C entry point for halo::ai::AiSystem::mark_recognized_objects_for_reaction; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ba80
 */
void ai_mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status)
{
    halo::ai::AiSystem::mark_recognized_objects_for_reaction(team_a, team_b, status);
}

/**
 * C entry point for halo::ai::PathFinder::navigate_around_obstacles; forwards to the C++ implementation unchanged.
 *
 * @address 0x43be90
 */
uint8_t ai_navigate_around_obstacles(path_find_context *context, int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    return halo::ai::PathFinder(context).navigate_around_obstacles(count, waypoints, out_count, out_waypoints, out_valid);
}

/**
 * C entry point for halo::ai::AiSystem::notify_actors_of_encounter_state_change; forwards to the C++ implementation unchanged.
 *
 * @address 0x42b940
 */
void ai_notify_actors_of_encounter_state_change(int16_t zone_a, int16_t zone_b, uint8_t status, uint8_t force_update)
{
    halo::ai::AiSystem::notify_actors_of_encounter_state_change(zone_a, zone_b, status, force_update);
}

/**
 * C entry point for halo::ai::AiObjects::object_attention_find_or_create; forwards to the C++ implementation unchanged.
 *
 * @address 0x435900
 */
ai_object_attention_record * ai_object_attention_find_or_create(datum_index object_index)
{
    return halo::ai::AiObjects::object_attention_find_or_create(object_index);
}

/**
 * C entry point for halo::ai::AiObjects::object_attention_remove; forwards to the C++ implementation unchanged.
 *
 * @address 0x435990
 */
void ai_object_attention_remove(datum_index object_index)
{
    halo::ai::AiObjects::object_attention_remove(object_index);
}

/**
 * C entry point for halo::ai::ObjectListView::clear_orders_with_weapon; forwards to the C++ implementation unchanged.
 *
 * @address 0x432ad0
 */
void ai_object_list_clear_orders_with_weapon(datum_index object_list_header_handle)
{
    halo::ai::ObjectListView(object_list_header_handle).clear_orders_with_weapon();
}

/**
 * C entry point for halo::ai::ObjectListView::detach_actors_from_encounters; forwards to the C++ implementation unchanged.
 *
 * @address 0x435260
 */
void ai_object_list_detach_actors_from_encounters(datum_index object_list_header_handle)
{
    halo::ai::ObjectListView(object_list_header_handle).detach_actors_from_encounters();
}

/**
 * C entry point for halo::ai::ObjectListView::initialize_shield_stun_thresholds; forwards to the C++ implementation unchanged.
 *
 * @address 0x561ab0
 */
void ai_object_list_initialize_shield_stun_thresholds(datum_index object_list_header_handle, float override_max_body_vitality, float override_max_shield_vitality)
{
    halo::ai::ObjectListView(object_list_header_handle).initialize_shield_stun_thresholds(override_max_body_vitality, override_max_shield_vitality);
}

/**
 * C entry point for halo::ai::ObjectListView::max_flee_grade; forwards to the C++ implementation unchanged.
 *
 * @address 0x434f20
 */
int16_t ai_object_list_max_flee_grade(datum_index object_list_header_handle)
{
    return halo::ai::ObjectListView(object_list_header_handle).max_flee_grade();
}

/**
 * C entry point for halo::ai::ObjectListView::remap_units_and_children; forwards to the C++ implementation unchanged.
 *
 * @address 0x433a70
 */
void ai_object_list_remap_units_and_children(datum_index object_list_header, uint32_t packed_reference, char notify)
{
    halo::ai::ObjectListView(object_list_header).remap_units_and_children(packed_reference, notify);
}

/**
 * C entry point for halo::ai::ObjectListView::reset_or_wake_awareness; forwards to the C++ implementation unchanged.
 *
 * @address 0x434590
 */
void ai_object_list_reset_or_wake_awareness(datum_index object_list_header_handle, char flag)
{
    halo::ai::ObjectListView(object_list_header_handle).reset_or_wake_awareness(flag);
}

/**
 * C entry point for halo::ai::ObjectListView::respawn_members; forwards to the C++ implementation unchanged.
 *
 * @address 0x432e80
 */
void ai_object_list_respawn_members(datum_index object_list_header_handle, uint32_t packed_reference)
{
    halo::ai::ObjectListView(object_list_header_handle).respawn_members(packed_reference);
}

/**
 * C entry point for halo::ai::ObjectListView::set_unit_flag_400; forwards to the C++ implementation unchanged.
 *
 * @address 0x4347b0
 */
void ai_object_list_set_unit_flag_400(datum_index object_list_header_handle, char flag)
{
    halo::ai::ObjectListView(object_list_header_handle).set_unit_flag_400(flag);
}

/**
 * C entry point for halo::ai::ObjectListView::set_unit_flag_800; forwards to the C++ implementation unchanged.
 *
 * @address 0x4348c0
 */
void ai_object_list_set_unit_flag_800(datum_index object_list_header_handle, char flag)
{
    halo::ai::ObjectListView(object_list_header_handle).set_unit_flag_800(flag);
}

/**
 * C entry point for halo::ai::ObjectListView::set_unit_flag_800000; forwards to the C++ implementation unchanged.
 *
 * @address 0x561d50
 */
void ai_object_list_set_unit_flag_800000(datum_index object_list_header_handle, char flag)
{
    halo::ai::ObjectListView(object_list_header_handle).set_unit_flag_800000(flag);
}

/**
 * C entry point for halo::ai::ObjectListView::spawn_members; forwards to the C++ implementation unchanged.
 *
 * @address 0x432a40
 */
void ai_object_list_spawn_members(datum_index object_list_header_handle, uint32_t packed_reference)
{
    halo::ai::ObjectListView(object_list_header_handle).spawn_members(packed_reference);
}

/**
 * C entry point for halo::ai::ObjectListView::start_user_animation_until_failure; forwards to the C++ implementation unchanged.
 *
 * @address 0x561e60
 */
uint8_t ai_object_list_start_user_animation_until_failure(datum_index object_list_header_handle, datum_index graph_tag_id, const char *animation_name, uint8_t interpolate)
{
    return halo::ai::ObjectListView(object_list_header_handle).start_user_animation_until_failure(graph_tag_id, animation_name, interpolate);
}

/**
 * C entry point for halo::ai::ObjectListView::update_vitality_fractions; forwards to the C++ implementation unchanged.
 *
 * @address 0x561cb0
 */
void ai_object_list_update_vitality_fractions(datum_index object_list_header_handle, float body_delta, float shield_delta)
{
    halo::ai::ObjectListView(object_list_header_handle).update_vitality_fractions(body_delta, shield_delta);
}

/**
 * C entry point for halo::ai::AiObjects::object_process_nearby_actors; forwards to the C++ implementation unchanged.
 *
 * @address 0x433cc0
 */
void ai_object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name, char allow_boarding_actors)
{
    halo::ai::AiObjects::object_process_nearby_actors(ai_reference, vehicle_index, seat_name, allow_boarding_actors);
}

/**
 * C entry point for halo::ai::AiSystem::pick_weighted_candidate; forwards to the C++ implementation unchanged.
 *
 * @address 0x438480
 */
int16_t ai_pick_weighted_candidate(ai_scored_candidate *table, ai_scored_candidate *out_entry)
{
    return halo::ai::AiSystem::pick_weighted_candidate(table, out_entry);
}

/**
 * C entry point for halo::ai::ReferenceView::clear_defending; forwards to the C++ implementation unchanged.
 *
 * @address 0x433200
 */
void ai_platoon_range_clear_defending(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).clear_defending();
}

/**
 * C entry point for halo::ai::ReferenceView::has_available; forwards to the C++ implementation unchanged.
 *
 * @address 0x433180
 */
uint8_t ai_platoon_range_has_available(uint32_t packed_reference)
{
    return halo::ai::ReferenceView(packed_reference).has_available();
}

/**
 * C entry point for halo::ai::ReferenceView::set_defending; forwards to the C++ implementation unchanged.
 *
 * @address 0x433270
 */
void ai_platoon_range_set_defending(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).set_defending();
}

/**
 * C entry point for halo::ai::ReferenceView::set_maneuvering; forwards to the C++ implementation unchanged.
 *
 * @address 0x4332e0
 */
void ai_platoon_range_set_maneuvering(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).set_maneuvering();
}

/**
 * C entry point for halo::ai::ReferenceView::set_maneuver_enabled; forwards to the C++ implementation unchanged.
 *
 * @address 0x433350
 */
void ai_platoon_range_set_maneuver_enabled(uint32_t packed_reference, char flag)
{
    halo::ai::ReferenceView(packed_reference).set_maneuver_enabled(flag);
}

/**
 * C entry point for halo::ai::AiSystem::process_vehicle_entry_queue; forwards to the C++ implementation unchanged.
 *
 * @address 0x42bf90
 */
void ai_process_vehicle_entry_queue()
{
    halo::ai::AiSystem::process_vehicle_entry_queue();
}

/**
 * C entry point for halo::ai::AiCommunication::propagate_communication_reaction; forwards to the C++ implementation unchanged.
 *
 * @address 0x42e9c0
 */
void ai_propagate_communication_reaction(datum_index object_index, ai_communication_order *order)
{
    halo::ai::AiCommunication::propagate_communication_reaction(object_index, order);
}

/**
 * C entry point for halo::ai::AiObjects::pursuit_check_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x436b90
 */
uint8_t ai_pursuit_check_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick, char create_if_missing, int16_t *out_count, uint32_t *out_last_tick)
{
    return halo::ai::AiObjects::pursuit_check_object(object_index, encounter_index, type, min_last_tick, create_if_missing, out_count, out_last_tick);
}

/**
 * C entry point for halo::ai::AiObjects::pursuit_note_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x436b10
 */
uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick)
{
    return halo::ai::AiObjects::pursuit_note_object(object_index, encounter_index, type, min_last_tick);
}

/**
 * C entry point for halo::ai::AiSystem::recompute_all_relationship_flags; forwards to the C++ implementation unchanged.
 *
 * @address 0x42bbb0
 */
void ai_recompute_all_relationship_flags()
{
    halo::ai::AiSystem::recompute_all_relationship_flags();
}

/**
 * C entry point for halo::ai::ReferenceView::activate_squads; forwards to the C++ implementation unchanged.
 *
 * @address 0x432b80
 */
void ai_reference_activate_squads(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).activate_squads();
}

/**
 * C entry point for halo::ai::ReferenceView::actor_iterator_init_cursor; forwards to the C++ implementation unchanged.
 *
 * @address 0x4369f0
 */
void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor)
{
    halo::ai::ReferenceView::actor_iterator_init_cursor(encounter_index, cursor);
}

/**
 * C entry point for halo::ai::ReferenceView::actor_iterator_new; forwards to the C++ implementation unchanged.
 *
 * @address 0x432650
 */
void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator)
{
    halo::ai::ReferenceView(packed_reference).actor_iterator_new(out_iterator);
}

/**
 * C entry point for halo::ai::ReferenceView::actor_iterator_next; forwards to the C++ implementation unchanged.
 *
 * @address 0x4326d0
 */
actor * ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator)
{
    return halo::ai::ReferenceView::actor_iterator_next(iterator);
}

/**
 * C entry point for halo::ai::ReferenceView::build_object_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x432740
 */
datum_index ai_reference_build_object_list(uint32_t packed_reference)
{
    return halo::ai::ReferenceView(packed_reference).build_object_list();
}

/**
 * C entry point for halo::ai::ReferenceView::clear_search_target; forwards to the C++ implementation unchanged.
 *
 * @address 0x434c80
 */
void ai_reference_clear_search_target(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).clear_search_target();
}

/**
 * C entry point for halo::ai::ReferenceView::detach_actors_from_encounters; forwards to the C++ implementation unchanged.
 *
 * @address 0x4351c0
 */
void ai_reference_detach_actors_from_encounters(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).detach_actors_from_encounters();
}

/**
 * C entry point for halo::ai::ReferenceView::expand_to_platoon_range; forwards to the C++ implementation unchanged.
 *
 * @address 0x432420
 */
void ai_reference_expand_to_platoon_range(uint32_t packed_reference, ai_reference_platoon_range *out_range)
{
    halo::ai::ReferenceView(packed_reference).expand_to_platoon_range(out_range);
}

/**
 * C entry point for halo::ai::ReferenceView::face_starting_location; forwards to the C++ implementation unchanged.
 *
 * @address 0x4349d0
 */
void ai_reference_face_starting_location(uint32_t packed_reference, uint8_t idle_only)
{
    halo::ai::ReferenceView(packed_reference).face_starting_location(idle_only);
}

/**
 * C entry point for halo::ai::ReferenceView::flee_if_ready; forwards to the C++ implementation unchanged.
 *
 * @address 0x434d90
 */
void ai_reference_flee_if_ready(uint32_t packed_reference, uint32_t readiness_param)
{
    halo::ai::ReferenceView(packed_reference).flee_if_ready(readiness_param);
}

/**
 * C entry point for halo::ai::ReferenceView::for_each_squad; forwards to the C++ implementation unchanged.
 *
 * @address 0x432f50
 */
void ai_reference_for_each_squad(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).for_each_squad();
}

/**
 * C entry point for halo::ai::ReferenceView::get_stat_pair; forwards to the C++ implementation unchanged.
 *
 * @address 0x432f90
 */
uint32_t ai_reference_get_stat_pair(uint32_t packed_reference, int16_t stat_kind, int32_t *out_member_count, uint32_t *out_extra)
{
    return halo::ai::ReferenceView(packed_reference).get_stat_pair(stat_kind, out_member_count, out_extra);
}

/**
 * C entry point for halo::ai::ReferenceView::invoke_squad_callback_406f80; forwards to the C++ implementation unchanged.
 *
 * @address 0x434e60
 */
void ai_reference_invoke_squad_callback_406f80(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).invoke_squad_callback_406f80();
}

/**
 * C entry point for halo::ai::ReferenceView::start_squad_timers; forwards to the C++ implementation unchanged.
 *
 * @address 0x432f10
 */
void ai_reference_start_squad_timers(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).start_squad_timers();
}

/**
 * C entry point for halo::ai::ReferenceView::max_activity_stage; forwards to the C++ implementation unchanged.
 *
 * @address 0x435700
 */
int16_t ai_reference_max_activity_stage(uint32_t packed_reference)
{
    return halo::ai::ReferenceView(packed_reference).max_activity_stage();
}

/**
 * C entry point for halo::ai::ReferenceView::notify_actors; forwards to the C++ implementation unchanged.
 *
 * @address 0x432bd0
 */
void ai_reference_notify_actors(uint32_t packed_reference, uint8_t flag)
{
    halo::ai::ReferenceView(packed_reference).notify_actors(flag);
}

/**
 * C entry point for halo::ai::ReferenceView::notify_squad_index; forwards to the C++ implementation unchanged.
 *
 * @address 0x432c20
 */
void ai_reference_notify_squad_index(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).notify_squad_index();
}

/**
 * C entry point for halo::ai::ReferenceView::parse; forwards to the C++ implementation unchanged.
 *
 * @address 0x432320
 */
uint8_t ai_reference_parse(char *reference_string, Scenario *scenario, uint32_t *out_packed_reference)
{
    return halo::ai::ReferenceView::parse(reference_string, scenario, out_packed_reference);
}

/**
 * C entry point for halo::ai::ReferenceView::refill_grenades; forwards to the C++ implementation unchanged.
 *
 * @address 0x434af0
 */
void ai_reference_refill_grenades(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).refill_grenades();
}

/**
 * C entry point for halo::ai::ReferenceView::reset_or_wake_awareness; forwards to the C++ implementation unchanged.
 *
 * @address 0x434500
 */
void ai_reference_reset_or_wake_awareness(uint32_t packed_reference, char flag)
{
    halo::ai::ReferenceView(packed_reference).reset_or_wake_awareness(flag);
}

/**
 * C entry point for halo::ai::ReferenceView::resolve_squad_datum; forwards to the C++ implementation unchanged.
 *
 * @address 0x432c80
 */
int32_t ai_reference_resolve_squad_datum(uint32_t packed_reference)
{
    return halo::ai::ReferenceView(packed_reference).resolve_squad_datum();
}

/**
 * C entry point for halo::ai::ReferenceView::respawn_all_players; forwards to the C++ implementation unchanged.
 *
 * @address 0x432d90
 */
void ai_reference_respawn_all_players(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).respawn_all_players();
}

/**
 * C entry point for halo::ai::ReferenceView::respawn_member; forwards to the C++ implementation unchanged.
 *
 * @address 0x432df0
 */
void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index)
{
    halo::ai::ReferenceView(packed_reference).respawn_member(unit_index);
}

/**
 * C entry point for halo::ai::ReferenceView::respawn_placed_members; forwards to the C++ implementation unchanged.
 *
 * @address 0x432d30
 */
void ai_reference_respawn_placed_members(uint32_t packed_reference, uint32_t respawn_reference)
{
    halo::ai::ReferenceView(packed_reference).respawn_placed_members(respawn_reference);
}

/**
 * C entry point for halo::ai::ReferenceView::set_combat_alert_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x435af0
 */
void ai_reference_set_combat_alert_flag(uint32_t packed_reference, uint8_t new_flag)
{
    halo::ai::ReferenceView(packed_reference).set_combat_alert_flag(new_flag);
}

/**
 * C entry point for halo::ai::ReferenceView::set_search_target_area; forwards to the C++ implementation unchanged.
 *
 * @address 0x434d00
 */
void ai_reference_set_search_target_area(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).set_search_target_area();
}

/**
 * C entry point for halo::ai::ReferenceView::set_search_target_point; forwards to the C++ implementation unchanged.
 *
 * @address 0x434cc0
 */
void ai_reference_set_search_target_point(uint32_t packed_reference, uint32_t reference_value)
{
    halo::ai::ReferenceView(packed_reference).set_search_target_point(reference_value);
}

/**
 * C entry point for halo::ai::ReferenceView::set_squads_dormancy_allowed; forwards to the C++ implementation unchanged.
 *
 * @address 0x435bc0
 */
void ai_reference_set_squads_dormancy_allowed(uint32_t packed_reference, char flag)
{
    halo::ai::ReferenceView(packed_reference).set_squads_dormancy_allowed(flag);
}

/**
 * C entry point for halo::ai::ReferenceView::set_charge_allowed; forwards to the C++ implementation unchanged.
 *
 * @address 0x434d40
 */
void ai_reference_set_charge_allowed(uint32_t packed_reference, char flag)
{
    halo::ai::ReferenceView(packed_reference).set_charge_allowed(flag);
}

/**
 * C entry point for halo::ai::ReferenceView::spawn_starting_location_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x4328c0
 */
void ai_reference_spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference)
{
    halo::ai::ReferenceView::spawn_starting_location_object(unit_index, packed_reference);
}

/**
 * C entry point for halo::ai::ReferenceView::squad_iterator_new; forwards to the C++ implementation unchanged.
 *
 * @address 0x4324f0
 */
void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator)
{
    halo::ai::ReferenceView(packed_reference).squad_iterator_new(out_iterator);
}

/**
 * C entry point for halo::ai::ReferenceView::squad_iterator_next; forwards to the C++ implementation unchanged.
 *
 * @address 0x4325b0
 */
encounter_squad_state * ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator)
{
    return halo::ai::ReferenceView::squad_iterator_next(iterator);
}

/**
 * C entry point for halo::ai::ReferenceView::squad_set_automatic_migration; forwards to the C++ implementation unchanged.
 *
 * @address 0x435ab0
 */
void ai_reference_squad_set_automatic_migration(uint32_t packed_reference, uint8_t value)
{
    halo::ai::ReferenceView(packed_reference).squad_set_automatic_migration(value);
}

/**
 * C entry point for halo::ai::ReferenceView::units_exit_vehicles; forwards to the C++ implementation unchanged.
 *
 * @address 0x433ea0
 */
void ai_reference_units_exit_vehicles(uint32_t packed_reference)
{
    halo::ai::ReferenceView(packed_reference).units_exit_vehicles();
}

/**
 * C entry point for halo::ai::AiObjects::refresh_unit_stimulus_and_alert; forwards to the C++ implementation unchanged.
 *
 * @address 0x42c2a0
 */
void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value)
{
    halo::ai::AiObjects::refresh_unit_stimulus_and_alert(object_index, priority, stimulus_value);
}

/**
 * C entry point for halo::ai::Encounters::release_actors_and_swarms; forwards to the C++ implementation unchanged.
 *
 * @address 0x428ea0
 */
void ai_release_actors_and_swarms()
{
    halo::ai::Encounters::release_actors_and_swarms();
}

/**
 * C entry point for halo::ai::EncounterView::release_actors_filtered; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ab00
 */
void ai_release_actors_filtered(datum_index encounter_index, int32_t platoon_index, int32_t squad_index, uint8_t is_dead)
{
    halo::ai::EncounterView(encounter_index).release_actors_filtered(platoon_index, squad_index, is_dead);
}

/**
 * C entry point for halo::ai::Encounters::release_inactive_encounters; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ae50
 */
int32_t ai_release_inactive_encounters(char *buffer, uint8_t *has_more, int16_t *state)
{
    return halo::ai::Encounters::release_inactive_encounters(buffer, has_more, state);
}

/**
 * C entry point for halo::ai::Encounters::release_inactive_swarms; forwards to the C++ implementation unchanged.
 *
 * @address 0x42abd0
 */
int ai_release_inactive_swarms(char *buffer, uint8_t *has_more)
{
    return halo::ai::Encounters::release_inactive_swarms(buffer, has_more);
}

/**
 * C entry point for halo::ai::AiSystem::reset_all_actors_perception; forwards to the C++ implementation unchanged.
 *
 * @address 0x429080
 */
void ai_reset_all_actors_perception()
{
    halo::ai::AiSystem::reset_all_actors_perception();
}

/**
 * C entry point for halo::ai::AiSystem::reset_fire_group_assignments; forwards to the C++ implementation unchanged.
 *
 * @address 0x42c940
 */
void ai_reset_fire_group_assignments()
{
    halo::ai::AiSystem::reset_fire_group_assignments();
}

/**
 * C entry point for halo::ai::AiSystem::reset_for_new_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a840
 */
void ai_reset_for_new_map()
{
    halo::ai::AiSystem::reset_for_new_map();
}

/**
 * C entry point for halo::ai::AiSystem::scan_for_recent_combat_activity; forwards to the C++ implementation unchanged.
 *
 * @address 0x42c3e0
 */
int32_t ai_scan_for_recent_combat_activity(uint8_t hard_difficulty)
{
    return halo::ai::AiSystem::scan_for_recent_combat_activity(hard_difficulty);
}

/**
 * C entry point for halo::ai::AiSearch::add_node; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b5a0
 */
int16_t ai_search_add_node(ai_search_context *context, int16_t parent, real_point2d *position, int32_t surface_index, int16_t point_id, uint8_t side, float base_cost)
{
    return halo::ai::AiSearch(context).add_node(parent, position, surface_index, point_id, side, base_cost);
}

/**
 * C entry point for halo::ai::ObstacleList::append_obstacle; forwards to the C++ implementation unchanged.
 *
 * @address 0x43c4b0
 */
uint8_t ai_search_append_obstacle(ai_search_obstacle_list *list, uint16_t flags, uint32_t object_index, real_point2d *position, float radius)
{
    return halo::ai::ObstacleList(list).append_obstacle(flags, object_index, position, radius);
}

/**
 * C entry point for halo::ai::AiSearchGeometry::choose_shorter_corner; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d240
 */
uint8_t ai_search_choose_shorter_corner(real_point2d *p, real_point2d *corner_a, real_point2d *q, real_point2d *corner_b, real_point2d *r, real_point2d *out_point)
{
    return halo::ai::AiSearchGeometry::choose_shorter_corner(p, corner_a, q, corner_b, r, out_point);
}

/**
 * C entry point for halo::ai::ObstacleList::compute_point_tangents; forwards to the C++ implementation unchanged.
 *
 * @address 0x43c9a0
 */
void ai_search_compute_point_tangents(ai_search_obstacle_list *list, int16_t point_index, real_point2d *position, real_vector2d *edge_neg, float radius, real_vector2d *out_a, real *out_b)
{
    halo::ai::ObstacleList(list).compute_point_tangents(point_index, position, edge_neg, radius, out_a, out_b);
}

/**
 * C entry point for halo::ai::AiSearch::context_init; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b790
 */
void ai_search_context_init(ai_search_context *context, uint8_t ignores_glass, uint32_t search_radius_bits, ai_search_obstacle_list *obstacles, real_point2d *origin, uint32_t structure_bsp, real_point2d *position, int32_t surface_index, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles)
{
    halo::ai::AiSearch(context).context_init(ignores_glass, search_radius_bits, obstacles, origin, structure_bsp, position, surface_index, origin_surface_index, final_leg, ignore_flagged_obstacles);
}

/**
 * C entry point for halo::ai::AiSearchGeometry::evaluate_edge_cost; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b830
 */
uint8_t ai_search_evaluate_edge_cost(void *context, uint8_t ignore_permission, ai_search_obstacle_list *obstacle_list, int16_t exclude_index, real_point2d *point, int32_t start_surface_index, float distance, float base_cost, uint8_t skip_direct, uint8_t apply_offset, uint8_t require_unflagged, ai_search_edge_result *out_result, real_vector2d *direction)
{
    return halo::ai::AiSearchGeometry::evaluate_edge_cost(context, ignore_permission, obstacle_list, exclude_index, point, start_surface_index, distance, base_cost, skip_direct, apply_offset, require_unflagged, out_result, direction);
}

/**
 * C entry point for halo::ai::AiSearch::expand_point_neighbors; forwards to the C++ implementation unchanged.
 *
 * @address 0x43ba60
 */
void ai_search_expand_point_neighbors(ai_search_context *context, int16_t node_index, int16_t start_point_id)
{
    halo::ai::AiSearch(context).expand_point_neighbors(node_index, start_point_id);
}

/**
 * C entry point for halo::ai::AiSearchGeometry::find_circle_portal_crossing; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d100
 */
void ai_search_find_circle_portal_crossing(real_point2d *center, real_point2d *portal, real_point2d *out_point, real_point2d *fallback_reference, float radius)
{
    halo::ai::AiSearchGeometry::find_circle_portal_crossing(center, portal, out_point, fallback_reference, radius);
}

/**
 * C entry point for halo::ai::AiSearchGeometry::find_circle_tangent_point; forwards to the C++ implementation unchanged.
 *
 * @address 0x43cf60
 */
void ai_search_find_circle_tangent_point(real_point2d *center, real_point2d *target, real_point2d *out_point, float radius, uint8_t side)
{
    halo::ai::AiSearchGeometry::find_circle_tangent_point(center, target, out_point, radius, side);
}

/**
 * C entry point for halo::ai::ObstacleList::find_covering_point; forwards to the C++ implementation unchanged.
 *
 * @address 0x43c890
 */
int16_t ai_search_find_covering_point(ai_search_obstacle_list *list, real_point2d *position, int16_t exclude_index, float extra_radius)
{
    return halo::ai::ObstacleList(list).find_covering_point(position, exclude_index, extra_radius);
}

/**
 * C entry point for halo::ai::ObstacleList::find_nearest_visible_point; forwards to the C++ implementation unchanged.
 *
 * @address 0x43c8f0
 */
uint8_t ai_search_find_nearest_visible_point(ai_search_obstacle_list *list, int16_t exclude_index, real_point2d *origin, real_vector2d *direction, float radius, float max_distance, uint8_t require_unflagged, ai_search_nearest_point_result *out_result)
{
    return halo::ai::ObstacleList(list).find_nearest_visible_point(exclude_index, origin, direction, radius, max_distance, require_unflagged, out_result);
}

/**
 * C entry point for halo::ai::ObstacleList::flood_fill_group; forwards to the C++ implementation unchanged.
 *
 * @address 0x43ca40
 */
void ai_search_flood_fill_group(ai_search_obstacle_list *list, float radius, uint32_t *out_bitmask, int16_t start_index)
{
    halo::ai::ObstacleList(list).flood_fill_group(radius, out_bitmask, start_index);
}

/**
 * C entry point for halo::ai::ObstacleList::gather_obstacles; forwards to the C++ implementation unchanged.
 *
 * @address 0x43c510
 */
void ai_search_gather_obstacles(ai_search_obstacle_list *list, real_point3d *center, float radius, real_vector3d *direction, uint32_t self_object_a, uint32_t self_object_b)
{
    halo::ai::ObstacleList(list).gather_obstacles(center, radius, direction, self_object_a, self_object_b);
}

/**
 * C entry point for halo::ai::AiSearch::heap_sift_down; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b4d0
 */
void ai_search_heap_sift_down(ai_search_context *context, int16_t index)
{
    halo::ai::AiSearch(context).heap_sift_down(index);
}

/**
 * C entry point for halo::ai::AiSearch::heap_sift_up; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b450
 */
void ai_search_heap_sift_up(ai_search_context *context, int16_t index)
{
    halo::ai::AiSearch(context).heap_sift_up(index);
}

/**
 * C entry point for halo::ai::ObstacleList::partition_into_groups; forwards to the C++ implementation unchanged.
 *
 * @address 0x43cb60
 */
void ai_search_partition_into_groups(ai_search_obstacle_list *list, float radius)
{
    halo::ai::ObstacleList(list).partition_into_groups(radius);
}

/**
 * C entry point for halo::ai::AiSearch::run; forwards to the C++ implementation unchanged.
 *
 * @address 0x43be20
 */
uint8_t ai_search_run(ai_search_context *context, uint8_t ignores_glass, ai_search_obstacle_list *obstacles, uint32_t search_radius_bits, real_point2d *position, int32_t surface_index, real_point2d *origin, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles)
{
    return halo::ai::AiSearch(context).run(ignores_glass, obstacles, search_radius_bits, position, surface_index, origin, origin_surface_index, final_leg, ignore_flagged_obstacles);
}

/**
 * C entry point for halo::ai::AiSearch::step; forwards to the C++ implementation unchanged.
 *
 * @address 0x43bcb0
 */
uint8_t ai_search_step(ai_search_context *context)
{
    return halo::ai::AiSearch(context).step();
}

/**
 * C entry point for halo::ai::AiCommunication::select_communication_target; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ec90
 */
int32_t ai_select_communication_target(uint32_t param_a, uint32_t param_b, int16_t line_id, int16_t sub_id, float *out_weight)
{
    return halo::ai::AiCommunication::select_communication_target(param_a, param_b, line_id, sub_id, out_weight);
}

/**
 * C entry point for halo::ai::Encounters::find_best_matching_member; forwards to the C++ implementation unchanged.
 *
 * @address 0x4333d0
 */
int32_t ai_squad_find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index, uint8_t *requested_actor_data, uint8_t *requested_actor_variant_data, char match_by_index)
{
    return halo::ai::Encounters::find_best_matching_member(packed_reference, requested_squad_index, requested_actor_data, requested_actor_variant_data, match_by_index);
}

/**
 * C entry point for halo::ai::Encounters::priority_compare; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ac90
 */
int __cdecl ai_squad_priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b)
{
    return halo::ai::Encounters::priority_compare(record_a, record_b);
}

/**
 * C entry point for halo::ai::Encounters::resolve_actor_type; forwards to the C++ implementation unchanged.
 *
 * @address 0x4374a0
 */
int16_t ai_squad_resolve_actor_type(ScenarioSquad *squad)
{
    return halo::ai::Encounters::resolve_actor_type(squad);
}

/**
 * C entry point for halo::ai::Encounters::merge; forwards to the C++ implementation unchanged.
 *
 * @address 0x433590
 */
void ai_squads_merge(uint32_t source_reference, uint32_t target_encounter_index, char notify, char is_platoon_merge)
{
    halo::ai::Encounters::merge(source_reference, target_encounter_index, notify, is_platoon_merge);
}

/**
 * C entry point for halo::ai::EncounterView::starting_location_derive_placement_flags; forwards to the C++ implementation unchanged.
 *
 * @address 0x436d40
 */
void ai_starting_location_derive_placement_flags(datum_index encounter_index, int16_t starting_location_index, uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d, int16_t *out_edx, int16_t *out_esi)
{
    halo::ai::EncounterView(encounter_index).starting_location_derive_placement_flags(starting_location_index, out_a, out_b, out_c, out_d, out_edx, out_esi);
}

/**
 * C entry point for halo::ai::AiSystem::target_distance_qsort_compare; forwards to the C++ implementation unchanged.
 *
 * @address 0x41d7a0
 */
int ai_target_distance_qsort_compare(void *record_a, void *record_b)
{
    return halo::ai::AiSystem::target_distance_qsort_compare(record_a, record_b);
}

/**
 * C entry point for halo::ai::AiSystem::tick_dispatcher; forwards to the C++ implementation unchanged.
 *
 * @address 0x42a900
 */
void ai_tick_dispatcher()
{
    halo::ai::AiSystem::tick_dispatcher();
}

/**
 * C entry point for halo::ai::AiSystem::unassigned_actors_attach_to_structure_bsp; forwards to the C++ implementation unchanged.
 *
 * @address 0x42ce90
 */
void ai_unassigned_actors_attach_to_structure_bsp()
{
    halo::ai::AiSystem::unassigned_actors_attach_to_structure_bsp();
}

/**
 * C entry point for halo::ai::AiUnitView::clear_actor_vocalization; forwards to the C++ implementation unchanged.
 *
 * @address 0x435a50
 */
void ai_unit_clear_actor_vocalization(datum_index unit_index)
{
    halo::ai::AiUnitView(unit_index).clear_actor_vocalization();
}

/**
 * C entry point for halo::ai::AiObjects::create_actor; forwards to the C++ implementation unchanged.
 *
 * @address 0x435420
 */
void ai_unit_create_actor(datum_index actor_variant_tag, datum_index unit_index)
{
    halo::ai::AiObjects::create_actor(actor_variant_tag, unit_index);
}

/**
 * C entry point for halo::ai::AiUnitView::dispatch_actor_event_d; forwards to the C++ implementation unchanged.
 *
 * @address 0x435a00
 */
void ai_unit_dispatch_actor_event_d(datum_index unit_index, int32_t unused)
{
    halo::ai::AiUnitView(unit_index).dispatch_actor_event_d(unused);
}

/**
 * C entry point for halo::ai::AiUnitView::flee_if_ready; forwards to the C++ implementation unchanged.
 *
 * @address 0x434df0
 */
void ai_unit_flee_if_ready(datum_index unit_index, uint32_t readiness_param)
{
    halo::ai::AiUnitView(unit_index).flee_if_ready(readiness_param);
}

/**
 * C entry point for halo::ai::AiUnitView::remap_actor_to_squad; forwards to the C++ implementation unchanged.
 *
 * @address 0x433970
 */
void ai_unit_remap_actor_to_squad(datum_index unit_index, uint32_t packed_reference, char notify)
{
    halo::ai::AiUnitView(unit_index).remap_actor_to_squad(packed_reference, notify);
}

/**
 * C entry point for halo::ai::AiUnitView::set_actor_force_active; forwards to the C++ implementation unchanged.
 *
 * @address 0x435540
 */
void ai_unit_set_actor_force_active(datum_index unit_index, uint8_t value)
{
    halo::ai::AiUnitView(unit_index).set_actor_force_active(value);
}

/**
 * C entry point for halo::ai::AiObjects::set_squad_reference; forwards to the C++ implementation unchanged.
 *
 * @address 0x435750
 */
void ai_unit_set_squad_reference(datum_index object_index, uint32_t packed_reference)
{
    halo::ai::AiObjects::set_squad_reference(object_index, packed_reference);
}

/**
 * C entry point for halo::ai::AiSystem::weighted_random_index; forwards to the C++ implementation unchanged.
 *
 * @address 0x432100
 */
int32_t ai_weighted_random_index(int16_t weight_offset, void *base, int16_t stride, uint16_t count, uint32_t *exclude_mask)
{
    return halo::ai::AiSystem::weighted_random_index(weight_offset, base, stride, count, exclude_mask);
}

/**
 * C entry point for halo::ai::EncounterView::activate; forwards to the C++ implementation unchanged.
 *
 * @address 0x437710
 */
uint8_t encounter_activate(datum_index encounter_index)
{
    return halo::ai::EncounterView(encounter_index).activate();
}

/**
 * C entry point for halo::ai::Encounters::add_actor; forwards to the C++ implementation unchanged.
 *
 * @address 0x436770
 */
void encounter_add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index, uint8_t keep_team)
{
    halo::ai::Encounters::add_actor(squad_index, actor_index, encounter_index, keep_team);
}

/**
 * C entry point for halo::ai::EncounterView::advance_grenade_timers; forwards to the C++ implementation unchanged.
 *
 * @address 0x438db0
 */
void encounter_advance_grenade_timers(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).advance_grenade_timers();
}

/**
 * C entry point for halo::ai::EncounterView::build_firing_position_claims; forwards to the C++ implementation unchanged.
 *
 * @address 0x4360d0
 */
void encounter_build_firing_position_claims(datum_index encounter_index, datum_index *out_claims)
{
    halo::ai::EncounterView(encounter_index).build_firing_position_claims(out_claims);
}

/**
 * C entry point for halo::ai::EncounterView::choose_vocalizations; forwards to the C++ implementation unchanged.
 *
 * @address 0x438580
 */
void encounter_choose_vocalizations(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).choose_vocalizations();
}

/**
 * C entry point for halo::ai::EncounterView::deactivate; forwards to the C++ implementation unchanged.
 *
 * @address 0x437870
 */
void encounter_deactivate(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).deactivate();
}

/**
 * C entry point for halo::ai::EncounterView::decay_squad_spawn_delays; forwards to the C++ implementation unchanged.
 *
 * @address 0x4392f0
 */
void encounter_decay_squad_spawn_delays(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).decay_squad_spawn_delays();
}

/**
 * C entry point for halo::ai::Encounters::definition_find_platoon_index_by_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x4322c0
 */
int32_t encounter_definition_find_platoon_index_by_name(ScenarioEncounter *encounter_definition, char *name)
{
    return halo::ai::Encounters::definition_find_platoon_index_by_name(encounter_definition, name);
}

/**
 * C entry point for halo::ai::Encounters::definition_find_squad_index_by_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x432260
 */
int32_t encounter_definition_find_squad_index_by_name(ScenarioEncounter *encounter_definition, char *name)
{
    return halo::ai::Encounters::definition_find_squad_index_by_name(encounter_definition, name);
}

/**
 * C entry point for halo::ai::EncounterView::evaluate_platoon_condition; forwards to the C++ implementation unchanged.
 *
 * @address 0x439f20
 */
uint8_t encounter_evaluate_platoon_condition(datum_index encounter_index, const ai_platoon_condition *condition)
{
    return halo::ai::EncounterView(encounter_index).evaluate_platoon_condition(condition);
}

/**
 * C entry point for halo::ai::EncounterView::evaluate_support_needs; forwards to the C++ implementation unchanged.
 *
 * @address 0x436dc0
 */
void encounter_evaluate_support_needs(datum_index encounter_index, datum_index self_actor_index, int16_t mode, uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked, uint8_t *out_a, uint8_t *out_b, uint8_t *out_reachable_a, uint8_t *out_reachable_b, uint8_t *out_any)
{
    halo::ai::EncounterView(encounter_index).evaluate_support_needs(self_actor_index, mode, phase, out_crowded, out_flanked, out_a, out_b, out_reachable_a, out_reachable_b, out_any);
}

/**
 * C entry point for halo::ai::EncounterView::gather_occupied_clusters; forwards to the C++ implementation unchanged.
 *
 * @address 0x436190
 */
void encounter_gather_occupied_clusters(datum_index encounter_index, uint32_t *out_clusters, uint8_t record_per_actor, uint32_t *other_clusters)
{
    halo::ai::EncounterView(encounter_index).gather_occupied_clusters(out_clusters, record_per_actor, other_clusters);
}

/**
 * C entry point for halo::ai::Encounters::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x437060
 */
void encounter_new(int16_t *squad_cursor, ScenarioEncounter *definition, int16_t *platoon_cursor)
{
    halo::ai::Encounters::create(squad_cursor, definition, platoon_cursor);
}

/**
 * C entry point for halo::ai::EncounterView::process_squad_reinforcements; forwards to the C++ implementation unchanged.
 *
 * @address 0x4390a0
 */
void encounter_process_squad_reinforcements(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).process_squad_reinforcements();
}

/**
 * C entry point for halo::ai::EncounterView::propagate_platoon_state_to_actors; forwards to the C++ implementation unchanged.
 *
 * @address 0x439d80
 */
void encounter_propagate_platoon_state_to_actors(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).propagate_platoon_state_to_actors();
}

/**
 * C entry point for halo::ai::EncounterView::recompute_morale; forwards to the C++ implementation unchanged.
 *
 * @address 0x437940
 */
void encounter_recompute_morale(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).recompute_morale();
}

/**
 * C entry point for halo::ai::EncounterView::redistribute_squads_toward_targets; forwards to the C++ implementation unchanged.
 *
 * @address 0x4394a0
 */
void encounter_redistribute_squads_toward_targets(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).redistribute_squads_toward_targets();
}

/**
 * C entry point for halo::ai::EncounterView::release_stale_props; forwards to the C++ implementation unchanged.
 *
 * @address 0x4382b0
 */
void encounter_release_stale_props(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).release_stale_props();
}

/**
 * C entry point for halo::ai::Encounters::remove_actor; forwards to the C++ implementation unchanged.
 *
 * @address 0x436620
 */
void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters)
{
    halo::ai::Encounters::remove_actor(actor_index, skip_counters);
}

/**
 * C entry point for halo::ai::EncounterView::set_team; forwards to the C++ implementation unchanged.
 *
 * @address 0x435b30
 */
void encounter_set_team(datum_index encounter_index, int16_t team)
{
    halo::ai::EncounterView(encounter_index).set_team(team);
}

/**
 * C entry point for halo::ai::EncounterView::spawn_squads; forwards to the C++ implementation unchanged.
 *
 * @address 0x437510
 */
void encounter_spawn_squads(datum_index encounter_index, int16_t platoon_filter, int16_t squad_filter)
{
    halo::ai::EncounterView(encounter_index).spawn_squads(platoon_filter, squad_filter);
}

/**
 * C entry point for halo::ai::EncounterView::squad_clear_spawn_delay; forwards to the C++ implementation unchanged.
 *
 * @address 0x439270
 */
void encounter_squad_clear_spawn_delay(datum_index encounter_index, int16_t squad_index)
{
    halo::ai::EncounterView(encounter_index).squad_clear_spawn_delay(squad_index);
}

/**
 * C entry point for halo::ai::EncounterView::squad_reset_starting_location_mask; forwards to the C++ implementation unchanged.
 *
 * @address 0x436f90
 */
void encounter_squad_reset_starting_location_mask(datum_index encounter_index, int16_t squad_index)
{
    halo::ai::EncounterView(encounter_index).squad_reset_starting_location_mask(squad_index);
}

/**
 * C entry point for halo::ai::EncounterView::squad_spawn_actor; forwards to the C++ implementation unchanged.
 *
 * @address 0x438e20
 */
uint8_t encounter_squad_spawn_actor(datum_index encounter_index, int16_t squad_index, uint32_t unit_type_index, uint32_t unused)
{
    return halo::ai::EncounterView(encounter_index).squad_spawn_actor(squad_index, unit_type_index, unused);
}

/**
 * C entry point for halo::ai::EncounterView::squad_spawn_reinforcement; forwards to the C++ implementation unchanged.
 *
 * @address 0x438f60
 */
uint32_t encounter_squad_spawn_reinforcement(datum_index encounter_index, int16_t squad_index)
{
    return halo::ai::EncounterView(encounter_index).squad_spawn_reinforcement(squad_index);
}

/**
 * C entry point for halo::ai::EncounterView::update_platoon_defending_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x4393b0
 */
void encounter_update_platoon_defending_flag(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).update_platoon_defending_flag();
}

/**
 * C entry point for halo::ai::Encounters::initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x435c00
 */
void encounters_initialize()
{
    halo::ai::Encounters::initialize();
}

/**
 * C entry point for halo::ai::Encounters::note_hostile_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x435f90
 */
void encounters_note_hostile_object(datum_index object_index)
{
    halo::ai::Encounters::note_hostile_object(object_index);
}

/**
 * C entry point for halo::ai::Encounters::recompute_dirty; forwards to the C++ implementation unchanged.
 *
 * @address 0x435f00
 */
void encounters_recompute_dirty()
{
    halo::ai::Encounters::recompute_dirty();
}

/**
 * C entry point for halo::ai::Encounters::reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x435cb0
 */
void encounters_reset()
{
    halo::ai::Encounters::reset();
}

/**
 * C entry point for halo::ai::Encounters::spawn_initial; forwards to the C++ implementation unchanged.
 *
 * @address 0x435d50
 */
void encounters_spawn_initial()
{
    halo::ai::Encounters::spawn_initial();
}

/**
 * C entry point for halo::ai::Encounters::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x435e00
 */
void encounters_update()
{
    halo::ai::Encounters::update();
}

/**
 * C entry point for halo::ai::Encounters::update_activation; forwards to the C++ implementation unchanged.
 *
 * @address 0x437e20
 */
void encounters_update_activation()
{
    halo::ai::Encounters::update_activation();
}

/**
 * C entry point for halo::ai::Encounters::find_nearest_squad_member; forwards to the C++ implementation unchanged.
 *
 * @address 0x41c2c0
 */
datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index, char stamp_group)
{
    return halo::ai::Encounters::find_nearest_squad_member(actor_index, reference, exclude_index, stamp_group);
}

/**
 * C entry point for halo::ai::PathFinder::compute_heuristic; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a310
 */
uint8_t path_find_compute_heuristic(path_find_context *context, uint32_t vertex_id, real_point3d *point, float *out_distance, float *out_secondary, real_vector3d *out_direction)
{
    return halo::ai::PathFinder(context).compute_heuristic(vertex_id, point, out_distance, out_secondary, out_direction);
}

/**
 * C entry point for halo::ai::PathFinder::context_init; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a700
 */
void path_find_context_init(path_find_context *context, const path_find_request *request, uint32_t second_param)
{
    halo::ai::PathFinder(context).context_init(request, second_param);
}

/**
 * C entry point for halo::ai::PathFinder::find_unobstructed_ancestor; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a220
 */
uint8_t path_find_find_unobstructed_ancestor(path_find_context *context, uint32_t vertex_id, real_point3d *point, uint8_t *out_used_start, real_point3d *out_position)
{
    return halo::ai::PathFinder(context).find_unobstructed_ancestor(vertex_id, point, out_used_start, out_position);
}

/**
 * C entry point for halo::ai::PathFindGeometry::gather_adjacent_edges; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b1c0
 */
int16_t path_find_gather_adjacent_edges(void *context, int32_t vertex_id, path_find_adjacent_edge *out_edges)
{
    return halo::ai::PathFindGeometry::gather_adjacent_edges(context, vertex_id, out_edges);
}

/**
 * C entry point for halo::ai::PathFinder::hash_lookup_vertex; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b2b0
 */
int16_t path_find_hash_lookup_vertex(path_find_context *context, uint32_t vertex_id)
{
    return halo::ai::PathFinder(context).hash_lookup_vertex(vertex_id);
}

/**
 * C entry point for halo::ai::PathFinder::heap_push; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b0f0
 */
void path_find_heap_push(path_find_context *context, int16_t node, int16_t key)
{
    halo::ai::PathFinder(context).heap_push(node, key);
}

/**
 * C entry point for halo::ai::PathFinder::heap_sift_down; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b010
 */
void path_find_heap_sift_down(path_find_context *context, int16_t index)
{
    halo::ai::PathFinder(context).heap_sift_down(index);
}

/**
 * C entry point for halo::ai::PathFinder::heap_sift_up; forwards to the C++ implementation unchanged.
 *
 * @address 0x43af70
 */
void path_find_heap_sift_up(path_find_context *context, int16_t index)
{
    halo::ai::PathFinder(context).heap_sift_up(index);
}

/**
 * C entry point for halo::ai::PathFindGeometry::heights_are_close; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d910
 */
uint8_t path_find_heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a, int32_t surface_b)
{
    return halo::ai::PathFindGeometry::heights_are_close(structure_bsp, point, surface_a, surface_b);
}

/**
 * C entry point for halo::ai::PathFinder::push_start_node; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a760
 */
uint8_t path_find_push_start_node(path_find_context *context)
{
    return halo::ai::PathFinder(context).push_start_node();
}

/**
 * C entry point for halo::ai::PathFinder::reconstruct_path; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a4d0
 */
uint8_t path_find_reconstruct_path(path_find_context *context, uint8_t *out_result)
{
    return halo::ai::PathFinder(context).reconstruct_path(out_result);
}

/**
 * C entry point for halo::ai::PathFinder::run; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a8b0
 */
uint8_t path_find_run(path_find_context *context)
{
    return halo::ai::PathFinder(context).run();
}

/**
 * C entry point for halo::ai::PathFinder::score_avoidance_penalty; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b3b0
 */
float path_find_score_avoidance_penalty(path_find_context *context, const real_point3d *segment_start, const real_point3d *segment_end, float *out_distance)
{
    return halo::ai::PathFinder(context).score_avoidance_penalty(segment_start, segment_end, out_distance);
}

/**
 * C entry point for halo::ai::PathFinder::set_avoid_sphere; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a070
 */
void path_find_set_avoid_sphere(path_find_context *context, const real_point3d *position, float avoid_radius, datum_index avoid_object_index, float avoid_weight)
{
    halo::ai::PathFinder(context).set_avoid_sphere(position, avoid_radius, avoid_object_index, avoid_weight);
}

/**
 * C entry point for halo::ai::PathFinder::set_goal; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a730
 */
void path_find_set_goal(path_find_context *context, const real_point3d *position, uint32_t goal_vertex_id, float goal_cost)
{
    halo::ai::PathFinder(context).set_goal(position, goal_vertex_id, goal_cost);
}

/**
 * C entry point for halo::ai::PathFinder::simplify_waypoints; forwards to the C++ implementation unchanged.
 *
 * @address 0x43cc00
 */
void path_find_simplify_waypoints(path_find_context *context, int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    halo::ai::PathFinder(context).simplify_waypoints(count, waypoints, out_count, out_waypoints, out_valid);
}

/**
 * C entry point for halo::ai::PathFindGeometry::test_direct_reachability; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a0a0
 */
uint8_t path_find_test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b, real_point3d *out_position, void *context, uint8_t *out_success)
{
    return halo::ai::PathFindGeometry::test_direct_reachability(point_a, point_b, out_position, context, out_success);
}

/**
 * C entry point for halo::ai::PathFindGeometry::test_segment_unobstructed; forwards to the C++ implementation unchanged.
 *
 * @address 0x43de90
 */
uint8_t path_find_test_segment_unobstructed(void *map, real_point3d *point_a, uint8_t ignore_permission, int32_t surface_a, real_point3d *point_b, int32_t surface_b, float radius, uint8_t flags, path_find_boundary_crossing *out_result)
{
    return halo::ai::PathFindGeometry::test_segment_unobstructed(map, point_a, ignore_permission, surface_a, point_b, surface_b, radius, flags, out_result);
}

/**
 * C entry point for halo::ai::PathFindGeometry::trace_bsp_boundary; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d9b0
 */
uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface, real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result)
{
    return halo::ai::PathFindGeometry::trace_bsp_boundary(map, ignore_permission, start, start_surface, end, target_surface, out_result);
}

/**
 * C entry point for halo::ai::PathFindGeometry::trace_cluster_boundary; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d4b0
 */
uint8_t path_find_trace_cluster_boundary(void *map, int32_t edge_index, real_point2d *origin, float radius, uint8_t side, uint8_t ignore_permission, real_point2d *out_point)
{
    return halo::ai::PathFindGeometry::trace_cluster_boundary(map, edge_index, origin, radius, side, ignore_permission, out_point);
}

/**
 * C entry point for halo::ai::PathFindGeometry::trace_cluster_boundary_from_vertex; forwards to the C++ implementation unchanged.
 *
 * @address 0x43d790
 */
uint8_t path_find_trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission, real_point2d *point, int32_t start_index, real_vector2d *direction, float max_distance, path_find_boundary_trace_result *out)
{
    return halo::ai::PathFindGeometry::trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_index, direction, max_distance, out);
}

/**
 * C entry point for halo::ai::PathFindGeometry::validate_and_record_goal; forwards to the C++ implementation unchanged.
 *
 * @address 0x43a190
 */
uint8_t path_find_validate_and_record_goal(ai_path_candidate_goal *candidate, void *context, uint32_t point_b, uint32_t unused_c, const real_point3d *position)
{
    return halo::ai::PathFindGeometry::validate_and_record_goal(candidate, context, point_b, unused_c, position);
}

/**
 * C entry point for halo::ai::PathFindGeometry::vertex_distance; forwards to the C++ implementation unchanged.
 *
 * @address 0x43b130
 */
float path_find_vertex_distance(ScenarioStructureBSP *structure_bsp, int32_t surface, real_point3d *point_a, real_point3d *out_point)
{
    return halo::ai::PathFindGeometry::vertex_distance(structure_bsp, surface, point_a, out_point);
}

/**
 * C entry point for halo::ai::ProjectileAim::get_aiming_vector; forwards to the C++ implementation unchanged.
 *
 * @address 0x4beec0
 */
uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag, real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override, uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed, real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line)
{
    return halo::ai::ProjectileAim::get_aiming_vector(target, speed_in, tag, origin, unused_param_3, max_time, max_speed_override, use_high_arc, out_direction, out_speed, out_time_or_fraction, out_range_or_length, out_used_straight_line);
}

/**
 * C entry point for halo::ai::ProjectileAim::solve_ballistic_arc; forwards to the C++ implementation unchanged.
 *
 * @address 0x4beb30
 */
uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin, real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc, real_vector3d *out_direction, real *max_speed_override, real *out_speed, real *out_time_of_flight, real *out_range, real *out_half_gravity_term, real *out_horizontal_speed)
{
    return halo::ai::ProjectileAim::solve_ballistic_arc(target, origin, speed_limit, gravity_scale, max_time, use_high_arc, out_direction, max_speed_override, out_speed, out_time_of_flight, out_range, out_half_gravity_term, out_horizontal_speed);
}

/**
 * C entry point for halo::ai::ProjectileAim::solve_straight_line; forwards to the C++ implementation unchanged.
 *
 * @address 0x4bee20
 */
uint8_t projectile_solve_straight_line(real_point3d *target, real_point3d *origin, real speed, real *out_time_of_flight, real_vector3d *out_direction, real *out_speed_echo, real *out_length)
{
    return halo::ai::ProjectileAim::solve_straight_line(target, origin, speed, out_time_of_flight, out_direction, out_speed_echo, out_length);
}

/**
 * C entry point for halo::ai::Encounters::find_encounter_index_by_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x432200
 */
int32_t scenario_find_encounter_index_by_name(Scenario *scenario, char *name)
{
    return halo::ai::Encounters::find_encounter_index_by_name(scenario, name);
}

/**
 * C entry point for halo::ai::ReferenceView::assign_team_and_request_order; forwards to the C++ implementation unchanged.
 *
 * @address 0x435590
 */
void squad_members_assign_team_and_request_order(uint32_t packed_reference, int16_t value)
{
    halo::ai::ReferenceView(packed_reference).assign_team_and_request_order(value);
}

/**
 * C entry point for halo::ai::ReferenceView::request_order; forwards to the C++ implementation unchanged.
 *
 * @address 0x435630
 */
void squad_members_request_order(uint32_t packed_reference, int16_t order_code)
{
    halo::ai::ReferenceView(packed_reference).request_order(order_code);
}

/**
 * C entry point for halo::ai::EncounterView::pick_random_starting_location; forwards to the C++ implementation unchanged.
 *
 * @address 0x437220
 */
int16_t squad_pick_random_starting_location(datum_index encounter_index, int16_t squad_index)
{
    return halo::ai::EncounterView(encounter_index).pick_random_starting_location(squad_index);
}

/**
 * C entry point for halo::ai::EncounterView::recent_object_get_or_create; forwards to the C++ implementation unchanged.
 *
 * @address 0x436c60
 */
datum_index squad_recent_object_get_or_create(datum_index encounter_index, int16_t type, int32_t min_last_tick, char create_if_missing)
{
    return halo::ai::EncounterView(encounter_index).recent_object_get_or_create(type, min_last_tick, create_if_missing);
}

/**
 * C entry point for halo::ai::EncounterView::recent_object_list_clear; forwards to the C++ implementation unchanged.
 *
 * @address 0x436c10
 */
void squad_recent_object_list_clear(datum_index encounter_index)
{
    halo::ai::EncounterView(encounter_index).recent_object_list_clear();
}

/**
 * C entry point for halo::ai::AiObjects::add_component; forwards to the C++ implementation unchanged.
 *
 * @address 0x4279a0
 */
void swarm_add_component(datum_index component_index, uint32_t unit_index, datum_index swarm_index)
{
    halo::ai::AiObjects::add_component(component_index, unit_index, swarm_index);
}

/**
 * C entry point for halo::ai::AiActorView::get_move_speed_for_range; forwards to the C++ implementation unchanged.
 *
 * @address 0x41bed0
 */
void unit_get_move_speed_for_range(datum_index actor_index, float param_a, float param_b, float param_dist, float *out_a, float *out_b)
{
    halo::ai::AiActorView(actor_index).get_move_speed_for_range(param_a, param_b, param_dist, out_a, out_b);
}

}
