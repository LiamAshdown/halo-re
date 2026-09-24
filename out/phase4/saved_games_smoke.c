#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// The header targets a 32-bit image, so every size below is stated for 4-byte
// pointers and corrected by the host pointer width times the number of pointers
// the struct contains.
#define PW ((int)sizeof(void *) - 4)
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes
typedef char check_win32_systemtime[(sizeof(win32_systemtime) == 0x10) ? 1 : -1];
typedef char check_game_state_header[(sizeof(game_state_header) == k_game_state_header_size) ? 1 : -1];
typedef char check_checkpoint_file_entry[(sizeof(checkpoint_file_entry) == 0x48) ? 1 : -1];
typedef char check_checkpoint_array[(sizeof(checkpoint_file_entry) * k_maximum_checkpoint_files == 0x1cb0) ? 1 : -1];
typedef char check_saved_player_profile[(sizeof(saved_player_profile) == k_saved_player_profile_size) ? 1 : -1];
typedef char check_saved_player_profile_file[(sizeof(saved_player_profile_file) == k_saved_player_profile_file_size) ? 1 : -1];
typedef char check_game_variant_file[(sizeof(game_variant_file) == k_game_variant_file_size) ? 1 : -1];
typedef char check_saved_player_profile_slot[(sizeof(saved_player_profile_slot) == 0x2004) ? 1 : -1];
typedef char check_variant_write_request[(sizeof(variant_write_request) == 0x9c) ? 1 : -1];
typedef char check_saved_game_index_entry[(sizeof(saved_game_index_entry) == k_savegame_index_record_size) ? 1 : -1];
typedef char check_saved_game_index_entry_b[(sizeof(saved_game_index_entry) == sizeof(savegame_index_record)) ? 1 : -1];
typedef char check_xgame_find_data[(sizeof(xgame_find_data) == 0x344) ? 1 : -1];
typedef char check_file_reference_record[(sizeof(file_reference_record) == 0x10c + PW) ? 1 : -1];
typedef char check_control_binding_descriptor[(sizeof(control_binding_descriptor) == 0x0c) ? 1 : -1];

// global block arithmetic
typedef char check_slot_stride[(sizeof(saved_player_profile_slot) == 0x801 * 4) ? 1 : -1];
typedef char check_slot_handle[(0x00712dd8 + OFF(saved_player_profile_slot, handle) == 0x00714dd4) ? 1 : -1];
typedef char check_default_cache[(0x0071d280 + 0x1001 * 4 == 0x00721284) ? 1 : -1];
typedef char check_variant_request_end[(0x00721288 + sizeof(variant_write_request) == 0x00721324) ? 1 : -1];
typedef char check_files_block[(0x00721330 + 0x2c7 * 4 == 0x00721e4c) ? 1 : -1];
typedef char check_files_index_handle[(0x00721330 + 0x108 == 0x00721438) ? 1 : -1];

// game_state_header
typedef char chk_gsh_name[(OFF(game_state_header, scenario_name) == 0x004) ? 1 : -1];
typedef char chk_gsh_build[(OFF(game_state_header, build_version) == 0x104) ? 1 : -1];
typedef char chk_gsh_players[(OFF(game_state_header, local_player_count) == 0x124) ? 1 : -1];
typedef char chk_gsh_difficulty[(OFF(game_state_header, difficulty) == 0x126) ? 1 : -1];
typedef char chk_gsh_map[(OFF(game_state_header, map_checksum) == 0x128) ? 1 : -1];
typedef char chk_gsh_file[(OFF(game_state_header, file_checksum) == 0x148) ? 1 : -1];

// checkpoint_file_entry
typedef char chk_cp_time[(OFF(checkpoint_file_entry, game_time) == 0x04) ? 1 : -1];
typedef char chk_cp_diff[(OFF(checkpoint_file_entry, difficulty) == 0x08) ? 1 : -1];
typedef char chk_cp_systime[(OFF(checkpoint_file_entry, time) == 0x0c) ? 1 : -1];
typedef char chk_cp_filetime[(OFF(checkpoint_file_entry, last_write_time) == 0x1c) ? 1 : -1];
typedef char chk_cp_kind[(OFF(checkpoint_file_entry, kind) == 0x24) ? 1 : -1];
typedef char chk_cp_name[(OFF(checkpoint_file_entry, name) == 0x28) ? 1 : -1];

