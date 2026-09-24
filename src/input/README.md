# `input` — Blam input abstraction and the DirectInput 8 layer

Retail Halo PC `halo.exe` 1.0.10, `0x48b3e0 .. 0x492340` (80 Ghidra functions, 77 real ones),
plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86. Every file here is one function, rewritten from
its Ghidra decompilation against `types/input.h`, with the decompile kept verbatim at the bottom
inside `#if 0 ... #endif`. Register-passed arguments are LTCG-invented per function; each file
header quotes the call-site / prologue evidence (`// blam-cc:` lines).

Gate: `python tools/build_check.py input` -> **77 ok, 0 failed** (full repo: 3489 ok, 0 failed).

## What the module contains

| Family | Range | What it is |
|---|---|---|
| state init / tick | `0x48b3e0`-`0x48b64f` | zero `input_globals`, millisecond time base, the per-frame tick that dispatches on `mode_flags` |
| bind / unbind / rebind capture | `0x48b650`-`0x48be9f`, `0x48f8c0`, `0x48fe60`-`0x49008f` | `bind` / `unbind` console commands, binding-string parser, rebind scan, last-used-binding cache |
| binding display text | `0x48bea0`-`0x48c89f`, `0x490e30`-`0x49166f` | `controls_*_names` tag lookups, name <-> index parsers, the bound-controls console dump |
| game action update | `0x48c8a0`-`0x48ec4f` | helpers (clamp, turn rate, axis scale, digital axis step, mouse acceleration curve) and the per-frame update that turns every bound key / button / axis / POV into `local_player_input_state` 0 |
| menu navigation events | `0x48ec50`-`0x48f8bf` | arrows / numpad / enter / escape / pad / mouse click and double click into the UI event queue |
| look inversion, idle test | `0x48fce0`-`0x48fe5f` | |
| device default profiles | `0x490090`-`0x49051f` | `InputDeviceDefaults` (`devc`) tag lookup and applying a named full profile |
| DirectInput 8 | `0x490520`-`0x492197` | create / acquire / unacquire / release, the per-frame poll, raw -> engine conversion of keyboard, mouse and up to 8 controllers, key block timers, WndProc key messages, error log |
| UI event queue | `0x492210`-`0x4923c7` | 4 queues x 8 `ui_input_event` records |

Frame flow: `input_update_tick` -> (`mode_flags == 1`, menu exit deadline passed)
`input_game_action_update` -> `input_accumulate_axis_value` / `input_mouse_acceleration_evaluate`
/ `input_last_used_binding_set` (the last one inlined in the binary) -> `input_accumulator_is_idle`
-> `input_should_invert_look`. Menu mode instead runs `input_menu_generate_events` ->
`input_queue_push_event`. The poll (`input_directinput_poll_devices`) is driven from the shell
loop and fills `key_frames`, `live_mouse_state` and `joystick_states`.

Global names: C forbids a global named like its typedef, so the singletons are `input_globals`
(0x00710328), `live_mouse_state` (0x006b180c) and `event_queue` (0x00712cc0). The binding tables
inside `input_globals.settings[0]` are declared as their own externs (`keyboard_bindings`
0x00710330, `mouse_button_bindings` 0x0071040a, `mouse_axis_bindings` 0x0071041a,
`gamepad_button_bindings` 0x00710426, `gamepad_action_buttons` 0x00710526,
`gamepad_axis_bindings` 0x00710536, `gamepad_pov_bindings` 0x00710736) because
`types/interface.h` keeps those bytes of `player_control_settings` opaque and this module may not
redefine it. `types/input.h` lists every global under these exact names. The fallback text at
0x00671fac is the wide string itself (`uint16_t missing_string_text[]`), not a pointer.

## Struct layouts

All in `types/input.h` unless noted; offsets are bytes from the struct base, `#pragma pack(1)`.

### `input_abstraction_globals` — size `0x25f0`, at `0x00710328`

