# `cutscene`: cinematic state, letterbox and titles, recorded animation codec

Retail Halo PC `halo.exe` 1.0.10, `0x449590 .. 0x44a815` (15 Ghidra functions), plain C / MSVC 7.1
(cl 13.10.3077, LTCG) / x86. Each file in this directory holds one function, rewritten from its
Ghidra decompilation against `types/cutscene.h`. The original decompile is kept verbatim at the
bottom of each file inside `#if 0 ... #endif`. Every file was also checked line by line against
`objdump -d -M intel` of `bin/halo.exe`. Where the decompiler went wrong the file says so and
quotes the instructions it relied on.

Gate: `python tools/build_check.py cutscene` → **12 ok, 0 failed**. 12 files are rewritten. The
other 3 addresses are misattributed library or helper code (see below).

## What the module contains

The address-run heuristic grouped three unrelated things into this module:

| Family | Range | What it is |
|---|---|---|
| sort | `0x449590`–`0x44971f` | Dword-array quicksort plus its small-partition fallback. This is cseries library code, not rewritten here. |
| cinematics | `0x449720`–`0x449f7f` | The engine side of the hs `cinematic_*` functions: start and stop, title queueing, and the per-frame letterbox and title draw. State lives in the 0x1c-byte game-state block at `*0x006f187c`. |
| recorded animations | `0x449f80`–`0x44a815` | The hs `cutscene_recording` codec: lookup by name, the versioned `unit_control_data` unpacker, the angle delta helpers, and the compressed (version 4) and v1 event handlers. The playback loop that drives them (`0x44a930`/`0x44aa90`) is in `src/devices` by address. |

Handlers reached only through tables (`0x44a060..0x44a0e0`, `0x44a550`, `0x44a590`,
`0x44a650..0x44a760`, `0x44a820`, `0x44a890`, `0x44a8b0`) were created in the Ghidra project and
rewritten in cleanup pass 1; see the table at the end of this file.

## Functions

| Address | File / name | Was | Rewrite conf. | Notes |
|---|---|---|---|---|
| `0x449590` | *(none)*: `qsort_dword_array` | | n/a | misattributed, see below |
| `0x4496d0` | *(none)*: `qsort_dword_array_shortsort` | | n/a | misattributed |
| `0x449720` | `cutscene_start.c` | | 0.9 | cdecl. It tail-jumps to `game_engine_cleanup_stray_projectiles` 0x467f70 |
| `0x449780` | *(none)*: `FUN_00449780` | | n/a | shared filled-rectangle helper (EAX colour, ECX `Rectangle2D *`), misattributed |
| `0x449960` | `cutscene_title_queue.c` | | 0.9 | cdecl (int16 index, float delay seconds) |
| `0x4499c0` | `chimera__letterbox.c` | | 0.7 | Chimera signature name. `cinematic_render` is the suggested real name. The bar rects and the title dest rect come from objdump, because Ghidra dropped them. |
| `0x449eb0` | `cutscene_stop.c` | | 0.85 | cdecl |
| `0x449f80` | `recorded_animation_find_by_name.c` | `FUN_00449f80` | 0.85 | EBX name, ESI `Scenario *`, returns int16 |
| `0x449fd0` | `unit_control_data_unpack.c` | `FUN_00449fd0` | 0.85 | EBX control. The stack holds (cursor **, uint8 version). |
| `0x44a110` | `recorded_animation_apply_char_difference.c` | `FUN_0044a110` | 0.9 | EAX angles, EDX int8 delta |
| `0x44a150` | `recorded_animation_apply_short_difference.c` | `FUN_0044a150` | 0.9 | EAX angles, EDX int16 delta |
| `0x44a190` | `recorded_animation_angle_to_vector.c` | | 0.85 | EAX out `real_vector3d *`, ECX angles |
| `0x44a1d0` | `recorded_animation_decode_char_difference_event.c` | `FUN_0044a1d0` | 0.8 | cdecl (state, control, header, cursor). Compressed types 7..14 |
| `0x44a390` | `recorded_animation_decode_short_difference_event.c` | `FUN_0044a390` | 0.8 | objdump is identical to 0x44a1d0 apart from the int16 delta. Compressed types 15..22 |
| `0x44a790` | `recorded_animation_decode_angle_vector_event_v1.c` | `FUN_0044a790` | 0.85 | cdecl (control, event, cursor). v1 types 0x10..0x16 |

The renames are recorded in `symbols/agent_phase4_cutscene.txt`.