// saved_player_profile
typedef char chk_pp_name[(OFF(saved_player_profile, name) == 0x002) ? 1 : -1];
typedef char chk_pp_color[(OFF(saved_player_profile, player_color) == 0x11a) ? 1 : -1];
typedef char chk_pp_flags[(OFF(saved_player_profile, flags) == 0x11c) ? 1 : -1];
typedef char chk_pp_progress[(OFF(saved_player_profile, campaign_progress) == 0x11e) ? 1 : -1];
typedef char chk_pp_last[(OFF(saved_player_profile, last_campaign_level) == 0x128) ? 1 : -1];
typedef char chk_pp_buttonset[(OFF(saved_player_profile, button_set) == 0x12c) ? 1 : -1];
typedef char chk_pp_look[(OFF(saved_player_profile, look_sensitivity) == 0x12e) ? 1 : -1];
typedef char chk_pp_133[(OFF(saved_player_profile, unknown_133) == 0x133) ? 1 : -1];
typedef char chk_pp_kbd[(OFF(saved_player_profile, keyboard_bindings) == 0x134) ? 1 : -1];
typedef char chk_pp_mbtn[(OFF(saved_player_profile, mouse_button_bindings) == 0x20e) ? 1 : -1];
typedef char chk_pp_maxis[(OFF(saved_player_profile, mouse_axis_bindings) == 0x21e) ? 1 : -1];
typedef char chk_pp_gbtn[(OFF(saved_player_profile, gamepad_button_bindings) == 0x22a) ? 1 : -1];
typedef char chk_pp_gact[(OFF(saved_player_profile, gamepad_action_buttons) == 0x32a) ? 1 : -1];
typedef char chk_pp_gaxis[(OFF(saved_player_profile, gamepad_axis_bindings) == 0x33a) ? 1 : -1];
typedef char chk_pp_gpov[(OFF(saved_player_profile, gamepad_pov_bindings) == 0x53a) ? 1 : -1];
typedef char chk_pp_93a[(OFF(saved_player_profile, unknown_93a) == 0x93a) ? 1 : -1];
typedef char chk_pp_93c[(OFF(saved_player_profile, unknown_93c) == 0x93c) ? 1 : -1];
typedef char chk_pp_954[(OFF(saved_player_profile, unknown_954) == 0x954) ? 1 : -1];
typedef char chk_pp_rate_a[(OFF(saved_player_profile, gamepad_rate_a) == 0x956) ? 1 : -1];
typedef char chk_pp_rate_b[(OFF(saved_player_profile, gamepad_rate_b) == 0x95a) ? 1 : -1];
typedef char chk_pp_960[(OFF(saved_player_profile, unknown_960) == 0x960) ? 1 : -1];
typedef char chk_pp_968[(OFF(saved_player_profile, unknown_968) == 0x968) ? 1 : -1];
typedef char chk_pp_width[(OFF(saved_player_profile, screen_width) == 0xa68) ? 1 : -1];
typedef char chk_pp_height[(OFF(saved_player_profile, screen_height) == 0xa6a) ? 1 : -1];
typedef char chk_pp_refresh[(OFF(saved_player_profile, refresh_rate) == 0xa6c) ? 1 : -1];
typedef char chk_pp_a6e[(OFF(saved_player_profile, unknown_a6e) == 0xa6e) ? 1 : -1];
typedef char chk_pp_a70[(OFF(saved_player_profile, unknown_a70) == 0xa70) ? 1 : -1];
typedef char chk_pp_a75[(OFF(saved_player_profile, unknown_a75) == 0xa75) ? 1 : -1];
typedef char chk_pp_gamma[(OFF(saved_player_profile, gamma) == 0xa76) ? 1 : -1];
typedef char chk_pp_master[(OFF(saved_player_profile, master_volume) == 0xb78) ? 1 : -1];
typedef char chk_pp_music[(OFF(saved_player_profile, music_volume) == 0xb7a) ? 1 : -1];
typedef char chk_pp_b7d[(OFF(saved_player_profile, unknown_b7d) == 0xb7d) ? 1 : -1];
typedef char chk_pp_b7f[(OFF(saved_player_profile, unknown_b7f) == 0xb7f) ? 1 : -1];
typedef char chk_pp_c80[(OFF(saved_player_profile, unknown_c80) == 0xc80) ? 1 : -1];
typedef char chk_pp_c8a[(OFF(saved_player_profile, unknown_c8a) == 0xc8a) ? 1 : -1];
typedef char chk_pp_d8b[(OFF(saved_player_profile, unknown_d8b) == 0xd8b) ? 1 : -1];
typedef char chk_pp_server[(OFF(saved_player_profile, server_name) == 0xd8c) ? 1 : -1];
typedef char chk_pp_password[(OFF(saved_player_profile, server_password) == 0xeac) ? 1 : -1];
typedef char chk_pp_ebe[(OFF(saved_player_profile, unknown_ebe) == 0xebe) ? 1 : -1];
typedef char chk_pp_ebf[(OFF(saved_player_profile, unknown_ebf) == 0xebf) ? 1 : -1];
typedef char chk_pp_fc0[(OFF(saved_player_profile, unknown_fc0) == 0xfc0) ? 1 : -1];
typedef char chk_pp_fc2[(OFF(saved_player_profile, unknown_fc2) == 0xfc2) ? 1 : -1];
typedef char chk_pp_sport[(OFF(saved_player_profile, server_port) == 0x1002) ? 1 : -1];
typedef char chk_pp_cport[(OFF(saved_player_profile, client_port) == 0x1004) ? 1 : -1];
typedef char chk_pp_gamepads[(OFF(saved_player_profile, gamepads) == 0x1108) ? 1 : -1];
typedef char chk_pp_gamepad_key[(0x1108 + OFF(controls_gamepad_record, device_key) == 0x1314) ? 1 : -1];
typedef char chk_pp_gamepad_2[(0x1108 + 2 * sizeof(controls_gamepad_record) == 0x1548) ? 1 : -1];
typedef char chk_pp_tail[(OFF(saved_player_profile, unknown_1988) == 0x1988) ? 1 : -1];

