#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "rasterizer.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/core/link.hpp"
#include "halo/saved_games/vars.hpp"

static auto &game_state_base = halo::link::ref<uint8_t *>(halo::saved_games::vars().game_state_base);
static auto &game_state_crc = halo::link::ref<uint32_t>(halo::saved_games::vars().game_state_crc);
static auto &game_state_cursor = halo::link::ref<int32_t>(halo::saved_games::vars().game_state_cursor);
static auto &saved_player_profile_slots_handle = halo::link::ref<int32_t>(halo::saved_games::vars().saved_player_profile_slots_handle);
static auto &game_state_write_in_progress = halo::link::ref<uint8_t>(halo::saved_games::vars().game_state_write_in_progress);
static auto &profile_load_complete = halo::link::ref<uint8_t>(halo::saved_games::vars().profile_load_complete);

namespace halo::saved_games {

Globals &globals()
{
    static Globals instance{::game_state_base, ::game_state_crc, ::game_state_cursor, ::saved_player_profile_slots_handle, ::game_state_write_in_progress, ::profile_load_complete};
    return instance;
}

void control_profile_clear_binding(const control_binding_descriptor *binding)
{
    halo::saved_games::ControlBinding(const_cast<control_binding_descriptor *>(binding)).clear_binding();
}

void control_profile_clear_device_slot_mappings(saved_player_profile *profile)
{
    halo::saved_games::control_profile::clear_device_slot_mappings(profile);
}

uint8_t control_profile_copy_gamepad_bindings_by_key(controls_gamepad_record *key,
    saved_player_profile *dest, saved_player_profile *source)
{
    return halo::saved_games::PlayerProfile(dest).copy_gamepad_bindings_by_key(key, source);
}

void control_profile_fill_default_gamepad_slots(saved_player_profile *profile)
{
    halo::saved_games::control_profile::fill_default_gamepad_slots(profile);
}

uint8_t control_profile_finalize_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    return halo::saved_games::control_profile::finalize_slot(profile, gamepad_index);
}

uint8_t control_profile_find_binding_for_action(const char *action_name,
    control_binding_descriptor *binding)
{
    return halo::saved_games::ControlBinding(binding).find_binding_for_action(action_name);
}

uint8_t control_profile_find_or_create_gamepad_slot(controls_gamepad_record *source,
    saved_player_profile *profile)
{
    return halo::saved_games::control_profile::find_or_create_gamepad_slot(source, profile);
}

int32_t control_profile_gamepad_slot_find(saved_player_profile *profile, controls_gamepad_record *key)
{
    return halo::saved_games::PlayerProfile(profile).gamepad_slot_find(key);
}

uint8_t control_profile_is_customized(saved_player_profile *profile, int32_t gamepad_index)
{
    return halo::saved_games::control_profile::is_customized(profile, gamepad_index);
}

void control_profile_reestablish_device_slot_mappings(saved_player_profile *profile)
{
    halo::saved_games::control_profile::reestablish_device_slot_mappings(profile);
}

void control_profile_reset_analog_bindings(saved_player_profile *profile)
{
    halo::saved_games::PlayerProfile(profile).reset_analog_bindings();
}

void control_profile_reset_digital_bindings(saved_player_profile *profile)
{
    halo::saved_games::PlayerProfile(profile).reset_digital_bindings();
}

void control_profile_reset_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    halo::saved_games::control_profile::reset_slot(profile, gamepad_index);
}

uint8_t control_profile_set_binding(const control_binding_descriptor *binding, int16_t value)
{
    return halo::saved_games::ControlBinding(const_cast<control_binding_descriptor *>(binding)).set_binding(value);
}

void control_profile_variant_write_wait_and_clear(void)
{
    halo::saved_games::control_profile::variant_write_wait_and_clear();
}

void directory_ensure_empty(const char *directory_path)
{
    halo::saved_games::directory::ensure_empty(directory_path);
}

uint8_t file_enumerate_find_next(file_reference_record *out_entry, uint32_t *out_write_time)
{
    return halo::saved_games::file_enumerate::find_next(out_entry, out_write_time);
}

void file_enumerate_start(uint32_t flags, file_reference_record *ref)
{
    halo::saved_games::file_enumerate::start(flags, ref);
}

uint8_t file_reference_close(file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).close();
}

int32_t file_reference_compare_full_path(const file_reference_record *a, const file_reference_record *b)
{
    return halo::saved_games::FileReference(const_cast<file_reference_record *>(a)).compare_full_path(b);
}

uint8_t file_reference_create(file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).create();
}

uint8_t file_reference_delete(file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).remove();
}

uint8_t file_reference_exists(file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).exists();
}

