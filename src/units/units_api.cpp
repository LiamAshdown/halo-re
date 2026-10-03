#include "halo/units/unit.hpp"
#include "halo/units/api.hpp"
#include "halo/core/link.hpp"
#include "halo/units/vars.hpp"

static auto &unit_updates_suppressed = halo::link::ref<uint8_t>(halo::units::vars().unit_updates_suppressed);

namespace halo::units {

Globals &globals()
{
    static Globals instance{::unit_updates_suppressed};
    return instance;
}

/**
 * C entry point for halo::units::BipedView::advance_frame_counter_trigger; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55eb90
 */


/**
 * C entry point for halo::units::BipedView::apply_idle_fidget; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e940
 */


/**
 * C entry point for halo::units::BipedView::check_evade_reaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e190
 */


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


/**
 * C entry point for halo::units::BipedView::ground_adjust_solve; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x558000
 */


/**
 * C entry point for halo::units::BipedView::ground_adjust_solve_node; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x557b80
 */


/**
 * C entry point for halo::units::BipedView::ground_adjust_step; forwards to the C++ implementation unchanged.
 *
 * @address 0x557a90
 */


/**
 * C entry point for halo::units::BipedView::integrate_movement; forwards to the C++ implementation unchanged.
 *
 * @address 0x55bea0
 */


/**
 * C entry point for halo::units::BipedView::integrate_movement_with_collision; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x55cfd0
 */


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
 * C entry point for halo::units::BipedView::update_facing; forwards to the C++ implementation unchanged.
 *
 * @address 0x55b7c0
 */


/**
 * C entry point for halo::units::BipedView::update_idle_basis; forwards to the C++ implementation unchanged.
 *
 * @address 0x55e840
 */


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
int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_communication_hold_tick, int16_t *dialogue_index, int32_t *chain_value)
{
    return halo::units::UnitView(unit_index).animation_change_priority_check(follow_fallback, requested_priority, allow_repeat, out_communication_hold_tick, dialogue_index, chain_value);
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


/**
 * C entry point for halo::units::UnitView::begin_throw_grenade; forwards to the C++ implementation unchanged.
 *
 * @address 0x56e080
 */


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


/**
 * C entry point for halo::units::UnitView::cause_melee_damage; forwards to the C++ implementation unchanged.
 *
 * @address 0x56f2d0
 */


/**
 * C entry point for halo::units::UnitView::check_fell_off_level; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e4a0
 */


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


/**
 * C entry point for halo::units::UnitView::choose_dialogue_variant; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x561990
 */


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
 * C entry point for halo::units::UnitView::detach_from_seat; forwards to the C++ implementation unchanged.
 *
 * @address 0x56c640
 */
void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    halo::units::UnitView(unit_index).detach_from_seat(suppress_trigger, require_client_flag, fire_trigger_event);
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


/**
 * C entry point for halo::units::UnitView::drop_inventory_weapons; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56f060
 */


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


/**
 * C entry point for halo::units::UnitView::enter_stunned_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x5705a0
 */


/**
 * C entry point for halo::units::UnitView::evaluate_flee_reaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55e2d0
 */


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


/**
 * C entry point for halo::units::UnitView::find_nearest_valid_surface_plane; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x560630
 */


/**
 * C entry point for halo::units::UnitView::find_next_grenade_type_with_count; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5699a0
 */


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


/**
 * C entry point for halo::units::UnitView::get_seat_or_state_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56c2f0
 */


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
uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, float *out_dx_to_key_frame, float *out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index)
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


/**
 * C entry point for halo::units::UnitView::is_seat_control_available; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5693a0
 */


/**
 * C entry point for halo::units::UnitView::melee_attack_scan; forwards to the C++ implementation unchanged.
 *
 * @address 0x56f550
 */


/**
 * C entry point for halo::units::UnitView::melee_lunge_damage_tick; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x56fc80
 */


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
 * C entry point for halo::units::UnitView::new_; forwards to the C++ implementation unchanged.
 *
 * @address 0x562180
 */
uint8_t unit_new(uint32_t object_index)
{
    return halo::units::UnitView(object_index).new_();
}

/**
 * C entry point for halo::units::UnitView::resolve_camera_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x569670
 */
uint32_t unit_noop_569670(uint32_t object_index)
{
    return halo::units::UnitView(object_index).resolve_camera_object();
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
 * C entry point for halo::units::UnitView::pick_random_spawned_actor_count; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x568540
 */


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


/**
 * C entry point for halo::units::UnitView::release_transient_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x568610
 */


/**
 * C entry point for halo::units::UnitView::release_transient_state_and_detach; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x568cb0
 */


/**
 * C entry point for halo::units::UnitView::reset_ground_adjust_state; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ad00
 */


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


/**
 * C entry point for halo::units::UnitView::set_throw_aim_direction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x5704d0
 */


/**
 * C entry point for halo::units::UnitView::snap_to_min_ground_height; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ecf0
 */


/**
 * C entry point for halo::units::UnitView::start_seat_overlay_animation_a; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x565e00
 */


/**
 * C entry point for halo::units::UnitView::start_seat_overlay_animation_b; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x566410
 */


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


/**
 * C entry point for halo::units::UnitView::track_target_lock_timeout; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x55ec90
 */


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


/**
 * C entry point for halo::units::UnitView::update_autoaim_interaction; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570720
 */


/**
 * C entry point for halo::units::UnitView::update_footstep_and_idle_triggers; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x560410
 */


/**
 * C entry point for halo::units::UnitView::update_ground_contact_counter; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575640
 */


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


/**
 * C entry point for halo::units::UnitView::update_marker_skid_effects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575460
 */


/**
 * C entry point for halo::units::UnitView::update_marker_traction_effects; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x575170
 */


/**
 * C entry point for halo::units::UnitView::update_random_turn_angle; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x570840
 */


/**
 * C entry point for halo::units::UnitView::update_recoil_decay; forwards to the C++ implementation unchanged.
 *
 * @address 0x574780
 */


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


/**
 * C entry point for halo::units::VehicleView::calculate_ground_contact_lean_alt; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574460
 */


/**
 * C entry point for halo::units::VehicleView::calculate_ground_lean_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x573100
 */


/**
 * C entry point for halo::units::VehicleView::calculate_lean_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572df0
 */


/**
 * C entry point for halo::units::VehicleView::calculate_mounted_controls_dispatch; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x573ee0
 */


/**
 * C entry point for halo::units::VehicleView::calculate_steering_wheel_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x572cd0
 */


/**
 * C entry point for halo::units::VehicleView::calculate_turret_controls; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x572b60
 */


/**
 * C entry point for halo::units::VehicleView::calculate_wing_flex_controls; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x5734d0
 */


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


/**
 * C entry point for halo::units::VehicleView::create_hover_thruster_midpoint_effects; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x574bc0
 */


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

}
