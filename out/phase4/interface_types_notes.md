# interface module types — evidence notes

Target: retail Halo PC `halo.exe` 1.0.10, module `interface` (376 functions, 0x44c290..0x4c9c80).
Header: `types/interface.h`. Syntax gate: `out/phase4/interface_smoke.c`
(`gcc -fsyntax-only -I types out/phase4/interface_smoke.c`, exit 0).

The smoke file includes `tags.h`, `memory.h`, `math.h`, `game.h`, `networking.h`, `interface.h`.
`math.h` and `game.h` are only there because `networking.h` needs them.

**Parse-order dependency.** `interface.h` uses `datum_index` (memory.h), `ColorARGB` (tags.h)
and `s_network_address` (networking.h). Ghidra must ingest those three before `interface.h`.
`datum_index` is existing precedent — `types/ai.h` already refers to it 100 times and sorts
before `memory.h` — but `s_network_address` is a new backwards reference and is worth knowing
about if the CParser run is ever switched to plain alphabetical order.

---

## Layouts the binary itself states

These are not inferred from scattered arithmetic; the image contains the size.

| struct | size | where the binary states it |
|---|---|---|
| `first_person_weapon_interface` | 0x1ea0 | `interface_globals_allocate` @0x494340 bumps the game-state cursor by 0x1ea0 and folds that literal into `crc32_update`; `interface_local_player_state_reset` @0x494390 zeroes 0x7a8 dwords |
| `widget_instance` | 0x60 | `widget_initialize_from_tag` @0x499780 zeroes exactly 0x18 dwords before writing anything |
| `console_message` | 0x124 | `terminal_initialize` @0x4963d0 calls `data_new("terminal output", 0x20)`; every accessor multiplies the datum index by 0x124 |
| `hud_globals_flags` | 0x004 | `hud_state_allocate` @0x4a9780 reserves 4 bytes |
| `hud_messaging_globals` | 0x488 | same function reserves 0x488; `hud_state_reset` @0x4a98d0 zeroes 0x122 dwords |
| `hud_unit_meter_state` | 0x05c | reserved 0x5c; reset zeroes 0x17 dwords |
| `hud_weapon_interface_state` | 0x07c | reserved 0x7c; reset writes 0x1f dwords of -1 |
| `hud_waypoint_state` | 0x030 | reserved 0x30; reset writes 0xc dwords of -1 |
| `motion_sensor_globals` | 0x570 | reserved 0x570; `motion_sensor_reset` @0x4b3660 zeroes 0x15c dwords |
| `server_browser_entry` | 0x220 | `server_list_add` @0x4b5800 copies 0x88 dwords; `server_list_remove` @0x4b5850 compacts with `count * 0x220` |
| `controls_device_label` | 0x210 | `controls_build_device_label_table` @0x4b4890 zeroes 0x840 dwords (0x2100 = 16 * 0x210) |
| `hud_text_message` | 0x14 | `hud_text_message_queue_init` @0x4a3ce0 writes `element_size = 0x14` into the `growable_array` header at 0x006b37e8 |
| `ui_list_item` | 0x10 | the three `growable_array` headers at 0x006b3830 are created with element size 0x10; `ui_list_free_all` @0x4a7b20 walks the group with a 0xc stride and stops at 0x006b385c, which is what fixes the group at three arrays |
| `map_list_entry` | 0x0c | `map_list_add_entry` @0x4950c0 computes the `GlobalReAlloc` size as `capacity * 0xc` |

`motion_sensor_reset` is the single best piece of layout evidence in the module: after the
0x570 byte zero fill it writes the empty-blip marker into 10 groups of 16 records, stepping the
record by 4 bytes and the group by 0x84. That one loop gives `motion_sensor_blip` (4),
`motion_sensor_frame` (0x84) and the history depth (10) at once.

---

## Per-struct field provenance

