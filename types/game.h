// Blam game module (halo.exe 1.0.10 retail, 0x459300..0x551d30, 428 functions).
// The game / player layer: the fixed 30Hz simulation clock, the player datum and the
// local-player control record, the multiplayer "game engine" (its five built-in gametypes,
// the 0x98-byte game variant option block and the per-engine callback table), the
// scoreboard / kill-feed / announcer plumbing, the CTF and King-of-the-Hill runtime state,
// the client/server player update queues, and the save-game directory helpers.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries
// the layout it is used in preference to the decompiler and the fact is called out:
//   - players_initialize (0x4735b0) is the anchor for the whole header. Its disassembly
//     (bin/halo.exe, 0x4735b3..0x47365b) shows the element sizes that Ghidra loses because
//     game_state_new takes its stride in EBX:
//         push 0x10 ; push "players" ; mov ebx,0x200 ; call game_state_new  -> 16 x 0x200
//         push 0x10 ; push "teams"   ; mov ebx,0x40  ; call game_state_new  -> 16 x 0x40
//       followed by two raw game-state allocations of 0x98 (player_globals, pointer parked at
//       0x0087a478) and 0x50 (player_control_globals, pointer at 0x006b145c).
//   - The gametype table at 0x00688308 is read straight out of .data. Indexing it with the
//     variant game_engine_index gives the five engine definitions
//         1 -> 0x00687d20 "ctf"      2 -> 0x006880f8 "slayer"   3 -> 0x00688258 "oddball"
//         4 -> 0x00687ec8 "king"     5 -> 0x006881a8 "race"
//     and the rows are 0xb0 bytes apart, which fixes the size of game_engine_definition. The
//     first two words of every row are a char * name and the engine own index, so the
//     callbacks start at +0x08. A sixth row at 0x00688048 names itself "stub".
//   - The netgame / starting-location layouts come from types/tags.h, not from the
//     decompiler: ScenarioNetgameFlags is 0x94 with type at +0x10 and usage_id at +0x12
//     (scenario reflexive at +0x378), ScenarioNetgameEquipment is 0x90 (reflexive at +0x384),
//     ScenarioPlayerStartingLocation is 0x34 (reflexive at +0x354) and
//     ScenarioPlayerStartingProfile is 0x68 (reflexive at +0x348). Every stride the module
//     multiplies by (0x94, 0x90, 0x34, 0x68) matches.
//   - GlobalsMultiplayerInformation (globals tag +0x168) carries the announcer sound list:
//     count at +0x5c, pointer at +0x60, GlobalsSound stride 0x10 with the sound
//     TagDependency at +0x0c. game_engine_get_multiplayer_sound_duration_ticks walks exactly
//     that chain.
//   - unit_control_data (types/units.h, 0x40 bytes) is the input record 0x472760 digitizes
//     into player_action_flags; the bit-to-field mapping below is taken from that function.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index, data_array, data_iterator, datum_header
//   types/math.h     real_point3d, real_vector3d
//   types/cache.h    tag_instance (0x0087bc14, tag data at +0x14)
//   types/objects.h  object, object_header (0x008603b0, stride 0x0c, data at +0x08)
//   types/units.h    unit_data (controlling_player at object+0x218, desired_weapon_index at
//                    0x2f4, desired_grenade_index at 0x31d, desired_zoom_level at 0x321),
//                    unit_control_data
//   types/hs.h       (R32) no longer has its own hs_game_time_globals slice; hs, objects
//                    and units use game_time_globals below. +0x1c is the fractional-tick
//                    accumulator (game_engine_advance_ticks 0x470b30), not seconds_per_tick.
//   types/tags.h     Scenario, ScenarioNetgameFlags, ScenarioNetgameEquipment,
//                    ScenarioPlayerStartingLocation, ScenarioPlayerStartingProfile, Globals,
//                    GlobalsMultiplayerInformation, GlobalsSound, GlobalsPlayerInformation
//
// Unicode: the engine stores player and variant names as UTF-16. There is no wchar_t for
// Ghidra CParser here, so they are declared as uint16_t arrays.

#include <stddef.h> // offsetof
#pragma pack(push, 1)

// ---------------------------------------------------------------------------
// game_main_globals (0x006b0b80 main_game_globals points at it): whether a map is loaded and
// the game running, the double-speed flag, map load progress, and the options the current game
// was started with. Offsets from the uses in src/game (game_start_new_map, game_unload_map,
// cache_file_switch_map_by_path, game_simulate_tick) and the difficulty reads across src/;
// the same layout OpenSauce documents for Custom Edition (s_game_globals).
// ---------------------------------------------------------------------------
typedef struct game_main_globals {
    uint8_t map_loaded;                 // 0x00 cleared by game_unload_map
    uint8_t active;                     // 0x01 set by game_start_new_map, cleared by game_stop_current_map
    uint8_t players_are_double_speed;   // 0x02 60Hz ticks (game_simulate_tick), half-rate effects
    uint8_t map_loading_in_progress;    // 0x03
    float map_load_progress;            // 0x04 cache_file_download_status_get, 1.0 when done
    int32_t unknown_08;                 // 0x08 start of the game options block
    int16_t unknown_0c;                 // 0x0c
    int16_t difficulty;                 // 0x0e
    uint32_t random_seed;               // 0x10 copied into random_seed_global at game start
} game_main_globals;                    // (only the fields above are known; used through a pointer)
typedef char game_main_globals_difficulty_at_0e[offsetof(game_main_globals, difficulty) == 0x0e ? 1 : -1];
typedef char game_main_globals_random_seed_at_10[offsetof(game_main_globals, random_seed) == 0x10 ? 1 : -1];
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum game_constants {
    k_maximum_players = 16,                 // players_initialize, data_new count
    k_maximum_teams = 16,                   // players_initialize, "teams" count
    k_player_size = 0x200,                  // mov ebx,0x200 before the "players" data_new
    k_team_size = 0x40,                     // mov ebx,0x40 before the "teams" data_new
    k_maximum_local_players = 1,            // every local-player bound test is "< 1"
    k_player_name_length = 11,              // _wcsncpy(player + 4, name, 0xb) + a NUL
    k_player_globals_size = 0x98,
    k_player_control_globals_size = 0x50,
    k_game_variant_size = 0x98,             // 0x26 dwords, block-moved everywhere
    k_game_engine_definition_size = 0xb0,   // spacing of the rows the 0x00688308 table points at
    k_game_engine_count = 5,                // game_variant_sanitize_options clamps to 1..5
    k_game_variant_history_entry_size = 0xa4,
    k_maximum_variant_history_entries = 99, // game_engine_get_variant_by_name gives up at 99
    k_maximum_scoreboard_entries = 16,      // game_engine_build_sorted_player_list
    k_scoreboard_entry_size = 0x1c,         // the element width it hands qsort
    k_maximum_custom_waypoints = 32,        // 0x006f1888..0x006f1c88, stride 0x20
    k_maximum_queued_multiplayer_sounds = 5,// game_engine_queue_multiplayer_sound refuses at 5
    k_maximum_ctf_flags = 32,               // the flag-id bitmask is one uint32
    k_maximum_team_pair_overrides = 8,      // team_pair_add refuses past 8
    k_team_pair_index_count = 10,           // every pair test bounds both indices to 0..9
    k_player_profile_cache_count = 16,      // 0x006b0b88..0x006b0e88, stride 0x30
    k_player_update_history_count = 120,    // player_update_queue_create, 120 x 0x2c
    k_player_update_history_record_size = 0x2c,
    k_position_update_queue_count = 30,     // position_update_queue_create, 30 x 0x14
    k_position_update_record_size = 0x14,
    k_vehicle_update_queue_count = 30,      // vehicle_update_queue_create, 30 x 0x48
    k_vehicle_update_record_size = 0x48,
    k_server_update_history_count = 32,     // the (tick & 0x1f) ring at 0x006f1d94
    k_client_update_history_count = 128,    // the (tick & 0x7f) ring at 0x006f7ed4
    k_update_record_size = 0x308,           // stride of both rings (0xc2 dwords)
    k_game_ticks_per_second = 30,           // 0x470b30 scales the time scale by 30.0
    k_savegame_index_record_size = 0x206,   // 0x53e0e0 / 0x53e420 divide by it
    k_maximum_user_save_paths = 8,          // user_save_path_register scans 8 slots
    k_user_save_path_length = 0x104,        // _strncpy(..., 0x104) per slot
    k_user_save_path_slot_stride = 0x105    // the stride the table itself is indexed by
} game_constants;

// ---------------------------------------------------------------------------
// game_time_globals  (pointer at 0x006f1d6c, 0x20 bytes)
// Allocated whole by game_engine_allocate_tick_record (0x470a80), which asks the game-state
// arena for exactly 0x20 bytes and zeroes all eight dwords. Only the offsets below are ever
// touched anywhere in the image; 0x04..0x0b are dead in this build.
// types/hs.h used to carry a partial copy of this record (hs_game_time_globals) that called
// +0x1c "seconds_per_tick"; R32 removed it, and every module now uses this struct.
// ---------------------------------------------------------------------------
typedef struct game_time_globals {
    uint8_t initialized;       // 0x00 main's game-time reset 0x4c8a60 clears it (0x4c8aee),
                               //      zeroes all 8 dwords, then sets it to 1 (0x4c8b0e); read
                               //      by main_queue_map_change (0x4c876b), 0x4c9770 and others
                               //      (0x48a420, 0x4c7c70, 0x4de780, 0x54b975) (R33)
    uint8_t active;            // 0x01 0x470ae0 sets it; advance_simulation_ticks refuses to
                               //      run any tick while it is zero
    uint8_t paused;            // 0x02 units / hs read it as a "time is running" gate, and both
                               //      game_engine_build_local_player_control_input (0x4710b0)
                               //      and game_engine_update_local_player_control (0x471ae0)
                               //      refuse to turn stick input into look deltas, or to cycle
                               //      the weapon / grenade / zoom level, while it is set
    uint8_t unknown_03[0x0c - 0x03]; // 0x03
    int32_t game_time;         // 0x0c the simulation tick counter, incremented once per tick
    int16_t ticks_this_frame;  // 0x10 how many ticks advance_simulation_ticks just ran
    int16_t unknown_12;        // 0x12
    int32_t elapsed_ticks;     // 0x14 second, never-reset counter bumped beside game_time
    float speed;               // 0x18 time scale; forced to 1.0 in any networked game
                               //      (0x470b4b fld [ecx+0x18]; game_engine_get_time_scale)
    float leftover_time;       // 0x1c fractional-tick accumulator carried between frames, in
                               //      seconds: 0x470b30 adds it to the frame dt (0x470b67),
                               //      stores the remainder (0x470bca) or 0 (0x470bdc). The
                               //      units code multiplies it by 29.999998 (0x55a1f7) to get
                               //      the fraction of a tick elapsed. NOT seconds_per_tick.
} game_time_globals;           // size 0x20

