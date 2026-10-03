#pragma once

/**
 * C++ API of the saved_games module. The engine headers (tags.h ... saved_games.h) must be included before this one.
 */

namespace halo::saved_games {

/**
 * View over a control_binding_descriptor record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class ControlBinding {
public:
    explicit ControlBinding(control_binding_descriptor *record) : self(record) {}
    void clear_binding() const;
    uint8_t find_binding_for_action(const char *action_name);
    uint8_t set_binding(int16_t value) const;

private:
    control_binding_descriptor *self;
};

}  // namespace halo::saved_games

namespace halo::saved_games {

/**
 * View over a saved_player_profile record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class PlayerProfile {
public:
    explicit PlayerProfile(saved_player_profile *record) : self(record) {}
    uint8_t copy_gamepad_bindings_by_key(controls_gamepad_record *key, saved_player_profile *source);
    int32_t gamepad_slot_find(controls_gamepad_record *key);
    void reset_analog_bindings();
    void reset_digital_bindings();
    uint8_t get(int32_t index);
    uint8_t get_or_cached_default(int32_t index);
    void initialize(int32_t local_player_index, uint8_t merge_existing);
    void save_539bf0(int32_t handle);
    void scan_campaign_progress(int16_t *out_type, int16_t *out_level);
    uint8_t set_default_audio_options();
    void set_default_server_options();
    uint8_t set_default_video_options(uint8_t allow_display_query);
    void write_data(int32_t handle);

private:
    saved_player_profile *self;
};

}  // namespace halo::saved_games

namespace halo::saved_games {

/**
 * View over a file_reference_record record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class FileReference {
public:
    explicit FileReference(file_reference_record *record) : self(record) {}
    uint8_t close();
    int32_t compare_full_path(const file_reference_record *b) const;
    uint8_t create();
    uint8_t remove();
    uint8_t exists();
    uint32_t get_size();
    uint8_t get_size_by_path(uint32_t *out_size);
    uint8_t open(uint8_t mode);
    uint8_t read(void *buffer, uint32_t size);
    uint8_t seek(int32_t offset);
    uint8_t set_length(int32_t offset);
    uint8_t write(const void *buffer, uint32_t size);

private:
    file_reference_record *self;
};

}  // namespace halo::saved_games

namespace halo::saved_games {

/**
 * View over a variant_write_request record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class VariantWriteRequest {
public:
    explicit VariantWriteRequest(variant_write_request *record) : self(record) {}
    uint32_t thread_proc();

private:
    variant_write_request *self;
};

}  // namespace halo::saved_games

namespace halo::saved_games::control_profile {

void clear_device_slot_mappings(saved_player_profile *profile);
void fill_default_gamepad_slots(saved_player_profile *profile);
uint8_t finalize_slot(saved_player_profile *profile, int32_t gamepad_index);
uint8_t find_or_create_gamepad_slot(controls_gamepad_record *source, saved_player_profile *profile);
uint8_t is_customized(saved_player_profile *profile, int32_t gamepad_index);
void reestablish_device_slot_mappings(saved_player_profile *profile);
void reset_slot(saved_player_profile *profile, int32_t gamepad_index);
void variant_write_wait_and_clear(void);

}  // namespace halo::saved_games::control_profile

namespace halo::saved_games::directory {

void ensure_empty(const char *directory_path);

}  // namespace halo::saved_games::directory

namespace halo::saved_games::file_enumerate {

uint8_t find_next(file_reference_record *out_entry, uint32_t *out_write_time);
void start(uint32_t flags, file_reference_record *ref);

}  // namespace halo::saved_games::file_enumerate

namespace halo::saved_games::file_reference {

file_reference_record *init(file_reference_record *ref, const char *component, uint8_t is_file);

}  // namespace halo::saved_games::file_reference

namespace halo::saved_games::checkpoint {

int32_t enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, checkpoint_enumerate_proc callback,
    void *user_data);
uint8_t get_next_filename(char *out_name, char *directory);
uint8_t print_list_entry(int32_t index, const char *name, int32_t level_index, int32_t difficulty,
    int32_t game_time_ticks, const win32_systemtime *time, void *user_data);
int16_t read_stats_file(int32_t *out_difficulty, const char *name, int32_t *out_game_time, win32_systemtime *out_time);
uint8_t reclaim_slot_callback(int32_t index, const char *name, int32_t level_index, int32_t difficulty,
    int32_t game_time_ticks, const win32_systemtime *time, void *user_data);
uint8_t save_new(void);
void write_stats_file(char *scenario_name, int32_t difficulty);
int32_t compare(const checkpoint_file_entry *a, const checkpoint_file_entry *b);

}  // namespace halo::saved_games::checkpoint

namespace halo::saved_games::game_state {

void after_load_restore_time(void);
void *allocate_buffer(int32_t cpu_size, int32_t extra_size);
void build_header(void);
void create_persistent_storage_file(void);
void dispatch_load_callbacks(void);
void load_checkpoint(void);
void load_core(char *name);
data_array *make(const char *name, int16_t maximum_count, int16_t element_size);
memory_pool *new_pool(const char *name, int32_t pool_size);
void *open_persistent_storage(char *name);
void perform_revert(void);
void perform_save(uint8_t is_checkpoint);
uint8_t queue_write(uint8_t final_flag);
uint8_t read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty, char *out_scenario_name);
uint8_t read_persistent_storage(void);
void read_persistent_storage_block(int32_t size, void *buffer);
void read_profile_file(char *name, int32_t size, void *buffer);
uint8_t read_profile_header(char *name, int32_t size, void *buffer);
void save_thread_proc(void);
void startup(void);
void write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size, int32_t total_size);
uint8_t write_profile_file(int32_t size, char *name, const void *buffer);

}  // namespace halo::saved_games::game_state

namespace halo::saved_games::variant {

void write_request_start(int32_t handle, game_variant *variant);

}  // namespace halo::saved_games::variant

namespace halo::saved_games::path {

void append_component(char *destination, const char *component);
void append_extension(char *destination, const char *suffix);
void build_full(char *source, char *destination, int16_t location);
void remove_last_component(char *path);
void split_components(char **dir_start_out, char *path, char **ext_fallback_out, char **name_end_out,
    char **ext_start_out, uint8_t split_extension);

}  // namespace halo::saved_games::path

namespace halo::saved_games::player_color {

float *get_rgb(float *out_rgb, int32_t color_index);

}  // namespace halo::saved_games::player_color

namespace halo::saved_games::player_profile {

uint32_t copy_files(const char *source_dir, char *dest_dir);
void mark_level_visited_and_select(int16_t local_player_index);
uint8_t rename(int32_t handle, uint16_t *new_name);
void select_local_slot(int16_t local_player_index);
void verify_thread_wait_and_clear(void);
void write_default_files(void);

}  // namespace halo::saved_games::player_profile

namespace halo::saved_games::playlist_profile {

void create_default_profiles_on_disk(void);

}  // namespace halo::saved_games::playlist_profile

namespace halo::saved_games::saved_game {

void allocate_new_slot(uint16_t *out_name);
int32_t check_storage_availability(void);
uint8_t copy_files_to_target(char *source_directory, const char *source_name, const char *target_name);
uint32_t create_custom_variant(uint32_t unused, uint16_t *name);
uint32_t create_default_profile(uint16_t *name);
uint32_t create_slot(uint16_t type, uint16_t *name);
void delete_by_display_name(const char *name);
uint8_t delete_by_handle(int32_t handle);
uint8_t delete_files(char *name);
void enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count);
uint8_t file_exists(const char *name);
void files_dispose(void);
void files_initialize(void);
int32_t find_by_name(char *name, int16_t type);
uint8_t get_directory_by_handle(int32_t handle, char *out_directory);
uint16_t *get_display_name(int32_t handle);
uint8_t get_variant(int32_t handle, game_variant *out);
uint8_t index_open_for_write(void);
int16_t index_register_default_playlists(void);
int16_t index_register_default_profiles(void);
void last_mp_map_clear(const void *data);
uint8_t last_mp_map_read(uint8_t *out_data);
void last_mp_variant_clear(const void *data);
uint8_t last_mp_variant_read(uint8_t *out_data);
void last_profile_clear(const void *data);
uint8_t last_profile_read(uint8_t *out_data);
void list_rebuild_index(void);
uint8_t load_checkpoint(char *name);
uint8_t load_checkpoint_by_name(const char *name);
uint8_t name_is_available(const uint16_t *name);
uint8_t open_file_by_handle(int32_t handle, file_reference_record *out_ref);
uint8_t validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer, uint32_t *expected_crc,
    uint8_t *corrupt_flag);
uint8_t verify_version_and_checksum(game_state_header *header, uint8_t report_error);
void report_last_error(void);

}  // namespace halo::saved_games::saved_game

