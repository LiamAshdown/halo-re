#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "halo/ai/record_layout.hpp"
#include "game.h"
#include "networking.h"
#include "physics.h"
#include "projectiles.h"

namespace halo::ai {

/**
 * Non-owning handle to an actor record in the actor data array. It carries only the datum index and is never
 * stored in game state; each member runs the engine behaviour that used to be a free function taking the actor index first.
 */
class ActorView {
public:
    datum_index actor_index;

    explicit constexpr ActorView(datum_index handle) : actor_index(handle) {}

    void mode_uncover_tick();
    void mode_uncover_update();
    void mode_vehicle_enter();
    void mode_vehicle_update();
    uint8_t mode_wait_process();
    void mode_wait_tick();
    void mode_wait_update();
    void movement_action_cancel();
    uint8_t movement_action_in_progress();
    uint8_t movement_action_is_complete();
    uint8_t movement_action_resolve(uint8_t record_distance, path_find_context *context);
    void movement_action_stop();
    void movement_actions_cancel();
    void movement_advance_waypoint();
    uint8_t movement_check_arrival();
    void movement_choose_avoidance_direction(real_vector3d *desired, real_vector3d *out_direction, float *out_scale);
    uint8_t movement_flying_needs_steering(const real_point3d *destination, float *out_avoidance_distance);
    void movement_get_stopping_distances(float *out_accelerate_stop_distance, float *out_stop_distance);
    uint8_t movement_set_destination_firing_position(int16_t formation_slot, path_find_context *path_context);
    uint8_t movement_set_destination_move_position(int16_t move_position_index);
    void movement_update();
    void notify_squad_and_flag_danger(uint8_t alternate_event, uint8_t raise_danger_flag);
    void obey_member_advance(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra);
    void obey_member_enter(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra);
    void obey_member_exit(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra);
    void obey_member_tick(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra);
    uint8_t probe_step_direction(float step_distance, real_vector2d *direction, uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param);
    uint8_t process_order_request(uint16_t order_code);
    uint8_t process_pending_command_list();
    uint8_t process_vehicle_seat_exit();
    void prop_iterator_init(actor_prop_iterator *out_iterator);
    void propagate_unit_field(int16_t value);
    void push_recognition_entry(int16_t firing_position_index, uint8_t type);
    void queue_recognized_target_dialogue(datum_index target_prop_index);
    void queue_search_position(real_point3d *position, int16_t priority, real_vector3d *velocity, uint32_t surface_index, uint32_t position_extra, uint32_t velocity_ticks, uint32_t prop_index, uint32_t prop_value, uint8_t prop_flag);
    uint8_t queue_secondary_action(int16_t action, const real_vector2d *direction);
    void queue_sighted_target_dialogue(datum_index target_prop_index, uint8_t already_noticed);
    void raise_timer_5f6(int32_t ticks);
    float rate_potential_target(datum_index target_prop_index);
    uint8_t react_to_disturbance(int16_t threshold);
    void react_to_flee_point(int32_t flee_source_object, const real_point3d *point);
    void react_to_seen_target(datum_index target_prop_index);
    void recompute_grenade_eligibility();
    void record_look_at_point(const uint32_t *point, int16_t priority, uint32_t data);
    void record_perception_event(int16_t event, int32_t data);
    void refresh_combat_context();
    uint8_t reject_firing_position_by_perception(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    uint8_t reject_firing_position_by_pursuit(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    uint8_t reject_firing_position_by_request_result(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    uint8_t reject_firing_position_by_target_approach(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    uint8_t reject_firing_position_unreachable(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    void release_from_cluster_or_delete(datum_index unit_index);
    void remove_from_unit_cluster(datum_index unit_index);
    void replace_object_reference(uint32_t new_reference, uint32_t old_reference);
    int32_t report_command_status();
    void report_firing_position_request(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    uint8_t request_move_and_face();
    uint8_t request_path_with_grenade_arc();
    void reseed_movement_pause_timer();
    uint8_t reset_queued_look_vector();
    void reset_squad_link_for_type_change(datum_index encounter_index, int16_t squad_index);
    uint8_t resolve_wander_or_look_direction(real_vector3d *out_direction);
    void run_mode_transition_loop();
    void run_movement_action_complete();
    uint8_t scale_value_by_ally_exposure(float *value);
    void scan_allies_for_backup_request();
    void schedule_grenade_throw();
    uint8_t score_blast_area_clear(float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count);
    void score_firing_positions_by_history(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    void score_firing_positions_by_range(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    void score_firing_positions_by_standoff(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    void score_firing_positions_by_threat(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    void score_firing_positions_close_range(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    void score_firing_positions_near_target(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates);
    uint8_t seek_vehicle_to_board();
    uint8_t select_facing_target_prop(uint8_t require_trust, uint8_t skip_lane_test, actor_recognition_scan_result *out_result, uint8_t *out_in_front);
    int16_t select_firing_position(actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok);
    int32_t select_move_position(int16_t select_mode, int32_t position_index, uint8_t *direction_flag);
    void select_stance_offset_pair(ActorVariant *base, actor_burst_parameters **out_a, actor_burst_scale **out_b);
    void set_combat_alert_flag(uint8_t new_flag);
    void set_flag_bit1();
    void set_mode(int32_t mode, void *mode_data);
    void set_override_target(uint8_t enable, datum_index override_target);
    void set_units_active(uint8_t dormant);
    uint8_t should_hold_position(const ActorVariant *definition);
    uint8_t should_throw_grenade(char force);
    void snapshot_orientation();
    uint32_t solve_grenade_lob(real_point3d *point);
    void squad_action_list_process(uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state, actor_command_aim *aim_state, uint8_t *finished_flag);
    void squad_action_reset_entry(uint32_t check_object_index, actor_squad_action_state *state, int16_t command_list_index, actor_command_aim *aim_state, uint8_t *next_action_index_out);
    int32_t squad_action_status_broadcast(int16_t command_list_index, actor_mode_obey_data *record);
    void squad_react_to_grenade(datum_index target_prop_index, int16_t grenade_type);
    void start_search_timer(datum_index prop_index);
    void swarm_for_each_component(char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, actor_mode_obey_data *obey);
    void swarm_for_each_component_thunk();
    uint8_t target_data_acquire(datum_index object_index, datum_index owner_reference, datum_index pair_reference);
    void target_data_refresh(uint32_t target_prop_index, actor_firing_positions *reference, char force, char allow_reassign);
    void target_evaluate_squad_link(datum_index object_index, ai_target_candidate_list *candidates_a, ai_target_candidate_list *candidates_b);
    uint16_t target_get_priority_class(datum_index target_prop_index);
    uint8_t target_has_conflicting_neighbor(datum_index target_prop_index);
    uint8_t target_is_visible_or_object_count_ok(int16_t kind);
    void target_relationship_think();
    void target_reset_seen_flags();
    void target_reset_shot_counters();
    void target_scan_potential_targets();
    uint8_t target_update_active_flag(datum_index target_prop_index);
    void target_update_tracking_speed(datum_index target_prop_index, actor_firing_positions *scratch);
    uint8_t try_grenade_evasion(uint8_t allow_pain_reaction, uint8_t use_alt_base);
    void type_crew_update();
    void type_elite_update();
    void type_engineer_update();
    void type_flood_carrier_update();
    void type_flood_update();
    void type_grunt_update();
    void type_hunter_update();
    void type_infection_swarm_update();
    void type_infection_update();
    void type_jackal_update();
    void type_marine_update();
    void type_mounted_weapon_update();
    void type_sentinel_update();
    void unlink_prop(datum_index prop_to_remove);
    void unlink_unit();
    void update_activation_state();
    void update_aim_wander();
    void update_awareness_level();
    uint8_t update_combat_behavior(uint8_t param_1, uint8_t param_2);
    void update_crouch_state();
    uint8_t update_danger_avoidance();
    void update_facing_change_timer();
    void update_firing_state();
    uint8_t update_flee_response();
    char update_grenade_and_morale_reactions();
    void update_grenade_eligibility_state();
    uint8_t update_grenade_throw_decision();
    void update_idle_stagger();
    void update_look_target();
    uint8_t update_melee_combat_action();
    uint8_t update_movement_destination();
    uint8_t update_path_if_needed();
    uint8_t update_special_mode();
    uint8_t update_squad_link_state();
    void update_target_combat_status();
    void update_target_lead_position();
    uint8_t validate_grenade_impact_point(real_point3d *candidate_point);
    uint8_t vehicle_not_recently_left(datum_index vehicle_index);
    uint8_t wants_reload_or_swap();
};

/**
 * Non-owning handle to a prop (target data) record in the prop data array, carried by its datum index.
 */
class TargetView {
public:
    datum_index target_prop_index;

    explicit constexpr TargetView(datum_index handle) : target_prop_index(handle) {}

    uint8_t movement_set_destination_near_target(datum_index actor_index, float radius);
    void notify_target_engaged(datum_index actor_index, uint8_t alternate_event);
    void scan_ally_death_panic_reaction(datum_index actor_index);
    void scan_backup_and_panic_reaction(datum_index actor_index);
    void set_target_alert_stage1(datum_index actor_index);
    void set_target_alert_stage2(datum_index actor_index);
    void set_target_alert_stage3(datum_index actor_index);
    uint32_t target_data_release(uint32_t actor_index, uint8_t *out_conflict_flag);
    uint8_t target_get_backup_priority();
    void target_get_relationship_object();
    void target_mark_engaged(datum_index actor_index, uint8_t mark_engaged);
    void target_reset_combat_flags(datum_index actor_index, uint32_t unused, uint8_t already_noticed);
};

/**
 * Actor AI operations that are not tied to a single actor handle: they take points, tags, object indices or
 * iterators as their first operand. Static members, grouped here instead of loose free functions.
 */
class ActorOps {
public:
    static void movement_apply_steering(int16_t cached_axis, uint8_t keep_z, datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed, float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale, float throttle_maximum, real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis, real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506);
    static void movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d, const real_vector3d *facing, const real_vector3d *reference, real_vector3d *out_axis, int16_t *out_index);
    static void movement_collect_obstacle_candidates(actor_movement_context *context);
    static void movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis, const real_vector3d *v, real_vector3d *out);
    static uint8_t movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra);
    static int16_t movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample, real_point3d *out_end_point, actor_movement_context *context, float *out_distance, uint8_t *out_clear_counter);
    static datum_index new_and_attach_to_unit(char reuse_existing, datum_index unit_index, datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor, char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68);
    static void notify_squad_of_threat_direction(const real_point3d *point, datum_index actor_index, int16_t event_kind, int16_t grenade_type_code);
    static void notify_weapon_pickup_once(datum_index object_index);
    static int32_t order_code_is_grenade_throw(int16_t order_code);
    static int32_t pick_dialogue_variant_a(int16_t category);
    static int32_t pick_dialogue_variant_b(int16_t category);
    static datum_index place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index, int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index, const actor_placement_request *placement_request);
    static uint8_t play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index, char *seat_name, int16_t seat_flags, int16_t count);
    static uint8_t point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis, float min_cos_threshold, float side_thresholds[2]);
    static prop * prop_iterator_next(actor_prop_iterator *iterator);
    static void queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index, datum_index actor_index);
    static void queue_point_reaction_dialogue(const real_point3d *point, datum_index actor_index);
    static void queue_search_and_relay_perception(datum_index prop_index, datum_index actor_index);
    static void queue_velocity_search_from_prop(datum_index prop_index, datum_index actor_index);
    static void react_to_registered_danger(const real_point3d *point, datum_index actor_index, int32_t danger_object_index);
    static void react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay);
    static int32_t reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index, int32_t seat_selector);
    static void reset_perception_scratch(datum_index unit_index);
    static uint8_t resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out, datum_index actor_index);
    static uint8_t resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table, uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback);
    static datum_index run_new(datum_index actor_variant_tag);
    static int16_t spawn_additional_units(datum_index actor_variant_tag, int16_t spawn_count, datum_index source_actor_index, float health_scale);
    static char squad_action_execute(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state);
    static uint8_t squad_action_is_complete(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state);
    static void squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index, datum_index other_object_index);
    static uint8_t take_danger_escape(real_vector3d *path_delta, datum_index actor_index, uint16_t direction_kind, float step_distance, float distance);
    static uint16_t target_hearing_check(const bsp_leaf_reference *record, int16_t stance, datum_index actor_index, const actor_firing_positions *target_ref, int16_t gate, real_point3d *listener_position);
    static uint8_t target_is_close_and_recognized(datum_index object_index, uint32_t unused, datum_index actor_index);
    static uint8_t targets_share_descriptor(datum_index actor_a, datum_index actor_b);
    static uint8_t toggle_active_state(uint8_t activate, datum_index actor_index);
    static void update_swarm_component_position(datum_index component_index, datum_index unit_index);
    static uint8_t validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag);
};

}