### `ui_key_event` (0x04)
`console_process_queued_input` @0x4965e0 pulls one 4-byte record out of the key ring at
0x006b16fe (cursor 0x006b16fa, limit 0x006b16fc) and passes it to 0x44c290, which reads
`+0x00 & 1` (shift), `+0x01` (character, 0xff = none, below 0x20 = not printable) and the
short at `+0x02` (key code, special values 0x1d / 0x4f / 0x50 / 0x54).

### `text_edit_state` (0x0a)
- `+0x00 text` — dereferenced and `strlen`ed by all five of 0x44c290, 0x44c5b0, 0x44c5e0,
  0x44c640, 0x44c780.
- `+0x04 maximum_length` — `(short)unaff_ESI[1]` in 0x44c640 gates insertion; `console_open`
  @0x496510 writes 0xff into it (`unaff_EDI[0xdc]`, i.e. console + 0x1b8 = edit + 0x04).
- `+0x06 cursor` — `*(short *)((int)p + 6)`, clamped into `[0, strlen]` by 0x44c780.
- `+0x08 selection_anchor` — `*(short *)(p + 2)` as a dword pointer, clamped into `[-1, strlen]`
  and collapsed to -1 when it equals the cursor.

`console_open` is the confirmation that the four fields are in that order and that the struct
is 0x0a: it sets `edit.text = &console->input[0]`, `edit.maximum_length = 0xff`,
`edit.cursor = strlen(text)` and `edit.selection_anchor = 0xffff` at console+0x1b4/b8/ba/bc.

### `widget_instance` (0x60)
Written by `widget_initialize_from_tag` @0x499780 unless noted.
- `+0x00 definition`, `+0x04 name` — `*in_ECX = in_EAX` (tag index) and
  `in_ECX[1] = param_2 + 2`, i.e. tag data + 4, which is `UIWidgetDefinition::name`.
- `+0x08 controller_index` — `widget_open` @0x497a70 maps `definition->controller_index`
  0..4 to 0,1,2,3,-1 and passes it in; `widget_close` @0x497c00 uses it to index a 0x40-stride
  per-controller array at 0x006b145c.
- `+0x0a local_x`, `+0x0c local_y` — `widget_create_children_from_tag` @0x499540 sets a new
  child from `ChildWidgetReference + 0x38` and `+ 0x36` plus the parent values.
  `widget_instance_point_in_bounds` @0x4999f0 sums `+0x0a` against the X bound and `+0x0c`
  against the Y bound while walking `+0x30`, which fixes which one is which.
- `+0x0e widget_type` — `*param_2` (tag data + 0). Compared against 1, 2 and 3 throughout.
- `+0x10 state` — set to 1 at creation, rewritten recursively by 0x498e60.
- `+0x11`, `+0x13` — `(definition->flags >> 9) & 1` and `(definition->flags >> 1) & 1`, i.e.
  `render_regardless_of_controller_index` and `pause_game_time` from the tags.h bitfield list.
  `+0x13` is what drives the `ui_pause_depth` counter at 0x00718fa6 in both 0x499780 and 0x497c00.
- `+0x12 hidden` — `*(char *)(child + 0x12) != 0` skips the child in the default-focus search
  (0x499780), in 0x499540 and in 0x49bba0.
- `+0x14 closing` — `widget_close` latches it to 1 on entry and returns early if already set.
- `+0x18 creation_time` — copied from 0x00718f9c.
- `+0x1c`, `+0x20` — tag fields 0x30 and 0x34 with the idiom `x & ((x < 1) - 1)`, i.e. clamp
  negatives to zero.
- `+0x24 scale` — 1.0 at creation; `widget_instance_get_cumulative_scale` @0x499c20 multiplies
  it by the same field of every ancestor reached through `+0x30`.
- `+0x28/+0x2c/+0x30/+0x34` — `widget_create_children_from_tag` links siblings
  (`tail->+0x2c = new; new->+0x28 = tail`), `widget_get_sibling_index` @0x498e30 walks
  `parent(+0x30)->first_child(+0x34)` then `+0x2c`, and `widget_close` unlinks all four.
- `+0x38 focused_child` — 0x499780 stores the chosen default-focus child here;
  0x49c000/0x49c040/0x49c080/0x49c0f0 move it around the sibling ring.