// ---------------------------------------------------------------------------
// game_variant  (0x98 bytes)
// The multiplayer option block. Its size is fixed three times over: every
// game_engine_variant_defaults_* function builds one on a 76-uint16 stack buffer and copies
// 0x26 dwords out of it, game_engine_get_variant_by_name declares the same 76-uint16 buffer,
// and the network session state reserves 0x98 bytes for it at +0x10c.
//
// Field meanings come from game_variant_sanitize_options (0x466730), which is the only
// function that touches nearly every field, plus the use sites of the aliased globals in the
// active copy at 0x006f1c88 (e.g. 0x006f1cbc == variant+0x34 is what
// game_engine_get_teams_enabled returns). Offsets 0x00..0x2f are zeroed by every defaults
// function and never read by this module -- they are the variant UTF-16 display name,
// which only the UI and the network layer fill in.
// ---------------------------------------------------------------------------
typedef struct game_variant {
    uint16_t name[24];         // 0x00 UTF-16 display name; all of it zero in the built-ins.
                               //      At most 23 wide characters plus the terminator at +0x2e:
                               //      saved_game_create_custom_variant wcsncpy's 0x17 chars and
                               //      stores 0 at +0x2e (0x53bbe0..0x53bbfe); the default-profile
                               //      writer does the same (0x53bd65, 0x53bda5) (R37)
    int32_t game_engine_index; // 0x30 sanitize clamps to 1..5, see game_engine_index
    uint8_t teams;             // 0x34 sanitize normalizes to 0/1; game_engine_get_teams_enabled
    uint8_t pad_35[3];         // 0x35
    uint32_t flags;            // 0x38 option bitfield; slayer forces bits 0 and 8 on
    int32_t objective_indicator;// 0x3c 0 motion tracker, 1 nav points (custom waypoints only draw at 1), 2 none;
    uint8_t odd_man_out;       // 0x40 sanitize normalizes to 0/1
    uint8_t pad_41[3];         // 0x41
    int32_t respawn_time_growth;// 0x44 clamped >= 0; on_player_death adds it to 0x30 and caps
                               //      the total at 5x this value
    int32_t respawn_time;      // 0x48 clamped >= 0; the base respawn countdown in ticks
    int32_t suicide_penalty;   // 0x4c clamped >= 0; added when the killer is the victim
    int32_t lives_per_round;   // 0x50 clamped >= 0; 0 means unlimited. A player whose death
                               //      count (player+0xae) reaches it is eliminated
    float speed_scale;         // 0x54 clamped to 0.25 .. 4.0
    int32_t score_limit;       // 0x58
    int32_t starting_equipment;// 0x5c clamped to 0 .. 0xd
    uint32_t vehicle_set;      // 0x60 low nibble clamped to 0 .. 8; the upper bits are a
                               //      packed 3-bit-per-slot table (the built-ins store
                               //      0x249240, i.e. every slot from index 2 up set to 1)
    uint32_t unknown_64;       // 0x64 same packed 3-bit encoding as 0x60
    int32_t time_limit;        // 0x68 in ticks (slayer default 0x708 == 60 s * 30)
    uint8_t friendly_fire_mode; // 0x6c 0..3 (UI clamps to 0..3, default 1); object_apply_damage switches on it (alias 0x006f1cf4)
    uint8_t pad_6d[3];         // 0x6d
    int32_t betrayal_penalty;  // 0x70 on_player_death multiplies it by player+0xc0
    uint8_t team_switch_restricted; // 0x74 when set, a client team-change request is only honored if game_engine_team_close_game_check passes (alias 0x006f1cfc)
    uint8_t pad_75[3];         // 0x75
    int32_t game_time_limit;    // 0x78 slayer default 36000 ticks (20 minutes)
    uint8_t ctf_option_7c;     // 0x7c the four bytes 0x7c..0x7f are only normalized when
    uint8_t ctf_option_7d;     // 0x7d game_engine_index is 1 (ctf); 0x7f is skipped when the
    uint8_t ctf_option_7e;     // 0x7e index is 2 (slayer), which also normalizes 0x7c..0x7e
    uint8_t ctf_option_7f;     // 0x7f
    int32_t ctf_value_80;      // 0x80 clamped >= 0 for game_engine_index 1
    int32_t oddball_trait_with_ball;    // 0x84 oddball engine (index 3): trait id for a ball carrier (1 = invisible: the KotH/oddball scorer resets the camo gauge otherwise)
    int32_t oddball_trait_without_ball; // 0x88 trait id for everyone else (game_engine_oddball_time_scale_override compares against 0x84 or 0x88)
    int32_t oddball_style;              // 0x8c 0 oddball, 1 reverse tag / accumulation, 2 juggernaut / stalker; 2 skips per-ball scoring
    int32_t ball_count;                 // 0x90 balls in play (<= 16): loop bound over king_hill_occupant_table and oddball_ball_timers
    uint16_t variant_flags;    // 0x94 (R37) flags word, same encoding as saved_games.h
                               //      saved_player_profile::flags: bit 0 = built-in/default
                               //      (every built-in writes 1; saved_game_create_custom_variant
                               //      clears it, and BYTE [v+0x94],0xfe at 0x53bbc9), high byte =
                               //      default index (playlist_profile_create_default_profiles_
                               //      on_disk ORs index<<8 in, 0x53bd8c..0x53bdad)
    int16_t unknown_96;        // 0x96
} game_variant;                // size 0x98

// game_variant::game_engine_index. The values are the indices into the 0x00688308 table and
// are confirmed by the index word each engine definition carries at its own +0x04.
typedef enum game_engine_index {
    _game_engine_none = 0,
    _game_engine_ctf = 1,
    _game_engine_slayer = 2,
    _game_engine_oddball = 3,
    _game_engine_king = 4,
    _game_engine_race = 5,
    _game_engine_stub = 6          // 0x00688048, name "stub"; unreachable through sanitize
} game_engine_index;

// ---------------------------------------------------------------------------
// game_engine_definition  (0xb0 bytes)
// One row per built-in gametype. Every slot from +0x08 on is an optional callback: the
// engines fill the ones they implement, leave 0x00000000 in the ones they do not, and park
// the shared do-nothing thunk at 0x0044ad80 in the rest -- so a caller has to test the slot
// against NULL, which every call site in the image does.
//
// Slot names below are taken from the single caller of each slot (the mapping was produced
// by scanning every "(**(code **)(DAT_006f1d20 + N))()" in out/halo_decompiled.c and
// attributing it to its enclosing function). Slots no call site in the image reaches are
// left as unknown_XX.
// ---------------------------------------------------------------------------
typedef struct game_engine_definition {
    char *name;                        // 0x00 "ctf" / "slayer" / "oddball" / "king" / "race"
    int32_t index;                     // 0x04 matches its slot in the 0x00688308 table
    void *dispose;                     // 0x08 game_engine_unload
    void *initialize_for_new_game;     // 0x0c 0x45c370; returns false to abort the start
    void *dispose_from_old_game;       // 0x10 0x45b370 (game shutdown)
    void *player_new_life;             // 0x14 0x45c440, after the per-life reset
    void *player_changed_object;       // 0x18 0x45c570
    void *unknown_1c;                  // 0x1c
    void *reset_round;                 // 0x20 0x45b8b0
    void *unknown_24;                  // 0x24
    void *unknown_28;                  // 0x28
    void *unknown_2c;                  // 0x2c
    void *unknown_30;                  // 0x30
    void *post_rasterize;              // 0x34 render_scene_draw (0x50bfb0)
    void *update;                      // 0x38 game_engine_tick
    void *object_in_play_update;       // 0x3c 0x45f560 (per-tick pickup bookkeeping)
    void *weapon_pickup_allowed;      // 0x40 (weapon, player) from game_engine_notify_weapon_ready_state_change; result is the permission
    void *object_expired;              // 0x44 0x45f510 (unclaimed item about to despawn)
    void *update_after_players;        // 0x48 game_engine_tick, once after the per-player loop
    void *get_score;                   // 0x4c 0x463480 / kill-feed builder; takes a player
                                       //      handle (or -1) and returns its score
    void *get_team_score;              // 0x50 called with 0 and 1
    void *build_player_text;             // 0x54 (player, wchar buffer) -> scoreboard row text
    void *build_score_header_text;     // 0x58 (wchar buffer)
    void *build_team_score_text;       // 0x5c (team, wchar buffer)
    void *weapon_use_permission;       // 0x60 reached from the units module (0x56da00)
    void *unknown_64;                  // 0x64 reached from the units module (0x5674a0)
    void *player_killed;                // 0x68 fired first thing in game_engine_on_player_death
    void *build_message_text;          // 0x6c variant override for the kill-feed text builder
    void *starting_location_scale;      // 0x70 (player, location) -> multiplier in game_engine_rate_player_starting_location
    void *player_team_changed;         // 0x74 0x4611b0
    void *allow_grenade_counts;        // 0x78 game_engine_apply_player_grenade_counts
    void *unknown_7c;                  // 0x7c
    void *waypoint_filter;             // 0x80 0x4620c0, per custom-waypoint slot
    void *feature_enabled;             // 0x84 (kind) predicate, called with 0 or 1; gates damage scaling and
                                       //      one kill-feed message class
    void *time_scale_override;         // 0x88 0x461550
    void *is_winner;                   // 0x8c 0x463730 / 0x45cf30
    void *profiles_updated;            // 0x90 0x466cb0 and the network layer (0x4dfa10)
    void *profile_post_update;         // 0x94 0x466e60
    void *player_round_reset;          // 0x98 0x463620
    void *qr2_server_key_hook;         // 0x9c (key, buffer), network_session_host_qr2_server_key
    void *qr2_player_key_hook;         // 0xa0 (key, index, buffer), qr2 player key; also reached from the units module (0x577e40)
    void *qr2_team_key_hook;           // 0xa4 (key, index, buffer)
    void *qr2_count_hook;              // 0xa8 (key_type) -> count
    void *reset_objects;               // 0xac 0x468260 / 0x468320
} game_engine_definition;              // size 0xb0

// The end-of-game state machine value at 0x0087aa10. 0 is also what
// game_engine_is_inactive tests for, so "not started" doubles as "no game running".
typedef enum game_engine_state {
    _game_engine_state_not_started = 0,
    _game_engine_state_ending = 1,     // begin_end_game_sequence, 7.0 s timer at 0x0087aa08
    _game_engine_state_ended = 2,      // 5.0 s timer, set by the second countdown stage
    _game_engine_state_post_game = 3   // carnage report is up; 0x0087aa0c fades 0 -> 1
} game_engine_state;