| Off | Type | Field |
|---|---|---|
| `0x0000` | `player_control_settings[4]` | `settings` (interface.h, stride `0x85c`; map below) |
| `0x2170` | `local_player_input_state[4]` | `states` (game.h, stride `0x28`; only `[0]` written here) |
| `0x2210` | `uint32_t` | `time_base` (ms at init) |
| `0x2214` | `uint8_t` | `unknown_2214` (written 1, never read) |
| `0x2218` | `uint8_t` | `idle` (1 each tick, cleared when the game update changed state) |
| `0x2219` | `uint8_t` | `unknown_2219` (written 1, never read) |
| `0x221a` | `uint8_t` | `mode_flags` (`input_mode_flags`: 1 game, 2 menu, 4 keyboard capture, 8 bind scan) |
| `0x221c` | `joystick_state[4]` | `scan_baselines` |
| `0x249c` | `control_binding_descriptor` | `scan_result` |
| `0x24a8` | `uint8_t[3]` | `system_key_states` (grave, escape, print screen) |
| `0x24ac` | `control_binding_descriptor[0x1b]` | `last_used_bindings` (per `input_action`) |

### `player_control_settings` as this module reads it — size `0x85c` (type owned by interface.h)

| Off | Type | Meaning |
|---|---|---|
| `0x000` / `0x004` | `float` | look yaw / pitch rate, copied to `look_{yaw,pitch}_rate_setting` every game update |
| `0x008` | `int16_t[0x6d]` | keyboard bindings |
| `0x0e2` | `int16_t[8]` | mouse button bindings |
| `0x0f2` | `int16_t[3][2]` | mouse axis bindings, `[0]` positive delta (direction 1), `[1]` negative (2) |
| `0x0fe` | `int16_t[4][0x20]` | gamepad button bindings |
| `0x1fe` | `int16_t[4][2]` | per-pad accept / back BUTTON index, -1 none |
| `0x20e` | `int16_t[4][0x20][2]` | gamepad axis bindings |
| `0x40e` | `int16_t[4][0x10][8]` | gamepad POV bindings per octant |
| `0x810`..`0x81c` | `float[4]` | digital step: forward, strafe, look x, look y |
| `0x820` / `0x824` | `float` | mouse delta divisor for forward / strafe bindings |
| `0x828` / `0x82c` | `float` | mouse look x / y sensitivity (acceleration curve argument) |
| `0x830` / `0x834` | `float` | gamepad throttle axis scale x / y (divisor) |
| `0x838` / `0x848` | `float[4]` | per-pad yaw / pitch rate written while a pad look axis is active |
| `0x858` / `0x859` | `uint8_t` | look inverted / look inverted while piloting (see `input_should_invert_look`) |

### `local_player_input_state` as written by `input_game_action_update` — size `0x28` (game.h)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int8_t[0x13]` | `buttons`: hold count per digital action, compared unsigned; 1, 3 and 11 forced to 0 unless exactly 1 |
| `0x14` | `float` | `throttle_x` (forward +) |
| `0x18` | `float` | `throttle_y` (left +) |
| `0x1c` | `float` | `look_x` (left +) |
| `0x20` | `float` | `look_y` (up +, negated when inverted) |
| `0x24` | `uint8_t` | `look_is_analog`: 1 from a pad axis, 0 from the mouse |

### `control_binding_descriptor` — size `0x0c` (saved_games.h)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `device_type` (1 keyboard, 2 mouse, 3 gamepad, 0 none) |
| `0x02` | `int16_t` | `device_index` (pad slot) |
| `0x04` | `int16_t` | `input_kind` (0 button, 1 axis, 2 POV) |
| `0x06` | `int16_t` | `input_index` |
| `0x08` | `int32_t` | `direction` (axis 1 / 2, POV octant; the POV diagonal path records octant +/- 1 unwrapped) |

### `joystick_state` — size `0xa0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t[0x20]` | `button_frames` (saturating hold count) |
| `0x20` | `int16_t[0x20]` | `axes` (-0x1000..0x1000) |
| `0x60` | `int32_t[0x10]` | `povs` (octant 0..7, -1 centered; every negative raw angle maps to -1) |