## Struct layouts

All of these live in `types/cutscene.h`. `#pragma pack(push,1)` is in force, and offsets are from
the struct base. Pointer-holding structs reach their stated size only under a 32-bit data model.

### `cinematic_globals`: size `0x1c`, at `*0x006f187c`

| Off | Type | Field | Writers / readers |
|---|---|---|---|
| `0x00` | `float` | `letterbox_scale` | 0..1. The letterbox draw steps it by elapsed ticks / 30. |
| `0x04` | `int32_t` | `letterbox_last_tick` | `game_time` of the last step. Set by start and by `cinematic_show_letterbox`. |
| `0x08` | `uint8_t` | `show_letterbox` | The fade target. Start sets it, stop and map dispose clear it. |
| `0x09` | `uint8_t` | `in_progress` | Set by start, cleared by stop. About 20 readers engine-wide. |
| `0x0a` | `uint8_t` | `skip_in_progress` | `cinematic_skip_start/stop_internal` |
| `0x0b` | `uint8_t` | `suppress_bsp_object_creation` | `cinematic_suppress_bsp_object_creation`. Read by 0x4f4860. |
| `0x0c` | `cinematic_title_slot[4]` | `titles` | Reset to -1/-1 by 0x45b050 |

### `cinematic_title_slot`: size `0x04`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `title_index` (a `Scenario.cutscene_titles` index; -1 means empty) |
| `0x02` | `int16_t` | `ticks` (starts at `-round(delay*30)`, gains `ticks_this_frame` each frame) |

### `recorded_animation_angles`: size `0x04`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `yaw` (units of pi/1000; wrapped by ±1000 once it leaves -1000..1000) |
| `0x02` | `int16_t` | `pitch` (never wrapped) |

### `recorded_animation_decoder_state`: size `0x0c` (`recorded_animation +0x54`)

| Off | Type | Field |
|---|---|---|
| `0x00` | `recorded_animation_angles` | `facing` |
| `0x04` | `recorded_animation_angles` | `aiming` |
| `0x08` | `recorded_animation_angles` | `looking` |

### `recorded_animation_char_difference` (0x02) / `recorded_animation_short_difference` (0x04)

| Off (char / short) | Type (char / short) | Field |
|---|---|---|
| `0x00` / `0x00` | `int8_t` / `int16_t` | `yaw` |
| `0x01` / `0x02` | `int8_t` / `int16_t` | `pitch` |

### `recorded_animation_event_v1`: size `0x04`, plus the v1 set events

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `type` (`recorded_animation_event_type`) |
| `0x02` | `uint16_t` | `delay_ticks` |

| Event struct | Size | Payload at `+0x04` |
|---|---|---|
| `..._animation_state_set_event_v1` | `0x06` | `int8_t animation_state`, pad |
| `..._aiming_speed_set_event_v1` | `0x06` | `int8_t aiming_speed`, pad |
| `..._control_flags_set_event_v1` | `0x06` | `uint16_t control_flags` |
| `..._weapon_index_set_event_v1` | `0x06` | `int16_t weapon_index` |
| `..._throttle_set_event_v1` | `0x0c` | `real_vector2d throttle` (k is zeroed) |
| `..._multi_vector_set_event_v1` | `0x10` | `real_vector3d vector` |
| `..._angle_vector_set_event_v1` | `0x0c` | `real_euler_angles2d angles` (radians) |

### `unit_control_data_field_layout`: size `0x0c` (tables `0x686cc8/0x686d40/0x686d58/0x686d70`)

| Off | Type | Field |
|---|---|---|
| `0x00` | `byte_swap_definition *` | `type` (not read by the unpacker; NULL in the terminator) |
| `0x04` | `int32_t` | `size` (-1 ends the table) |
| `0x08` | `int32_t` | `offset` into `unit_control_data` (-1 means skip the bytes) |

### `recorded_animation_codec`: size `0x08` (`0x686fd8` compressed, `0x686fe0` v1)

| Off | Type | Field |
|---|---|---|
| `0x00` | `recorded_animation_begin_proc` | `begin` |
| `0x04` | `recorded_animation_update_proc` | `update` |

