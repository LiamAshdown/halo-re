# `main` — the engine main loop, console front end, timing and view setup

Retail Halo PC `halo.exe` 1.0.10, `0x43ed20` plus `0x4c6390 .. 0x4ca493` (48 functions,
16,594 bytes of code), plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86. Every file in this
directory is one function, rewritten from its Ghidra decompilation (or, where Ghidra fails,
from the raw `objdump -d -M intel` disassembly) against `types/main.h`, with the original
decompile preserved verbatim at the bottom of the file inside `#if 0 ... #endif`.

Gate: `python tools/build_check.py main` → **48 ok, 0 failed**. The type smoke test
`out/phase4/main_smoke.c` passes with and without `-m32`, and every file also builds with
`-m32` with no warning outside the `#if 0` blocks.

The module list (`out/phase4/main_functions.md`) has 51 entries; three of them are not
functions (see *Misattributed entries* below), so 48 files cover the whole module.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| intro movies | `0x43ed20` | `movie_play_bink`: a modal Bink player (its own object file, far from the rest) |
| console | `0x4c6390`–`0x4c6e7f` | `-exec` / `init.txt`, open / close / toggle, clipboard paste, the per frame key handler, the three console printers, command history, the hs context mask, command execution and tab completion |
| timing | `0x4c6e80`, `0x4c9f30`, `0x4c9f90` | the 16 sample frame time average, timer re-baseline, the 30 Hz frame pacer |
| `-timedemo` | `0x4c6f30` | benchmark bucket sampling, the scripted a30 / b30 / c10 / d20 run and `timedemo.txt` |
| main loop | `0x4c7610`–`0x4c8333` | `main_loop`: start-up, then the per frame request servicing, input, network, simulation and render |
| connect staging | `0x4c8340`–`0x4c873f` | `-connect` / join by `host[:port]`: a resolver thread with a 10 s timeout, then a staged numeric address the loop connects to |
| map / session requests | `0x4c8740`–`0x4c8d9a`, `0x4c95f0`–`0x4c9e8a` | queue a map, open a cache file, load the UI map, return to the main menu, campaign advance, credits, session begin, level transition, tick skipping, checkpoint save, bsp switch, first map start, shutdown |
| views | `0x4c8da0`–`0x4c952f` | split screen rectangles, pregame view, per player camera fill, local view count, the per frame view list and render dispatch |
| capture | `0x4c9530`, `0x4ca1a0` | movie frame `.tga` export and tiled screenshots |

The main loop, per frame (see `main_loop.c` for the exact order):

```
game_frame_rate_average_update            (-checkfpu: finit / fldcw 0x7e)
service main_globals requests             bsp switch, lost map revert (after 0x5a frames),
                                          won map advance, coop respawn, checkpoint write,
                                          level transition, revert, reset map, core save/load,
                                          main menu return, tick skip, cache open, connect
input poll, shell_pump_windows_messages   quit -> main_loop_shutdown_cleanup
network update and bandwidth graph        client / host health checks -> chat_close
main_loop_frame_pacer, ui_cursor_update, interface_tick
idle tracking                             last_activity / last_gameplay (idle quit never armed)
no game: render_pregame_view_initialize   else: console, simulation (unless the console holds
                                          a local game), camera, observer, end game sequence
main_save_map_private                     render skip threshold 0x006894bc against the average
timedemo_benchmark_update or render       render_frame_all_views(leftover_time, clamped delta)
movie_capture_frame_export                reset_frame_timers, MsgWait while inactive, new sample
```

## Struct layouts

All of these live in `types/main.h`; offsets are byte offsets, `#pragma pack(push,1)` is in
force, and every access in the image to these ranges was enumerated with its width (see
`out/phase4/main_types_notes.md`). Foreign records the module fills (`render_view`,
`observer_camera`, `network_scenario_load_request`, `file_reference_record`, `BitmapData`,
`terminal_console`, `game_time_globals`, `player_globals`, `input_abstraction_globals`) are reused
from their own headers and not repeated here.

