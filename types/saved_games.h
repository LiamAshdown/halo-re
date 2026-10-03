// Blam saved_games module (halo.exe 1.0.10 retail, 0x537f70..0x556170, 116 functions).
// Four layers live in this range:
//   - game_state: the 0x440000-byte game-state arena (game_state_new / game_state_new_pool
//     carve data_array / memory_pool blocks out of it), its 0x14c-byte header, the async
//     writer thread that flushes it to savegame.bin, and the core save/load debug path.
//   - checkpoints: the checkpoints\*.sav text companions (level, difficulty, tick, date) and
//     the enumerate / sort / reclaim / copy logic around them.
//   - saved game files: player profiles (blam.sav, 0x1ffc bytes + crc) and game variants /
//     playlists (blam.lst, 0x98 bytes + crc) under savegames\, the 0x206-byte index records,
//     the default profiles and default playlists, the control-binding editors.
//   - files: the file_reference helpers (0x5554c0..0x556170), path builders and the
//     recursive directory enumerator.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries
// the layout it is used in preference to the decompiler, and the fact is called out:
//   - The game-state header size is the literal 0x14c that game_state_startup reserves and
//     folds into the allocation crc, and the 0x53-dword memset in game_state_build_header.
//     The file checksum at +0x148 is the lea eax,[ecx+0x148] the save thread hands
//     game_state_write_persistent_storage (0x538a41), and the pointer both checkpoint
//     readers pass saved_game_validate_crc (0x538290, 0x538328).
//   - The checkpoint entry size is the qsort width 0x48 and the GlobalAlloc of 0x1cb0
//     (0x66 entries) in game_checkpoint_enumerate_files.
//   - The profile body size is the crc length 0x1ffc used by every profile reader and writer
//     and the 0x7ff-dword copies; the variant body is the crc length 0x98, which is exactly
//     sizeof(game_variant) from types/game.h. The crc dword sits right after either body.
//   - The profile control-binding layout comes from the three disjoint setters in
//     control_profile_set_binding (0x53ae10) and the matching fills in
//     player_profile_initialize / control_profile_reset_slot, and the five carry-over copies
//     in player_profile_initialize (0x93c, 0x110, 0x108, 0x10b and 0x880 bytes) fix the
//     block boundaries 0x12c / 0xa68 / 0xb78 / 0xc80 / 0xd8b and 0x1108 / 0x1988.
//   - The file_reference layout is the 0x43-dword memset (0x10c) plus the fixed +0x108 handle
//     offset every file_reference_* helper uses.
//   - The index record layout is the stack frames of saved_game_create_slot,
//     saved_game_list_rebuild_index and the two default registrars, which build one record
//     and hand the whole 0x206 bytes to the index writer, and the handle packer 0x53e630.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h      data_array (game_state_new), memory_pool (game_state_new_pool)
//   types/game.h        game_variant (the blam.lst body), win32_find_dataa,
//                       savegame_index_record (opaque 0x206 view of saved_game_index_entry)
//   types/networking.h  network_mutex_record, network_thread_record (mutex_create /
//                       network_thread_create hand back pointers into their tables)
//   types/interface.h   controls_gamepad_record (the four profile gamepads at +0x1108),
//                       input_guid (its first 16 key bytes)
//   types/hs.h          file_reference (opaque 0x10c; file_reference_record below is its
//                       field-level layout -- see the note at that struct)
// So a translation unit needs tags.h, memory.h, math.h, game.h, networking.h and
// interface.h before this header, as out/phase4/saved_games_smoke.c does.
//
// Sizes: like types/memory.h, structs holding pointers only measure to the documented size
// under a 32-bit data organization.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module-wide constants
// ---------------------------------------------------------------------------
typedef enum saved_games_limits {
    k_game_state_cpu_size = 0x400000,           // game_state_startup pushes it
    k_game_state_extra_size = 0x40000,          // ... and passes this in ECX; the buffer is the sum
    k_game_state_size = 0x440000,               // bytes the writer thread flushes and the crc covers
    k_game_state_file_size = 0x480000,          // savegame.bin is pre-sized to this
    k_game_state_file_initial_block = 0x4000,   // zero block written when the file is created
    k_game_state_write_chunk_size = 0x4000,     // save thread WriteFile granularity
    k_game_state_crc_chunk_size = 0x20000,      // saved_game_validate_crc read granularity
    k_game_state_header_size = 0x14c,
    k_game_state_after_load_proc_count = 13,    // table at 0x0069e7b4
    k_game_state_build_version_length = 0xe,    // 13 characters plus the terminator

    k_maximum_checkpoint_slots = 100,           // checkpoints\checkpoint0 .. checkpoint99
    k_maximum_checkpoint_files = 0x66,          // enumerator array capacity
    k_checkpoint_name_length = 0x20,

    k_saved_player_profile_size = 0x1ffc,
    k_saved_player_profile_version = 9,         // player_profile_get rejects any other byte 0
    k_saved_player_profile_file_size = 0x2000,  // body plus crc
    k_game_variant_file_size = 0x2000,          // 0x98 body, crc, then padding: every writer and
                                                // reader moves 0x2000 bytes (mov esi,0x2000)
    k_player_profile_name_length = 12,          // 11 wide characters plus the terminator
    k_game_variant_name_length = 24,            // 23 wide characters plus the terminator
    k_maximum_local_player_profiles = 1,        // the slot selectors accept only index 0
    k_default_player_profile_count = 2,         // 00.sav and 01.sav
    k_default_game_variant_count = 0x26,        // table at 0x0069e838, one blam.lst each
    k_player_color_count = 18,                  // player_color_get_rgb clamps to 0..0x11
    k_campaign_level_count = 10,                // the per-level progress bytes

    k_maximum_saved_games = 999,                // handle index bound (< 999) and auto-name loop limit
    k_maximum_saved_game_index_count = 0x3e7,   // index writers stop once the count passes 0x3e6
    k_saved_game_minimum_free_disk_space = 0x2800000,
    k_saved_game_path_length = 0x100,
    k_saved_game_display_name_length = 0x80,
    k_saved_game_index_entry_size = 0x206,

    k_file_reference_path_length = 0x100,
    k_file_enumeration_maximum_depth = 8        // handles seeded to -1 at 0x0069fb60, UNSURE
} saved_games_limits;

// saved_game_index_entry::type and the low nibble of a saved-game handle
typedef enum saved_game_type {
    _saved_game_type_none = -1,                 // stored as 0xffff for unrecognised directories
    _saved_game_type_player_profile = 0,        // blam.sav, body k_saved_player_profile_size
    _saved_game_type_game_variant = 1           // blam.lst, body sizeof(game_variant)
} saved_game_type;

// A saved-game handle as packed by savegame_slot_handle_pack (0x53e630) and unpacked by
// saved_game_delete_by_handle / saved_game_open_file_by_handle / 0x53c600 / 0x53d080.
// -1 means none (and, for the profile slot, the built-in default profile).
typedef enum saved_game_handle_bits {
    k_saved_game_handle_type_mask = 0xf,        // saved_game_type
    k_saved_game_handle_index_shift = 16,       // index-file slot number
    k_saved_game_handle_index_mask = 0xfff,
    k_saved_game_handle_builtin_bit = 0x40000000, // entry.builtin: never passed to XDeleteSaveGame
    k_saved_game_handle_valid_bit = 0x80000000    // entry.checksum_valid
} saved_game_handle_bits;

// saved_game_check_storage_availability result
typedef enum saved_game_storage_status {
    _saved_game_storage_ok = 0,
    _saved_game_storage_low_disk_space = 1,     // under k_saved_game_minimum_free_disk_space
    _saved_game_storage_too_many_saves = 2      // more than 0x3e6 entries in savegames
} saved_game_storage_status;

