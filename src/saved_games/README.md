# `saved_games`: game state, checkpoints, profiles, variants and file helpers

Retail Halo PC `halo.exe` 1.0.10, `0x537f70 .. 0x556170` (116 functions in the Phase 2 list),
plain C / MSVC 7.1 (cl 13.10.3077, engine objects built with LTCG) / x86. Every file in this
directory is one function, rewritten from its Ghidra decompilation against
`types/saved_games.h`, with the original decompile (or the objdump listing, where Ghidra had no
function) kept at the bottom inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py saved_games` → **118 ok, 0 failed**.

Coverage: 118 files = 115 of the 116 listed addresses, plus 3 real functions that Ghidra never
split out (`0x5385d0` game_state_after_load_restore_time, `0x538ac0`
game_checkpoint_reclaim_slot_callback, `0x539110` game_checkpoint_print_list_entry; all three are
reached only through function-pointer tables or callbacks). The 116th listed address, `0x555d30`
("structure_render_lightmaps"), is not a function: it is the `je 0x555d6b` at the middle of
`file_enumerate_find_next` (the flag test that starts at `0x555d29`), nothing calls or jumps
to it, and it is covered inside `file_enumerate_find_next.c`.

Because LTCG invented a register convention per function, every file states its own
`register convention:` line; where a callee takes register arguments the extern carries a
`blam-cc:` note (for example `savegame_find_first`: EAX out, stack root). Treat the C parameter
order as the logical order, not the push order.

Include order for any translation unit: `tags.h`, `memory.h`, `math.h`, `game.h`,
`networking.h`, `interface.h`, then `saved_games.h` (add `rasterizer.h` or `cache.h` before
`saved_games.h` when needed). `saved_games.h` reuses `game_variant`, `win32_find_dataa`,
`network_mutex_record`, `network_thread_record`, `controls_gamepad_record`, `input_guid` and
`data_array` / `memory_pool` from those headers and never redefines them.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| game state | `0x537f70`–`0x538980` | the 0x440000-byte game-state arena (`game_state_new` / `game_state_new_pool` carve `data_array` / `memory_pool` blocks out of it), its 0x14c-byte header, snapshot, revert, core save / load |
| async writer and persistent storage | `0x538980`, `0x539330`–`0x539a40` | `savegame.bin` (pre-sized to 0x480000), the writer thread (0x4000-byte chunks), crc validation in 0x20000-byte chunks |
| checkpoints | `0x538ac0`–`0x539330` | `checkpoints\*.sav` text companions (`level,difficulty,tick` then two date/time lines), enumerate / sort (qsort, 0x48-byte entries) / reclaim / copy |
| player profiles | `0x539a40`–`0x53b240` | `blam.sav` (0x1ffc body + crc = 0x2000), defaults, the one local profile slot, level progress |
| control bindings | `0x539ff0`–`0x53b7f0` | keyboard / mouse / gamepad binding tables inside the profile, the binding editor the controls UI calls, gamepad slot matching by device guid |
| game variants | `0x53bae0`–`0x53c260` | `blam.lst` (0x98 body + crc, padded to 0x2000 on disk), the 0x26 built-in playlists, the async variant writer thread |
| saved-game index | `0x53c260`–`0x53e060` | `savegames\` directory scan into the 0x206-byte index records, handles, create / delete / rename / copy, `lastprof.txt` / `lastmpvr.txt` / `lastmpmp.txt` |
| files | `0x5554c0`–`0x556170` | `file_reference_*` (a 0x10c-byte path + HANDLE record), path builders, a recursive directory enumerator |

The index functions at `0x53e060..0x53e630` (read / write / append / remove slot, slot count,
handle pack) are outside this range; they are written in `src/game` under the names
`savegame_index_*` / `savegame_slot_handle_pack`, which the externs here use.

A saved-game handle (`saved_game_handle_bits`) is `type | index << 16 | builtin << 30 |
checksum_valid << 31`, packed by `savegame_slot_handle_pack` (EAX index, ECX type, DL builtin,
stack byte checksum_valid); `-1` means none, and for the profile slot the built-in default
profile.

## Struct layouts

All of these are in `types/saved_games.h` (`#pragma pack(push,1)`). Offsets are from the struct
base; sizes are for the 32-bit layout.

