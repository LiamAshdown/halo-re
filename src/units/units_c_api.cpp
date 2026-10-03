#include "halo/units/unit.hpp"

/**
 * C entry point for halo::units::BipedView::advance_frame_counter_trigger; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55eb90
 */
void biped_advance_frame_counter_trigger(uint32_t object_index, char *state_out)
{
    halo::units::BipedView(object_index).advance_frame_counter_trigger(state_out);
}

/**
 * C entry point for halo::units::BipedView::apply_idle_fidget; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e940
 */
void biped_apply_idle_fidget(uint32_t object_index, uint8_t *state_out)
{
    halo::units::BipedView(object_index).apply_idle_fidget(state_out);
}

/**
 * C entry point for halo::units::biped_build_update_delta_unit_grenade_count_mod1; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x55e9ff
 */
void biped_build_update_delta_unit_grenade_count_mod1(uint32_t flags, object *object_base, float magnitude, float dir_x, float dir_y, float dir_z, char already_idle, uint8_t *state_out)
{
    halo::units::biped_build_update_delta_unit_grenade_count_mod1(flags, object_base, magnitude, dir_x, dir_y, dir_z, already_idle, state_out);
}

/**
 * C entry point for halo::units::BipedView::check_evade_reaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e190
 */
void biped_check_evade_reaction(uint32_t object_index)
{
    halo::units::BipedView(object_index).check_evade_reaction();
}

/**
 * C entry point for halo::units::BipedView::clear_ground_surface_references; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x559f70
 */
void biped_clear_ground_surface_references(uint32_t object_index)
{
    halo::units::BipedView(object_index).clear_ground_surface_references();
}

/**
 * C entry point for halo::units::BipedView::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x558dc0
 */
uint8_t biped_create(datum_index object_index)
{
    return halo::units::BipedView(object_index).create();
}

/**
 * C entry point for halo::units::BipedView::get_cached_look_at_position; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ab30
 */
datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position)
{
    return halo::units::BipedView(object_index).get_cached_look_at_position(out_position);
}

/**
 * C entry point for halo::units::BipedView::ground_adjust_apply_node_rotations; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x558a20
 */
void biped_ground_adjust_apply_node_rotations(uint32_t object_index, real_matrix4x3 *nodes, real_point3d *saved_positions)
{
    halo::units::BipedView(object_index).ground_adjust_apply_node_rotations(nodes, saved_positions);
}

/**
 * C entry point for halo::units::BipedView::ground_adjust_solve; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x558000
 */
void biped_ground_adjust_solve(uint32_t object_index, real_matrix4x3 *nodes)
{
    halo::units::BipedView(object_index).ground_adjust_solve(nodes);
}

/**
 * C entry point for halo::units::BipedView::ground_adjust_solve_node; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x557b80
 */
char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position, int32_t node_index, real_matrix4x3 *nodes, real_point3d *own_position, uint32_t *success_bits)
{
    return halo::units::BipedView(object_index).ground_adjust_solve_node(reference_position, node_index, nodes, own_position, success_bits);
}

/**
 * C entry point for halo::units::BipedView::ground_adjust_step; forwards to the C++ implementation unchanged.
 *
 * @address 0x557a90
 */
uint32_t biped_ground_adjust_step(uint32_t object_index)
{
    return halo::units::BipedView(object_index).ground_adjust_step();
}

/**
 * C entry point for halo::units::BipedView::integrate_movement; forwards to the C++ implementation unchanged.
 *
 * @address 0x55bea0
 */
void biped_integrate_movement(uint32_t object_index, object *obj, int8_t *state)
{
    halo::units::BipedView(object_index).integrate_movement(obj, state);
}

/**
 * C entry point for halo::units::BipedView::integrate_movement_with_collision; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x55cfd0
 */
void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state)
{
    halo::units::BipedView(object_index).integrate_movement_with_collision(state);
}

/**
 * C entry point for halo::units::BipedView::is_idle_eligible; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e8e0
 */
uint32_t biped_is_idle_eligible(uint32_t object_index)
{
    return halo::units::BipedView(object_index).is_idle_eligible();
}

/**
 * C entry point for halo::units::BipedView::is_old_enough; forwards to the C++ implementation unchanged.
 *
 * @address 0x55b780
 */
uint8_t biped_is_old_enough(uint32_t object_index)
{
    return halo::units::BipedView(object_index).is_old_enough();
}

/**
 * C entry point for halo::units::biped_movement_solve; forwards to the C++ implementation unchanged.
 *
 * @address 0x55efd0
 */
void biped_movement_solve(biped_movement_solver_data *solve)
{
    halo::units::biped_movement_solve(solve);
}

/**
 * C entry point for halo::units::BipedView::network_baseline_take; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55b3d0
 */
void biped_network_baseline_take(uint32_t object_index)
{
    halo::units::BipedView(object_index).network_baseline_take();
}

/**
 * C entry point for halo::units::BipedView::placement_offset_centered_pill; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x558d50
 */
void biped_placement_offset_centered_pill(datum_index object_index, object_placement_data *placement)
{
    halo::units::BipedView(object_index).placement_offset_centered_pill(placement);
}

/**
 * C entry point for halo::units::BipedView::reset_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x559f10
 */
void biped_reset_state(uint32_t object_index)
{
    halo::units::BipedView(object_index).reset_state();
}

/**
 * C entry point for halo::units::BipedView::trigger_on_velocity_threshold; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ec20
 */
void biped_trigger_on_velocity_threshold(uint32_t object_index)
{
    halo::units::BipedView(object_index).trigger_on_velocity_threshold();
}

/**
 * C entry point for halo::units::BipedView::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x5590a0
 */
uint8_t biped_update(uint32_t object_index)
{
    return halo::units::BipedView(object_index).update();
}

/**
 * C entry point for halo::units::biped_update_animation_frame_trigger; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55eaa0
 */
void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base)
{
    halo::units::biped_update_animation_frame_trigger(threshold, timing_table, object_base);
}

/**
 * C entry point for halo::units::BipedView::update_facing; forwards to the C++ implementation unchanged.
 *
 * @address 0x55b7c0
 */
void biped_update_facing(uint32_t object_index, int8_t *out_animation_state)
{
    halo::units::BipedView(object_index).update_facing(out_animation_state);
}

/**
 * C entry point for halo::units::BipedView::update_idle_basis; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e840
 */
void biped_update_idle_basis(uint32_t object_index, uint8_t *state_out)
{
    halo::units::BipedView(object_index).update_idle_basis(state_out);
}

/**
 * C entry point for halo::units::BipedView::update_scale_function_inputs; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x559e40
 */
void biped_update_scale_function_inputs(uint32_t object_index)
{
    halo::units::BipedView(object_index).update_scale_function_inputs();
}

/**
 * C entry point for halo::units::biped_update_target_lock_timer; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e0a0
 */
void biped_update_target_lock_timer(datum_index target, uint32_t object_index)
{
    halo::units::biped_update_target_lock_timer(target, object_index);
}

/**
 * C entry point for halo::units::object_find_nearest_biped; forwards to the C++ implementation unchanged.
 *
 * @address 0x56bee0
 */
int32_t object_find_nearest_biped(int32_t reference_object_index)
{
    return halo::units::object_find_nearest_biped(reference_object_index);
}

/**
 * C entry point for halo::units::object_find_next_untargeted; forwards to the C++ implementation unchanged.
 *
 * @address 0x56bdc0
 */
int32_t object_find_next_untargeted(int32_t starting_object_index)
{
    return halo::units::object_find_next_untargeted(starting_object_index);
}

/**
 * C entry point for halo::units::UnitView::accumulate_clamped_offset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570400
 */
void unit_accumulate_clamped_offset(uint32_t object_index, float new_value)
{
    halo::units::UnitView(object_index).accumulate_clamped_offset(new_value);
}

/**
 * C entry point for halo::units::UnitView::add_initial_weapons; forwards to the C++ implementation unchanged.
 *
 * @address 0x56cf10
 */
void unit_add_initial_weapons(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).add_initial_weapons();
}

/**
 * C entry point for halo::units::UnitView::add_marker_relative_offset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x569190
 */
void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator)
{
    halo::units::UnitView(unit_index).add_marker_relative_offset(mode, world_point, reference_direction, offsets, accumulator);
}