// checkpoint_file_entry::kind, from the file stem. Sorting puts the larger kind first.
typedef enum checkpoint_kind {
    _checkpoint_kind_checkpoint = 0,
    _checkpoint_kind_autosave1 = 1,             // checkpoints\autosave1, the previous autosave
    _checkpoint_kind_autosave = 2               // checkpoints\autosave, the newest autosave
} checkpoint_kind;

// saved_player_profile::flags
typedef enum saved_player_profile_flags {
    _saved_player_profile_default_bit = 0x0001, // built by player_profile_initialize
    _saved_player_profile_flag_bit1 = 0x0002,   // 0x53b9b0 only deletes profiles with this set,
                                                // UNSURE (device-generated profile?)
    _saved_player_profile_end_credits_reached_bit = 0x0004, // or BYTE [0x712ef4],4 when the
                                                // end credits load (0x4c8d45 in
                                                // credits_load_directly_for_endgame, 0x481415);
                                                // 0x4c69c0 tests it to unlock console context 0x20
    k_saved_player_profile_default_index_shift = 8 // high byte: default profile index
} saved_player_profile_flags;

// control-binding values. Every binding slot holds an input action index, or this.
typedef enum control_binding_constants {
    k_control_binding_unbound = 0x7fff,
    k_control_keyboard_key_count = 0x6d,        // control_profile_set_binding accepts 0..0x6c
    k_control_mouse_button_count = 8,
    k_control_mouse_axis_count = 3,
    k_control_gamepad_count = 4,
    k_control_gamepad_button_count = 32,
    k_control_gamepad_axis_count = 32,
    k_control_gamepad_pov_count = 16,
    k_control_gamepad_pov_direction_count = 8
} control_binding_constants;

// control_binding_descriptor::device_type
typedef enum control_device_type {
    _control_device_keyboard = 1,
    _control_device_mouse = 2,
    _control_device_gamepad = 3
} control_device_type;

// control_binding_descriptor::input_kind (mouse uses button and axis only)
typedef enum control_input_kind {
    _control_input_button = 0,
    _control_input_axis = 1,
    _control_input_pov = 2
} control_input_kind;

// file_reference_record::flags and file_reference_open modes
typedef enum file_reference_flags {
    _file_reference_is_file_bit = 0x01          // clear: the reference names a directory
} file_reference_flags;

typedef enum file_reference_open_mode {
    _file_open_read = 0x1,                      // GENERIC_READ
    _file_open_write = 0x2,                     // GENERIC_WRITE
    _file_open_append = 0x4                     // seek to the end after opening
} file_reference_open_mode;

// file_reference_record::location, the path_build_full (0x5560d0) selector
typedef enum file_reference_location {
    _file_location_relative = -1,               // anything < 1: prefixed with dot-backslash unless the
                                                // path already has a drive or a leading backslash
    _file_location_root = 1,                    // prefixed with the root template at 0x0069fa50
    _file_location_absolute = 2                 // copied as is; every module caller uses this
} file_reference_location;

// file_enumeration_flags (0x0069fa58)
typedef enum file_enumeration_flags {
    _file_enumeration_recursive_bit = 0x1,
    _file_enumeration_directories_bit = 0x2     // return directories instead of files
} file_enumeration_flags;

typedef enum file_reference_signature {
    k_file_reference_signature = 0x66696c6f     // filo
} file_reference_signature;

// ---------------------------------------------------------------------------
// callbacks
// ---------------------------------------------------------------------------
// game_state_before_save_proc / game_state_revert_proc / game_state_after_load_procs[13]
typedef void (*game_state_proc)(void);

// the game_checkpoint_enumerate_files callback (0x538ac0 reclaims a slot, 0x539110 prints
// the list). A nonzero return counts the entry. level_index is pushed as a full dword.
struct win32_systemtime;
typedef uint8_t (*checkpoint_enumerate_proc)(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time, const struct win32_systemtime *time, void *user_data);

// one row of the 0x26 built-in variant builders at 0x0069e838 (the first is
// game_engine_variant_defaults_classic_slayer 0x463c40). The out buffer is pushed and the
// same pointer comes back in EAX.
typedef game_variant *(*game_variant_defaults_proc)(game_variant *out);

// ---------------------------------------------------------------------------
// win32_systemtime  (0x10 bytes)
// Not a Blam type: the Win32 SYSTEMTIME that game_checkpoint_write_stats_file gets from
// GetLocalTime and game_checkpoint_read_stats_file rebuilds from the stats file lines
// (month,day,year and hour,minute,second; day_of_week and milliseconds stay 0).
// ---------------------------------------------------------------------------
typedef struct win32_systemtime {
    uint16_t year;             // 0x00
    uint16_t month;            // 0x02
    uint16_t day_of_week;      // 0x04 never read back, written 0
    uint16_t day;              // 0x06
    uint16_t hour;             // 0x08
    uint16_t minute;           // 0x0a
    uint16_t second;           // 0x0c
    uint16_t milliseconds;     // 0x0e written 0 by the reader
} win32_systemtime;            // size 0x10

// ---------------------------------------------------------------------------
// game_state_header  (0x14c bytes at the start of the game-state arena)
// game_state_startup reserves it first, game_state_build_header fills it, and
// saved_game_verify_version_and_checksum (0x538430) checks it field by field on load.
// ---------------------------------------------------------------------------
typedef struct game_state_header {
    uint32_t allocation_checksum;  // 0x000 copy of game_state_crc (0x006e2dd4), the running crc
                                   //       of every game_state_new size; must match on load
    char scenario_name[0x100];     // 0x004 tag path of the scenario (tag_instances[
                                   //       global_scenario_index] +0x10); compared with strcmp
    char build_version[k_game_state_build_version_length];
                                   // 0x104 01.00.10.0621 when written; the loader also accepts
                                   //       the eight 01.00.03.0606 .. 01.00.10.0620 strings
    uint8_t unknown_112[0x12];     // 0x112 zeroed, never written or read
    int16_t local_player_count;    // 0x124 from 0x006894b8, the loop bound of the local player
                                   //       slot scans (UNSURE name); must match on load
    int16_t difficulty;            // 0x126 game globals (0x006b0b80) +0x0e; the checkpoint loader
                                   //       requires it to equal the pending difficulty 0x00696564
    uint32_t map_checksum;         // 0x128 cache_file_current_header.crc32 (0x006a81b8); must match
    uint8_t unknown_12c[0x1c];     // 0x12c zeroed, never written or read
    uint32_t file_checksum;        // 0x148 crc32 of the whole k_game_state_size image, computed
                                   //       with this field zeroed (game_state_write_persistent_storage)
} game_state_header;               // size 0x14c

// ---------------------------------------------------------------------------
// checkpoint_file_entry  (0x48 bytes; game_checkpoint_enumerate_files builds up to 0x66 in a
// GlobalAlloc buffer, qsorts them with saved_game_checkpoint_compare and hands each one to
// the callback)
// The three numbers are the first line of the checkpoint .sav text file written by
// game_checkpoint_write_stats_file: level index, difficulty, game tick. The disassembly of
// the writer (0x538bdc..0x538c07) pushes tick, difficulty, then reuses the stack slots so
// fprintf sees (level, difficulty, tick); the reader returns the level, stores the
// difficulty through EBX and the tick through its second argument.
// ---------------------------------------------------------------------------
typedef struct checkpoint_file_entry {
    int16_t level_index;       // 0x00 campaign level index of the scenario (0x4c8b90), -1 unknown
    uint8_t pad_02[2];         // 0x02
    int32_t game_time;         // 0x04 game tick (game_time_globals +0x0c) when the save was made
    int32_t difficulty;        // 0x08 0..3, the value saved_game_load_checkpoint_by_name applies
    win32_systemtime time;     // 0x0c local wall-clock time of the save
    uint32_t last_write_time[2]; // 0x1c FILETIME of the .sav file, secondary sort key
    int32_t kind;              // 0x24 checkpoint_kind, primary sort key (larger first)
    char name[k_checkpoint_name_length]; // 0x28 file stem after the last backslash
} checkpoint_file_entry;       // size 0x48