### `joystick_raw_state` — size `0xe0` (the custom DirectInput data format)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t[0x20]` | `axes` |
| `0x80` | `uint32_t[0x10]` | `povs` (hundredths of a degree, low word `0xffff` centered) |
| `0xc0` | `uint8_t[0x20]` | `buttons` (bit 7) |

### `di_mouse_state2` — size `0x14` (DIMOUSESTATE2)

| Off | Type | Field |
|---|---|---|
| `0x00` / `0x04` / `0x08` | `int32_t` | `x`, `y` (down +), `z` (wheel) |
| `0x0c` | `uint8_t[8]` | `buttons` (bit 7) |

### `mouse_state` — size `0x1c` (`live_mouse_state` 0x006b180c, `mouse_neutral_state` 0x006b1828)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `x` |
| `0x04` | `int32_t` | `y` (negated raw, up +) |
| `0x08` | `int32_t` | `wheel` = -(z / granularity) |
| `0x0c` | `uint8_t[8]` | `button_frames` through the swap map (the physical right button lands in slot 2) |
| `0x14` | `uint8_t[8]` | `button_pressed`: 1 on the poll the button was RELEASED (corrected in this pass; the old comment said went down) |

### `input_device` — size `0x240`, 8 at `0x006b1868`

| Off | Type | Field |
|---|---|---|
| `0x000` | `controls_gamepad_record` | `record` (name, `device_key` = product guid + instance number) |
| `0x220` | `input_guid` | `instance_guid` |
| `0x230` | `int32_t` | `slot` (pad slot or -1) |
| `0x234` | `int32_t` | `axis_count` (<= 0x20) |
| `0x238` | `int32_t` | `button_count` (<= 0x20) |
| `0x23c` | `int32_t` | `pov_count` (<= 0x10) |

### `key_block_timer` — size `0x08`, 4 at `0x006b1600`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `deadline` (ms, `0xffffffff` free) |
| `0x04` | `int16_t` | `key` (-1 free) |
| `0x06` | `int16_t` | `pad_06` (never touched) |

### `menu_repeat_state` — size `0x08`, 4 at `0x0068e4fc` (up, down, left, right)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `last_event_time` (ms, 0 released) |
| `0x04` | `int32_t` | `key`: running virtual key id of the owning input, -1 none |

### `mouse_acceleration_point` — size `0x10`, defaults `0x0068e41c`, working `0x0068e48c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `magnitude` (working: slow - (slow - fast) * acceleration, truncated) |
| `0x04` | `int32_t` | `magnitude_slow` |
| `0x08` | `float` | `rate` |
| `0x0c` | `float` | `boost` (times the sensitivity argument) |

### `input_event_queue` — size `0x10c`, at `0x00712cc0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t` | `enabled` |
| `0x01` | `uint8_t` | `push_disabled` (never written) |
| `0x04` | `uint32_t` | `last_event_time` |
| `0x08` | `uint32_t` | `start_time` |
| `0x0c` | `ui_input_event[4][8]` | `events` (interface.h, 8 bytes each: kind, controller_index, code, pressed, axis_y) |

The DirectInput SDK records (`di_object_data_format`, `di_data_format`, `di_property_*`,
`di_device_object_data`, `di_device_caps`, `di_device_instance`, `di_device_object_instance`) are
the standard dinput.h layouts. The COM method pointers (`directinput8create_proc`,
`idirectinput8_*_proc`, `idirectinputdevice8_*_proc`) now live in `types/input.h` with their
vtable byte offsets; no file keeps a local typedef.

## Misattributed Ghidra entries (not written)

| Ghidra entry | Name in functions.txt | What it really is |
|---|---|---|
| `0x48b520` | `effect_new_on_object_marker` (OpenSauce CE entry) | the `jne` inside `input_update_tick`'s mode dispatch; folded into `input_update_tick.c` |
| `0x48d270` | `hs_return` (OpenSauce CE entry) | the middle of `input_game_action_update` (the `test ah,5` of the mouse-button forward case); covered by `input_game_action_update.c` |
| `0x4921a0` | `chimera__main_menu_music` | main-menu title music / bink stop; belongs to the interface module |