### game_state_header (0x14c, at the start of the game-state arena)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0x000 | allocation_checksum | uint32_t | running crc of every `game_state_new` size (0x006e2dd4); must match on load |
| 0x004 | scenario_name | char[0x100] | scenario tag path |
| 0x104 | build_version | char[0xe] | `01.00.10.0621`; the loader also accepts eight older strings |
| 0x112 | unknown_112 | uint8_t[0x12] | zeroed |
| 0x124 | local_player_count | int16_t | from 0x006894b8 |
| 0x126 | difficulty | int16_t | game globals +0x0e |
| 0x128 | map_checksum | uint32_t | cache header crc32 |
| 0x12c | unknown_12c | uint8_t[0x1c] | zeroed |
| 0x148 | file_checksum | uint32_t | crc32 of the whole 0x440000 image with this field zeroed |

### checkpoint_file_entry (0x48, 0x66 of them in a GlobalAlloc block)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0x00 | level_index | int16_t | campaign level from `FUN_004c8b90(scenario name)` |
| 0x04 | game_time | int32_t | game tick of the save |
| 0x08 | difficulty | int32_t | |
| 0x0c | time | win32_systemtime | local wall-clock time from the `.sav` text |
| 0x1c | last_write_time | uint32_t[2] | FILETIME, secondary sort key |
| 0x24 | kind | int32_t | `checkpoint_kind`: 0 checkpoint, 1 autosave1, 2 autosave (exact stem match) |
| 0x28 | name | char[0x20] | file stem |

### saved_player_profile (0x1ffc, the `blam.sav` body)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0x000 | version | uint8_t | 9 |
| 0x002 | name | uint16_t[12] | wide display name |
| 0x01a | unknown_01a | uint8_t[0x100] | zeroed |
| 0x11a | player_color | int16_t | -1 default; index into the 18-entry color table |
| 0x11c | flags | uint16_t | bit 0 default profile, high byte the default index, bit 1 checked by delete-by-name |
| 0x11e | campaign_progress | uint8_t[10] | per level, bit n = finished on difficulty n |
| 0x128 | last_campaign_level | int16_t | |
| 0x12c | button_set, joystick_set, look_sensitivity (0x12e, 3), unknown_12f..133 | uint8_t | controls block starts here (carried over as 0x93c bytes) |
| 0x134 | keyboard_bindings | int16_t[0x6d] | action per key, 0x7fff unbound |
| 0x20e | mouse_button_bindings | int16_t[8] | |
| 0x21e | mouse_axis_bindings | int16_t[3][2] | [0] direction 1, [1] direction 2 |
| 0x22a | gamepad_button_bindings | int16_t[4][32] | |
| 0x32a | gamepad_action_buttons | int16_t[4][2] | button index bound to action 8 / 9, -1 none |
| 0x33a | gamepad_axis_bindings | int16_t[4][32][2] | |
| 0x53a | gamepad_pov_bindings | int16_t[4][16][8] | |
| 0x93c | unknown_93c | float[6] | 1.0, 1.0, 0.188495576 (0x3e4104fc), same, 128.0, 128.0 |
| 0x954 | unknown_954 / unknown_955 | uint8_t | 3 |
| 0x956 | gamepad_rate_a / gamepad_rate_b (0x95a) | uint8_t[4] | 3 each |
| 0x960 | unknown_960 | float[2] | 0.75 |
| 0xa68 | screen_width, screen_height, refresh_rate | int16_t | video block (0x110 bytes): 800x600x60 default, 640x480 low-end, `-vidmode` or current display mode |
| 0xa6e | unknown_a6e..a75 | uint8_t | capability flags from the machine class |
| 0xa76 | gamma | int8_t | low byte of rasterizer_gamma_exponent, 0 -> 1, 0xff -> 0xfe |
| 0xb78 | master_volume (10), effects_volume (10), music_volume (6), unknown_b7b..b7f | uint8_t | audio block (0x108 bytes) |
| 0xc80 | unknown_c80..c8a | uint8_t | 0x10b-byte block |
| 0xd8c | server_name | uint16_t[0x90] | L"Halo" |
| 0xeac | server_password | uint16_t[9] | empty |
| 0xebe | unknown_ebe (0), unknown_ebf (3), unknown_fc0 (1), unknown_fc2 (wide, empty) | | |
| 0x1002 | server_port / client_port (0x1004) | uint16_t | 2302 / 2303 |
| 0x1108 | gamepads | controls_gamepad_record[4] | stride 0x220, guid at +0x20c, carried over as 0x880 bytes |
| 0x1988 | unknown_1988 | uint8_t[0x674] | never written here |