uint32_t file_reference_get_size(file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).get_size();
}

uint8_t file_reference_get_size_by_path(file_reference_record *ref, uint32_t *out_size)
{
    return halo::saved_games::FileReference(ref).get_size_by_path(out_size);
}

file_reference_record *file_reference_init(file_reference_record *ref, const char *component,
    uint8_t is_file)
{
    return halo::saved_games::file_reference::init(ref, component, is_file);
}

uint8_t file_reference_open(file_reference_record *ref, uint8_t mode)
{
    return halo::saved_games::FileReference(ref).open(mode);
}

uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size)
{
    return halo::saved_games::FileReference(ref).read(buffer, size);
}

uint8_t file_reference_seek(int32_t offset, file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).seek(offset);
}

uint8_t file_reference_set_length(int32_t offset, file_reference_record *ref)
{
    return halo::saved_games::FileReference(ref).set_length(offset);
}

uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size)
{
    return halo::saved_games::FileReference(ref).write(buffer, size);
}

int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first,
    checkpoint_enumerate_proc callback, void *user_data)
{
    return halo::saved_games::checkpoint::enumerate_files(include_autosaves, sort_newest_first, callback, user_data);
}

uint8_t game_checkpoint_get_next_filename(char *out_name, char *directory)
{
    return halo::saved_games::checkpoint::get_next_filename(out_name, directory);
}

uint8_t game_checkpoint_print_list_entry(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    return halo::saved_games::checkpoint::print_list_entry(index, name, level_index, difficulty, game_time_ticks, time, user_data);
}

int16_t game_checkpoint_read_stats_file(int32_t *out_difficulty, char *name, int32_t *out_game_time,
    win32_systemtime *out_time)
{
    return halo::saved_games::checkpoint::read_stats_file(out_difficulty, name, out_game_time, out_time);
}

uint8_t game_checkpoint_reclaim_slot_callback(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    return halo::saved_games::checkpoint::reclaim_slot_callback(index, name, level_index, difficulty, game_time_ticks, time, user_data);
}

uint8_t game_checkpoint_save_new(void)
{
    return halo::saved_games::checkpoint::save_new();
}

void game_checkpoint_write_stats_file(char *scenario_name, int32_t difficulty)
{
    halo::saved_games::checkpoint::write_stats_file(scenario_name, difficulty);
}

void game_state_after_load_restore_time(void)
{
    halo::saved_games::game_state::after_load_restore_time();
}

void *game_state_allocate_buffer(int32_t cpu_size, int32_t extra_size)
{
    return halo::saved_games::game_state::allocate_buffer(cpu_size, extra_size);
}

void game_state_build_header(void)
{
    halo::saved_games::game_state::build_header();
}

void game_state_create_persistent_storage_file(void)
{
    halo::saved_games::game_state::create_persistent_storage_file();
}

void game_state_dispatch_load_callbacks(void)
{
    halo::saved_games::game_state::dispatch_load_callbacks();
}

void game_state_load_checkpoint(void)
{
    halo::saved_games::game_state::load_checkpoint();
}

void game_state_load_core(char *name)
{
    halo::saved_games::game_state::load_core(name);
}

data_array *game_state_new(const char *name, int16_t maximum_count, int16_t element_size)
{
    return halo::saved_games::game_state::make(name, maximum_count, element_size);
}

memory_pool *game_state_new_pool(const char *name, int32_t pool_size)
{
    return halo::saved_games::game_state::new_pool(name, pool_size);
}

void *game_state_open_persistent_storage(char *name)
{
    return halo::saved_games::game_state::open_persistent_storage(name);
}

void game_state_perform_revert(void)
{
    halo::saved_games::game_state::perform_revert();
}

void game_state_perform_save(uint8_t is_checkpoint)
{
    halo::saved_games::game_state::perform_save(is_checkpoint);
}

uint8_t game_state_queue_write(uint8_t final_flag)
{
    return halo::saved_games::game_state::queue_write(final_flag);
}

uint8_t game_state_read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty,
    char *out_scenario_name)
{
    return halo::saved_games::game_state::read_checkpoint_summary(corrupt_flag, out_difficulty, out_scenario_name);
}

uint8_t game_state_read_persistent_storage(void)
{
    return halo::saved_games::game_state::read_persistent_storage();
}

void game_state_read_persistent_storage_block(int32_t size, void *buffer)
{
    halo::saved_games::game_state::read_persistent_storage_block(size, buffer);
}

void game_state_read_profile_file(char *name, int32_t size, void *buffer)
{
    halo::saved_games::game_state::read_profile_file(name, size, buffer);
}

