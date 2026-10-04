#pragma once
// Blam main module (halo.exe 1.0.10 retail, 0x43ed20 plus 0x4c6390..0x4ca1a0, 51 Ghidra functions).
// The top level of the engine: the main loop (0x4c7610, Ghidra name game_state_save_core, and its
// split-off tail 0x4c7f10), the frame pacer and timers (0x4c6e80, 0x4c9f30, 0x4c9f90), the queued
// map / revert / save / core requests the loop services (0x4c8740..0x4c8d40, 0x4c95f0..0x4c9dd0),
// the developer console front end (0x4c6390..0x4c6bc0), the connect-by-address staging with its
// hostname worker thread (0x4c8340..0x4c8660), the per frame view setup (0x4c8da0..0x4c9260),
// the -timedemo benchmark (0x4c6f30), screenshots and movie frame capture (0x4c9530, 0x4ca1a0)
// (the intro movie player was removed).
//
// Offsets in comments are byte offsets from the struct base. Almost every global here is reached
// through an absolute address, never through a base pointer, so the struct groupings below are
// pinned by the addresses and by the access widths in the disassembly (every access to
// 0x006b7020..0x006b7aa8 and 0x00719700..0x00719b70 in the image was enumerated; see
// out/phase4/main_types_notes.md). Where the binary carries more than that it is called out:
//
//   - console_globals.terminal is the terminal_console of types/interface.h: console_toggle
//     0x4c6530 passes 0x006b7024 to console_open and console_paste_clipboard_text 0x4c6570
//     compares console_active (0x006b2f0c) against it. Every field the module touches inside it
//     (key_event_count +0x00, key_events[i].key_code +0x04 + i*4, color +0x84, prompt +0x94,
//     input +0xb4, edit.cursor +0x1ba, edit.selection_anchor +0x1bc) lands where interface.h
//     puts it.
//
//   - The render_view records this module fills (the main owned array 0x00719b70 and the
//     pregame view 0x006b79e8) are types/render.h render_view (0xac). FUN_004c8f20 writes
//     0x006b79e8..0x006b7a83 field by field and copies 0x15 dwords from +0x58 to +0x04; the
//     next referenced global, the timedemo shader string, starts exactly at +0xac (0x006b7a94).
//
//   - The scenario load request that game_scenario_session_begin 0x4c95f0 takes in EAX and
//     copies (0x43 dwords) to game globals +0x08 is types/networking.h
//     network_scenario_load_request (0x10c). Main builds it on the stack in 0x4c8930, 0x4c9770
//     and 0x4c9dd0: zero 0x43 dwords, +0x06 = 1 then the pending difficulty 0x00696564,
//     +0x08 = 0xdeadbeef, +0x0c = the scenario path. Its +0x06 is therefore the difficulty
//     (game globals +0x0e, which types/saved_games.h already pins as difficulty), not a seed.
//
//   - The file_reference built on the stack by 0x4c9530 and 0x4ca1a0 (0x43 dword memset,
//     signature filo at +0x00, flags byte +0x04, location -1 at +0x06) is types/saved_games.h
//     file_reference_record; the bitmap descriptor screenshot_render allocates (0x30 bytes,
//     signature bitm, width +0x04, height +0x06, depth 1 +0x08, type 0 +0x0a, format 10 +0x0c,
//     flags 0x40 or 0x41 +0x0e, pixel buffer +0x2c) is types/tags.h BitmapData.
//
//   - FUN_004c9050 takes an observer_camera (types/camera.h, 0x3c) in EAX: it reads position
//     +0x00, forward +0x20, up +0x2c and field_of_view +0x38, exactly that layout.
//
//   - Every pointer field is held as uint32_t with the pointee type written first in its
//     comment (the render.h / effects.h convention), so the 32 bit sizes survive a 64 bit host.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h      datum_index, data_array
//   types/interface.h   terminal_console, text_edit_state, ui_key_event, console_message,
//                       map_list_entry (the list the multiplayer map table below is fed into)
//   types/input.h       input_key (the console reacts to _input_key_tab, _input_key_enter,
//                       _input_key_numpad_enter, _input_key_up and _input_key_down), input_mode_flags
//   types/render.h      render_view, k_maximum_render_views
//   types/rasterizer.h  render_camera
//   types/camera.h      observer_camera
//   types/game.h        game_time_globals (+0x00 byte is read and written here, see the notes),
//                       player_globals (+0x04 local_players, +0x0c the local player count),
//                       player (+0x02 local_player_index, stride 0x200)
//   types/networking.h  network_scenario_load_request
//   types/saved_games.h file_reference_record, saved_player_profile (flags bit 2, see below)
//   types/tags.h        BitmapData, Rectangle2D, ColorARGB, Point3D, Vector3D
//
// Functions in this address range that are misnamed, misattributed, or not functions at all are
// listed at the end of out/phase4/main_types_notes.md.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// ---------------------------------------------------------------------------
typedef enum main_constants {
    k_main_path_length = 0x100,                   // the three map name buffers: strncpy 0xff,
                                                  // then byte 0xff cleared
    k_main_connect_address_length = 0x20,         // strncpy 0x1f, byte 0x1f cleared
    k_main_connect_password_length = 9,           // strncpy 8, byte 8 cleared
    k_main_frame_time_history_count = 0x10,       // game_frame_rate_average_update caps at 0x10
    k_main_frame_time_clamp_ms = 200,             // the loop clamps the newest sample to 200
    k_main_save_retry_frames = 10,                // main_save_map_private re-arms the countdown
    k_main_save_give_up_attempts = 0xf0,          // attempts above 0xef give up when timed out
    k_main_save_safe_streak = 3,                  // the save goes through when the streak was
                                                  // already above 2 at a fresh safe check
    k_main_revert_delay_frames = 0x5a,            // lost_map waits until the counter passes 0x5a
    k_main_respawn_delay_frames = 0x5a,           // same for respawn_coop_players; hs arms 0x5b
    k_main_level_transition_fade_ms = 1000,       // main_level_transition_update fade out
    k_main_idle_gameplay_grace_ms = 10000,        // the idle test in the loop
    k_main_hostname_resolve_timeout_ms = 10000,   // WaitForSingleObject in 0x4c8370
    k_main_hostname_thread_stack_size = 0x10400,  // CreateThread in 0x4c8370
    k_main_connect_thread_stack_size = 0x4000,    // CreateThread in 0x4c8500
    k_main_campaign_level_count = 10,             // FUN_004c8b90 table, campaign_level_advance
    k_main_multiplayer_map_count = 19,            // 0x0068e588 .. 0x0068e66c, stride 0x0c
    k_main_screenshot_scale_maximum = 3,          // screenshot_render clamps 0x00696568 to 1..3
    k_main_movie_frame_path_length = 0x200,       // _snprintf limit of movie frame%06d.tga
    k_main_screenshot_path_length = 0x200         // sprintf buffer of screenshot_render
} main_constants;

