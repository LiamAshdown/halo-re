# `camera`: Blam camera script, director and observer

Retail Halo PC `halo.exe` 1.0.10, `0x444c00 .. 0x449170` (55 Ghidra functions, of which 3 are
mis-split tails), plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86. Every file in this directory is
one function, rewritten against `types/camera.h`. The original Ghidra decompile is kept at the
bottom of each file inside `#if 0 ... #endif`. For the 7 real functions Ghidra has no entry for,
the objdump listing is kept there instead.

Gate: `python tools/build_check.py camera` gives **59 ok, 0 failed** (52 Ghidra functions plus 7
functions with no Ghidra entry). `out/phase4/camera_smoke.c` still passes with `-Wall` under
both 64-bit gcc and `-m32`.

LTCG gave every function its own register convention. Each file states its convention in its
header (`// blam-cc: ...`), checked against the callee prologue and at least one call site in
`objdump -d -M intel`. The house conventions do not apply here.

## What the module contains

Three Blam source files share this range:

| Layer | Range | What it is | Globals |
|---|---|---|---|
| camera script | `0x444b30`..`0x4450e0` | the hs camera: `camera_set` to a cutscene camera point, camera animations, first person and dead camera on a scripted object, and the scripted pov procedure | `.data 0x006869d0` `camera_script` |
| director | `0x4450e0`..`0x447370` | per local player mode selection. Sets `director.pov_proc` (first person / third person / dead / scripted / flying / editor), the per-mode data union, the debug look input and axis smoothing, and the flying / orbiting / editor cameras | `.bss 0x006ac558..0x006ac657`, `0x006f17f8..0x006f186c`, `.data 0x00686a10..0x00686ad0` |
| observer | `0x447680`..`0x449170` | the final camera. It eases every parameter of the `observer_command` a pov procedure produced towards its target with a per-channel quintic spline, clamps it, pushes it out of geometry and publishes `observer_camera` for the renderer and sound | `.bss 0x006ac658..0x006ac8f7`, `.data 0x00686ae0..0x00686af7` |

Per frame:

```
camera_update(dt)                                   0x445640
  director_build_camera_input(0, &input)            0x445f90   ESI = &input
    camera_input_axes_update                        0x446170   debug look axes
  director_choose_gameplay_camera / director_set_flying_camera / first person reset
  directors[0].pov_proc(&data, &input, &command)    one of:
    camera_first_person_compute_pov  0x446d60   camera_third_person_compute_pov 0x447370
    camera_track_compute_pov (dead)  0x445380   camera_debug_compute_pov (hs)   0x444d50
    flying_camera_compute_pov        0x4464f0   editor_camera_compute_pov       0x446e90
  directors[0].command = command; observers[0].command = &directors[0].command
observer_update(dt, add_bob)                        0x447880
  observer_set_command -> observer_advance -> observer_commit
    observer_compute_remaining_offset, _compute_spline_coefficients,
    _evaluate_spline_acceleration / _velocity / _value_and_orthonormalize
    observer_avoid_collision -> observer_collision_test_ray -> collision_test_movement_segment
```

The spline: every tick `0x447be0` refits a quintic P with P(T) = 0 at the time left T and
P(0) = the remaining offset (target minus current, `0x448710`). `0x448210` adds P(T - dt) onto the
running state, so the `+=` in that file is correct. When a channel runs out of time and the
command has the valid bit, `0x448210` copies the target instead.

## Struct layouts

All of these are in `types/camera.h` (`#pragma pack(push,1)`). Offsets are byte offsets from the
struct base. Only the fields that code touches are listed. Unreferenced padding is listed as
unknown.

### `camera_script_globals` (0x40, `.data 0x006869d0`, extern `camera_script`)

| Off | Type | Field |
|---|---|---|
| 0x00 | uint8_t | camera_control (hs `camera_control`, mirrored through `0x0087bc0c`) |
| 0x01 | uint8_t | changed |
| 0x02 | int16_t | mode (`camera_script_mode`, -1 / 0 point / 1 animation / 2 first person / 3 dead) |
| 0x04 | int16_t | camera_point_index |
| 0x06 | int16_t | unknown_06 |
| 0x08 | float | time_remaining (seconds; ticks / 30) |
| 0x0c | Point3D | position |
| 0x18 | Vector3D | forward |
| 0x24 | Vector3D | up |
| 0x30 | float | field_of_view |
| 0x34 | datum_index | object |
| 0x38 | datum_index | animation_tag |
| 0x3c | int16_t | animation_index |
| 0x3e | int16_t | unknown_3e |

