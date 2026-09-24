#include "tags.h"
#include "memory.h"
#include "camera.h"

// ucrt64 gcc has 64-bit pointers: director (pov_proc) and observer (command) hold one each, so
// their checks are gated on PTRS32 and only fire with -m32 (same gate as sound_smoke.c).
// Run both:  gcc -fsyntax-only -I types out/phase4/camera_smoke.c
//            gcc -m32 -fsyntax-only -I types out/phase4/camera_smoke.c
#define PTRS32 (sizeof(void *) == 4)

typedef char check_camera_script_globals[(sizeof(camera_script_globals) == 0x40) ? 1 : -1];
typedef char check_camera_input_axis_definition[(sizeof(camera_input_axis_definition) == 0x1c) ? 1 : -1];
typedef char check_camera_input_axis_state[(sizeof(camera_input_axis_state) == 0x0c) ? 1 : -1];
typedef char check_camera_input[(sizeof(camera_input) == 0x24) ? 1 : -1];
typedef char check_observer_parameters[(sizeof(observer_parameters) == 0x38) ? 1 : -1];
typedef char check_observer_command[(sizeof(observer_command) == 0x68) ? 1 : -1];
typedef char check_observer_parameter_derivatives[(sizeof(observer_parameter_derivatives) == 0x2c) ? 1 : -1];
typedef char check_observer_camera[(sizeof(observer_camera) == 0x3c) ? 1 : -1];
typedef char check_observer[(!PTRS32 || (sizeof(observer) == 0x29c)) ? 1 : -1];
typedef char check_first_person_camera_data[(sizeof(first_person_camera_data) == 0x04) ? 1 : -1];
typedef char check_third_person_camera_data[(sizeof(third_person_camera_data) == 0x1c) ? 1 : -1];
typedef char check_dead_camera_data[(sizeof(dead_camera_data) == 0x30) ? 1 : -1];
typedef char check_editor_camera_data[(sizeof(editor_camera_data) == 0x1c) ? 1 : -1];
typedef char check_orbiting_camera_data[(sizeof(orbiting_camera_data) == 0x1c) ? 1 : -1];
typedef char check_director_camera_data[(sizeof(director_camera_data) == 0x40) ? 1 : -1];
typedef char check_director[(!PTRS32 || (sizeof(director) == 0xf8)) ? 1 : -1];
typedef char check_director_globals[(sizeof(director_globals) == 0x08) ? 1 : -1];
typedef char check_flying_camera_home[(sizeof(flying_camera_home) == 0x14) ? 1 : -1];

/* offsets pinned by absolute addresses: director base 0x006ac560, observer base 0x006ac65c,
   camera_script_globals 0x006869d0, camera_input_axes 0x00686a28 */