// ---------------------------------------------------------------------------
// saved_player_profile  (0x1ffc bytes; the blam.sav body)
// Built by player_profile_initialize (0x53a1c0), read by player_profile_get (0x53a770),
// written by player_profile_write_data (0x53a950). The same body is the stack record of the
// UI carousel (types/interface.h profile_carousel_slot) and of the working copy at
// 0x00714e80 (types/interface.h saved_item_working_copy).
// Block boundaries are the five copies player_profile_initialize makes when it merges an
// existing profile (param_3): 0x12c..0xa68 controls, 0xa68..0xb78 video, 0xb78..0xc80 audio,
// 0xc80..0xd8b a further settings block, 0x1108..0x1988 gamepads. Identity/progress
// (0x000..0x12c) and network (0xd8c..0x1108) are not carried over.
// ---------------------------------------------------------------------------
typedef struct saved_player_profile {
    uint8_t version;               // 0x000 k_saved_player_profile_version
    uint8_t pad_001;               // 0x001
    uint16_t name[k_player_profile_name_length];
                                   // 0x002 wide display name; wcsncpy 11 then +0x18 cleared
    uint8_t unknown_01a[0x100];    // 0x01a zeroed by player_profile_initialize, never written
    int16_t player_color;          // 0x11a -1 by default; index into player_color_table,
                                   //       player_color_get_rgb clamps to 0..0x11
    uint16_t flags;                // 0x11c saved_player_profile_flags
    uint8_t campaign_progress[k_campaign_level_count];
                                   // 0x11e per level, bit n set = finished on difficulty n
                                   //       (0x539d50 sets it, 0x539e00 scans it)
    int16_t last_campaign_level;   // 0x128 level index 0x539cb0 compares and updates
    uint8_t unknown_12a[2];        // 0x12a
    // controls (0x12c .. 0xa68)
    uint8_t button_set;            // 0x12c 0 by default; UNSURE, name from types/interface.h
    uint8_t joystick_set;          // 0x12d 0 by default; UNSURE, name from types/interface.h
    uint8_t look_sensitivity;      // 0x12e 3 by default; interface maps clamp(x - 1, 0, 9)
                                   //       through its 80 / 40 look-rate tables
    uint8_t look_inverted;         // 0x12f 0, or 1 for default profile 1; 0x496060 copies it
                                   //       to the live settings +0x858, which negates look_y
                                   //       after every input update (0x48ea21..0x48ea46)
    uint8_t unknown_130;           // 0x130 0
    uint8_t look_inverted_driving; // 0x131 0; copied to the live settings +0x859 (0x4963a0):
                                   //       negates look_y while 0x48fd60 reports a driver seat
    uint8_t auto_center_look;      // 0x132 player_profile_get_flag_by_id 0x495a60 returns profile+0x132;
                                   //    game_engine_update_local_player_control gates autolevelling_ticks on it; UI
                                   //    0x4a0fb0 5th row
    uint8_t unknown_133;           // 0x133 0
    int16_t keyboard_bindings[k_control_keyboard_key_count];
                                   // 0x134 action per key; control_profile_reset_digital_bindings
                                   //       fills 0x7fff then 21 defaults
    int16_t mouse_button_bindings[k_control_mouse_button_count];
                                   // 0x20e action per mouse button
    int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];
                                   // 0x21e action per mouse axis, [0] direction 1, [1] direction 2
    int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count];
                                   // 0x22a action per gamepad button
    int16_t gamepad_action_buttons[k_control_gamepad_count][2];
                                   // 0x32a the BUTTON index bound to action 8 ([0]) and action 9
                                   //       ([1]), -1 when unbound; binding one clears the other
                                   //       if it names the same button
    int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];
                                   // 0x33a action per gamepad axis, [0] direction 1, [1] direction 2
    int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count];
                                   // 0x53a action per pov hat direction
    uint8_t unknown_93a[2];        // 0x93a never written
    float forward_rate;            // 0x93c 1.0 by default; 0x496060 copies 0x93c..0x950
                                   //       verbatim to the live settings +0x810..+0x824:
                                   //       digital throttle_x step per tick
    float strafe_rate;             // 0x940 1.0; digital throttle_y step (settings +0x814)
    float look_x_rate;             // 0x944 0.1885; digital look_x step (settings +0x818)
    float look_y_rate;             // 0x948 0.1885; digital look_y step (settings +0x81c)
    float mouse_forward_scale;     // 0x94c 128.0; mouse delta divisor for forward/backward
                                   //       bindings (settings +0x820)
    float mouse_strafe_scale;      // 0x950 128.0; mouse delta divisor for left/right bindings
                                   //       (settings +0x824)
    uint8_t mouse_look_x_sensitivity; // 0x954 3; slider index: 0x496060 maps min(x, 9) through
                                   //       its 0.1..4.0 table into the live settings +0x828
    uint8_t mouse_look_y_sensitivity; // 0x955 3; same table, into settings +0x82c
    uint8_t gamepad_rate_a[k_control_gamepad_count];
                                   // 0x956 3 each; per gamepad, copied with the gamepad block by
                                   //       0x53b700; interface maps it through its 80 table
    uint8_t gamepad_rate_b[k_control_gamepad_count];
                                   // 0x95a 3 each; same, through its 40 table
    uint8_t unknown_95e[2];        // 0x95e never written
    float gamepad_axis_scale_x;    // 0x960 0.75; copied to the live settings +0x830 (0x496316)
    float gamepad_axis_scale_y;    // 0x964 0.75; copied to the live settings +0x834
    uint8_t unknown_968[0x100];    // 0x968 never written by this module
    // video (0xa68 .. 0xb78), player_profile_set_default_video_options 0x53b000
    int16_t screen_width;          // 0xa68 800 by default; 640 on the low-end / 0x007196f0 paths;
                                   //       the -vidmode value or the current display mode when set
    int16_t screen_height;         // 0xa6a 600 (480)
    int16_t refresh_rate;          // 0xa6c 60, or the -vidmode refresh
    uint8_t unknown_a6e;           // 0xa6e 2
    uint8_t frame_rate_mode;       // 0xa6f player_profile_apply_video_options 0x495580: vsync = !=0, 0x6894ba (30 fps
                                   //    lock) = ==2; video_options_menu_populate row after refresh, 0..2
    uint8_t specular;              // 0xa70 0x495580 feeds the three specular toggles (0x6893f7/f6/fa) unless
                                   //    config_disable_specular; 0x53b000 default !config_disable_specular; UI greys
                                   //    it without ps1.1
    uint8_t shadows;               // 0xa71 0x495580 -> console_debug_toggle_6893f2 only on shader version >=
                                   //    0xffff0101; UI row after specular (Halo PC video menu order); default 1 on
                                   //    capable machines
    uint8_t decals;                // 0xa72 0x495580 -> decals_for_all_responses gated on raster caps 0x6000000;
                                   //    0x53b000 default rasterizer_decal_zbias_active()
    uint8_t particles;             // 0xa73 0x495580 -> particle_systems_enabled / particle_spawn_debug_mode; UI list
                                   //    0..2; default 2, low-end !safe_mode
    uint8_t texture_quality;       // 0xa74 0x495580 maps 0/1/2 to renderer_texture_quality 2/1/0 and flushes
                                   //    texture_cache on change; UI list 0..2
    uint8_t unknown_a75;           // 0xa75 2
    int8_t gamma;                  // 0xa76 rasterizer_gamma_exponent (0x0071d1e0), 0 -> 1, -1 -> -2
    uint8_t unknown_a77[0x101];    // 0xa77 never written by this module
    // audio (0xb78 .. 0xc80)
    uint8_t master_volume;         // 0xb78 10; 0..10, x 0.1 in audio_options_apply_from_profile
    uint8_t effects_volume;        // 0xb79 10
    uint8_t music_volume;          // 0xb7a 6
    uint8_t hardware_acceleration; // 0xb7b 0x4957d0 passes (+0xb7b == 1) as arg2 of sound_driver_set_quality (effects
                                   //    object reinit); UI row shown checked only with DirectSound+EAX
    uint8_t eax_enabled;           // 0xb7c 0x4957d0 environment flag = directsound_eax_available && +0xb7c ->
                                   //    sound_driver_set_quality arg1; UI 0x4a22e0 hides the row without EAX
    uint8_t sound_quality;         // 0xb7d 0x4957d0 passes it as the quality arg of sound_driver_set_quality 0x5480f0
                                   //    (clamped 0..2 -> directsound_quality); UI list 0..2
    uint8_t unknown_b7e;           // 0xb7e 0
    uint8_t sound_variety;         // 0xb7f 0x4957d0 sound_permutation_limit = +0xb7f; UI list 0..2; default 2 fast
                                   //    machine else 1
    uint8_t unknown_b80[0x100];    // 0xb80 never written by this module
    // 0xc80 .. 0xd8b, carried over as one 0x10b-byte block
    uint8_t server_browser_sort_column; // 0xc80 server_browser_closed_event 0x4b7920 stores
                                        //    server_browser_sort_column (0x719489) at +0xc80
    uint8_t server_browser_sort_ascending; // 0xc81 0x4b7920 stores server_browser_sort_ascending (0x6953f8) at +0xc81
    uint8_t server_browser_allow_password; // 0xc82 0x4b7920 stores server_browser_allow_password (0x6953f9) at +0xc82
    uint8_t server_browser_dedicated_only; // 0xc83 0x4b7920 stores server_browser_filter_dedicated_only (0x71948b) at
                                           //    +0xc83
    uint8_t server_browser_classic_only; // 0xc84 0x4b7920 stores server_browser_filter_classic_only (0x71948c) at
                                         //    +0xc84
    uint8_t server_browser_allow_unknown_map; // 0xc85 0x4b7920 stores server_browser_filter_allow_unknown_map
                                              //    (0x71948d) at +0xc85
    uint8_t server_browser_allow_empty; // 0xc86 0x4b7920 stores server_browser_allow_empty at +0xc86 (default 1)
    uint8_t server_browser_allow_full; // 0xc87 0x4b7920 stores server_browser_allow_full at +0xc87 (default 1)
    uint8_t server_browser_game_type; // 0xc88 0x4b7920 stores server_browser_filter_gametype (0x71948e) at +0xc88
    uint8_t server_browser_team_play; // 0xc89 0x4b7920 stores server_browser_filter_teamplay at +0xc89
    uint8_t server_browser_ping_limit; // 0xc8a 0x4b7920 stores server_browser_filter_ping_limit_index at +0xc8a
    uint8_t unknown_c8b[0x100];    // 0xc8b never written by this module
    uint8_t unknown_d8b;           // 0xd8b outside every copy
    // network (0xd8c .. 0x1108), player_profile_set_default_server_options (0x53a150) writes the same defaults
    uint16_t server_name[0x90];    // 0xd8c wide, Halo by default; the UI wcscpys it to 0x00719170.
                                   //       Length UNSURE: 0x120 bytes run up to the password
    uint16_t server_password[9];   // 0xeac wide, empty by default (8 characters plus terminator)
    uint8_t unknown_ebe;           // 0xebe 0
    uint8_t server_maximum_players_index; // 0xebf ui_network_host_setup_defaults_init 0x4a2ad0: clamped to row count
                                          //    of the +0xfc0 choice, sv_maxplayers_value = table_0x65bf74[it];
                                          //    0x4a2f10 saves it
    uint8_t unknown_ec0[0x100];    // 0xec0 never written by this module
    uint8_t connection_type;       // 0xfc0 0x4a2ad0/0x4a39e0/0x4a2f10/0x4a3960: a 0..4 spinner choice that selects
                                   //    the max-player table row (Halo PC network setup connection type)
    uint8_t unknown_fc1;           // 0xfc1
    uint16_t join_server_address[0x20]; // 0xfc2 ui_network_client_connect_and_save 0x4a4a30 wcscpys the host-address
                                        //    edit field 0x719238 (32 chars) here before connecting
                                   //       name field (0x00719238) here
    uint16_t server_port;          // 0x1002 2302 (0x8fe)
    uint16_t client_port;          // 0x1004 2303 (0x8ff)
    uint8_t unknown_1006[0x102];   // 0x1006 never written by this module
    // gamepads (0x1108 .. 0x1988)
    controls_gamepad_record gamepads[k_control_gamepad_count];
                                   // 0x1108 stride 0x220, a nonzero first word marks a used slot;
                                   //       product_guid is the instance guid 0x53b500 passes
                                   //       to input_device_default_profile_tag_find, and
                                   //       0x53b6b0 matches product_instance then product_guid
    uint8_t unknown_1988[0x674];   // 0x1988 never written by this module
} saved_player_profile;            // size 0x1ffc