// ---------------------------------------------------------------------------
// game_variant_history_entry  (0xa4 bytes)
// The GlobalAlloc-ed recent / custom variants array behind 0x00687b0c. Both pointers are
// freed one by one by game_engine_free_custom_variant_cache, and
// game_engine_variant_add_to_history writes the ASCII name into slot 0x04 and the 0x26-dword
// option block starting at 0x0c.
// ---------------------------------------------------------------------------
typedef struct game_variant_history_entry {
    char *path;                // 0x00 freed alongside name; UNSURE which of the two is which
    char *name;                // 0x04 GlobalAlloc-ed copy of the variant name
    int32_t unknown_08;        // 0x08
    game_variant options;      // 0x0c
} game_variant_history_entry;  // size 0xa4

// ---------------------------------------------------------------------------
// circular_queue  (0x18 bytes)
// The shared fixed-record ring behind position_update_queue_create (30 x 0x14),
// vehicle_update_queue_create (30 x 0x48) and the server-side per-player queue. The
// constructors GlobalAlloc the record storage and a separate table of one pointer per
// record, then fill the table with evenly strided pointers into the storage -- so the
// records never move and push/pop only ever copy into or out of records[i].
// One slot is always left empty: push refuses once the used count reaches capacity - 1.
// ---------------------------------------------------------------------------
typedef struct circular_queue {
    int32_t capacity;          // 0x00 record count
    int32_t record_size;       // 0x04 bytes per record
    void **records;            // 0x08 capacity pointers into storage
    int32_t write_index;       // 0x0c
    int32_t read_index;        // 0x10 equal to write_index means empty
    void *storage;             // 0x14 capacity * record_size bytes
} circular_queue;              // size 0x18

// ---------------------------------------------------------------------------
// player_update_queue  (0x3c bytes)
// The larger ring player_update_queue_create (0x479f40) builds: the same 0x18 header plus a
// held copy of the record currently being replayed. 0x479fb0 decrements the record
// refcount at +0x04, pops it once it hits zero, copies 0xb dwords of it to the caller and
// then copies its dwords 3..10 into the tail below. Placed at player + 0x120, the 0x3c bytes
// end exactly where the next initialized field (player + 0x15c) begins.
// ---------------------------------------------------------------------------
typedef struct player_update_queue {
    circular_queue queue;      // 0x00 120 records of 0x2c
    uint8_t has_current;       // 0x18 set once a record has been latched
    uint8_t pad_19[3];         // 0x19
    int32_t current[8];        // 0x1c dwords 3..10 of the latched record
} player_update_queue;         // size 0x3c

// One entry of the per-tick server/client update ring. Both rings use the same 0x308 stride:
// the server ring is 32 deep at 0x006f1d94 and indexed (tick & 0x1f), the client ring is
// 128 deep at 0x006f7ed4 and indexed (tick & 0x7f). Only the two header fields are named
// here; the body is the packed per-player payload the network module encodes.
typedef struct update_record {
    int32_t tick;              // 0x00
    uint16_t player_count;     // 0x04
    uint8_t body[0x308 - 0x06];// 0x06
} update_record;               // size 0x308 == k_update_record_size

// One element of the "update server queues" data_array (16 x 0x64). 0x472c90 creates the
// datum and immediately calls player_update_queue_create on it, and every ring access in
// 0x472cc0 is at base + 0x28 .. + 0x3c, so the 0x3c-byte queue sits at 0x28 and fills the
// record exactly.
typedef struct update_server_queue {
    int16_t identifier;                // 0x00 datum_header
    uint8_t unknown_02[0x28 - 0x02];   // 0x02
    player_update_queue queue;         // 0x28
} update_server_queue;                 // size 0x64

// ---------------------------------------------------------------------------
// position_update_record (0x14) / vehicle_update_record (0x48)
// The two record shapes carried by the two circular_queues embedded in every player:
// position_update_queue_create (0x47a020) builds 30 x 0x14 at player + 0x170, and
// vehicle_update_queue_create (0x47a250) builds 30 x 0x48 at player + 0x1d0. Both records
// start with the same two dwords -- the simulation tick the update belongs to and a small
// wrapping sequence distance that position_update_queue_find_and_remove (0x47a100) and
// vehicle_update_queue_find_and_remove (0x47a2c0) compare modulo 0x40 to decide whether a
// record is merely early (keep it) or already stale (discard it). Pushing is
// position_update_queue_push (0x47a0c0) for the small one; the vehicle variant is pushed by
// the network module, not from this module.
// ---------------------------------------------------------------------------
typedef struct position_update_record {
    int32_t tick;              // 0x00
    int32_t sequence;          // 0x04 wrapping distance, compared mod 0x40
    real_point3d position;     // 0x08 copied out as three raw dwords by the queue functions
                               //      themselves; the consumer
                               //      apply_remote_player_position_update (0x477350) uses them
                               //      as a position
} position_update_record;      // size 0x14

// The 0x40-byte payload of a vehicle update. Field identities come from the one consumer,
// apply_remote_player_vehicle_position_update (0x477490), which feeds them to
// object_set_position_and_orientation; UNSURE throughout.
typedef struct vehicle_update_body {
    int32_t parent_or_tag;     // 0x00 UNSURE
    real_point3d position;     // 0x04
    real_vector3d velocity;    // 0x10
    real_vector3d angular_velocity; // 0x1c
    real_vector3d forward;     // 0x28
    real_vector3d up;          // 0x34
} vehicle_update_body;         // size 0x40

typedef struct vehicle_update_record {
    int32_t tick;              // 0x00
    int32_t sequence;          // 0x04 same wrapping distance as position_update_record
    vehicle_update_body body;  // 0x08
} vehicle_update_record;       // size 0x48 == vehicle_update_queue record_size

// ---------------------------------------------------------------------------
// client_update_carry  (0x10 bytes, 16 of them on the stack)
// The per-player side-band record update_client_queue_apply_tick (0x4730d0) writes into its
// second output array and game_engine_players_update_server (0x4740a0) then consumes. Neither
// end of that pair is attested by anything else in the image, so every field here is UNSURE;
// the byte/dword split is taken from how the consumer accesses it.
// ---------------------------------------------------------------------------
typedef struct client_update_carry {
    uint8_t flag_a;            // 0x00 gates the 0x4e7b50 / grenade-value block
    uint8_t flag_b;            // 0x01 gates whether field1 becomes the grenade value for this tick
    uint8_t pad_02[2];         // 0x02
    int32_t field1;            // 0x04
    int32_t field2;            // 0x08
    int32_t field3;            // 0x0c
} client_update_carry;         // size 0x10

