/**
 * @file include/halo/units/api.hpp
 * Functions of the units module that other modules and the data tables call (namespace halo::units). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>



struct Biped;
struct TagID;
struct Unit;
struct animation_state;
struct biped_movement_solver_data;
struct bsp_leaf_reference;
struct damage_data;
struct object;
struct object_placement_data;
struct real_orientation;
struct real_plane3d;
struct real_point3d;
struct real_vector2d;
struct real_vector3d;
struct unit_control_data;
struct unit_data;
struct unit_network_control_packet;
struct unit_speech;
struct unit_state_change_record;
typedef uint32_t datum_index;
typedef float real;

namespace halo::units {

/**
 * The engine globals the units module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    uint8_t &updates_suppressed;
};

Globals &globals();

void biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base, float magnitude, float dir_x, float dir_y, float dir_z, char already_idle, uint8_t *state_out);
void biped_clear_ground_surface_references(uint32_t object_index);
uint8_t biped_create(datum_index object_index);
datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position);
uint32_t biped_is_idle_eligible(uint32_t object_index);
uint8_t biped_is_old_enough(uint32_t object_index);
void biped_movement_solve(biped_movement_solver_data *solve);
void biped_network_baseline_take(uint32_t object_index);
void biped_placement_offset_centered_pill(datum_index object_index, object_placement_data *placement);
void biped_reset_state(uint32_t object_index);
uint8_t biped_update(uint32_t object_index);
void biped_update_animation_frame_trigger(float threshold, const Biped *timing_table, object *object_base);
void biped_update_scale_function_inputs(uint32_t object_index);
void biped_update_target_lock_timer(datum_index target, uint32_t object_index);
int32_t object_find_nearest_biped(int32_t reference_object_index);
int32_t object_find_next_untargeted(int32_t starting_object_index);
void unit_accumulate_clamped_offset(uint32_t object_index, float new_value);
void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator);
void unit_ai_update_stagger_allocate(void);
void unit_ai_update_stagger_reset(void);
uint8_t unit_all_seats_unoccupied(uint32_t unit_index);
int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_communication_hold_tick, int16_t *dialogue_index, int32_t *chain_value);
void unit_animation_set_state(void);
uint8_t unit_animation_state_allows_parent_ik(const unit_data &unit);
uint8_t unit_animation_state_allows_weapon_ik(const unit_data &unit);
int32_t unit_animation_state_from_seat_type(int16_t animation_state);
uint8_t unit_animation_state_is_compatible(const unit_data &unit, int16_t requested_state);
uint8_t unit_any_dying_or_seat_transition(void);
uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index);
void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id);
void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local);
void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse);
void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse);
void unit_apply_network_control_update(unit_network_control_packet *packet);
void unit_apply_network_health_update(uint32_t object_index, void *message);
int16_t unit_base_animation_state_from_name(const char *name);
void unit_broadcast_state_change_event(unit_state_change_record record);
int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget);
datum_index unit_build_seat_occupant_zone_list(uint32_t unit_index);
uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index);
uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction, uint8_t use_aiming_bounds);
void unit_clear_selected_equipment(uint32_t unit_index);
void unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index);
int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode);
int16_t unit_count_deployed_weapons(uint32_t unit_index);
uint8_t unit_current_weapon_is_type(uint32_t unit_index, datum_index weapon_tag_id);
uint8_t unit_current_weapon_type_is_2_or_3(uint32_t unit_index);
void unit_detach_and_enter_named_seat(uint32_t unit_index, uint32_t target_parent_index, char *seat_marker_name);
int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name);
void unit_detach_from_parent(object *obj, uint32_t unit_index, real_vector3d *cross_out, real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand, real_point3d *reposition_target);
void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
void unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event);
void unit_detach_reposition_and_nudge(uint32_t unit_index);
uint8_t unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code);
void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index);
void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key);
void unit_dispatch_seat_exit_message(int32_t *message);
void unit_dispatch_seat_overlay_command(uint32_t unit_index, int16_t command);
uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force);
void unit_drop_inventory_weapons_except_current(uint32_t unit_index);
uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
void unit_exit_seat_end(void);
void unit_exit_vehicle_seat(uint32_t player_index);
uint16_t unit_find_best_seat_to_enter(uint32_t unit_index, uint32_t vehicle_index, int16_t *out_seat);
int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction);
uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a, real_vector3d *reference_direction);
int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices);
uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit);
uint16_t unit_find_weapon_index_with_fixed_flag(uint32_t unit_index);
uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint);
void unit_forget_object_reference(uint32_t object_index, datum_index forgotten_object_index);
float unit_get_active_weapon_scale(uint32_t unit_index, int16_t zoom_level);
void unit_get_aiming_vector(uint32_t unit_index, real_vector3d *out);
int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state);
uint8_t unit_get_average_active_marker_direction(uint32_t unit_index, real_vector3d *out_direction);
uint32_t unit_get_biped_specific_value(uint32_t object_index);
void unit_get_camera_position(uint32_t unit_index, real_point3d *out);
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out);
int8_t unit_get_current_grenade_index(uint32_t unit_index);
int32_t unit_get_custom_animation_time_remaining(uint32_t object_index);
uint32_t unit_get_flag_bit6(uint32_t unit_index);
void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out);
int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type);
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second);
void unit_get_look_origin_and_direction(uint32_t object_index, uint32_t *out_autoaim_width, real_vector3d *out_direction, real_point3d *out_origin);
void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out);
TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second);
void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out);
uint32_t unit_get_tag_flag_bit7(uint32_t unit_index);
uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, uint32_t out_dx_to_key_frame, uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index);
datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index);
uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag);
void unit_initialize_random_turn_angle(uint32_t object_index);
void unit_inventory_get_weapon(void);
uint8_t unit_is_area_clear_of_fast_objects(void);
uint8_t unit_is_child_seated_at_named_marker(uint32_t unit_index, char *seat_label, uint32_t child_object_index);
uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index);
uint8_t unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index);
uint8_t unit_local_player_weapon_flag_check(void);
int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority);
void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit);
void unit_mark_zone_occupants_flag(uint32_t zone_list_index);
uint8_t unit_named_seat_occupant_in_zone(uint32_t unit_index, char *seat_label, uint32_t zone_list_index);
void unit_network_create_update_apply(void *incoming_record);
uint8_t unit_new(uint32_t object_index);
uint32_t unit_noop_569670(uint32_t object_index);
void unit_notify_weapon_removed(int32_t object_index);
void unit_pick_and_ready_next_weapon(uint32_t unit_index);
TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number);
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
void unit_recompute_seat_occupants(uint32_t unit_index);
void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag);
void unit_region_damage_reaction(uint32_t object_index, uint32_t unused, uint32_t flags);
void unit_release_selected_equipment(uint32_t unit_index);
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index);
void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index);
void unit_reset_velocity_and_ground_flag(uint32_t unit_index, uint8_t set_flag);
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
void unit_spawn_with_starting_weapons(void *command_record);
uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name, uint8_t interpolate);
uint8_t unit_state_allows_control(const unit_data &unit);
uint8_t unit_state_is_scripted_animation(unit_data *unit);
int32_t unit_submit_periodic_network_update(datum_index object_index, void *buffer, int32_t bit_budget, int32_t update_type);
int32_t unit_test_placement_candidate(uint32_t unit_index, const real_vector3d *direction, real_vector3d *out_normal, float distance, real_point3d *out_position);
void unit_throw_grenade_release(void);
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
void unit_update_ik_detail_nodes(uint32_t object_index, void *node_base);
void unit_update_scale_function_inputs(uint32_t object_index);
void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still);
void unit_update_up_vector(Biped *biped_tag, object *obj);
void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta);
uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index);
void vehicle_apply_network_update(datum_index vehicle_index, void **message, uint8_t *connection);
void vehicle_blend_animations(datum_index object_index, real_orientation *orientations);
void vehicle_calculate_animation_controls(uint32_t unit_index);
uint8_t vehicle_create(datum_index object_index);
int32_t vehicle_encode_network_create(datum_index vehicle_index, int32_t buffer, int32_t bit_budget);
int32_t vehicle_encode_network_update(datum_index vehicle_index, void *buffer, int32_t bit_budget, int32_t full_update);
uint8_t vehicle_is_old_enough(uint32_t object_index);
void vehicle_network_baseline_take(uint32_t object_index);
void vehicle_reset_state(uint32_t object_index);
uint32_t vehicle_update(uint32_t object_index);

}