// blam.sav on disk: player_profile_get compares the dword after the body with a crc32 of it
typedef struct saved_player_profile_file {
    saved_player_profile profile;  // 0x0000
    uint32_t checksum;             // 0x1ffc crc32 (seed -1) of profile
} saved_player_profile_file;       // size 0x2000

// blam.lst on disk (types/game.h game_variant is the body; its variant_flags word (+0x94) carries the
// same default bit 0 and default index in the high byte as saved_player_profile::flags,
// written by saved_game_create_custom_variant and playlist_profile_create_default_profiles_
// on_disk). Like blam.sav the file is 0x2000 bytes: saved_game_create_slot (0x53c8a0),
// playlist_profile_create_default_profiles_on_disk (0x53be91), game_variant_write_thread_proc
// (0x53c1f9) and saved_game_create_custom_variant (0x53bc31) all write 0x2000 bytes, and
// saved_game_get_variant (0x53bf8b), saved_game_list_rebuild_index and
// saved_game_index_register_default_playlists all read 0x2000. Only the first 0x9c bytes mean
// anything; the default-playlist writer does not even initialize the tail (stack garbage).
typedef struct game_variant_file {
    game_variant variant;          // 0x0000
    uint32_t checksum;             // 0x0098 crc32 (seed -1) of variant
    uint8_t padding_09c[0x2000 - 0x9c]; // 0x009c zeroed by saved_game_create_slot and
                                   //        saved_game_create_custom_variant, else undefined
} game_variant_file;               // size 0x2000

// ---------------------------------------------------------------------------
// saved_player_profile_slot  (0x2004 bytes, one at 0x00712dd8)
// The loaded local profile. FUN_00539cb0 / FUN_00539d50 index it as slot * 0x801 dwords and
// accept slot 0 only. types/interface.h already lists +0x1ffc as current_profile_index.
// ---------------------------------------------------------------------------
typedef struct saved_player_profile_slot {
    saved_player_profile profile;  // 0x0000
    int32_t handle;                // 0x1ffc saved-game handle of the profile, -1 for the default
    uint8_t unknown_2000;          // 0x2000 byte flag the interface writes (0x49d186, and per
                                   //       slot at 0x4a1852) and reads (0x4a5b2b)
    uint8_t pad_2001[3];           // 0x2001
} saved_player_profile_slot;       // size 0x2004

// ---------------------------------------------------------------------------
// variant_write_request  (0x9c bytes at 0x00721288)
// The parameter block FUN_0053c0b0 fills and hands network_thread_create; the thread proc
// FUN_0053c150 writes variant to the file of handle. Zeroed (0x29 dwords, which also covers
// the thread pointer and the default-variant counter after it) by FUN_0053bae0 and
// saved_game_files_initialize.
// ---------------------------------------------------------------------------
typedef struct variant_write_request {
    int32_t handle;                // 0x00 saved-game handle
    game_variant variant;          // 0x04 copied in as 0x26 dwords
} variant_write_request;           // size 0x9c