// main_globals.game_connection. 0x4c9dd0 stores 3 when start_film_playback is set; the loop
// quits on 3 (film playback is not supported by this build); main_loop_shutdown_cleanup
// 0x4c9e90 disposes the client on 1 and the host on 2. Read as a 16 bit value by ~240
// instructions across the image (types/game.h calls the same global network_game_mode).
typedef enum game_connection {
    _game_connection_local = 0,
    _game_connection_network_client = 1,
    _game_connection_network_server = 2,
    _game_connection_film_playback = 3
} game_connection;

// The -timedemo script, keyed on the frame counter at 0x007196d8 (timedemo_benchmark_update
// 0x4c6f30). Each step fires once; any other value just increments the counter.
typedef enum timedemo_step {
    _timedemo_step_load_a30 = 100,                // main_queue_map_change with the a30 name
    _timedemo_step_load_b30 = 0x44c,              // hs map_name b30
    _timedemo_step_load_c10 = 0x898,              // hs map_name c10
    _timedemo_step_load_d20 = 0xc4e,              // hs map_name d20
    _timedemo_step_report = 0x125c                // writes timedemo.txt and sets quit
} timedemo_step;

// Frame time thresholds of the timedemo buckets, in milliseconds (strictly greater than).
typedef enum timedemo_bucket_threshold {
    k_timedemo_below_60fps_ms = 0x10,
    k_timedemo_below_50fps_ms = 0x14,
    k_timedemo_below_40fps_ms = 0x19,
    k_timedemo_below_30fps_ms = 0x21,
    k_timedemo_below_25fps_ms = 0x28,
    k_timedemo_below_20fps_ms = 0x32,
    k_timedemo_below_15fps_ms = 0x42,
    k_timedemo_below_10fps_ms = 100,
    k_timedemo_below_5fps_ms = 200,
    k_timedemo_bucket_count = 9
} timedemo_bucket_threshold;