### `console_globals` — size `0x9c4`, global `0x006b7020`

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint8_t` | `active` — the console is open |
| `0x001` | `uint8_t` | `enabled` — `-console` was given |
| `0x002` | `uint8_t[2]` | never referenced |
| `0x004` | `terminal_console` | `terminal` (types/interface.h, `0x1be`); `input` at `0x006b70d8`, `edit.cursor` at `0x006b71de` |
| `0x1c2` | `uint8_t[2]` | never referenced |
| `0x1c4` | `char[8][0xff]` | `history` — ring of submitted lines |
| `0x9bc` | `int16_t` | `history_count` (saturates at 8) |
| `0x9be` | `int16_t` | `history_newest_index` (`(x + 1) & 7`, -1 when empty) |
| `0x9c0` | `int16_t` | `history_browse_index` (-1 when not browsing) |
| `0x9c2` | `uint8_t[2]` | never referenced |

### `main_globals` — size `0x3b0`, global `0x00719700`

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint32_t` ×2 | `frame_counter_low/high` — QPC at the start of the frame |
| `0x008` | `uint32_t` | `frame_time_ms` |
| `0x00c` | `uint8_t[4]` | never referenced |
| `0x010` | `uint32_t` ×2 | `render_counter_low/high` — QPC of the last render |
| `0x018` | `uint8_t` | `frame_time_overflow` (raw frame time over 1 s) |
| `0x01c` | `float` | `frame_delta_time` |
| `0x020` | `int16_t` | `game_connection` (0 local, 1 client, 2 server, 3 film playback) |
| `0x022` | `uint16_t` | `screenshot_index` |
| `0x024` | `uint32_t` | `movie_frame_bitmap` (`BitmapData *`, non NULL while capturing) |
| `0x030` | `int32_t` | `movie_frame_index` |
| `0x034` | `float` | `movie_frame_delta_time` |
| `0x038`–`0x03f` | `uint8_t` ×8 | `reset_map`, `level_transition`, `revert_map`, `revert_map_if_allowed`, `save_map`, `save_map_require_safe`, `save_map_with_timeout`, `save_map_write_pending` |
| `0x040` | `int32_t` | `save_map_retry_countdown` |
| `0x044` | `int32_t` | `save_map_attempt_count` |
| `0x048` | `uint32_t` | `level_transition_fade_end_ms` |
| `0x04c` | `int16_t` | `save_map_safe_streak` |
| `0x04e`–`0x053` | `uint8_t` ×6 | `won_map`, `lost_map`, `respawn_coop_players`, `save_core`, `load_core`, `load_core_next_session` |
| `0x054` | `int16_t` | `switch_structure_bsp_index` (-1 none) |
| `0x056` | `uint8_t` | `main_menu_scenario_loaded` |
| `0x057` | `uint8_t` | `return_to_main_menu` |
| `0x058`–`0x05b` | `uint8_t` ×4 | `unknown_058`, `idle_timeout_reached`, `unknown_05a`, `quit` |
| `0x05c` | `int32_t` | `idle_timeout_ms` (never written in this build) |
| `0x060` | `int32_t` | `last_gameplay_time_ms` |
| `0x064` | `int32_t` | `last_activity_time_ms` |
| `0x068`–`0x06c` | `uint8_t` ×5 | `start_film_playback`, `time_is_running`, `reset_frame_timers`, `unknown_06b`, `skip_ticks` |
| `0x06e` | `int16_t` | `skip_tick_count` |
| `0x070` | `int16_t` | `lost_map_frames` |
| `0x072` | `int16_t` | `respawn_coop_frames` |
| `0x074` | `uint8_t` | `cache_file_open_pending` |
| `0x078` | `uint8_t` | `restore_checkpoint_on_load` |
| `0x079` | `char[0x100]` | `scenario_path` |
| `0x179` | `char[0x100]` | `multiplayer_map_name` |
| `0x279` | `char[0x100]` | `pending_cache_file_name` |
| `0x379` | `uint8_t` | `connect_pending` |
| `0x37a` | `char[0x20]` | `connect_address` |
| `0x39a` | `char[9]` | `connect_password` |
| `0x3a8` | `uint8_t` | `disable_frame_output` (only read) |
| `0x3a9` | `uint8_t` | `debug_game_save` (only read) |
| `0x3ac` | `int16_t` | `screenshot_tile_count` |

Gaps `0x028`, `0x06d`, `0x075`, `0x3a3`, `0x3aa`, `0x3ae` are never referenced.