// ---------------------------------------------------------------------------
// saved_game_index_entry  (0x206 bytes)
// Field-level layout of the record types/game.h keeps opaque as savegame_index_record: one
// per savegames\ subdirectory, flat in the index file savegame_index_file (0x00721330).
// ---------------------------------------------------------------------------
typedef struct saved_game_index_entry {
    char path[k_saved_game_path_length];   // 0x000 full path of blam.sav / blam.lst
    uint16_t display_name[k_saved_game_display_name_length];
                                   // 0x100 wide; wcsncpy 0x7f, +0x1fe cleared
    int16_t type;                  // 0x200 saved_game_type
    int16_t index;                 // 0x202 its own slot number in the index file
    uint8_t builtin;               // 0x204 1 for the default profiles / playlists; packed into
                                   //       handle bit 30, which stops XDeleteSaveGame
    uint8_t checksum_valid;        // 0x205 the file crc matched; handle bit 31
} saved_game_index_entry;          // size 0x206 == k_savegame_index_record_size

// ---------------------------------------------------------------------------
// xgame_find_data  (0x344 bytes)
// Not a Blam type: the XGAME_FIND_DATA shape the Xbox save API emulation fills.
// savegame_find_first (0x551bc0, game module) copies the directory to +0x140, and
// saved_game_list_rebuild_index reads the directory at +0x140 and the wide name at +0x244.
// ---------------------------------------------------------------------------
typedef struct xgame_find_data {
    win32_find_dataa find_data;    // 0x000
    char save_game_directory[0x104];   // 0x140 ends with a backslash
    uint16_t save_game_name[0x80]; // 0x244 wide display name
} xgame_find_data;                 // size 0x344

// ---------------------------------------------------------------------------
// file_reference_record  (0x10c bytes)
// The field layout of the files-module record types/hs.h declares opaque as file_reference.
// It is NOT a second definition of that name: hs.h owns the name, and this struct describes
// the same bytes so the file_reference_* helpers can name their fields. Every constructor
// in the module (file_reference_init 0x5554c0, saved_game_open_file_by_handle, the last*.txt helpers, the
// default-file writers) zeroes 0x43 dwords, stores the signature, location 2, appends the
// path and sets the is-file bit.
// ---------------------------------------------------------------------------
typedef struct file_reference_record {
    uint32_t signature;            // 0x000 k_file_reference_signature
    uint8_t flags;                 // 0x004 file_reference_flags; file_reference_create and
                                   //       file_reference_delete branch on bit 0
    uint8_t unknown_005;           // 0x005 zeroed, never read
    int16_t location;              // 0x006 file_reference_location; file_enumerate_find_next
                                   //       copies it from the enumeration root
    char path[k_file_reference_path_length];
                                   // 0x008 path_append_component bounds it at 0xff
    void *handle;                  // 0x108 Win32 HANDLE from file_reference_open, 0 when closed
} file_reference_record;           // size 0x10c

// ---------------------------------------------------------------------------
// file_enumeration_position  (0x04 bytes, global 0x0069fa5c)
// The depth / location pair is one dword in the binary (depth low word, location high word),
// written jointly by file_enumerate_start and file_enumerate_find_next.
// ---------------------------------------------------------------------------
typedef struct file_enumeration_position {
    int16_t depth;                 // 0x00 0x0069fa5c, -1 when idle
    int16_t location;              // 0x02 0x0069fa5e, file_reference_location of the root
} file_enumeration_position;       // size 0x04

// ---------------------------------------------------------------------------
// win32_file_attribute_data  (0x24 bytes)
// Not a Blam type: WIN32_FILE_ATTRIBUTE_DATA, the GetFileExInfoStandard block
// file_reference_get_size_by_path hands GetFileAttributesExA.
// ---------------------------------------------------------------------------
typedef struct win32_file_attribute_data {
    uint32_t file_attributes;      // 0x00
    uint32_t creation_time[2];     // 0x04 FILETIME
    uint32_t last_access_time[2];  // 0x0c FILETIME
    uint32_t last_write_time[2];   // 0x14 FILETIME
    uint32_t file_size_high;       // 0x1c
    uint32_t file_size_low;        // 0x20
} win32_file_attribute_data;       // size 0x24

