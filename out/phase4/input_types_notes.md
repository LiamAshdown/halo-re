# input module: type recovery notes

Target: retail Halo PC `halo.exe` 1.0.10, module `input` (80 Ghidra functions, 0x48b3e0..0x492340).
Header: `types/input.h`. Smoke test: `out/phase4/input_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/input_smoke.c
```

It passes with `-Wall -Wextra` and with `-m32`. It checks every struct size, about 90 field offsets
and array ends against the absolute addresses the decompile uses (for example
`0x00710328 + offsetof(input_abstraction_globals, last_used_bindings) == 0x007127d4`), and the
reused foreign types at the sizes this module relies on. A translation unit that includes every
header in `types/` plus `input.h` also compiles, so no name collides.

**Dependencies and parse order.** The header embeds, and does not redefine,
`local_player_input_state` (game.h), `player_control_settings`, `controls_gamepad_record`,
`input_guid`, `ui_key_event`, `ui_input_event` (interface.h) and `control_binding_descriptor`
(saved_games.h). It also uses the `k_control_*` counts from saved_games.h. So the smoke file
includes `tags.h memory.h math.h game.h networking.h interface.h saved_games.h` first.
`interface.h` and `saved_games.h` sort **after** `input.h`. If Ghidra's CParser reads `types/*.h`
in plain alphabetical order, these are forward references. interface.h already does the same
thing with `s_network_address` (networking.h). The only alternative was to redefine
`control_binding_descriptor` and `player_control_settings`, which the rules forbid. If the
CParser run fails on input.h, parse `interface.h` and `saved_games.h` before it.

## Binary evidence the layouts hang on

| struct | size | where the binary states it |
|---|---|---|
| `input_abstraction_globals` | 0x25f0 | `input_state_initialize` 0x48b3e0: `mov ecx,0x97c ; mov edi,0x710328 ; rep stosd`. The block ends at 0x00712918, which is a separate global (the menu exit deadline). |
| `joystick_raw_state` | 0xe0 | `input_system_initialize` 0x491a80 builds the 0x50 DIOBJECTDATAFORMAT entries at 0x00879f60 (axes at offsets 0..0x7c, type 0x80ffff03; POVs at 0x80..0xbc, 0x80ffff10; buttons at 0xc0..0xdf, 0x80ffff0c). The DIDATAFORMAT at 0x0068e51c is `{0x18,0x10,1,0xe0,0x50,0x00879f60}`. The poll passes 0xe0 to GetDeviceState, and the enum callback passes 0x0068e51c to SetDataFormat (0x491dd3). |
| `di_mouse_state2` | 0x14 | GetDeviceState(0x14) in the poll. c_dfDIMouse2 at 0x0064e1e4 is `{0x18,0x10,2,0x14,0x0b}`. |
| `joystick_state` | 0xa0 | Every copy is `rep movsd` of 0x28 dwords (0x48b6b0, 0x490760, 0x490aa0). |
| `input_device` | 0x240 | Every walk strides 0x90 dwords. The fields come from the EnumDevices callback 0x491d70 (disassembled; not a Ghidra function). |
| `mouse_state` | 0x1c | The poll error path and 0x490aa0 zero exactly the seven dwords 0x006b180c..0x006b1827. The neutral copy fills 0x006b1828..0x006b1843, and 0x006b1844 is the device count. |
| `input_event_queue` | 0x10c | `input_queue_initialize` 0x492250 zeroes 0x43 dwords at 0x00712cc0. |
| `mouse_acceleration_point` | 0x10 | 0x48cb60 copies 0x1c dwords (7 entries). The rebuild loop steps 0x10 from 0x0068e420 to 0x0068e490, and the search runs to 0x0068e4fc. |

## Per struct: which functions established which fields

### input_abstraction_globals (0x00710328)
- `settings[4]` +0x0000, stride 0x85c. This is `player_control_settings` from interface.h. The 0x85c
  stride comes from `input_joystick_set_axis_scale_x/_y` 0x48c930 / 0x48c9a0 (`in_CX * 0x85c`). The
  binding tables inside it were pinned by `input_apply_control_binding` 0x48b7b0,
  `input_clear_control_binding` 0x48b9b0, `input_find_binding_for_event` 0x48bae0 and
  `input_print_bound_controls` 0x48bea0: keyboard +0x008 (loop to 0x0071040a, 0x6d entries),
  mouse buttons +0x0e2 (to 0x0071041a, 8), mouse axes +0x0f2 as [3][2] (to 0x00710426), gamepad
  buttons +0x0fe `[pad*0x20+i]`, accept/back button index +0x1fe `[pad*2]` / `[pad*2+1]` (0x48bae0
  for actions 8 and 9, 0x48cca0 and 0x48ec50 for the state reads), gamepad axes +0x20e
  `[(pad*0x20+axis)*2 + (dir!=1)]`, and POVs +0x40e `[(pad*0x10+pov)*8 + octant]`. The float block
  comes from 0x48ca10 / 0x48cca0: +0x810 forward, +0x814 strafe, +0x818 look x and +0x81c look y
  digital rates; +0x820 / +0x824 mouse throttle divisors; +0x828 / +0x82c the mouse look
  sensitivities passed to 0x48cb60; +0x830 / +0x834 gamepad scales; +0x838 / +0x848 per-pad rates.
  +0x858 / +0x859 are the look inversion bytes (0x48ea21 .. 0x48ea46). The header documents these
  as a comment map only. They should become field names in interface.h (see below).