- `+0x3c text` — `widget_close` frees `puVar6[0xf]` back to the widget heap when
  `widget_type == 1` (text_box); 0x49b1d0 is the routine that allocates it.
- `+0x40 selection_index`, `+0x48 item_count` — `widget_list_scroll_window` @0x4a7400 reads
  `(short)(w + 0x40)` as the selected index and `(ushort)(w + 0x48)` as the wrap count;
  0x499780 seeds `+0x40` to -1 for a text_box and 0x499540 increments `+0x48` once per child.
- `+0x44 list_items` — allocated (400 bytes, `heap_reallocate`) by `ui_build_profile_list`
  @0x49dd70 and freed by 0x49df70 and 0x4a0a30.
- `+0x4c extended_description` — loaded from tag field 0x1b0 by 0x499540 for list types and
  closed recursively by `widget_close`.
- `+0x50 list_render_data` — freed by `widget_close` when `1 < type < 4`.
- `+0x58`, `+0x5e` — see the open question below.

### `widget_history_node` (0x10)
`list_node_prepend` @0x499430 allocates from the widget heap and copies three dwords from a
template plus the old head into `+0x0c`; `list_node_pop` @0x499460 reverses it.
`widget_instance_close_and_restore_previous` @0x49c3e0 pops one, reloads `+0x00` through
`widget_open` with `+0x04` as the controller argument, and hands the low half of `+0x08` to
`widget_instance_select_list_index` @0x49bd00.

### `ui_pending_error` (0x04)
`display_error` @0x498f20 writes `(&DAT_00718fb6)[i*2]` as a short and
`(&DAT_00718fb8)[i*4]` / `(&DAT_00718fb9)[i*4]` as bytes for the same `i`, which is only
consistent with a 4-byte record `{short, byte, byte}` based at 0x00718fb6.

### `console_message` (0x124)
`console_printf` @0x496b50 pins almost everything: `vsnprintf(msg + 0x0d, 0xfe, ...)`,
the four colour dwords at 0x110/0x114/0x118/0x11c (defaulting to 1.0, 0.7, 0.7, 0.7), the echo
flag at 0x0c and `age = 0` at 0x120. `console_message_expire_old` @0x4966e0 increments 0x120
and deletes past 150. `console_message_new` @0x496420 and `console_message_delete` @0x496490
give the list links: `+0x04` toward the head (0x006b2f04, newest) and `+0x08` toward the tail
(0x006b2f08, oldest).

### `terminal_console` (0x1be)
- `+0x00` / `+0x02` — `console_process_queued_input` appends at
  `psVar3 + (*psVar3)*2 + 1` (short units), i.e. byte 2 + count*4, and refuses past 0x20.
- `+0x84` colour — `console_draw_overlay` @0x496730 copies console+0x84/88/8c/90 into the text
  renderer alpha/red/green/blue globals.
- `+0x94 prompt` / `+0xb4 input` — the same function NUL-terminates 0xb3 and 0x1b3, and
  `console_restore_cursor` @0x496c20 does `strncpy(console_window_title, console + 0x94, 0x1f)`
  while `console_draw_input_line` @0x4970a0 formats `"%s %s"` from the title and console + 0xb4.
- `+0x1b4 edit` — see `text_edit_state` above. `console_update_display` @0x496d40 and
  `console_position_cursor` @0x4971a0 both read the cursor at `+0x1ba`, which is `edit.cursor`.

### `map_list_entry` (0x0c)
`map_list_add_entry` @0x4950c0: `+0x00` is a `GlobalAlloc`/`GlobalReAlloc` path buffer that is
lowercased and has its extension stripped, `+0x04` is the caller-supplied id, `+0x08` is the
return value of `cache_file_exists`, and the count at 0x00712dd0 is incremented last.
`map_list_free_all` @0x495260 frees `+0x00` for each entry and then the array.