/**
 * C entry point for halo::units::unit_ai_update_stagger_allocate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561fe0
 */
void unit_ai_update_stagger_allocate(void)
{
    halo::units::unit_ai_update_stagger_allocate();
}

/**
 * C entry point for halo::units::unit_ai_update_stagger_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x562020
 */
void unit_ai_update_stagger_reset(void)
{
    halo::units::unit_ai_update_stagger_reset();
}

/**
 * C entry point for halo::units::UnitView::all_seats_unoccupied; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566910
 */
uint8_t unit_all_seats_unoccupied(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).all_seats_unoccupied();
}

/**
 * C entry point for halo::units::UnitView::animation_change_priority_check; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x560d00
 */
int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index, int32_t *chain_value)
{
    return halo::units::UnitView(unit_index).animation_change_priority_check(follow_fallback, requested_priority, allow_repeat, out_unknown_3f0, dialogue_index, chain_value);
}

/**
 * C entry point for halo::units::unit_animation_set_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x569450
 */
void unit_animation_set_state(void)
{
    halo::units::unit_animation_set_state();
}

/**
 * C entry point for halo::units::unit_animation_state_allows_parent_ik; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565d60
 */
uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block)
{
    return halo::units::unit_animation_state_allows_parent_ik(animation_block);
}

/**
 * C entry point for halo::units::unit_animation_state_allows_weapon_ik; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565d00
 */
uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block)
{
    return halo::units::unit_animation_state_allows_weapon_ik(animation_block);
}

/**
 * C entry point for halo::units::unit_animation_state_from_seat_type; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565da0
 */
int32_t unit_animation_state_from_seat_type(int16_t animation_state)
{
    return halo::units::unit_animation_state_from_seat_type(animation_state);
}

/**
 * C entry point for halo::units::unit_animation_state_is_compatible; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565be0
 */
uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state)
{
    return halo::units::unit_animation_state_is_compatible(animation_block, requested_state);
}

/**
 * C entry point for halo::units::unit_any_dying_or_seat_transition; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c070
 */
uint8_t unit_any_dying_or_seat_transition(void)
{
    return halo::units::unit_any_dying_or_seat_transition();
}

/**
 * C entry point for halo::units::UnitView::any_flagged_seat_occupied; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56cc80
 */
uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).any_flagged_seat_occupied();
}

/**
 * C entry point for halo::units::UnitView::apply_control_block; forwards to the C++ implementation unchanged.
 *
 * @address 0x5639f0
 */
void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id)
{
    halo::units::UnitView(unit_index).apply_control_block(control, source_id);
}

/**
 * C entry point for halo::units::UnitView::apply_damage_effects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5674a0
 */
void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local)
{
    halo::units::UnitView(unit_index).apply_damage_effects(dd, flags, shield_damage, body_damage, region_index, is_local);
}

/**
 * C entry point for halo::units::UnitView::apply_fall_damage; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e4f0
 */
void unit_apply_fall_damage(uint32_t object_index, float fall_speed)
{
    halo::units::UnitView(object_index).apply_fall_damage(fall_speed);
}

/**
 * C entry point for halo::units::UnitView::apply_impulse; forwards to the C++ implementation unchanged.
 *
 * @address 0x559fa0
 */
void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse)
{
    halo::units::UnitView(object_index).apply_impulse(impulse);
}

/**
 * C entry point for halo::units::UnitView::apply_impulse_to_seat; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x571cb0
 */
void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse)
{
    halo::units::UnitView(unit_index).apply_impulse_to_seat(impulse);
}

/**
 * C entry point for halo::units::unit_apply_network_control_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566c90
 */
void unit_apply_network_control_update(unit_network_control_packet *packet)
{
    halo::units::unit_apply_network_control_update(packet);
}

/**
 * C entry point for halo::units::UnitView::apply_network_health_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55b5f0
 */
void unit_apply_network_health_update(uint32_t object_index, void *message)
{
    halo::units::UnitView(object_index).apply_network_health_update(message);
}

/**
 * C entry point for halo::units::UnitView::apply_scale_change; forwards to the C++ implementation unchanged.
 *
 * @address 0x562030
 */
void unit_apply_scale_change(uint32_t unit_index, unit_scale_request *request)
{
    halo::units::UnitView(unit_index).apply_scale_change(request);
}

/**
 * C entry point for halo::units::unit_base_animation_state_from_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56eb90
 */
int16_t unit_base_animation_state_from_name(const char *name)
{
    return halo::units::unit_base_animation_state_from_name(name);
}

/**
 * C entry point for halo::units::UnitView::begin_throw_grenade; forwards to the C++ implementation unchanged.
 *
 * @address 0x56e080
 */
uint8_t unit_begin_throw_grenade(uint32_t unit_index, const real_vector2d *direction)
{
    return halo::units::UnitView(unit_index).begin_throw_grenade(direction);
}

/**
 * C entry point for halo::units::unit_broadcast_state_change_event; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566c00
 */
void unit_broadcast_state_change_event(unit_state_change_record record)
{
    halo::units::unit_broadcast_state_change_event(record);
}

/**
 * C entry point for halo::units::UnitView::build_network_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55aed0
 */
int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget)
{
    return halo::units::UnitView(object_index).build_network_update(buffer, bit_budget);
}

/**
 * C entry point for halo::units::UnitView::build_seat_occupant_zone_list; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56bbd0
 */
datum_index unit_build_seat_occupant_zone_list(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).build_seat_occupant_zone_list();
}

/**
 * C entry point for halo::units::UnitView::calculate_luminosity; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ec60
 */
void unit_calculate_luminosity(uint32_t object_index)
{
    halo::units::UnitView(object_index).calculate_luminosity();
}

/**
 * C entry point for halo::units::UnitView::can_see_point; forwards to the C++ implementation unchanged.
 *
 * @address 0x56f800
 */
void unit_can_see_point(uint32_t unit_index, real_vector3d *target_direction, real_vector3d *perp, real_vector3d *up)
{
    halo::units::UnitView(unit_index).can_see_point(target_direction, perp, up);
}

/**
 * C entry point for halo::units::UnitView::cause_melee_damage; forwards to the C++ implementation unchanged.
 *
 * @address 0x56f2d0
 */
void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect, uint32_t target_object_index, int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7)
{
    halo::units::UnitView(unit_index).cause_melee_damage(suppress_effect, target_object_index, damage_param4, damage_param5, damage_param6, damage_param7);
}

/**
 * C entry point for halo::units::UnitView::check_fell_off_level; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e4a0
 */
void unit_check_fell_off_level(uint32_t object_index)
{
    halo::units::UnitView(object_index).check_fell_off_level();
}

/**
 * C entry point for halo::units::UnitView::check_weapon_use_permission; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56da00
 */
uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index)
{
    return halo::units::UnitView(unit_index).check_weapon_use_permission(weapon_index);
}

/**
 * C entry point for halo::units::UnitView::choose_combat_reaction_animation; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x561140
 */
uint8_t unit_choose_combat_reaction_animation(uint32_t unit_index, const datum_index *reaction_source, uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias)
{
    return halo::units::UnitView(unit_index).choose_combat_reaction_animation(reaction_source, is_scripted, allow_second_tier, distance_bias);
}

/**
 * C entry point for halo::units::UnitView::choose_dialogue_variant; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561990
 */
void unit_choose_dialogue_variant(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).choose_dialogue_variant();
}

/**
 * C entry point for halo::units::UnitView::clamp_direction_to_aim_or_look_bounds; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5697a0
 */
uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction, uint8_t use_aiming_bounds)
{
    return halo::units::UnitView(unit_index).clamp_direction_to_aim_or_look_bounds(world_direction, use_aiming_bounds);
}

/**
 * C entry point for halo::units::UnitView::clear_ground_adjust_dirty; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ad70
 */
void unit_clear_ground_adjust_dirty(uint32_t object_index)
{
    halo::units::UnitView(object_index).clear_ground_adjust_dirty();
}

/**
 * C entry point for halo::units::UnitView::clear_selected_equipment; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d2c0
 */
void unit_clear_selected_equipment(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).clear_selected_equipment();
}

/**
 * C entry point for halo::units::unit_clear_weapon_switch_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565a70
 */
void unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index)
{
    halo::units::unit_clear_weapon_switch_state(unit, skip_notify, sound_definition_index);
}

