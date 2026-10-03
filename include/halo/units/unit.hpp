#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "models.h"

namespace halo::units {

struct ai_update_stagger_state {
    int16_t threshold;
    int16_t highest;
    uint8_t claimed;
};

static_assert(sizeof(ai_update_stagger_state) == 6);

/**
 * Non-owning handle to a unit object in the object data array. It carries only the datum index, so it is the
 * same size as the index and never stored in game state; every member forwards to the engine behaviour that
 * used to be a free function taking the unit index first.
 */
class UnitView {
public:
    uint32_t datum_handle;

    explicit constexpr UnitView(uint32_t datum) : datum_handle(datum) {}

    void clear_ground_adjust_dirty();
    void reset_ground_adjust_state();

    int32_t animation_change_priority_check(uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index, int32_t *chain_value);
    uint8_t choose_combat_reaction_animation(const datum_index *reaction_source, uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias);
    uint8_t dispatch_reaction_animation(int16_t reaction_code);
    void evaluate_flee_reaction();
    void fire_animation_sound_trigger(uint32_t trigger_kind, int16_t contact_point_index);
    int32_t get_animation_frames_remaining(int16_t *out_animation_state);
    int32_t get_custom_animation_time_remaining();
    uint8_t is_in_busy_animation_state();
    void play_default_reaction_sound(datum_index sound_tag, datum_index sound_handle);
    void region_damage_reaction(uint32_t unused, uint32_t flags);
    uint8_t scripted_action_animation_exists(int16_t command);
    void scripting_set_emotion_animation(const char *emotion_name);
    void set_custom_animation(datum_index graph, int16_t animation_index);
    uint8_t set_custom_animation_frame(uint8_t warn_if_missing, datum_index graph_tag_id, const char *animation_name, int16_t frame);
    void start_seat_overlay_animation_a(int16_t command);
    void start_seat_overlay_animation_b(int16_t command);
    uint8_t start_user_animation(datum_index graph_tag, const char *animation_name, uint8_t interpolate);
    uint8_t try_set_animation_state(int16_t new_state);
    uint8_t try_start_scripted_action_animation(int16_t command, const real_vector2d *direction);
    uint16_t update_animation_state_machine(const int8_t *request);
    void update_animation_timers();
    void update_footstep_and_idle_triggers();
    void update_ik_detail_nodes(void *node_base);

    void accumulate_clamped_offset(float new_value);
    void apply_control_block(const unit_control_data *control, int32_t source_id);
    uint8_t clamp_direction_to_aim_or_look_bounds(real_vector3d *world_direction, uint8_t use_aiming_bounds);
    void get_aiming_vector(real_vector3d *out);
    void get_camera_position(real_point3d *out);
    void get_look_origin_and_direction(uint32_t *out_autoaim_width, real_vector3d *out_direction, real_point3d *out_origin);
    void initialize_random_turn_angle();
    uint8_t is_look_target_valid();
    int32_t predict_aim_target_position(real_point3d *out_position);
    void project_onto_aiming_axis(real *out_speed, uint8_t project_point, uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis);
    void reset_orientation_and_find_position(uint32_t vehicle_index);
    void rotate_basis_about_axis();
    void sample_camera_shake_from_velocity();
    void set_control_countdown(int32_t countdown, uint32_t extra_control_flags);
    void set_facing_from_index_table();
    void track_target_lock_timeout();
    void update_aiming_overlay_angles(void *output);
    void update_autoaim_interaction();
    void update_look_delta_controls();
    void update_random_turn_angle(real_vector3d *out_axis);
    void update_stance_and_jump(uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still);
    void update_steering_deviation_effects(real_vector3d *reference_direction, uint8_t *contact_points);