### `recorded_animation`: size `0x64` (datum of the data_array at `0x006b0a10`)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` |
| `0x02` | `int16_t` | `unknown_02` (never touched) |
| `0x04` | `datum_index` | `unit_index` |
| `0x08` | `int16_t` | `ticks_remaining` |
| `0x0a` | `uint16_t` | `flags` (`recorded_animation_flags`) |
| `0x0c` | `int32_t` | `event_ticks` |
| `0x10` | `uint8_t *` | `event_cursor` |
| `0x14` | `unit_control_data` | `control_data` (0x40, `types/units.h`) |
| `0x54` | `recorded_animation_decoder_state` | `decoder_state` |
| `0x60` | `int16_t` | `codec_index` (version - 1) |
| `0x62` | `int16_t` | `unknown_62` (never touched) |

## Globals and extern naming

| Address | Declared here as | Note |
|---|---|---|
| `0x006f187c` | `cinematic_globals *cinematic_globals_ptr` | It needs the `_ptr` suffix because `cinematic_globals` is the typedef. About 13 files in other modules still declare it as `uint8_t *cinematic_globals`, and a few use ad-hoc names (`some_globals_006f187c`, `saved_games_something_006f187c`, `unknown_006f187c`). |
| `0x00686b60` | `float cinematic_saved_music_gain` | -1.0 when nothing is saved |
| `0x0087a478` | `player_globals *local_player_globals` | Same as the repo majority. `+0x11` is still `unknown_11`. |
| `0x00880354` | `ai_globals *ai_globals_ptr` | The repo is split between `ai_globals_ptr` (41) and `ai_global_data` (34). |
| `0x006f1d6c` | `game_time_globals *game_time` | Same as the repo majority |
| `0x0071cfc4` / `0x0071cfc0` | `cinematic_screen_effect_state` / `rasterizer_model_ambient_reflection_tint` | From `types/render.h` |
| `0x00718fb6` | `ui_pending_error ui_pending_errors[4]` | From `types/interface.h` |
| `0x007c3140` | `Rectangle2D letterbox_screen_bounds` | Only `.top`/`.left` are read. Other modules name the same pair `render_viewport_top` or `screen_safe_area_origin`. |
| `0x0071d144` | `uint32_t text_shadow_color_argb` | |
| `0x00672dd8` | `float recorded_animation_angle_scale` | This is an .rdata literal (pi/1000), not a global. |

`FUN_00449780` is declared as `(uint32_t packed_color, Rectangle2D *rect)`, which matches the other
callers in the repo.

## Known gaps

- **Three functions are not rewritten** because they are misattributed:
  - `0x449590` `qsort_dword_array` and `0x4496d0` `qsort_dword_array_shortsort` are a cseries sort
    (EAX count, ECX base, stack comparator). The callers are in ai (0x412ba0) and sound (0x552cf0).
  - `0x449780` is the generic filled screen rectangle (EAX packed ARGB, ECX `Rectangle2D *`). It
    builds a `ui_quad_render_state` for 0x51c9a0 and has 7 callers across interface and rasterizer.
    It belongs with interface/rasterizer.
- The table-only handlers listed above were rewritten in cleanup pass 1 (end of this file).
- `chimera__letterbox` keeps its Chimera signature name. `render_nonplayer_frame` calls it by that
  name. `cinematic_render` is the suggested real name.
- `src/ai/actor_squad_action_execute.c` still declares `extern int32_t FUN_00449f80(void)`. It
  should become `recorded_animation_find_by_name`, which returns int16 and takes EBX name and ESI
  `Scenario *`. That file is outside this module and was not edited.
- The v1 unit_control_data table drops 2 bytes (`size 2, offset -1`) between weapon_index and
  throttle. What those bytes hold is unknown.
- The v1 event types 7 and 8 have no handler.

## Fixes made in the review pass

`chimera__letterbox.c`:
- The raw `hud_globals_tag_data + 0x54` read was flagged UNSURE. It is now
  `fullscreen_font.tag_id`. The `TagDependency` sits at 0x48 and its tag_id is its last dword, at
  0x54, so the earlier "discrepancy" never existed.
- `ScenarioCutsceneTitle.string_index` is `uint16_t` in `tags.h`, so the `< 0` guard was dead
  code. The binary tests it signed (`test ax,ax / jl`, then `movsx` before the count compare). The
  file now casts it to `int16_t` in the guard, the bound check and the string lookup.
- The letterbox fade-out clamp is now `<= 0.0f` (it was `< 0.0f`), which matches
  `fcom / test ah,0x41 / je` at 0x449a49.
- The `FUN_00449780` prototype now uses the repo-wide argument order.
- An unbalanced apostrophe in the `#if 0` trailer is gone.