// ---------------------------------------------------------------------------
// player  (0x200 bytes, the "players" data_array at 0x0087a480)
// The two constructors, 0x473780 (network-replicated player) and 0x473940 (locally created
// player), between them initialize every field that has a non-zero default, and
// players_dispose / player_delete tear the same set down, so the offsets below are all
// directly observed writes. The names come from the functions that read them back; anything
// that stayed ambiguous is unknown_XX.
//
// The three network queues are embedded, not pointed to. Their offsets are pinned by the
// "lea esi,[ebp+N]" ahead of each constructor call in 0x473940:
//   +0x120 player_update_queue_create (120 x 0x2c)   -> 0x3c header, ends exactly at 0x15c
//   +0x170 position_update_queue_create (30 x 0x14)  -> 0x18 header, ends exactly at 0x188
//   +0x1d0 vehicle_update_queue_create (30 x 0x48)   -> 0x18 header, ends exactly at 0x1e8
// ---------------------------------------------------------------------------
typedef struct player {
    int16_t identifier;                // 0x00 datum_header salt
    int16_t local_player_index;        // 0x02 -1 unless this player is driven locally;
                                       //      game_set_local_player keeps it in sync with
                                       //      player_globals::local_players
    uint16_t name[12];                 // 0x04 UTF-16, _wcsncpy of 11 chars plus the NUL the
                                       //      constructors write at 0x1a
    int32_t unknown_1c;                // 0x1c both constructors write -1
    int32_t team;                      // 0x20 0..k_team_pair_index_count-1; both constructors
                                       //      seed it with 1, 0x45c440 recomputes it
    datum_index interaction_object;    // 0x24 board / swap / assassinate target
    int16_t interaction_type;          // 0x28 player_set_pending_interaction_action keeps the
                                       //      highest value; 0xb clears the slot
    int16_t interaction_seat;          // 0x2a vehicle seat the pending action would use
    int32_t respawn_timer;             // 0x2c ticks left; clamped to 0x5a..9000 on death
    int32_t respawn_time_growth;       // 0x30 accumulates game_variant::respawn_time_growth,
                                       //      capped at 5x it, and is refunded to the killer
    datum_index unit;                  // 0x34 the object this player drives, -1 when dead
    datum_index previous_unit;         // 0x38 0x474e10 rolls unit into it on every change
    int16_t bsp_cluster;               // 0x3c constructors write -1
    int16_t unknown_3e;                // 0x3e
    datum_index observer_target;       // 0x40 camera_observer_update (0x4593b0) result
    int32_t observer_state;            // 0x44 written beside 0x40 by the same function
    uint16_t identifier_name[12];      // 0x48 second copy of the name: 0x473940 does a
                                       //      rep movsd of 8 dwords from the caller
                                       //      player-identifier record into 0x48
    int32_t unknown_60;                // 0x60 tail of that same 0x20-byte copy
    int16_t machine_index;             // 0x64 network machine slot that owns the player; only the low byte is read and compared against the machine table (game_engine_notify_kill_event, network_session_autoban_player)
    int8_t team_index;                 // 0x66 0x45c440 assigns it round-robin in team games
    int8_t team_index_desired;         // 0x67 the requested team it is derived from
    int16_t kill_streak[2];            // 0x68 slot 0 also sets object flag 0x10 and stamps
                                       //      the streak method into unit+0x422; 0x479d10
                                       //      counts both down once per tick
    float speed;                       // 0x6c constructors write 1.0; part of the profile
    datum_index teleporter_entrance_flag; // 0x70 cached ScenarioNetgameFlags index of the entrance last used (game_engine_update_teleporter); -1 none
    datum_index engine_message;      // 0x74 pending game-engine HUD message id (KOTH writes 0x23/0x29; game_engine_pick_hud_hint clears the respawn ids 0x17..0x1a); -1 none
    datum_index engine_message_subject; // 0x78 the player that message is about; -1 none
    datum_index nameplate_target;     // 0x7c tracked teammate handle the nameplate HUD keeps (hud_draw_teammate_nameplate); -1 none
    int32_t nameplate_hysteresis;      // 0x80 counter 0..0xf: hud_draw_teammate_nameplate ticks it up while the candidate matches nameplate_target, down otherwise, and only switches target at 0
    int32_t last_death_tick;           // 0x84 game_time when this player last died; the
                                       //      odd-man-out test orders players by it
    int32_t unknown_88;                // 0x88 part of the profile block
    uint8_t odd_man_out;               // 0x8c cached result of 0x460e40
    uint8_t unknown_8d[0x96 - 0x8d];   // 0x8d
    // The statistics block. game_engine_attribute_player_death (0x46ff00) is the one
    // function that writes all of it, and it separates the three roles cleanly: the victim
    // gets deaths (and suicides when the killer is itself) and has its three streak fields
    // reset, the killer gets kills or -- when teams_are_enemies says the pair is friendly --
    // betrayals, and every surviving recent damager gets assists. The profile cache mirrors
    // 0x9c..0xb2 verbatim.
    int16_t killing_spree_count;       // 0x96 +1 per kill, zeroed on death
    int16_t multikill_count;           // 0x98 reset to 1 when the previous kill is more than
                                       //      0x78 ticks (4 s) old, else incremented
    int16_t last_kill_tick;            // 0x9a game_time of the last kill, -1 on death
    int16_t kills;                     // 0x9c
    int16_t unknown_9e;                // 0x9e
    int32_t unknown_a0;                // 0xa0
    int16_t assists;                   // 0xa4 credited to each of the four recent damagers
    int16_t unknown_a6;                // 0xa6
    int32_t unknown_a8;                // 0xa8
    int16_t betrayals;                 // 0xac +1 when killer and victim are not enemies
    int16_t deaths;                    // 0xae compared against game_variant::lives_per_round
                                       //      to decide elimination
    int16_t suicides;                  // 0xb0 +1 when the killer is the victim
    int16_t unknown_b2;                // 0xb2
    int32_t unknown_b4;                // 0xb4 game_engine_update_teleporter
    uint8_t unknown_b8[0xc0 - 0xb8];   // 0xb8
    int16_t betrayal_penalty_count;    // 0xc0 +1 per betrayal; on_player_death scales it by
                                       //      game_variant::betrayal_penalty and clears it
    int16_t unknown_c2;                // 0xc2
    int32_t objective_time;            // 0xc4 hill / ball time in ticks. The profile cache
                                       //      divides it by 30 on the way out and multiplies
                                       //      it back on the way in when the engine is king
    int16_t unknown_c8;                // 0xc8 flag touches; also mirrored by the profile
    uint8_t unknown_ca[0xd0 - 0xca];   // 0xca
    datum_index removal_tick;          // 0xd0 game_time at which game_engine_flag_local_player_units marks the player for deletion; constructors write -1 (none)
    uint8_t teleport_blocked;          // 0xd4 set when a teleporter destination is obstructed by this player's unit, cleared every tick (main_switch_structure_bsp)
    uint8_t marked_for_deletion;       // 0xd5 1 makes 0x474e10 call player_remove; every
                                       //      respawn / scoreboard path skips such a player
    uint8_t unknown_d6[0xdc - 0xd6];   // 0xd6
    int32_t ping_ms;                   // 0xdc round-trip time: now minus the ping-timestamp send time (network_game_message_handle_ping_timestamp); shown on the scoreboard
    int32_t medal_streak_count;        // 0xe0 0x479eb0 bumps it and fires the medal event
                                       //      once it reaches the threshold at 0x006894a4
    int32_t medal_streak_timer;        // 0xe4 seeded negative from 0x0069956c; the streak is
                                       //      only extended while it is >= 0
    int32_t unknown_e8;                // 0xe8
    datum_index unknown_ec;            // 0xec 0x473940 writes -1
    int32_t unknown_f0;                // 0xf0 start of a 0xc-dword run the local constructor
    datum_index unknown_f4;            // 0xf4 zeroes; the network constructor writes -1 here
    int32_t unknown_f8;                // 0xf8
    uint8_t unknown_fc[0x104 - 0xfc];  // 0xfc
    int32_t unknown_104;               // 0x104 network constructor writes -1
    uint8_t rate_window_started;   // 0x108 network_client_check_connection_quality has taken its first sample
    uint8_t pad_109[3];                // 0x109
    int32_t rate_window_start_ms;  // 0x10c start of the current sampling window (restarted after 10 s)
    int32_t rate_window_units;     // 0x110 units received in that window
    int32_t rate_last_sample_ms;   // 0x114 time of the previous sample
    int32_t rate_slow_samples;     // 0x118 consecutive samples at or below 39.9 units/s; more than 5 drops the client
    int32_t unknown_11c;               // 0x11c
    player_update_queue update_history;       // 0x120 120 records of 0x2c
    int32_t last_remote_update_id;     // 0x15c (R35) last remote update sequence (byte
                                       //      value), -1 = none; the local constructor writes -1.
                                       //      is_remote_player_update_in_order 0x4e6a20: mov ecx,
                                       //      [eax+0x15c]; cmp ecx,-1; sub edi,ecx; cmp edi,4
    datum_index unknown_160;           // 0x160 local constructor writes -1
    int32_t unknown_164;               // 0x164
    int32_t unknown_168;               // 0x168
    int32_t unknown_16c;               // 0x16c
    circular_queue position_updates;          // 0x170 30 records of 0x14
    int32_t unknown_188;               // 0x188
    datum_index unknown_18c;           // 0x18c local constructor writes -1
    uint8_t unknown_190[0x1d0 - 0x190];// 0x190 0x10 dwords the local constructor zeroes
    circular_queue vehicle_updates;            // 0x1d0 30 records of 0x48
    int32_t unknown_1e8;               // 0x1e8
    int32_t unknown_1ec;               // 0x1ec
    int32_t unknown_1f0;               // 0x1f0
    int32_t unknown_1f4;               // 0x1f4
    int32_t unknown_1f8;               // 0x1f8
    int32_t unknown_1fc;               // 0x1fc
} player;                              // size 0x200 == k_player_size

// ---------------------------------------------------------------------------
// team  (0x40 bytes, the "teams" data_array at 0x0087a47c)
// players_initialize allocates it and players_dispose empties it, and those are the only two
// references to 0x0087a47c in the whole image -- no function in this build ever creates or
// reads a team datum, so the contents are unknown. The size is still proved by the
// "mov ebx,0x40" ahead of its data_new.
// ---------------------------------------------------------------------------
typedef struct team {
    int16_t identifier;        // 0x00 datum_header
    uint8_t unknown_02[0x3e];  // 0x02 never written in this build
} team;                        // size 0x40 == k_team_size

// ---------------------------------------------------------------------------
// player_globals  (pointer at 0x0087a478, 0x98 bytes)
// players_initialize asks the game-state arena for 0x98 bytes and seeds three fields;
// players_dispose zeroes all 0x26 dwords and then re-seeds the same set. The two datum
// arrays at +0x04 and +0x08 are indexed by local_player_index, and every bound test in the
// module is "< 1", so this build has exactly one local player slot.
// ---------------------------------------------------------------------------
typedef struct player_globals {
    datum_index unknown_00;            // 0x00 seeded to -1, never read
    datum_index local_players[1];      // 0x04 local_player_to_player_index / game_set_local_player
    datum_index local_player_units[1]; // 0x08 the unit each local player drives; indexed by
                                       //      player::local_player_index
    int16_t local_player_count;        // 0x0c number of active local players (R34): cmp WORD
                                       //      [esi+0xc],1 (0x4531cc) / ,2 (0x45333a), movsx
                                       //      (0x451edc); HUD / render split-screen tests use > 1
    int16_t respawn_stagger;           // 0x0e bumped and decremented while respawns are
                                       //      spread across frames
    uint8_t no_player_has_a_unit;      // 0x10 0x474e10 sets 1 then clears it if any player
                                       //      still has a unit
    uint8_t input_disabled;            // 0x11 player_enable_input(false) and cinematics set it; player
                                       //      updates skip local input while it is set
    int16_t unknown_12;                // 0x12 seeded to -1
    int16_t mode;                      // 0x14 written with 0 and with 3
    uint8_t unknown_16;                // 0x16
    uint8_t unknown_17;                // 0x17
    uint32_t cluster_pvs[0x20];        // 0x18 a bit per structure cluster the local players can see
                                       //      (game_engine_build_visible_cluster_bitmask fills it)
} player_globals;                      // size 0x98
typedef char player_globals_size[sizeof(player_globals) == 0x98 ? 1 : -1];

