#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// The header targets a 32-bit image, so every size below is stated for 4-byte
// pointers and corrected by the host pointer width times the number of pointers
// the struct contains.
#define PW ((int)sizeof(void *) - 4)

typedef char check_ui_key_event[(sizeof(ui_key_event) == 0x04) ? 1 : -1];
typedef char check_text_edit_state[(sizeof(text_edit_state) == 0x0a + 1*PW) ? 1 : -1];
typedef char check_widget_instance[(sizeof(widget_instance) == 0x60 + 10*PW) ? 1 : -1];
typedef char check_widget_history_node[(sizeof(widget_history_node) == 0x10 + 1*PW) ? 1 : -1];
typedef char check_ui_pending_error[(sizeof(ui_pending_error) == 0x04) ? 1 : -1];
typedef char check_console_message[(sizeof(console_message) == 0x124) ? 1 : -1];
typedef char check_terminal_console[(sizeof(terminal_console) == 0x1be + 1*PW) ? 1 : -1];
typedef char check_map_list_entry[(sizeof(map_list_entry) == 0x0c + 1*PW) ? 1 : -1];
typedef char check_video_resolution[(sizeof(video_resolution) == 0x4c) ? 1 : -1];
typedef char check_ui_list_item[(sizeof(ui_list_item) == 0x10 + 2*PW) ? 1 : -1];
typedef char check_server_browser_entry[(sizeof(server_browser_entry) == 0x220) ? 1 : -1];
typedef char check_controls_device_label[(sizeof(controls_device_label) == 0x210) ? 1 : -1];
typedef char check_fpw_interface[(sizeof(first_person_weapon_interface) == 0x1ea0) ? 1 : -1];
typedef char check_hud_message_slot[(sizeof(hud_message_slot) == 0x8c) ? 1 : -1];
typedef char check_hud_player_messaging[(sizeof(hud_player_messaging_state) == 0x460) ? 1 : -1];
typedef char check_hud_messaging_globals[(sizeof(hud_messaging_globals) == 0x488) ? 1 : -1];
typedef char check_hud_text_message[(sizeof(hud_text_message) == 0x14 + 1*PW) ? 1 : -1];
typedef char check_hud_waypoint[(sizeof(hud_waypoint) == 0x0c) ? 1 : -1];
typedef char check_hud_waypoint_state[(sizeof(hud_waypoint_state) == 0x30) ? 1 : -1];
typedef char check_motion_sensor_blip[(sizeof(motion_sensor_blip) == 0x04) ? 1 : -1];
typedef char check_motion_sensor_frame[(sizeof(motion_sensor_frame) == 0x84) ? 1 : -1];
typedef char check_motion_sensor_player[(sizeof(motion_sensor_player_state) == 0x568) ? 1 : -1];
typedef char check_motion_sensor_globals[(sizeof(motion_sensor_globals) == 0x570) ? 1 : -1];
typedef char check_hud_unit_meter_state[(sizeof(hud_unit_meter_state) == 0x5c) ? 1 : -1];
typedef char check_hud_weapon_state[(sizeof(hud_weapon_interface_state) == 0x7c) ? 1 : -1];
typedef char check_hud_globals_flags[(sizeof(hud_globals_flags) == 0x04) ? 1 : -1];
typedef char check_virtual_keyboard[(sizeof(virtual_keyboard_globals) == 0x74 + 3*PW) ? 1 : -1];
typedef char check_player_control_settings[(sizeof(player_control_settings) == 0x217) ? 1 : -1];

// Field offsets the arithmetic pins individually, so a reordering fails here.
typedef char chk_widget_parent[(__builtin_offsetof(widget_instance, parent) == 0x30 + 3*PW) ? 1 : -1];
typedef char chk_widget_first_child[(__builtin_offsetof(widget_instance, first_child) == 0x34 + 4*PW) ? 1 : -1];
typedef char chk_widget_focused[(__builtin_offsetof(widget_instance, focused_child) == 0x38 + 5*PW) ? 1 : -1];
typedef char chk_widget_type[(__builtin_offsetof(widget_instance, widget_type) == 0x0e + 1*PW) ? 1 : -1];
typedef char chk_console_text[(__builtin_offsetof(console_message, text) == 0x0d) ? 1 : -1];
typedef char chk_console_color[(__builtin_offsetof(console_message, color) == 0x110) ? 1 : -1];
typedef char chk_console_age[(__builtin_offsetof(console_message, age) == 0x120) ? 1 : -1];
typedef char chk_terminal_prompt[(__builtin_offsetof(terminal_console, prompt) == 0x94) ? 1 : -1];
typedef char chk_terminal_input[(__builtin_offsetof(terminal_console, input) == 0xb4) ? 1 : -1];
typedef char chk_terminal_edit[(__builtin_offsetof(terminal_console, edit) == 0x1b4) ? 1 : -1];
typedef char chk_fpw_weapon[(__builtin_offsetof(first_person_weapon_interface, weapon_index) == 0x08) ? 1 : -1];
typedef char chk_fpw_control[(__builtin_offsetof(first_person_weapon_interface, animation_control) == 0x8c) ? 1 : -1];
typedef char chk_fpw_pose[(__builtin_offsetof(first_person_weapon_interface, previous_pose) == 0x88c) ? 1 : -1];
typedef char chk_fpw_hud[(__builtin_offsetof(first_person_weapon_interface, weapon_hud_valid) == 0x1d8c) ? 1 : -1];
typedef char chk_fpw_dev[(__builtin_offsetof(first_person_weapon_interface, device_hud_valid) == 0x1e0e) ? 1 : -1];
typedef char chk_msg_active[(__builtin_offsetof(hud_message_slot, active) == 0x82) ? 1 : -1];
typedef char chk_msg_source[(__builtin_offsetof(hud_message_slot, source) == 0x84) ? 1 : -1];
typedef char chk_msg_args[(__builtin_offsetof(hud_player_messaging_state, arguments) == 0x434) ? 1 : -1];
typedef char chk_msg_prompt[(__builtin_offsetof(hud_player_messaging_state, action_prompt_shown) == 0x458) ? 1 : -1];
typedef char chk_msg_seq[(__builtin_offsetof(hud_messaging_globals, next_sequence) == 0x465) ? 1 : -1];
typedef char chk_msg_counter[(__builtin_offsetof(hud_messaging_globals, counter_target) == 0x47c) ? 1 : -1];
typedef char chk_ms_extra[(__builtin_offsetof(motion_sensor_frame, extra_blips) == 0x40) ? 1 : -1];
typedef char chk_ms_facing[(__builtin_offsetof(motion_sensor_frame, viewer_facing) == 0x7c) ? 1 : -1];
typedef char chk_ms_tracked[(__builtin_offsetof(motion_sensor_player_state, tracked_objects) == 0x528) ? 1 : -1];
typedef char chk_ms_frameidx[(__builtin_offsetof(motion_sensor_globals, frame_index) == 0x56c) ? 1 : -1];
typedef char chk_vres_rates[(__builtin_offsetof(video_resolution, refresh_rates) == 0x2c) ? 1 : -1];
typedef char chk_server_addr[(__builtin_offsetof(server_browser_entry, address) == 0x20c) ? 1 : -1];
typedef char chk_device_type[(__builtin_offsetof(controls_device_label, device_type) == 0x20c) ? 1 : -1];
typedef char chk_vkbd_text[(__builtin_offsetof(virtual_keyboard_globals, text) == 0x28 + 3*PW) ? 1 : -1];

int main(void) { return 0; }