Other files:
- The renamed files now record name confidence 0.8, matching the new symbols file.
- Rewrite confidences were raised after the objdump check.
- The inlining note in the two difference decoders is now a NOTE instead of an UNSURE.

## Open questions for hook verification

1. **Yaw wrap by 1000, not 2000.** `apply_*_difference` and the inlined facing branch change yaw
   by ±1000 (a half turn at pi/1000 per unit) once it leaves -1000..1000. That reverses the
   direction instead of wrapping it. Log `decoder_state.facing` across a recorded turn to check
   whether real streams ever cross ±1000.
2. **Units of the title timing.** `ScenarioCutsceneTitle.fade_in_time`/`up_time`/`fade_out_time`
   are compared directly against the tick counter. Check whether tag postprocess turns the seconds
   into ticks, or whether titles really fade over N ticks. `up_time` also acts as an absolute end
   time, not a duration.
3. **Which vectors the v1 angle and multi-vector types skip.** Angle types 0x13/0x14/0x15 skip
   looking/aiming/facing and 0x10..0x12 and 0x16 write all three. Multi-vector types 12/13/14 skip
   looking/aiming/facing. The offsets of the two numberings differ. Hook 0x44a790 and 0x44a820 on
   a v1-3 recording to see which types actually occur.
4. **The letterbox bar geometry.** The top bar spans `ROUND(bounds.top)..ROUND(bounds.top + h)`
   and the bottom bar spans `ROUND(480 - h)..481`, with the right edge fixed at 640 and
   `h = scale*0.125*480`. Check the `Rectangle2D` passed to 0x449780 at both call sites (ECX at
   0x449b25 and 0x449b99), and that `0x007c3140` holds the safe-area origin.
5. **The destination-rect fallback.** 0x514ab0 gets ECX = `&title->text_bounds`, or
   `&hud_globals->default_chapter_title_bounds` (+0x2dc) when the bounds are empty, with EAX = 0.
   Confirm the `+0x2dc` offset and the argument order of `chimera__draw_16_bit_text`. The
   letterbox reconstruction depends on both.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x44a060` | `recorded_animation_decode_animation_state_event` | 17 | 0.85 | 0.9 | 0 |  |
| `0x44a080` | `recorded_animation_decode_aiming_speed_event` | 18 | 0.85 | 0.9 | 0 |  |
| `0x44a0a0` | `recorded_animation_decode_control_flags_event` | 21 | 0.85 | 0.9 | 0 |  |
| `0x44a0c0` | `recorded_animation_decode_weapon_index_event` | 21 | 0.85 | 0.9 | 0 |  |
| `0x44a0e0` | `recorded_animation_decode_throttle_event` | 34 | 0.85 | 0.9 | 0 |  |
| `0x44a550` | `recorded_animation_compressed_begin` | 56 | 0.75 | 0.9 | 0 |  |
| `0x44a590` | `recorded_animation_compressed_update` | 169 | 0.75 | 0.85 | 1 |  |
| `0x44a650` | `recorded_animation_decode_animation_state_event_v1` | 21 | 0.85 | 0.9 | 0 |  |
| `0x44a670` | `recorded_animation_decode_aiming_speed_event_v1` | 22 | 0.85 | 0.9 | 0 |  |
| `0x44a690` | `recorded_animation_decode_control_flags_event_v1` | 24 | 0.85 | 0.9 | 0 |  |
| `0x44a6b0` | `recorded_animation_decode_weapon_index_event_v1` | 24 | 0.85 | 0.9 | 0 |  |
| `0x44a6d0` | `recorded_animation_decode_throttle_event_v1` | 35 | 0.85 | 0.9 | 0 |  |
| `0x44a700` | `recorded_animation_decode_facing_vector_event_v1` | 38 | 0.85 | 0.9 | 0 |  |
| `0x44a730` | `recorded_animation_decode_aiming_vector_event_v1` | 38 | 0.85 | 0.9 | 0 |  |
| `0x44a760` | `recorded_animation_decode_looking_vector_event_v1` | 38 | 0.85 | 0.9 | 0 |  |
| `0x44a820` | `recorded_animation_decode_multi_vector_event_v1` | 104 | 0.85 | 0.9 | 0 |  |
| `0x44a890` | `recorded_animation_v1_begin` | 25 | 0.75 | 0.9 | 0 |  |
| `0x44a8b0` | `recorded_animation_v1_update` | 114 | 0.75 | 0.85 | 0 |  |

Gate: `python tools/build_check.py cutscene` clean after the cleanup review.