// Command context mask built by FUN_004c69c0 (console_process_command passes it in EDX to
// chimera__autocomplete_gather 0x483c90, which stores it in hs_autocomplete_gametype_mask).
// Low byte: contexts that are active; high byte: contexts that are forbidden, and a forbidden
// bit clears the matching low bit. The caller ORs in its own bits: 0 from the console,
// 0x2000 from console_exec_file_run. The names describe what 0x4c69c0 tests, not what the hs
// function table means by each bit (types/hs.h names the same bits by multiplayer game type).
typedef enum console_command_context_flags {
    _console_context_default_bit = 0x0001,        // always set; cleared when the caller forbids it
    _console_context_host_bit = 0x0002,           // game_connection 2
    _console_context_unknown_04 = 0x0004,         // never set here; clients forbid it (0x0400)
    _console_context_multiplayer_bit = 0x0008,    // a multiplayer game engine is loaded (0x006f1d20)
    _console_context_no_multiplayer_bit = 0x0010, // no multiplayer game engine
    _console_context_unknown_20 = 0x0020,         // never set here; forbidden (0x2000) while
                                                  // the player profile flags bit 2 is clear
                                                  // (credits_load_directly_for_endgame sets that
                                                  // bit) or a multiplayer engine is loaded
    _console_context_always_bit = 0x0040,         // always set
    k_console_context_forbidden_shift = 8,
    k_console_context_client_forbidden = 0x0600,  // game_connection 1 forbids host and unknown_04
    k_console_context_exec_file = 0x2000          // console_exec_file_run passes this
} console_command_context_flags;

typedef enum console_constants {
    k_console_history_count = 8,                  // ring of command lines, index & 7
    k_console_history_line_length = 0xff,         // stride of console_globals.history
    k_console_exec_line_length = 200              // fgets buffer of console_exec_file_run (199)
} console_constants;

// ---------------------------------------------------------------------------
// console_globals  (0x006b7020; console_initialize 0x4c62d0, console_deactivate 0x4c64b0,
// console_toggle 0x4c6530, console_paste_clipboard_text 0x4c6570, FUN_004c65c0 (the per frame
// key handler), console_process_command 0x4c6a80, console_autocomplete_command 0x4c6bc0)
// Every access is absolute. console_initialize fills color and prompt from .data, clears the
// input line and history count, sets both history indices to -1 and sets enabled when the
// command line carries -console. The 8 line history is a ring: newest_index advances mod 8 on
// every submitted command, browse_index walks back from it with the up / down keys.
// ---------------------------------------------------------------------------
typedef struct console_globals {
    uint8_t active;                   // 0x000 the console is open; console_toggle stores the
                                      //       console_open result. Also read by the camera view
                                      //       setup 0x4c9050, sound_update, chat and the screen
                                      //       flash code (types/effects.h formerly called it
                                      //       player_effect_suppressed; R08)
    uint8_t enabled;                  // 0x001 -console was on the command line
    uint8_t unknown_002[2];           // 0x002 never referenced
    terminal_console terminal;        // 0x004 types/interface.h, 0x1be bytes, handed to
                                      //       console_open; input is terminal.input (0x006b70d8)
    uint8_t pad_1c2[2];               // 0x1c2 never referenced (alignment)
    char history[k_console_history_count][k_console_history_line_length];
                                      // 0x1c4 submitted lines, stride 0xff, copied with strcpy
    int16_t history_count;            // 0x9bc saturates at 8
    int16_t history_newest_index;     // 0x9be ring slot of the newest line, -1 when empty
    int16_t history_browse_index;     // 0x9c0 lines back from the newest, -1 when not browsing;
                                      //       clamped to history_count - 1
    uint8_t pad_9c2[2];               // 0x9c2 never referenced (alignment)
} console_globals;                    // size 0x9c4
static_assert(sizeof(console_globals) == 0x9c4, "console_globals layout");