### File records

| Struct | Size | Layout |
|---|---|---|
| saved_player_profile_file | 0x2000 | profile (0x1ffc) + checksum (crc32 seed -1) at 0x1ffc |
| game_variant_file | 0x2000 | variant (`game_variant`, 0x98) + checksum at 0x98 + padding; every writer and reader moves 0x2000 bytes |
| saved_player_profile_slot | 0x2004 | profile + handle at 0x1ffc (0x00714dd4 for slot 0), 1 slot at 0x00712dd8 |
| variant_write_request | 0x9c | handle + variant, the writer thread parameter at 0x00721288 |

### saved_game_index_entry (0x206, one per `savegames\` subdirectory in the index file)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0x000 | path | char[0x100] | full path of blam.sav / blam.lst |
| 0x100 | display_name | uint16_t[0x80] | wide; [0x7f] always cleared; this, not the path, is the name handed to XCreateSaveGame / XDeleteSaveGame |
| 0x200 | type | int16_t | `saved_game_type`: 0 profile, 1 variant, -1 unrecognised |
| 0x202 | index | int16_t | slot number in the index file |
| 0x204 | builtin | uint8_t | default profile / playlist: never deleted from disk |
| 0x205 | checksum_valid | uint8_t | the body crc matched |

### file_reference_record (0x10c, the field view of `types/hs.h` `file_reference`)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0x000 | signature | uint32_t | 'filo' |
| 0x004 | flags | uint8_t | bit 0 is-file |
| 0x006 | location | int16_t | `file_reference_location`, 2 (absolute) everywhere in this module |
| 0x008 | path | char[0x100] | [0xff] forced to 0 by every append |
| 0x108 | handle | void * | Win32 HANDLE, 0 when closed |

### Small records

| Struct | Size | Layout |
|---|---|---|
| win32_systemtime | 0x10 | SYSTEMTIME: year, month, day_of_week, day, hour, minute, second, milliseconds (uint16_t each) |
| xgame_find_data | 0x344 | win32_find_dataa (0x140), save_game_directory char[0x104] at 0x140, save_game_name uint16_t[0x80] at 0x244 |
| control_binding_descriptor | 0x0c | device_type, device_index, input_kind, input_index (int16_t each), direction int32_t at 0x08 |
| file_enumeration_position | 0x04 | depth int16_t (0x0069fa5c, -1 idle), location int16_t (0x0069fa5e) |
| win32_file_attribute_data | 0x24 | WIN32_FILE_ATTRIBUTE_DATA: attributes, three FILETIMEs, size high, size low |

The header also carries the full global map of the module (`0x006e2dc8..0x006e3208`,
`0x0069e7ac..0x0069e8d4`, `0x00712dd8..0x00721e4c`, `0x0069fa50..0x0069fce0`) as comments.

## Phase 4 review: what changed

Gate at the start of the review: 58 ok / 60 failed, all 60 from an include order that did not
bring in `memory.h` / `math.h` / `game.h` / `networking.h` / `interface.h` before
`saved_games.h`; plus one callback type mismatch. Fixed in every file; now 118 / 0.

TYPES-GAP typedefs folded into the header and removed from the files: `file_enumeration_position`
(2 copies), `win32_file_attribute_data`. A third local struct (`saved_game_create_record`, 0x20d
bytes) was dropped instead: objdump shows it was a stack artefact, and the function now uses
`saved_game_index_entry`. `game_variant_file` grew to 0x2000 bytes (see the table above).

28 functions were checked line by line against objdump (marked in the table below). Behaviour
fixes:

- Dropped or wrong register arguments: `player_profile_load` (AX index, EDX profile) from
  select_local_slot and mark_level_visited; `player_profile_rename` handle (EAX) from
  player_profile_write_data; XCreateSaveGame's EAX name in saved_game_create_slot (was -1);
  XDeleteSaveGame's EAX is the entry's wide display name, not its path (delete_by_handle,
  rename); XCreateSaveGame's out buffer in allocate_new_slot was aliased to the name;
  `player_profile_get` argument order in delete_by_display_name; `FUN_004c8b90` has one
  argument, not three (the other two were fprintf arguments pushed early).