### `camera_input_axis_definition` (0x1c, `.data 0x00686a28` x4, extern `camera_input_axes`)

| Off | Type | Field |
|---|---|---|
| 0x00 | int16_t | decrease_key_bit |
| 0x02 | int16_t | increase_key_bit |
| 0x04 | int16_t | reset_key_bit |
| 0x06 | int16_t | unknown_06 |
| 0x08 | float | acceleration |
| 0x0c | float | reset_value |
| 0x10 | float | minimum_value |
| 0x14 | float | maximum_value |
| 0x18 | uint8_t | scale_by_zoom (only row 0 is ever read: absolute `0x00686a40`) |
| 0x19 | uint8_t[3] | unknown_19 |

### `camera_input_axis_state` (0x0c, `director + 0xc8` x4)

| Off | Type | Field |
|---|---|---|
| 0x00 | float | value |
| 0x04 | float | velocity |
| 0x08 | float | delta |

### `camera_input` (0x24, on the stack of `camera_update`)

| Off | Type | Field |
|---|---|---|
| 0x00 | int16_t | local_player_index |
| 0x02 | uint8_t | has_look_input |
| 0x03 | uint8_t | unknown_03 |
| 0x04 | float | dt |
| 0x08 | float | yaw_delta (mouse x * -0.0031415927) |
| 0x0c | float | pitch_delta (mouse y * 0.0031415927) |
| 0x10 | float | roll_delta (axis 1) |
| 0x14 | float | move_forward (axis 2) |
| 0x18 | float | move_left (axis 3) |
| 0x1c | float | move_up (axis 0) |
| 0x20 | float | zoom_delta (mouse wheel) |

### `observer_parameters` (0x38 = 14 floats)

| Off | Type | Field |
|---|---|---|
| 0x00 | Point3D | position |
| 0x0c | Vector3D | focus_offset (x along horizontal forward, y perpendicular, z world up) |
| 0x18 | float | distance |
| 0x1c | float | field_of_view |
| 0x20 | Vector3D | forward |
| 0x2c | Vector3D | up |

### `observer_command` (0x68, `director + 0x58`, `observer + 0x08`)

| Off | Type | Field |
|---|---|---|
| 0x00 | uint32_t | flags (0x01 valid, 0x08 snap, 0x10 no collision, 0x20 frozen) |
| 0x04 | observer_parameters | parameters |
| 0x3c | Vector3D | velocity |
| 0x48 | float | timer |
| 0x4c | uint8_t[5] | interpolation_flags (0x01 own time, 0x02 exact) |
| 0x51 | uint8_t[3] | unknown_51 |
| 0x54 | float[5] | channel_times |

### `observer_parameter_derivatives` (0x2c = 11 floats)

| Off | Type | Field |
|---|---|---|
| 0x00 | Vector3D | position |
| 0x0c | Vector3D | focus_offset |
| 0x18 | float | distance |
| 0x1c | float | field_of_view |
| 0x20 | Vector3D | rotation (axis * angle) |

### `observer_camera` (0x3c, `observer + 0x74`; `observer_get_camera` returns it)

| Off | Type | Field |
|---|---|---|
| 0x00 | Point3D | position |
| 0x0c | int32_t | leaf_index |
| 0x10 | int16_t | cluster_index |
| 0x12 | int16_t | unknown_12 (stale upper half of a dword store) |
| 0x14 | Vector3D | velocity |
| 0x20 | Vector3D | forward |
| 0x2c | Vector3D | up |
| 0x38 | float | field_of_view |

### `observer` (0x29c, `.bss 0x006ac65c`, extern `observers[1]`)