    void apply_impulse(real_vector3d *impulse);
    void can_see_point(real_vector3d *target_direction, real_vector3d *perp, real_vector3d *up);
    void check_fell_off_level();
    void forget_object_reference(datum_index forgotten_object_index);
    uint32_t get_biped_specific_value();
    uint32_t get_flag_bit6();
    uint8_t get_recently_updated_flag();
    uint32_t get_tag_flag_bit7();
    uint8_t has_child_of_type5();
    uint8_t update();
    void update_ground_contact_counter(uint8_t *contact_points);

    void apply_damage_effects(damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local);
    void apply_fall_damage(float fall_speed);
    void cause_melee_damage(uint8_t suppress_effect, uint32_t target_object_index, int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7);
    void enter_stunned_state(uint32_t responsible_object);
    void melee_attack_scan();
    void melee_lunge_damage_tick();
    void record_recent_damage_and_react(float damage_amount, int16_t response_index, uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object);
    void update_recoil_decay();
    void update_vitality_fractions(float body_delta, float shield_delta);

    void choose_dialogue_variant();
    int32_t commit_speech(const unit_speech *source, int16_t mode);
    void dialogue_determine_variant();

    void add_marker_relative_offset(uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator);
    void calculate_luminosity();
    void compute_marker_offset_position(real_vector3d *reference_direction, int16_t mode, real_point3d *out_position, float *base_position, float *offsets);
    uint8_t get_average_active_marker_direction(real_vector3d *out_direction);
    void get_forward_vector_or_marker_normal(real_vector3d *out);
    void get_primary_eye_marker_position(real_point3d *out);
    void get_secondary_eye_marker_position(real_point3d *out);
    void update_marker_skid_effects(uint8_t *contact_points);
    uint32_t update_marker_traction_effects();

    void apply_scale_change(unit_scale_request *request);
    void find_nearest_valid_surface_plane();
    uint8_t new_();
    uint32_t resolve_camera_object();
    int32_t pick_random_spawned_actor_count();
    void place(uint8_t *placement);
    void recalculate_position();
    void release_transient_state(uint8_t is_light_reset);
    void reset_velocity_and_ground_flag(uint8_t set_flag);
    uint32_t snap_to_min_ground_height();
    int32_t test_placement_candidate(const real_vector3d *direction, real_vector3d *out_normal, float distance, real_point3d *out_position);
    void update_scale_function_inputs();

    void apply_network_health_update(void *message);
    int32_t build_network_update(int32_t buffer, int32_t bit_budget);
    int32_t submit_periodic_network_update(void *buffer, int32_t bit_budget, int32_t update_type);