- Char returns tested only in AL: `saved_game_get_directory_by_handle` was declared int32_t in
  13 callers; `game_state_queue_write` and `game_state_read_persistent_storage` return bools.
- Wrong data: `0x00671fac` is the L"<missing string>" array, not a pointer (4 files); the
  display-mode block from `display_mode_get_current` is `rasterizer_display_mode` (int32 fields
  and a vsync byte), not four int16s; the 0.1885 float is 0x3e4104fc; `allocate_new_slot` read
  string 2 at `tag_data + 0x28` instead of `strings.pointer + 0x28`.
- Control flow: copy_files returned the wrong copy result and did not skip the `*.bin` phase
  after a failed `*.sav` copy; allocate_new_slot stopped at 998 and never cleared the name;
  the inlined `path_append_component` in three files advanced past the separator twice and
  used a one-too-large bound; find_by_name packed the handle from `entry.index` instead of the
  loop counter.
- Sizes: every blam.lst / blam.sav writer and the index readers move 0x2000 bytes; the first
  rewrites used body + 4 (and register_default_playlists read 0x9c bytes into a 0x98-byte local).

Consistency: `saved_game_slot_exists_for_id` was renamed `saved_game_name_is_available`
(it returns 1 when the name is free); callee externs that pointed at `src/game` functions by
`FUN_` names now use those names; FUN_ names of functions defined in this module were replaced
by their module names in every extern; shared globals now use one type and name per address
inside the module (`input_gamepad_count`, `no_simd_matrix_multiply_flag`,
`rasterizer_gamma_exponent`, `empty_string`, `missing_string_text`, `game_state_snapshot_source`).
All names established here are in `symbols/agent_phase4_saved_games.txt`;
`symbols/functions.txt` has not been regenerated.

## Known gaps

- Cross-module prototypes that disagree with objdump-confirmed call sites here (not edited,
  they belong to other modules): `src/game` `XCreateSaveGame` / `XDeleteSaveGame` take the wide
  save-game name in EAX (they call it `validity_token`); `savegame_index_read_slot` /
  `_write_slot` / `_remove_slot` take (slot, entry *) on the stack, and `savegame_index_append_slot`
  takes (entry *, int32_t *out_slot), not (unused, out_count); `path_append_component` in
  `src/game` and `src/networking` is declared with a `file_reference *`; `src/interface`
  still calls `FUN_0053d080`, `FUN_0053d1e0`, `FUN_0053b470` and declares
  `saved_game_find_by_name` with three arguments and `saved_game_delete_files` with none.
- 47 `UNSURE` markers remain in 29 files, mostly global identities outside the module
  (`0x00719738..0x00719779`, `0x007196f0`, `0x00722b6c`, `0x006894ba`, `0x00692af8`), the
  physical key each binding index names, and the recursive enumerator's depth bookkeeping.
- Reproduced quirks, not fixed: `control_profile_find_binding_for_action` lets the last pov
  match win; `control_profile_clear_device_slot_mappings` walks as many profile gamepad
  records as there are connected devices (it can run past the four); the default-playlist
  writer writes 0x1f64 bytes of uninitialised stack after the variant; `player_profile_rename`
  finalizes without copying when the path has no `blam.sav`.
- Not rewritten: nothing. `0x555d30` is not a function (see above).

## Functions

Rewrite confidence is the file's own figure after the review; "objdump-reviewed" means the whole
function was compared instruction by instruction during the phase-4 review.