// carry-over block lengths in player_profile_initialize
typedef char chk_pp_copy_controls[(OFF(saved_player_profile, screen_width) - OFF(saved_player_profile, button_set) == 0x24f * 4) ? 1 : -1];
typedef char chk_pp_copy_video[(OFF(saved_player_profile, master_volume) - OFF(saved_player_profile, screen_width) == 0x44 * 4) ? 1 : -1];
typedef char chk_pp_copy_audio[(OFF(saved_player_profile, unknown_c80) - OFF(saved_player_profile, master_volume) == 0x42 * 4) ? 1 : -1];
typedef char chk_pp_copy_c80[(OFF(saved_player_profile, unknown_d8b) - OFF(saved_player_profile, unknown_c80) == 0x42 * 4 + 3) ? 1 : -1];
typedef char chk_pp_copy_pads[(OFF(saved_player_profile, unknown_1988) - OFF(saved_player_profile, gamepads) == 0x220 * 4) ? 1 : -1];

// saved_player_profile_file / game_variant_file
typedef char chk_ppf_crc[(OFF(saved_player_profile_file, checksum) == 0x1ffc) ? 1 : -1];
typedef char chk_gvf_crc[(OFF(game_variant_file, checksum) == 0x98) ? 1 : -1];
typedef char chk_gv_flags[(OFF(game_variant, unknown_94) == 0x94) ? 1 : -1];
typedef char chk_gv_name_end[(OFF(game_variant, name) + 23 * 2 == 0x2e) ? 1 : -1];
typedef char chk_vwr_variant[(OFF(variant_write_request, variant) == 0x04) ? 1 : -1];

// saved_game_index_entry
typedef char chk_sgi_name[(OFF(saved_game_index_entry, display_name) == 0x100) ? 1 : -1];
typedef char chk_sgi_type[(OFF(saved_game_index_entry, type) == 0x200) ? 1 : -1];
typedef char chk_sgi_index[(OFF(saved_game_index_entry, index) == 0x202) ? 1 : -1];
typedef char chk_sgi_builtin[(OFF(saved_game_index_entry, builtin) == 0x204) ? 1 : -1];
typedef char chk_sgi_valid[(OFF(saved_game_index_entry, checksum_valid) == 0x205) ? 1 : -1];

// xgame_find_data
typedef char chk_xfd_dir[(OFF(xgame_find_data, save_game_directory) == 0x140) ? 1 : -1];
typedef char chk_xfd_name[(OFF(xgame_find_data, save_game_name) == 0x244) ? 1 : -1];

// file_reference_record
typedef char chk_fr_flags[(OFF(file_reference_record, flags) == 0x004) ? 1 : -1];
typedef char chk_fr_location[(OFF(file_reference_record, location) == 0x006) ? 1 : -1];
typedef char chk_fr_path[(OFF(file_reference_record, path) == 0x008) ? 1 : -1];
typedef char chk_fr_handle[(OFF(file_reference_record, handle) == 0x108) ? 1 : -1];

// control_binding_descriptor
typedef char chk_cbd_kind[(OFF(control_binding_descriptor, input_kind) == 0x04) ? 1 : -1];
typedef char chk_cbd_index[(OFF(control_binding_descriptor, input_index) == 0x06) ? 1 : -1];
typedef char chk_cbd_dir[(OFF(control_binding_descriptor, direction) == 0x08) ? 1 : -1];

// the per-slot binding strides control_profile_set_binding multiplies by
typedef char chk_stride_gbtn[(sizeof(((saved_player_profile *)0)->gamepad_button_bindings[0]) == 0x40) ? 1 : -1];
typedef char chk_stride_gaxis[(sizeof(((saved_player_profile *)0)->gamepad_axis_bindings[0]) == 0x80) ? 1 : -1];
typedef char chk_stride_gpov[(sizeof(((saved_player_profile *)0)->gamepad_pov_bindings[0]) == 0x100) ? 1 : -1];

int main(void) { return 0; }