/**
 * C entry point for halo::units::UnitView::commit_speech; forwards to the C++ implementation unchanged.
 *
 * @address 0x560f20
 */
int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode)
{
    return halo::units::UnitView(unit_index).commit_speech(source, mode);
}

/**
 * C entry point for halo::units::UnitView::compute_marker_offset_position; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55a170
 */
void unit_compute_marker_offset_position(uint32_t object_index, real_vector3d *reference_direction, int16_t mode, real_point3d *out_position, float *base_position, float *offsets)
{
    halo::units::UnitView(object_index).compute_marker_offset_position(reference_direction, mode, out_position, base_position, offsets);
}

/**
 * C entry point for halo::units::UnitView::count_deployed_weapons; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d990
 */
int16_t unit_count_deployed_weapons(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).count_deployed_weapons();
}

/**
 * C entry point for halo::units::UnitView::current_weapon_has_flag; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565b60
 */
uint8_t unit_current_weapon_has_flag(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).current_weapon_has_flag();
}

/**
 * C entry point for halo::units::UnitView::current_weapon_is_type; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561f80
 */
uint8_t unit_current_weapon_is_type(uint32_t unit_index, datum_index weapon_tag_id)
{
    return halo::units::UnitView(unit_index).current_weapon_is_type(weapon_tag_id);
}

/**
 * C entry point for halo::units::UnitView::current_weapon_type_is_2_or_3; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56bd60
 */
uint8_t unit_current_weapon_type_is_2_or_3(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).current_weapon_type_is_2_or_3();
}

/**
 * C entry point for halo::units::UnitView::detach_and_enter_named_seat; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x569d40
 */
void unit_detach_and_enter_named_seat(uint32_t unit_index, uint32_t target_parent_index, char *seat_marker_name)
{
    halo::units::UnitView(unit_index).detach_and_enter_named_seat(target_parent_index, seat_marker_name);
}

/**
 * C entry point for halo::units::UnitView::detach_child_at_named_seat; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ab50
 */
int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name)
{
    return halo::units::UnitView(unit_index).detach_child_at_named_seat(seat_marker_name);
}

/**
 * C entry point for halo::units::unit_detach_from_parent; forwards to the C++ implementation unchanged.
 *
 * @address 0x570140
 */
void unit_detach_from_parent(object *obj, uint32_t unit_index, real_vector3d *cross_out, real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand, real_point3d *reposition_target)
{
    halo::units::unit_detach_from_parent(obj, unit_index, cross_out, cross_ecx_operand, cross_stack_operand, reposition_target);
}

/**
 * C entry point for halo::units::UnitView::detach_from_seat; forwards to the C++ implementation unchanged.
 *
 * @address 0x56c640
 */
void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    halo::units::UnitView(unit_index).detach_from_seat(suppress_trigger, require_client_flag, fire_trigger_event);
}

/**
 * C entry point for halo::units::unit_detach_if_flag_clear; forwards to the C++ implementation unchanged.
 *
 * @address 0x56c440
 */
void unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    halo::units::unit_detach_if_flag_clear(skip_flag, unit_index, suppress_trigger, require_client_flag, fire_trigger_event);
}

/**
 * C entry point for halo::units::UnitView::detach_reposition_and_nudge; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ca40
 */
void unit_detach_reposition_and_nudge(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).detach_reposition_and_nudge();
}

/**
 * C entry point for halo::units::UnitView::dialogue_determine_variant; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5618e0
 */
void unit_dialogue_determine_variant(uint32_t object_index)
{
    halo::units::UnitView(object_index).dialogue_determine_variant();
}

/**
 * C entry point for halo::units::UnitView::dispatch_reaction_animation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5614a0
 */
uint8_t unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code)
{
    return halo::units::UnitView(unit_index).dispatch_reaction_animation(reaction_code);
}

/**
 * C entry point for halo::units::unit_dispatch_scripted_event_1b; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56dcd0
 */
void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index)
{
    halo::units::unit_dispatch_scripted_event_1b(event_byte, unit_index);
}

/**
 * C entry point for halo::units::unit_dispatch_scripted_event_9; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c370
 */
void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key)
{
    halo::units::unit_dispatch_scripted_event_9(event_byte, hash_key);
}

/**
 * C entry point for halo::units::unit_dispatch_seat_exit_message; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c400
 */
void unit_dispatch_seat_exit_message(int32_t *message)
{
    halo::units::unit_dispatch_seat_exit_message(message);
}

/**
 * C entry point for halo::units::UnitView::dispatch_seat_overlay_command; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x567400
 */
void unit_dispatch_seat_overlay_command(uint32_t unit_index, int16_t command)
{
    halo::units::UnitView(unit_index).dispatch_seat_overlay_command(command);
}

/**
 * C entry point for halo::units::UnitView::drop_current_weapon; forwards to the C++ implementation unchanged.
 *
 * @address 0x56dec0
 */
uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force)
{
    return halo::units::UnitView(unit_index).drop_current_weapon(force);
}

/**
 * C entry point for halo::units::UnitView::drop_grenades; forwards to the C++ implementation unchanged.
 *
 * @address 0x56ef60
 */
void unit_drop_grenades(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).drop_grenades();
}

/**
 * C entry point for halo::units::UnitView::drop_inventory_weapons; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56f060
 */
void unit_drop_inventory_weapons(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).drop_inventory_weapons();
}

/**
 * C entry point for halo::units::UnitView::drop_inventory_weapons_except_current; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56d360
 */
void unit_drop_inventory_weapons_except_current(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).drop_inventory_weapons_except_current();
}

/**
 * C entry point for halo::units::UnitView::drop_object_from_hand; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ed00
 */
void unit_drop_object_from_hand(uint32_t unit_index, uint32_t object_index)
{
    halo::units::UnitView(unit_index).drop_object_from_hand(object_index);
}

/**
 * C entry point for halo::units::UnitView::enter_stunned_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x5705a0
 */
void unit_enter_stunned_state(uint32_t unit_index, uint32_t responsible_object)
{
    halo::units::UnitView(unit_index).enter_stunned_state(responsible_object);
}

/**
 * C entry point for halo::units::unit_enter_vehicle_seat; forwards to the C++ implementation unchanged.
 *
 * @address 0x566970
 */
uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index)
{
    return halo::units::unit_enter_vehicle_seat(vehicle_index, seat_index, unit_index);
}

/**
 * C entry point for halo::units::UnitView::evaluate_flee_reaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e2d0
 */
void unit_evaluate_flee_reaction(uint32_t object_index)
{
    halo::units::UnitView(object_index).evaluate_flee_reaction();
}

/**
 * C entry point for halo::units::unit_exit_seat_end; forwards to the C++ implementation unchanged.
 *
 * @address 0x56fd40
 */
void unit_exit_seat_end(void)
{
    halo::units::unit_exit_seat_end();
}

/**
 * C entry point for halo::units::unit_exit_vehicle_seat; forwards to the C++ implementation unchanged.
 *
 * @address 0x568120
 */
void unit_exit_vehicle_seat(uint32_t player_index)
{
    halo::units::unit_exit_vehicle_seat(player_index);
}

/**
 * C entry point for halo::units::UnitView::find_best_seat_to_enter; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566560
 */
uint16_t unit_find_best_seat_to_enter(uint32_t unit_index, uint32_t vehicle_index, int16_t *out_seat)
{
    return halo::units::UnitView(unit_index).find_best_seat_to_enter(vehicle_index, out_seat);
}

/**
 * C entry point for halo::units::UnitView::find_empty_weapon_slot; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d660
 */
int16_t unit_find_empty_weapon_slot(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).find_empty_weapon_slot();
}

/**
 * C entry point for halo::units::UnitView::find_nearest_valid_surface_plane; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x560630
 */
void unit_find_nearest_valid_surface_plane(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).find_nearest_valid_surface_plane();
}

/**
 * C entry point for halo::units::UnitView::find_next_grenade_type_with_count; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5699a0
 */
int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index, int16_t direction)
{
    return halo::units::UnitView(unit_index).find_next_grenade_type_with_count(start_index, direction);
}

/**
 * C entry point for halo::units::UnitView::find_next_zone_permitted_weapon_slot; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56dba0
 */