| Off | Type | Field |
|---|---|---|
| 0x000 | uint32_t | header_signature (0x72616421) |
| 0x004 | observer_command * | command |
| 0x008 | observer_command | current_command (its channel_times are the running countdowns) |
| 0x070 | uint8_t | updated |
| 0x071 | uint8_t | has_command |
| 0x072 | int16_t | unknown_072 |
| 0x074 | observer_camera | camera |
| 0x0b0 | observer_parameters | parameters |
| 0x0e8 | observer_parameter_derivatives | velocity |
| 0x114 | float[3] | unknown_114 |
| 0x120 | observer_parameter_derivatives | acceleration |
| 0x14c | float[3] | unknown_14c |
| 0x158 | observer_parameter_derivatives | coefficient_t5 |
| 0x184 | observer_parameter_derivatives | coefficient_t4 |
| 0x1b0 | observer_parameter_derivatives | coefficient_t3 |
| 0x1dc | observer_parameter_derivatives | coefficient_t2 |
| 0x208 | observer_parameter_derivatives | coefficient_t1 |
| 0x234 | observer_parameter_derivatives | coefficient_t0 |
| 0x260 | observer_parameter_derivatives | remaining_offset |
| 0x28c | float[3] | unknown_28c |
| 0x298 | uint32_t | trailer_signature |

### `first_person_camera_data` (0x04, member of the director union)

| Off | Type | Field |
|---|---|---|
| 0x00 | float | field_of_view (last value from 0x471f90) |

### `third_person_camera_data` (0x1c)

| Off | Type | Field |
|---|---|---|
| 0x00 | uint8_t | initialized |
| 0x01 | uint8_t | unknown_01 |
| 0x02 | uint8_t | crouch_or_jump |
| 0x03 | uint8_t | unknown_03 |
| 0x04 | int16_t | unknown_04 |
| 0x06 | int16_t | unknown_06 |
| 0x08 | datum_index | unit |
| 0x0c | int16_t | seat_index |
| 0x0e | int16_t | unknown_0e |
| 0x10 | float | yaw_offset |
| 0x14 | float | pitch_offset |
| 0x18 | float | distance_scale (0..5) |

### `dead_camera_data` (0x30)

| Off | Type | Field |
|---|---|---|
| 0x00 | Point3D | focus |
| 0x0c | float | yaw |
| 0x10 | float | pitch |
| 0x14 | float | distance |
| 0x18 | float | field_of_view |
| 0x1c | float | transition_time (exactly 3.0 on a new target) |
| 0x20 | datum_index | local_player |
| 0x24 | datum_index | target_player |
| 0x28 | datum_index | target_unit (seeded from player +0x38 previous_unit) |
| 0x2c | float | retarget_time |

### `editor_camera_data` (0x1c, also the flying camera)

| Off | Type | Field |
|---|---|---|
| 0x00 | Point3D | position |
| 0x0c | float | yaw |
| 0x10 | float | pitch (+-1.5676548) |
| 0x14 | float | roll |
| 0x18 | float | field_of_view |

### `orbiting_camera_data` (0x1c)

| Off | Type | Field |
|---|---|---|
| 0x00 | float | unknown_00 |
| 0x04 | float | distance (floored at 0.6) |
| 0x08 | float | unknown_08 |
| 0x0c | float | yaw |
| 0x10 | float | pitch (+-1.2566371) |
| 0x14 | float | unknown_14 |
| 0x18 | float | unknown_18 |

### `director_camera_data` (union, 0x40, `director + 0x0c`)

| Off | Type | Field |
|---|---|---|
| 0x00 | first_person_camera_data | first_person |
| 0x00 | third_person_camera_data | third_person |
| 0x00 | dead_camera_data | dead |
| 0x00 | editor_camera_data | editor |
| 0x00 | orbiting_camera_data | orbiting |
| 0x00 | uint8_t[0x40] | raw |

### `director` (0xf8, `.bss 0x006ac560`, extern `directors[1]`)