- `states[4]` +0x2170: `local_player_input_state` from game.h. input_update_tick 0x48b4b0,
  bind_capture_reset 0x48b5f0 and 0x48ec50 clear 0x00712498..0x007124bc. The 0x28 stride and the
  axis offsets come from 0x48ca10 (ECX +0x14/+0x18/+0x1c/+0x20). `idle` compare 0x48fce0 reads the
  same four floats plus the 0x13 button bytes. Only [0] is written by this module.
- `time_base` +0x2210: 0x48b3e0 and 0x48b470 (dword store).
- `unknown_2214` +0x2214, `unknown_2219` +0x2219: byte stores of 1 in 0x48b3e0 (0x48b3fc, 0x48b459).
  No reads anywhere in the image.
- `idle` +0x2218: set to 1 by 0x48b4b0 / 0x48b5f0 / interface 0x4c7956. It is cleared at 0x48ea1c
  when 0x48fce0 reports a change, and read at 0x4c7c53.
- `mode_flags` +0x221a: the dispatch in 0x48b4b0 (disassembly 0x48b519..0x48b554). Bit 2 comes from
  0x48b650, bit 3 from 0x48b6b0, bit 1 is tested in the poll (0x49088a), and bit 0 is set by 0x48b3e0.
  Interface code also toggles bits 2 and 3.
- `scan_baselines[4]` +0x221c: 0x48b6b0 (copy per slot, zero 0xa0 dwords on exit) and 0x48f8c0
  (axis compare at +0x20, 0x00712564).
- `scan_result` +0x249c: 0x48f8c0 writes device_type / device_index as one dword (0x007127c4),
  input_kind / input_index as one dword (0x007127c8), and direction (0x007127cc). 0x48b3e0 zeroes it.
- `system_key_states[3]` +0x24a8: 0x48b4b0 loop over 0x0068e40c. `input_key_block_timer_set`
  0x490bf0 clears the matching entry. Interface reads [0] at 0x4c65dc and [1] at 0x49c23f.
- `last_used_bindings[0x1b]` +0x24ac: 0x48bae0, 0x48bde0, 0x48be50, 0x490050, 0x48cca0 (12-byte
  record, `index * 0xc`, fields at +0/+2/+4/+6/+8).

### joystick_state / joystick_raw_state
- `input_joystick_state_process` 0x491fd0: buttons from raw +0xc0 (bit 7) into +0x00; POVs from raw
  +0x80 into +0x60, as octants; axes from raw +i*4 into +0x20 as int16. The counts come from the
  input_device in EBX (+0x234 / +0x238 / +0x23c).
- The readers are 0x48cca0 (button bytes by index, the accept/back lookup), 0x48f8c0 (axes +0x20,
  POVs +0x60 compared to -1) and 0x48ec50 (0x006b2d18 / 0x006b2d58 in the neutral copy).
  `input_system_initialize` sets the neutral POVs (0x006b2d58, 0x10 dwords) to -1.
- The axis range and dead zone come from the EnumObjects callback 0x491c50 (not a Ghidra function):
  DIPROP_RANGE is -0x1000..0x1000 and DIPROP_DEADZONE is 1000.

### mouse_state / di_mouse_state2
- `input_mouse_state_process` 0x491bc0: x copied, y negated, wheel `-(z / 0x006b1808)`, button frames
  +0x0c and went-down flags +0x14, both through the swap map 0x0068e534. Raw buttons are at +0x0c.
- The readers are 0x48cca0 / 0x48f8c0 (x, y, wheel), `input_get_mouse_button_state` 0x490e00
  (+0x0c), and 0x48ec50 (0x006b1818 left frames, 0x006b181a, 0x006b1820 left went-down for the
  double click).