// ---------------------------------------------------------------------------
// local_player_control  (0x40 bytes)
// The look / aim state one local player owns. game_engine_init_player_look_state_from_object
// (0x470e80) zeroes all 0x10 dwords and then writes every default below, which is what fixes
// the size; game_engine_reset_player_look_state (0x470de0) does the same thing for slot 0
// through the header.
// ---------------------------------------------------------------------------
// RESOLVED (phase 4 review, game_engine_build_local_player_control_input 0x4710b0 and
// game_engine_update_local_player_control 0x471ae0): 0x04 and 0x14..0x1c are this tick
// player_control_input echoed back into the record, 0x08/0x0a are the two 16-bit button
// suppression masks, 0x26/0x27 are the look auto-levelling pair, and 0x30/0x34 complete the
// aim-assist block that starts at 0x28.
typedef struct local_player_control {
    datum_index unit;              // 0x00 the controlled object, -1 when there is none
    uint32_t input_control_flags;  // 0x04 player_control_input::control_flags, stored by
                                   //      0x471ae0 after it builds this tick input
    uint16_t suppressed_buttons;   // 0x08 bit per digital button index (0..18): while set, the
                                   //      button reads as "not pressed". 0x4710b0 mask loop is
                                   //      a 16-bit AND/ANDN pair, so buttons 16..18 can never be
                                   //      suppressed even though the loop counts to 19.
    uint16_t suppressed_until_released; // 0x0a the subset of suppressed_buttons that is released
                                   //      again (both bits cleared) as soon as the physical
                                   //      button reads zero
    float yaw;                     // 0x0c atan2(facing.j, facing.i), wrapped into [0, 2pi)
    float pitch;                   // 0x10 atan2(facing.k, |facing.ij|)
    float input_throttle_x;        // 0x14 player_control_input::throttle_x, stored by 0x471ae0
    float input_throttle_y;        // 0x18 player_control_input::throttle_y
    float input_primary_trigger;   // 0x1c player_control_input::primary_trigger
    int16_t desired_weapon_index;  // 0x20 seeded from unit+0x2f4; 0x472100 rewrites it
    int16_t desired_grenade_index; // 0x22 seeded from unit+0x31d
    int16_t desired_zoom_level;    // 0x24 seeded from unit+0x321; 0x4726f0 invalidates it
                                   //      with 0xffff and 0x472740 reads it back
    uint8_t autolevelling_active;  // 0x26 set once autolevelling_ticks passes
                                   //      GlobalsPlayerControl::minimum_autolevelling_ticks;
                                   //      game_engine_update_local_player_look reads it as the
                                   //      "keep steering the pitch toward level" gate
    int8_t autolevelling_ticks;    // 0x27 clamped to 0..0x7f; 0x471ae0 counts up while the
                                   //      player is on foot, running (|throttle_x| > 0.5), not
                                   //      looking up or down and has no aim-assist target, and
                                   //      resets it to 0 otherwise
    datum_index nameplate_target;  // 0x28 written -1 by both initializers, and read back by
                                   //      hud_find_nearby_teammate_for_nameplate (0x45e340) as
                                   //      the teammate whose nameplate is currently drawn.
                                   //      Ghidra shows that read as
                                   //      "local_player_index * 0x40 + DAT_006b145c + 0x38",
                                   //      i.e. the compiler folded the 0x10-byte header into
                                   //      the field offset: 0x10 + 0x28 == 0x38.
    float nameplate_weight;        // 0x2c the same read gate, "0.0 < weight" means the track
                                   //      above is live; left 0.0 by both initializers.
                                   //      It is also camera_observer_get_target_angles
                                   //      out_weight_primary -- 0x4710b0 calls that function as
                                   //        get_target_angles(&nameplate_weight,
                                   //                          &aim_assist_weight, ..., slot)
                                   //      so 0x28/0x2c/0x30 are one aim-assist tracker block
                                   //      that the HUD nameplate code reuses.
    float aim_assist_weight;       // 0x30 out_weight_secondary of the same call; the magnetism
                                   //      strength, 0.0 when there is no target
    float look_acceleration_timer; // 0x34 seconds the look stick has been held past
                                   //      GlobalsPlayerControl::look_peg_threshold; drives the
                                   //      look_acceleration_time / look_acceleration_scale ramp
                                   //      and is zeroed the moment the stick falls back inside
                                   //      the threshold
    float pitch_minimum;           // 0x38 -1.4906585 (-85.4 degrees)
    float pitch_maximum;           // 0x3c +1.4906585
} local_player_control;            // size 0x40

// ---------------------------------------------------------------------------
// player_control_globals  (pointer at 0x006b145c, 0x50 bytes)
// The 0x50-byte game-state block players_initialize allocates right after player_globals:
// a 0x10-byte header of digitized action flags followed by one local_player_control.
// Every indexed access in the module is "local_player_index * 0x40 + 0x10 + base".
// ---------------------------------------------------------------------------
typedef struct player_control_globals {
    uint32_t action_flags;         // 0x00 player_action_flags, rebuilt every tick by 0x472760
    uint32_t action_flags_latched; // 0x04 bits whose edge has already been consumed
    uint32_t action_flags_edge;    // 0x08 bits 0x472760 arms and clears again
    uint32_t flags;                // 0x0c bit 0 suppresses player input: both
                                   //      game_engine_build_local_player_control_input (look
                                   //      deltas) and game_engine_update_local_player_control
                                   //      (zoom cycling) short-circuit while it is set
    local_player_control local_players[1]; // 0x10
} player_control_globals;          // size 0x50

// ---------------------------------------------------------------------------
// player_control_input  (0x20 bytes, built on the caller stack)
// One local the player control input for one tick.
// game_engine_build_local_player_control_input (0x4710b0) zeroes all eight dwords and fills
// them, then tail-calls game_engine_digitize_control_input (0x472760) with this record in EDX;
// game_engine_update_local_player_control (0x471ae0) is the only caller and copies the fields
// back out into local_player_control and into a player_action.
// The field offsets are pinned from both ends: 0x4710b0's stores and 0x472760's loads agree on
// every one of them (0x472760 tests +0x08 > 0 for _player_action_primary_trigger, which is the
// field 0x4710b0 sets to exactly 1.0 or 0.0 from control_flags bit 0x800).
// ---------------------------------------------------------------------------
typedef struct player_control_input {
    float throttle_x;              // 0x00 forward / back; +0x00 and +0x04 are normalized as a
                                   //      2D pair so the magnitude never exceeds 1
    float throttle_y;              // 0x04 left / right
    float primary_trigger;         // 0x08 1.0 when control_flags bit 0x800 is set, else 0.0
    float yaw_delta;               // 0x0c radians to turn this tick (already * dt * 30)
    float pitch_delta;             // 0x10 radians to pitch this tick
    int8_t action;                 // 0x14 raw hold count of digital button 2, unless that button
                                   //      is in local_player_control::suppressed_buttons
    int8_t melee;                  // 0x15 raw hold count of digital button 9, same condition
    int16_t pad_16;                // 0x16 never written (the record is zeroed first)
    uint32_t control_flags;        // 0x18 the digitized button set; the bit names are the
                                   //      unit_control_flags this eventually feeds
    uint32_t button_flags;         // 0x1c a second digitized set. CORRECTED (phase 4 review, the
                                   //      0x471ae0 disassembly): 0x01 next weapon (from digital
                                   //      button 3), 0x02 next grenade (button 1), 0x04 zoom
                                   //      (button 11); 0x08 and 0x10 are the single-player debug
                                   //      "possess the nearest / next unit" pair and 0x20 the
                                   //      debug camera-shake sample. The earlier note had 0x01
                                   //      and 0x02 the other way round.
} player_control_input;            // size 0x20

// ---------------------------------------------------------------------------
// player_action  (0x20 bytes)
// The per-tick control record the local machine stages for the network layer.
// game_engine_update_local_player_control (0x471ae0) assembles one on its stack and
// `rep movs`-es eight dwords to 0x006f7ea4 + update_client_staged_count * 0x20; 0x473270 copies
// it back out with the same eight-dword move and rewrites dword 0, and 0x473310 copies it into
// the outgoing packet. Nothing in this build ever stages more than one, so the array at
// 0x006f7ea4 is one element long and 0x006f7ec4 begins immediately after it.
// ---------------------------------------------------------------------------
typedef struct player_action {
    uint32_t control_flags;        // 0x00 player_control_input::control_flags verbatim; 0x473270
                                   //      masks it with 0x4d0 before re-staging
    float desired_yaw;             // 0x04 local_player_control::yaw (absolute, radians)
    float desired_pitch;           // 0x08 local_player_control::pitch
    float throttle_x;              // 0x0c player_control_input::throttle_x
    float throttle_y;              // 0x10 player_control_input::throttle_y
    float primary_trigger;         // 0x14 player_control_input::primary_trigger
    int16_t weapon_index;          // 0x18 local_player_control::desired_weapon_index
    int16_t grenade_index;         // 0x1a local_player_control::desired_grenade_index
    int16_t zoom_level;            // 0x1c local_player_control::desired_zoom_level
    int16_t pad_1e;                // 0x1e left uninitialized by 0x471ae0
} player_action;                   // size 0x20

// ---------------------------------------------------------------------------
// player_update_record  (0x2c bytes, the record type of player_update_queue)
// player_update_queue_pop_current (0x479fb0) copies 11 dwords of one of these to its caller
// and mirrors dwords 3..10 into player_update_queue::current -- which is what fixes the tail
// as exactly one player_action.
// The two counters are proved by the pair of functions at the two ends. 0x479fb0 does
// "dec [record+0x04]" and only advances the ring read cursor once that reaches zero, and its
// callers (0x474590 at 0x4746af, 0x4740a0) then test "out[1] == out[2] - 1" -- i.e. the
// pre-decrement remaining count equalled the total -- to decide whether THIS consumer is the
// first one to see the record, which is exactly when they call
// player_apply_first_position_update (0x476cf0). So +0x04 counts down and +0x08 is the total.
// ---------------------------------------------------------------------------
typedef struct player_update_record {
    uint32_t field0;             // 0x00 UNSURE: forwarded whole to 0x476cf0
    int32_t references_remaining;// 0x04 decremented by every pop; 0 retires the record
    int32_t reference_count;     // 0x08 how many consumers the record was queued for
    player_action action;        // 0x0c the staged control record for this tick
} player_update_record;        // size 0x2c == player_update_queue record_size

// ---------------------------------------------------------------------------
// local_player_input_state  (0x28 bytes, 0x00712498 + local_player_index * 0x28)
// NOT this module type -- it belongs to the input module, which has had no type recovery yet.
// The slice below is only what game_engine_build_local_player_control_input reads out of it, and
// the stride comes straight out of that function own address arithmetic
//   (objdump 0x471143: lea edx,[eax+eax*4] ; lea edx,[edx*8+0x712498], i.e. index * 0x28).
// Declared here so the one function that reads it does not have to use raw offsets.
// ---------------------------------------------------------------------------
typedef struct local_player_input_state {
    int8_t buttons[0x13];          // 0x00 one hold count per digital button, 19 of them. The
                                   //      control_flags bits 0x4710b0 builds out of them are
                                   //      button 10 -> 0x0001, 0 -> 0x0002, 2 -> 0x0040,
                                   //      5 -> 0x0010, 13 -> 0x0400, 7 -> 0x0800,
                                   //      6 -> 0x1000 and 0x2000 together, 4 -> 0x0080, and
                                   //      0x4000 from button 14 or from button 2 held at least
                                   //      GlobalsPlayerControl::minimum_weapon_swap_ticks ticks.
                                   //      The button_flags bits are button 3 -> 0x01,
                                   //      1 -> 0x02, 11 -> 0x04. Buttons 2 and 9 are also copied
                                   //      raw into player_control_input::action / ::melee.
    int8_t pad_13;                 // 0x13
    float throttle_x;              // 0x14 quantized to -1/0/+1 in any networked game
    float throttle_y;              // 0x18 same
    float look_x;                  // 0x1c raw look stick, before the square-to-circle scaling
    float look_y;                  // 0x20
    uint8_t look_is_analog;        // 0x24 zero selects the digital look path (raw rates, no
                                   //      response curve, no aim assist)
    uint8_t pad_25[3];             // 0x25
} local_player_input_state;        // size 0x28