uint8_t game_state_read_profile_header(char *name, int32_t size, void *buffer)
{
    return halo::saved_games::game_state::read_profile_header(name, size, buffer);
}

void game_state_save_thread_proc(void)
{
    halo::saved_games::game_state::save_thread_proc();
}

void game_state_startup(void)
{
    halo::saved_games::game_state::startup();
}

void game_state_write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size,
    int32_t total_size)
{
    halo::saved_games::game_state::write_persistent_storage(crc_slot, buffer, header_size, total_size);
}

uint8_t game_state_write_profile_file(int32_t size, char *name, const void *buffer)
{
    return halo::saved_games::game_state::write_profile_file(size, name, buffer);
}

void game_variant_write_request_start(int32_t handle, game_variant *variant)
{
    halo::saved_games::variant::write_request_start(handle, variant);
}

uint32_t game_variant_write_thread_proc(variant_write_request *request)
{
    return halo::saved_games::VariantWriteRequest(request).thread_proc();
}

void path_append_component(char *destination, const char *component)
{
    halo::saved_games::path::append_component(destination, component);
}

void path_append_extension(char *destination, const char *suffix)
{
    halo::saved_games::path::append_extension(destination, suffix);
}

void path_build_full(char *source, char *destination, int16_t location)
{
    halo::saved_games::path::build_full(source, destination, location);
}

void path_remove_last_component(char *path)
{
    halo::saved_games::path::remove_last_component(path);
}

void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out, char **name_end_out,
    char **ext_start_out, uint8_t split_extension)
{
    halo::saved_games::path::split_components(dir_start_out, path, ext_fallback_out, name_end_out, ext_start_out, split_extension);
}

float *player_color_get_rgb(float *out_rgb, int32_t color_index)
{
    return halo::saved_games::player_color::get_rgb(out_rgb, color_index);
}

uint32_t player_profile_copy_files(const char *source_dir, char *dest_dir)
{
    return halo::saved_games::player_profile::copy_files(source_dir, dest_dir);
}

uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer)
{
    return halo::saved_games::PlayerProfile(out_buffer).get(index);
}

uint8_t player_profile_get_or_cached_default(saved_player_profile *out_buffer, int32_t index)
{
    return halo::saved_games::PlayerProfile(out_buffer).get_or_cached_default(index);
}

void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index,
    uint8_t merge_existing)
{
    halo::saved_games::PlayerProfile(profile).initialize(local_player_index, merge_existing);
}

void player_profile_mark_level_visited_and_select(int16_t local_player_index)
{
    halo::saved_games::player_profile::mark_level_visited_and_select(local_player_index);
}

uint8_t player_profile_rename(int32_t handle, uint16_t *new_name)
{
    return halo::saved_games::player_profile::rename(handle, new_name);
}

void player_profile_save_539bf0(int32_t handle, saved_player_profile *profile)
{
    halo::saved_games::PlayerProfile(profile).save_539bf0(handle);
}

void player_profile_scan_campaign_progress(int16_t *out_type, saved_player_profile *profile,
    int16_t *out_level)
{
    halo::saved_games::PlayerProfile(profile).scan_campaign_progress(out_type, out_level);
}

void player_profile_select_local_slot(int16_t local_player_index)
{
    halo::saved_games::player_profile::select_local_slot(local_player_index);
}

uint8_t player_profile_set_default_audio_options(saved_player_profile *profile)
{
    return halo::saved_games::PlayerProfile(profile).set_default_audio_options();
}

void player_profile_set_default_server_options(saved_player_profile *profile)
{
    halo::saved_games::PlayerProfile(profile).set_default_server_options();
}

uint8_t player_profile_set_default_video_options(saved_player_profile *profile,
    uint8_t allow_display_query)
{
    return halo::saved_games::PlayerProfile(profile).set_default_video_options(allow_display_query);
}

void player_profile_verify_thread_wait_and_clear(void)
{
    halo::saved_games::player_profile::verify_thread_wait_and_clear();
}

void player_profile_write_data(int32_t handle, saved_player_profile *profile)
{
    halo::saved_games::PlayerProfile(profile).write_data(handle);
}

void player_profile_write_default_files(void)
{
    halo::saved_games::player_profile::write_default_files();
}

void playlist_profile_create_default_profiles_on_disk(void)
{
    halo::saved_games::playlist_profile::create_default_profiles_on_disk();
}

void saved_game_allocate_new_slot(uint16_t *out_name)
{
    halo::saved_games::saved_game::allocate_new_slot(out_name);
}

int32_t saved_game_check_storage_availability(void)
{
    return halo::saved_games::saved_game::check_storage_availability();
}