### input_device
- The 0x491d70 callback stores: +0x21c instance number (from 0x491d30), +0x220 guidInstance, +0x20c
  guidProduct, and the name through 0x557990 (0x20a byte limit), with a (N) suffix appended when
  +0x21c is nonzero. It also stores +0x230 = -1, and +0x234 / +0x238 / +0x23c from DIDEVCAPS clamped
  to 0x20 / 0x20 / 0x10.
- The readers are `input_device_get_axis/button/pov_count` 0x491610 / 0x491630 / 0x491650,
  `input_device_find_index_by_guid` 0x4916e0 (+0x20c 16-byte compare, +0x21c pre-check), and
  `input_device_list_print` 0x491750 (copies 0x88 dwords, then StringFromGUID2 of +0x20c).
- This settles interface.h's UNSURE `controls_gamepad_record::device_key`: it is guidProduct plus
  the instance number among devices with that product.

### key_block_timer (0x006b1600)
- 0x4918a0 / 0x490aa0 seed {-1, -1} (`puVar1[-2]`, `*puVar1`, stride 8, ending at 0x006b1624).
  0x490bf0 fills an entry, 0x490ca0 expires entries, and 0x490b50 tests `key` at 0x006b1604.
  The +0x06 word is never touched.

### menu_repeat_state (0x0068e4fc)
- 0x48ec50 cases 0x4d/0x62, 0x4e/0x5c, 0x4f/0x5e, 0x50/0x60, with the 0x15d ms compare. The fourth
  entry's key is held in a register and stored back to 0x0068e518 after the loop.

### mouse_acceleration_point (0x0068e41c defaults, 0x0068e48c working)
- 0x48cb60 (disassembled): `[esi]` +4 and `[esi-4]` +0 of each default give the rebuilt working +0
  (`mov [esi+0x6c],eax`). The evaluation reads +0 of i-1 and i, +8 of i-1 and i, and +0xc of i.

### input_event_queue (0x00712cc0)
- 0x492250 (clear, times, enable), 0x4922b0 (pop: scans 0x00712d04 - 8k, copies two dwords, clears
  the kind word), 0x492340 (push: tests +1, writes the queue index into record+2, then memmove(dst
  0x00712ccc + q*0x40, src 0x00712cd4 + q*0x40, 0x38), stores slot 0, and sets +4).
  The records are interface.h `ui_input_event`. The menu generator builds them with kind 3 and
  codes 8 / 9 / 0xa.. for buttons, and kind 4 for mouse buttons (0x48f544..0x48f761).
- Note: the push shifts slots 1..7 down and then overwrites slot 0, so slot 7 is never replaced by
  a push while the pop takes the highest nonzero slot. This is observed behaviour, recorded as-is.

### DirectInput records
- `di_object_data_format` / `di_data_format`: see the table above.
- `di_property_dword`: 0x4918a0 (DIPROP_BUFFERSIZE, header 0x14/0x10) and 0x490620 (DIPROP_GRANULARITY
  3 of object 8 DIMOFS_Z by offset, result into 0x006b1808).
- `di_property_range`: 0x491c50 (0x18/0x10, by id).
- `di_device_object_data`: the poll's GetDeviceData with cbObjectData 0x14 (`local_f4` offset,
  `cStack_f0` data byte).
- `di_device_caps`: 0x491d70 (size 0x2c, dwAxes +0xc, dwButtons +0x10, dwPOVs +0x14).
- `di_device_instance` and `di_device_object_instance` are the callback arguments of 0x491d70 and
  0x491c50 (+4, +0x14, +0x28; +0x18, +0x20). These are standard dinput.h 8 layouts, included so the
  two callbacks can be typed once Ghidra has functions for them.

## Unresolved offsets / UNSURE

- `input_abstraction_globals::unknown_2214` (0x0071253c) and `unknown_2219` (0x00712541): written
  to 1 once and never read.
- `input_event_queue::push_disabled` (0x00712cc1): only read, and zero from the initializer.
- `key_block_timer::pad_06`: never accessed.
- 0x00712922..0x00712927 (between `mouse_axis_frames` and `joystick_axis_frames`), 0x00712c30 and
  0x00712c38..0x00712cbf: no reference anywhere in the image. They are not claimed.
- 0x006b15fa..0x006b15fb and 0x006b17fe..0x006b17ff: no references (padding before the pointer
  and after the key ring).
- `mouse_acceleration_point::rate` units (radians per count is a guess from the magnitudes
  0.00075..0.0015) and `boost`.
- `player_control_settings +0x859` (look inversion while driving): 0x48fd60 returns true for a
  driver seat (seat flag bit 2) when the unit definition short at +0x2f4 is 3 or 5. The meaning of
  that short is not pinned.
- `input_suppressed` (0x006b15f9) is only ever written 0 (0x490781), so every "neutral" path is dead
  in this build. The name comes from what the readers do.
