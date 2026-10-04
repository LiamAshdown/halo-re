#include "halo/ai/actor_view.hpp"
#include "halo/ai/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

static auto &actor_data = halo::link::ref<data_array *>(halo::ai::vars().actor_data);
static auto &prop_data = halo::link::ref<data_array *>(halo::ai::vars().prop_data);
static auto &encounter_data = halo::link::ref<data_array *>(halo::ai::vars().encounter_data);
static auto &swarm_data = halo::link::ref<data_array *>(halo::ai::vars().swarm_data);
static auto &swarm_component_data = halo::link::ref<data_array *>(halo::ai::vars().swarm_component_data);
static auto &ai_conversation_data = halo::link::ref<data_array *>(halo::ai::vars().ai_conversation_data);
static auto &ai_pursuit_data = halo::link::ref<data_array *>(halo::ai::vars().ai_pursuit_data);
static auto &ai_globals_ptr = halo::link::ref<ai_globals *>(halo::ai::vars().ai_globals_ptr);
static auto &encounter_squad_states = halo::link::ref<encounter_squad_state *>(halo::ai::vars().encounter_squad_states);
static auto &encounter_platoon_states = halo::link::ref<encounter_platoon_state *>(halo::ai::vars().encounter_platoon_states);
static auto &ai_communication_quiet_until_tick = halo::link::ref<int32_t>(halo::ai::vars().ai_communication_quiet_until_tick);