Both OpenSauce names are CE-only addresses that drifted onto retail code; they should be dropped
from the symbol set rather than renamed.

## Known gaps

- `player_control_settings` (interface.h) and `saved_player_profile` (saved_games.h) still carry
  `unknown_810` / `unknown_830` / `unknown_858` / `unknown_859` and `unknown_93c` / `unknown_954` /
  `unknown_960` / `unknown_12f` / `unknown_131`; the rewrites cast them. The names are in the field
  map above and should be applied in those headers by their owners.
- Other modules still extern input globals and functions under stale names or shapes:
  interface (`directinput_unknown_buffer_1/2` = `key_frames` / `key_release_pending`,
  `queued_key_event_*`, `cursor_delta_*` over `mouse_device` / `live_mouse_state`,
  `input_gamepads`, `input_gamepad_count`), game / saved_games / interface (`team_slot_table`,
  `input_slot_to_device`, `local_player_slot_hint_table` = `joystick_slot_devices`), and `FUN_`
  prototypes for `input_get_last_used_binding` (returns `uint8_t` found),
  `input_get_binding_display_name`, `input_last_used_binding_copy`, `input_queue_pop_event`
  (returns `uint8_t`), `input_device_get_*_count`, `input_keyboard_set_capture_mode`,
  `input_queue_sample_time_update`. Not edited here (outside the module).
- `string_format_wide_va_bounded` 0x557910 takes a hidden character bound in EDX (0xe / 0x18 at
  the call sites); the shared prototype does not expose it.
- `saved_game_create_default_profile` 0x539ab0 is called with ECX = name and one pushed 0 that
  its rewrite does not read.
- `input_device_find_index_by_guid` returns the index in AX with the EAX upper half 0, or
  `0xffffffff`; the input rewrite returns `uint32_t`, the saved_games callers read `int16_t`.
- `input_game_action_update`: the gamepad axis direction local is uninitialized stack in the
  binary; only the POV octant-0 guard byte depends on it (see the file header).
- `input_joystick_pov_name_to_index` hands the direction parser prefix + 1 + strlen(digits), so
  a "povN direction" string never parses; reproduced, caller impact unverified.
- The mouse-state NULL read in `input_game_action_update` and `input_scan_any_bound_input` when
  no mouse device exists is reproduced, not guarded.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite (file header), `U`
