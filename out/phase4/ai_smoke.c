#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

typedef char check_actor[(sizeof(actor) == 0x724) ? 1 : -1];
typedef char check_actor_order[(sizeof(actor_order) == 0x5c) ? 1 : -1];
typedef char check_actor_movement_action[(sizeof(actor_movement_action) == 0x18) ? 1 : -1];
typedef char check_actor_mode_definition[(sizeof(actor_mode_definition) == 0x38) ? 1 : -1];
typedef char check_swarm[(sizeof(swarm) == 0x98) ? 1 : -1];
typedef char check_swarm_component[(sizeof(swarm_component) == 0x40) ? 1 : -1];
typedef char check_prop[(sizeof(prop) == 0x138) ? 1 : -1];
typedef char check_encounter[(sizeof(encounter) == 0x6c) ? 1 : -1];
typedef char check_encounter_squad_state[(sizeof(encounter_squad_state) == 0x20) ? 1 : -1];
typedef char check_encounter_platoon_state[(sizeof(encounter_platoon_state) == 0x10) ? 1 : -1];
typedef char check_ai_pursuit[(sizeof(ai_pursuit) == 0x28) ? 1 : -1];
typedef char check_ai_conversation[(sizeof(ai_conversation) == 0x64) ? 1 : -1];
typedef char check_ai_conversation_event[(sizeof(ai_conversation_event) == 0x10) ? 1 : -1];
typedef char check_ai_globals[(sizeof(ai_globals) == 0x8dc) ? 1 : -1];
typedef char check_path_find_node[(sizeof(path_find_node) == 0x34) ? 1 : -1];
typedef char check_path_find_heap_entry[(sizeof(path_find_heap_entry) == 0x04) ? 1 : -1];
typedef char check_path_find_context[(sizeof(path_find_context) == 0x1008c) ? 1 : -1];
typedef char check_ai_search_node[(sizeof(ai_search_node) == 0x28) ? 1 : -1];
typedef char check_ai_search_context[(sizeof(ai_search_context) == 0x1532) ? 1 : -1];
typedef char check_ai_search_obstacle[(sizeof(ai_search_obstacle) == 0x14) ? 1 : -1];
typedef char check_ai_search_obstacle_list[(sizeof(ai_search_obstacle_list) == 0xa08) ? 1 : -1];
typedef char check_actor_movement_obstacle[(sizeof(actor_movement_obstacle) == 0x18) ? 1 : -1];
typedef char check_actor_movement_context[(sizeof(actor_movement_context) == 0x6048) ? 1 : -1];

// the actor offsets the constructor and the dispatchers pin, checked individually so a
// reordering fails here rather than silently shifting the whole record
typedef char check_actor_type[(__builtin_offsetof(actor, type) == 0x04) ? 1 : -1];
typedef char check_actor_unit_index[(__builtin_offsetof(actor, unit_index) == 0x18) ? 1 : -1];
typedef char check_actor_swarm_index[(__builtin_offsetof(actor, swarm_index) == 0x28) ? 1 : -1];
typedef char check_actor_encounter_index[(__builtin_offsetof(actor, encounter_index) == 0x34) ? 1 : -1];
typedef char check_actor_first_prop[(__builtin_offsetof(actor, first_prop) == 0x50) ? 1 : -1];
typedef char check_actor_mode[(__builtin_offsetof(actor, mode) == 0x6c) ? 1 : -1];
typedef char check_actor_mode_data[(__builtin_offsetof(actor, mode_data) == 0x9c) ? 1 : -1];
typedef char check_actor_facing[(__builtin_offsetof(actor, facing) == 0x174) ? 1 : -1];
typedef char check_actor_target_unit[(__builtin_offsetof(actor, target_unit_index) == 0x270) ? 1 : -1];
typedef char check_actor_firing_position[(__builtin_offsetof(actor, firing_position_index) == 0x3b8) ? 1 : -1];
typedef char check_actor_flags[(__builtin_offsetof(actor, flags) == 0x6d0) ? 1 : -1];
typedef char check_actor_override_target[(__builtin_offsetof(actor, override_target) == 0x720) ? 1 : -1];