namespace halo::ai {

Globals &globals()
{
    static Globals instance{::actor_data, ::prop_data, ::encounter_data, ::swarm_data, ::swarm_component_data, ::ai_conversation_data, ::ai_pursuit_data, ::ai_globals_ptr, ::encounter_squad_states, ::encounter_platoon_states, ::ai_communication_quiet_until_tick};
    return instance;
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_uncover_tick; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x408470
 */
void actor_mode_uncover_tick(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_uncover_tick();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_uncover_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x408680
 */
void actor_mode_uncover_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_uncover_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_vehicle_enter; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x408b50
 */
void actor_mode_vehicle_enter(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_vehicle_enter();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_vehicle_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x408e80
 */
void actor_mode_vehicle_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_vehicle_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_wait_process; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x409b30
 */
uint8_t actor_mode_wait_process(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).mode_wait_process();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_wait_tick; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x409cc0
 */
void actor_mode_wait_tick(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_wait_tick();
}

/**
 * Free-function entry point for halo::ai::ActorView::mode_wait_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl, called through the mode table).
 *
 * @address 0x409dc0
 */
void actor_mode_wait_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).mode_wait_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_action_cancel; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDI -> actor_index.
 *
 * @address 0x428650
 */
void actor_movement_action_cancel(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).movement_action_cancel();
}

/**
 * Free-function entry point for halo::ai::ActorView::run_movement_action_complete; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x41a430
 */
void actor_movement_action_complete(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).run_movement_action_complete();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_action_in_progress; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x41a980
 */
uint8_t actor_movement_action_in_progress(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).movement_action_in_progress();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_action_is_complete; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x41a960
 */
uint8_t actor_movement_action_is_complete(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).movement_action_is_complete();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_action_resolve; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, record_distance, context.
 *
 * @address 0x41a460
 */
uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context)
{
    return halo::ai::ActorView(actor_index).movement_action_resolve(record_distance, context);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_action_stop; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDX -> actor_index.
 *
 * @address 0x417570
 */
void actor_movement_action_stop(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).movement_action_stop();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_actions_cancel; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x417a30
 */
void actor_movement_actions_cancel(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).movement_actions_cancel();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_advance_waypoint; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x4163e0
 */
void actor_movement_advance_waypoint(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).movement_advance_waypoint();
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_apply_steering; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> cached_axis, ECX -> keep_z, stack -> the 15 parameters in order.
 *
 * @address 0x4180c0
 */
void actor_movement_apply_steering(int16_t cached_axis, uint8_t keep_z, datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed, float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale, float throttle_maximum, real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis, real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506)
{
    halo::ai::ActorOps::movement_apply_steering(cached_axis, keep_z, actor_index, want_avoid_check, avoid_threshold, order_failed, steering_maximum, oversteer_min, oversteer_max, avoidance_scale, throttle_maximum, desired_direction, out_direction, out_axis, out_heading, out_flag_507, out_flag_506);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_check_arrival; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x416700
 */
uint8_t actor_movement_check_arrival(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).movement_check_arrival();
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_choose_avoidance_direction; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> (actor_index, desired, out_direction, out_scale).
 *
 * @address 0x4193d0
 */
void actor_movement_choose_avoidance_direction(uint32_t actor_index, real_vector3d *desired, real_vector3d *out_direction, float *out_scale)
{
    halo::ai::ActorView(actor_index).movement_choose_avoidance_direction(desired, out_direction, out_scale);
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_choose_strafe_axis; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> direction, BL -> use_3d, ESI -> facing, EDI -> reference, stack -> out_axis, out_index.
 *
 * @address 0x418a40
 */
void actor_movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d, const real_vector3d *facing, const real_vector3d *reference, real_vector3d *out_axis, int16_t *out_index)
{
    halo::ai::ActorOps::movement_choose_strafe_axis(direction, use_3d, facing, reference, out_axis, out_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_collect_obstacle_candidates; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> context.
 *
 * @address 0x418ce0
 */
void actor_movement_collect_obstacle_candidates(actor_movement_context *context)
{
    halo::ai::ActorOps::movement_collect_obstacle_candidates(context);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_flying_needs_steering; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> destination, EDI -> out_avoidance_distance.
 *
 * @address 0x41aab0
 */
uint8_t actor_movement_flying_needs_steering(datum_index actor_index, const real_point3d *destination, float *out_avoidance_distance)
{
    return halo::ai::ActorView(actor_index).movement_flying_needs_steering(destination, out_avoidance_distance);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_get_stopping_distances; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EBX -> out_accelerate_stop_distance, EDI -> out_stop_distance.
 *
 * @address 0x4173a0
 */
void actor_movement_get_stopping_distances(datum_index actor_index, float *out_accelerate_stop_distance, float *out_stop_distance)
{
    halo::ai::ActorView(actor_index).movement_get_stopping_distances(out_accelerate_stop_distance, out_stop_distance);
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_project_into_frame; forwards to the C++ implementation unchanged.
 * Register convention of the original: AL -> use_3d, ECX -> frame_axis, stack -> v, out.
 *
 * @address 0x418c20
 */
void actor_movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis, const real_vector3d *v, real_vector3d *out)
{
    halo::ai::ActorOps::movement_project_into_frame(use_3d, frame_axis, v, out);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_set_destination_firing_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDI -> actor_index, stack -> formation_slot, path_context (reused by the path request when not null).
 *
 * @address 0x417830
 */
uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot, path_find_context *path_context)
{
    return halo::ai::ActorView(actor_index).movement_set_destination_firing_position(formation_slot, path_context);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_set_destination_move_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDI -> actor_index, stack -> move_position_index.
 *
 * @address 0x417750
 */
uint8_t actor_movement_set_destination_move_position(datum_index actor_index, int16_t move_position_index)
{
    return halo::ai::ActorView(actor_index).movement_set_destination_move_position(move_position_index);
}

/**
 * Free-function entry point for halo::ai::TargetView::movement_set_destination_near_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index, stack -> actor_index, stack -> radius.
 *
 * @address 0x417910
 */
uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index, float radius)
{
    return halo::ai::TargetView(target_prop_index).movement_set_destination_near_target(actor_index, radius);
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_set_destination_point; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> destination, stack -> actor_index, stack -> parameter, stack -> extra.
 *
 * @address 0x417610
 */
uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra)
{
    return halo::ai::ActorOps::movement_set_destination_point(destination, actor_index, parameter, extra);
}

/**
 * Free-function entry point for halo::ai::ActorOps::movement_test_obstacle_ray; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> out_elevation, ECX -> sample, EDX -> out_end_point, EDI -> context,.
 *
 * @address 0x418f70
 */
int16_t actor_movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample, real_point3d *out_end_point, actor_movement_context *context, float *out_distance, uint8_t *out_clear_counter)
{
    return halo::ai::ActorOps::movement_test_obstacle_ray(out_elevation, sample, out_end_point, context, out_distance, out_clear_counter);
}

/**
 * Free-function entry point for halo::ai::ActorView::movement_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x416790
 */
void actor_movement_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).movement_update();
}

/**
 * Free-function entry point for halo::ai::ActorOps::run_new; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDX -> array.
 *
 * @address 0x426760
 */
datum_index actor_new(datum_index actor_variant_tag)
{
    return halo::ai::ActorOps::run_new(actor_variant_tag);
}

/**
 * Free-function entry point for halo::ai::ActorOps::new_and_attach_to_unit; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> the twelve arguments.
 *
 * @address 0x426ac0
 */
datum_index actor_new_and_attach_to_unit(char reuse_existing, datum_index unit_index, datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor, char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68)
{
    return halo::ai::ActorOps::new_and_attach_to_unit(reuse_existing, unit_index, actor_variant_tag, encounter_or_none, squad_index, ignore_squad, exclude_actor, start_active, unknown_60, unknown_62, unknown_90, unknown_68);
}

/**
 * Free-function entry point for halo::ai::ActorView::notify_squad_and_flag_danger; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> alternate_event, stack -> raise_danger_flag.
 *
 * @address 0x423600
 */
void actor_notify_squad_and_flag_danger(datum_index actor_index, uint8_t alternate_event, uint8_t raise_danger_flag)
{
    halo::ai::ActorView(actor_index).notify_squad_and_flag_danger(alternate_event, raise_danger_flag);
}

/**
 * Free-function entry point for halo::ai::ActorOps::notify_squad_of_threat_direction; forwards to the C++ implementation unchanged.
 * Register convention of the original: EBX -> point, EDI -> actor_index, stack -> event_kind, grenade_type_code.
 *
 * @address 0x4234f0
 */
void actor_notify_squad_of_threat_direction(const real_point3d *point, datum_index actor_index, int16_t event_kind, int16_t grenade_type_code)
{
    halo::ai::ActorOps::notify_squad_of_threat_direction(point, actor_index, event_kind, grenade_type_code);
}

/**
 * Free-function entry point for halo::ai::TargetView::notify_target_engaged; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index, ECX -> actor_index, EDX -> alternate_event.
 *
 * @address 0x4220c0
 */
void actor_notify_target_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t alternate_event)
{
    halo::ai::TargetView(target_prop_index).notify_target_engaged(actor_index, alternate_event);
}

/**
 * Free-function entry point for halo::ai::ActorOps::notify_weapon_pickup_once; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> object_index.
 *
 * @address 0x42c370
 */
void actor_notify_weapon_pickup_once(datum_index object_index)
{
    halo::ai::ActorOps::notify_weapon_pickup_once(object_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::obey_member_advance; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra.
 *
 * @address 0x406f80
 */
void actor_obey_member_advance(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    halo::ai::ActorView(actor_index).obey_member_advance(unit_index, command_list_index, action, aim, callback_extra);
}

/**
 * Free-function entry point for halo::ai::ActorView::obey_member_enter; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra.
 *
 * @address 0x406f30
 */
void actor_obey_member_enter(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    halo::ai::ActorView(actor_index).obey_member_enter(unit_index, command_list_index, action, aim, callback_extra);
}

/**
 * Free-function entry point for halo::ai::ActorView::obey_member_exit; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra.
 *
 * @address 0x406fa0
 */
void actor_obey_member_exit(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    halo::ai::ActorView(actor_index).obey_member_exit(unit_index, command_list_index, action, aim, callback_extra);
}

/**
 * Free-function entry point for halo::ai::ActorView::obey_member_tick; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra.
 *
 * @address 0x406ff0
 */
void actor_obey_member_tick(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    halo::ai::ActorView(actor_index).obey_member_tick(unit_index, command_list_index, action, aim, callback_extra);
}

/**
 * Free-function entry point for halo::ai::ActorOps::order_code_is_grenade_throw; forwards to the C++ implementation unchanged.
 * Register convention of the original: AX -> order_code.
 *
 * @address 0x404340
 */
int32_t actor_order_code_is_grenade_throw(int16_t order_code)
{
    return halo::ai::ActorOps::order_code_is_grenade_throw(order_code);
}

/**
 * Free-function entry point for halo::ai::ActorOps::pick_dialogue_variant_a; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX (low 16 bits) -> category.
 *
 * @address 0x424aa0
 */
int32_t actor_pick_dialogue_variant_a(int16_t category)
{
    return halo::ai::ActorOps::pick_dialogue_variant_a(category);
}

/**
 * Free-function entry point for halo::ai::ActorOps::pick_dialogue_variant_b; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX (low 16 bits) -> category.
 *
 * @address 0x424b80
 */
int32_t actor_pick_dialogue_variant_b(int16_t category)
{
    return halo::ai::ActorOps::pick_dialogue_variant_b(category);
}

/**
 * Free-function entry point for halo::ai::ActorOps::place_new_unit; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> placement_request, stack -> actor_variant_or_palette_tag, encounter_index,.
 *
 * @address 0x427080
 */
datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index, int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index, const actor_placement_request *placement_request)
{
    return halo::ai::ActorOps::place_new_unit(actor_variant_or_palette_tag, encounter_index, squad_index, use_palette_entry, unit_type_index, placement_request);
}

/**
 * Free-function entry point for halo::ai::ActorOps::play_first_valid_vocalization; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> seat_list, ECX -> vehicle_index, stack -> (actor_index, seat_name, seat_flags, count).
 *
 * @address 0x40e260
 */
uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index, char *seat_name, int16_t seat_flags, int16_t count)
{
    return halo::ai::ActorOps::play_first_valid_vocalization(seat_list, vehicle_index, actor_index, seat_name, seat_flags, count);
}

/**
 * Free-function entry point for halo::ai::ActorOps::point_in_directional_lane; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> to_point, ECX -> forward, EDX -> cone_axis, stack -> min_cos_threshold,.
 *
 * @address 0x414990
 */
uint8_t actor_point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis, float min_cos_threshold, float side_thresholds[2])
{
    return halo::ai::ActorOps::point_in_directional_lane(to_point, forward, cone_axis, min_cos_threshold, side_thresholds);
}

/**
 * Free-function entry point for halo::ai::ActorView::probe_step_direction; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, stack -> step_distance, ECX -> direction, stack -> variant,.
 *
 * @address 0x417e50
 */
uint8_t actor_probe_step_direction(datum_index actor_index, float step_distance, real_vector2d *direction, uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param)
{
    return halo::ai::ActorView(actor_index).probe_step_direction(step_distance, direction, variant, step_up, out_flag, extra_param);
}

/**
 * Free-function entry point for halo::ai::ActorView::process_order_request; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> (actor_index, order_code).
 *
 * @address 0x409ea0
 */
uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code)
{
    return halo::ai::ActorView(actor_index).process_order_request(order_code);
}

/**
 * Free-function entry point for halo::ai::ActorView::process_pending_command_list; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40a140
 */
uint8_t actor_process_pending_command_list(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).process_pending_command_list();
}

