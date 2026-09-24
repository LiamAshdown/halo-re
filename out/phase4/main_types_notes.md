# main module: type recovery notes

Header: `types/main.h`. Smoke test: `out/phase4/main_smoke.c`, built with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/main_smoke.c
C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -I types out/phase4/main_smoke.c
```

Both pass. The smoke file asserts every struct size, 56 `main_globals` field offsets against
their absolute addresses (base 0x00719700), the four string terminators the code writes
(0x719878, 0x719978, 0x719a99, 0x719aa2), the `console_globals` offsets against 0x006b7020..
0x006b79e0, the frame-rate and timedemo records against 0x00719ab0..0x00719b64, and the span of
the multiplayer map table. `terminal_console` (interface.h) holds a `char *`, so the
`console_globals` checks past +0x004 are gated on 32-bit pointers and only fire with `-m32`
(same gate as camera/sound smoke). The smoke includes `tags.h memory.h math.h game.h
networking.h interface.h main.h`: more than the three headers the task named. The extra ones are
needed because `console_globals` embeds interface.h `terminal_console` and the smoke also checks
the foreign records main relies on. A combined include of tags, memory, math, game, networking,
interface, rasterizer, render, camera, saved_games, input, effects, shell, hs and main gives no
error that mentions main.h and no redefinition, so the new header adds no name clashes.

Method: `objdump -d -M intel` of `bin/halo.exe`, then every instruction in the image whose
operand is an absolute address in 0x006b7010..0x006b7ab0 or 0x007196c0..0x00719cd0 was listed
with its access width (BYTE / WORD / DWORD PTR, or the register width of `mov al/ax/eax,ds:`)
and its containing function. That list, not the Ghidra types, fixed every field width and every
"never referenced" gap below. The .data / .rdata sections were also scanned for stored pointers
into those ranges: there are none, so no field is reached through a table.

---

## Records reused, not redefined

| type | owner | main module evidence |
|---|---|---|
| `terminal_console` 0x1be | interface.h | `console_toggle` 0x4c6530 passes 0x006b7024 to `console_open`; `console_paste_clipboard_text` 0x4c6570 compares `console_active` with it. Fields touched: key_event_count +0x00 (0x6b7024 WORD), key_events[i].key_code +0x04+i*4 (`(&0x6b7028)[i*2]` as short), color +0x84 (0x6b70a8..0x6b70b4, `console_initialize` 0x4c62d0), prompt +0x94 (0x6b70b8.. dword+word+byte, halo( ), input +0xb4 (0x6b70d8), edit.cursor +0x1ba (0x6b71de), edit.selection_anchor +0x1bc (0x6b71e0). All agree with interface.h. |
| `render_view` 0xac | render.h | `render_frame_all_views` 0x4c9260 fills 0x00719b70 with stride 0x56 shorts; `FUN_004c8f20` fills 0x006b79e8 field by field: +0x00 = -1 (WORD), +0x02 = 1, rasterizer_camera at +0x58: position 0,0,0; forward 0,0,1; up 0,1,0; mirrored 0 (+0x7c BYTE); fov +0x80; viewport +0x84 (stack arg of 0x4c8da0); window +0x8c (ECX of 0x4c8da0); z_near 0.01 +0x94; z_far 1.0 +0x98; then 0x15 dwords +0x58 -> +0x04. 0x006b79e8 + 0xac = 0x006b7a94, the next referenced global. |
| `observer_camera` 0x3c | camera.h | `FUN_004c9050` EAX (caller 0x4c93ad `mov eax,edi`): position +0x00..+0x08, forward +0x20..+0x28, up +0x2c..+0x34, field_of_view +0x38 (dword indices 0,1,2,8..0xe). |
| `network_scenario_load_request` 0x10c | networking.h | Built on the stack by 0x4c8930, 0x4c9770, 0x4c9dd0 (0x43-dword memset, +0x04 = 0, +0x06 = 1 then 0x00696564, +0x08 = 0xdeadbeef, +0x0c path via strncpy 0xff, +0x10b cleared) and passed in EAX to `game_scenario_session_begin` 0x4c95f0, which copies 0x43 dwords to game globals (0x006b0b80) +0x08. **Correction for networking.h:** +0x06 is the difficulty, not a seed. It lands at game globals +0x0e, which saved_games.h already pins as the difficulty the checkpoint loader compares with 0x00696564. |
| `file_reference_record` 0x10c | saved_games.h | 0x4c9530 / 0x4ca1a0: 0x43-dword memset, `filo` at +0x00, flags byte +0x04 (bit 0 tested then set), location -1 (WORD) at +0x06, path built by `path_append_component` from +0x08. |
| `BitmapData` 0x30 | tags.h | `screenshot_render` 0x4ca1a0 GlobalAllocs 0x30 bytes, zeroes 0xc dwords: `bitm` +0x00, width +0x04, height +0x06, depth 1 +0x08, type 0 +0x0a, format 10 +0x0c, flags 0x40 (0x41 when both sides are powers of two) +0x0e, +0x14 = 0, pixel buffer pointer +0x2c. main_globals.movie_frame_bitmap (0x00719724) points at one of these; `FUN_00518180` (rasterizer capture) takes it on the stack. |
| `game_time_globals` 0x20 | game.h | Byte +0x00 is read by `main_queue_map_change` 0x4c8740 and `main_level_transition_update` 0x4c9770 (`*game_time != 0 && (byte 1 or byte 2)`) and set to 1 by `main_menu_return_and_reset` 0x4c8a60 after zeroing 8 dwords. **Correction for game.h:** `unknown_00` is read and written (initialised flag). +0x0c game_time is read by the loop for the timedemo tick; +0x18 speed is saved, forced to 1.0 and restored by 0x4c99e0; +0x1c is passed to `render_frame_all_views`. |
| `player_globals` / `player` | game.h | 0x4c8800 and 0x4c9220 / 0x4c9260: +0x04 local_players[0], +0x0c int16 local player count (clamped to 1..1; game.h calls it unknown_0c), player +0x02 local_player_index with stride 0x200. |
| `saved_player_profile` | saved_games.h | `FUN_004c69c0` copies 0x7ff dwords from 0x00712dd8 and tests flags (+0x11c) bit 2; `credits_load_directly_for_endgame` 0x4c8d40 sets it (`or 0x00712ef4, 4`) before writing the profile. **New for saved_games.h:** flags bit 2 (0x0004) = end credits reached; it unlocks console context 0x20. |
| `input_key` | input.h | The console key handler 0x4c65c0 switches on `_input_key_tab` 0x1e (autocomplete), `_input_key_enter` 0x38 / `_input_key_numpad_enter` 0x66 (submit or close), `_input_key_up` 0x4d / `_input_key_down` 0x4e (history), plus key 6 (paste; input.h has no name for 6, F1..F10 are 1..10). |

---

## Structs defined in main.h

### console_globals (0x9c4, global 0x006b7020)

| offset | field | established by |
|---|---|---|
| 0x000 | active | `console_toggle` 0x4c6530 stores the `console_open` result; tested by 0x4c64b0, 0x4c65c0, 0x4c6860, 0x4c9050, the loop, and outside the module by 0x457000 (screen flash), 0x4aa700 / 0x4aaa90 (chat), `sound_update`. |
| 0x001 | enabled | `console_initialize` 0x4c62d0 (-console, string 0x0066b248); gated in 0x4c64b0, 0x4c6530, 0x4c65c0. |
| 0x004 | terminal | see the table above |
| 0x1c4 | history[8][0xff] | `console_process_command` 0x4c6a80 (`0x6b71e4 + newest*0xff`), the up/down path of 0x4c65c0 (`0x6b71e4 + ((newest - browse + 8) & 7) * 0xff`). |
| 0x9bc | history_count | WORD; 0x4c6a80 saturates at 8; 0x4c62d0 clears it. |
| 0x9be | history_newest_index | WORD; `(x + 1) & 7` in 0x4c6a80; -1 from 0x4c62d0. |
| 0x9c0 | history_browse_index | WORD (one dword load at 0x4c66b8 only feeds `si`); -1 on submit and in 0x4c62d0; +2 then -1 on up, -1 on down, clamped to count-1. |

Unresolved: 0x002..0x003, 0x1c2..0x1c3 and 0x9c2..0x9c3 are never referenced (treated as
alignment). 0x006b79e4..0x006b79e7 between the end of this block and the pregame render view is
never referenced and is not assigned to either.

### main_globals (0x3b0, global 0x00719700)

| offset | field | established by |
|---|---|---|
| 0x000/0x004 | frame_counter_low/high | `game_timer_reset` 0x4c9f30, `main_loop_frame_pacer` 0x4c9f90 (subtracts it, timedemo adds freq/30), loop 0x4c822b (`reset_frame_timers`) |
| 0x008 | frame_time_ms | same three; 0x4c9770 compares the fade end with it; timedemo adds 0x21 |
| 0x010/0x014 | render_counter_low/high | 0x4c9f30, loop render path (0x4c80f5) |
| 0x018 | frame_time_overflow | BYTE; 0x4c9f90 (`1.0 < delta`), passed to `update_server_send_update` |
| 0x01c | frame_delta_time | 0x4c9f90 writes; loop multiplies by time_is_running for the simulation and `camera_shake_tick` |
| 0x020 | game_connection | WORD in ~238 instructions image-wide; 3 set by 0x4c9dd0 |
| 0x022 | screenshot_index | WORD; `screenshot_render` 0x4ca1a0 formats and increments it |
| 0x024 | movie_frame_bitmap | DWORD; 0x4c9530 pushes it to `FUN_00518180` and passes it in EAX to `targa_export`; 0x4c9f90 uses it as the capture-on flag |
| 0x030 | movie_frame_index | 0x4c9530 post-increment into movie frame%06d.tga |
| 0x034 | movie_frame_delta_time | 0x4c9f90 `fld` when capturing |
| 0x038..0x03f | map request bytes | loop 0x4c7610; writers: hs 0x47f519 / 0x45fd04 (reset_map), 0x4c8740 (level_transition), hs 0x47fbe9 (revert_map), 0x472795 / 0x47f824 (revert_map_if_allowed), many (save_map), hs 0x47fae5..0x47fb94 and `main_save_map_private` 0x4c9a70 (0x3c..0x3f) |
| 0x040/0x044/0x04c | save retry / attempts / safe streak | 0x4c9a70; hs 0x47faf4..0x47fb6c resets them (DWORD, DWORD, WORD) |
| 0x048 | level_transition_fade_end_ms | 0x4c9770 (`frame_time_ms + 1000`, float fade into 0x00718fa8), `main_menu_play_title_music` 0x4993e0 |
| 0x04e..0x053 | won / lost / respawn / core flags | 0x4c9bd0, loop, hs 0x47fa0b / 0x47fa2b / 0x474d06, 0x47423e / 0x474743, hs 0x482914 / 0x4828f4 / 0x47fc04, `game_scenario_session_begin` 0x4c95f0 (moves 0x53 into 0x52) |
| 0x054 | switch_structure_bsp_index | WORD; hs 0x474c33 writes, `FUN_004c9b60` 0x4c9b60 reads it (dword load, `si` used) and resets -1 |
| 0x056 | main_menu_scenario_loaded | 0x4c8930 sets, 0x4c8b40 clears, 0x4c8800 / 0x4c8a60 / 0x4c9770 / loop read |
| 0x057 | return_to_main_menu | many writers (hs, interface, networking), loop reads |
| 0x058 | unknown_058 | loop 0x4c79f3 only: `if set, clear` |
| 0x059 / 0x05a | idle_timeout_reached / unknown_05a | loop 0x4c7d42 sets 0x59; 0x4c9770 tests 0x59 and sets 0x5a; 0x5a never read |
| 0x05b | quit | hs 0x482829, 0x4a1c8c, timedemo report; loop exits |
| 0x05c / 0x060 / 0x064 | idle_timeout_ms / last_gameplay_time_ms / last_activity_time_ms | loop idle test; 0x64 also by 0x4c95f0 and the loop prologue. 0x5c is never written anywhere, so the idle quit is dead in this build (UNSURE purpose) |
| 0x068 | start_film_playback | hs 0x47f6c4, read by 0x4c9dd0 |
| 0x069 / 0x06a | time_is_running / reset_frame_timers | loop, `scenario_structure_bsp_switch` 0x53eeb0, 0x5381c0 |
| 0x06b | unknown_06b | only `chimera__load_ui_map` 0x4c8a39 writes 1; never read |
| 0x06c / 0x06e | skip_ticks / skip_tick_count | hs 0x47fc95 / 0x47fc9b, `game_engine_flush_pending_simulation_ticks` 0x4c99e0 |
| 0x070 / 0x072 | lost_map_frames / respawn_coop_frames | WORD; loop; hs arms 0x72 with 0x5b |
| 0x074 | cache_file_open_pending | `FUN_004c8900` (EAX name), loop calls `cache_file_open_by_name`, 0x45af4e clears |
| 0x078 | restore_checkpoint_on_load | 0x4c8740 sets, 0x4c9bd0 clears after queueing, 0x4c95f0 calls `game_state_load_checkpoint` |
| 0x079 / 0x179 / 0x279 | the three path buffers | strncpy 0xff + terminator at 0x878 / 0x978 (0x279 is terminated only by FUN_004c8900 clearing byte 0 and by the strncpy limit) |
| 0x379..0x3a2 | connect staging | 0x4c83e0, 0x4c8500, 0x4c8660; `interface_draw_screen` 0x4974f0 reads all three |
| 0x3a8 | disable_frame_output | read twice by the loop, never written |
| 0x3a9 | debug_game_save | read by 0x4c9a70 and eight times by `game_safe_to_save` 0x45ba50, never written |
| 0x3ac | screenshot_tile_count | WORD; 0x4c9260 sets 1 when the screenshot input state (0x007127d2 or 0x007124aa) is set, 0x4ca1a0 loops n by n and clears; 0x4c9530, 0x50ba80, 0x5138a0..0x513ba0 read |

Unresolved: 0x00c..0x00f, 0x019..0x01b, 0x028..0x02f, 0x06d, 0x075..0x077, 0x3a3..0x3a7,
0x3aa..0x3ab, 0x3ae..0x3af are never referenced. The end of the block (0x3b0) is an inference:
nothing ties 0x00719ab0 onward to it, and CEA/OpenSauce sizes were used only as a hint. The
names 0x058, 0x05a, 0x06b stay unknown because they are only ever written or only ever
cleared.

### main_frame_rate_average (0x4c, global 0x00719ab0)

`game_frame_rate_average_update` 0x4c6e80 (sum of history[0..count-1], shift up one, count to 16,
stores the QPC ms in +0x00) and the end of the loop (history[0] = now - sample_time, clamped 200).
Unresolved: +0x04 (0x00719ab4) is never referenced; Ghidra lists it only because of the
`piVar3[-1]` shift loop, which stops at index 0.

### timedemo_bucket (0x08) / timedemo_globals (0x68, global 0x00719afc)

`timedemo_benchmark_update` 0x4c6f30: frame count +0x00, total +0x04, nine time/frame pairs
+0x08..+0x4c (thresholds 0x10, 0x14, 0x19, 0x21, 0x28, 0x32, 0x42, 100, 200 ms; the report
prints them from 5 fps up), current/previous/frame time +0x58/+0x5c/+0x60. +0x64 (0x00719b60) is
written by the loop tail (0x4c80df) with the game tick that last ran the benchmark; grouping it
here is a choice. Unresolved: +0x50..+0x57 never referenced.

### multiplayer_map_table_entry (0x0c, .data 0x0068e588, 19 entries)

Loop prologue 0x4c76ac..0x4c76c8: `esi` from 0x68e58c while < 0x68e670 step 0xc, pushes
`[esi-4]` (map_id) and passes `[esi]` (name) in EAX to `map_list_add_entry` 0x4950c0. +0x08 is
1 for the first 13 entries and 0 for the last 6; only `FUN_0045c370` reads a byte of the table
(0x0068e590, entry 0 +0x08), so its meaning is unresolved.

### bink_movie_prefix (0x100 prefix of RAD HBINK)

`movie_play_bink` 0x43ed20 reads +0x08 and +0x0c (`cmp [ebp+0xc],[ebp+0x8]` ends playback) and
+0xfc (paused, around `_BinkPause@8`). +0x00 / +0x04 are named from the RAD layout, not from this
code. Library data: the real struct is larger; only this prefix is modelled.

### Enums

`main_constants`, `game_connection` (0x4c9dd0, 0x4c9e90, loop), `timedemo_step` (0x4c70cc..0x4c7138:
100 queues the a30 name 0x00669a58 through `main_queue_map_change`, 0x44c / 0x898 / 0xc4e run
map_name b30 / c10 / d20, 0x125c writes timedemo.txt and sets quit), `timedemo_bucket_threshold`,
`console_command_context_flags` (`FUN_004c69c0` 0x4c69c0; the result goes to
`chimera__autocomplete_gather` in EDX at 0x4c6b4c), `console_constants`.

The context mask conflicts with types/hs.h, which names the same bits by multiplayer game type
(ctf, slayer, ...). The producer here sets bit 1 for the host, bit 3 when a multiplayer engine is
loaded, bit 4 when none is, bits 0 and 6 always, and forbids bits 1 and 2 on a client, bit 5
without the credits profile flag. The hs.h names should be revisited when the hs function flags
are read against this producer.

---

## Register arguments confirmed at call sites (for the rewriters)

- `main_queue_map_change` 0x4c8740: EAX = map name (0x4c7106, 0x4c9c39).
- `main_queue_map_change_by_name_or_clear` 0x4c87a0: EDI = name or NULL (0x45fc81, 0x47f55d).
- `FUN_004c8900` 0x4c8900: EAX = cache file name or NULL (0x45aeea).
- `viewport_split_rect_compute` 0x4c8da0: EAX = view count, EDX = view index, ECX = window rect
  (render_view +0x8c), one stack argument = viewport rect (+0x84) (0x4c9326..0x4c9332).
- `FUN_004c9050` 0x4c9050: EAX = observer_camera or NULL, ECX = render_view (0x4c93ad).
- `game_scenario_session_begin` 0x4c95f0: EAX = network_scenario_load_request (0x4c89de, 0x4c999e).
- `network_hostname_resolve_with_timeout` 0x4c8370: ECX = host name (0x4c8410).
- `console_process_command` 0x4c6a80: EDI = command line, one stack argument = context bits
  (0x4c6404 with EDI = map_name b30 string 0x0066b224, 0x4c646f with 0x2000).
- `console_print_error_va` 0x4c67c0: AL = clear the terminal first, then format and varargs on
  the stack (0x461a63, 0x46afe0).

---

## Misnamed, misattributed or not functions

- **0x4c7610 `game_state_save_core`** is the engine main loop (CEA main_loop); the core.bin
  strings made the hint pick the wrong name. It runs to 0x4c8334.
- **0x4c7f10 `weapon_stop_reload`** is not a function: it is the tail of the main loop, entered
  only by the loop body (0 callers, `in_stack` and `unaff_` registers everywhere). Its types are
  the loop's.
- **0x4c9c60 `console_response_printf`** is not a function: bytes 0x4c9c60..0x4c9c6a are the
  `return_to_main_menu = 1; pop; ret` tail of `campaign_level_advance` 0x4c9bd0.
- **0x4c9dc0 `console_process_command`** (4 bytes) is not a function: it is the `mov esp,ebp;
  pop ebp; ret` epilogue of `FUN_004c9c80` (the -connect start-up helper, not in this module list).
- **0x4c62d0 / 0x4c62f0 / 0x4c6340** (`FUN_004c62d0`, `weapon_prevents_grenade_throwing`,
  `weapon_get_first_person_animation_time`) are one function, `console_initialize` 0x4c62d0..
  0x4c638c, split by Ghidra. They are outside this module list but they are console code and
  own no weapon types; console_globals records what they write.
- **0x4c6390 `chimera__exec_init`** is console startup (CEA console_startup): it runs init.txt
  and otherwise, when 0x0071d1a8 is set, the command map_name b30.
- **0x4c8b90 `FUN_004c8b90`**: the summary says nine entries; the table has ten (a10 .. d40) and
  the function returns 0..9 or -1.
- **0x43ed20 `movie_play_bink`** sits far from the rest of the module (its own object file); it
  is main-owned code but uses only the Bink prefix and one foreign flag (0x007196d4).
- The **types/effects.h** name `player_effect_suppressed` for 0x006b7020 is the console active
  flag; the screen flash code only reads it.

No function in the list is library code. None was skipped for types.
