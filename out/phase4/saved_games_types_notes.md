# saved_games module: type recovery notes

Header: `types/saved_games.h`. Smoke test: `out/phase4/saved_games_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/saved_games_smoke.c
```

It passes with `-Wall -Wextra` (and with `-m32`). It asserts every struct size, ~90 field
offsets, the five carry-over block lengths of `player_profile_initialize`, the three
per-gamepad binding strides and the address arithmetic of the global blocks, so a field
reordering breaks the build. A translation unit that includes every header in `types/`
(hs.h included) together with this one also compiles, and there it additionally proves
`sizeof(file_reference) == sizeof(file_reference_record)`.

Like `interface.h`, this header is not standalone: it embeds `game_variant` and
`win32_find_dataa` (game.h), `controls_gamepad_record` (interface.h) and points at
`network_mutex_record` / `network_thread_record` (networking.h), so the smoke file includes
`tags.h memory.h math.h game.h networking.h interface.h` first. Ghidra ingests `types/*.h`
alphabetically and all of those sort before `saved_games.h`, so the by-value embeddings
resolve in the DTM.

Nothing already defined elsewhere is redefined: `data_array` / `memory_pool` (memory.h, the
two game-state allocators), `game_variant`, `win32_find_dataa`, `savegame_index_record`
(game.h), `controls_gamepad_record` (interface.h), `network_*_record` (networking.h),
`file_reference` (hs.h).

## Binary evidence the layouts hang on

- **0x14c game-state header**: `game_state_startup` reserves 0x14c and crcs the literal;
  `game_state_build_header` zeroes 0x53 dwords. `+0x148` is `lea eax,[ecx+0x148]` at
  0x538a41 (the save thread passing the crc slot to `game_state_write_persistent_storage` in
  EAX) and the `lea eax,[esp+0x14c]` pointer both checkpoint readers pass
  `saved_game_validate_crc` (0x538290 / 0x538328). `+0x126` is read at 0x5382ce and 0x53834b.
- **0x440000 arena**: `game_state_startup` pushes 0x400000 and passes 0x40000 in ECX to
  `game_state_allocate_buffer` (0x537f92/0x537f97); the file is pre-sized to 0x480000.
- **0x48 checkpoint entry**: qsort width 0x48, GlobalAlloc 0x1cb0 = 0x66 * 0x48. The
  field order of the three numbers is from the writer disassembly 0x538bdc..0x538c07 (push
  tick, push difficulty, push name, call 0x4c8b90, `add esp,4`, push result -> fprintf sees
  level, difficulty, tick) and the reader call at 0x538f6c..0x538f7f (EBX = difficulty out,
  second arg = tick out, third = SYSTEMTIME).
- **Profile body 0x1ffc / file 0x2000**: every crc32 over the body is 0x1ffc long and the
  compared dword is the next one (`local_4` vs `local_2000` in `player_profile_get`).
  `player_profile_initialize`'s merge copies are 0x24f, 0x44, 0x42, 0x42+3 and 0x220
  dwords, which pin 0x12c..0xa68, 0xa68..0xb78, 0xb78..0xc80, 0xc80..0xd8b, 0x1108..0x1988.
- **Variant file 0x9c**: crc over 0x98 (= `sizeof(game_variant)`) then the crc dword.
- **Index entry 0x206**: `file_reference_read` with ESI 0x206 at 0x53c561; field offsets from
  the stack frames of `saved_game_create_slot`, `saved_game_list_rebuild_index` and both
  default registrars; the flag bytes from the handle packer call at 0x53c588..0x53c5a7
  (DL = +0x204 -> bit 30, stack = +0x205 -> bit 31, ECX = +0x200 type, EAX = loop index).
- **file_reference 0x10c**: 0x43-dword memset everywhere, handle at +0x108 in every helper.
- **.data dumps**: 0x0069e7ac..0x0069e8d0 (callbacks, player colors, variant builders) and
  0x0069fa50..0x0069fba0 (files globals) were read with `objdump -s`.

## Structs, and which functions established which fields

### `game_state_header` (0x14c)
| offset | field | established by |
|---|---|---|
| 0x000 | allocation_checksum | 0x538000 writes `game_state_crc`; 0x538430 compares with 0x006e2dd4 |
| 0x004 | scenario_name[0x100] | 0x538000 strcpy from `tag_instances[global_scenario_index]+0x10`; 0x538430 strcmp; 0x538320 copies it out (EDI); 0x538980 passes it to the stats writer |
| 0x104 | build_version[0xe] | 0x538000 writes 01.00.10.0621 as 3 dwords + word; 0x538430 compares 0xe bytes against nine build strings |
| 0x124 | local_player_count | 0x538000 from 0x006894b8; 0x538430 compares |
| 0x126 | difficulty | 0x538000 from game globals +0x0e; 0x538280 compares with 0x00696564; 0x538320 returns it (ESI); 0x538980 passes it to the stats writer |
| 0x128 | map_checksum | 0x538000 from 0x006a81b8 (= `cache_file_current_header.crc32`); 0x538430 compares |
| 0x148 | file_checksum | 0x539710 (EAX) writes; 0x539570 reads/compares |