### `video_resolution` (0x4c)
`video_resolution_add` @0x4badc0 indexes `refresh_rate_count` as
`(&DAT_006b66b8)[i * 0x13]` on a dword array — a 0x4c byte stride — and the rate array as
`&DAT_006b66bc + (i*0x13 + n)*2` on the same dword base, so `refresh_rates` starts 4 bytes
after the count and is capped at 8 by an explicit `uVar1 < 8`. The width/height pair at
+0x00/+0x04 is confirmed independently by `video_resolution_compare` @0x4bab50, which orders
by `a[0]` then `a[1]`. The 0x1e bytes at +0x08 are the `"%d x %d"` wide name.

### `ui_list_item` (0x10)
`ui_list_add_entry` @0x4a7ba0 writes `+0x00` (`GlobalAlloc`ed wide name), `+0x04` (optional
`GlobalAlloc`ed blob), `+0x08` (id) and the byte at `+0x0c` which, when set, also raises
0x007192f8. `ui_list_get_data` @0x4a7c50 and `ui_list_get_id` @0x4a7c80 read `+0x04` and
`+0x08` at `i * 0x10`; `ui_list_find_default` @0x4a7cb0 scans `+0x0c` at the same stride.

### `server_browser_entry` (0x220)
`server_list_find` @0x4b5760 compares `entry + 0x21c` first and then four dwords from
`entry + 0x20c` against the same offsets of the caller record — a 0x14 byte key at 0x20c,
which is byte for byte `types/networking.h s_network_address` (ipv4 at +0x00, size at +0x10,
port at +0x12). The two lists are distinguished by pointer identity, not by a field:
0x006b42d8 has capacity 8 (favorites) and 0x006b53d8 capacity 4 (history), and
`0x006b53d8 - 0x006b42d8 == 0x1100 == 8 * 0x220` confirms the favorites array sits immediately
before the history array.

### `controls_device_label` (0x210)
`controls_device_label_add` @0x4b4830: `wcsncpy(base + i*0x210, name, 0x104)`, terminator
short at `+0x208`, id dword at `+0x20c`. The table bound is the loop test `psVar1 < 0x6953e8`,
giving 16 entries from 0x006932e8.

### `first_person_weapon_interface` (0x1ea0)
- `+0x00 attached` — tested and written by 0x493c60, 0x493e50, 0x4926f0.
- `+0x04 unit_index` — 0x493c60 resolves it through the object header array at 0x008603b0
  (stride 0xc, data pointer at +8) and then reads `unit + 0x2f2` (current weapon slot) and
  `unit + 0x2f8 + slot*4` (weapon object).
- `+0x08 weapon_index` — the result of that lookup; `local_player_index_for_object` @0x4926f0
  matches against it.
- `+0x0c state` — switched on by 0x492d20 and validated by `first_person_weapon_set_state`
  @0x492e60.
- `+0x12 shutdown_countdown` — reseeded to 0x1e by 0x4942e0.
- `+0x14 animation_index` — 0x493c60 writes the animation-graph index it found, guarded by
  `graph->...[idx * 0xb4 + 0x22] > 8`.
- `+0x2c charge` — action code 0 in `first_person_weapon_process_action` @0x4940f0 adds to it.
- `+0x88 / +0x8a` and the two 0x800 byte blocks — `first_person_weapon_snapshot_pose`
  @0x4930b0 copies from `+0x8c` to `+0x88c` and updates the two shorts at `+0x88`/`+0x8a`.
  0x800 is the gap between the two blocks, not a stated size; see the open questions.
- `+0x1d8c` / `+0x1e0e` — `hud_meter_find_matching_element` @0x493f00 is called twice by
  0x493c60, with output tables at `+0x1d8e` and `+0x1e10` and the success byte stored just
  before each table. 0x4924b0 gates on the same two bytes.