// The bits 0x472760 sets in player_control_globals::action_flags from a player_control_input.
// CORRECTED (phase 4 review): the source records are player_control_input fields, not
// types/units.h unit_control_data fields -- the two layouts differ (unit_control_data::throttle
// starts at +0x0c, player_control_input::throttle_x at +0x00), and 0x472760 is only ever called
// by game_engine_build_local_player_control_input with the record it just built.
typedef enum player_action_flags {
    _player_action_jump = 0x0001,          // control_flags bit 0x40
    _player_action_flashlight = 0x0002,    // control_flags bit 0x02
    _player_action_action = 0x0004,        // action (+0x14) nonzero
    _player_action_melee = 0x0008,         // melee (+0x15) nonzero
    _player_action_primary_trigger = 0x0010,// primary_trigger (+0x08) > 0
    _player_action_reload = 0x0020,        // control_flags bit 0x2000
    _player_action_exchange_weapon = 0x0040,// button_flags (+0x1c) bit 0x04, i.e. zoom
    _player_action_look_up = 0x0080,       // pitch_delta (+0x10) > 0
    _player_action_look_down = 0x0100,     //   same field, < 0
    _player_action_look_left = 0x0200,     // yaw_delta (+0x0c) > 0
    _player_action_look_right = 0x0400,    //   same field, < 0
    _player_action_forward = 0x0800,       // throttle_x (+0x00) > 0
    _player_action_backward = 0x1000,      //   same field, < 0
    _player_action_left = 0x2000,          // throttle_y (+0x04) > 0
    _player_action_right = 0x4000          //   same field, < 0
} player_action_flags;

// ---------------------------------------------------------------------------
// player_profile  (0x30 bytes, 16 of them at 0x006b0b88)
// The score snapshot the server keeps for every player so a reconnecting or re-created
// player datum can be restored. player_profile_cache_initialize zeroes 0xc0 dwords starting
// at 0x006b0b88, which is exactly 16 * 0x30, and the table ends at 0x006b0e88.
// The field-to-player mapping is the pair 0x466ee0 (capture) / 0x466d00 (apply), which copy
// the same twelve values in the same order. Note 0x1e is deliberately misaligned: the
// compiler packs the int32 from player+0xc4 straight after three int16s.
// ---------------------------------------------------------------------------
typedef struct player_profile {
    uint8_t in_use;            // 0x00 the only field the free-slot scan looks at
    uint8_t pad_01[3];         // 0x01
    datum_index player;        // 0x04 the handle 0x466e80 matches against
    int16_t kills;             // 0x08 <- player + 0x9c   (the cache copies 0x9c..0xb2 as
    int16_t unknown_0a;        // 0x0a <- player + 0x9e    four dwords and three words, so
    int32_t unknown_0c;        // 0x0c <- player + 0xa0    the field split here is the
    int16_t assists;           // 0x10 <- player + 0xa4    player's, not the copy's)
    int16_t unknown_12;        // 0x12 <- player + 0xa6
    int32_t unknown_14;        // 0x14 <- player + 0xa8
    int16_t betrayals;         // 0x18 <- player + 0xac
    int16_t deaths;            // 0x1a <- player + 0xae
    int16_t suicides;          // 0x1c <- player + 0xb0
    int32_t objective_time;    // 0x1e <- player + 0xc4 (seconds here, ticks in the player;
                               //         the king engine rescales by 30 across the copy)
    int16_t unknown_22;        // 0x22 <- player + 0xc8
    int32_t unknown_24;        // 0x24 <- player + 0x88
    uint8_t odd_man_out;       // 0x28 <- player + 0x8c
    uint8_t pad_29[3];         // 0x29
    float speed;               // 0x2c <- player + 0x6c
} player_profile;              // size 0x30

// ---------------------------------------------------------------------------
// team_pair_override / team_pair_globals  (0xb4 bytes, pointer at 0x006b0b84)
// The "are these two 0..9 indices hostile" table. 0x45bc30 allocates 0x2d dwords (0xb4) of
// game state and zeroes it; the two 100-bit relationship bitmaps live at the end and the
// override list at the front. Both bitmaps are addressed as (a * 10 + b), so they need
// 100 bits == four uint32 each, and 0x94 + 0x10 + 0x10 == 0xb4 accounts for every byte.
// ---------------------------------------------------------------------------
typedef struct team_pair_override {
    int16_t index_a;           // 0x00
    int16_t index_b;           // 0x02 either ordering matches in every lookup
    int16_t threshold;         // 0x04 0x45bfc0 fires team_pair_set once the counter reaches it
    int16_t timer_reset;       // 0x06 value the countdown is refreshed to
    uint8_t index_a_is_other;  // 0x08 index_a is the non-player ("other") side of the pair; matched for (b,a) lookups
    uint8_t index_b_is_other;  // 0x09 index_b is the "other" side; matched for (a,b) lookups
    uint8_t active;            // 0x0a set to 1 when the entry is inserted
    uint8_t status;            // 0x0b the extra byte 0x45be00 returns and 0x45c0f0 clears
    uint8_t other_is_human;    // 0x0c the other side is the human category (adjust_counter reports !this through out_flag)
    uint8_t pad_0d;            // 0x0d
    int16_t refcount;          // 0x0e decremented when the countdown expires; at 0 the entry
                               //      is torn down
    int16_t timer;             // 0x10 ticks down once per call to 0x45bcf0
} team_pair_override;          // size 0x12

typedef struct team_pair_globals {
    int16_t override_count;                              // 0x00 at most 8
    team_pair_override overrides[8];                     // 0x02
    uint8_t pad_92[2];                                   // 0x92
    uint32_t secondary_bits[4];                          // 0x94 indexed (b * 10 + a)
    uint32_t enemy_bits[4];                              // 0xa4 indexed (a * 10 + b);
                                                         //      teams_are_enemies inverts it
} team_pair_globals;                                     // size 0xb4

// ---------------------------------------------------------------------------
// scoreboard_entry  (0x1c bytes, up to 16 of them)
// game_engine_build_sorted_player_list hands qsort an element width of 0x1c and then walks
// the sorted array comparing dwords 2..5 of adjacent entries to detect ties, which is what
// identifies the four sort keys and the trailing place field. The high bit of place marks a
// tie with the entry above.
// ---------------------------------------------------------------------------
typedef struct scoreboard_entry {
    datum_index player;        // 0x00
    int32_t unknown_04;        // 0x04 never compared
    int32_t key_0;             // 0x08 primary sort key, built by 0x45cc30: a clamped score
    int32_t key_1;             // 0x0c   biased by +1000 plus bit 0x40000000 when the player
    int32_t key_2;             // 0x10   still has lives left and bit 0x20000000 when the
    int32_t key_3;             // 0x14   player is not marked for deletion
    int32_t place;             // 0x18 0-based rank; bit 0x80000000 marks a tie with the
                               //      entry above
} scoreboard_entry;            // size 0x1c == k_scoreboard_entry_size

// ---------------------------------------------------------------------------
// custom_waypoint  (0x20 bytes, 32 of them at 0x006f1888)
// The HUD nav-point table. Every accessor indexes it as "slot * 8" over a uint32 array, so
// the stride is 0x20, and the 32 slots run from 0x006f1888 to 0x006f1c88, which is exactly
// where the active game_variant copy begins -- the two blocks are adjacent in .data.
// -1 in player, team or owner means "matches anything" (0x4620c0).
// ---------------------------------------------------------------------------
typedef struct custom_waypoint {
    real_point3d position;     // 0x00 the caller point, raised by 0.63 world units
    uint8_t active;            // 0x0c
    uint8_t pad_0d[3];         // 0x0d
    datum_index player;        // 0x10 -1 = every player
    int16_t team;              // 0x14 -1 = every team; otherwise compared to player::team
    int16_t pad_16;            // 0x16
    datum_index owner;         // 0x18 -1 = no owner filter
    int16_t icon;              // 0x1c resolved by name through 0x4af070
    int16_t pad_1e;            // 0x1e
} custom_waypoint;             // size 0x20

// ---------------------------------------------------------------------------
// multiplayer_sound_request  (0x10 bytes, 5 of them at 0x006b10f0)
// The announcer queue. game_engine_queue_multiplayer_sound refuses to add a sixth entry, and
// the tick handler shifts the whole array down by one 0x10-byte record when the head
// expires; 0x006b10f0 + 5 * 0x10 == 0x006b1140, which is where the count lives.
// ---------------------------------------------------------------------------
typedef struct multiplayer_sound_request {
    datum_index player;        // 0x00 recipient, -1 for everyone
    int32_t sound_index;       // 0x04 index into GlobalsMultiplayerInformation::sounds
    int32_t remaining_ticks;   // 0x08 duration + 5, counted down once per tick
    uint8_t broadcast;         // 0x0c only ever 1 while hosting
    uint8_t pad_0d[3];         // 0x0d
} multiplayer_sound_request;   // size 0x10

// ---------------------------------------------------------------------------
// observer_target_candidate  (0x38 bytes, 64 of them on the stack)
// camera_observer_find_best_target (0x459a00) declares a 0xe00-byte stack array and hands
// qsort an element width of 0x38, which gives 64 slots. 0x459b10 fills one slot and is where
// every field below comes from; the comparator 0x45a4a0 orders by weight_secondary, then
// weight_primary, then distance, then angle, then object index.
// ---------------------------------------------------------------------------
typedef struct observer_target_candidate {
    datum_index object;        // 0x00
    real_point3d point;        // 0x04 closest point on the target segment to the observer
    real_vector3d offset;      // 0x10 point - observer position
    real_vector3d direction;   // 0x1c normalized copy of offset
    float distance;            // 0x28 length of offset
    float angle;               // 0x2c angle between direction and the observer facing
    float weight_primary;      // 0x30 falloff(distance, cone.distance_a) * falloff(angle, cone.angle_a)
    float weight_secondary;    // 0x34 the same product against the b pair, boosted by the
                               //      globals tag player information +0x08 when the target
                               //      tag carries flag 0x80000
} observer_target_candidate;   // size 0x38

// The four-float cone camera_observer_target_score takes by pointer. Passing NULL zeroes
// both weights, which is how the "collect everything" pass is expressed.
typedef struct observer_target_cone {
    float angle_a;             // 0x00
    float distance_a;          // 0x04
    float angle_b;             // 0x08
    float distance_b;          // 0x0c
} observer_target_cone;        // size 0x10