| Off | Type | Field |
|---|---|---|
| 0x00 | int16_t | unknown_00 (write-only, 2) |
| 0x02 | int16_t | unknown_02 |
| 0x04 | float | transition_time |
| 0x08 | director_pov_proc | pov_proc |
| 0x0c | director_camera_data | data |
| 0x4c | int32_t | unknown_4c |
| 0x50 | uint8_t | unknown_50 |
| 0x51 | uint8_t | suppress_look_update |
| 0x52 | uint8_t | look_input_consumed |
| 0x53 | uint8_t | unknown_53 |
| 0x54 | int16_t | seat_camera_state |
| 0x56 | int16_t | camera_type |
| 0x58 | observer_command | command |
| 0xc0 | uint8_t | unknown_c0 (write-only) |
| 0xc1 | uint8_t[3] | unknown_c1 |
| 0xc4 | float | look_scale (0.01..50) |
| 0xc8 | camera_input_axis_state[4] | axes |

### `director_globals` (0x08, `.bss 0x006ac558`, extern `camera_director_globals`)

| Off | Type | Field |
|---|---|---|
| 0x00 | float | dt |
| 0x04 | int16_t | mode (`director_camera_mode`) |
| 0x06 | uint8_t | mode_changed |
| 0x07 | uint8_t | unknown_07 |

### `flying_camera_home` (0x14, `.bss 0x006f1800`, extern `flying_camera_home_location`)

| Off | Type | Field |
|---|---|---|
| 0x00 | Point3D | position |
| 0x0c | float | yaw |
| 0x10 | float | pitch |

### `unit_camera_properties` (0x58, a view of `UnitSeat + 0x84` or `Unit + 0x1a8`)

| Off | Type | Field |
|---|---|---|
| 0x00 | TagString | camera_marker_name |
| 0x20 | TagString | camera_submerged_marker_name |
| 0x40 | float | pitch_auto_level |
| 0x44 | float[2] | pitch_range |
| 0x4c | TagReflexive | camera_tracks |

Types other modules own and this module uses, without redefining them: `player`,
`player_globals`, `player_control_globals` / `local_player_control`, `camera_basis_out`,
`game_time_globals` (game.h), `object`, `object_header`, `object_marker`, `bsp_leaf_reference`
(objects.h), `unit_data` at object +0x1f4 (units.h), `collision_result` (projectiles.h),
`mouse_state` (input.h), `render_camera` (rasterizer.h), `data_array`, `data_iterator` (memory.h),
and the tag structs (tags.h).

## Misattributed / mis-split entries

| Address | Ghidra / applied name | What it is |
|---|---|---|
| 0x445230 | `physical_memory_initialize` | `pop esi; ret`, the second epilogue of `dead_camera_new` (0x4450e0..0x445239) |
| 0x445350 | `physical_memory_map_predict_resources` | the loop tail of `camera_dead_find_next_teammate` (0x4452c0..0x445370) |
| 0x4462a0 | `director_load_camera` | the second half of the `camera_input_axes_update` loop (0x446170..0x44634a) |

`symbols/agent_phase4_camera.txt` renames all three to `<owner>_...tail` / `_epilogue` so that
the OpenSauce CE names stop being applied. They have no files of their own.

Ghidra names that described the wrong thing and were renamed: `camera_shake_initialize`
(0x4450e0) to `dead_camera_new`, `camera_debug_update_transform` (0x4465d0) to
`flying_camera_update`, `camera_shake_tick` (0x447880) to `observer_update`, and
`camera_get_globals_for_player` (0x4479a0) to `observer_get_camera`. Three names were kept
because they agree with `symbols/functions.txt`, although better names exist:
`camera_debug_start` 0x444c00 (really hs `camera_set`), `camera_debug_compute_pov` 0x444d50 (the
scripted camera pov), `camera_track_compute_pov` 0x445380 (the dead camera pov).

## Functions

The rewrite confidence is the one in each file header after the phase 4 gate review. Files marked
**new** have no Ghidra function.