### `main_frame_rate_average` — size `0x4c`, global `0x00719ab0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `sample_time_ms` |
| `0x04` | `uint8_t[4]` | never referenced |
| `0x08` | `int32_t[16]` | `history` — newest first; the loop writes `history[0]` (clamped to 200, unsigned) |
| `0x48` | `int32_t` | `count` (grows to 16) |

### `timedemo_bucket` — size `0x08`; `timedemo_globals` — size `0x68`, global `0x00719afc`

| Off | Type | Field (`timedemo_globals`) |
|---|---|---|
| `0x00` | `uint32_t` | `frame_count` |
| `0x04` | `uint32_t` | `total_time_ms` |
| `0x08` | `timedemo_bucket[9]` | `{ time_ms, frames }` for frame times above 16, 20, 25, 33, 40, 50, 66, 100, 200 ms (below 60 .. 5 fps) |
| `0x50` | `uint8_t[8]` | never referenced |
| `0x58` | `uint32_t` | `current_time_ms` |
| `0x5c` | `uint32_t` | `previous_time_ms` |
| `0x60` | `uint32_t` | `frame_time_ms` (1 on the first frame) |
| `0x64` | `int32_t` | `last_game_time` — game tick the loop last ran the benchmark for |

### `multiplayer_map_table_entry` — size `0x0c`, `.data 0x0068e588`, 19 entries

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `map_id` (0 .. 0x12) |
| `0x04` | `uint32_t` | `name` (`char *`, beavercreek, sidewinder, ...) |
| `0x08` | `int32_t` | `unknown_08` — 1 for the first 13 entries, not read here |

### `bink_movie_prefix` — the first `0x100` bytes of a RAD `HBINK`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `width` (RAD layout, not read) |
| `0x04` | `uint32_t` | `height` (RAD layout, not read) |
| `0x08` | `uint32_t` | `frame_count` |
| `0x0c` | `uint32_t` | `frame_index` — playback ends when it reaches `frame_count` |
| `0xfc` | `int32_t` | `paused` |

## Misattributed entries

- **`0x4c7610` `game_state_save_core`** is the engine main loop (`main_loop`, CEA main_loop).
- **`0x4c7f10` `weapon_stop_reload`** is not a function: it is the return address of the call to
  `game_engine_advance_simulation_ticks` at `0x4c7f0b` inside `main_loop` (0 callers).
- **`0x4c9c60` `console_response_printf`** is the `return_to_main_menu = 1; pop; ret` tail of
  `campaign_level_advance`.
- **`0x4c9dc0` `console_process_command`** is the epilogue of `0x4c9c80`
  (`network_autojoin_from_command_line`, interface module).
- **`0x4c62d0` / `0x4c62f0` / `0x4c6340`** are one function, `console_initialize`
  (outside this list; `main_loop` calls it).

These are recorded as `code` entries in `symbols/agent_phase4_main.txt`, together with the ten
`FUN_` renames (`console_process_key_events`, `console_process_rcon_command`,
`console_command_context_mask`, `main_ensure_local_players`, `main_queue_cache_file_open`,
`campaign_level_find_index_for_path`, `render_pregame_view_initialize`,
`render_view_camera_fill`, `render_local_view_count`, `main_switch_structure_bsp_and_notify`).

## Known gaps

- `types/memory.h` `data_iterator` stops at `+0x0c`; the iterators built on the stack carry a
  fourth dword, `data ^ 'iter'`, at `+0x0c` (`main_loop` 0x4c7f35, and many game / ai files). It
  is kept as a separate local here, as the other modules do.
- `0x006f187c` (called `cinematic_globals` in several modules; bytes `+0x09` and `+0x0a` gate
  HUD, respawn, revert and idle logic here) has no struct anywhere.
- Foreign globals still declared as raw bytes: `0x006f1d38`, `0x006b0b88` (0xc0 dwords),
  `0x0087ab20` (0x26 dwords) in `chimera__load_ui_map`; `0x0087ac00` / `0x0087ac08` in
  `game_scenario_session_begin`; `0x00710301` in the frame pacer; `0x00873d30` in the camera
  fill; `0x00718fa0`, `0x00719230`, `0x0068943c`, `0x006894bc`, `0x006894b0`, `0x0069fdfc`.