// ---------------------------------------------------------------------------
// main_globals  (0x00719700 .. 0x00719aaf)
// The engine state block the main loop polls. Requests are raised by the hs evaluators
// (0x47f500..0x47fca0, 0x4828xx), the interface, the game engine and the networking code, and
// serviced once per frame by the main loop 0x4c7610. Field widths come from the access sizes
// (BYTE / WORD / DWORD PTR, or the register width of the moves); the end of the block at 0xaaf
// is an inference: 0x00719ab0 onward is touched only by the frame rate average and the
// timedemo code, which are kept as separate records below.
// CEA / OpenSauce call the block main_globals; their field names were used as hints only and
// every name below is backed by what the retail code does with the field.
// ---------------------------------------------------------------------------
typedef struct main_globals {
    uint32_t frame_counter_low;       // 0x000 QueryPerformanceCounter at the start of the frame
    uint32_t frame_counter_high;      // 0x004 (the pacer subtracts it from the fresh reading;
                                      //       timedemo advances it by frequency / 30)
    uint32_t frame_time_ms;           // 0x008 the same instant in milliseconds; timedemo adds 0x21
    uint8_t unknown_00c[4];           // 0x00c never referenced
    uint32_t render_counter_low;      // 0x010 QueryPerformanceCounter of the last render_frame
    uint32_t render_counter_high;     // 0x014
    uint8_t frame_time_overflow;      // 0x018 the raw frame time exceeded one second; handed to
                                      //       update_server_send_update
    uint8_t unknown_019[3];           // 0x019 never referenced
    float frame_delta_time;           // 0x01c seconds, clamped to 0..1 and to 1/15 (1/30 when
                                      //       0x00710301 is set) in a local game
    int16_t game_connection;          // 0x020 game_connection
    uint16_t screenshot_index;        // 0x022 file number of the next screenshot
    uint32_t movie_frame_bitmap;      // 0x024 BitmapData * the frame capture writes into;
                                      //       non NULL means movie capture is on
    uint8_t unknown_028[8];           // 0x028 never referenced
    int32_t movie_frame_index;        // 0x030 number of the next movie frame file
    float movie_frame_delta_time;     // 0x034 fixed frame time used while capturing
    uint8_t reset_map;                // 0x038 reload the current map in place (bsp switch,
                                      //       input reset, game re-initialise)
    uint8_t level_transition;         // 0x039 run main_level_transition_update this frame
    uint8_t revert_map;               // 0x03a revert to the last checkpoint
    uint8_t revert_map_if_allowed;    // 0x03b revert only when 0x006e3000 is clear and the game
                                      //       engine allows it (engine +0x0a)
    uint8_t save_map;                 // 0x03c run main_save_map_private this frame
    uint8_t save_map_require_safe;    // 0x03d wait for game_safe_to_save; clear means save now
                                      //       and print unsafe save
    uint8_t save_map_with_timeout;    // 0x03e give up after k_main_save_give_up_attempts
    uint8_t save_map_write_pending;   // 0x03f the loop writes the checkpoint (game_state_queue_write)
    int32_t save_map_retry_countdown; // 0x040 frames until the next safety check
    int32_t save_map_attempt_count;   // 0x044
    uint32_t level_transition_fade_end_ms; // 0x048 frame_time_ms + 1000 while the menu music
                                      //       fades; 0 when no fade is running
    int16_t save_map_safe_streak;     // 0x04c consecutive successful safety checks
    uint8_t won_map;                  // 0x04e campaign_level_advance
    uint8_t lost_map;                 // 0x04f revert after k_main_revert_delay_frames
    uint8_t respawn_coop_players;     // 0x050 FUN_00473e90 after k_main_respawn_delay_frames
    uint8_t save_core;                // 0x051 write core.bin
    uint8_t load_core;                // 0x052 game_state_load_core
    uint8_t load_core_next_session;   // 0x053 moved into load_core by game_scenario_session_begin
    int16_t switch_structure_bsp_index; // 0x054 -1 for none; FUN_004c9b60 switches and resets it
    uint8_t main_menu_scenario_loaded;  // 0x056 levels/ui/ui is the loaded map
    uint8_t return_to_main_menu;        // 0x057 main_menu_return_and_reset this frame
    uint8_t unknown_058;              // 0x058 cleared by the loop when set, never set
    uint8_t idle_timeout_reached;     // 0x059 set with level_transition by the idle test while
                                      //       ui_split_screen is set; main_level_transition_update
                                      //       then skips the load, clears return_to_main_menu
                                      //       and sets unknown_05a
    uint8_t unknown_05a;              // 0x05a set by main_level_transition_update only, never read
    uint8_t quit;                     // 0x05b the loop runs main_loop_shutdown_cleanup and returns
    int32_t idle_timeout_ms;          // 0x05c the idle test is armed when positive; never
                                      //       written in this build, UNSURE
    int32_t last_gameplay_time_ms;    // 0x060 refreshed every frame of active gameplay
    int32_t last_activity_time_ms;    // 0x064 refreshed on input, console use and session start
    uint8_t start_film_playback;      // 0x068 set by hs; 0x4c9dd0 then selects connection 3
    uint8_t time_is_running;          // 0x069 multiplies the simulation and camera shake delta
    uint8_t reset_frame_timers;       // 0x06a re-baseline both counters, then set time_is_running
    uint8_t unknown_06b;              // 0x06b set to 1 by chimera__load_ui_map, never read
    uint8_t skip_ticks;               // 0x06c run game_engine_flush_pending_simulation_ticks
    uint8_t unknown_06d;              // 0x06d never referenced
    int16_t skip_tick_count;          // 0x06e ticks to run back to back at 1/30 s
    int16_t lost_map_frames;          // 0x070
    int16_t respawn_coop_frames;      // 0x072
    uint8_t cache_file_open_pending;  // 0x074 open pending_cache_file_name (FUN_004c8900 sets)
    uint8_t unknown_075[3];           // 0x075 never referenced
    uint8_t restore_checkpoint_on_load; // 0x078 main_queue_map_change sets it;
                                      //       game_scenario_session_begin then calls
                                      //       game_state_load_checkpoint
    char scenario_path[k_main_path_length];        // 0x079 the map the next session loads
    char multiplayer_map_name[k_main_path_length]; // 0x179 main_queue_map_change_by_name_or_clear
    char pending_cache_file_name[k_main_path_length]; // 0x279 FUN_004c8900
    uint8_t connect_pending;          // 0x379 connect to connect_address this frame
    char connect_address[k_main_connect_address_length];   // 0x37a dotted address[:port]
    char connect_password[k_main_connect_password_length];  // 0x39a
    uint8_t unknown_3a3[5];           // 0x3a3 never referenced
    uint8_t disable_frame_output;     // 0x3a8 skips rendering and movie capture; only read
    uint8_t debug_game_save;          // 0x3a9 prints the unsafe save / gave up messages and is
                                      //       read by game_safe_to_save; only read
    uint8_t unknown_3aa[2];           // 0x3aa never referenced
    int16_t screenshot_tile_count;    // 0x3ac pending screenshot: n by n tiles, 0 when idle
    uint8_t unknown_3ae[2];           // 0x3ae never referenced
} main_globals;                       // size 0x3b0
static_assert(sizeof(main_globals) == 0x3b0, "main_globals layout");