// ---------------------------------------------------------------------------
// ctf_globals / king_globals
// Two parallel 0x148-byte blocks are zeroed together by game_engine_ctf_initialize_flags
// (0x52 dwords each): 0x006b1290 holds the live state and 0x0087a520 the replicated copy the
// network layer serializes. The first dword is a bitmask of the netgame_flag usage_ids that
// exist on this map (they are assigned uniquely in 0..31 by 0x46d800), and the sixteen dwords
// after it are the per-team active flag id, seeded either with the lowest usage_id on the map
// or with -1 depending on game_variant::ctf_option_7c. The rest of the block is unresolved.
// ---------------------------------------------------------------------------
typedef struct ctf_globals {
    uint32_t flag_id_mask;             // 0x00 bit i set means usage_id i exists on this map
    int32_t team_flag_id[16];          // 0x04
    uint8_t unknown_44[0x148 - 0x44];  // 0x44 UNRESOLVED: per-team capture counters, the
                                       //      captured bitmasks 0x46ec10 sends, and the
                                       //      neutral-flag slot at 0x006b1314
} ctf_globals;                         // size 0x148

// King of the Hill occupancy. The three globals are contiguous and are always written as a
// set by game_engine_koth_update_hill_occupancy_state.
typedef struct king_globals {
    int32_t hill_state;        // 0x006b1050 see king_hill_state
    int32_t hill_ticks;        // 0x006b1054 how long the current holder has held it; the
                               //      "contested" cue only plays past 0x12d ticks
    datum_index occupant;      // 0x006b1058 the single holder, -1 when empty or contested
} king_globals;                // size 0x0c

typedef enum king_hill_state {
    _king_hill_empty = 0,
    _king_hill_held = 1,       // one player holds it (non-team play)
    _king_hill_team_0 = 2,     // in team play, one team holds it
    _king_hill_team_1 = 3,
    _king_hill_contested = 4
} king_hill_state;

// The moving-hill marker history at 0x0087a9a0: four positions seeded with the current hill
// location plus four parallel state slots, reset together by 0x46b250.
typedef struct king_hill_marker_history {
    real_point3d position[4];  // 0x00
    int32_t state[4];          // 0x30
} king_hill_marker_history;    // size 0x40

// ---------------------------------------------------------------------------
// save games
// The index file is a flat array of fixed 0x206-byte records: 0x53e0e0 seeks to
// slot * 0x206 and reads one record, 0x53e1f0 writes one, and 0x53e420 divides the file size
// by 0x206 to get the record count. Every one of them rebuilds the same file_reference
// (types/hs.h, 0x10c bytes) in place at 0x00721330, which is why the 0x43-dword memset
// appears four times.
// The record body is opaque here -- no function in this module reads a field inside it.
// ---------------------------------------------------------------------------
typedef struct savegame_index_record {
    uint8_t data[0x206];
} savegame_index_record;       // size 0x206 == k_savegame_index_record_size

// savegame_find_first registers the directory it is enumerating under the Win32 find handle
// so savegame_find_next can rebuild full paths; the table is a parallel pair of arrays and
// 0x00721f30 + 8 * 0x105 == 0x00722758 accounts for every byte between them.
// The three functions themselves take the two arrays separately (they are two distinct
// globals, not one struct), so src/game declares them as
//   extern uint32_t user_save_path_keys[k_maximum_user_save_paths];       // 0x00722758
//   extern char user_save_paths[k_maximum_user_save_paths][k_user_save_path_slot_stride];
// and this struct is kept only to record that the two are contiguous and account for every
// byte between 0x00721f30 and 0x00722778.
typedef struct user_save_path_table {
    char paths[k_maximum_user_save_paths][k_user_save_path_slot_stride];
                               // 0x00721f30, _strncpy of k_user_save_path_length chars
    uint32_t keys[k_maximum_user_save_paths];
                               // 0x00722758, 0 marks a free slot. NOT necessarily a user id:
                               //   savegame_find_first registers a Win32 find HANDLE here.
} user_save_path_table;        // size 0x848

// ---------------------------------------------------------------------------
// win32_find_dataa  (0x140 bytes)
// Not a Blam type: the Win32 WIN32_FIND_DATAA that XDeleteSaveGame (0x5519a0),
// savegame_find_first (0x551bc0) and savegame_find_next (0x551d30) hand to
// FindFirstFileA / FindNextFileA. Declared here because types/ has no Win32 header and three
// functions in this module need the layout; only dwFileAttributes (tested for 0x10, i.e.
// FILE_ATTRIBUTE_DIRECTORY) and cFileName (+0x2c) are ever read. The trailing pad makes the
// declared size the real 0x140 so the OS has room to write, which matters because
// savegame_find_first builds its path scratch 0x140 bytes past the base of that buffer.
// ---------------------------------------------------------------------------
typedef struct win32_find_dataa {
    uint32_t dwFileAttributes;     // 0x00 bit 0x10 == directory
    uint32_t ftCreationTime[2];    // 0x04 FILETIME
    uint32_t ftLastAccessTime[2];  // 0x0c
    uint32_t ftLastWriteTime[2];   // 0x14
    uint32_t nFileSizeHigh;        // 0x1c
    uint32_t nFileSizeLow;         // 0x20
    uint32_t dwReserved0;          // 0x24
    uint32_t dwReserved1;          // 0x28
    char cFileName[260];           // 0x2c MAX_PATH
    char cAlternateFileName[14];   // 0x130
    uint8_t pad_13e[2];            // 0x13e
} win32_find_dataa;            // size 0x140

// ---------------------------------------------------------------------------
// hud_text_bounds (0x08) and hud_world_text_params (0x10)
// The shared 16-bit text rasterizer at 0x514ab0 takes two register arguments -- a value in EAX
// and, in ECX, a pointer to the 8-byte packed rectangle below. Both wrappers in this module
// build that rectangle on their own stack and then `lea ecx,[...]` it into the call:
//   hud_draw_scoreboard_row_text  0x45d670  top = row * 0x12,        bottom = top + 0x1a
//   hud_draw_world_relative_text  0x4653f0  top = row * 0x0f + 0x3b, bottom = top + 0x11
// In both, `left` and `right` are the high halves of the two screen-anchor dwords at 0x007c3148
// and 0x007c314c with the high half of 0x007c3140 subtracted from each. The field order is the
// Blam short-rectangle order (top, left, bottom, right), which is what makes the two 32-bit
// stores those functions actually emit line up.
// ---------------------------------------------------------------------------
typedef struct hud_text_bounds {
    int16_t top;                   // 0x00
    int16_t left;                  // 0x02
    int16_t bottom;                // 0x04
    int16_t right;                 // 0x06
} hud_text_bounds;                 // size 0x08

// The caller-built block hud_draw_world_relative_text (0x4653f0) reads through EAX. It has the
// tags.h ColorARGB layout, alpha first (R36): 0x4653f0 copies +0x00 into 0x006e4738 (0x465592)
// and +0x04/+0x08/+0x0c, possibly brightened, into 0x006e473c/40/44 -- i.e. text.h's
// ColorARGB text_color (alpha, red, green, blue), whose callers store 1.0 in alpha. All four are
// IEEE-754 floats in memory -- 0x4653f0 only ever moves them with plain `mov`/`fld`, never
// `cvtsi2ss`/`fild`, which is what rules out Ghidra's "(float)in_EAX[1]" integer-cast reading.
// Kept as its own struct (same field order and names as ColorARGB) so callers keep the name.
typedef struct hud_world_text_params {
    real alpha;                    // 0x00 -> text_color.alpha (0x006e4738)
    real red;                      // 0x04 -> text_color.red   (0x006e473c)
    real green;                    // 0x08 -> text_color.green (0x006e4740)
    real blue;                     // 0x0c -> text_color.blue  (0x006e4744)
} hud_world_text_params;           // size 0x10 == sizeof(ColorARGB)

// ---------------------------------------------------------------------------
// The debug spawn cheats shared record is types/tags.h TagDependency, not a type of its own.
// RESOLVED (phase 4 review): the first pass declared a `cheat_tag_record` here with an opaque
// 0x0c-byte head and a tag handle at +0x0c. That is exactly TagDependency
// (tag_fourcc, path_pointer, path_size, tag_id) -- and the proof is the two other entry points
// into cheat_spawn_objects_near_camera (0x45a800):
//   - cheat_spawn_all_object_tags (0x45a530) passes Globals::weapon_list.pointer directly
//     (an array of GlobalsWeapon, i.e. one TagDependency each) when that list is non-empty, and
//     otherwise builds a 16-slot stack array in the same shape from a tag_iterator filtered on
//     'weap' -- so its own scratch array must be TagDependency-shaped.
//   - cheat_spawn_warthog (0x45a5c0) walks
//     Globals::multiplayer_information -> GlobalsMultiplayerInformation::vehicles with a 0x10
//     stride, compares each entry +0x04 (TagDependency::path_pointer) against "warthog", and
//     passes &vehicles[i] as a one-element array.
// No `cheat_tag_record` is declared here any more; src/game/cheat_spawn_*.c use TagDependency.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// camera_basis_out  (0x18 bytes, built on the caller stack)
// The first-person camera basis chimera__spectate_fp_camera_position (0x472020) fills and
// game_engine_update_local_player_look (0x472160) consumes. Folded here from per-file copies in
// those two files, which is the only reason it lives in this header: no camera-module header
// exists yet, so this is a slice of a type another module owns, pinned by its one producer and
// consumer agreeing on all four fields.
// UNSURE: 0x0c is a pointer into a model node / marker transform, not a type this module names.
// ---------------------------------------------------------------------------
typedef struct camera_basis_out {
    datum_index unit;              // 0x00 the unit the camera is attached to
    int16_t seat_index;            // 0x04 -1 when the unit is on foot
    int16_t pad_06;                // 0x06
    uint8_t *marker_offset;        // 0x08 UNSURE: the seat camera marker, NULL when on foot
    real_point3d position;         // 0x0c
} camera_basis_out;                // size 0x18