int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction)
{
    return halo::units::UnitView(unit_index).find_next_zone_permitted_weapon_slot(start_slot, direction);
}

/**
 * C entry point for halo::units::unit_find_placement_position; forwards to the C++ implementation unchanged.
 *
 * @address 0x55a500
 */
uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a, real_vector3d *reference_direction)
{
    return halo::units::unit_find_placement_position(anchor_object, orientation_object, out_position, radius, grid_mode, skip_reposition, scale_radius, object_index_a, reference_direction);
}

/**
 * C entry point for halo::units::UnitView::find_seats_matching_name_and_flags; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56a310
 */
int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices)
{
    return halo::units::UnitView(unit_index).find_seats_matching_name_and_flags(name_filter, flag_selector, out_indices, max_indices);
}

/**
 * C entry point for halo::units::UnitView::find_weapon_index_by_flag; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570520
 */
uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit)
{
    return halo::units::UnitView(unit_index).find_weapon_index_by_flag(flag_bit);
}

/**
 * C entry point for halo::units::UnitView::find_weapon_index_with_fixed_flag; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x570460
 */
uint16_t unit_find_weapon_index_with_fixed_flag(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).find_weapon_index_with_fixed_flag();
}

/**
 * C entry point for halo::units::UnitView::find_weapon_marker_transform; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5640a0
 */
uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint)
{
    return halo::units::UnitView(unit_index).find_weapon_marker_transform(vehicle_index, seat_index, out_entry, out_seat, out_hint);
}

/**
 * C entry point for halo::units::UnitView::fire_animation_sound_trigger; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x560590
 */
void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index)
{
    halo::units::UnitView(unit_index).fire_animation_sound_trigger(trigger_kind, contact_point_index);
}

/**
 * C entry point for halo::units::UnitView::forget_object_reference; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56f0f0
 */
void unit_forget_object_reference(uint32_t object_index, datum_index forgotten_object_index)
{
    halo::units::UnitView(object_index).forget_object_reference(forgotten_object_index);
}

/**
 * C entry point for halo::units::UnitView::get_active_weapon_scale; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565ab0
 */
float unit_get_active_weapon_scale(uint32_t unit_index, int16_t zoom_level)
{
    return halo::units::UnitView(unit_index).get_active_weapon_scale(zoom_level);
}

/**
 * C entry point for halo::units::UnitView::get_aiming_vector; forwards to the C++ implementation unchanged.
 *
 * @address 0x5696f0
 */
void unit_get_aiming_vector(uint32_t unit_index, real_vector3d *out)
{
    halo::units::UnitView(unit_index).get_aiming_vector(out);
}

/**
 * C entry point for halo::units::UnitView::get_animation_frames_remaining; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x564390
 */
int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state)
{
    return halo::units::UnitView(unit_index).get_animation_frames_remaining(out_animation_state);
}

/**
 * C entry point for halo::units::UnitView::get_average_active_marker_direction; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x575e30
 */
uint8_t unit_get_average_active_marker_direction(uint32_t unit_index, real_vector3d *out_direction)
{
    return halo::units::UnitView(unit_index).get_average_active_marker_direction(out_direction);
}

/**
 * C entry point for halo::units::UnitView::get_biped_specific_value; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570ad0
 */
uint32_t unit_get_biped_specific_value(uint32_t object_index)
{
    return halo::units::UnitView(object_index).get_biped_specific_value();
}

/**
 * C entry point for halo::units::UnitView::get_camera_position; forwards to the C++ implementation unchanged.
 *
 * @address 0x568f80
 */
void unit_get_camera_position(uint32_t unit_index, real_point3d *out)
{
    halo::units::UnitView(unit_index).get_camera_position(out);
}

/**
 * C entry point for halo::units::unit_get_crouch_height_offset; forwards to the C++ implementation unchanged.
 *
 * @address 0x55a2e0
 */
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out)
{
    halo::units::unit_get_crouch_height_offset(object_position, object_index, pill_height, pill_radius_out);
}

/**
 * C entry point for halo::units::UnitView::get_current_grenade_index; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56e060
 */
int8_t unit_get_current_grenade_index(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).get_current_grenade_index();
}

/**
 * C entry point for halo::units::UnitView::get_current_weapon_label; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56dfd0
 */
char * unit_get_current_weapon_label(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).get_current_weapon_label();
}

/**
 * C entry point for halo::units::UnitView::get_custom_animation_time_remaining; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5701b0
 */
int32_t unit_get_custom_animation_time_remaining(uint32_t object_index)
{
    return halo::units::UnitView(object_index).get_custom_animation_time_remaining();
}

/**
 * C entry point for halo::units::UnitView::get_flag_bit6; forwards to the C++ implementation unchanged.
 *
 * @address 0x569bc0
 */
uint32_t unit_get_flag_bit6(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).get_flag_bit6();
}

/**
 * C entry point for halo::units::UnitView::get_forward_vector_or_marker_normal; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569720
 */
void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out)
{
    halo::units::UnitView(unit_index).get_forward_vector_or_marker_normal(out);
}

/**
 * C entry point for halo::units::UnitView::get_grenade_count; forwards to the C++ implementation unchanged.
 *
 * @address 0x56e030
 */
int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type)
{
    return halo::units::UnitView(unit_index).get_grenade_count(grenade_type);
}

/**
 * C entry point for halo::units::unit_get_hud_interface_tag_id; forwards to the C++ implementation unchanged.
 *
 * @address 0x560c70
 */
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second)
{
    return halo::units::unit_get_hud_interface_tag_id(unit_tag, use_second);
}

/**
 * C entry point for halo::units::UnitView::get_look_origin_and_direction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55a390
 */
void unit_get_look_origin_and_direction(uint32_t object_index, uint32_t *out_autoaim_width, real_vector3d *out_direction, real_point3d *out_origin)
{
    halo::units::UnitView(object_index).get_look_origin_and_direction(out_autoaim_width, out_direction, out_origin);
}

/**
 * C entry point for halo::units::UnitView::get_primary_eye_marker_position; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x568f50
 */
void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out)
{
    halo::units::UnitView(object_index).get_primary_eye_marker_position(out);
}

/**
 * C entry point for halo::units::UnitView::get_recently_updated_flag; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570c80
 */
uint8_t unit_get_recently_updated_flag(uint32_t object_index)
{
    return halo::units::UnitView(object_index).get_recently_updated_flag();
}

/**
 * C entry point for halo::units::unit_get_seat_hud_interface_tag_id; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x560cb0
 */
TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second)
{
    return halo::units::unit_get_seat_hud_interface_tag_id(unit_tag, seat_index, use_second);
}

/**
 * C entry point for halo::units::UnitView::get_seat_or_state_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c2f0
 */
char * unit_get_seat_or_state_name(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).get_seat_or_state_name();
}

/**
 * C entry point for halo::units::UnitView::get_secondary_eye_marker_position; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569280
 */
void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out)
{
    halo::units::UnitView(object_index).get_secondary_eye_marker_position(out);
}

/**
 * C entry point for halo::units::UnitView::get_tag_flag_bit7; forwards to the C++ implementation unchanged.
 *
 * @address 0x571c70
 */
uint32_t unit_get_tag_flag_bit7(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).get_tag_flag_bit7();
}

/**
 * C entry point for halo::units::UnitView::get_weapon_marker_indices; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5642c0
 */
uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, uint32_t out_dx_to_key_frame, uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index)
{
    return halo::units::UnitView(unit_index).get_weapon_marker_indices(use_alternate, out_dx_to_key_frame, out_dx_total, out_frame_count, out_key_frame_index);
}

/**
 * C entry point for halo::units::UnitView::get_weapon_object_index; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x569970
 */
datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index)
{
    return halo::units::UnitView(unit_index).get_weapon_object_index(slot_index);
}

/**
 * C entry point for halo::units::UnitView::has_child_of_type5; forwards to the C++ implementation unchanged.
 *
 * @address 0x570d70
 */
uint8_t unit_has_child_of_type5(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).has_child_of_type5();
}

/**
 * C entry point for halo::units::UnitView::has_weapon_of_type; forwards to the C++ implementation unchanged.
 *
 * @address 0x56d610
 */
uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag)
{
    return halo::units::UnitView(unit_index).has_weapon_of_type(weapon_group_tag);
}