// ---------------------------------------------------------------------------
// control_binding_descriptor  (0x0c bytes)
// The record control_profile_find_binding_for_action (0x53aa20) fills and control_profile_clear_binding /
// control_profile_set_binding read in ESI; the interface keeps it as int16_t record[6].
// ---------------------------------------------------------------------------
typedef struct control_binding_descriptor {
    int16_t device_type;           // 0x00 control_device_type
    int16_t device_index;          // 0x02 gamepad 0..3
    int16_t input_kind;            // 0x04 control_input_kind
    int16_t input_index;           // 0x06 key, button, axis or pov index
    int32_t direction;             // 0x08 axis: 1 or 2 (selects [0] or [1]); pov: 0..7
} control_binding_descriptor;      // size 0x0c

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// game state (individual globals; other modules already extern the first three by these names)
// global 0x006e2dc8: uint8_t *game_state_base                 == map_memory (0x006ac548)
// global 0x006e2dcc: int32_t game_state_cursor                bytes carved so far
// global 0x006e2dd0: uint32_t unknown_006e2dd0                never referenced
// global 0x006e2dd4: uint32_t game_state_crc                  running crc of the allocation sizes
// global 0x006e2dd8: uint8_t game_state_header_valid          set by game_state_build_header only
// global 0x006e2dd9: uint8_t game_state_revert_available      last queued save succeeded
// global 0x006e2ddc: int32_t game_state_revert_time           -1 when built, game tick after a load
//                                                             (the after-load proc 0x5385d0)
// global 0x006e2de0: game_state_header *game_state_header     == game_state_base + 0
// global 0x006e2de4: uint8_t *game_state_write_buffer         GlobalAlloc(k_game_state_size)
// global 0x006e2de8: uint8_t game_state_write_buffer_allocated
// global 0x006e2dec: uint8_t *game_state_snapshot_source      == map_memory
// global 0x006e2df0: uint32_t game_state_size                 k_game_state_size
// global 0x006e2df4: uint8_t game_state_persistent_storage_created
// global 0x006e2df5: uint8_t game_state_write_completed
// global 0x006e2df8: void *game_state_persistent_storage      savegame.bin HANDLE
// global 0x006e2dfc: char game_state_persistent_storage_path[0x100]  profile dir\savegame.bin
// global 0x006e2efc: char game_state_core_directory[0x100]   profile dir\core
// global 0x006e2ffc: void *game_state_write_event             auto-reset event of the save thread
// global 0x006e3000: uint8_t game_state_write_in_progress
// global 0x006e3001: uint8_t game_state_write_is_checkpoint   also rotate autosave / write stats
// global 0x006e3008: uint16_t saved_game_display_name_buffer[0x80]  returned by 0x53c600
// global 0x006e3108: char hdmu_map_path[0x100]                root\saved\hdmu.map
//
// game state callback tables (.data)
// global 0x0069e7ac: game_state_proc game_state_before_save_proc    0x44ad80, the shared no-op
// global 0x0069e7b0: game_state_proc game_state_revert_proc         0x543a90
// global 0x0069e7b4: game_state_proc game_state_after_load_procs[13]
// global 0x0069e7e8: uint8_t checkpoint_sort_newest_first          qsort direction for equal kinds
//
// profiles and variants (.data)
// global 0x0069e7f0: uint32_t player_color_table[18]   packed 0x00RRGGBB: white, black, red,
//                    blue, gray, yellow, green, pink, purple, cyan, cobalt, orange, teal, sage,
//                    brown, tan, maroon, salmon
// global 0x0069e838: game_variant_defaults_proc default_game_variant_procs[0x26]
//
// profiles (bss)
// global 0x00712dd8: saved_player_profile_slot saved_player_profile_slots[1]
// global 0x0071d280: saved_player_profile default_player_profile   built at init, returned for
//                    handle -1 by player_profile_get_or_cached_default (0x539bc0)
// global 0x0071f27c: uint8_t unknown_0071f27c[0x2000]  never referenced; zeroed with the default
//                    profile (0x1001 dwords from 0x0071d280 in player_profile_verify_thread_wait_and_clear / init)
// global 0x0072127c: network_thread_record *player_profile_thread  joined and cleared, never created
// global 0x00721280: uint8_t default_player_profile_initialized
// global 0x00721288: variant_write_request variant_write_request
// global 0x00721324: network_thread_record *variant_write_thread
// global 0x00721328: int16_t default_game_variant_count  blam.lst files written by 0x53bc70
// global 0x0072132a: uint8_t unknown_0072132a            set to 1 by init, never read
//
// saved game files: 0x00721330..0x00721e4c is zeroed as one 0x2c7-dword block by
// saved_game_files_initialize, which accounts for every byte between them
// global 0x00721330: file_reference savegame_index_file   (types/game.h lists it too)
// global 0x0072143c: network_mutex_record *saved_game_files_mutex   profile / file access
// global 0x00721440: network_mutex_record *savegame_index_mutex    (types/game.h: void **)
// global 0x00721444: int16_t savegame_index_write_count  -1 outside a rebuild
// global 0x00721446: uint8_t saved_game_files_initialized both mutexes created
// global 0x00721447: uint8_t savegame_index_dirty        rebuild before the next lookup
// global 0x00721448: uint8_t savegame_index_file_open
// global 0x00721449: char saved_game_root_directory[0x100]          copy of profile_directory
// global 0x00721549: char savegames_directory[0x100]                root\savegames
// global 0x00721649: char saved_directory[0x100]                    root\saved
// global 0x00721749: char player_profiles_directory[0x100]          root\saved\player_profiles
// global 0x00721849: char default_player_profiles_directory[0x100]  ...\player_profiles\default_profile
// global 0x00721949: char playlists_directory[0x100]                root\saved\playlists
// global 0x00721a49: char default_playlists_directory[0x100]        ...\playlists\default_playlist
// global 0x00721b49: char last_profile_path[0x100]                  root\lastprof.txt
// global 0x00721c49: char last_game_variant_path[0x100]             root\lastmpvr.txt
// global 0x00721d49: char last_multiplayer_map_path[0x100]          root\lastmpmp.txt
// (0x00721e49..0x00721e4c: 3 bytes of the block, never used)
//
// files (.data)
// global 0x0069fa50: char file_root_template[4]          ?:\ prefix for _file_location_root
// global 0x0069fa58: uint32_t file_enumeration_flags     file_enumeration_flags
// global 0x0069fa5c: int16_t file_enumeration_depth      -1 when idle (the .data initializer)
// global 0x0069fa5e: int16_t file_enumeration_location   location of the root reference
// global 0x0069fa60: char file_enumeration_path[0x100]   current directory of the walk
// global 0x0069fb60: void *file_enumeration_handles[8]   one FindFirstFile handle per depth,
//                    seeded to -1; 0x0069fb80..0x0069fba0 is never referenced (UNSURE whether
//                    the array is really 16 long with only 8 seeded)
// global 0x0069fba0: win32_find_dataa file_enumeration_find_data

// ---------------------------------------------------------------------------
// globals this module reads but does not own
// ---------------------------------------------------------------------------
// 0x006ac548  void *map_memory                   (cache)      the game-state arena lives here
// 0x006ac900  char profile_directory[0x105]      (cache)
// 0x006a81b8  cache_file_current_header.crc32    (cache)      header map_checksum
// 0x006894b8  int16_t local_player_count          (main/ui)   header local_player_count
// 0x006b0b80  game globals *, +0x0e difficulty    (game)
// 0x00696564  int16_t pending difficulty          (main)      applied by checkpoint loads
// 0x0069e8d4  datum_index global_scenario_index   (hs)
// 0x0087bc14  tag_instances                       (cache)     +0x10 tag path, +0x14 tag data
// 0x006f1d6c  game_time_globals *game_time        (game)      +0x0c tick
// 0x00714dd4  saved_player_profile_slots[0].handle, also named current_profile_index in
//             types/interface.h
// 0x00714e7c  selected_saved_item / 0x00714e80 the working copy (interface), which
//             control_profile_set_binding / _clear_binding edit as a saved_player_profile
// 0x00718fac  pending UI error message (interface)   set to 0x21 / 0x22 / 0x24 on slot failures
// 0x006b1844 / 0x006b1868 / 0x006b1a98 / 0x006b2ce8  input device count, table (stride 0x240,
//             first 0x220 bytes a controls_gamepad_record), device -> slot and slot -> device

#pragma pack(pop)