/**
 * Free-function entry point for halo::ai::ActorView::process_vehicle_seat_exit; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x40b080
 */
uint8_t actor_process_vehicle_seat_exit(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).process_vehicle_seat_exit();
}

/**
 * Free-function entry point for halo::ai::ActorView::prop_iterator_init; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> out_iterator.
 *
 * @address 0x43ecd0
 */
void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator)
{
    halo::ai::ActorView(actor_index).prop_iterator_init(out_iterator);
}

/**
 * Free-function entry point for halo::ai::ActorOps::prop_iterator_next; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDX -> iterator.
 *
 * @address 0x43ecf0
 */
prop * actor_prop_iterator_next(actor_prop_iterator *iterator)
{
    return halo::ai::ActorOps::prop_iterator_next(iterator);
}

/**
 * Free-function entry point for halo::ai::ActorView::propagate_unit_field; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ESI -> value.
 *
 * @address 0x4276e0
 */
void actor_propagate_unit_field(datum_index actor_index, int16_t value)
{
    halo::ai::ActorView(actor_index).propagate_unit_field(value);
}

/**
 * Free-function entry point for halo::ai::ActorView::push_recognition_entry; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, CX -> firing_position_index, DL -> type.
 *
 * @address 0x4141a0
 */
void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type)
{
    halo::ai::ActorView(actor_index).push_recognition_entry(firing_position_index, type);
}

/**
 * Free-function entry point for halo::ai::ActorOps::queue_directional_reaction_event; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> direction, ECX -> target_prop_index, stack -> actor_index.
 *
 * @address 0x422270
 */