/**
 * C entry point for halo::units::UnitView::initialize_random_turn_angle; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570650
 */
void unit_initialize_random_turn_angle(uint32_t object_index)
{
    halo::units::UnitView(object_index).initialize_random_turn_angle();
}

/**
 * C entry point for halo::units::unit_inventory_get_weapon; forwards to the C++ implementation unchanged.
 *
 * @address 0x56d070
 */
void unit_inventory_get_weapon(void)
{
    halo::units::unit_inventory_get_weapon();
}

/**
 * C entry point for halo::units::unit_is_area_clear_of_fast_objects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575c50
 */
uint8_t unit_is_area_clear_of_fast_objects(void)
{
    return halo::units::unit_is_area_clear_of_fast_objects();
}

/**
 * C entry point for halo::units::UnitView::is_child_seated_at_named_marker; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56b520
 */
uint8_t unit_is_child_seated_at_named_marker(uint32_t unit_index, char *seat_label, uint32_t child_object_index)
{
    return halo::units::UnitView(unit_index).is_child_seated_at_named_marker(seat_label, child_object_index);
}

/**
 * C entry point for halo::units::UnitView::is_in_busy_animation_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x569c90
 */
uint8_t unit_is_in_busy_animation_state(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).is_in_busy_animation_state();
}

/**
 * C entry point for halo::units::UnitView::is_look_target_valid; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x562570
 */
uint8_t unit_is_look_target_valid(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).is_look_target_valid();
}

/**
 * C entry point for halo::units::UnitView::is_seat_control_available; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5693a0
 */
uint8_t unit_is_seat_control_available(uint32_t unit_index, int16_t command)
{
    return halo::units::UnitView(unit_index).is_seat_control_available(command);
}

/**
 * C entry point for halo::units::unit_is_seat_occupied; forwards to the C++ implementation unchanged.
 *
 * @address 0x56cc10
 */
uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index)
{
    return halo::units::unit_is_seat_occupied(parent_index, seat_index);
}

/**
 * C entry point for halo::units::unit_lacks_weapon_type_of; forwards to the C++ implementation unchanged.
 *
 * @address 0x56da80
 */
uint8_t unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index)
{
    return halo::units::unit_lacks_weapon_type_of(reference_object_index, unit_index);
}

/**
 * C entry point for halo::units::unit_local_player_weapon_flag_check; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565b00
 */
uint8_t unit_local_player_weapon_flag_check(void)
{
    return halo::units::unit_local_player_weapon_flag_check();
}

/**
 * C entry point for halo::units::unit_map_action_command_to_animation_state; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5692b0
 */
int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority)
{
    return halo::units::unit_map_action_command_to_animation_state(command, out_priority);
}

/**
 * C entry point for halo::units::unit_mark_zone_list_alt_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x56c1d0
 */
void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit)
{
    halo::units::unit_mark_zone_list_alt_flag(zone_list_index, use_second_bit);
}

/**
 * C entry point for halo::units::unit_mark_zone_occupants_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x56b290
 */
void unit_mark_zone_occupants_flag(uint32_t zone_list_index)
{
    halo::units::unit_mark_zone_occupants_flag(zone_list_index);
}

/**
 * C entry point for halo::units::UnitView::melee_attack_scan; forwards to the C++ implementation unchanged.
 *
 * @address 0x56f550
 */
void unit_melee_attack_scan(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).melee_attack_scan();
}

/**
 * C entry point for halo::units::UnitView::melee_lunge_damage_tick; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56fc80
 */
void unit_melee_lunge_damage_tick(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).melee_lunge_damage_tick();
}

/**
 * C entry point for halo::units::UnitView::named_seat_occupant_in_zone; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56b380
 */
uint8_t unit_named_seat_occupant_in_zone(uint32_t unit_index, char *seat_label, uint32_t zone_list_index)
{
    return halo::units::UnitView(unit_index).named_seat_occupant_in_zone(seat_label, zone_list_index);
}

/**
 * C entry point for halo::units::unit_network_create_update_apply; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55b110
 */
void unit_network_create_update_apply(void *incoming_record)
{
    halo::units::unit_network_create_update_apply(incoming_record);
}

/**
 * C entry point for halo::units::UnitView::new_; forwards to the C++ implementation unchanged.
 *
 * @address 0x562180
 */
uint8_t unit_new(uint32_t object_index)
{
    return halo::units::UnitView(object_index).new_();
}

/**
 * C entry point for halo::units::UnitView::noop_569670; forwards to the C++ implementation unchanged.
 *
 * @address 0x569670
 */
uint32_t unit_noop_569670(uint32_t object_index)
{
    return halo::units::UnitView(object_index).noop_569670();
}

/**
 * C entry point for halo::units::UnitView::notify_weapon_removed; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ab10
 */
void unit_notify_weapon_removed(int32_t object_index)
{
    halo::units::UnitView(object_index).notify_weapon_removed();
}

/**
 * C entry point for halo::units::UnitView::notify_weapon_removed_dup; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ab30
 */
void unit_notify_weapon_removed_dup(int32_t object_index)
{
    halo::units::UnitView(object_index).notify_weapon_removed_dup();
}

/**
 * C entry point for halo::units::UnitView::pick_and_ready_next_weapon; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d6a0
 */
void unit_pick_and_ready_next_weapon(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).pick_and_ready_next_weapon();
}

/**
 * C entry point for halo::units::unit_pick_random_dialogue_variant; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561a00
 */
TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number)
{
    return halo::units::unit_pick_random_dialogue_variant(unit_tag, variant_number);
}

/**
 * C entry point for halo::units::UnitView::pick_random_spawned_actor_count; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x568540
 */
int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).pick_random_spawned_actor_count();
}

/**
 * C entry point for halo::units::unit_pickup_weapon; forwards to the C++ implementation unchanged.
 *
 * @address 0x56d400
 */
uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index)
{
    return halo::units::unit_pickup_weapon(pickup_mode, weapon_index, unit_index);
}

/**
 * C entry point for halo::units::UnitView::place; forwards to the C++ implementation unchanged.
 *
 * @address 0x558d40
 */
void unit_place(datum_index object_index, uint8_t *placement)
{
    halo::units::UnitView(object_index).place(placement);
}

/**
 * C entry point for halo::units::UnitView::play_default_reaction_sound; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561030
 */
void unit_play_default_reaction_sound(uint32_t unit_index, datum_index sound_tag, datum_index sound_handle)
{
    halo::units::UnitView(unit_index).play_default_reaction_sound(sound_tag, sound_handle);
}

/**
 * C entry point for halo::units::unit_point_in_front_and_asleep; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56bc80
 */
uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index)
{
    return halo::units::unit_point_in_front_and_asleep(world_point, unit_index);
}

/**
 * C entry point for halo::units::unit_point_within_look_cone; forwards to the C++ implementation unchanged.
 *
 * @address 0x56c100
 */
uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point)
{
    return halo::units::unit_point_within_look_cone(cone_angle, unit_index, world_point);
}

/**
 * C entry point for halo::units::UnitView::predict_aim_target_position; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x571de0
 */
int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position)
{
    return halo::units::UnitView(unit_index).predict_aim_target_position(out_position);
}

/**
 * C entry point for halo::units::unit_predict_movement_delta; forwards to the C++ implementation unchanged.
 *
 * @address 0x55cca0
 */
uint32_t unit_predict_movement_delta(real_vector3d *out_position_delta, real_vector3d *out_forward_delta, real_vector3d *out_up_delta, float time_fraction)
{
    return halo::units::unit_predict_movement_delta(out_position_delta, out_forward_delta, out_up_delta, time_fraction);
}

/**
 * C entry point for halo::units::unit_process_melee_special_interaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ff40
 */
void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair, uint32_t region_pair, uint32_t material, real_point3d *contact_point, real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf)
{
    halo::units::unit_process_melee_special_interaction(attacker_index, target_index, node_pair, region_pair, material, contact_point, contact_plane, contact_leaf);
}

/**
 * C entry point for halo::units::UnitView::project_onto_aiming_axis; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5658f0
 */
void unit_project_onto_aiming_axis(datum_index unit_index, real *out_speed, uint8_t project_point, uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis)
{
    halo::units::UnitView(unit_index).project_onto_aiming_axis(out_speed, project_point, use_unit_aiming_vector, point, axis);
}