typedef char check_swarm_units[(__builtin_offsetof(swarm, unit_index) == 0x18) ? 1 : -1];
typedef char check_swarm_components[(__builtin_offsetof(swarm, component_index) == 0x58) ? 1 : -1];
typedef char check_prop_next[(__builtin_offsetof(prop, next_in_actor) == 0x08) ? 1 : -1];
typedef char check_prop_object[(__builtin_offsetof(prop, object_index) == 0x18) ? 1 : -1];
typedef char check_prop_kind[(__builtin_offsetof(prop, kind) == 0x24) ? 1 : -1];
typedef char check_prop_distance[(__builtin_offsetof(prop, distance) == 0x11c) ? 1 : -1];
typedef char check_encounter_first_actor[(__builtin_offsetof(encounter, first_actor) == 0x14) ? 1 : -1];
typedef char check_encounter_first_pursuit[(__builtin_offsetof(encounter, first_pursuit) == 0x38) ? 1 : -1];
typedef char check_globals_events[(__builtin_offsetof(ai_globals, conversation_events) == 0x30) ? 1 : -1];
typedef char check_globals_vehicle_queue[(__builtin_offsetof(ai_globals, vehicle_entry_queue) == 0x8bc) ? 1 : -1];
typedef char check_path_nodes[(__builtin_offsetof(path_find_context, nodes) == 0x84) ? 1 : -1];
typedef char check_path_heap_count[(__builtin_offsetof(path_find_context, heap_count) == 0xd084) ? 1 : -1];
typedef char check_path_hash[(__builtin_offsetof(path_find_context, vertex_hash) == 0xe08a) ? 1 : -1];
typedef char check_search_nodes[(__builtin_offsetof(ai_search_context, nodes) == 0x30) ? 1 : -1];
typedef char check_search_heap[(__builtin_offsetof(ai_search_context, heap) == 0x1432) ? 1 : -1];
typedef char check_move_obstacles[(__builtin_offsetof(actor_movement_context, obstacles) == 0x40) ? 1 : -1];
typedef char check_move_radius[(__builtin_offsetof(actor_movement_context, search_radius) == 0x6044) ? 1 : -1];

// tag-side layouts this header leans on
typedef char check_scenario_encounters[(__builtin_offsetof(Scenario, encounters) == 0x42c) ? 1 : -1];
typedef char check_scenario_conversations[(__builtin_offsetof(Scenario, ai_conversations) == 0x468) ? 1 : -1];
typedef char check_scenario_encounter[(sizeof(ScenarioEncounter) == 0xb0) ? 1 : -1];
typedef char check_scenario_squad[(sizeof(ScenarioSquad) == 0xe8) ? 1 : -1];
typedef char check_scenario_platoon[(sizeof(ScenarioPlatoon) == 0xac) ? 1 : -1];
typedef char check_firing_position[(sizeof(ScenarioFiringPosition) == 0x18) ? 1 : -1];
typedef char check_ai_conversation_tag[(sizeof(ScenarioAIConversation) == 0x74) ? 1 : -1];