void actor_queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::ActorOps::queue_directional_reaction_event(direction, target_prop_index, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::queue_point_reaction_dialogue; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> point, ECX -> actor_index.
 *
 * @address 0x422780
 */
void actor_queue_point_reaction_dialogue(const real_point3d *point, datum_index actor_index)
{
    halo::ai::ActorOps::queue_point_reaction_dialogue(point, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::queue_recognized_target_dialogue; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> target_prop_index.
 *
 * @address 0x422550
 */
void actor_queue_recognized_target_dialogue(datum_index actor_index, datum_index target_prop_index)
{
    halo::ai::ActorView(actor_index).queue_recognized_target_dialogue(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::queue_search_and_relay_perception; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> prop_index, EBX -> actor_index.
 *
 * @address 0x4221f0
 */
void actor_queue_search_and_relay_perception(datum_index prop_index, datum_index actor_index)
{
    halo::ai::ActorOps::queue_search_and_relay_perception(prop_index, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::queue_search_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> position, EDX -> priority, ESI -> velocity,.
 *
 * @address 0x421af0
 */
void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority, real_vector3d *velocity, uint32_t surface_index, uint32_t position_extra, uint32_t velocity_ticks, uint32_t prop_index, uint32_t prop_value, uint8_t prop_flag)
{
    halo::ai::ActorView(actor_index).queue_search_position(position, priority, velocity, surface_index, position_extra, velocity_ticks, prop_index, prop_value, prop_flag);
}

/**
 * Free-function entry point for halo::ai::ActorView::queue_secondary_action; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> action, stack -> payload.
 *
 * @address 0x417a60
 */
uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, const real_vector2d *direction)
{
    return halo::ai::ActorView(actor_index).queue_secondary_action(action, direction);
}

/**
 * Free-function entry point for halo::ai::ActorView::queue_sighted_target_dialogue; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, target_prop_index, already_noticed.
 *
 * @address 0x421c20
 */
void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index, uint8_t already_noticed)
{
    halo::ai::ActorView(actor_index).queue_sighted_target_dialogue(target_prop_index, already_noticed);
}

/**
 * Free-function entry point for halo::ai::ActorOps::queue_velocity_search_from_prop; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> prop_index, stack -> actor_index.
 *
 * @address 0x4221b0
 */
void actor_queue_velocity_search_from_prop(datum_index prop_index, datum_index actor_index)
{
    halo::ai::ActorOps::queue_velocity_search_from_prop(prop_index, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::raise_timer_5f6; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDX -> ticks.
 *
 * @address 0x40f7a0
 */
void actor_raise_timer_5f6(datum_index actor_index, int32_t ticks)
{
    halo::ai::ActorView(actor_index).raise_timer_5f6(ticks);
}

/**
 * Free-function entry point for halo::ai::ActorView::rate_potential_target; forwards to the C++ implementation unchanged.
 *
 * @address 0x41fd50
 */
float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index)
{
    return halo::ai::ActorView(actor_index).rate_potential_target(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::react_to_disturbance; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, threshold.
 *
 * @address 0x40a1e0
 */
uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold)
{
    return halo::ai::ActorView(actor_index).react_to_disturbance(threshold);
}

/**
 * Free-function entry point for halo::ai::ActorView::react_to_flee_point; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, flee_source_object, point.
 *
 * @address 0x422c00
 */
void actor_react_to_flee_point(datum_index actor_index, int32_t flee_source_object, const real_point3d *point)
{
    halo::ai::ActorView(actor_index).react_to_flee_point(flee_source_object, point);
}

/**
 * Free-function entry point for halo::ai::ActorOps::react_to_registered_danger; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> point, stack -> actor_index, danger_object_index.
 *
 * @address 0x422930
 */
void actor_react_to_registered_danger(const real_point3d *point, datum_index actor_index, int32_t danger_object_index)
{
    halo::ai::ActorOps::react_to_registered_danger(point, actor_index, danger_object_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::react_to_seen_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> target_prop_index.
 *
 * @address 0x422ec0
 */
void actor_react_to_seen_target(datum_index actor_index, datum_index target_prop_index)
{
    halo::ai::ActorView(actor_index).react_to_seen_target(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::react_to_threat_event; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> self_object_index, other_object_index, event_kind, magnitude,.
 *
 * @address 0x42be40
 */
void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay)
{
    halo::ai::ActorOps::react_to_threat_event(self_object_index, other_object_index, event_kind, magnitude, extra_param, suppress_vehicle_relay);
}

/**
 * Free-function entry point for halo::ai::ActorOps::reassign_vehicle_seat; forwards to the C++ implementation unchanged.
 * Register convention of the original: EBX -> vehicle_object_index, EDI -> self_object_index, stack -> seat_selector.
 *
 * @address 0x42b880
 */
int32_t actor_reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index, int32_t seat_selector)
{
    return halo::ai::ActorOps::reassign_vehicle_seat(vehicle_object_index, self_object_index, seat_selector);
}

/**
 * Free-function entry point for halo::ai::ActorView::recompute_grenade_eligibility; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x42f260
 */
void actor_recompute_grenade_eligibility(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).recompute_grenade_eligibility();
}

/**
 * Free-function entry point for halo::ai::ActorView::record_look_at_point; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> point, EDX -> priority, stack -> data.
 *
 * @address 0x421bc0
 */
void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data)
{
    halo::ai::ActorView(actor_index).record_look_at_point(point, priority, data);
}

/**
 * Free-function entry point for halo::ai::ActorView::record_perception_event; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDX -> event, ESI -> data.
 *
 * @address 0x422070
 */
void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data)
{
    halo::ai::ActorView(actor_index).record_perception_event(event, data);
}

/**
 * Free-function entry point for halo::ai::ActorView::refresh_combat_context; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (cdecl).
 *
 * @address 0x4297a0
 */
void actor_refresh_combat_context(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).refresh_combat_context();
}

/**
 * Free-function entry point for halo::ai::ActorView::reject_firing_position_by_perception; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, candidate.
 *
 * @address 0x4124c0
 */
uint8_t actor_reject_firing_position_by_perception(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::ActorView(actor_index).reject_firing_position_by_perception(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::reject_firing_position_by_pursuit; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, candidate.
 *
 * @address 0x412350
 */
uint8_t actor_reject_firing_position_by_pursuit(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::ActorView(actor_index).reject_firing_position_by_pursuit(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::reject_firing_position_by_request_result; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, candidate.
 *
 * @address 0x412620
 */
uint8_t actor_reject_firing_position_by_request_result(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::ActorView(actor_index).reject_firing_position_by_request_result(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::reject_firing_position_by_target_approach; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, candidate.
 *
 * @address 0x412570
 */
uint8_t actor_reject_firing_position_by_target_approach(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::ActorView(actor_index).reject_firing_position_by_target_approach(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::reject_firing_position_unreachable; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, candidate.
 *
 * @address 0x412290
 */
uint8_t actor_reject_firing_position_unreachable(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::ActorView(actor_index).reject_firing_position_unreachable(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::release_from_cluster_or_delete; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> unit_index.
 *
 * @address 0x428e50
 */
void actor_release_from_cluster_or_delete(datum_index actor_index, datum_index unit_index)
{
    halo::ai::ActorView(actor_index).release_from_cluster_or_delete(unit_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::remove_from_unit_cluster; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> actor_index, stack -> unit_index.
 *
 * @address 0x427c90
 */
void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index)
{
    halo::ai::ActorView(actor_index).remove_from_unit_cluster(unit_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::replace_object_reference; forwards to the C++ implementation unchanged.
 * Register convention of the original: ESI -> new_reference, EDI -> old_reference, stack -> actor_index.
 *
 * @address 0x428470
 */
void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference)
{
    halo::ai::ActorView(actor_index).replace_object_reference(new_reference, old_reference);
}

/**
 * Free-function entry point for halo::ai::ActorView::report_command_status; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x4048b0
 */
int32_t actor_report_command_status(uint32_t actor_index)
{
    return halo::ai::ActorView(actor_index).report_command_status();
}

/**
 * Free-function entry point for halo::ai::ActorView::report_firing_position_request; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> query, EBX -> candidate.
 *
 * @address 0x4120f0
 */
void actor_report_firing_position_request(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    halo::ai::ActorView(actor_index).report_firing_position_request(query, candidate);
}

/**
 * Free-function entry point for halo::ai::ActorView::request_move_and_face; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4049d0
 */
uint8_t actor_request_move_and_face(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).request_move_and_face();
}

/**
 * Free-function entry point for halo::ai::ActorView::request_path_with_grenade_arc; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x408300
 */
uint8_t actor_request_path_with_grenade_arc(uint32_t actor_index)
{
    return halo::ai::ActorView(actor_index).request_path_with_grenade_arc();
}

/**
 * Free-function entry point for halo::ai::ActorView::reseed_movement_pause_timer; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4104e0
 */
void actor_reseed_movement_pause_timer(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).reseed_movement_pause_timer();
}

/**
 * Free-function entry point for halo::ai::ActorOps::reset_perception_scratch; forwards to the C++ implementation unchanged.
 * Register convention of the original: ESI -> unit_index.
 *
 * @address 0x428f40
 */
void actor_reset_perception_scratch(datum_index unit_index)
{
    halo::ai::ActorOps::reset_perception_scratch(unit_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::reset_queued_look_vector; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x417ae0
 */
uint8_t actor_reset_queued_look_vector(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).reset_queued_look_vector();
}

/**
 * Free-function entry point for halo::ai::ActorView::reset_squad_link_for_type_change; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EBX -> encounter_index.
 *
 * @address 0x4290f0
 */
void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index, int16_t squad_index)
{
    halo::ai::ActorView(actor_index).reset_squad_link_for_type_change(encounter_index, squad_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::resolve_flee_source_point; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> reason, EDI -> out, stack -> actor_index.
 *
 * @address 0x4146c0
 */
uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out, datum_index actor_index)
{
    return halo::ai::ActorOps::resolve_flee_source_point(reason, out, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::resolve_look_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> preferred_direction, stack -> actor_index, stack -> deviation_table,.
 *
 * @address 0x414d00
 */
uint8_t actor_resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table, uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback)
{
    return halo::ai::ActorOps::resolve_look_target(preferred_direction, actor_index, deviation_table, require_trust, use_aiming_deviation, force_fallback);
}

/**
 * Free-function entry point for halo::ai::ActorView::resolve_wander_or_look_direction; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> out_direction.
 *
 * @address 0x4287a0
 */
uint8_t actor_resolve_wander_or_look_direction(datum_index actor_index, real_vector3d *out_direction)
{
    return halo::ai::ActorView(actor_index).resolve_wander_or_look_direction(out_direction);
}

/**
 * Free-function entry point for halo::ai::ActorView::run_mode_transition_loop; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x429ee0
 */
void actor_run_mode_transition_loop(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).run_mode_transition_loop();
}

/**
 * Free-function entry point for halo::ai::ActorView::scale_value_by_ally_exposure; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> value.
 *
 * @address 0x420c90
 */
uint8_t actor_scale_value_by_ally_exposure(datum_index actor_index, float *value)
{
    return halo::ai::ActorView(actor_index).scale_value_by_ally_exposure(value);
}

/**
 * Free-function entry point for halo::ai::ActorView::scan_allies_for_backup_request; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x420ec0
 */
void actor_scan_allies_for_backup_request(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).scan_allies_for_backup_request();
}

/**
 * Free-function entry point for halo::ai::TargetView::scan_ally_death_panic_reaction; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index, EBX -> actor_index.
 *
 * @address 0x4233d0
 */
void actor_scan_ally_death_panic_reaction(datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::TargetView(target_prop_index).scan_ally_death_panic_reaction(actor_index);
}

/**
 * Free-function entry point for halo::ai::TargetView::scan_backup_and_panic_reaction; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index, stack -> actor_index.
 *
 * @address 0x423220
 */
void actor_scan_backup_and_panic_reaction(datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::TargetView(target_prop_index).scan_backup_and_panic_reaction(actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::schedule_grenade_throw; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> actor_index.
 *
 * @address 0x402f80
 */
void actor_schedule_grenade_throw(uint32_t actor_index)
{
    halo::ai::ActorView(actor_index).schedule_grenade_throw();
}

/**
 * Free-function entry point for halo::ai::ActorView::score_blast_area_clear; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> blast_radius, safety_radius, point, out_count.
 *
 * @address 0x410da0
 */
uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count)
{
    return halo::ai::ActorView(actor_index).score_blast_area_clear(blast_radius, safety_radius, point, out_count);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_by_history; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, count, candidates.
 *
 * @address 0x411ee0
 */
void actor_score_firing_positions_by_history(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_by_history(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_by_range; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, count, candidates.
 *
 * @address 0x411bf0
 */
void actor_score_firing_positions_by_range(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_by_range(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_by_standoff; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, count, candidates.
 *
 * @address 0x411980
 */
void actor_score_firing_positions_by_standoff(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_by_standoff(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_by_threat; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index (in a float slot), query, count, candidates.
 *
 * @address 0x4112b0
 */
void actor_score_firing_positions_by_threat(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_by_threat(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_close_range; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, count, candidates.
 *
 * @address 0x411b60
 */
void actor_score_firing_positions_close_range(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_close_range(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::score_firing_positions_near_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, query, count, candidates.
 *
 * @address 0x411840
 */
void actor_score_firing_positions_near_target(datum_index actor_index, actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    halo::ai::ActorView(actor_index).score_firing_positions_near_target(query, count, candidates);
}

/**
 * Free-function entry point for halo::ai::ActorView::seek_vehicle_to_board; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40ac70
 */
uint8_t actor_seek_vehicle_to_board(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).seek_vehicle_to_board();
}

/**
 * Free-function entry point for halo::ai::ActorView::select_facing_target_prop; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> require_trust, stack -> skip_lane_test,.
 *
 * @address 0x414a90
 */
uint8_t actor_select_facing_target_prop(datum_index actor_index, uint8_t require_trust, uint8_t skip_lane_test, actor_recognition_scan_result *out_result, uint8_t *out_in_front)
{
    return halo::ai::ActorView(actor_index).select_facing_target_prop(require_trust, skip_lane_test, out_result, out_in_front);
}

/**
 * Free-function entry point for halo::ai::ActorView::select_firing_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX a, ECX b.
 *
 * @address 0x413e50
 */
int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok)
{
    return halo::ai::ActorView(actor_index).select_firing_position(query, out_candidate, out_previous_owner, path_context, out_path_ok);
}

/**
 * Free-function entry point for halo::ai::ActorView::select_move_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> select_mode, position_index, direction_flag.
 *
 * @address 0x4014c0
 */
int32_t actor_select_move_position(uint32_t actor_index, int16_t select_mode, int32_t position_index, uint8_t *direction_flag)
{
    return halo::ai::ActorView(actor_index).select_move_position(select_mode, position_index, direction_flag);
}

/**
 * Free-function entry point for halo::ai::ActorView::select_stance_offset_pair; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDX -> base, ESI -> out_b, EDI -> out_a.
 *
 * @address 0x4106b0
 */
void actor_select_stance_offset_pair(datum_index actor_index, ActorVariant *base, actor_burst_parameters **out_a, actor_burst_scale **out_b)
{
    halo::ai::ActorView(actor_index).select_stance_offset_pair(base, out_a, out_b);
}

/**
 * Free-function entry point for halo::ai::ActorView::set_combat_alert_flag; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EBX -> new_flag.
 *
 * @address 0x421a40
 */
void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag)
{
    halo::ai::ActorView(actor_index).set_combat_alert_flag(new_flag);
}

/**
 * Free-function entry point for halo::ai::ActorView::set_flag_bit1; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x42a5b0
 */
void actor_set_flag_bit1(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).set_flag_bit1();
}

/**
 * Free-function entry point for halo::ai::ActorView::set_mode; forwards to the C++ implementation unchanged.
 *
 * @address 0x40d8d0
 */
void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data)
{
    halo::ai::ActorView(actor_index).set_mode(mode, mode_data);
}

/**
 * Free-function entry point for halo::ai::ActorView::set_override_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> enable, override_target.
 *
 * @address 0x42a5e0
 */
void actor_set_override_target(datum_index actor_index, uint8_t enable, datum_index override_target)
{
    halo::ai::ActorView(actor_index).set_override_target(enable, override_target);
}

/**
 * Free-function entry point for halo::ai::TargetView::set_target_alert_stage1; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> target_prop_index, ESI -> actor_index.
 *
 * @address 0x41fb00
 */
void actor_set_target_alert_stage1(datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::TargetView(target_prop_index).set_target_alert_stage1(actor_index);
}

/**
 * Free-function entry point for halo::ai::TargetView::set_target_alert_stage2; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> target_prop_index, ESI -> actor_index.
 *
 * @address 0x41fb60
 */
void actor_set_target_alert_stage2(datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::TargetView(target_prop_index).set_target_alert_stage2(actor_index);
}

/**
 * Free-function entry point for halo::ai::TargetView::set_target_alert_stage3; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDX -> target_prop_index, ESI -> actor_index.
 *
 * @address 0x41fbc0
 */
void actor_set_target_alert_stage3(datum_index target_prop_index, datum_index actor_index)
{
    halo::ai::TargetView(target_prop_index).set_target_alert_stage3(actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::set_units_active; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EBX -> activate.
 *
 * @address 0x427860
 */
void actor_set_units_active(datum_index actor_index, uint8_t dormant)
{
    halo::ai::ActorView(actor_index).set_units_active(dormant);
}

/**
 * Free-function entry point for halo::ai::ActorView::should_hold_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDX -> definition.
 *
 * @address 0x4105c0
 */
uint8_t actor_should_hold_position(datum_index actor_index, const ActorVariant *definition)
{
    return halo::ai::ActorView(actor_index).should_hold_position(definition);
}

/**
 * Free-function entry point for halo::ai::ActorView::should_throw_grenade; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> force.
 *
 * @address 0x40b840
 */
uint8_t actor_should_throw_grenade(uint32_t actor_index, char force)
{
    return halo::ai::ActorView(actor_index).should_throw_grenade(force);
}

/**
 * Free-function entry point for halo::ai::ActorView::snapshot_orientation; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x4294d0
 */
void actor_snapshot_orientation(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).snapshot_orientation();
}

/**
 * Free-function entry point for halo::ai::ActorView::solve_grenade_lob; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX target, EAX speed_in, the rest on the stack.
 *
 * @address 0x410780
 */
uint32_t actor_solve_grenade_lob(datum_index actor_index, real_point3d *point)
{
    return halo::ai::ActorView(actor_index).solve_grenade_lob(point);
}

/**
 * Free-function entry point for halo::ai::ActorOps::spawn_additional_units; forwards to the C++ implementation unchanged.
 * Register convention of the original: EBX -> actor_variant_tag, EDX -> spawn_count, stack -> source_actor_index,.
 *
 * @address 0x427280
 */
int16_t actor_spawn_additional_units(datum_index actor_variant_tag, int16_t spawn_count, datum_index source_actor_index, float health_scale)
{
    return halo::ai::ActorOps::spawn_additional_units(actor_variant_tag, spawn_count, source_actor_index, health_scale);
}

/**
 * Free-function entry point for halo::ai::ActorOps::squad_action_execute; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> aim_state, stack -> (actor_index, check_object_index, command_list_index, state).
 *
 * @address 0x405520
 */
char actor_squad_action_execute(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state)
{
    return halo::ai::ActorOps::squad_action_execute(aim_state, actor_index, check_object_index, command_list_index, state);
}

/**
 * Free-function entry point for halo::ai::ActorOps::squad_action_is_complete; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> aim_state, ECX -> check_object_index, stack -> (actor_index, command_list_index, state).
 *
 * @address 0x4066d0
 */
uint8_t actor_squad_action_is_complete(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state)
{
    return halo::ai::ActorOps::squad_action_is_complete(aim_state, actor_index, check_object_index, command_list_index, state);
}

/**
 * Free-function entry point for halo::ai::ActorView::squad_action_list_process; forwards to the C++ implementation unchanged.
 *
 * @address 0x406e30
 */
void actor_squad_action_list_process(uint32_t actor_index, uint32_t check_object_index, uint16_t command_list_index, actor_squad_action_state *state, actor_command_aim *aim_state, uint32_t callback_extra)
{
    halo::ai::ActorView(actor_index).squad_action_list_process(check_object_index, command_list_index, state, aim_state, reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(callback_extra)));
}

/**
 * Free-function entry point for halo::ai::ActorView::squad_action_reset_entry; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> check_object_index, EBX -> state,.
 *
 * @address 0x406c50
 */
void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, actor_squad_action_state *state, int16_t command_list_index, actor_command_aim *aim_state, uint8_t *next_action_index_out)
{
    halo::ai::ActorView(actor_index).squad_action_reset_entry(check_object_index, state, command_list_index, aim_state, next_action_index_out);
}

/**
 * Free-function entry point for halo::ai::ActorView::squad_action_status_broadcast; forwards to the C++ implementation unchanged.
 * Register convention of the original: ESI -> record, stack -> actor_index, command_list_index.
 *
 * @address 0x407140
 */
int32_t actor_squad_action_status_broadcast(uint32_t actor_index, int16_t command_list_index, actor_mode_obey_data *record)
{
    return halo::ai::ActorView(actor_index).squad_action_status_broadcast(command_list_index, record);
}

/**
 * Free-function entry point for halo::ai::ActorView::squad_react_to_grenade; forwards to the C++ implementation unchanged.
 * Register convention of the original: ESI -> actor_index, stack -> target_prop_index, EAX -> grenade_type.
 *
 * @address 0x42a3a0
 */
void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index, int16_t grenade_type)
{
    halo::ai::ActorView(actor_index).squad_react_to_grenade(target_prop_index, grenade_type);
}

/**
 * Free-function entry point for halo::ai::ActorOps::squad_react_to_grenade_for_vehicle_occupants; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> vehicle_object_index, EBX -> other_object_index.
 *
 * @address 0x42bd70
 */
void actor_squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index, datum_index other_object_index)
{
    halo::ai::ActorOps::squad_react_to_grenade_for_vehicle_occupants(vehicle_object_index, other_object_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::start_search_timer; forwards to the C++ implementation unchanged.
 * Register convention of the original: EBX -> actor_index, EDI -> prop_index.
 *
 * @address 0x422130
 */
void actor_start_search_timer(datum_index actor_index, datum_index prop_index)
{
    halo::ai::ActorView(actor_index).start_search_timer(prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::swarm_for_each_component; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> reset_first/callback/callback_extra,.
 *
 * @address 0x407040
 */
void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, actor_mode_obey_data *obey)
{
    halo::ai::ActorView(actor_index).swarm_for_each_component(reset_first, callback, callback_extra, obey);
}

/**
 * Free-function entry point for halo::ai::ActorView::swarm_for_each_component_thunk; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x407240
 */
void actor_swarm_for_each_component_thunk(uint32_t actor_index)
{
    halo::ai::ActorView(actor_index).swarm_for_each_component_thunk();
}

/**
 * Free-function entry point for halo::ai::ActorOps::take_danger_escape; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> path_delta, stack -> (actor_index, escape_direction, step_distance bits, step_up).
 *
 * @address 0x40e060
 */
uint8_t actor_take_danger_escape(real_vector3d *path_delta, datum_index actor_index, uint16_t direction_kind, float step_distance, float distance)
{
    return halo::ai::ActorOps::take_danger_escape(path_delta, actor_index, direction_kind, step_distance, distance);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_data_acquire; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, unused_param, owner_reference, pair_reference.
 *
 * @address 0x41f7d0
 */
uint8_t actor_target_data_acquire(datum_index actor_index, datum_index object_index, datum_index owner_reference, datum_index pair_reference)
{
    return halo::ai::ActorView(actor_index).target_data_acquire(object_index, owner_reference, pair_reference);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_data_refresh; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, target_prop_index, reference, force, allow_reassign.
 *
 * @address 0x41c4b0
 */
void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, actor_firing_positions *reference, char force, char allow_reassign)
{
    halo::ai::ActorView(actor_index).target_data_refresh(target_prop_index, reference, force, allow_reassign);
}

/**
 * Free-function entry point for halo::ai::TargetView::target_data_release; forwards to the C++ implementation unchanged.
 * Register convention of the original: EBX -> target_prop_index, stack -> actor_index, out_conflict_flag.
 *
 * @address 0x41b980
 */
uint32_t actor_target_data_release(datum_index target_prop_index, uint32_t actor_index, uint8_t *out_conflict_flag)
{
    return halo::ai::TargetView(target_prop_index).target_data_release(actor_index, out_conflict_flag);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_evaluate_squad_link; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, object_index, candidates_a, candidates_b.
 *
 * @address 0x41e320
 */
void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_index, ai_target_candidate_list *candidates_a, ai_target_candidate_list *candidates_b)
{
    halo::ai::ActorView(actor_index).target_evaluate_squad_link(object_index, candidates_a, candidates_b);
}

/**
 * Free-function entry point for halo::ai::TargetView::target_get_backup_priority; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index.
 *
 * @address 0x420e50
 */
uint8_t actor_target_get_backup_priority(datum_index target_prop_index)
{
    return halo::ai::TargetView(target_prop_index).target_get_backup_priority();
}

/**
 * Free-function entry point for halo::ai::ActorView::target_get_priority_class; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> target_prop_index.
 *
 * @address 0x41be10
 */
uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index)
{
    return halo::ai::ActorView(actor_index).target_get_priority_class(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::TargetView::target_get_relationship_object; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index.
 *
 * @address 0x41f3a0
 */
void actor_target_get_relationship_object(datum_index target_prop_index)
{
    halo::ai::TargetView(target_prop_index).target_get_relationship_object();
}

/**
 * Free-function entry point for halo::ai::ActorView::target_has_conflicting_neighbor; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> target_prop_index.
 *
 * @address 0x41f410
 */
uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index)
{
    return halo::ai::ActorView(actor_index).target_has_conflicting_neighbor(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorOps::target_hearing_check; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, ECX -> target_ref, EBX -> gate, ESI -> listener_position,.
 *
 * @address 0x41c030
 */
uint16_t actor_target_hearing_check(const bsp_leaf_reference *record, int16_t stance, datum_index actor_index, const actor_firing_positions *target_ref, int16_t gate, real_point3d *listener_position)
{
    return halo::ai::ActorOps::target_hearing_check(record, stance, actor_index, target_ref, gate, listener_position);
}

/**
 * Free-function entry point for halo::ai::ActorOps::target_is_close_and_recognized; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> object_index, the unused second argument, actor_index.
 *
 * @address 0x42f480
 */
uint8_t actor_target_is_close_and_recognized(datum_index object_index, uint32_t unused, datum_index actor_index)
{
    return halo::ai::ActorOps::target_is_close_and_recognized(object_index, unused, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_is_visible_or_object_count_ok; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> kind.
 *
 * @address 0x40f700
 */
uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind)
{
    return halo::ai::ActorView(actor_index).target_is_visible_or_object_count_ok(kind);
}

/**
 * Free-function entry point for halo::ai::TargetView::target_mark_engaged; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> target_prop_index, EBX -> actor_index, stack -> mark_engaged.
 *
 * @address 0x41fa80
 */
void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged)
{
    halo::ai::TargetView(target_prop_index).target_mark_engaged(actor_index, mark_engaged);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_relationship_think; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x41abd0
 */
void actor_target_relationship_think(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).target_relationship_think();
}

/**
 * Free-function entry point for halo::ai::TargetView::target_reset_combat_flags; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> target_prop_index, stack -> actor_index, unused, already_noticed.
 *
 * @address 0x41baf0
 */
void actor_target_reset_combat_flags(datum_index target_prop_index, datum_index actor_index, uint32_t unused, uint8_t already_noticed)
{
    halo::ai::TargetView(target_prop_index).target_reset_combat_flags(actor_index, unused, already_noticed);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_reset_seen_flags; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x41f9d0
 */
void actor_target_reset_seen_flags(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).target_reset_seen_flags();
}

/**
 * Free-function entry point for halo::ai::ActorView::target_reset_shot_counters; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x41fa20
 */
void actor_target_reset_shot_counters(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).target_reset_shot_counters();
}

/**
 * Free-function entry point for halo::ai::ActorView::target_scan_potential_targets; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x41d7e0
 */
void actor_target_scan_potential_targets(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).target_scan_potential_targets();
}

/**
 * Free-function entry point for halo::ai::ActorView::target_update_active_flag; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDI -> target_prop_index.
 *
 * @address 0x41fc60
 */
uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index)
{
    return halo::ai::ActorView(actor_index).target_update_active_flag(target_prop_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::target_update_tracking_speed; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index, target_prop_index, scratch.
 *
 * @address 0x41c8f0
 */
void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index, actor_firing_positions *scratch)
{
    halo::ai::ActorView(actor_index).target_update_tracking_speed(target_prop_index, scratch);
}

/**
 * Free-function entry point for halo::ai::ActorOps::targets_share_descriptor; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_a, ECX -> actor_b.
 *
 * @address 0x40e380
 */
uint8_t actor_targets_share_descriptor(datum_index actor_a, datum_index actor_b)
{
    return halo::ai::ActorOps::targets_share_descriptor(actor_a, actor_b);
}

/**
 * Free-function entry point for halo::ai::ActorOps::toggle_active_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> activate, EDI -> actor_index.
 *
 * @address 0x4277c0
 */
uint8_t actor_toggle_active_state(uint8_t activate, datum_index actor_index)
{
    return halo::ai::ActorOps::toggle_active_state(activate, actor_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::try_grenade_evasion; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> allow_pain_reaction, use_alt_base.
 *
 * @address 0x40c530
 */
uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base)
{
    return halo::ai::ActorView(actor_index).try_grenade_evasion(allow_pain_reaction, use_alt_base);
}

/**
 * Free-function entry point for halo::ai::ActorView::type_crew_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x423890
 */
void actor_type_crew_update(uint32_t actor_index)
{
    halo::ai::ActorView(actor_index).type_crew_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_elite_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x423a90
 */
void actor_type_elite_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_elite_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_engineer_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x423d40
 */
void actor_type_engineer_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_engineer_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_flood_carrier_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x423740
 */
void actor_type_flood_carrier_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_flood_carrier_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_flood_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x423f30
 */
void actor_type_flood_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_flood_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_grunt_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x424590
 */
void actor_type_grunt_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_grunt_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_hunter_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x424810
 */
void actor_type_hunter_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_hunter_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_infection_swarm_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x424c20
 */
void actor_type_infection_swarm_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_infection_swarm_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_infection_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x424980
 */
void actor_type_infection_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_infection_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_jackal_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x425f70
 */
void actor_type_jackal_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_jackal_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_marine_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4261d0
 */
void actor_type_marine_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_marine_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_mounted_weapon_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4263f0
 */
void actor_type_mounted_weapon_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_mounted_weapon_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::type_sentinel_update; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4264d0
 */
void actor_type_sentinel_update(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).type_sentinel_update();
}

/**
 * Free-function entry point for halo::ai::ActorView::unlink_prop; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDI -> prop_to_remove.
 *
 * @address 0x43ea20
 */
void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove)
{
    halo::ai::ActorView(actor_index).unlink_prop(prop_to_remove);
}

/**
 * Free-function entry point for halo::ai::ActorView::unlink_unit; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x427bc0
 */
void actor_unlink_unit(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).unlink_unit();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_activation_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: ESI -> actor_index.
 *
 * @address 0x429160
 */
void actor_update_activation_state(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_activation_state();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_aim_wander; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40fcb0
 */
void actor_update_aim_wander(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_aim_wander();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_awareness_level; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x420290
 */
void actor_update_awareness_level(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_awareness_level();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_combat_behavior; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDI -> actor_index, stack -> param_1, param_2.
 *
 * @address 0x40d610
 */
uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2)
{
    return halo::ai::ActorView(actor_index).update_combat_behavior(param_1, param_2);
}

/**
 * Free-function entry point for halo::ai::ActorView::update_crouch_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4213b0
 */
void actor_update_crouch_state(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_crouch_state();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_danger_avoidance; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40c040
 */
uint8_t actor_update_danger_avoidance(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_danger_avoidance();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_facing_change_timer; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x423670
 */
void actor_update_facing_change_timer(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_facing_change_timer();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_firing_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40e7b0
 */
void actor_update_firing_state(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_firing_state();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_flee_response; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDX -> actor_index.
 *
 * @address 0x414250
 */
uint8_t actor_update_flee_response(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_flee_response();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_grenade_and_morale_reactions; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x40b920
 */
char actor_update_grenade_and_morale_reactions(uint32_t actor_index)
{
    return halo::ai::ActorView(actor_index).update_grenade_and_morale_reactions();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_grenade_eligibility_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x42f370
 */
void actor_update_grenade_eligibility_state(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_grenade_eligibility_state();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_grenade_throw_decision; forwards to the C++ implementation unchanged.
 * Register convention of the original: EDI -> actor_index.
 *
 * @address 0x40b770
 */
uint8_t actor_update_grenade_throw_decision(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_grenade_throw_decision();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_idle_stagger; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x429430
 */
void actor_update_idle_stagger(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_idle_stagger();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_look_target; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x415480
 */
void actor_update_look_target(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_look_target();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_melee_combat_action; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x40cdf0
 */
uint8_t actor_update_melee_combat_action(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_melee_combat_action();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_movement_destination; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x403180
 */
uint8_t actor_update_movement_destination(uint32_t actor_index)
{
    return halo::ai::ActorView(actor_index).update_movement_destination();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_path_if_needed; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x4017b0
 */
uint8_t actor_update_path_if_needed(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_path_if_needed();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_special_mode; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x40d820
 */
uint8_t actor_update_special_mode(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_special_mode();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_squad_link_state; forwards to the C++ implementation unchanged.
 * Register convention of the original: stack -> actor_index.
 *
 * @address 0x429270
 */
uint8_t actor_update_squad_link_state(datum_index actor_index)
{
    return halo::ai::ActorView(actor_index).update_squad_link_state();
}

/**
 * Free-function entry point for halo::ai::ActorOps::update_swarm_component_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> component_index, ECX -> unit_index.
 *
 * @address 0x428130
 */
void actor_update_swarm_component_position(datum_index component_index, datum_index unit_index)
{
    halo::ai::ActorOps::update_swarm_component_position(component_index, unit_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::update_target_combat_status; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x4200d0
 */
void actor_update_target_combat_status(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_target_combat_status();
}

/**
 * Free-function entry point for halo::ai::ActorView::update_target_lead_position; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x429570
 */
void actor_update_target_lead_position(datum_index actor_index)
{
    halo::ai::ActorView(actor_index).update_target_lead_position();
}

/**
 * Free-function entry point for halo::ai::ActorOps::validate_grenade_ally_candidate; forwards to the C++ implementation unchanged.
 * Register convention of the original: ECX -> candidate_actor, BL -> caller_type_flag.
 *
 * @address 0x40e4a0
 */
uint8_t actor_validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag)
{
    return halo::ai::ActorOps::validate_grenade_ally_candidate(candidate_actor, caller_type_flag);
}

/**
 * Free-function entry point for halo::ai::ActorView::validate_grenade_impact_point; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, EDI -> candidate_point.
 *
 * @address 0x410710
 */
uint8_t actor_validate_grenade_impact_point(datum_index actor_index, real_point3d *candidate_point)
{
    return halo::ai::ActorView(actor_index).validate_grenade_impact_point(candidate_point);
}

/**
 * Free-function entry point for halo::ai::ActorView::vehicle_not_recently_left; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index, stack -> vehicle_index.
 *
 * @address 0x40ac30
 */
uint8_t actor_vehicle_not_recently_left(datum_index actor_index, datum_index vehicle_index)
{
    return halo::ai::ActorView(actor_index).vehicle_not_recently_left(vehicle_index);
}

/**
 * Free-function entry point for halo::ai::ActorView::wants_reload_or_swap; forwards to the C++ implementation unchanged.
 * Register convention of the original: EAX -> actor_index.
 *
 * @address 0x40ab80
 */
uint8_t actor_wants_reload_or_swap(uint32_t actor_index)
{
    return halo::ai::ActorView(actor_index).wants_reload_or_swap();
}

}