/**
 * C entry point for halo::units::unit_propagate_position_delta_to_children; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x570cb0
 */
void unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index)
{
    halo::units::unit_propagate_position_delta_to_children(new_position, unit_index);
}

/**
 * C entry point for halo::units::UnitView::ready_desired_weapon; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d6e0
 */
void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force)
{
    halo::units::UnitView(unit_index).ready_desired_weapon(force);
}

/**
 * C entry point for halo::units::UnitView::recalculate_position; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x558eb0
 */
void unit_recalculate_position(uint32_t object_index)
{
    halo::units::UnitView(object_index).recalculate_position();
}

/**
 * C entry point for halo::units::UnitView::recompute_seat_occupants; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ce30
 */
void unit_recompute_seat_occupants(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).recompute_seat_occupants();
}

/**
 * C entry point for halo::units::UnitView::record_recent_damage_and_react; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x568230
 */
void unit_record_recent_damage_and_react(uint32_t unit_index, float damage_amount, int16_t response_index, uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object)
{
    halo::units::UnitView(unit_index).record_recent_damage_and_react(damage_amount, response_index, allow_broadcast, responsible_player, team_index, responsible_object);
}

/**
 * C entry point for halo::units::UnitView::refresh_targeting_flag_and_weapons; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569bf0
 */
void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag)
{
    halo::units::UnitView(unit_index).refresh_targeting_flag_and_weapons(initial_targeting_flag);
}

/**
 * C entry point for halo::units::UnitView::region_damage_reaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56f1c0
 */
void unit_region_damage_reaction(uint32_t object_index, uint32_t unused, uint32_t flags)
{
    halo::units::UnitView(object_index).region_damage_reaction(unused, flags);
}

/**
 * C entry point for halo::units::UnitView::release_selected_equipment; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d300
 */
void unit_release_selected_equipment(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).release_selected_equipment();
}

/**
 * C entry point for halo::units::UnitView::release_thrown_grenade; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56e440
 */
void unit_release_thrown_grenade(uint32_t object_index, uint8_t early)
{
    halo::units::UnitView(object_index).release_thrown_grenade(early);
}

/**
 * C entry point for halo::units::UnitView::release_transient_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x568610
 */
void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset)
{
    halo::units::UnitView(unit_index).release_transient_state(is_light_reset);
}

/**
 * C entry point for halo::units::UnitView::release_transient_state_and_detach; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x568cb0
 */
void unit_release_transient_state_and_detach(uint32_t unit_index, uint8_t is_light_reset)
{
    halo::units::UnitView(unit_index).release_transient_state_and_detach(is_light_reset);
}

/**
 * C entry point for halo::units::UnitView::reset_ground_adjust_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ad00
 */
void unit_reset_ground_adjust_state(uint32_t object_index)
{
    halo::units::UnitView(object_index).reset_ground_adjust_state();
}

/**
 * C entry point for halo::units::unit_reset_light_effect; forwards to the C++ implementation unchanged.
 *
 * @address 0x56ec10
 */
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index)
{
    return halo::units::unit_reset_light_effect(state, animation_graph_tag_index, object_index);
}

/**
 * C entry point for halo::units::UnitView::reset_orientation_and_find_position; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x55add0
 */
void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index)
{
    halo::units::UnitView(object_index).reset_orientation_and_find_position(vehicle_index);
}

/**
 * C entry point for halo::units::UnitView::reset_velocity_and_ground_flag; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56a290
 */
void unit_reset_velocity_and_ground_flag(uint32_t unit_index, uint8_t set_flag)
{
    halo::units::UnitView(unit_index).reset_velocity_and_ground_flag(set_flag);
}

/**
 * C entry point for halo::units::UnitView::rotate_basis_about_axis; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e6b0
 */
void unit_rotate_basis_about_axis(uint32_t object_index)
{
    halo::units::UnitView(object_index).rotate_basis_about_axis();
}

/**
 * C entry point for halo::units::UnitView::sample_camera_shake_from_velocity; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56bfc0
 */
void unit_sample_camera_shake_from_velocity(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).sample_camera_shake_from_velocity();
}

/**
 * C entry point for halo::units::UnitView::scripted_action_animation_exists; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569470
 */
uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command)
{
    return halo::units::UnitView(unit_index).scripted_action_animation_exists(command);
}

/**
 * C entry point for halo::units::UnitView::scripting_set_emotion_animation; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569cf0
 */
void unit_scripting_set_emotion_animation(uint32_t unit_index, const char *emotion_name)
{
    halo::units::UnitView(unit_index).scripting_set_emotion_animation(emotion_name);
}

/**
 * C entry point for halo::units::unit_scripting_set_or_drop_weapon; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ddb0
 */
void unit_scripting_set_or_drop_weapon(int32_t *message)
{
    halo::units::unit_scripting_set_or_drop_weapon(message);
}

/**
 * C entry point for halo::units::unit_seat_candidates_from_zone_and_enter; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56a4c0
 */
int16_t unit_seat_candidates_from_zone_and_enter(datum_index vehicle_index, char *seat_name, datum_index object_list)
{
    return halo::units::unit_seat_candidates_from_zone_and_enter(vehicle_index, seat_name, object_list);
}

/**
 * C entry point for halo::units::UnitView::seat_flag_bit10; forwards to the C++ implementation unchanged.
 *
 * @address 0x56cdd0
 */
uint8_t unit_seat_flag_bit10(uint32_t unit_index, int16_t seat_index)
{
    return halo::units::UnitView(unit_index).seat_flag_bit10(seat_index);
}

/**
 * C entry point for halo::units::UnitView::seat_flag_bit2; forwards to the C++ implementation unchanged.
 *
 * @address 0x56cd10
 */
uint8_t unit_seat_flag_bit2(uint32_t unit_index, int16_t seat_index)
{
    return halo::units::UnitView(unit_index).seat_flag_bit2(seat_index);
}

/**
 * C entry point for halo::units::UnitView::seat_flag_bit3; forwards to the C++ implementation unchanged.
 *
 * @address 0x56cd70
 */
uint8_t unit_seat_flag_bit3(uint32_t unit_index, int16_t seat_index)
{
    return halo::units::UnitView(unit_index).seat_flag_bit3(seat_index);
}

/**
 * C entry point for halo::units::unit_seat_index_is_valid; forwards to the C++ implementation unchanged.
 *
 * @address 0x565150
 */
uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index)
{
    return halo::units::unit_seat_index_is_valid(other_object_index, unit_index, seat_index);
}

/**
 * C entry point for halo::units::unit_seat_is_occupied_by_other; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566840
 */
uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index, uint32_t *out_occupant_index)
{
    return halo::units::unit_seat_is_occupied_by_other(self_index, seat_index, vehicle_index, out_occupant_index);
}

/**
 * C entry point for halo::units::UnitView::set_control_countdown; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x563b20
 */
void unit_set_control_countdown(uint32_t unit_index, int32_t countdown, uint32_t extra_control_flags)
{
    halo::units::UnitView(unit_index).set_control_countdown(countdown, extra_control_flags);
}

/**
 * C entry point for halo::units::UnitView::set_custom_animation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56ebd0
 */
void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index)
{
    halo::units::UnitView(object_index).set_custom_animation(graph, animation_index);
}

/**
 * C entry point for halo::units::UnitView::set_custom_animation_frame; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570220
 */
uint8_t unit_set_custom_animation_frame(uint32_t unit_index, uint8_t warn_if_missing, datum_index graph_tag_id, const char *animation_name, int16_t frame)
{
    return halo::units::UnitView(unit_index).set_custom_animation_frame(warn_if_missing, graph_tag_id, animation_name, frame);
}

/**
 * C entry point for halo::units::UnitView::set_facing_from_index_table; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570de0
 */
void unit_set_facing_from_index_table(uint32_t object_index)
{
    halo::units::UnitView(object_index).set_facing_from_index_table();
}

/**
 * C entry point for halo::units::UnitView::set_grenade_type_and_count_delta; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x56d160
 */
int32_t unit_set_grenade_type_and_count_delta(uint32_t unit_index, int16_t grenade_type, int8_t delta)
{
    return halo::units::UnitView(unit_index).set_grenade_type_and_count_delta(grenade_type, delta);
}