int32_t saved_game_checkpoint_compare(const checkpoint_file_entry *a, const checkpoint_file_entry *b)
{
    return halo::saved_games::checkpoint::compare(a, b);
}

uint8_t saved_game_copy_files_to_target(char *source_directory, char *source_name, char *target_name)
{
    return halo::saved_games::saved_game::copy_files_to_target(source_directory, source_name, target_name);
}

uint32_t saved_game_create_custom_variant(uint32_t unused, uint16_t *name)
{
    return halo::saved_games::saved_game::create_custom_variant(unused, name);
}

uint32_t saved_game_create_default_profile(uint16_t *name)
{
    return halo::saved_games::saved_game::create_default_profile(name);
}

uint32_t saved_game_create_slot(uint16_t type, uint16_t *name)
{
    return halo::saved_games::saved_game::create_slot(type, name);
}

void saved_game_delete_by_display_name(const char *name)
{
    halo::saved_games::saved_game::delete_by_display_name(name);
}

uint8_t saved_game_delete_by_handle(int32_t handle)
{
    return halo::saved_games::saved_game::delete_by_handle(handle);
}

uint8_t saved_game_delete_files(char *name)
{
    return halo::saved_games::saved_game::delete_files(name);
}

void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count)
{
    halo::saved_games::saved_game::enumerate_by_type(type, out_handles, builtin_only, capacity_and_count);
}

uint8_t saved_game_file_exists(char *name)
{
    return halo::saved_games::saved_game::file_exists(name);
}

void saved_game_files_dispose(void)
{
    halo::saved_games::saved_game::files_dispose();
}

void saved_game_files_initialize(void)
{
    halo::saved_games::saved_game::files_initialize();
}

int32_t saved_game_find_by_name(char *name, int16_t type)
{
    return halo::saved_games::saved_game::find_by_name(name, type);
}

uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory)
{
    return halo::saved_games::saved_game::get_directory_by_handle(handle, out_directory);
}

uint16_t *saved_game_get_display_name(int32_t handle)
{
    return halo::saved_games::saved_game::get_display_name(handle);
}

uint8_t saved_game_get_variant(int32_t handle, game_variant *out)
{
    return halo::saved_games::saved_game::get_variant(handle, out);
}

uint8_t saved_game_index_open_for_write(void)
{
    return halo::saved_games::saved_game::index_open_for_write();
}

int16_t saved_game_index_register_default_playlists(void)
{
    return halo::saved_games::saved_game::index_register_default_playlists();
}

int16_t saved_game_index_register_default_profiles(void)
{
    return halo::saved_games::saved_game::index_register_default_profiles();
}

void saved_game_last_mp_map_clear(const void *data)
{
    halo::saved_games::saved_game::last_mp_map_clear(data);
}

uint8_t saved_game_last_mp_map_read(uint8_t *out_data)
{
    return halo::saved_games::saved_game::last_mp_map_read(out_data);
}

void saved_game_last_mp_variant_clear(const void *data)
{
    halo::saved_games::saved_game::last_mp_variant_clear(data);
}

uint8_t saved_game_last_mp_variant_read(uint8_t *out_data)
{
    return halo::saved_games::saved_game::last_mp_variant_read(out_data);
}

void saved_game_last_profile_clear(const void *data)
{
    halo::saved_games::saved_game::last_profile_clear(data);
}

uint8_t saved_game_last_profile_read(uint8_t *out_data)
{
    return halo::saved_games::saved_game::last_profile_read(out_data);
}

void saved_game_list_rebuild_index(void)
{
    halo::saved_games::saved_game::list_rebuild_index();
}

uint8_t saved_game_load_checkpoint(char *name)
{
    return halo::saved_games::saved_game::load_checkpoint(name);
}

uint8_t saved_game_load_checkpoint_by_name(char *name)
{
    return halo::saved_games::saved_game::load_checkpoint_by_name(name);
}

uint8_t saved_game_name_is_available(const uint16_t *name)
{
    return halo::saved_games::saved_game::name_is_available(name);
}

uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref)
{
    return halo::saved_games::saved_game::open_file_by_handle(handle, out_ref);
}

uint8_t saved_game_validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer,
    uint32_t *expected_crc, uint8_t *corrupt_flag)
{
    return halo::saved_games::saved_game::validate_crc(total_size, header_size, header_buffer, expected_crc, corrupt_flag);
}

uint8_t saved_game_verify_version_and_checksum(game_state_header *header, uint8_t report_error)
{
    return halo::saved_games::saved_game::verify_version_and_checksum(header, report_error);
}

void saved_games_report_last_error(void)
{
    halo::saved_games::saved_game::report_last_error();
}

}