// ---------------------------------------------------------------------------
// main_frame_rate_average  (0x00719ab0; game_frame_rate_average_update 0x4c6e80 and the end of
// the main loop). The loop stores now - sample_time_ms (clamped to 200) in history[0]; the
// update returns the mean of the first count entries, shifts them up one slot and grows count
// to 16. The mean is compared against 0x006894bc to decide whether to skip rendering.
// ---------------------------------------------------------------------------
typedef struct main_frame_rate_average {
    int32_t sample_time_ms;           // 0x00 performance counter in milliseconds
    uint8_t unknown_04[4];            // 0x04 never referenced
    int32_t history[k_main_frame_time_history_count]; // 0x08 milliseconds, newest first
    int32_t count;                    // 0x48
} main_frame_rate_average;            // size 0x4c
static_assert(sizeof(main_frame_rate_average) == 0x4c, "main_frame_rate_average layout");

// ---------------------------------------------------------------------------
// timedemo_bucket / timedemo_globals  (0x00719afc; timedemo_benchmark_update 0x4c6f30, and the
// main loop for last_game_time). Once per game tick the benchmark adds the frame time to
// total_time_ms and to every bucket whose threshold it exceeds. The report prints, per bucket,
// the share of time (time_ms / total_time_ms) and of frames (frames / frame_count).
// ---------------------------------------------------------------------------
typedef struct timedemo_bucket {
    uint32_t time_ms;                 // 0x00
    uint32_t frames;                  // 0x04
} timedemo_bucket;                    // size 0x08
static_assert(sizeof(timedemo_bucket) == 0x8, "timedemo_bucket layout");