    uint8_t all_seats_unoccupied();
    uint8_t any_flagged_seat_occupied();
    void apply_impulse_to_seat(real_vector3d *impulse);
    datum_index build_seat_occupant_zone_list();
    void detach_and_enter_named_seat(uint32_t target_parent_index, char *seat_marker_name);
    int16_t detach_child_at_named_seat(char *seat_marker_name);
    void detach_from_seat(uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
    void detach_reposition_and_nudge();
    void dispatch_seat_overlay_command(int16_t command);
    uint16_t find_best_seat_to_enter(uint32_t vehicle_index, int16_t *out_seat);
    int16_t find_next_zone_permitted_weapon_slot(int32_t start_slot, int16_t direction);
    int16_t find_seats_matching_name_and_flags(char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices);
    char * get_seat_or_state_name();
    uint8_t is_child_seated_at_named_marker(char *seat_label, uint32_t child_object_index);
    uint8_t is_seat_control_available(int16_t command);
    uint8_t named_seat_occupant_in_zone(char *seat_label, uint32_t zone_list_index);
    void recompute_seat_occupants();
    void release_transient_state_and_detach(uint8_t is_light_reset);
    uint8_t seat_flag_bit10(int16_t seat_index);
    uint8_t seat_flag_bit2(int16_t seat_index);
    uint8_t seat_flag_bit3(int16_t seat_index);
    uint8_t set_or_test_seat_and_weapon_label(const char *seat_label, const char *weapon_label, uint8_t apply);
    void try_exit_controlled_seat();

    void add_initial_weapons();
    uint8_t begin_throw_grenade(const real_vector2d *direction);
    uint8_t check_weapon_use_permission(uint32_t weapon_index);
    void clear_selected_equipment();
    int16_t count_deployed_weapons();
    uint8_t current_weapon_has_flag();
    uint8_t current_weapon_is_type(datum_index weapon_tag_id);
    uint8_t current_weapon_type_is_2_or_3();
    uint8_t drop_current_weapon(uint8_t force);
    void drop_grenades();
    void drop_inventory_weapons();
    void drop_inventory_weapons_except_current();
    void drop_object_from_hand(uint32_t object_index);
    int16_t find_empty_weapon_slot();
    int32_t find_next_grenade_type_with_count(int32_t start_index, int16_t direction);
    uint16_t find_weapon_index_by_flag(uint8_t flag_bit);
    uint16_t find_weapon_index_with_fixed_flag();
    uint8_t find_weapon_marker_transform(uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint);
    float get_active_weapon_scale(int16_t zoom_level);
    int8_t get_current_grenade_index();
    char * get_current_weapon_label();
    int32_t get_grenade_count(int16_t grenade_type);
    uint8_t get_weapon_marker_indices(uint8_t use_alternate, uint32_t out_dx_to_key_frame, uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index);
    datum_index get_weapon_object_index(int16_t slot_index);
    uint8_t has_weapon_of_type(int32_t weapon_group_tag);
    void notify_weapon_removed();
    void notify_weapon_removed_dup();
    void pick_and_ready_next_weapon();
    void ready_desired_weapon(uint8_t force);
    void refresh_targeting_flag_and_weapons(uint8_t initial_targeting_flag);
    void release_selected_equipment();
    void release_thrown_grenade(uint8_t early);
    int32_t set_grenade_type_and_count_delta(int16_t grenade_type, int8_t delta);
    void set_throw_aim_direction(const real_vector2d *direction_xy);
    void throw_grenade_move_to_hand();
    uint8_t try_ready_weapon(uint8_t forced, const real_vector2d *direction);
    uint8_t try_ready_weapon_variant(const real_vector2d *direction);
    uint8_t try_select_equipment(uint32_t new_equipment_object_index, int16_t release_current);
    void validate_and_clear_weapon_switch();

};

/**
 * Handle to a biped unit. Adds the biped specific behaviour (movement solver, ground adjust, idle and evade
 * logic) on top of the common unit behaviour it inherits.
 */
class BipedView : public UnitView {
public:
    explicit constexpr BipedView(uint32_t datum) : UnitView(datum) {}

    void advance_frame_counter_trigger(char *state_out);
    void apply_idle_fidget(uint8_t *state_out);
    void check_evade_reaction();
    void clear_ground_surface_references();
    uint8_t create();
    datum_index get_cached_look_at_position(real_point3d *out_position);
    void integrate_movement(object *obj, int8_t *state);
    void integrate_movement_with_collision(int8_t *state);
    uint32_t is_idle_eligible();
    uint8_t is_old_enough();
    void placement_offset_centered_pill(object_placement_data *placement);
    void reset_state();
    void trigger_on_velocity_threshold();
    uint8_t update();
    void update_facing(int8_t *out_animation_state);
    void update_idle_basis(uint8_t *state_out);
    void update_scale_function_inputs();

    void ground_adjust_apply_node_rotations(real_matrix4x3 *nodes, real_point3d *saved_positions);
    void ground_adjust_solve(real_matrix4x3 *nodes);
    char ground_adjust_solve_node(real_point3d *reference_position, int32_t node_index, real_matrix4x3 *nodes, real_point3d *own_position, uint32_t *success_bits);
    uint32_t ground_adjust_step();

    void network_baseline_take();

};

/**
 * Handle to a vehicle unit. Adds the vehicle specific behaviour (control blending, hover and wing controls,
 * network encode/decode) on top of the common unit behaviour it inherits.
 */
class VehicleView : public UnitView {
public:
    explicit constexpr VehicleView(uint32_t datum) : UnitView(datum) {}