/**
 * C entry point for halo::units::UnitView::set_or_test_seat_and_weapon_label; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5651e0
 */
uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, const char *seat_label, const char *weapon_label, uint8_t apply)
{
    return halo::units::UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_label, weapon_label, apply);
}

/**
 * C entry point for halo::units::UnitView::set_throw_aim_direction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5704d0
 */
void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy)
{
    halo::units::UnitView(object_index).set_throw_aim_direction(direction_xy);
}

/**
 * C entry point for halo::units::UnitView::snap_to_min_ground_height; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ecf0
 */
uint32_t unit_snap_to_min_ground_height(uint32_t object_index)
{
    return halo::units::UnitView(object_index).snap_to_min_ground_height();
}

/**
 * C entry point for halo::units::unit_spawn_with_starting_weapons; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572110
 */
void unit_spawn_with_starting_weapons(void *command_record)
{
    halo::units::unit_spawn_with_starting_weapons(command_record);
}

/**
 * C entry point for halo::units::UnitView::start_seat_overlay_animation_a; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565e00
 */
void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command)
{
    halo::units::UnitView(unit_index).start_seat_overlay_animation_a(command);
}

/**
 * C entry point for halo::units::UnitView::start_seat_overlay_animation_b; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566410
 */
void unit_start_seat_overlay_animation_b(uint32_t unit_index, int16_t command)
{
    halo::units::UnitView(unit_index).start_seat_overlay_animation_b(command);
}

/**
 * C entry point for halo::units::UnitView::start_user_animation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5702a0
 */
uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name, uint8_t interpolate)
{
    return halo::units::UnitView(unit_index).start_user_animation(graph_tag, animation_name, interpolate);
}

/**
 * C entry point for halo::units::unit_state_allows_control; forwards to the C++ implementation unchanged.
 *
 * @address 0x565ca0
 */
uint8_t unit_state_allows_control(const uint8_t *animation_block)
{
    return halo::units::unit_state_allows_control(animation_block);
}

/**
 * C entry point for halo::units::unit_state_is_scripted_animation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565c60
 */
uint8_t unit_state_is_scripted_animation(unit_data *unit)
{
    return halo::units::unit_state_is_scripted_animation(unit);
}

/**
 * C entry point for halo::units::UnitView::submit_periodic_network_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55b440
 */
int32_t unit_submit_periodic_network_update(datum_index object_index, void *buffer, int32_t bit_budget, int32_t update_type)
{
    return halo::units::UnitView(object_index).submit_periodic_network_update(buffer, bit_budget, update_type);
}

/**
 * C entry point for halo::units::UnitView::test_placement_candidate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55aa20
 */
int32_t unit_test_placement_candidate(uint32_t unit_index, const real_vector3d *direction, real_vector3d *out_normal, float distance, real_point3d *out_position)
{
    return halo::units::UnitView(unit_index).test_placement_candidate(direction, out_normal, distance, out_position);
}

/**
 * C entry point for halo::units::UnitView::throw_grenade_move_to_hand; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56e280
 */
void unit_throw_grenade_move_to_hand(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).throw_grenade_move_to_hand();
}

/**
 * C entry point for halo::units::unit_throw_grenade_release; forwards to the C++ implementation unchanged.
 *
 * @address 0x571b40
 */
void unit_throw_grenade_release(void)
{
    halo::units::unit_throw_grenade_release();
}

/**
 * C entry point for halo::units::UnitView::track_target_lock_timeout; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ec90
 */
void unit_track_target_lock_timeout(uint32_t object_index)
{
    halo::units::UnitView(object_index).track_target_lock_timeout();
}

/**
 * C entry point for halo::units::unit_trigger_material_hit_effect; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56f210
 */
void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index)
{
    halo::units::unit_trigger_material_hit_effect(material_index, unit_tag_id, object_index);
}

/**
 * C entry point for halo::units::UnitView::try_exit_controlled_seat; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56b5f0
 */
void unit_try_exit_controlled_seat(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).try_exit_controlled_seat();
}

/**
 * C entry point for halo::units::unit_try_give_grenade; forwards to the C++ implementation unchanged.
 *
 * @address 0x56d080
 */
uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index)
{
    return halo::units::unit_try_give_grenade(tag_source_index, unit_index);
}

/**
 * C entry point for halo::units::UnitView::try_ready_weapon; forwards to the C++ implementation unchanged.
 *
 * @address 0x569a20
 */
uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const real_vector2d *direction)
{
    return halo::units::UnitView(unit_index).try_ready_weapon(forced, direction);
}

/**
 * C entry point for halo::units::UnitView::try_ready_weapon_variant; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x569b30
 */
uint8_t unit_try_ready_weapon_variant(uint32_t unit_index, const real_vector2d *direction)
{
    return halo::units::UnitView(unit_index).try_ready_weapon_variant(direction);
}

/**
 * C entry point for halo::units::UnitView::try_select_equipment; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56d1a0
 */
uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index, int16_t release_current)
{
    return halo::units::UnitView(unit_index).try_select_equipment(new_equipment_object_index, release_current);
}

/**
 * C entry point for halo::units::UnitView::try_set_animation_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565f90
 */
uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state)
{
    return halo::units::UnitView(unit_index).try_set_animation_state(new_state);
}

/**
 * C entry point for halo::units::UnitView::try_start_scripted_action_animation; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x569530
 */
uint8_t unit_try_start_scripted_action_animation(uint32_t unit_index, int16_t command, const real_vector2d *direction)
{
    return halo::units::UnitView(unit_index).try_start_scripted_action_animation(command, direction);
}

/**
 * C entry point for halo::units::unit_try_start_seat_exit_animation; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c470
 */
uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index)
{
    return halo::units::unit_try_start_seat_exit_animation(force_flag, unit_index);
}

/**
 * C entry point for halo::units::UnitView::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x5625b0
 */
uint8_t unit_update(uint32_t unit_index)
{
    return halo::units::UnitView(unit_index).update();
}

/**
 * C entry point for halo::units::UnitView::update_aiming_overlay_angles; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x563b50
 */
void unit_update_aiming_overlay_angles(uint32_t unit_index, void *output)
{
    halo::units::UnitView(unit_index).update_aiming_overlay_angles(output);
}

/**
 * C entry point for halo::units::UnitView::update_animation_state_machine; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565420
 */
uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request)
{
    return halo::units::UnitView(unit_index).update_animation_state_machine(request);
}

/**
 * C entry point for halo::units::UnitView::update_animation_timers; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561620
 */
void unit_update_animation_timers(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).update_animation_timers();
}

/**
 * C entry point for halo::units::UnitView::update_autoaim_interaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570720
 */
void unit_update_autoaim_interaction(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).update_autoaim_interaction();
}

/**
 * C entry point for halo::units::UnitView::update_footstep_and_idle_triggers; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x560410
 */
void unit_update_footstep_and_idle_triggers(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).update_footstep_and_idle_triggers();
}

/**
 * C entry point for halo::units::UnitView::update_ground_contact_counter; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575640
 */
void unit_update_ground_contact_counter(uint32_t unit_index, uint8_t *contact_points)
{
    halo::units::UnitView(unit_index).update_ground_contact_counter(contact_points);
}

/**
 * C entry point for halo::units::UnitView::update_ik_detail_nodes; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5643f0
 */
void unit_update_ik_detail_nodes(uint32_t object_index, void *node_base)
{
    halo::units::UnitView(object_index).update_ik_detail_nodes(node_base);
}

/**
 * C entry point for halo::units::UnitView::update_look_delta_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56e820
 */
void unit_update_look_delta_controls(uint32_t object_index)
{
    halo::units::UnitView(object_index).update_look_delta_controls();
}

/**
 * C entry point for halo::units::UnitView::update_marker_skid_effects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575460
 */
void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points)
{
    halo::units::UnitView(unit_index).update_marker_skid_effects(contact_points);
}

/**
 * C entry point for halo::units::UnitView::update_marker_traction_effects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575170
 */
uint32_t unit_update_marker_traction_effects(uint32_t object_index)
{
    return halo::units::UnitView(object_index).update_marker_traction_effects();
}

/**
 * C entry point for halo::units::UnitView::update_random_turn_angle; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570840
 */
void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis)
{
    halo::units::UnitView(object_index).update_random_turn_angle(out_axis);
}