/* Added by the phase-4 ai review pass: the fields it split out of larger runs. */
typedef char check_actor_aim_origin[(__builtin_offsetof(actor, aim_origin) == 0x120) ? 1 : -1];
typedef char check_actor_body_position[(__builtin_offsetof(actor, body_position) == 0x12c) ? 1 : -1];
typedef char check_actor_active_unit_index[(__builtin_offsetof(actor, active_unit_index) == 0x158) ? 1 : -1];
typedef char check_actor_danger_segment_end[(__builtin_offsetof(actor, danger_segment_end) == 0x2c8) ? 1 : -1];
typedef char check_actor_danger_radius[(__builtin_offsetof(actor, danger_radius) == 0x2d8) ? 1 : -1];
typedef char check_actor_danger_center[(__builtin_offsetof(actor, danger_center) == 0x2dc) ? 1 : -1];
typedef char check_actor_unknown_5fc[(__builtin_offsetof(actor, unknown_5fc) == 0x5fc) ? 1 : -1];
typedef char check_actor_unknown_5fe[(__builtin_offsetof(actor, unknown_5fe) == 0x5fe) ? 1 : -1];
typedef char check_query[(sizeof(actor_firing_position_query) == 0x664) ? 1 : -1];
typedef char check_query_spheres[(__builtin_offsetof(actor_firing_position_query, danger_spheres) == 0x54) ? 1 : -1];
typedef char check_query_hazards[(__builtin_offsetof(actor_firing_position_query, hazards) == 0x25c) ? 1 : -1];
typedef char check_query_have_target[(__builtin_offsetof(actor_firing_position_query, have_target) == 0x5fc) ? 1 : -1];
typedef char check_query_baseline[(__builtin_offsetof(actor_firing_position_query, baseline_penalty) == 0x660) ? 1 : -1];
typedef char check_candidate[(sizeof(actor_firing_position_candidate) == 0x3c) ? 1 : -1];
typedef char check_candidate_valid[(__builtin_offsetof(actor_firing_position_candidate, valid) == 0x30) ? 1 : -1];
typedef char check_candidate_score[(__builtin_offsetof(actor_firing_position_candidate, score) == 0x38) ? 1 : -1];
typedef char check_hazard[(sizeof(actor_firing_position_hazard) == 0x1c) ? 1 : -1];
typedef char check_sphere[(sizeof(actor_firing_position_danger_sphere) == 0x10) ? 1 : -1];
typedef char check_rule[(sizeof(actor_firing_position_rule) == 8) ? 1 : -1];
typedef char check_path_find_request[(sizeof(path_find_request) == 0x48) ? 1 : -1];
typedef char check_path_find_request_start[(__builtin_offsetof(path_find_request, start_position) == 0x14) ? 1 : -1];
typedef char check_vocalization_context[(sizeof(actor_vocalization_context) == 0x10) ? 1 : -1];
typedef char check_combat_consideration[(sizeof(actor_combat_consideration) == 0x38) ? 1 : -1];
typedef char check_axis_request[(sizeof(actor_axis_request) == 0x18) ? 1 : -1];
typedef char check_range_lookup[(sizeof(ai_conversation_range_lookup) == 0x14) ? 1 : -1];

int ai_smoke(void) { return (int)(sizeof(actor) + sizeof(prop) + sizeof(encounter)); }

/* Added by the Opus phase-4 ai module review: the perception tally folded in from
   actor_choose_best_target @0x4203a0, and the prop point that was cut one dword low. */
typedef char check_actor_tally[(__builtin_offsetof(actor, tally) == 0x1ec) ? 1 : -1];
typedef char check_tally_size[(sizeof(actor_target_tally) == 0x7b) ? 1 : -1];
typedef char check_tally_hist[(__builtin_offsetof(actor_target_tally, by_threat_class) == 0x02) ? 1 : -1];
typedef char check_tally_a[(__builtin_offsetof(actor_target_tally, group_a_by_actor_type) == 0x17) ? 1 : -1];
typedef char check_tally_b[(__builtin_offsetof(actor_target_tally, group_b_by_actor_type) == 0x39) ? 1 : -1];
typedef char check_tally_c[(__builtin_offsetof(actor_target_tally, group_c_marked_by_actor_type) == 0x6b) ? 1 : -1];
typedef char check_actor_unknown_267[(__builtin_offsetof(actor, unknown_267) == 0x267) ? 1 : -1];
typedef char check_prop_surface[(__builtin_offsetof(prop, path_surface_index) == 0xec) ? 1 : -1];
typedef char check_prop_ground[(__builtin_offsetof(prop, ground_position) == 0xf0) ? 1 : -1];
typedef char check_prop_unknown_fc[(__builtin_offsetof(prop, unknown_fc) == 0xfc) ? 1 : -1];