    uint8_t create();
    uint8_t is_old_enough();
    void reset_state();
    uint32_t update();

    void blend_animations(real_orientation *orientations);
    void calculate_animation_controls();
    void calculate_ground_contact_lean(void *out_record, void *out_transform);
    void calculate_ground_contact_lean_alt(void *out_record, void *out_transform);
    void calculate_ground_lean_controls(uint8_t *out_transform);
    void calculate_lean_controls(void *mass_points, float *powered_states);
    void calculate_mounted_controls_dispatch(void *out_transform, void *out_record);
    void calculate_steering_wheel_controls(void *mass_points, float *powered_states);
    void calculate_turret_controls(void *mass_points, float *powered_states);
    void calculate_wing_flex_controls(float angle, uint8_t *node_output, uint8_t *contact_points);
    void create_hover_thruster_effects();
    void create_hover_thruster_midpoint_effects();

    void apply_network_update(void **message, uint8_t *connection);
    int32_t encode_network_create(int32_t buffer, int32_t bit_budget);
    int32_t encode_network_update(void *buffer, int32_t bit_budget, int32_t full_update);
    void network_baseline_take();

};

void biped_movement_solve(biped_movement_solver_data *solve);
void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base);
void biped_update_target_lock_timer(datum_index target, uint32_t object_index);
void biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base, float magnitude, float dir_x, float dir_y, float dir_z, char already_idle, uint8_t *state_out);
void unit_ai_update_stagger_allocate(void);
void unit_ai_update_stagger_reset(void);
void unit_animation_set_state(void);
uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block);
uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block);
int32_t unit_animation_state_from_seat_type(int16_t animation_state);
uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state);
int16_t unit_base_animation_state_from_name(const char *name);
int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority);
uint8_t unit_state_is_scripted_animation(unit_data *unit);
uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index);
uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point);
uint32_t unit_predict_movement_delta(real_vector3d *out_position_delta, real_vector3d *out_forward_delta, real_vector3d *out_up_delta, float time_fraction);
uint8_t unit_state_allows_control(const uint8_t *animation_block);
void unit_update_up_vector(Biped *biped_tag, object *obj);
void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index);
void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key);
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out);
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second);
uint8_t unit_is_area_clear_of_fast_objects(void);
uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index);
void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair, uint32_t region_pair, uint32_t material, real_point3d *contact_point, real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf);
TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number);
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index);
void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index);
uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a, real_vector3d *reference_direction);
void unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index);
void unit_apply_network_control_update(unit_network_control_packet *packet);
void unit_broadcast_state_change_event(unit_state_change_record record);
void unit_network_create_update_apply(void *incoming_record);
uint8_t unit_any_dying_or_seat_transition(void);
void unit_detach_from_parent(object *obj, uint32_t unit_index, real_vector3d *cross_out, real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand, real_point3d *reposition_target);
void unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
void unit_dispatch_seat_exit_message(int32_t *message);
uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
void unit_exit_seat_end(void);
void unit_exit_vehicle_seat(uint32_t player_index);
TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second);
uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index);
void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit);
void unit_mark_zone_occupants_flag(uint32_t zone_list_index);
int16_t unit_seat_candidates_from_zone_and_enter(datum_index vehicle_index, char *seat_name, datum_index object_list);
uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index);
uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index, uint32_t *out_occupant_index);
int32_t object_find_nearest_biped(int32_t reference_object_index);
int32_t object_find_next_untargeted(int32_t starting_object_index);
void unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index);
void unit_inventory_get_weapon(void);
uint8_t unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index);
uint8_t unit_local_player_weapon_flag_check(void);
uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index);
void unit_scripting_set_or_drop_weapon(int32_t *message);
void unit_spawn_with_starting_weapons(void *command_record);
void unit_throw_grenade_release(void);
uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index);
uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index);
void vehicle_calculate_hover_lift_toward_target(void);
void vehicle_calculate_hover_turn_controls(void);

}