- The keyboard create (0x4918a0) and reset (0x490aa0) zero only 0x10 dwords (16 records) of the
  0x40-record key ring. The count reset makes the rest unreachable.

## Misattributed / not real functions

- **0x48b520 `effect_new_on_object_marker`**: not a function. It is the tail of `input_update_tick`
  0x48b4b0 (0x48b520 is the `jne` of `cmp al,1` at 0x48b51e; the jump table at 0x48b5d4 belongs to
  the tick). No types.
- **0x48d270 `hs_return`**: not a function. It is the interior of the game action update 0x48cca0
  (its labels such as LAB_0048d71e and LAB_0048dc7c appear in 0x48cca0's own decompile, and the
  code runs on to 0x48eb8x). It has no script behaviour. Its globals are all covered by the
  structs above.
- **0x4921a0 `chimera__main_menu_music`**: main-menu title music / bink stop. It belongs to the
  interface / main-menu code, not input. Its types are skipped.
- 0x490280 (`input_profile_copy_bindings_by_device`) copies ranges between two
  `saved_player_profile` records (saved_games.h). Its only caller is 0x4901b0, so it stays here,
  but its type is the profile module's.
- 0x48fd60 (`input_should_invert_look`) reads player / object / unit-definition data owned by
  game.h / objects.h / tags.h. No new types.
- 0x490090 (`test_input_device_defaults_find`), 0x490110 and 0x4901b0 read `InputDeviceDefaults`
  (tags.h): device_type +0, device_id pointer +0x10, profile pointer +0x24. The profile block is a
  `saved_player_profile` image (0x7ff dwords copied; the name is compared at profile+2).

## Corrections to earlier names / summaries (for the rename pass)

- 0x48b650: the keyboard vtable call +0x28 is GetDeviceData(0x14, NULL, &INFINITE, 0), a buffer
  flush, not SetProperty.
- 0x48b6b0: this is the bind-scan begin/end (snapshot joystick states into `scan_baselines` and set
  mode bit 3, or zero them and clear the bit). It is not "axis binding profile load". Likewise
  0x006b2a68 / 0x006b2cf8 are joystick states, not axis-binding profiles.
- 0x48ec50: the menu navigation event generator (arrow keys / numpad, enter / escape, accept /
  back buttons, sticks past 0x7ff, mouse click and double click into the UI event queue). It is
  not a movement double-tap detector.
- 0x490bf0 / 0x490ca0: key block timers (a key reads as up until the deadline), not key repeat.
- 0x490b50: `input_get_key_state`, with virtual either-side modifiers 0x6e..0x71.
- 0x491590: parses a "povN direction" name (prefix `pov` at 0x0065b920, number table 0x0065b988,
  direction via 0x491480). It is not a device-by-name lookup. 0x4912e0 parses "buttonN" (prefix
  0x0065b8f0), not "sbuttonN".
- 0x491480 scans the compass table at 0x0065b938 (10-byte stride), not 0x0065b988.
- 0x006b15f9 is not a "use default profile" flag (see `input_suppressed`).

## Findings for other headers (not changed here)

- **types/game.h** lists `global 0x006b2ce8: int32_t team_slot_table[4]` (scanned by 0x473730).
  It is the input slot-to-device map `joystick_slot_devices[4]`, seeded to -1 by
  `input_system_initialize` and read by every input slot lookup. 0x473730 (`cmp [eax*4+0x6b2ce8],-1`)
  is only one more reader, not its owner. It should be relabelled there.
- **types/interface.h** `ui_edit_key_code`: 0x4f / 0x50 are the left / right arrow keys in the
  DIK table 0x0065bd58 (DIK 0xcb / 0xcd), not home / end. Home is 0x52 and end is 0x55. 0x1d
  backspace and 0x54 delete are right.
- **types/interface.h** `player_control_settings`: the unknown ranges can take the names in the
  input.h field map (keyboard / mouse / gamepad binding tables, +0x810 rates, +0x828 mouse
  sensitivities, +0x830 gamepad scales, +0x858 look_inverted).
- **types/saved_games.h** `saved_player_profile`: correspondingly `unknown_12f` = look_inverted,
  `unknown_131` = look_inverted_driving, `unknown_93c[6]` = the six rates / divisors, `unknown_954/955`
  = mouse look sensitivity x / y, `unknown_960[2]` = gamepad axis scale x / y.
- **out/phase4/interface_smoke.c** no longer compiles. It still names `server_browser_entry` (now
  `controls_gamepad_record`) and asserts `sizeof(player_control_settings) == 0x217` (the header now
  says 0x85c, which this smoke confirms).