#define OFF(t, f) __builtin_offsetof(t, f)
typedef char c_dir_transition[(!PTRS32 || (OFF(director, transition_time) == 0x6ac564 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_pov[(!PTRS32 || (OFF(director, pov_proc) == 0x6ac568 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_data[(!PTRS32 || (OFF(director, data) == 0x6ac56c - 0x6ac560)) ? 1 : -1];
typedef char c_dir_4c[(!PTRS32 || (OFF(director, unknown_4c) == 0x6ac5ac - 0x6ac560)) ? 1 : -1];
typedef char c_dir_50[(!PTRS32 || (OFF(director, unknown_50) == 0x6ac5b0 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_suppress[(!PTRS32 || (OFF(director, suppress_look_update) == 0x6ac5b1 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_consumed[(!PTRS32 || (OFF(director, look_input_consumed) == 0x6ac5b2 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_seat[(!PTRS32 || (OFF(director, seat_camera_state) == 0x6ac5b4 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_type[(!PTRS32 || (OFF(director, camera_type) == 0x6ac5b6 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_command[(!PTRS32 || (OFF(director, command) == 0x6ac5b8 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_cmd_timer[(!PTRS32 || (OFF(director, command.timer) == 0x6ac600 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_cmd_times[(!PTRS32 || (OFF(director, command.channel_times) == 0x6ac60c - 0x6ac560)) ? 1 : -1];
typedef char c_dir_cmd_time4[(!PTRS32 || (OFF(director, command.channel_times) + 16 == 0x6ac61c - 0x6ac560)) ? 1 : -1];
typedef char c_dir_c0[(!PTRS32 || (OFF(director, unknown_c0) == 0x6ac620 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_scale[(!PTRS32 || (OFF(director, look_scale) == 0x6ac624 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_axes[(!PTRS32 || (OFF(director, axes) == 0x6ac628 - 0x6ac560)) ? 1 : -1];
typedef char c_dir_axis3_delta[(!PTRS32 || (OFF(director, axes) + 3 * 12 + 8 == 0x6ac654 - 0x6ac560)) ? 1 : -1];
typedef char c_tp_unit[(!PTRS32 || (OFF(director, data.third_person.unit) == 0x6ac574 - 0x6ac560)) ? 1 : -1];
typedef char c_tp_seat[(!PTRS32 || (OFF(director, data.third_person.seat_index) == 0x6ac578 - 0x6ac560)) ? 1 : -1];
typedef char c_tp_yaw[(!PTRS32 || (OFF(director, data.third_person.yaw_offset) == 0x6ac57c - 0x6ac560)) ? 1 : -1];
typedef char c_tp_scale[(!PTRS32 || (OFF(director, data.third_person.distance_scale) == 0x6ac584 - 0x6ac560)) ? 1 : -1];
typedef char c_ed_roll[(!PTRS32 || (OFF(director, data.editor.roll) == 0x6ac580 - 0x6ac560)) ? 1 : -1];
typedef char c_ed_fov[(!PTRS32 || (OFF(director, data.editor.field_of_view) == 0x6ac584 - 0x6ac560)) ? 1 : -1];
typedef char c_dead_retarget[(OFF(dead_camera_data, retarget_time) == 0x0b * 4) ? 1 : -1];
typedef char c_obs_cmdptr[(!PTRS32 || (OFF(observer, command) == 0x6ac660 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cmd[(!PTRS32 || (OFF(observer, current_command) == 0x6ac664 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_target[(!PTRS32 || (OFF(observer, current_command.parameters) == 0x6ac668 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_vel_in_cmd[(!PTRS32 || (OFF(observer, current_command.velocity) == 0x6ac6a0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_ch_flags[(!PTRS32 || (OFF(observer, current_command.interpolation_flags) == 0x6ac6b0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_ch_times[(!PTRS32 || (OFF(observer, current_command.channel_times) == 0x6ac6b8 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_updated[(!PTRS32 || (OFF(observer, updated) == 0x6ac6cc - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_hascmd[(!PTRS32 || (OFF(observer, has_command) == 0x6ac6cd - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_camera[(!PTRS32 || (OFF(observer, camera) == 0x6ac6d0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_leaf[(!PTRS32 || (OFF(observer, camera.leaf_index) == 0x6ac6dc - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cluster[(!PTRS32 || (OFF(observer, camera.cluster_index) == 0x6ac6e0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cvel[(!PTRS32 || (OFF(observer, camera.velocity) == 0x6ac6e4 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cfwd[(!PTRS32 || (OFF(observer, camera.forward) == 0x6ac6f0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cup[(!PTRS32 || (OFF(observer, camera.up) == 0x6ac6fc - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_cfov[(!PTRS32 || (OFF(observer, camera.field_of_view) == 0x6ac708 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_params[(!PTRS32 || (OFF(observer, parameters) == 0x6ac70c - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_off[(!PTRS32 || (OFF(observer, parameters.focus_offset) == 0x6ac718 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_dist[(!PTRS32 || (OFF(observer, parameters.distance) == 0x6ac724 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_fov[(!PTRS32 || (OFF(observer, parameters.field_of_view) == 0x6ac728 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_fwd[(!PTRS32 || (OFF(observer, parameters.forward) == 0x6ac72c - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_up[(!PTRS32 || (OFF(observer, parameters.up) == 0x6ac738 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_vel[(!PTRS32 || (OFF(observer, velocity) == 0x6ac744 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_acc[(!PTRS32 || (OFF(observer, acceleration) == 0x6ac77c - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t5[(!PTRS32 || (OFF(observer, coefficient_t5) == 0x6ac7b4 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t4[(!PTRS32 || (OFF(observer, coefficient_t4) == 0x6ac7e0 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t3[(!PTRS32 || (OFF(observer, coefficient_t3) == 0x6ac80c - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t2[(!PTRS32 || (OFF(observer, coefficient_t2) == 0x6ac838 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t1[(!PTRS32 || (OFF(observer, coefficient_t1) == 0x6ac864 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_t0[(!PTRS32 || (OFF(observer, coefficient_t0) == 0x6ac890 - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_rem[(!PTRS32 || (OFF(observer, remaining_offset) == 0x6ac8bc - 0x6ac65c)) ? 1 : -1];
typedef char c_obs_trailer[(!PTRS32 || (OFF(observer, trailer_signature) == 0x298)) ? 1 : -1];
typedef char c_script_time[(OFF(camera_script_globals, time_remaining) == 0x6869d8 - 0x6869d0) ? 1 : -1];
typedef char c_script_pos[(OFF(camera_script_globals, position) == 0x6869dc - 0x6869d0) ? 1 : -1];
typedef char c_script_fwd[(OFF(camera_script_globals, forward) == 0x6869e8 - 0x6869d0) ? 1 : -1];
typedef char c_script_up[(OFF(camera_script_globals, up) == 0x6869f4 - 0x6869d0) ? 1 : -1];
typedef char c_script_fov[(OFF(camera_script_globals, field_of_view) == 0x686a00 - 0x6869d0) ? 1 : -1];
typedef char c_script_obj[(OFF(camera_script_globals, object) == 0x686a04 - 0x6869d0) ? 1 : -1];
typedef char c_script_tag[(OFF(camera_script_globals, animation_tag) == 0x686a08 - 0x6869d0) ? 1 : -1];
typedef char c_script_anim[(OFF(camera_script_globals, animation_index) == 0x686a0c - 0x6869d0) ? 1 : -1];
typedef char c_axis_reset[(OFF(camera_input_axis_definition, reset_value) == 0x686a34 - 0x686a28) ? 1 : -1];
typedef char c_axis_flag[(OFF(camera_input_axis_definition, scale_by_zoom) == 0x686a40 - 0x686a28) ? 1 : -1];
/* tag layouts the module relies on (types/tags.h) */
typedef char c_scn_cut[(OFF(Scenario, cutscene_camera_points) + 4 == 0x4f4) ? 1 : -1];
typedef char c_scn_psl[(OFF(Scenario, player_starting_locations) == 0x354) ? 1 : -1];
typedef char c_ccp_size[(sizeof(ScenarioCutsceneCameraPoint) == 0x68) ? 1 : -1];
typedef char c_ccp_pos[(OFF(ScenarioCutsceneCameraPoint, position) == 0x28) ? 1 : -1];
typedef char c_ccp_fov[(OFF(ScenarioCutsceneCameraPoint, field_of_view) == 0x40) ? 1 : -1];
typedef char c_unit_cam[(OFF(Unit, camera_marker_name) == 0x1a8) ? 1 : -1];
typedef char c_unit_tracks[(OFF(Unit, camera_tracks) == 0x1a8 + 0x4c) ? 1 : -1];
typedef char c_seat_cam[(OFF(UnitSeat, camera_marker_name) == 0x84) ? 1 : -1];
typedef char c_seat_tracks[(OFF(UnitSeat, camera_tracks) == 0x84 + 0x4c) ? 1 : -1];
typedef char check_unit_camera_properties[(sizeof(unit_camera_properties) == 0x58) ? 1 : -1];
typedef char c_ucp_tracks[(OFF(unit_camera_properties, camera_tracks) == 0x4c) ? 1 : -1];
typedef char c_track_cp[(sizeof(CameraTrackControlPoint) == 0x3c) ? 1 : -1];

int camera_smoke(void) { return (int)(sizeof(director) + sizeof(observer)); }