- `0x007196d4` (nonzero aborts `movie_play_bink`) has no established owner.
- The idle quit (`main_globals.idle_timeout_ms`) is never armed in this build.
- `timedemo_benchmark_update` names the sound and render option globals after the report
  labels they feed; `0x551620` (`user_profile_signin_state_is_valid` in the game module) gates
  "Environmental Sound= EAX" there, which makes that name doubtful.
- Foreign prototypes that disagree with their own module files (the main files follow the
  disassembly): `rasterizer_device_reset` takes its present parameters on the stack, not EBX;
  `bitmap_group_free` 0x43f880 takes the bitmap in ESI; `scenario_load` 0x53e6a0 takes the
  scenario path in EAX; `update_server_send_update` takes a tick count, not a pointer.

## Phase 4 review

The two rewriters left four functions unwritten (`movie_play_bink`,
`console_autocomplete_command`, `timedemo_benchmark_update`, `main_loop`); all four were then
written from the disassembly. Semantic drift fixed in the other files, all confirmed against
`objdump`:

- dropped register arguments: `cache_file_switch_map_by_path` (EAX path, BL 1) in
  `game_scenario_session_begin`, `game_start_new_single_player_map`,
  `main_level_transition_update` and the second call of `chimera__load_ui_map` (which passed the
  literal instead of `request.map_name`); `scenario_load` (EAX path, not the staging block);
  `scenario_structure_bsp_switch` (SI index); `predicted_resource_list_touch` (ESI) twice;
  `console_close` (EAX); the HUD message player index in
  `main_switch_structure_bsp_and_notify`;
- `screenshot_render`: the console clear flag (AL = 1), the empty format, and the tile / page
  pointers passed to `render_frame` (EBX) and the capture (EAX) were wrong;
- access widths: WORD stores written as dwords (`0x006b2f28`, `0x006b2f68`) in three files,
  a WORD store written as a byte (`0x0087ac08`), a DWORD store written as a byte
  (`0x00719230`);
- `movie_capture_frame_export` passed the file reference base instead of its path (+0x08) to
  `path_append_component`;
- `network_game_client_connect_to_address_async`: normalize buffer 0x20 bytes (was 0x19), AL
  return; `network_game_client_connect_by_hostname` is a `__stdcall` thread procedure;
- float constants written as the floats they are (0.6375f, 0.85f) and the pacer clamps as the
  doubles 1/15 and 1/30;
- aliases folded into the owning struct: `0x00719720` (was `network_game_mode`, six files) is
  `main_globals.game_connection`, `0x00719739` is `main_globals.level_transition`, `0x0071976b`
  is `main_globals.unknown_06b`, `0x00712542` / `0x007127d2` / `0x007124aa` are
  `input_globals` fields, `0x00714dd4` is `saved_player_profile_slots[0].handle`, `0x006b2f28` /
  `0x006b2f68` are `progress_screen_text` / `progress_screen_subtext`.

## Functions

Name and rewrite confidence come from each file header; UNSURE counts the markers above the
`#if 0` block.