// added by the phase-4 module review (encounter cluster): the shapes this session
// recovered or re-typed, so a later edit to types/ai.h cannot silently shift them.
typedef char check_encounter_iterator[(sizeof(encounter_iterator) == 0x18 + sizeof(void *) - 4) ? 1 : -1];
typedef char check_ai_scored_candidate[(sizeof(ai_scored_candidate) == 0x10) ? 1 : -1];
typedef char check_ai_object_attention_record[(sizeof(ai_object_attention_record) == 0x28) ? 1 : -1];
typedef char check_sq_mask[(__builtin_offsetof(encounter_squad_state, starting_location_mask) == 0x00) ? 1 : -1];
typedef char check_sq_free[(__builtin_offsetof(encounter_squad_state, starting_location_free) == 0x04) ? 1 : -1];
typedef char check_sq_respawn[(__builtin_offsetof(encounter_squad_state, respawn_budget) == 0x0c) ? 1 : -1];
typedef char check_sq_delay[(__builtin_offsetof(encounter_squad_state, squad_delay_ticks) == 0x12) ? 1 : -1];
typedef char check_sq_members[(__builtin_offsetof(encounter_squad_state, member_count) == 0x16) ? 1 : -1];
typedef char check_sq_vitality[(__builtin_offsetof(encounter_squad_state, average_vitality) == 0x1c) ? 1 : -1];
typedef char check_pl_vitality[(__builtin_offsetof(encounter_platoon_state, average_vitality) == 0x0c) ? 1 : -1];
typedef char check_enc_delay[(__builtin_offsetof(encounter, activation_delay) == 0x0e) ? 1 : -1];
typedef char check_enc_tick[(__builtin_offsetof(encounter, activation_tick) == 0x10) ? 1 : -1];
typedef char check_enc_snapshot[(__builtin_offsetof(encounter, unknown_1a) == 0x1a) ? 1 : -1];
typedef char check_enc_vitality[(__builtin_offsetof(encounter, average_vitality) == 0x34) ? 1 : -1];
typedef char check_actor_swarm_pending[(__builtin_offsetof(actor, swarm_pending) == 0x0b) ? 1 : -1];
typedef char check_prop_cluster[(__builtin_offsetof(prop, cluster_index) == 0x100) ? 1 : -1];
typedef char check_swarm_component_flags[(__builtin_offsetof(swarm_component, flags) == 0x02) ? 1 : -1];
// reconciliation R06 / R53
typedef char check_pfc_structure_bsp[(__builtin_offsetof(path_find_context, structure_bsp) == 0x64) ? 1 : -1];
typedef char check_amc_structure_bsp[(__builtin_offsetof(actor_movement_context, structure_bsp) == 0x00) ? 1 : -1];
typedef char check_amc_collision_bsp[(__builtin_offsetof(actor_movement_context, collision_bsp) == 0x04) ? 1 : -1];
typedef char check_trace_result[(sizeof(path_find_boundary_trace_result) == 0x0c) ? 1 : -1];
typedef char check_trace_edge[(__builtin_offsetof(path_find_boundary_trace_result, edge_index) == 0x08) ? 1 : -1];
typedef char check_edge_result[(sizeof(ai_search_edge_result) == 0x10) ? 1 : -1];
typedef char check_edge_result_surface[(__builtin_offsetof(ai_search_edge_result, surface_index) == 0x04) ? 1 : -1];
typedef char check_edge_result_link[(__builtin_offsetof(ai_search_edge_result, link) == 0x0e) ? 1 : -1];
