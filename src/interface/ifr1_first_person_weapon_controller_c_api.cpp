#include "halo/interface/ifr1_first_person_weapon_controller.hpp"

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::center_flashlight.
 *
 * @address 0x492b80
 */
extern "C" void first_person_weapon_center_flashlight(datum_index unit_index, real_point3d *out_origin, real_vector3d *out_extents, real_vector3d *out_direction)
{
    halo::interface::FirstPersonWeaponController::center_flashlight(unit_index, out_origin, out_extents, out_direction);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::get_marker_data.
 *
 * @address 0x492ad0
 */
extern "C" uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name, object_marker *out, uint32_t maximum)
{
    return halo::interface::FirstPersonWeaponController::get_marker_data(weapon_index, marker_name, out, maximum);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::interface_initialize.
 * blam-cc: stack -> local_player_index
 *
 * @address 0x493c60
 */
extern "C" void first_person_weapon_interface_initialize(int16_t local_player_index)
{
    halo::interface::FirstPersonWeaponController(local_player_index).interface_initialize();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::interface_tick.
 *
 * @address 0x4923d0
 */
extern "C" void first_person_weapon_interface_tick(void)
{
    halo::interface::FirstPersonWeaponController::interface_tick();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::interface_tick_reset.
 * blam-cc: AX -> local_player_index
 *
 * @address 0x4942e0
 */
extern "C" void first_person_weapon_interface_tick_reset(int16_t local_player_index)
{
    halo::interface::FirstPersonWeaponController(local_player_index).interface_tick_reset();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::process_action.
 * blam-cc: stack -> local_player_index, action_code
 *
 * @address 0x4940f0
 */
extern "C" void first_person_weapon_process_action(int16_t local_player_index, int16_t action_code)
{
    halo::interface::FirstPersonWeaponController(local_player_index).process_action(action_code);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::set_attached.
 *
 * @address 0x493e50
 */
extern "C" void first_person_weapon_set_attached(int16_t local_player_index, uint8_t attached)
{
    halo::interface::FirstPersonWeaponController(local_player_index).set_attached(attached);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::set_state.
 * blam-cc: AX -> new_state, stack -> local_player_index, force_pose_snapshot
 *
 * @address 0x492e60
 */
extern "C" void first_person_weapon_set_state(int16_t local_player_index, uint8_t force_pose_snapshot, int16_t new_state)
{
    halo::interface::FirstPersonWeaponController(local_player_index).set_state(force_pose_snapshot, new_state);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::snapshot_pose.
 *
 * @address 0x4930b0
 */
extern "C" void first_person_weapon_snapshot_pose(int16_t local_player_index, int16_t blend_gap)
{
    halo::interface::FirstPersonWeaponController(local_player_index).snapshot_pose(blend_gap);
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update.
 *
 * @address 0x493150
 */
extern "C" void first_person_weapon_update(int16_t local_player_index)
{
    halo::interface::FirstPersonWeaponController(local_player_index).update();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_active_state.
 *
 * @address 0x492430
 */
extern "C" void first_person_weapon_update_active_state(void)
{
    halo::interface::FirstPersonWeaponController::update_active_state();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_animation_controls.
 *
 * @address 0x493740
 */
extern "C" void first_person_weapon_update_animation_controls(int16_t local_player_index)
{
    halo::interface::FirstPersonWeaponController(local_player_index).update_animation_controls();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_lighting.
 * blam-cc: EAX -> model_tag_ref, ECX -> node_records, 11 stack arguments (objdump 0x492636..0x49266a)
 * blam-cc: none
 *
 * @address 0x4924b0
 */
extern "C" void first_person_weapon_update_lighting(void)
{
    halo::interface::FirstPersonWeaponController::update_lighting();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_screen_effects.
 *
 * @address 0x494730
 */
extern "C" void first_person_weapon_update_screen_effects(void)
{
    halo::interface::FirstPersonWeaponController::update_screen_effects();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_state.
 *
 * @address 0x492d20
 */
extern "C" void first_person_weapon_update_state(int16_t local_player_index)
{
    halo::interface::FirstPersonWeaponController(local_player_index).update_state();
}

/**
 * C ABI entry point; forwards to halo::interface::FirstPersonWeaponController::update_zoom_static_tint.
 * blam-cc: AL -> enabled
 *
 * @address 0x494af0
 */
extern "C" void first_person_weapon_update_zoom_static_tint(uint8_t enabled)
{
    halo::interface::FirstPersonWeaponController::update_zoom_static_tint(enabled);
}