### `checkpoint_file_entry` (0x48)
`game_checkpoint_enumerate_files` 0x538e70 (fills every field), `saved_game_checkpoint_compare`
0x538e30 (+0x24 then FILETIME +0x1c), `game_checkpoint_read_stats_file` 0x538c60 and
`game_checkpoint_write_stats_file` 0x538b70 (the three numbers and SYSTEMTIME), callbacks
0x538ac0 / 0x539110 (argument order: index, name, level, difficulty, tick, time, user).

### `win32_systemtime` (0x10)
0x538c60 rebuilds it as four CONCAT22 dwords; 0x538b70 prints wMonth/wDay/wYear and
wHour/wMinute/wSecond from GetLocalTime.

### `saved_player_profile` (0x1ffc)
| range | fields | established by |
|---|---|---|
| 0x000 | version = 9 | 0x53a1c0 writes; 0x53a770 rejects anything else |
| 0x002 | name[12] | 0x539ab0 / 0x53a770 `wcsncpy(+2, .., 0xb)` then clear +0x18; 0x53b9b0 wcsicmp; 0x53a950 passes +2 to the rename |
| 0x11a | player_color = -1 | 0x53a1c0; 0x539c20 clamps the index to 0..0x11 |
| 0x11c | flags | 0x53a1c0 `|= index << 8 | 1`; 0x539ab0 / 0x53a770 clear; 0x53b9b0 tests bit 1 |
| 0x11e | campaign_progress[10] | 0x539d50 sets bit difficulty of byte level; 0x539e00 scans the 10 bytes |
| 0x128 | last_campaign_level | 0x53a1c0 clears; 0x539cb0 compares/updates |
| 0x12c..0x133 | button_set .. unknown_133 | 0x53a1c0 defaults |
| 0x134 | keyboard_bindings[0x6d] | 0x539ff0, 0x53a1c0, 0x53ae10 (type 1), 0x53ad00, 0x53aa20 (bound 0x6c) |
| 0x20e | mouse_button_bindings[8] | 0x53a0d0, 0x53ae10 / 0x53ad00 (type 2, kind != 1), 0x53aa20 (8 entries) |
| 0x21e | mouse_axis_bindings[3][2] | 0x53a0d0, 0x53ae10 (type 2, kind 1, direction 1 -> +0x21e, else +0x220) |
| 0x22a | gamepad_button_bindings[4][32] | 0x53a1c0, 0x53b2b0, 0x53b370, 0x53b700, 0x53ae10 (type 3 button) |
| 0x32a | gamepad_action_buttons[4][2] | 0x53ae10 (actions 8 / 9 store the button index), 0x53aa20, 0x53b2b0 (-1), 0x53b370, 0x53b700 |
| 0x33a | gamepad_axis_bindings[4][32][2] | 0x53ae10 (kind 1), 0x53a1c0, 0x53b2b0, 0x53b370, 0x53b700 |
| 0x53a | gamepad_pov_bindings[4][16][8] | 0x53ae10 (kind 2), 0x53a1c0, 0x53b2b0, 0x53b370, 0x53b700 |
| 0x93c..0x967 | floats and slider bytes | 0x53a1c0 defaults; 0x53b700 copies 0x956+i / 0x95a+i per gamepad; semantics of the slider tables from types/interface.h `player_control_settings` |
| 0xa68..0xa76 | width, height, refresh, flags, gamma | 0x53b000 |
| 0xb78..0xb7f | volumes and sound options | 0x53a1c0, 0x53b240; names from src/interface/audio_options_apply_from_profile.c |
| 0xc80..0xc8a | 11 bytes of defaults | 0x53a1c0 |
| 0xd8c, 0xeac, 0xebe, 0xebf, 0xfc0, 0xfc2, 0x1002, 0x1004 | network defaults | 0x53a1c0 and 0x53a150 (identical writes); ports 0x8fe / 0x8ff |
| 0x1108 | gamepads[4] (`controls_gamepad_record`) | 0x53b2b0 / 0x53b470 (0x88-dword slot), 0x53b370 / 0x53b500 / 0x53b620 / 0x53b7f0 (first word = used), 0x53b500 (+0x20c..+0x218 guid by value), 0x53b6b0 (+0x21c then +0x20c..+0x218) |

### `saved_player_profile_file` (0x2000), `game_variant_file` (0x9c)
Crc placement from 0x539ab0, 0x53a610, 0x53a770, 0x53a950, 0x53c660 (generic: zero 0x800
dwords, crc at +size), 0x53d720, 0x53db40, 0x53dde0 (profiles) and 0x53bb50, 0x53bc70,
0x53bee0, 0x53c150 (variants).