/**
 * C entry point for halo::units::UnitView::update_recoil_decay; forwards to the C++ implementation unchanged.
 *
 * @address 0x574780
 */
void unit_update_recoil_decay(uint32_t object_index)
{
    halo::units::UnitView(object_index).update_recoil_decay();
}

/**
 * C entry point for halo::units::UnitView::update_scale_function_inputs; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x563860
 */
void unit_update_scale_function_inputs(uint32_t object_index)
{
    halo::units::UnitView(object_index).update_scale_function_inputs();
}

/**
 * C entry point for halo::units::UnitView::update_stance_and_jump; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566de0
 */
void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still)
{
    halo::units::UnitView(unit_index).update_stance_and_jump(force_ready, allow_death_reaction, suppress_shield_check, ignore_disoriented, force_reaction, turn_angle, weapon_class_index, throttle, require_still);
}

/**
 * C entry point for halo::units::UnitView::update_steering_deviation_effects; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574f30
 */
void unit_update_steering_deviation_effects(uint32_t unit_index, real_vector3d *reference_direction, uint8_t *contact_points)
{
    halo::units::UnitView(unit_index).update_steering_deviation_effects(reference_direction, contact_points);
}

/**
 * C entry point for halo::units::unit_update_up_vector; forwards to the C++ implementation unchanged.
 *
 * @address 0x560800
 */
void unit_update_up_vector(Biped *biped_tag, object *obj)
{
    halo::units::unit_update_up_vector(biped_tag, obj);
}

/**
 * C entry point for halo::units::UnitView::update_vitality_fractions; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561b80
 */
void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta)
{
    halo::units::UnitView(unit_index).update_vitality_fractions(body_delta, shield_delta);
}

/**
 * C entry point for halo::units::UnitView::validate_and_clear_weapon_switch; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5659c0
 */
void unit_validate_and_clear_weapon_switch(uint32_t unit_index)
{
    halo::units::UnitView(unit_index).validate_and_clear_weapon_switch();
}

/**
 * C entry point for halo::units::unit_weapon_is_best_of_type; forwards to the C++ implementation unchanged.
 *
 * @address 0x56dae0
 */
uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index)
{
    return halo::units::unit_weapon_is_best_of_type(reference_weapon_index, unit_index);
}

/**
 * C entry point for halo::units::VehicleView::apply_network_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5726e0
 */
void vehicle_apply_network_update(datum_index vehicle_index, void **message, uint8_t *connection)
{
    halo::units::VehicleView(vehicle_index).apply_network_update(message, connection);
}

/**
 * C entry point for halo::units::VehicleView::blend_animations; forwards to the C++ implementation unchanged.
 *
 * @address 0x5718e0
 */
void vehicle_blend_animations(datum_index object_index, real_orientation *orientations)
{
    halo::units::VehicleView(object_index).blend_animations(orientations);
}

/**
 * C entry point for halo::units::VehicleView::calculate_animation_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5756f0
 */
void vehicle_calculate_animation_controls(uint32_t unit_index)
{
    halo::units::VehicleView(unit_index).calculate_animation_controls();
}

/**
 * C entry point for halo::units::VehicleView::calculate_ground_contact_lean; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x573f60
 */
void vehicle_calculate_ground_contact_lean(uint32_t unit_index, void *out_record, void *out_transform)
{
    halo::units::VehicleView(unit_index).calculate_ground_contact_lean(out_record, out_transform);
}

/**
 * C entry point for halo::units::VehicleView::calculate_ground_contact_lean_alt; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574460
 */
void vehicle_calculate_ground_contact_lean_alt(uint32_t unit_index, void *out_record, void *out_transform)
{
    halo::units::VehicleView(unit_index).calculate_ground_contact_lean_alt(out_record, out_transform);
}

/**
 * C entry point for halo::units::VehicleView::calculate_ground_lean_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x573100
 */
void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform)
{
    halo::units::VehicleView(unit_index).calculate_ground_lean_controls(out_transform);
}

/**
 * C entry point for halo::units::vehicle_calculate_hover_lift_toward_target; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5739a0
 */
void vehicle_calculate_hover_lift_toward_target(void)
{
    halo::units::vehicle_calculate_hover_lift_toward_target();
}

/**
 * C entry point for halo::units::vehicle_calculate_hover_turn_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5738b0
 */
void vehicle_calculate_hover_turn_controls(void)
{
    halo::units::vehicle_calculate_hover_turn_controls();
}

/**
 * C entry point for halo::units::VehicleView::calculate_lean_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572df0
 */
void vehicle_calculate_lean_controls(uint32_t unit_index, void *mass_points, float *powered_states)
{
    halo::units::VehicleView(unit_index).calculate_lean_controls(mass_points, powered_states);
}

/**
 * C entry point for halo::units::VehicleView::calculate_mounted_controls_dispatch; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x573ee0
 */
void vehicle_calculate_mounted_controls_dispatch(uint32_t unit_index, void *out_transform, void *out_record)
{
    halo::units::VehicleView(unit_index).calculate_mounted_controls_dispatch(out_transform, out_record);
}

/**
 * C entry point for halo::units::VehicleView::calculate_steering_wheel_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x572cd0
 */
void vehicle_calculate_steering_wheel_controls(uint32_t unit_index, void *mass_points, float *powered_states)
{
    halo::units::VehicleView(unit_index).calculate_steering_wheel_controls(mass_points, powered_states);
}

/**
 * C entry point for halo::units::VehicleView::calculate_turret_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572b60
 */
void vehicle_calculate_turret_controls(uint32_t unit_index, void *mass_points, float *powered_states)
{
    halo::units::VehicleView(unit_index).calculate_turret_controls(mass_points, powered_states);
}

/**
 * C entry point for halo::units::VehicleView::calculate_wing_flex_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5734d0
 */
void vehicle_calculate_wing_flex_controls(uint32_t unit_index, float angle, uint8_t *node_output, uint8_t *contact_points)
{
    halo::units::VehicleView(unit_index).calculate_wing_flex_controls(angle, node_output, contact_points);
}

/**
 * C entry point for halo::units::VehicleView::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x570bb0
 */
uint8_t vehicle_create(datum_index object_index)
{
    return halo::units::VehicleView(object_index).create();
}

/**
 * C entry point for halo::units::VehicleView::create_hover_thruster_effects; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574900
 */
void vehicle_create_hover_thruster_effects(uint32_t unit_index)
{
    halo::units::VehicleView(unit_index).create_hover_thruster_effects();
}

/**
 * C entry point for halo::units::VehicleView::create_hover_thruster_midpoint_effects; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574bc0
 */
void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index)
{
    halo::units::VehicleView(unit_index).create_hover_thruster_midpoint_effects();
}

/**
 * C entry point for halo::units::VehicleView::encode_network_create; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x571f20
 */
int32_t vehicle_encode_network_create(datum_index vehicle_index, int32_t buffer, int32_t bit_budget)
{
    return halo::units::VehicleView(vehicle_index).encode_network_create(buffer, bit_budget);
}

/**
 * C entry point for halo::units::VehicleView::encode_network_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5724d0
 */
int32_t vehicle_encode_network_update(datum_index vehicle_index, void *buffer, int32_t bit_budget, int32_t full_update)
{
    return halo::units::VehicleView(vehicle_index).encode_network_update(buffer, bit_budget, full_update);
}

/**
 * C entry point for halo::units::VehicleView::is_old_enough; forwards to the C++ implementation unchanged.
 *
 * @address 0x572a30
 */
uint8_t vehicle_is_old_enough(uint32_t object_index)
{
    return halo::units::VehicleView(object_index).is_old_enough();
}

/**
 * C entry point for halo::units::VehicleView::network_baseline_take; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572410
 */
void vehicle_network_baseline_take(uint32_t object_index)
{
    halo::units::VehicleView(object_index).network_baseline_take();
}

/**
 * C entry point for halo::units::VehicleView::reset_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x570b00
 */
void vehicle_reset_state(uint32_t object_index)
{
    halo::units::VehicleView(object_index).reset_state();
}

/**
 * C entry point for halo::units::VehicleView::update; forwards to the C++ implementation unchanged.
 *
 * @address 0x570ee0
 */
uint32_t vehicle_update(uint32_t object_index)
{
    return halo::units::VehicleView(object_index).update();
}