### `hud_message_slot` (0x8c) and `hud_player_messaging_state` (0x460)
`hud_message_find_slot` @0x4ae480 loops `index < 4` with a 0x8c stride and reads `+0x00`
(oldest wins), `+0x84` (`piVar3[0x21]`, the source key), `+0x8a` (the source kind) and `+0x82`
(active). `hud_message_add` @0x4ae180 does `wcsncpy(slot + 4, text, 0x3f)` — which is what
puts `active` at 0x82 rather than 0x84 — then stamps `+0x00` from game time + 0xc, sets
`+0x84 = -1`, `+0x82 = 1` and `+0x83` from the rolling counter at `hud_messaging + 0x465`.
`hud_message_compare` @0x4ae500 orders by `+0x00`, then `+0x84`, then `+0x83`.

The player record: `hud_action_prompt_set` @0x4ae110 does
`wcsncpy(record + 0x230, L"", 0xff)` and writes `+0x454 = 0`, `+0x458`, `+0x45e`, `+0x45f`;
`hud_message_set_numeric_argument` @0x4ae050 and `..._string_argument` @0x4ae0b0 write
`record + 0x434 + i*4` and clear/set bit i of the byte at `+0x459`, gated on `+0x458` and
`+0x454`. Every one of these multiplies the local player index by 0x460.

### `hud_messaging_globals` (0x488)
`hud_counter_get_value` @0x4adcc0 reads the block base, not a player record: `+0x487`
(active), `+0x47c` (target, 0xffff means no value), `+0x486` (absolute) and `+0x478` (base).
Since the player record is 0x460 and the block is 0x488, the tail is exactly 0x28 bytes and
those four offsets all land inside it.

### `hud_waypoint` (0x0c)
`hud_waypoint_set` @0x4af0d0 and `hud_waypoint_clear` @0x4af230 both iterate `index < 4` with
a 0xc stride inside `hud_waypoints + local_player * 0x30`. `+0x00` is the string index
(0xffff empty), the low nibble of `+0x02` is the type (0xf free), `+0x04` is the caller
payload and `+0x08` is the object index (-1 free). `hud_waypoint_update_for_player` @0x4af370
reads all four the same way.

### motion sensor
`motion_sensor_update_for_player` @0x4b3e10 completes what `motion_sensor_reset` started:
- blip x/y are `puVar9[-2]` and `puVar9[-1]` relative to the type byte, so the record is
  `{int8 x, int8 y, uint8 type, uint8 subtype}`;
- `blip_fill` @0x4b35f0 writes `+0x02` (from `blip_type_get`) and `+0x03` (0..2);
- `frame + 0x70/0x74` take the camera position, `frame + 0x7c` the camera yaw plus half pi;
- `frame + 0x60` is a 0x10 byte source array filled by 0x462190 and `frame + 0x80` its count;
- `frame + 0x40 + n*2` holds the int8 x/y pair for each of those extra blips;
- the tracked object handles are `player_base + 0x528 + n*4`, 16 of them, which together with
  the 10 * 0x84 history exactly fills the 0x568 per-player stride that
  `motion_sensor_render` @0x4b4120 uses;
- `motion_sensor + 0x56c` is the frame cursor (`(cursor - k + 10) % 10` in the render loop)
  and `+0x56e` the enable byte.

### `virtual_keyboard_globals` (0x74 at 0x007193a8)
Every field address is written literally by `virtual_keyboard_initialize` @0x4a88f0 and
`virtual_keyboard_open` @0x4a89a0. The text buffer is `wcsncpy(&DAT_007193d0, src, 0x20)`,
so 0x40 bytes at +0x28; 0x0071940e is its last wide character, which is why the buffer ends
exactly where `name_is_valid` (0x00719410) begins. `virtual_keyboard_close` @0x4a9250 copies
back with `maximum_length >> 1` wide characters and NUL-terminates at
`(maximum_length & ~1) - 2`, confirming `maximum_length` counts bytes.

---

## Unresolved offsets

- `widget_instance + 0x58` and `+ 0x5e`. 0x499780 sets `+0x5e` from
  `background_bitmap_tag->sequences[0].bitmap_count`, and 0x49c000 sets `+0x58` to 1 for the
  selected list child only when `+0x5e == 2`. The reading in the header (frame index plus
  frame count of the background bitmap, so a two-frame bitmap gives selected/unselected art)
  fits both sites, but the phase-2 pass read `+0x5e` as a second widget-type field and `+0x58`
  as a plain selected flag. Whoever writes 0x49a8c0 / 0x49bac0 should settle it.