| Address | Function | Size | Rewrite conf. |
|---|---|---|---|
| 0x444c00 | camera_debug_start (hs camera_set) | 327 | 0.8 |
| 0x444d50 | camera_debug_compute_pov (scripted pov) | 885 | 0.75 |
| 0x4450e0 | dead_camera_new | 344 | 0.85 |
| 0x445240 | camera_dead_player_has_teammate | 121 | 0.85 |
| 0x4452c0 | camera_dead_find_next_teammate | 179 | 0.85 |
| 0x445380 | camera_track_compute_pov (dead pov) | 476 | 0.85 |
| 0x445560 | director_game_state_loaded **new** | 22 | 0.95 |
| 0x445580 | camera_initialize | 104 | 0.85 |
| 0x4455f0 | camera_is_local_player_default_first_person | 74 | 0.85 |
| 0x445640 | camera_update | 554 | 0.8 |
| 0x445880 | camera_debug_save_to_file | 192 | 0.85 |
| 0x445940 | camera_debug_load_from_file | 382 | 0.75 |
| 0x445ac0 | camera_get_type_for_player | 94 | 0.9 |
| 0x445b20 | camera_get_seat_camera_state | 219 | 0.85 |
| 0x445c00 | director_update_seat_camera | 184 | 0.8 |
| 0x445cc0 | camera_control | 243 | 0.85 |
| 0x445dc0 | director_choose_gameplay_camera | 376 | 0.8 |
| 0x445f40 | director_set_flying_camera | 67 | 0.85 |
| 0x445f90 | director_build_camera_input | 477 | 0.8 |
| 0x446170 | camera_input_axes_update (+ tail 0x4462a0) | 474 | 0.8 |
| 0x446350 | flying_camera_initialize | 280 | 0.8 |
| 0x446470 | flying_camera_attach_to_object | 116 | 0.85 |
| 0x4464f0 | flying_camera_compute_pov **new** | 215 | 0.8 |
| 0x4465d0 | flying_camera_update | 669 | 0.75 |
| 0x446870 | orbiting_camera_update | 303 | 0.8 |
| 0x4469a0 | flying_camera_enter_flying **new** (unreachable in this build) | 112 | 0.8 |
| 0x446a10 | flying_camera_enter_orbiting **new** | 123 | 0.85 |
| 0x446a90 | first_person_camera_deterministic | 217 | 0.85 |
| 0x446b70 | first_person_camera_for_unit_and_vector | 439 | 0.85 |
| 0x446d30 | first_person_camera_command_for_unit | 44 | 0.85 |
| 0x446d60 | camera_first_person_compute_pov | 199 | 0.8 |
| 0x446e30 | editor_camera_set_position_and_direction | 91 | 0.85 |
| 0x446e90 | editor_camera_compute_pov **new** | 353 | 0.8 |
| 0x447000 | scalar_catmull_rom_interpolate | 121 | 0.95 |
| 0x447080 | vector3d_catmull_rom_interpolate | 135 | 0.9 |
| 0x447110 | unit_get_camera_properties | 127 | 0.85 |
| 0x447190 | first_person_camera_track_offset | 247 | 0.8 |
| 0x447290 | first_person_camera_apply_weapon_offset | 210 | 0.85 |
| 0x447370 | camera_third_person_compute_pov | 770 | 0.8 |
| 0x447680 | real_approximately_equal | 55 | 0.85 |
| 0x4476c0 | real_is_valid | 24 | 0.9 |
| 0x4476e0 | vector3d_is_unit_length | 89 | 0.85 |
| 0x447740 | observer_new | 291 | 0.8 |
| 0x447870 | observer_initialize **new** | 10 | 0.95 |
| 0x447880 | observer_update | 275 | 0.8 |
| 0x4479a0 | observer_get_camera | 23 | 0.9 |
| 0x4479c0 | vector3d_compute_up_from_forward | 149 | 0.85 |
| 0x447a60 | observer_update_location **new** | 80 | 0.9 |
| 0x447ab0 | observer_set_command | 155 | 0.85 |
| 0x447b50 | observer_advance | 124 | 0.85 |
| 0x447be0 | observer_compute_spline_coefficients | 604 | 0.55 |
| 0x447e40 | observer_evaluate_spline_acceleration | 452 | 0.55 |
| 0x448010 | observer_evaluate_spline_velocity | 496 | 0.55 |
| 0x448210 | observer_evaluate_spline_value_and_orthonormalize | 1258 | 0.8 |
| 0x448710 | observer_compute_remaining_offset | 364 | 0.85 |
| 0x448880 | vector3d_rotate_basis_by_axis_angle | 116 | 0.7 |
| 0x448900 | observer_commit | 1074 | 0.8 |
| 0x448d40 | observer_avoid_collision | 1053 | 0.7 |
| 0x449170 | observer_collision_test_ray | 98 | 0.85 |