| Address | Name | Bytes | Name conf | Rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x43ed20` | `movie_play_bink` | 740 | 0.6 | 0.8 | 2 |
| `0x4c6390` | `chimera__exec_init` | 140 | 0.8 | 0.8 | 1 |
| `0x4c6420` | `console_exec_file_run` | 144 | 0.6 | 0.75 | 0 |
| `0x4c64b0` | `console_deactivate` | 119 | 0.6 | 0.85 | 0 |
| `0x4c6530` | `console_toggle` | 64 | 0.6 | 0.85 | 1 |
| `0x4c6570` | `console_paste_clipboard_text` | 70 | 0.6 | 0.8 | 0 |
| `0x4c65c0` | `console_process_key_events` | 376 | 0.45 | 0.8 | 2 |
| `0x4c67c0` | `console_print_error_va` | 157 | 0.6 | 0.8 | 0 |
| `0x4c6860` | `console_out_printf` | 177 | 0.6 | 0.8 | 0 |
| `0x4c6920` | `console_print_va` | 114 | 0.55 | 0.75 | 2 |
| `0x4c69a0` | `console_process_rcon_command` | 26 | 0.4 | 0.75 | 0 |
| `0x4c69c0` | `console_command_context_mask` | 187 | 0.4 | 0.85 | 2 |
| `0x4c6a80` | `console_process_command` | 320 | 0.8 | 0.7 | 2 |
| `0x4c6bc0` | `console_autocomplete_command` | 697 | 0.65 | 0.8 | 2 |
| `0x4c6e80` | `game_frame_rate_average_update` | 164 | 0.5 | 0.8 | 0 |
| `0x4c6f30` | `timedemo_benchmark_update` | 1755 | 0.6 | 0.8 | 7 |
| `0x4c7610` | `main_loop` | 3364 | 0.8 | 0.7 | 7 |
| `0x4c8340` | `network_hostname_resolve_thread_proc` | 34 | 0.6 | 0.8 | 1 |
| `0x4c8370` | `network_hostname_resolve_with_timeout` | 111 | 0.55 | 0.75 | 0 |
| `0x4c83e0` | `network_game_client_connect_by_hostname` | 274 | 0.6 | 0.8 | 1 |
| `0x4c8500` | `network_game_client_connect_to_address_async` | 341 | 0.6 | 0.8 | 1 |
| `0x4c8660` | `network_game_client_connect_to_resolved_address` | 224 | 0.5 | 0.75 | 1 |
| `0x4c8740` | `main_queue_map_change` | 83 | 0.55 | 0.8 | 1 |
| `0x4c87a0` | `main_queue_map_change_by_name_or_clear` | 91 | 0.5 | 0.75 | 2 |
| `0x4c8800` | `main_ensure_local_players` | 255 | 0.4 | 0.8 | 1 |
| `0x4c8900` | `main_queue_cache_file_open` | 46 | 0.4 | 0.75 | 0 |
| `0x4c8930` | `chimera__load_ui_map` | 303 | 0.6 | 0.75 | 5 |
| `0x4c8a60` | `main_menu_return_and_reset` | 214 | 0.55 | 0.75 | 2 |
| `0x4c8b40` | `main_menu_music_stop` | 73 | 0.55 | 0.8 | 0 |
| `0x4c8b90` | `campaign_level_find_index_for_path` | 428 | 0.4 | 0.8 | 1 |
| `0x4c8d40` | `credits_load_directly_for_endgame` | 91 | 0.8 | 0.8 | 0 |
| `0x4c8da0` | `viewport_split_rect_compute` | 379 | 0.5 | 0.8 | 1 |
| `0x4c8f20` | `render_pregame_view_initialize` | 302 | 0.45 | 0.8 | 0 |
| `0x4c9050` | `render_view_camera_fill` | 459 | 0.45 | 0.8 | 7 |
| `0x4c9220` | `render_local_view_count` | 64 | 0.4 | 0.7 | 0 |
| `0x4c9260` | `render_frame_all_views` | 714 | 0.55 | 0.8 | 3 |
| `0x4c9530` | `movie_capture_frame_export` | 181 | 0.55 | 0.8 | 1 |
| `0x4c95f0` | `game_scenario_session_begin` | 384 | 0.5 | 0.75 | 3 |
| `0x4c9770` | `main_level_transition_update` | 615 | 0.5 | 0.75 | 2 |
| `0x4c99e0` | `game_engine_flush_pending_simulation_ticks` | 141 | 0.55 | 0.8 | 0 |
| `0x4c9a70` | `main_save_map_private` | 228 | 0.8 | 0.8 | 1 |
| `0x4c9b60` | `main_switch_structure_bsp_and_notify` | 99 | 0.4 | 0.8 | 2 |
| `0x4c9bd0` | `campaign_level_advance` | 153 | 0.55 | 0.8 | 0 |
| `0x4c9dd0` | `game_start_new_single_player_map` | 187 | 0.5 | 0.8 | 0 |
| `0x4c9e90` | `main_loop_shutdown_cleanup` | 147 | 0.5 | 0.8 | 0 |
| `0x4c9f30` | `game_timer_reset` | 86 | 0.55 | 0.75 | 0 |
| `0x4c9f90` | `main_loop_frame_pacer` | 519 | 0.55 | 0.8 | 2 |
| `0x4ca1a0` | `screenshot_render` | 754 | 0.75 | 0.7 | 2 |