- `widget_instance + 0x16`, `+0x4a`, `+0x54`, `+0x56`, `+0x5a`. `+0x54` is zeroed for list
  types by 0x499540 and 0x499d00 reads an event table at `+0x54/+0x58` per the phase-2 note,
  which conflicts with the frame-index reading above. Not resolved.
- `widget_instance + 0x15`. Tested by `main_menu_on_shown` @0x498ab0 and never written in any
  function read here.
- `first_person_weapon_interface + 0x30 .. 0x87` (0x58 bytes) and `+0x108c .. 0x1d8b`
  (0xd00 bytes). 0x493150 and 0x493740 write into the first region (aim sway, idle timers)
  and 0x493ea0/0x4924b0 gather 0x34 byte node records through the second, but no single
  function states either extent. The 0x800 sizes of `animation_control` and `previous_pose`
  are the gap between the two block bases in 0x4930b0, not a declared size; the real copy
  length is derived at run time from the weapon node/track count, so the true blocks are
  smaller and the remainder belongs to the 0xd00 scratch.
- `first_person_weapon_interface + 0x0e, 0x10, 0x16, 0x18, 0x1a, 0x1c, 0x20, 0x22, 0x28` —
  all written with sentinels by 0x493c60 but never read in any function read here.
- `first_person_weapon_interface + 0x1e98 / 0x1e9c` — reset to -1 by 0x494390 and 0x493c60
  and not read anywhere in the packs examined.
- `hud_weapon_interface_state` (0x7c). Only the all-minus-one reset is pinned. The six
  functions that write it (0x4b1740, 0x4b1970, 0x4b1e20, 0x4b1ff0, 0x4b2ac0, 0x4b2cf0) were
  not disassembled in this pass, so the struct is left as `int32_t entries[0x1f]`.
- `hud_unit_meter_state + 0x0c`, `+0x20`, `+0x22`, `+0x24`, `+0x26`, `+0x58`. The three
  leading floats and the three -1 dwords at 0x14/0x18/0x1c come from the reset pattern; the
  twelve -1 dwords at 0x28 are assumed to be the looping-sound datums that 0x4afd30 starts and
  stops, which was not verified field by field.
- `hud_messaging_globals + 0x460..0x464`, `+0x466..0x477`, `+0x47a`, `+0x47e..0x485`.
- `hud_player_messaging_state + 0x430..0x433` and `+0x45a..0x45d`.
- `motion_sensor_frame + 0x78` and `motion_sensor_globals + 0x568`.
- `terminal_console + 0x82..0x83`.
- `virtual_keyboard_globals + 0x01..0x03`, `+0x0a`, `+0x12`, `+0x69..0x6b`.
- `player_control_settings` (0x217). Only the stride is pinned, by
  `player_profile_refresh_settings_cache` @0x496060 (`&DAT_00710328 + profile * 0x217`). The
  field layout was not recovered; the source record is the 0x2004 byte saved profile, which
  belongs to the profile module (see below).
- `motion_sensor_plot_blip` @0x4b37a0 dereferences its EAX as `float *` for two components,
  but the blip record it is plotting stores int8 x/y. Either Ghidra mistyped an integer load
  or the caller hands it a converted pair on the stack. Not resolved; the header keeps the
  int8 pair because the reset and update routines both write bytes.
- `widget_memory_pool_initialize` @0x4979b0 ends with `*puVar9 = puVar9`, storing a pointer to
  `heap::blocks[0]` into `heap::blocks[0]`. That contradicts the `memory.h` comment that a
  NULL slot means free. Either Ghidra mis-rendered the instruction or this heap is used
  slightly differently; nothing in `interface.h` depends on it.