| Address | Function | Bytes | Name conf. | Rewrite conf. | UNSURE | objdump-reviewed |
|---|---|---|---|---|---|---|
| `0x537f70` | game_state_dispatch_load_callbacks | 27 | 0.5 | 0.9 | 0 |  |
| `0x537f90` | game_state_startup | 98 | 0.5 | 0.75 | 0 |  |
| `0x538000` | game_state_build_header | 200 | 0.55 | 0.75 | 0 |  |
| `0x5380d0` | game_state_new | 122 | 0.6 | 0.8 | 0 |  |
| `0x538150` | game_state_new_pool | 112 | 0.55 | 0.8 | 0 |  |
| `0x5381c0` | game_state_perform_save | 50 | 0.55 | 0.85 | 3 | yes |
| `0x538200` | game_state_perform_revert | 122 | 0.5 | 0.85 | 6 | yes |
| `0x538280` | game_state_load_checkpoint | 149 | 0.5 | 0.75 | 1 |  |
| `0x538320` | game_state_read_checkpoint_summary | 100 | 0.4 | 0.55 | 0 |  |
| `0x538390` | game_state_load_core | 146 | 0.9 | 0.75 | 0 |  |
| `0x538430` | saved_game_verify_version_and_checksum | 405 | 0.55 | 0.7 | 0 |  |
| `0x5385d0` | game_state_after_load_restore_time | 31 | 0.45 | 0.6 | 1 |  |
| `0x5385f0` | game_state_allocate_buffer | 158 | 0.8 | 0.75 | 0 |  |
| `0x538690` | game_state_create_persistent_storage_file | 105 | 0.55 | 0.8 | 0 |  |
| `0x538700` | game_state_queue_write | 105 | 0.55 | 0.9 | 0 |  |
| `0x538770` | saved_game_file_exists | 100 | 0.5 | 0.75 | 0 |  |
| `0x5387e0` | saved_game_copy_files_to_target | 214 | 0.5 | 0.55 | 0 |  |
| `0x5388c0` | saved_game_delete_files | 192 | 0.55 | 0.7 | 0 |  |
| `0x538980` | game_state_save_thread_proc | 308 | 0.55 | 0.6 | 0 |  |
| `0x538ac0` | game_checkpoint_reclaim_slot_callback | 30 | 0.45 | 0.85 | 0 |  |
| `0x538ae0` | game_checkpoint_get_next_filename | 141 | 0.5 | 0.8 | 0 |  |
| `0x538b70` | game_checkpoint_write_stats_file | 232 | 0.55 | 0.8 | 0 |  |
| `0x538c60` | game_checkpoint_read_stats_file | 335 | 0.55 | 0.55 | 0 |  |
| `0x538db0` | game_checkpoint_save_new | 122 | 0.5 | 0.65 | 0 |  |
| `0x538e30` | saved_game_checkpoint_compare | 64 | 0.55 | 0.8 | 0 |  |
| `0x538e70` | game_checkpoint_enumerate_files | 671 | 0.6 | 0.8 | 0 | yes |
| `0x539110` | game_checkpoint_print_list_entry | 143 | 0.45 | 0.8 | 3 | yes |
| `0x5391a0` | saved_game_load_checkpoint_by_name | 238 | 0.55 | 0.5 | 0 |  |
| `0x539290` | saved_game_load_checkpoint | 155 | 0.55 | 0.55 | 0 |  |
| `0x539330` | game_state_read_persistent_storage | 148 | 0.55 | 0.85 | 0 | yes |
| `0x5393d0` | game_state_write_profile_file | 143 | 0.5 | 0.7 | 0 |  |
| `0x539460` | game_state_read_profile_header | 123 | 0.5 | 0.7 | 0 |  |
| `0x5394e0` | game_state_read_profile_file | 135 | 0.5 | 0.7 | 0 |  |
| `0x539570` | saved_game_validate_crc | 408 | 0.55 | 0.6 | 0 |  |
| `0x539710` | game_state_write_persistent_storage | 319 | 0.55 | 0.65 | 0 |  |
| `0x539850` | game_state_read_persistent_storage_block | 141 | 0.5 | 0.7 | 0 |  |
| `0x5398e0` | game_state_open_persistent_storage | 350 | 0.6 | 0.55 | 0 |  |
| `0x539a40` | player_profile_verify_thread_wait_and_clear | 94 | 0.4 | 0.7 | 0 |  |
| `0x539ab0` | saved_game_create_default_profile | 268 | 0.4 | 0.45 | 1 |  |
| `0x539bc0` | player_profile_get_or_cached_default | 39 | 0.4 | 0.65 | 0 |  |
| `0x539bf0` | player_profile_save_539bf0 | 36 | 0.7 | 0.9 | 0 | yes |
| `0x539c20` | player_color_get_rgb | 132 | 0.65 | 0.9 | 0 | yes |
| `0x539cb0` | player_profile_select_local_slot | 152 | 0.5 | 0.8 | 1 | yes |
| `0x539d50` | player_profile_mark_level_visited_and_select | 162 | 0.35 | 0.8 | 2 | yes |
| `0x539e00` | player_profile_scan_campaign_progress | 482 | 0.3 | 0.45 | 0 |  |
| `0x539ff0` | control_profile_reset_digital_bindings | 212 | 0.55 | 0.6 | 1 |  |
| `0x53a0d0` | control_profile_reset_analog_bindings | 116 | 0.55 | 0.65 | 1 |  |
| `0x53a150` | player_profile_set_default_server_options | 105 | 0.4 | 0.6 | 0 |  |
| `0x53a1c0` | player_profile_initialize | 1089 | 0.55 | 0.8 | 0 | yes |
| `0x53a610` | player_profile_write_default_files | 347 | 0.4 | 0.4 | 1 |  |
| `0x53a770` | player_profile_get | 470 | 0.5 | 0.5 | 0 |  |
| `0x53a950` | player_profile_write_data | 194 | 0.5 | 0.8 | 0 | yes |
| `0x53aa20` | control_profile_find_binding_for_action | 723 | 0.6 | 0.85 | 2 |  |
| `0x53ad00` | control_profile_clear_binding | 257 | 0.5 | 0.9 | 0 |  |
| `0x53ae10` | control_profile_set_binding | 489 | 0.5 | 0.85 | 0 |  |
| `0x53b000` | player_profile_set_default_video_options | 563 | 0.55 | 0.7 | 3 | yes |
| `0x53b240` | player_profile_set_default_audio_options | 107 | 0.4 | 0.7 | 0 |  |
| `0x53b2b0` | control_profile_reset_slot | 184 | 0.55 | 0.75 | 0 |  |
| `0x53b370` | control_profile_is_customized | 243 | 0.5 | 0.65 | 0 |  |
| `0x53b470` | control_profile_find_or_create_gamepad_slot | 133 | 0.45 | 0.55 | 0 |  |
| `0x53b500` | control_profile_finalize_slot | 149 | 0.4 | 0.85 | 1 | yes |
| `0x53b5a0` | control_profile_clear_device_slot_mappings | 122 | 0.4 | 0.45 | 1 |  |
| `0x53b620` | control_profile_reestablish_device_slot_mappings | 131 | 0.4 | 0.6 | 0 |  |
| `0x53b6b0` | control_profile_gamepad_slot_find | 62 | 0.55 | 0.85 | 0 |  |
| `0x53b700` | control_profile_copy_gamepad_bindings_by_key | 227 | 0.5 | 0.75 | 0 |  |
| `0x53b7f0` | control_profile_fill_default_gamepad_slots | 436 | 0.4 | 0.8 | 4 | yes |
| `0x53b9b0` | saved_game_delete_by_display_name | 292 | 0.55 | 0.8 | 0 | yes |
| `0x53bae0` | control_profile_variant_write_wait_and_clear | 94 | 0.5 | 0.8 | 0 |  |
| `0x53bb50` | saved_game_create_custom_variant | 288 | 0.45 | 0.5 | 2 |  |
| `0x53bc70` | playlist_profile_create_default_profiles_on_disk | 607 | 0.85 | 0.7 | 0 | yes |
| `0x53bee0` | saved_game_get_variant | 459 | 0.4 | 0.4 | 2 |  |
| `0x53c0b0` | game_variant_write_request_start | 141 | 0.4 | 0.6 | 0 |  |
| `0x53c150` | game_variant_write_thread_proc | 260 | 0.4 | 0.4 | 1 |  |
| `0x53c260` | saved_game_files_initialize | 535 | 0.9 | 0.6 | 0 |  |
| `0x53c480` | saved_game_files_dispose | 89 | 0.9 | 0.7 | 0 |  |
| `0x53c4e0` | saved_game_enumerate_by_type | 278 | 0.8 | 0.85 | 0 | yes |
| `0x53c600` | saved_game_get_display_name | 93 | 0.5 | 0.7 | 0 |  |
| `0x53c660` | saved_game_create_slot | 764 | 0.8 | 0.8 | 1 | yes |
| `0x53c960` | saved_game_delete_by_handle | 143 | 0.55 | 0.75 | 1 | yes |
| `0x53c9f0` | saved_game_open_file_by_handle | 136 | 0.5 | 0.75 | 0 |  |
| `0x53ca80` | saved_game_allocate_new_slot | 221 | 0.5 | 0.8 | 0 | yes |
| `0x53cb70` | player_profile_copy_files | 769 | 0.6 | 0.8 | 0 | yes |
| `0x53ce80` | player_profile_rename | 509 | 0.5 | 0.75 | 1 | yes |
| `0x53d080` | saved_game_get_directory_by_handle | 159 | 0.4 | 0.75 | 0 | yes |
| `0x53d120` | saved_game_check_storage_availability | 182 | 0.8 | 0.6 | 0 |  |
| `0x53d1e0` | saved_game_name_is_available | 57 | 0.8 | 0.85 | 0 | yes |
| `0x53d220` | saved_game_last_profile_clear | 142 | 0.55 | 0.7 | 0 |  |
| `0x53d2b0` | saved_game_last_profile_read | 168 | 0.5 | 0.7 | 0 |  |
| `0x53d360` | saved_game_last_mp_variant_clear | 142 | 0.5 | 0.7 | 0 |  |
| `0x53d3f0` | saved_game_last_mp_variant_read | 168 | 0.5 | 0.7 | 0 |  |
| `0x53d4a0` | saved_game_find_by_name | 306 | 0.5 | 0.85 | 0 | yes |
| `0x53d5e0` | saved_game_last_mp_map_clear | 142 | 0.5 | 0.7 | 0 |  |
| `0x53d670` | saved_game_last_mp_map_read | 168 | 0.5 | 0.7 | 0 |  |
| `0x53d720` | saved_game_list_rebuild_index | 887 | 0.6 | 0.8 | 0 | yes |
| `0x53daa0` | saved_game_index_open_for_write | 148 | 0.5 | 0.6 | 0 |  |
| `0x53db40` | saved_game_index_register_default_playlists | 667 | 0.5 | 0.75 | 1 | yes |
| `0x53dde0` | saved_game_index_register_default_profiles | 633 | 0.5 | 0.75 | 1 | yes |
| `0x5554c0` | file_reference_init | 96 | 0.5 | 0.75 | 0 |  |
| `0x555520` | directory_ensure_empty | 144 | 0.5 | 0.6 | 0 |  |
| `0x5555b0` | file_reference_create | 189 | 0.8 | 0.7 | 1 |  |
| `0x555670` | file_reference_delete | 173 | 0.8 | 0.7 | 0 |  |
| `0x555720` | file_reference_exists | 114 | 0.8 | 0.8 | 0 |  |
| `0x5557a0` | file_reference_open | 230 | 0.8 | 0.75 | 0 |  |
| `0x555890` | file_reference_close | 90 | 0.8 | 0.85 | 0 |  |
| `0x5558f0` | file_reference_seek | 87 | 0.8 | 0.85 | 0 |  |
| `0x555950` | file_reference_get_size | 81 | 0.8 | 0.85 | 0 |  |
| `0x5559b0` | file_reference_set_length | 98 | 0.5 | 0.8 | 0 |  |
| `0x555a20` | file_reference_read | 112 | 0.8 | 0.85 | 0 |  |
| `0x555a90` | file_reference_write | 103 | 0.8 | 0.85 | 0 |  |
| `0x555b00` | file_reference_get_size_by_path | 139 | 0.5 | 0.75 | 0 |  |
| `0x555b90` | file_enumerate_start | 125 | 0.45 | 0.65 | 1 |  |
| `0x555c10` | file_enumerate_find_next | 304 | 0.8 | 0.5 | 1 |  |
| `0x555ec0` | path_append_component | 84 | 0.8 | 0.85 | 0 |  |
| `0x555f20` | path_append_extension | 84 | 0.5 | 0.85 | 0 |  |
| `0x555f80` | path_remove_last_component | 113 | 0.8 | 0.55 | 1 |  |
| `0x556000` | path_split_components | 202 | 0.4 | 0.45 | 1 |  |
| `0x5560d0` | path_build_full | 152 | 0.5 | 0.7 | 0 |  |
| `0x556170` | saved_games_report_last_error | 55 | 0.5 | 0.85 | 0 |  |
| `0x555d30` | *(not a function: mid-body of file_enumerate_find_next)* | | | | | yes |
