#pragma once
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
    uint8_t unknown_132;           // 0x132 never written
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
    uint8_t vsync_mode;           // 0xa6f 2 on low-end, else from the current display mode
    uint8_t specular_enabled;           // 0xa70 capability flags set from the machine class
    uint8_t shadows_enabled;           // 0xa71
    uint8_t decals_enabled;           // 0xa72 rasterizer_decal_zbias_active on high-end
    uint8_t particles_enabled;           // 0xa73
    uint8_t texture_quality;           // 0xa74
    uint8_t unknown_a75;           // 0xa75 2
    int8_t gamma;                  // 0xa76 rasterizer_gamma_exponent (0x0071d1e0), 0 -> 1, -1 -> -2
    uint8_t unknown_a77[0x101];    // 0xa77 never written by this module
    // audio (0xb78 .. 0xc80)
    uint8_t master_volume;         // 0xb78 10; 0..10, x 0.1 in audio_options_apply_from_profile
    uint8_t effects_volume;        // 0xb79 10
    uint8_t music_volume;          // 0xb7a 6
    uint8_t hardware_sound_enabled;           // 0xb7b 0; UI row gated on directsound init + EAX available (hardware acceleration toggle, UNSURE which)
    uint8_t eax_enabled;           // 0xb7c 0; boolean UI row shown only when EAX is available (environmental sound / EAX)
    uint8_t sound_quality;           // 0xb7d 1 on a fast machine else 0; UI list 0..2
    uint8_t unknown_b7e;           // 0xb7e 0
    uint8_t sound_variety;           // 0xb7f 2 on a fast machine else 1; UI list 0..2
    uint8_t unknown_b80[0x100];    // 0xb80 never written by this module
    // 0xc80 .. 0xd8b, carried over as one 0x10b-byte block
    uint8_t browser_sort_column;           // 0xc80 3
    uint8_t browser_sort_ascending;           // 0xc81 1
    uint8_t browser_allow_password;           // 0xc82 1
    uint8_t browser_filter_dedicated_only;           // 0xc83 0
    uint8_t browser_filter_classic_only;           // 0xc84 0
    uint8_t browser_filter_allow_unknown_map;           // 0xc85 0
    uint8_t browser_allow_empty;           // 0xc86 1
    uint8_t browser_allow_full;           // 0xc87 1
    uint8_t browser_filter_gametype;           // 0xc88 0
    uint8_t browser_filter_teamplay;           // 0xc89 0
    uint8_t browser_filter_ping_limit_index;           // 0xc8a 0
    uint8_t unknown_c8b[0x100];    // 0xc8b never written by this module
    uint8_t unknown_d8b;           // 0xd8b outside every copy
    // network (0xd8c .. 0x1108), player_profile_set_default_server_options (0x53a150) writes the same defaults
    uint16_t server_name[0x90];    // 0xd8c wide, Halo by default; the UI wcscpys it to 0x00719170.
                                   //       Length UNSURE: 0x120 bytes run up to the password
    uint16_t server_password[9];   // 0xeac wide, empty by default (8 characters plus terminator)
    uint8_t unknown_ebe;           // 0xebe 0
    uint8_t unknown_ebf;           // 0xebf 3; written from a UI selection by ui_event_4a2f10
    uint8_t unknown_ec0[0x100];    // 0xec0 never written by this module
    uint8_t unknown_fc0;           // 0xfc0 1; the UI reads it as a 0..4 choice
    uint8_t unknown_fc1;           // 0xfc1
    uint16_t join_host_name[0x20];    // 0xfc2 wide string, empty by default; the UI wcscpys a host
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