// ---------------------------------------------------------------------------
// koth_fence_corner  (0x44 bytes = 17 floats)
// One vertex of the King-of-the-Hill boundary "fence" that
// game_engine_koth_build_hill_boundary_fence (0x46b2d0) builds and
// game_engine_koth_submit_hill_marker_geometry rasterizes four at a time. This is a rasterizer
// vertex format, not a game type; it is folded here from that file local copy because both
// files already agree on the 17-float stride and the render module has no header yet.
// UNSURE: everything except position, normal and the two texture coordinates.
// ---------------------------------------------------------------------------
typedef struct koth_fence_corner {
    float position[3];             // 0x00
    float normal[3];               // 0x0c shared outward face normal for the whole edge
    float unused_18[6];            // 0x18
    float u;                       // 0x30 runs with the accumulated edge length
    float v;                       // 0x34 fixed per corner (ground vs top)
    float unused_38[3];            // 0x38
} koth_fence_corner;               // size 0x44

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// players and teams (players_initialize @0x4735b0, players_dispose @0x473670)
// global 0x0087a480: data_array *player_data              "players", 16 x 0x200
// global 0x0087a47c: data_array *team_data                "teams", 16 x 0x40 (never populated)
// global 0x0087a478: player_globals *player_globals       0x98 of game state
// global 0x006b145c: player_control_globals *player_control_globals   0x50 of game state
// global 0x006f1d74: float player_look_yaw_rate[1]     degrees/second, seeded from
//                      GlobalsPlayerControl::look_default_yaw_rate by 0x470de0 when still zero
// global 0x006f1d78: float player_look_pitch_rate[1]   same, from look_default_pitch_rate
// global 0x006f1d7d: uint8_t player_look_aim_assist_enabled   gates the magnetism blend
// global 0x006f1d7f: uint8_t player_look_rate_boost_enabled   UNSURE; when set, digital button
//                      0x0b toggles the 1x / 2x look-rate tier below
// global 0x006f1d80: uint8_t player_look_rate_boost         UNSURE; 0 or 1, +1 is the tier
// global 0x006b1460: datum_index machine_to_player[16]    cleared to -1 by players_dispose;
//                      0x473940 and 0x473390 index it with a machine index
// 0x006b2ce8: int32_t joystick_slot_devices[4] -- NOT owned here; see the read-but-not-owned list
//   at the end of this file (input.h owns it: slot -> input device index, -1 = none).

// simulation clock (game_engine_allocate_tick_record @0x470a80)
// global 0x006f1d6c: game_time_globals *game_time         0x20 of game state
// global 0x006f1d48: int32_t game_time_unknown_48         cleared beside the tick record
// global 0x006f1d49: uint8_t game_time_unknown_49         set to 1 beside the tick record
// global 0x007196d8: int32_t game_time_force_single_tick  nonzero pins the frame to one tick

// the multiplayer game engine. The active game_variant copy at 0x006f1c88 is immediately
// followed in .data by the engine pointer, so every "DAT_006f1cXX" is a variant field:
//   +0x30 0x006f1cb8  +0x34 0x006f1cbc  +0x38 0x006f1cc0  +0x40 0x006f1cc8
//   +0x44 0x006f1ccc  +0x48 0x006f1cd0  +0x4c 0x006f1cd4  +0x50 0x006f1cd8
//   +0x54 0x006f1cdc  +0x70 0x006f1cf8  +0x7c 0x006f1d04  +0x80 0x006f1d08
// global 0x006f1c88: game_variant game_engine_variant     the live option block
// global 0x006f1d20: game_engine_definition *current_game_engine   NULL outside multiplayer.
//                      Other modules headers call this address game_is_server or a
//                      "network / predicted state flag"; it is neither -- it is the loaded
//                      gametype, and nonzero does mean that a multiplayer engine is
//                      running, which is why those tests still work.
// global 0x00688308: game_engine_definition *game_engine_definitions[7]   indexed by
//                      game_engine_index; entry 0 is NULL
// global 0x0087aa00: int32_t game_engine_unknown_aa00
// global 0x0087aa04: int32_t game_engine_auto_team_counter  round-robin team assignment
// global 0x0087aa08: float game_engine_end_game_timer       7.0 s, then 5.0 s
// global 0x0087aa0c: float game_engine_post_game_fade       ramps 0 -> 1
// global 0x0087aa10: int32_t game_engine_state              game_engine_state
// global 0x0087aa14: int32_t game_engine_unknown_aa14
// global 0x0087aa18: uint8_t game_engine_dedicated_idle
// global 0x0087aa1c: float game_engine_dedicated_idle_timer
// global 0x0087aa20: int32_t game_engine_unknown_aa20
// global 0x0087aa24: int32_t game_engine_ctf_reset_ticks    seeded with 0x1e
// global 0x0087aa40: char game_engine_pending_variant_name[0x40]
// global 0x0087aa80: game_variant game_engine_pending_variant
// global 0x0087ab20: game_variant game_engine_active_variant
// global 0x00688328: uint8_t multiplayer_sound_enabled[]    gate per announcer sound index

// custom game variant history (game_engine_variant_add_to_history @0x463980)
// global 0x00687b0c: game_variant_history_entry *game_variant_history   GlobalAlloc-ed
// global 0x00687b10: uint32_t game_variant_history_count
// global 0x00687b14: uint32_t game_variant_history_capacity   grown four entries at a time
// global 0x00687b18: int32_t game_variant_history_current     -1 when the cache is empty
// global 0x00714de0: game_variant game_variant_saved_default  restored by 0x463b20
// global 0x00714e78: uint8_t game_variant_saved_default_valid

// HUD / scoreboard / announcer
// global 0x006f1888: custom_waypoint custom_waypoints[32]
// global 0x006b10f0: multiplayer_sound_request multiplayer_sound_queue[5]
// global 0x006b1140: int32_t multiplayer_sound_queue_count
// global 0x00660c34: char ui_multiplayer_game_text_path[]   "ui\\multiplayer_game_text"

// team relationships (0x45bc30)
// global 0x006b0b84: team_pair_globals *team_pair_globals   0xb4 of game state

// player profile cache (player_profile_cache_initialize @0x466c20)
// global 0x006b0b88: player_profile player_profile_cache[16]
// global 0x006f1d34: int32_t player_profile_cache_count
// global 0x006f1d38: uint8_t player_profile_cache_initialized

// objective engines
// global 0x006b1290: ctf_globals ctf_globals
// global 0x0087a520: ctf_globals ctf_globals_network         the replicated copy
// global 0x006b1314: int32_t ctf_neutral_flag_id             game_engine_ctf_pick_random_flag
// global 0x006b1050: king_globals king_globals               hill_state / hill_ticks / occupant
// global 0x0087a9a0: king_hill_marker_history king_hill_markers
// global 0x006b0f50: int32_t king_starting_location_count    0x461080 result count

// player update queues (update_server_new @0x472aa0, update_client_new @0x472f40)
// global 0x006f1d88: uint8_t update_server_initialized
// global 0x006f1d8c: int32_t update_server_tick              index into the 32-deep ring
// global 0x006f1d90: data_array *update_server_queues        "update server queues", 16 x 0x64
// global 0x006f1d94: update_record update_server_history[32]
// global 0x006f7e98: uint8_t update_client_initialized
// global 0x006f7e9c: int32_t update_client_base_tick
// global 0x006f7ea0: int32_t update_client_unknown_ea0       seeded to -1
// global 0x006f7ea4: player_action update_client_staged[1]   the local the player staged action;
//                      src/game keeps the raw "uint32_t update_client_staged[8]" spelling at
//                      this address because three files copy it eight dwords at a time
// global 0x006f7ec4: int32_t update_client_staged_pending     0x473270 decrements it
// global 0x006f7ec8: uint32_t update_client_staged_flag_latch update_client_staged[0] & 0x4d0
// global 0x006f7ecc: int32_t update_client_staged_count       incremented per staged action
// global 0x006f7ed0: data_array *update_client_queues        "update client queues", 16 x 0x64
// global 0x006f7ed4: update_record update_client_history[128]
// global 0x006887b0: int32_t update_client_write_cursor      masked with 0x7f

// random numbers
// global 0x00719cd0: uint32_t random_seed_global            seed = seed * 0x19660d + 0x3c6ef35f

// save games
// global 0x00721330: file_reference savegame_index_file     rebuilt in place, 0x10c bytes
// global 0x00721440: network_mutex_record *savegame_index_mutex   (networking.h; same 4 bytes)
//                      WaitForSingleObject(savegame_index_mutex->handle, 5000) (R14)
// global 0x00721f28: char *user_save_path_default
// global 0x00721f30: char user_save_paths[8][0x105]
// global 0x00722758: uint32_t user_save_path_handles[8]

// ---------------------------------------------------------------------------
// globals this module reads but does not own
// ---------------------------------------------------------------------------
// 0x008603b0  data_array *object_headers   (objects)  stride 0x0c, object pointer at +0x08
// 0x0087bc14  tag_instance *tag_instances  (cache)    stride 0x20, tag data at +0x14
// 0x00746f8c  Scenario *global_scenario    (cache)    +0x348 player_starting_profile,
//                                                     +0x354 player_starting_locations,
//                                                     +0x378 netgame_flags,
//                                                     +0x384 netgame_equipment
// 0x00746fa0  Globals *global_globals      (cache)    TagReflexive is 12 bytes, so the .pointer
//                                                     halves this module reads are
//                                                     +0x114 player_control,
//                                                     +0x150 weapon_list,
//                                                     +0x168 multiplayer_information,
//                                                     +0x174 player_information.
//                                                     CORRECTED: an earlier revision of this
//                                                     list called +0x114 player_information;
//                                                     player_information is +0x174, and it is
//                                                     +0x174 +0x84 (stun_turning_penalty)
//                                                     that 0x4710b0 multiplies by the unit
//                                                     stun meter.
// 0x00712498  local_player_input_state[]   (input)    stride 0x28 per local player
// 0x006b2ce8  int32_t joystick_slot_devices[4] (input) slot -> input-device index, -1 = none.
//                                                     CORRECTED (R02): formerly listed here as
//                                                     "team_slot_table". Input seeds it to -1
//                                                     (0x481974) and stores devices (0x4818fc);
//                                                     0x473743 in local_player_find_free_slot_index
//                                                     is the only game-side reader.
// 0x006ac5b0  director+0x50                (camera)   i.e. camera.h directors[] (array at
//                                                     0x006ac560, stride 0xf8 per local
//                                                     player) field block +0x50. Byte +1
//                                                     (0x006ac5b1) = director.suppress_look_update
//                                                     (skips the look update); byte +2
//                                                     (0x006ac5b2) = director.look_input_consumed
//                                                     (blanks the whole control input). Writers
//                                                     0x445f90 / 0x446870, readers 0x471ae0 /
//                                                     0x4c6f30. (R03)
// 0x00719720  int16_t network_game_mode    (network)  0 local, 1 client, 2 host, 3 replay.
//                                                     CORRECTED: every one of the ~180 accesses
//                                                     in the image is a 16-bit one (97 of them
//                                                     "cmp WORD PTR ds:0x719720,0x2"), so it is
//                                                     an int16, not an int32.
// 0x0071c2d4  void *network_session         (network)  +0x10c the variant, +0x13c its
//                                                     game_engine_index, +0xa0f the
//                                                     end-of-game flag
// 0x0071c2d8  void *network_client          (network)
// 0x006e2dc8  uint8_t *game_state_base      (saved_games)  the game-state arena
// 0x006e2dcc  int32_t game_state_cursor     (saved_games)
// 0x006e2dd4  uint32_t game_state_crc       (saved_games)

#pragma pack(pop)