- The ordering of `ui_root_widget` (0x00718f94) against `ui_widget_history` (0x00718f98).
  `widget_open` indexes `(&DAT_00718f94)[controller]` and `0x49c3e0` indexes
  `(&DAT_00718f98)[controller]`, which would overlap if both were 4-entry arrays.
  `widget_memory_pool_initialize` sets 0x00718fa4 to -1, which is where a 3-entry history
  array ending at 0x00718fa3 would stop, so the header declares `ui_root_widget[1]` and
  `ui_widget_history[3]`. The bound on `ui_root_widget` is weak: `widget_close` only ever
  checks index 0, but `widget_open` can be reached with a controller index up to 3.
- `ui_pending_error_alternate` at 0x00718fb2 versus `ui_pending_errors[4]` at 0x00718fb6.
  `display_error` only ever indexes the 0x00718fb6 base, but
  `widget_memory_pool_initialize` resets 0x718fa4, 0x718fac, 0x718fb2 and 0x718fb6 to -1, and
  `interface_handle_quit_request` @0x499170 arms "either the single-player or the split-screen
  slot". The split into one standalone record plus an array is the most consistent reading of
  those two facts but is not directly proven.
- `ui_player_help_string` (0x00718fac). Phase 2 calls it a triple indexed by player slot
  (0x49e090, 0x4a4e20), but only element 0 is reset, so the stride was not confirmed.

---

## Misattributed or out-of-module functions

- **0x4a3ce0 `hud_text_message_queue_init` has zero callers** in the module (pack reports
  `callers=0`). It is still the definitive source for the `growable_array` header at
  0x006b37e8, so its types are kept.
- **0x495190 and 0x4951f0** are named `first_person_weapons_update` and
  `first_person_weapon_render_update` by the stale Ghidra symbols, but neither touches any
  weapon, unit or interface record. Both are map-list entry helpers sharing the tail of
  0x4950c0 (path lowercase, extension strip, `cache_file_exists`, count increment). No
  first-person types were taken from them.
- **0x49d850 `render_widget_recursive`** performs no rendering and no tree walk; it is a
  `stricmp` scan over the `map_list_entry` path pointers at 0x00712dcc. Treated as a map-list
  function.
- **0x49a2e0 `render_ui_cursor`** draws nothing; it is the child-recursion tail of
  `widget_instance_handle_input_event`. No cursor types taken from it.
- **0x493e50 `chimera__first_person_node_base_address`** is an attach/detach toggle for the
  first-person model (it calls 0x450cb0 or 0x450d50 + 0x455c80 and compares against
  `first_person_weapon_interface + 0x00`), not a node base-address getter.
- **0x4bab50 / 0x4bab80 / 0x4badc0 (`video_*`)** carry a cea-pdb hint saying module `game`,
  and **0x4b3450 `blip_type_get`** a hint saying module `ai`. Their code only touches
  interface-owned globals (the resolution table at 0x006b6690, the motion sensor at
  0x00719438), so their types are defined here; the hints are noted but not followed.
- **The chat listbox is not an interface structure.** 0x4ab240, 0x4ab300, 0x4ab400 and
  0x4ab4b0 drive an `"oListbox"` control through the function-pointer table at
  0x00721ea4..0x00721ee8, which belongs to the embedded GUI library, not to this module.
  Only the two things the module owns are declared: the expiry timestamps at 0x006b3a20 and
  the count at 0x00719424 (capped at 8, 8000 ms per message).
- **The saved player profile record (0x2004 bytes) is not declared here.** It is written and
  read by 0x0053a950 / 0x0053c4e0 / 0x0053d360 in the profile module; the interface functions
  (0x495370, 0x495970, 0x495be0, 0x495d40, 0x496060) only copy fields out of it.
  `player_control_settings` is the interface-side cache and is the only part declared.
- **The game variant and saved-map working copies** (0x00714e80 / 0x00716e7c, 0x98 and 0x1ffc
  bytes) are `game_variant` and the campaign save record from `types/game.h`; they are listed
  as `void *` globals here rather than redeclared.
- `0x49c369 display_scenario_help_fail` is a compiler-duplicated tail fragment of
  `ui_check_for_pause_game` @0x49c1a0, not a separate routine, and owns no types.