### `saved_player_profile_slot` (0x2004)
0x539cb0 / 0x539d50 (`slot * 0x801` dwords, slot must be 0, handle at +0x1ffc =
0x00714dd4), 0x53a1c0 (copies slot 0 when a profile is loaded). +0x2000 is a byte only the
interface touches (0x49d186, 0x4a1852, 0x4a5b2b).

### `variant_write_request` (0x9c)
0x53c0b0 fills handle + 0x26 dwords and passes `&0x00721288` to `network_thread_create`;
0x53c150 reads `param+4` as the variant; 0x53bae0 / 0x53c260 zero 0x29 dwords.

### `saved_game_index_entry` (0x206)
| offset | field | established by |
|---|---|---|
| 0x000 | path[0x100] | 0x53c660 snprintf, 0x53d080 / 0x53ce80 strncpy, 0x53db40 / 0x53dde0 |
| 0x100 | display_name[0x80] | 0x53c600 (returns it), 0x53ce80 wcscmp / wcsncpy, 0x53c660, 0x53d720 |
| 0x200 | type | 0x53c4e0, 0x53d4a0 filters; 0x53d080 (0/1 only); 0x53ce80 branch; 0x53d720 writes 0, 1 or -1 |
| 0x202 | index | 0x53c660 (current count), 0x53d720 / 0x53db40 / 0x53dde0 (write counter) |
| 0x204 | builtin | 0x53db40 / 0x53dde0 set 1; 0x53c4e0 filter; handle bit 30 (0x53c960 skips XDeleteSaveGame) |
| 0x205 | checksum_valid | set after a crc match in 0x53d720 / 0x53db40 / 0x53dde0, after a successful write in 0x53c660; handle bit 31 |

### `xgame_find_data` (0x344)
0x53d720 frame: record at `local_2f58`, find data at `local_2d50` (+0x2c cFileName =
`local_2d24`, +0x140 directory = `local_2c10`, +0x244 wide name = `local_2b0c`);
`savegame_find_first` 0x551bc0 copies the directory to +0x140.

### `file_reference_record` (0x10c)
0x5554c0 (constructor), 0x5555b0 / 0x555670 (flags bit 0 = file vs directory), 0x5557a0 /
0x555890 / 0x5558f0 / 0x555950 / 0x5559b0 / 0x555a20 / 0x555a90 (handle +0x108),
0x555b90 / 0x555c10 (location +0x06 copied to the enumeration state, path +0x08), 0x5560d0
(location selector: 2 absolute, 1 root template, < 1 relative), 0x555520 (location -1).

### `control_binding_descriptor` (0x0c)
0x53aa20 writes kind/index/direction ([2], [3], dword [4]); 0x53ad00 / 0x53ae10 read
device_type [0], device_index [1], kind [2], index [3], direction dword at +8; the interface
passes it as `int16_t record[6]`.

### Enums / constants
All from literals in the listed functions: handle bits (0x53e630, 0x53c960), storage status
(0x53d120), checkpoint kind (0x538e70), control counts (0x53aa20 bounds, 0x53a1c0 fills),
file open modes (0x5557a0), enumeration flags (0x555c10), profile flag bits (0x53a1c0,
0x53b9b0), 38 default variants (0x53bc70 loop bound), 18 player colors (0x539c20 clamp).

## Globals

All listed in the header with `// global` lines. The game-state block is kept as individual
globals because several other modules already extern `game_state_base` / `game_state_cursor` /
`game_state_crc` by name. The two memset blocks (0x0071d280 + 0x1001 dwords, 0x00721330 +
0x2c7 dwords) and the 0x29-dword block at 0x00721288 are asserted in the smoke file so
every byte in them is accounted for. The unlisted `.data` word 0x0069e7ec and the bss gaps
0x006e2dd0, 0x00721284, 0x0072132c, 0x0069fa54 and 0x0069fb80..0x0069fba0 have no code
reference at all (checked against a full `objdump -d` of `.text`).

## Unresolved offsets

- `game_state_header` 0x112..0x124 and 0x12c..0x148: zeroed, never written or read.
- `saved_player_profile`: 0x01a..0x11a (0x100 bytes, zero), 0x12a, the meaning of
  0x12c..0x133 (only defaults), 0x93a, the six floats at 0x93c, 0x954 / 0x955, 0x95e,
  0x960 floats, 0x968..0xa68, video bytes 0xa6e..0xa75, 0xa77..0xb78, audio 0xb7b..0xb7f
  semantics, 0xb80..0xc80, the whole 0xc80..0xd8b block (defaults only), where the server
  name ends inside 0xd8c..0xeac, 0xebe, 0xebf, 0xec0..0xfc0, 0xfc0 / 0xfc1, 0xfc2 string
  length, 0x1006..0x1108 and 0x1988..0x1ffc. What actions 8 and 9 are (the two
  gamepad_action_buttons columns).
