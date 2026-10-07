/**
 * @file include/halo/saved_games/api.hpp
 * Functions of the saved_games module that other modules and the data tables call (namespace halo::saved_games). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>



struct checkpoint_file_entry;
struct control_binding_descriptor;
struct controls_gamepad_record;
struct data_array;
struct file_reference_record;
struct game_state_header;
struct game_variant;
struct memory_pool;
struct saved_player_profile;
struct variant_write_request;
struct win32_systemtime;
typedef uint8_t (*checkpoint_enumerate_proc)(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time, const struct win32_systemtime *time, void *user_data);

namespace halo::saved_games {

/**
 * The engine globals the saved_games module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    uint8_t *&game_state_base;
    uint32_t &game_state_crc;
    int32_t &game_state_cursor;
    int32_t &player_profile_slots_handle;
    uint8_t &game_state_write_in_progress;
    uint8_t &profile_load_complete;
};

Globals &globals();

void control_profile_clear_binding(const control_binding_descriptor *binding);
void control_profile_clear_device_slot_mappings(saved_player_profile *profile);
uint8_t control_profile_copy_gamepad_bindings_by_key(controls_gamepad_record *key, saved_player_profile *dest, saved_player_profile *source);
void control_profile_fill_default_gamepad_slots(saved_player_profile *profile);
uint8_t control_profile_finalize_slot(saved_player_profile *profile, int32_t gamepad_index);
uint8_t control_profile_find_binding_for_action(const char *action_name, control_binding_descriptor *binding);
uint8_t control_profile_find_or_create_gamepad_slot(controls_gamepad_record *source, saved_player_profile *profile);
int32_t control_profile_gamepad_slot_find(saved_player_profile *profile, controls_gamepad_record *key);
uint8_t control_profile_is_customized(saved_player_profile *profile, int32_t gamepad_index);
void control_profile_reestablish_device_slot_mappings(saved_player_profile *profile);
void control_profile_reset_analog_bindings(saved_player_profile *profile);
void control_profile_reset_digital_bindings(saved_player_profile *profile);
void control_profile_reset_slot(saved_player_profile *profile, int32_t gamepad_index);
uint8_t control_profile_set_binding(const control_binding_descriptor *binding, int16_t value);
void control_profile_variant_write_wait_and_clear(void);
void directory_ensure_empty(const char *directory_path);
uint8_t file_enumerate_find_next(file_reference_record *out_entry, uint32_t *out_write_time);
void file_enumerate_start(uint32_t flags, file_reference_record *ref);
uint8_t file_reference_close(file_reference_record *ref);
int32_t file_reference_compare_full_path(const file_reference_record *a, const file_reference_record *b);
uint8_t file_reference_create(file_reference_record *ref);
uint8_t file_reference_delete(file_reference_record *ref);
uint8_t file_reference_exists(file_reference_record *ref);
uint32_t file_reference_get_size(file_reference_record *ref);
uint8_t file_reference_get_size_by_path(file_reference_record *ref, uint32_t *out_size);
file_reference_record * file_reference_init(file_reference_record *ref, const char *component, uint8_t is_file);
uint8_t file_reference_open(file_reference_record *ref, uint8_t mode);
uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size);
uint8_t file_reference_seek(int32_t offset, file_reference_record *ref);
uint8_t file_reference_set_length(int32_t offset, file_reference_record *ref);
uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size);
int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, checkpoint_enumerate_proc callback, void *user_data);
uint8_t game_checkpoint_get_next_filename(char *out_name, char *directory);
uint8_t game_checkpoint_print_list_entry(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data);
int16_t game_checkpoint_read_stats_file(int32_t *out_difficulty, const char *name, int32_t *out_game_time, win32_systemtime *out_time);
uint8_t game_checkpoint_reclaim_slot_callback(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data);
uint8_t game_checkpoint_save_new(void);
void game_checkpoint_write_stats_file(char *scenario_name, int32_t difficulty);
void game_state_after_load_restore_time(void);
void * game_state_allocate_buffer(int32_t cpu_size, int32_t extra_size);
void game_state_build_header(void);
void game_state_create_persistent_storage_file(void);
void game_state_dispatch_load_callbacks(void);
void game_state_load_checkpoint(void);
void game_state_load_core(char *name);
data_array * game_state_new(const char *name, int16_t maximum_count, int16_t element_size);
memory_pool * game_state_new_pool(const char *name, int32_t pool_size);
void * game_state_open_persistent_storage(char *name);
void game_state_perform_revert(void);
/** The live game state blob, and making a blob (possibly another machine's) the game state the way a revert does. */
const uint8_t *game_state_snapshot_bytes(uint32_t *size);
void game_state_apply_snapshot(const uint8_t *bytes);
/** What a revert goes back to (the last save), and making a blob (possibly another machine's) that. */
const uint8_t *game_state_checkpoint_bytes();
void game_state_set_checkpoint(const uint8_t *bytes);
void game_state_perform_save(uint8_t is_checkpoint);
uint8_t game_state_queue_write(uint8_t final_flag);
uint8_t game_state_read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty, char *out_scenario_name);
uint8_t game_state_read_persistent_storage(void);
void game_state_read_persistent_storage_block(int32_t size, void *buffer);
void game_state_read_profile_file(char *name, int32_t size, void *buffer);
uint8_t game_state_read_profile_header(char *name, int32_t size, void *buffer);
void game_state_save_thread_proc(void *parameter);
void game_state_startup(void);
void game_state_write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size, int32_t total_size);
uint8_t game_state_write_profile_file(int32_t size, char *name, const void *buffer);
void game_variant_write_request_start(int32_t handle, game_variant *variant);
uint32_t game_variant_write_thread_proc(variant_write_request *request);
void path_append_component(char *destination, const char *component);
void path_append_extension(char *destination, const char *suffix);
void path_build_full(char *source, char *destination, int16_t location);
void path_remove_last_component(char *path);
void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out, char **name_end_out, char **ext_start_out, uint8_t split_extension);
float * player_color_get_rgb(float *out_rgb, int32_t color_index);
uint32_t player_profile_copy_files(const char *source_dir, char *dest_dir);
uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer);
uint8_t player_profile_get_or_cached_default(saved_player_profile *out_buffer, int32_t index);
void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index, uint8_t merge_existing);
void player_profile_mark_level_visited_and_select(int16_t local_player_index);
uint8_t player_profile_rename(int32_t handle, uint16_t *new_name);
void player_profile_save_539bf0(int32_t handle, saved_player_profile *profile);
void player_profile_scan_campaign_progress(int16_t *out_type, saved_player_profile *profile, int16_t *out_level);
void player_profile_select_local_slot(int16_t local_player_index);
uint8_t player_profile_set_default_audio_options(saved_player_profile *profile);
void player_profile_set_default_server_options(saved_player_profile *profile);
uint8_t player_profile_set_default_video_options(saved_player_profile *profile, uint8_t allow_display_query);
void player_profile_verify_thread_wait_and_clear(void);
void player_profile_write_data(int32_t handle, saved_player_profile *profile);
void player_profile_write_default_files(void);
void playlist_profile_create_default_profiles_on_disk(void);
void saved_game_allocate_new_slot(uint16_t *out_name);
int32_t saved_game_check_storage_availability(void);
int32_t saved_game_checkpoint_compare(const checkpoint_file_entry *a, const checkpoint_file_entry *b);
uint8_t saved_game_copy_files_to_target(char *source_directory, const char *source_name, const char *target_name);
uint32_t saved_game_create_custom_variant(uint32_t unused, uint16_t *name);
uint32_t saved_game_create_default_profile(uint16_t *name);
uint32_t saved_game_create_slot(uint16_t type, uint16_t *name);
void saved_game_delete_by_display_name(const char *name);
uint8_t saved_game_delete_by_handle(int32_t handle);
uint8_t saved_game_delete_files(char *name);
void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count);
uint8_t saved_game_file_exists(const char *name);
void saved_game_files_dispose(void);
void saved_game_files_initialize(void);
int32_t saved_game_find_by_name(char *name, int16_t type);
uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory);
uint16_t * saved_game_get_display_name(int32_t handle);
uint8_t saved_game_get_variant(int32_t handle, game_variant *out);
uint8_t saved_game_index_open_for_write(void);
int16_t saved_game_index_register_default_playlists(void);
int16_t saved_game_index_register_default_profiles(void);
void saved_game_last_mp_map_clear(const void *data);
uint8_t saved_game_last_mp_map_read(uint8_t *out_data);
void saved_game_last_mp_variant_clear(const void *data);
uint8_t saved_game_last_mp_variant_read(uint8_t *out_data);
void saved_game_last_profile_clear(const void *data);
uint8_t saved_game_last_profile_read(uint8_t *out_data);
void saved_game_list_rebuild_index(void);
uint8_t saved_game_load_checkpoint(char *name);
uint8_t saved_game_load_checkpoint_by_name(const char *name);
uint8_t saved_game_name_is_available(const uint16_t *name);
uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref);
uint8_t saved_game_validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer, uint32_t *expected_crc, uint8_t *corrupt_flag);
uint8_t saved_game_verify_version_and_checksum(game_state_header *header, uint8_t report_error);
void saved_games_report_last_error(void);

}