## Semantic fixes made in the phase 4 gate review

Each fix is described in the header of its file.

* `camera_update`: the three director calls had lost their register arguments, so the stack
  `camera_input` handed to every pov procedure was never initialized. They now pass ESI = &input,
  DI / AX = 0 and the mode_changed flag.
* Four files read unit fields through `(unit_data *)object`. `unit_data` starts at object +0x1f4,
  so every field was read 0x1f4 bytes too early. The four files are
  `camera_get_seat_camera_state`, `first_person_camera_command_for_unit`,
  `first_person_camera_deterministic` and `first_person_camera_for_unit_and_vector`.
* `dead_camera_new` targets player +0x38 `previous_unit`, not +0x34 `unit`.
* `camera_get_type_for_player` tests `director.transition_time`, not the first person fov.
* `camera_dead_find_next_teammate` stops at the first later candidate (it used to return the
  last one). The team is compared as int32 in both teammate helpers, and has_teammate returns a
  byte.
* `camera_debug_compute_pov`: a relative object that `object_try_and_get` rejects now skips the
  whole command (it used to emit one at the default position). The animation frame is clamped
  as int16.
* `observer_collision_test_ray` / `observer_avoid_collision` pass a full 0x50 byte
  `collision_result`. The 0x18 byte scratch was overrun, and the TYPES-GAP is closed.
* `observer_avoid_collision` / `observer_commit` / `observer_update_location`: the leaf index is
  masked with 0x7fffffff before indexing the leaves, and `bsp3d_node_find_leaf` gets its real
  register arguments. `observer_commit` passes the location and point that `FUN_0053ee00` (the
  water depth) reads in EAX / EDI.
* `camera_get_seat_camera_state` returns a 16-bit value (both callers test AX).
* `first_person_camera_apply_weapon_offset`: 0x628630 is the CRT `_CIasin`. The extra pointer
  argument was the early push of the next call.
* `camera_debug_start` calls `camera_update(0.0f)` (it was declared with an int16 parameter),
  takes ticks as int16 and calls the observer functions with their register index.

## Known gaps

* Not written: none of the 55 entries. The 3 mis-split tails are folded into their owners.
* `observer_compute_spline_coefficients`, `_evaluate_spline_acceleration` and
  `_evaluate_spline_velocity` (0.55) were not re-derived line by line in the review. Their callers
  and register conventions were checked.
* `FUN_0053ed60` (a leaf-reference predicate that selects collision flags 0x40a1 over 0x40e1) and
  `FUN_004d49b0` (the animation frame to matrix sampler) are outside the module and unnamed.
* `data_iterator` in types/memory.h is 0x0c bytes. The binary also stores the iterator self-check
  word `data ^ 0x69746572` at +0x0c (0x44527e, 0x445303), which `data_iterator_next` never reads.
  It is omitted here.
* The key letters behind `camera_input_key_bits`, the name `flying_camera_follow_script`, and the
  meaning of key 0x1d / mouse button_frames[1] in `director_build_camera_input` are unresolved.
* Unreferenced offsets are listed in `types/camera.h` ("Unresolved offsets") and in
  `out/phase4/camera_types_notes.md`.

## Cross-module disagreements found (not edited, owners should check)

* `object_get_node_local_transform` 0x4f6080: src/objects declares EAX/ECX/EDX register arguments
  and an int32 result. Both camera call sites push four stack arguments (`add esp,0x10`) and test
  AX, and the prologue reads `[esp+0x4]`.
* 0x471f90 is called `game_engine_get_max_look_pitch` in src/game, but the first person camera
  stores its result as the command field of view.
* `game_time_globals +0x1c` is `leftover_time` in game.h and `seconds_per_tick` in hs.h.
  observer_update passes it to `unit_predict_movement_delta` as the time fraction.
* `unit_predict_movement_delta` 0x55cca0 is declared `uint32_t` in src/units, but
  `observer_update` tests only AL.