- `saved_player_profile_slot` +0x2000 (interface-only byte).
- `file_reference_record` +0x005 (zeroed, never read).
- Globals: 0x006e2dd8 (written once, never read), 0x0071f27c (0x2000 zeroed bytes, never
  referenced), 0x0072132a (written 1, never read), whether `file_enumeration_handles` is 8 or
  16 long (only 8 are seeded to -1; the depth counter is never bounds-checked).
- `game_variant_defaults_proc` return: the caller uses EAX as the source of its copy; the
  builders were not re-read to confirm they return their argument.

## Misattributed / misnamed functions in the range

- **0x555d30 `structure_render_lightmaps`**: not a function. 0x555d30 is the `je` operand
  in the middle of `file_enumerate_find_next` (0x555c10..0x555eb7); the label is a stale
  signature hit and Ghidra split the body there. No types; it should not be rewritten as a
  separate function.
- **0x538690 `chimera__multiple_instance_2`**: stale Chimera label; it creates and pre-sizes
  savegame.bin (`game_state_create_persistent_storage_file`).
- **0x5554c0..0x556170**: the Blam *files* module (file_reference, path helpers, directory
  enumeration), not saved-game logic proper. Its layout was never defined anywhere (hs.h
  keeps it opaque), so it is defined here as `file_reference_record`. 0x556170 is Blam code
  (FormatMessageA error reporter), not CRT.
- Summary corrections that affect types: 0x539ab0 does not build a network packet, it
  creates a new profile slot (`saved_game_create_slot(0, name)`) and writes a fresh default
  profile to it; 0x53c0b0 / 0x53c150 / 0x53bae0 are the asynchronous *game variant* writer,
  not a profile rename; 0x53ce80 (`player_profile_rename`) renames any saved game, both
  types; 0x539e00 scans `campaign_progress`; 0x538320 returns a checkpoint's difficulty
  (ESI) and scenario name (EDI).
- In-range functions missing from the 116-function list: **0x5385d0** (after-load proc 10,
  stores the game tick in 0x006e2ddc), **0x538ac0** (checkpoint reclaim callback, copies the
  first name into user_data) and **0x539110** (checkpoint list print callback, ticks to
  h:m:s, level name from 0x00696574). Their signatures match `game_state_proc` /
  `checkpoint_enumerate_proc`.

## Cross-module findings (not fixed here)

- `src/interface/FUN_0049cc80.c` and `FUN_0049ce00.c` read the per-level byte at profile
  +0x11c; the binary says +0x11e (their own Ghidra `abStack_3ee2` is `local_4000 + 0x11e`,
  and 0x539d50 / 0x539e00 agree). +0x11c is the flags word.
- Several interface files declare `saved_profile_records[3][0x2004]` at 0x00712dd8. This
  module accepts slot 0 only, and a second slot would overlap 0x00714e7c / 0x00714e80.
  (The dead 0x49cc80 does copy 0x00714ddc as if it were slot 1.)
- types/game.h: `savegame_index_mutex` (0x00721440) is a `network_mutex_record *`, not
  `void **` (same bytes); `game_variant.unknown_94` carries bit 0 = built-in and the default
  index in its high byte, like the profile flags word; `game_variant.name` is terminated at
  +0x2e (23 characters).
- types/hs.h: `file_reference` could become the field layout (or a typedef of
  `file_reference_record`) now that its owner module has one; left alone because hs.h must
  keep parsing standalone.

## Register conventions seen (confirm per function before rewriting)

`game_state_new` stride in BX; `game_state_allocate_buffer` extra size in ECX;
`saved_game_validate_crc` ECX total size, EDX header size, EBX header buffer;
`game_state_write_persistent_storage` EAX = crc slot; `game_state_read_persistent_storage_block`
EAX size; `file_reference_read` / `_write` EDX reference, ECX buffer, ESI size;
`file_reference_seek` ECX reference, EAX offset; `file_reference_get_size` / `_create` EAX
reference; `_open` / `_close` / `_delete` ESI reference; `path_append_component` ESI
destination, EBX component; `path_build_full` EAX source, EDX destination, CX location;
saved-game handle in EAX for 0x53c600 / 0x53c9f0 / 0x53ce80 / 0x53d080 and in EDI for
0x53c960; binding descriptor in ESI for 0x53aa20 / 0x53ad00 / 0x53ae10; profile base in EDX
(0x539ff0, 0x539e00), ECX (0x53a0d0), ESI (0x53a150), EAX (0x53b240). The file_reference_read
convention and the checkpoint/validate_crc ones were checked in objdump; the rest are
Ghidra register reads.