#include <stddef.h>
static_assert(sizeof(win32_systemtime) == 0x10, "win32_systemtime size");
static_assert(offsetof(win32_systemtime, year) == 0x00, "win32_systemtime::year");
static_assert(offsetof(win32_systemtime, month) == 0x02, "win32_systemtime::month");
static_assert(offsetof(win32_systemtime, day_of_week) == 0x04, "win32_systemtime::day_of_week");
static_assert(offsetof(win32_systemtime, day) == 0x06, "win32_systemtime::day");
static_assert(offsetof(win32_systemtime, hour) == 0x08, "win32_systemtime::hour");
static_assert(offsetof(win32_systemtime, minute) == 0x0a, "win32_systemtime::minute");
static_assert(offsetof(win32_systemtime, second) == 0x0c, "win32_systemtime::second");
static_assert(offsetof(win32_systemtime, milliseconds) == 0x0e, "win32_systemtime::milliseconds");
static_assert(sizeof(game_state_header) == 0x14c, "game_state_header size");
static_assert(offsetof(game_state_header, allocation_checksum) == 0x000, "game_state_header::allocation_checksum");
static_assert(offsetof(game_state_header, scenario_name) == 0x004, "game_state_header::scenario_name");
static_assert(offsetof(game_state_header, build_version) == 0x104, "game_state_header::build_version");
static_assert(offsetof(game_state_header, unknown_112) == 0x112, "game_state_header::unknown_112");
static_assert(offsetof(game_state_header, local_player_count) == 0x124, "game_state_header::local_player_count");
static_assert(offsetof(game_state_header, difficulty) == 0x126, "game_state_header::difficulty");
static_assert(offsetof(game_state_header, map_checksum) == 0x128, "game_state_header::map_checksum");
static_assert(offsetof(game_state_header, unknown_12c) == 0x12c, "game_state_header::unknown_12c");
static_assert(offsetof(game_state_header, file_checksum) == 0x148, "game_state_header::file_checksum");
static_assert(sizeof(checkpoint_file_entry) == 0x48, "checkpoint_file_entry size");
static_assert(offsetof(checkpoint_file_entry, level_index) == 0x00, "checkpoint_file_entry::level_index");
static_assert(offsetof(checkpoint_file_entry, pad_02) == 0x02, "checkpoint_file_entry::pad_02");
static_assert(offsetof(checkpoint_file_entry, game_time) == 0x04, "checkpoint_file_entry::game_time");
static_assert(offsetof(checkpoint_file_entry, difficulty) == 0x08, "checkpoint_file_entry::difficulty");
static_assert(offsetof(checkpoint_file_entry, time) == 0x0c, "checkpoint_file_entry::time");
static_assert(offsetof(checkpoint_file_entry, last_write_time) == 0x1c, "checkpoint_file_entry::last_write_time");
static_assert(offsetof(checkpoint_file_entry, kind) == 0x24, "checkpoint_file_entry::kind");
static_assert(offsetof(checkpoint_file_entry, name) == 0x28, "checkpoint_file_entry::name");
static_assert(sizeof(saved_player_profile) == 0x1ffc, "saved_player_profile size");
static_assert(offsetof(saved_player_profile, version) == 0x000, "saved_player_profile::version");
static_assert(offsetof(saved_player_profile, pad_001) == 0x001, "saved_player_profile::pad_001");
static_assert(offsetof(saved_player_profile, name) == 0x002, "saved_player_profile::name");
static_assert(offsetof(saved_player_profile, unknown_01a) == 0x01a, "saved_player_profile::unknown_01a");
static_assert(offsetof(saved_player_profile, player_color) == 0x11a, "saved_player_profile::player_color");
static_assert(offsetof(saved_player_profile, flags) == 0x11c, "saved_player_profile::flags");
static_assert(offsetof(saved_player_profile, campaign_progress) == 0x11e, "saved_player_profile::campaign_progress");
static_assert(offsetof(saved_player_profile, last_campaign_level) == 0x128, "saved_player_profile::last_campaign_level");
static_assert(offsetof(saved_player_profile, unknown_12a) == 0x12a, "saved_player_profile::unknown_12a");
static_assert(offsetof(saved_player_profile, button_set) == 0x12c, "saved_player_profile::button_set");
static_assert(offsetof(saved_player_profile, joystick_set) == 0x12d, "saved_player_profile::joystick_set");
static_assert(offsetof(saved_player_profile, look_sensitivity) == 0x12e, "saved_player_profile::look_sensitivity");
static_assert(offsetof(saved_player_profile, look_inverted) == 0x12f, "saved_player_profile::look_inverted");
static_assert(offsetof(saved_player_profile, unknown_130) == 0x130, "saved_player_profile::unknown_130");
static_assert(offsetof(saved_player_profile, look_inverted_driving) == 0x131, "saved_player_profile::look_inverted_driving");
static_assert(offsetof(saved_player_profile, auto_center_look) == 0x132, "saved_player_profile::auto_center_look");
static_assert(offsetof(saved_player_profile, unknown_133) == 0x133, "saved_player_profile::unknown_133");
static_assert(offsetof(saved_player_profile, keyboard_bindings) == 0x134, "saved_player_profile::keyboard_bindings");
static_assert(offsetof(saved_player_profile, mouse_button_bindings) == 0x20e, "saved_player_profile::mouse_button_bindings");
static_assert(offsetof(saved_player_profile, unknown_93a) == 0x93a, "saved_player_profile::unknown_93a");
static_assert(offsetof(saved_player_profile, forward_rate) == 0x93c, "saved_player_profile::forward_rate");
static_assert(offsetof(saved_player_profile, strafe_rate) == 0x940, "saved_player_profile::strafe_rate");
static_assert(offsetof(saved_player_profile, look_x_rate) == 0x944, "saved_player_profile::look_x_rate");
static_assert(offsetof(saved_player_profile, look_y_rate) == 0x948, "saved_player_profile::look_y_rate");
static_assert(offsetof(saved_player_profile, mouse_forward_scale) == 0x94c, "saved_player_profile::mouse_forward_scale");
static_assert(offsetof(saved_player_profile, mouse_strafe_scale) == 0x950, "saved_player_profile::mouse_strafe_scale");
static_assert(offsetof(saved_player_profile, mouse_look_x_sensitivity) == 0x954, "saved_player_profile::mouse_look_x_sensitivity");
static_assert(offsetof(saved_player_profile, mouse_look_y_sensitivity) == 0x955, "saved_player_profile::mouse_look_y_sensitivity");
static_assert(offsetof(saved_player_profile, gamepad_rate_a) == 0x956, "saved_player_profile::gamepad_rate_a");
static_assert(offsetof(saved_player_profile, gamepad_rate_b) == 0x95a, "saved_player_profile::gamepad_rate_b");
static_assert(offsetof(saved_player_profile, unknown_95e) == 0x95e, "saved_player_profile::unknown_95e");
static_assert(offsetof(saved_player_profile, gamepad_axis_scale_x) == 0x960, "saved_player_profile::gamepad_axis_scale_x");
static_assert(offsetof(saved_player_profile, gamepad_axis_scale_y) == 0x964, "saved_player_profile::gamepad_axis_scale_y");
static_assert(offsetof(saved_player_profile, unknown_968) == 0x968, "saved_player_profile::unknown_968");
static_assert(offsetof(saved_player_profile, screen_width) == 0xa68, "saved_player_profile::screen_width");
static_assert(offsetof(saved_player_profile, screen_height) == 0xa6a, "saved_player_profile::screen_height");
static_assert(offsetof(saved_player_profile, refresh_rate) == 0xa6c, "saved_player_profile::refresh_rate");
static_assert(offsetof(saved_player_profile, unknown_a6e) == 0xa6e, "saved_player_profile::unknown_a6e");
static_assert(offsetof(saved_player_profile, frame_rate_mode) == 0xa6f, "saved_player_profile::frame_rate_mode");
static_assert(offsetof(saved_player_profile, specular) == 0xa70, "saved_player_profile::specular");
static_assert(offsetof(saved_player_profile, shadows) == 0xa71, "saved_player_profile::shadows");
static_assert(offsetof(saved_player_profile, decals) == 0xa72, "saved_player_profile::decals");
static_assert(offsetof(saved_player_profile, particles) == 0xa73, "saved_player_profile::particles");
static_assert(offsetof(saved_player_profile, texture_quality) == 0xa74, "saved_player_profile::texture_quality");
static_assert(offsetof(saved_player_profile, unknown_a75) == 0xa75, "saved_player_profile::unknown_a75");
static_assert(offsetof(saved_player_profile, gamma) == 0xa76, "saved_player_profile::gamma");
static_assert(offsetof(saved_player_profile, unknown_a77) == 0xa77, "saved_player_profile::unknown_a77");
static_assert(offsetof(saved_player_profile, master_volume) == 0xb78, "saved_player_profile::master_volume");
static_assert(offsetof(saved_player_profile, effects_volume) == 0xb79, "saved_player_profile::effects_volume");
static_assert(offsetof(saved_player_profile, music_volume) == 0xb7a, "saved_player_profile::music_volume");
static_assert(offsetof(saved_player_profile, hardware_acceleration) == 0xb7b, "saved_player_profile::hardware_acceleration");
static_assert(offsetof(saved_player_profile, eax_enabled) == 0xb7c, "saved_player_profile::eax_enabled");
static_assert(offsetof(saved_player_profile, sound_quality) == 0xb7d, "saved_player_profile::sound_quality");
static_assert(offsetof(saved_player_profile, unknown_b7e) == 0xb7e, "saved_player_profile::unknown_b7e");
static_assert(offsetof(saved_player_profile, sound_variety) == 0xb7f, "saved_player_profile::sound_variety");
static_assert(offsetof(saved_player_profile, unknown_b80) == 0xb80, "saved_player_profile::unknown_b80");
static_assert(offsetof(saved_player_profile, server_browser_sort_column) == 0xc80, "saved_player_profile::server_browser_sort_column");
static_assert(offsetof(saved_player_profile, server_browser_sort_ascending) == 0xc81, "saved_player_profile::server_browser_sort_ascending");
static_assert(offsetof(saved_player_profile, server_browser_allow_password) == 0xc82, "saved_player_profile::server_browser_allow_password");
static_assert(offsetof(saved_player_profile, server_browser_dedicated_only) == 0xc83, "saved_player_profile::server_browser_dedicated_only");
static_assert(offsetof(saved_player_profile, server_browser_classic_only) == 0xc84, "saved_player_profile::server_browser_classic_only");
static_assert(offsetof(saved_player_profile, server_browser_allow_unknown_map) == 0xc85, "saved_player_profile::server_browser_allow_unknown_map");
static_assert(offsetof(saved_player_profile, server_browser_allow_empty) == 0xc86, "saved_player_profile::server_browser_allow_empty");
static_assert(offsetof(saved_player_profile, server_browser_allow_full) == 0xc87, "saved_player_profile::server_browser_allow_full");
static_assert(offsetof(saved_player_profile, server_browser_game_type) == 0xc88, "saved_player_profile::server_browser_game_type");
static_assert(offsetof(saved_player_profile, server_browser_team_play) == 0xc89, "saved_player_profile::server_browser_team_play");
static_assert(offsetof(saved_player_profile, server_browser_ping_limit) == 0xc8a, "saved_player_profile::server_browser_ping_limit");
static_assert(offsetof(saved_player_profile, unknown_c8b) == 0xc8b, "saved_player_profile::unknown_c8b");
static_assert(offsetof(saved_player_profile, unknown_d8b) == 0xd8b, "saved_player_profile::unknown_d8b");
static_assert(offsetof(saved_player_profile, server_name) == 0xd8c, "saved_player_profile::server_name");
static_assert(offsetof(saved_player_profile, server_password) == 0xeac, "saved_player_profile::server_password");
static_assert(offsetof(saved_player_profile, unknown_ebe) == 0xebe, "saved_player_profile::unknown_ebe");
static_assert(offsetof(saved_player_profile, server_maximum_players_index) == 0xebf, "saved_player_profile::server_maximum_players_index");
static_assert(offsetof(saved_player_profile, unknown_ec0) == 0xec0, "saved_player_profile::unknown_ec0");
static_assert(offsetof(saved_player_profile, connection_type) == 0xfc0, "saved_player_profile::connection_type");
static_assert(offsetof(saved_player_profile, unknown_fc1) == 0xfc1, "saved_player_profile::unknown_fc1");
static_assert(offsetof(saved_player_profile, join_server_address) == 0xfc2, "saved_player_profile::join_server_address");
static_assert(offsetof(saved_player_profile, server_port) == 0x1002, "saved_player_profile::server_port");
static_assert(offsetof(saved_player_profile, client_port) == 0x1004, "saved_player_profile::client_port");
static_assert(offsetof(saved_player_profile, unknown_1006) == 0x1006, "saved_player_profile::unknown_1006");
static_assert(offsetof(saved_player_profile, gamepads) == 0x1108, "saved_player_profile::gamepads");
static_assert(offsetof(saved_player_profile, unknown_1988) == 0x1988, "saved_player_profile::unknown_1988");
static_assert(sizeof(saved_player_profile_file) == 0x2000, "saved_player_profile_file size");
static_assert(offsetof(saved_player_profile_file, profile) == 0x0000, "saved_player_profile_file::profile");
static_assert(offsetof(saved_player_profile_file, checksum) == 0x1ffc, "saved_player_profile_file::checksum");
static_assert(sizeof(game_variant_file) == 0x2000, "game_variant_file size");
static_assert(offsetof(game_variant_file, variant) == 0x0000, "game_variant_file::variant");
static_assert(offsetof(game_variant_file, checksum) == 0x0098, "game_variant_file::checksum");
static_assert(offsetof(game_variant_file, padding_09c) == 0x009c, "game_variant_file::padding_09c");
static_assert(sizeof(saved_player_profile_slot) == 0x2004, "saved_player_profile_slot size");
static_assert(offsetof(saved_player_profile_slot, profile) == 0x0000, "saved_player_profile_slot::profile");
static_assert(offsetof(saved_player_profile_slot, handle) == 0x1ffc, "saved_player_profile_slot::handle");
static_assert(offsetof(saved_player_profile_slot, unknown_2000) == 0x2000, "saved_player_profile_slot::unknown_2000");
static_assert(offsetof(saved_player_profile_slot, pad_2001) == 0x2001, "saved_player_profile_slot::pad_2001");
static_assert(sizeof(variant_write_request) == 0x9c, "variant_write_request size");
static_assert(offsetof(variant_write_request, handle) == 0x00, "variant_write_request::handle");
static_assert(offsetof(variant_write_request, variant) == 0x04, "variant_write_request::variant");
static_assert(sizeof(saved_game_index_entry) == 0x206, "saved_game_index_entry size");
static_assert(offsetof(saved_game_index_entry, path) == 0x000, "saved_game_index_entry::path");
static_assert(offsetof(saved_game_index_entry, display_name) == 0x100, "saved_game_index_entry::display_name");
static_assert(offsetof(saved_game_index_entry, type) == 0x200, "saved_game_index_entry::type");
static_assert(offsetof(saved_game_index_entry, index) == 0x202, "saved_game_index_entry::index");
static_assert(offsetof(saved_game_index_entry, builtin) == 0x204, "saved_game_index_entry::builtin");
static_assert(offsetof(saved_game_index_entry, checksum_valid) == 0x205, "saved_game_index_entry::checksum_valid");
static_assert(sizeof(xgame_find_data) == 0x344, "xgame_find_data size");
static_assert(offsetof(xgame_find_data, find_data) == 0x000, "xgame_find_data::find_data");
static_assert(offsetof(xgame_find_data, save_game_directory) == 0x140, "xgame_find_data::save_game_directory");
static_assert(offsetof(xgame_find_data, save_game_name) == 0x244, "xgame_find_data::save_game_name");
static_assert(sizeof(file_reference_record) == 0x10c, "file_reference_record size");
static_assert(offsetof(file_reference_record, signature) == 0x000, "file_reference_record::signature");
static_assert(offsetof(file_reference_record, flags) == 0x004, "file_reference_record::flags");
static_assert(offsetof(file_reference_record, unknown_005) == 0x005, "file_reference_record::unknown_005");
static_assert(offsetof(file_reference_record, location) == 0x006, "file_reference_record::location");
static_assert(offsetof(file_reference_record, path) == 0x008, "file_reference_record::path");
static_assert(offsetof(file_reference_record, handle) == 0x108, "file_reference_record::handle");
static_assert(sizeof(file_enumeration_position) == 0x04, "file_enumeration_position size");
static_assert(offsetof(file_enumeration_position, depth) == 0x00, "file_enumeration_position::depth");
static_assert(offsetof(file_enumeration_position, location) == 0x02, "file_enumeration_position::location");
static_assert(sizeof(win32_file_attribute_data) == 0x24, "win32_file_attribute_data size");
static_assert(offsetof(win32_file_attribute_data, file_attributes) == 0x00, "win32_file_attribute_data::file_attributes");
static_assert(offsetof(win32_file_attribute_data, creation_time) == 0x04, "win32_file_attribute_data::creation_time");
static_assert(offsetof(win32_file_attribute_data, last_access_time) == 0x0c, "win32_file_attribute_data::last_access_time");
static_assert(offsetof(win32_file_attribute_data, last_write_time) == 0x14, "win32_file_attribute_data::last_write_time");
static_assert(offsetof(win32_file_attribute_data, file_size_high) == 0x1c, "win32_file_attribute_data::file_size_high");
static_assert(offsetof(win32_file_attribute_data, file_size_low) == 0x20, "win32_file_attribute_data::file_size_low");
static_assert(sizeof(control_binding_descriptor) == 0x0c, "control_binding_descriptor size");
static_assert(offsetof(control_binding_descriptor, device_type) == 0x00, "control_binding_descriptor::device_type");
static_assert(offsetof(control_binding_descriptor, device_index) == 0x02, "control_binding_descriptor::device_index");
static_assert(offsetof(control_binding_descriptor, input_kind) == 0x04, "control_binding_descriptor::input_kind");
static_assert(offsetof(control_binding_descriptor, input_index) == 0x06, "control_binding_descriptor::input_index");
static_assert(offsetof(control_binding_descriptor, direction) == 0x08, "control_binding_descriptor::direction");