counts `UNSURE` markers.

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x48b3e0` | `input_state_initialize` | 133 | 0.6 | 0.85 | 0 |
| `0x48b470` | `input_time_base_resync` | 64 | 0.55 | 0.9 | 0 |
| `0x48b4b0` | `input_update_tick` | 109 | 0.55 | 0.75 | 0 |
| `0x48b5f0` | `input_bind_capture_reset` | 83 | 0.55 | 0.85 | 1 |
| `0x48b650` | `input_keyboard_set_capture_mode` | 89 | 0.55 | 0.7 | 0 |
| `0x48b6b0` | `input_bind_scan_set_active` | 145 | 0.5 | 0.7 | 0 |
| `0x48b750` | `hs_bind_control` | 90 | 0.5 | 0.65 | 0 |
| `0x48b7b0` | `input_apply_control_binding` | 285 | 0.5 | 0.75 | 0 |
| `0x48b8d0` | `hs_unbind_control` | 209 | 0.55 | 0.65 | 2 |
| `0x48b9b0` | `input_clear_control_binding` | 291 | 0.5 | 0.75 | 0 |
| `0x48bae0` | `input_refresh_last_used_binding` | 757 | 0.4 | 0.6 | 0 |
| `0x48bde0` | `input_get_last_used_binding` | 100 | 0.45 | 0.75 | 0 |
| `0x48be50` | `input_last_used_binding_copy` | 69 | 0.45 | 0.75 | 0 |
| `0x48bea0` | `input_print_bound_controls` | 2352 | 0.55 | 0.6 | 0 |
| `0x48c7f0` | `input_get_binding_display_name` | 174 | 0.45 | 0.75 | 0 |
| `0x48c8a0` | `input_clamp_unit_float` | 53 | 0.5 | 0.9 | 0 |
| `0x48c8e0` | `input_sensitivity_to_turn_rate` | 71 | 0.5 | 0.85 | 0 |
| `0x48c930` | `input_joystick_set_axis_scale_x` | 109 | 0.4 | 0.75 | 0 |
| `0x48c9a0` | `input_joystick_set_axis_scale_y` | 109 | 0.4 | 0.75 | 0 |
| `0x48ca10` | `input_accumulate_axis_value` | 302 | 0.4 | 0.75 | 0 |
| `0x48cb60` | `input_mouse_acceleration_evaluate` | 306 | 0.45 | 0.6 | 0 |
| `0x48cca0` | `input_game_action_update` | 8112 (0x48cca0..0x48ec4f; Ghidra: 1481) | 0.6 | 0.6 | 2 |
| `0x48ec50` | `input_menu_generate_events` | 2938 | 0.5 | 0.45 | 1 |
| `0x48f8c0` | `input_scan_any_bound_input` | 1043 | 0.55 | 0.55 | 2 |
| `0x48fce0` | `input_accumulator_is_idle` | 116 | 0.4 | 0.55 | 0 |
| `0x48fd60` | `input_should_invert_look` | 242 | 0.5 | 0.5 | 1 |
| `0x48fe60` | `input_action_name_to_index` | 55 | 0.5 | 0.75 | 0 |
| `0x48fea0` | `input_parse_device_binding_string` | 431 | 0.55 | 0.65 | 0 |
| `0x490050` | `input_last_used_binding_set` | 57 | 0.5 | 0.7 | 0 |
| `0x490090` | `test_input_device_defaults_find` | 124 | 0.9 | 0.7 | 0 |
| `0x490110` | `input_device_default_profile_tag_find` | 149 | 0.5 | 0.8 | 0 |
| `0x4901b0` | `input_apply_named_device_default_profile` | 200 | 0.5 | 0.65 | 0 |
| `0x490280` | `input_profile_copy_bindings_by_device` | 660 | 0.4 | 0.55 | 1 |
| `0x490520` | `input_directinput_initialize` | 90 | 0.85 | 0.75 | 0 |
| `0x490580` | `input_directinput_release_devices` | 160 | 0.8 | 0.6 | 0 |
| `0x490620` | `input_directinput_acquire_devices` | 191 | 0.85 | 0.6 | 0 |
| `0x4906e0` | `input_directinput_unacquire_devices` | 128 | 0.85 | 0.7 | 0 |
| `0x490760` | `input_directinput_poll_devices` | 823 | 0.5 | 0.5 | 0 |
| `0x490aa0` | `input_reset_state_and_axis_configs` | 164 | 0.5 | 0.7 | 0 |
| `0x490b50` | `input_get_key_state` | 138 | 0.5 | 0.6 | 0 |
| `0x490bf0` | `input_key_block_timer_set` | 161 | 0.5 | 0.6 | 0 |
| `0x490ca0` | `input_key_block_timers_expire` | 108 | 0.5 | 0.7 | 0 |
| `0x490d10` | `input_record_windows_key_message` | 228 | 0.55 | 0.65 | 0 |
| `0x490e00` | `input_get_mouse_button_state` | 34 | 0.65 | 0.7 | 0 |
| `0x490e30` | `input_get_keyboard_key_name` | 109 | 0.65 | 0.7 | 0 |
| `0x490ea0` | `input_keyboard_key_name_to_index` | 127 | 0.6 | 0.7 | 0 |
| `0x490f20` | `input_get_mouse_button_name` | 109 | 0.65 | 0.7 | 0 |
| `0x490f90` | `input_mouse_button_name_to_index` | 127 | 0.6 | 0.7 | 0 |
| `0x491010` | `input_get_mouse_axis_name` | 163 | 0.55 | 0.65 | 0 |
| `0x4910c0` | `input_mouse_axis_name_to_index` | 172 | 0.55 | 0.6 | 1 |
| `0x491180` | `input_get_axis_direction_name` | 109 | 0.65 | 0.7 | 0 |
| `0x4911f0` | `input_axis_direction_name_to_index` | 127 | 0.6 | 0.7 | 0 |
| `0x491270` | `chimera__button_text` | 108 | 0.45 | 0.6 | 1 |
| `0x4912e0` | `input_joystick_button_name_to_index` | 82 | 0.5 | 0.75 | 0 |
| `0x491340` | `chimera__axis_text` | 155 | 0.55 | 0.6 | 1 |
| `0x4913e0` | `input_joystick_axis_name_to_index` | 155 | 0.5 | 0.6 | 1 |
| `0x491480` | `input_joystick_pov_direction_name_to_index` | 56 | 0.6 | 0.75 | 0 |
| `0x4914c0` | `chimera__pov_text` | 194 | 0.55 | 0.6 | 1 |
| `0x491590` | `input_joystick_pov_name_to_index` | 127 | 0.5 | 0.6 | 2 |
| `0x491610` | `input_device_get_axis_count` | 30 | 0.65 | 0.75 | 0 |
| `0x491630` | `input_device_get_button_count` | 30 | 0.65 | 0.75 | 0 |
| `0x491650` | `input_device_get_pov_count` | 30 | 0.65 | 0.75 | 0 |
| `0x491670` | `input_guid_parse_ansi` | 104 | 0.5 | 0.65 | 0 |
| `0x4916e0` | `input_device_find_index_by_guid` | 100 | 0.65 | 0.75 | 0 |
| `0x491750` | `input_device_list_print` | 335 | 0.65 | 0.55 | 2 |
| `0x4918a0` | `input_keyboard_device_create` | 286 | 0.75 | 0.75 | 1 |
| `0x4919c0` | `input_mouse_device_create` | 188 | 0.75 | 0.8 | 1 |
| `0x491a80` | `input_system_initialize` | 297 | 0.6 | 0.6 | 1 |
| `0x491bc0` | `input_mouse_state_process` | 134 | 0.5 | 0.65 | 0 |
| `0x491d30` | `input_device_count_by_guid` | 57 | 0.6 | 0.75 | 0 |
| `0x491f80` | `input_device_release` | 68 | 0.55 | 0.6 | 0 |
| `0x491fd0` | `input_joystick_state_process` | 365 | 0.5 | 0.6 | 0 |
| `0x492150` | `input_error_log_once` | 66 | 0.55 | 0.85 | 0 |
| `0x492210` | `input_queue_sample_time_update` | 64 | 0.55 | 0.85 | 0 |
| `0x492250` | `input_queue_initialize` | 94 | 0.6 | 0.85 | 0 |
| `0x4922b0` | `input_queue_pop_event` | 133 | 0.65 | 0.6 | 0 |
| `0x492340` | `input_queue_push_event` | 135 | 0.65 | 0.6 | 0 |

Checked line by line against `python tools/pack.py` and `objdump -d` in the phase-4 consistency
pass (fixed where marked): `input_game_action_update` (written in this pass),
`input_refresh_last_used_binding`, `input_menu_generate_events` (fixed),
`input_print_bound_controls` (fixed), `input_directinput_poll_devices` (DIERR names),
`input_scan_any_bound_input`, `input_profile_copy_bindings_by_device`, `hs_unbind_control`,
`input_mouse_state_process`, `input_mouse_acceleration_evaluate`, `input_should_invert_look`
(fixed), `input_apply_named_device_default_profile` (fixed), `input_accumulator_is_idle` (fixed),
`input_joystick_state_process`, `input_apply_control_binding`, `input_clear_control_binding`,
`input_get_last_used_binding` (fixed), `input_queue_pop_event` (fixed), `chimera__pov_text`, and
the five name parsers (fixed).