typedef struct timedemo_globals {
    uint32_t frame_count;             // 0x00
    uint32_t total_time_ms;           // 0x04
    timedemo_bucket buckets[k_timedemo_bucket_count]; // 0x08 below 60, 50, 40, 30, 25, 20, 15,
                                      //      10 and 5 fps (timedemo_bucket_threshold order)
    uint8_t unknown_50[8];            // 0x50 never referenced
    uint32_t current_time_ms;         // 0x58 FUN_00449210 clock
    uint32_t previous_time_ms;        // 0x5c
    uint32_t frame_time_ms;           // 0x60 1 on the first frame
    int32_t last_game_time;           // 0x64 game_time_globals.game_time the benchmark last ran
                                      //      for; the loop calls it once per new tick
} timedemo_globals;                   // size 0x68
static_assert(sizeof(timedemo_globals) == 0x68, "timedemo_globals layout");

// ---------------------------------------------------------------------------
// multiplayer_map_table_entry  (.data 0x0068e588, 19 entries; the main loop feeds each one to
// map_list_add_entry 0x4950c0 before the first frame: EAX = name, the stacked argument = map_id)
// ---------------------------------------------------------------------------
typedef struct multiplayer_map_table_entry {
    int32_t map_id;                   // 0x00 0 .. 0x12
    uint32_t name;                    // 0x04 char * (beavercreek, sidewinder, ...)
    int32_t unknown_08;               // 0x08 1 for the first 13 entries, 0 after; not read here
} multiplayer_map_table_entry;        // size 0x0c
static_assert(sizeof(multiplayer_map_table_entry) == 0xc, "multiplayer_map_table_entry layout");

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x00719700: main_globals main_globals
// global 0x00719ab0: main_frame_rate_average main_frame_rate_average
// global 0x00719afc: timedemo_globals timedemo_globals
// global 0x007196dc: int32_t timedemo_last_frame_index        low dword of the rasterizer frame
//                    counter 0x0069c648 the benchmark last sampled
// global 0x00719b64: uint32_t connect_thread                  HANDLE of the
//                    network_game_client_connect_by_hostname worker; 0 when idle
// global 0x00719b68: int32_t hostname_resolve_complete        set by the resolver thread
// global 0x00719b6c: uint32_t hostname_resolve_result         struct hostent * from gethostbyname
// global 0x00719b70: render_view render_views[2]              types/render.h; the local player
//                    views followed by the trailing non player view
// global 0x006b7020: console_globals console_globals
// global 0x006b79e8: render_view pregame_render_view          FUN_004c8f20, handed to the pregame
//                    frame 0x50c590 (types/render.h)
// global 0x006b7a94: char timedemo_pixel_shader_version[0x14]  %d.%d; ends at 0x006b7aa8
// global 0x00696554: ColorARGB console_default_color          (1, 1, 0.3, 1), console_initialize
// global 0x00696564: int16_t main_pending_difficulty          1 by default; copied into every
//                    scenario load request built here and compared by the checkpoint loader
// global 0x00696568: int16_t screenshot_scale                 clamped to 1..3; the render and
//                    decal code read it while a tiled screenshot is taken
// global 0x00696570: uint8_t main_unknown_696570              cleared once before the loop
// global 0x00696574: char *campaign_level_paths[10]           levels a10 .. d40 scenario paths
// global 0x0068e588: multiplayer_map_table_entry multiplayer_maps[19]
// .rdata 0x00669a38: char campaign_level_short_names[10][4]    d40 first, a10 last; FUN_004c8b90
//                    tests them in reverse order with strstr
//
// ---------------------------------------------------------------------------
// globals this module reads or writes but does not own
// ---------------------------------------------------------------------------
// 0x007196d4  int32_t (UNSURE owner) nonzero aborts movie playback; hs quit writes it
// 0x007196d8  int32_t -timedemo frame counter (types/game.h game_time_force_single_tick);
//             written by shell_winmain, advanced by timedemo_benchmark_update
// 0x007196e0 / 0x007196e4 / 0x007196e8 / 0x007196f4  shell command line BOOLs (types/shell.h)
// 0x007196d3  uint8_t error file logging switch read by the three console printers
// 0x006b2efc / 0x006b2f00 / 0x006b2f04 / 0x006b2f08 / 0x006b2f0c / 0x006b2f1c  terminal state
//             (types/interface.h)
// 0x006b1620 / 0x006b168d / 0x006b1800  key state arrays and the DirectInput keyboard (input)
// 0x00710328  input_globals (types/input.h input_abstraction_globals): states[0] 0x00712498
//             (buttons[0x12] at 0x007124aa requests a screenshot), idle 0x00712540, mode_flags
//             0x00712542, system_key_states[3] 0x007127d0..0x007127d2 (grave, escape, print; print
//             requests a screenshot); the main loop clears states[0] and the three key states on a
//             map reset
// 0x00712cc0  event_queue (types/input.h input_event_queue); the loop pushes an empty event to
//             queue 0 when no event arrived since the previous frame
// 0x006f1d6c  game_time_globals *; 0x006f187c game engine globals; 0x006f1d20 game engine
// 0x0087a478 / 0x0087a480  player_globals / players (types/game.h)
// 0x006b0b80  game globals * (scenario load request copied to +0x08)
// 0x006ac6d0  observer cameras (types/camera.h), passed to FUN_004c9050
// 0x0069c634 / 0x0069c63c  game window and screen rectangles (rasterizer)
// 0x0069c648  int64 rasterizer frame counter; 0x0069c65c / 0x0069c660 default clip distances
// 0x00719ccc  int32 counter bumped and dropped around render_frame_all_views (types/effects.h
//             lists it as player_effect_reentry_count)
// 0x006ac8f8  performance counter frequency (math)
// 0x00718f8c .. 0x00718fcd  interface globals (progress screen, ui_split_screen, music)
// 0x00712dd8 / 0x00712ef4 / 0x00714dd4  the player profile record, its flags word and slot
// 0x0071c2d4 / 0x0071c2d8  network session / client
// 0x006b85{9c,a0,a4,a8,ac,b0}  ban list and buffer pools (types/networking.h)
// 0x006894bc  int32_t render skip threshold in ms (UNSURE owner): -1 disables, otherwise clamped
//             to at least 20; the loop drops the frame when the frame time average reaches it
// 0x006894b0  uint32_t default bandwidth graph sample interval (UNSURE owner), copied into
//             0x00719ce0 +0x08 by the loop
// 0x0071d1a4  int32_t -checkfpu: the loop runs finit / fldcw 0x7e every frame while set
// 0x00710301  uint8_t (UNSURE owner) selects the 1/30 s local frame delta clamp instead of 1/15 s
// 0x007196e8 / 0x007196f4 / 0x0071d1a8  -novideo or -connect, safe mode, -window: any of them (or
//             -timedemo) skips the three intro movies

#pragma pack(pop)