extern "C" {
void biped_advance_frame_counter_trigger(uint32_t object_index, char *state_out);
void biped_apply_idle_fidget(uint32_t object_index, uint8_t *state_out);
void biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base, float magnitude, float dir_x, float dir_y, float dir_z, char already_idle, uint8_t *state_out);
void biped_check_evade_reaction(uint32_t object_index);
void biped_clear_ground_surface_references(uint32_t object_index);
uint8_t biped_create(datum_index object_index);
datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position);
void biped_ground_adjust_apply_node_rotations(uint32_t object_index, real_matrix4x3 *nodes, real_point3d *saved_positions);
void biped_ground_adjust_solve(uint32_t object_index, real_matrix4x3 *nodes);
char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position, int32_t node_index, real_matrix4x3 *nodes, real_point3d *own_position, uint32_t *success_bits);
uint32_t biped_ground_adjust_step(uint32_t object_index);
void biped_integrate_movement(uint32_t object_index, object *obj, int8_t *state);
void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state);
uint32_t biped_is_idle_eligible(uint32_t object_index);
uint8_t biped_is_old_enough(uint32_t object_index);
void biped_movement_solve(biped_movement_solver_data *solve);
void biped_network_baseline_take(uint32_t object_index);
void biped_placement_offset_centered_pill(datum_index object_index, object_placement_data *placement);
void biped_reset_state(uint32_t object_index);
void biped_trigger_on_velocity_threshold(uint32_t object_index);
uint8_t biped_update(uint32_t object_index);
void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base);
void biped_update_facing(uint32_t object_index, int8_t *out_animation_state);
void biped_update_idle_basis(uint32_t object_index, uint8_t *state_out);
void biped_update_scale_function_inputs(uint32_t object_index);
void biped_update_target_lock_timer(datum_index target, uint32_t object_index);
int32_t object_find_nearest_biped(int32_t reference_object_index);
int32_t object_find_next_untargeted(int32_t starting_object_index);
void unit_accumulate_clamped_offset(uint32_t object_index, float new_value);
void unit_add_initial_weapons(uint32_t unit_index);
void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator);
void unit_ai_update_stagger_allocate(void);
void unit_ai_update_stagger_reset(void);
uint8_t unit_all_seats_unoccupied(uint32_t unit_index);
int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index, int32_t *chain_value);
void unit_animation_set_state(void);
uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block);
uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block);
int32_t unit_animation_state_from_seat_type(int16_t animation_state);
uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state);
uint8_t unit_any_dying_or_seat_transition(void);
uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index);
void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id);
void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local);
void unit_apply_fall_damage(uint32_t object_index, float fall_speed);
void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse);
void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse);
void unit_apply_network_control_update(unit_network_control_packet *packet);
void unit_apply_network_health_update(uint32_t object_index, void *message);
void unit_apply_scale_change(uint32_t unit_index, unit_scale_request *request);
int16_t unit_base_animation_state_from_name(const char *name);
uint8_t unit_begin_throw_grenade(uint32_t unit_index, const real_vector2d *direction);
void unit_broadcast_state_change_event(unit_state_change_record record);
int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget);
datum_index unit_build_seat_occupant_zone_list(uint32_t unit_index);
void unit_calculate_luminosity(uint32_t object_index);
void unit_can_see_point(uint32_t unit_index, real_vector3d *target_direction, real_vector3d *perp, real_vector3d *up);
void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect, uint32_t target_object_index, int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7);
void unit_check_fell_off_level(uint32_t object_index);
uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index);
uint8_t unit_choose_combat_reaction_animation(uint32_t unit_index, const datum_index *reaction_source, uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias);
void unit_choose_dialogue_variant(uint32_t unit_index);
uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction, uint8_t use_aiming_bounds);
void unit_clear_ground_adjust_dirty(uint32_t object_index);
void unit_clear_selected_equipment(uint32_t unit_index);
void unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index);
int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode);
void unit_compute_marker_offset_position(uint32_t object_index, real_vector3d *reference_direction, int16_t mode, real_point3d *out_position, float *base_position, float *offsets);
int16_t unit_count_deployed_weapons(uint32_t unit_index);
uint8_t unit_current_weapon_has_flag(uint32_t unit_index);
uint8_t unit_current_weapon_is_type(uint32_t unit_index, datum_index weapon_tag_id);
uint8_t unit_current_weapon_type_is_2_or_3(uint32_t unit_index);
void unit_detach_and_enter_named_seat(uint32_t unit_index, uint32_t target_parent_index, char *seat_marker_name);
int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name);
void unit_detach_from_parent(object *obj, uint32_t unit_index, real_vector3d *cross_out, real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand, real_point3d *reposition_target);
void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
void unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
void unit_detach_reposition_and_nudge(uint32_t unit_index);
void unit_dialogue_determine_variant(uint32_t object_index);
uint8_t unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code);
void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index);
void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key);
void unit_dispatch_seat_exit_message(int32_t *message);
void unit_dispatch_seat_overlay_command(uint32_t unit_index, int16_t command);
uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force);
void unit_drop_grenades(uint32_t unit_index);
void unit_drop_inventory_weapons(uint32_t unit_index);
void unit_drop_inventory_weapons_except_current(uint32_t unit_index);
void unit_drop_object_from_hand(uint32_t unit_index, uint32_t object_index);
void unit_enter_stunned_state(uint32_t unit_index, uint32_t responsible_object);
uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
void unit_evaluate_flee_reaction(uint32_t object_index);
void unit_exit_seat_end(void);
void unit_exit_vehicle_seat(uint32_t player_index);
uint16_t unit_find_best_seat_to_enter(uint32_t unit_index, uint32_t vehicle_index, int16_t *out_seat);
int16_t unit_find_empty_weapon_slot(uint32_t unit_index);
void unit_find_nearest_valid_surface_plane(uint32_t unit_index);
int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index, int16_t direction);
int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction);
uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a, real_vector3d *reference_direction);
int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices);
uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit);
uint16_t unit_find_weapon_index_with_fixed_flag(uint32_t unit_index);
uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint);
void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index);
void unit_forget_object_reference(uint32_t object_index, datum_index forgotten_object_index);
float unit_get_active_weapon_scale(uint32_t unit_index, int16_t zoom_level);
void unit_get_aiming_vector(uint32_t unit_index, real_vector3d *out);
int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state);
uint8_t unit_get_average_active_marker_direction(uint32_t unit_index, real_vector3d *out_direction);
uint32_t unit_get_biped_specific_value(uint32_t object_index);
void unit_get_camera_position(uint32_t unit_index, real_point3d *out);
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out);
int8_t unit_get_current_grenade_index(uint32_t unit_index);
char * unit_get_current_weapon_label(uint32_t unit_index);
int32_t unit_get_custom_animation_time_remaining(uint32_t object_index);
uint32_t unit_get_flag_bit6(uint32_t unit_index);
void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out);
int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type);
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second);
void unit_get_look_origin_and_direction(uint32_t object_index, uint32_t *out_autoaim_width, real_vector3d *out_direction, real_point3d *out_origin);
void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out);
uint8_t unit_get_recently_updated_flag(uint32_t object_index);
TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second);
char * unit_get_seat_or_state_name(uint32_t unit_index);
void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out);
uint32_t unit_get_tag_flag_bit7(uint32_t unit_index);
uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, uint32_t out_dx_to_key_frame, uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index);
datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index);
uint8_t unit_has_child_of_type5(uint32_t unit_index);
uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag);
void unit_initialize_random_turn_angle(uint32_t object_index);
void unit_inventory_get_weapon(void);
uint8_t unit_is_area_clear_of_fast_objects(void);
uint8_t unit_is_child_seated_at_named_marker(uint32_t unit_index, char *seat_label, uint32_t child_object_index);
uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
uint8_t unit_is_look_target_valid(uint32_t unit_index);
uint8_t unit_is_seat_control_available(uint32_t unit_index, int16_t command);
uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index);
uint8_t unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index);
uint8_t unit_local_player_weapon_flag_check(void);
int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority);
void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit);
void unit_mark_zone_occupants_flag(uint32_t zone_list_index);
void unit_melee_attack_scan(uint32_t unit_index);
void unit_melee_lunge_damage_tick(uint32_t unit_index);
uint8_t unit_named_seat_occupant_in_zone(uint32_t unit_index, char *seat_label, uint32_t zone_list_index);
void unit_network_create_update_apply(void *incoming_record);
uint8_t unit_new(uint32_t object_index);
uint32_t unit_noop_569670(uint32_t object_index);
void unit_notify_weapon_removed(int32_t object_index);
void unit_notify_weapon_removed_dup(int32_t object_index);
void unit_pick_and_ready_next_weapon(uint32_t unit_index);
TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number);
int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index);
uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index);
void unit_place(datum_index object_index, uint8_t *placement);
void unit_play_default_reaction_sound(uint32_t unit_index, datum_index sound_tag, datum_index sound_handle);
uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index);
uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point);
int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position);
uint32_t unit_predict_movement_delta(real_vector3d *out_position_delta, real_vector3d *out_forward_delta, real_vector3d *out_up_delta, float time_fraction);
void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair, uint32_t region_pair, uint32_t material, real_point3d *contact_point, real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf);
void unit_project_onto_aiming_axis(datum_index unit_index, real *out_speed, uint8_t project_point, uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis);
void unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index);
void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force);
void unit_recalculate_position(uint32_t object_index);
void unit_recompute_seat_occupants(uint32_t unit_index);
void unit_record_recent_damage_and_react(uint32_t unit_index, float damage_amount, int16_t response_index, uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object);
void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag);
void unit_region_damage_reaction(uint32_t object_index, uint32_t unused, uint32_t flags);
void unit_release_selected_equipment(uint32_t unit_index);
void unit_release_thrown_grenade(uint32_t object_index, uint8_t early);
void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset);
void unit_release_transient_state_and_detach(uint32_t unit_index, uint8_t is_light_reset);
void unit_reset_ground_adjust_state(uint32_t object_index);
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index);
void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index);
void unit_reset_velocity_and_ground_flag(uint32_t unit_index, uint8_t set_flag);
void unit_rotate_basis_about_axis(uint32_t object_index);
void unit_sample_camera_shake_from_velocity(uint32_t unit_index);
uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command);
void unit_scripting_set_emotion_animation(uint32_t unit_index, const char *emotion_name);
void unit_scripting_set_or_drop_weapon(int32_t *message);
int16_t unit_seat_candidates_from_zone_and_enter(datum_index vehicle_index, char *seat_name, datum_index object_list);
uint8_t unit_seat_flag_bit10(uint32_t unit_index, int16_t seat_index);
uint8_t unit_seat_flag_bit2(uint32_t unit_index, int16_t seat_index);
uint8_t unit_seat_flag_bit3(uint32_t unit_index, int16_t seat_index);
uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index);
uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index, uint32_t *out_occupant_index);
void unit_set_control_countdown(uint32_t unit_index, int32_t countdown, uint32_t extra_control_flags);
void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index);
uint8_t unit_set_custom_animation_frame(uint32_t unit_index, uint8_t warn_if_missing, datum_index graph_tag_id, const char *animation_name, int16_t frame);
void unit_set_facing_from_index_table(uint32_t object_index);
int32_t unit_set_grenade_type_and_count_delta(uint32_t unit_index, int16_t grenade_type, int8_t delta);
uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, const char *seat_label, const char *weapon_label, uint8_t apply);
void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy);
uint32_t unit_snap_to_min_ground_height(uint32_t object_index);
void unit_spawn_with_starting_weapons(void *command_record);
void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command);
void unit_start_seat_overlay_animation_b(uint32_t unit_index, int16_t command);
uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name, uint8_t interpolate);
uint8_t unit_state_allows_control(const uint8_t *animation_block);
uint8_t unit_state_is_scripted_animation(unit_data *unit);
int32_t unit_submit_periodic_network_update(datum_index object_index, void *buffer, int32_t bit_budget, int32_t update_type);
int32_t unit_test_placement_candidate(uint32_t unit_index, const real_vector3d *direction, real_vector3d *out_normal, float distance, real_point3d *out_position);
void unit_throw_grenade_move_to_hand(uint32_t unit_index);
void unit_throw_grenade_release(void);
void unit_track_target_lock_timeout(uint32_t object_index);
void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index);
void unit_try_exit_controlled_seat(uint32_t unit_index);
uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index);
uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const real_vector2d *direction);
uint8_t unit_try_ready_weapon_variant(uint32_t unit_index, const real_vector2d *direction);
uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index, int16_t release_current);
uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state);
uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command, const real_vector2d *direction);
uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index);
uint8_t unit_update(uint32_t unit_index);
void unit_update_aiming_overlay_angles(uint32_t unit_index, void *output);
uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request);
void unit_update_animation_timers(uint32_t unit_index);
void unit_update_autoaim_interaction(uint32_t unit_index);
void unit_update_footstep_and_idle_triggers(uint32_t unit_index);
void unit_update_ground_contact_counter(uint32_t unit_index, uint8_t *contact_points);
void unit_update_ik_detail_nodes(uint32_t object_index, void *node_base);
void unit_update_look_delta_controls(uint32_t object_index);
void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points);
uint32_t unit_update_marker_traction_effects(uint32_t object_index);
void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis);
void unit_update_recoil_decay(uint32_t object_index);
void unit_update_scale_function_inputs(uint32_t object_index);
void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still);
void unit_update_steering_deviation_effects(uint32_t unit_index, real_vector3d *reference_direction, uint8_t *contact_points);
void unit_update_up_vector(Biped *biped_tag, object *obj);
void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta);
void unit_validate_and_clear_weapon_switch(uint32_t unit_index);
uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index);
void vehicle_apply_network_update(datum_index vehicle_index, void **message, uint8_t *connection);
void vehicle_blend_animations(datum_index object_index, real_orientation *orientations);
void vehicle_calculate_animation_controls(uint32_t unit_index);
void vehicle_calculate_ground_contact_lean(uint32_t unit_index, void *out_record, void *out_transform);
void vehicle_calculate_ground_contact_lean_alt(uint32_t unit_index, void *out_record, void *out_transform);
void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform);
void vehicle_calculate_hover_lift_toward_target(void);
void vehicle_calculate_hover_turn_controls(void);
void vehicle_calculate_lean_controls(uint32_t unit_index, void *mass_points, float *powered_states);
void vehicle_calculate_mounted_controls_dispatch(uint32_t unit_index, void *out_transform, void *out_record);
void vehicle_calculate_steering_wheel_controls(uint32_t unit_index, void *mass_points, float *powered_states);
void vehicle_calculate_turret_controls(uint32_t unit_index, void *mass_points, float *powered_states);
void vehicle_calculate_wing_flex_controls(uint32_t unit_index, float angle, uint8_t *node_output, uint8_t *contact_points);
uint8_t vehicle_create(datum_index object_index);
void vehicle_create_hover_thruster_effects(uint32_t unit_index);
void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index);
int32_t vehicle_encode_network_create(datum_index vehicle_index, int32_t buffer, int32_t bit_budget);
int32_t vehicle_encode_network_update(datum_index vehicle_index, void *buffer, int32_t bit_budget, int32_t full_update);
uint8_t vehicle_is_old_enough(uint32_t object_index);
void vehicle_network_baseline_take(uint32_t object_index);
void vehicle_reset_state(uint32_t object_index);
uint32_t vehicle_update(uint32_t object_index);
}
