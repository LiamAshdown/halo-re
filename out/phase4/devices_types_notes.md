# devices module: type recovery notes

Header: `types/devices.h`. Smoke test: `out/phase4/devices_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/devices_smoke.c
```

It passes, and it carries negative-array assertions for every struct size plus ~45
`offsetof` assertions written as *object* offsets (`0x1f4 + offsetof(device_data, type_flags)
== 0x214` and so on), so a field reordering fails the build instead of passing silently. The
other eight phase-4 smoke files still compile unchanged.

Like `objects.h`, `items.h`, `units.h` and `projectiles.h`, `devices.h` uses `real_point3d`,
so the smoke file includes `math.h` (and `objects.h`) as well as `tags.h` and `memory.h`.
`tags.h + memory.h + devices.h` alone does not compile, exactly as `tags.h + memory.h +
objects.h` alone does not; this is the established convention, not a defect in this header.

## The binary evidence everything hangs on

### 1. object_type_definition rows -> every struct size

The 12-pointer table at **0x0069bfdc** (PE image base 0x400000, `.data` VA 0x676000 -> file
offset 0x276000, so file offset == VA - 0x400000). Each row is
`{char *name, uint32 group, int16 object_size, int16 placement_off, int16 palette_off,
int16 placement_size, int32 network_delta_message_type, ...}`:

| row | name | group | object_size | placement | palette | stride | net |
|---|---|---|---|---|---|---|---|
| 0x0069ba68 | scenery | `scen` | 0x1f8 | 0x210 | 0x21c | 0x48 | -1 |
| 0x0069bcc0 | **machine** | `mach` | **0x228** | 0x294 | 0x2a0 | 0x40 | -1 |
| 0x0069bd88 | **control** | `ctrl` | **0x21c** | 0x2ac | 0x2b8 | 0x40 | -1 |
| 0x0069be50 | **light_fixture** | `lifi` | **0x22c** | 0x2c4 | 0x2d0 | 0x58 | -1 |

The common object record is 0x1f4, so:

* `device_data` is **0x24** bytes at object+0x1f4 (the largest offset any device function
  touches in the shared part is the flags word at 0x214, which ends at 0x218, and 0x218 is
  exactly where the control record ends minus its 4-byte tail);
* `device_machine_data` is 0x24 + 0x10 = **0x34** (object size 0x228);
* `device_control_data` is 0x24 + 0x04 = **0x28** (object size 0x21c);
* `device_light_fixture_data` is 0x24 + 0x14 = **0x38** (object size 0x22c).

No size in the header is inferred from the highest offset seen. The three placement offsets
in those rows are `Scenario.machines` 0x294, `Scenario.controls` 0x2ac and
`Scenario.light_fixtures` 0x2c4, and the strides match `sizeof(ScenarioMachine) ==
sizeof(ScenarioControl) == 0x40` and `sizeof(ScenarioLightFixture) == 0x58` in `types/tags.h`.
All three devices have `network_delta_message_type == -1`, which is why there is no device
network delta record in this header.

### 2. The Device tag in types/tags.h is confirmed field for field

`Device` is 0x290 and sits on top of `Object` (0x17c). Every one of the offsets the module
reads out of the tag lands on a named field, with no residue:

```
0x198  device_a_in .. device_d_in     FUN_0044ba10 walks 4 int16 selectors from here
0x21c  automatic_activation_radius    device_machine_update, guarded by >= 0.0001
0x274  inverse_power_acceleration_time          device_update_change_values
0x278  inverse_power_transition_time            (also the case-2 divisor in FUN_0044ba10)
0x27c  inverse_depowered_position_acceleration_time
0x280  inverse_depowered_position_transition_time
0x284  inverse_position_acceleration_time
0x288  inverse_position_transition_time         (also the case-4 divisor in FUN_0044ba10)
0x28c  delay_time_ticks                         (also the case-6 divisor in FUN_0044ba10)
0x290  DeviceMachine.machine_type / DeviceControl.type   the two switches in this module
0x292  DeviceMachine.machine_flags / DeviceControl.triggers_when
0x294  DeviceMachine.door_open_time / DeviceControl.call_value
0x2ea  DeviceMachine.elevator_node
0x320  DeviceMachine.door_open_time_ticks       read as *(int *)(tag + 800)
```

The pair at 0x290 is the strongest single confirmation in the module:
`device_change_power_state` switches on `*(int16 *)(tag + 0x290)` with cases 0..3 and reads a
float at `tag + 0x294` in case 3, which is exactly `DeviceType {toggle_switch, on_button,
off_button, call_button}` plus `DeviceControl.call_value`; `device_machine_update` switches on
the same offset against 0 and 2, which is `MachineType {door, platform, gear}`.
`FUN_0044adf0` gates on `*(int16 *)(tag + 0x292) == 0`, i.e.
`DeviceControl.triggers_when == touched_by_player`.

### 3. UnitFlags bit 14 identifies the automatic-activation scan

`device_machine_update` rejects a nearby object when
`*(uint32 *)(tag_data + 0x17c) & 0x4000` is set. `Object` is 0x17c, so that is the first
dword of the derived tag; for a unit it is `UnitFlags`, whose bit 14 is spelled
`cannot_open_doors_automatically` in `types/tags.h`. That is what pins the whole block as the
automatic door-opening proximity scan rather than a generic collision sweep.

### 4. device_new reads a placement slice, not a whole placement record

`device_new` is handed a `short *` (EDI) and reads `[0]`, `[1]` and the uint32 at `+4`.
`ScenarioMachine`, `ScenarioControl` and `ScenarioLightFixture` all carry
`power_group` at 0x28, `position_group` at 0x2a and `device_flags` at 0x2c, so the pointer is
`&placement->power_group` and the slice is an 8-byte record shared by all three placement
types. `device_placement_data` in the header is that slice; the smoke file asserts it lines up
against all three scenario structs.

The flag arithmetic confirms the meanings:
`(*(byte *)(edi + 4) & 8)` -> device flags bit 0 and `& 0x10` -> bit 1, which are
`ScenarioDeviceFlags` bits 3 and 4, `position_reversed` and `not_usable_from_any_side`;
and the position group is created with
`flags = (short)((placement_flags & 4 | 0x10) >> 2)`, whose low bit is
`ScenarioDeviceFlags` bit 2, `can_change_only_once`.

## Structs defined, and which function established which field

### `device_group` (0x08, one element of the data_array at 0x0087abf0)

| offset | field | established by |
|---|---|---|
| 0x00 | `identifier` | `datum_new` writes the salt at element+0; no device function reads it |
| 0x02 | `flags` | `device_groups_initialize` (writes `scenario_group->flags & 1`), `device_new` (writes 4, or 5 with can_change_only_once), `device_group_set_value` (ORs in 2), `FUN_0044c0c0` and `FUN_0044ba10` (read bits 0 and 1) |
| 0x04 | `value` | `device_group_set_value`, `device_group_set_value_immediate`, `device_change_power_state`, `device_update_change_values`, `device_new`, `FUN_0044c0c0`, `device_machine_update` |

Stride 8 is fixed by the uniform indexing `*(int *)(0x0087abf0 + 0x34) + index * 8 + field`
in all seven of those functions. Every field is accounted for.

The flag bits: bit 0 `can_change_only_once` (from `ScenarioDeviceGroup.flags` bit 0 at level
load, from `ScenarioDeviceFlags` bit 2 for runtime groups), bit 1 `changed` (set by
`device_group_set_value` on each accepted change). The locked predicate everywhere in the
module is *both* bits set, which `out/phase2/results/devices_00.json` records backwards for
0x44c0c0 ("requiring bits 0 and 1 both set to allow change" -- the code returns 0 in that
case, so both set means refuse).

### `device_placement_data` (0x08, the shared slice at scenario placement +0x28)

All three fields from `device_new`, cross-checked against `ScenarioMachine`,
`ScenarioControl` and `ScenarioLightFixture` in `types/tags.h`.

### `device_data` (0x24, object+0x1f4 .. 0x218)

| offset | field | established by |
|---|---|---|
| 0x1f4 | `flags` | `device_new` (bits 0,1 from placement), `FUN_0044c0c0` (reads bit 1), `device_update_change_values` / `device_group_set_value_immediate` / `device_machine_update` (set bit 2), `device_machine_update` (clears bit 2 after repositioning) |
| 0x1f8 | `power_group` | `device_new`, `device_update_change_values`, `device_group_set_value_immediate`, `FUN_0044c0c0` |
| 0x1fa | `unknown_1fa` | never read; 2 bytes of alignment before the float at 0x1fc |
| 0x1fc | `power` | `device_new`, `device_update_change_values`, `device_machine_update` (the gear blend), `FUN_0044ba10` cases 1 and 5, `device_group_set_value_immediate` |
| 0x200 | `power_change` | `device_update_change_values`, `FUN_0044ba10` case 2, zeroed by `device_group_set_value_immediate` |
| 0x204 | `position_group` | `device_new`, `device_machine_update`, `device_update_change_values`, `FUN_0044b5d0`, `FUN_0044c0c0`, `FUN_0044ba10`, `device_group_set_value_immediate` |
| 0x206 | `unknown_206` | never read; alignment before the float at 0x208 |
| 0x208 | `position` | `device_machine_update`, `device_update_change_values`, `FUN_0044ba10` cases 3 and 5, `device_group_set_value_immediate` |
| 0x20c | `position_change` | `device_update_change_values` (sign flip triggers the state-change effect), `FUN_0044ba10` case 4, `device_machine_update` (zeroed on the gear path), zeroed by `device_group_set_value_immediate` |
| 0x210 | `delay_ticks` | `device_update_change_values` (compared against `Device.delay_time_ticks`, incremented, reset to 0 when settled), `FUN_0044ba10` case 6 |
| 0x212 | `unknown_212` | never read; alignment before the dword at 0x214 |
| 0x214 | `type_flags` | `device_machine_update` (bits 0,1), `FUN_0044ba10` case 5 (bits 0,1,2), `FUN_0044b5d0` (bit 3), `device_frontfacing` (bit 0 on a control) |

`type_flags` carries the concrete placement flags word, which is `ScenarioMachineFlags` for a
machine and `ScenarioControlFlags` for a control. It is filled outside this module (by the
machine/control placement path in `objects`), so only the reads are evidence here. The reads
are unambiguous:

* bit 0 gates the entire proximity scan -> `does_not_operate_automatically`;
* bit 1, together with a closed door, restricts the scan to objects whose displacement has a
  positive dot product with the device forward vector -> `one_sided`;
* bit 2 forces the `devicein_locked` function output back to 0.0 -> `never_appears_locked`;
* bit 3 is the only thing `FUN_0044b5d0` tests before slamming the position group to 1.0 ->
  `opened_by_melee_attack`. This corrects the phase-2 reading, which called 0x44b5d0
  `device_apply_initial_open_flag` and guessed a "starts open" placement flag; bit 3 of
  `ScenarioMachineFlags` is `opened_by_melee_attack`, and the single caller plus the
  immediate (not interpolated) open are exactly a melee response.
* For a `device_control`, `ScenarioControlFlags` has only bit 0, `usable_from_both_sides`,
  and `device_frontfacing` skips its front-marker test when that bit is set. Phase 2 called
  the object it fetches "a unit"; it is not -- `object_try_and_get(0x100)` is
  `_object_mask_device_control`, so the 0x214 read is on the control itself.

### `device_machine_data` (0x34, object size 0x228)

| offset | field | established by |
|---|---|---|
| 0x218 | `ticks_since_fully_open` | `device_machine_update`: incremented only while `machine_type == door` and `position == 1.0`, compared against `DeviceMachine.door_open_time_ticks` (tag+0x320) to close, reset to 0 when the door leaves 1.0 and to -3 when the proximity scan reopens it |
| 0x21c | `last_elevator_position` | `device_machine_update`, elevator branch: the node position from the previous tick, differenced against this tick to translate riders |

The elevator branch is also where the node addressing is pinned:
`node = object + *(int16 *)(object + 0x1f2) + elevator_node * 0x34`, i.e.
`object.nodes` (`object_block_reference` at 0x1f0, byte offset at 0x1f2) with a
`real_matrix4x3` stride of 0x34 and the translation at node+0x28. That matches
`types/objects.h` and `types/math.h` and needed nothing new.

### `device_control_data` (0x28, object size 0x21c)

`unknown_218`, 4 bytes. No function in this module reads it. See unresolved offsets below.

### `device_light_fixture_data` (0x38, object size 0x22c)

`unknown_218[0x14]`, 20 bytes. No function in this module reads it.

## Enums and constants defined

* `device_group_flags` -- bits 0, 1 established as above; bit 2 is **UNSURE** (see below).
* `device_flags` (object+0x1f4) -- 3 bits, all established.
* `device_machine_flags` / `device_control_flags` (object+0x214) -- mirrors of
  `ScenarioMachineFlags` / `ScenarioControlFlags`, declared in the engine spelling because
  the module tests them on the object rather than on the placement.
* `device_constants` -- the two `object_find_in_sphere` buffer sizes (0x10 for the
  auto-open scan, 0x800 for the elevator rider sweep, both `__chkstk` stack buffers in
  `device_machine_update`), the 4-tick stagger `(game_time + object_index) & 3`, the -3 grace
  reset, the 4-entry function loop, and the two group tags
  `0x65666665` (effect) and `0x736e6421` (sound) that `device_play_state_change_effect`
  compares `tag_instance[i].group_tag` against.

`DeviceIn` in `types/tags.h` already enumerates the six selector values `FUN_0044ba10`
switches on (`power`, `change_in_power`, `position`, `change_in_position`, `locked`, `delay`),
in that exact order, so it is not redefined. The case bodies confirm the order: case 1 reads
`power`, case 2 divides `|power_change|` by `inverse_power_transition_time`, case 3 reads
`position`, case 4 divides `|position_change|` by `inverse_position_transition_time`, case 5
is the lock/door test, case 6 divides `delay_ticks` by `delay_time_ticks`.

## Globals

Owned:

```
0x0087abf0  data_array *device_groups   elements are device_group, stride 8
```

Read but not owned: `0x008603b0` (objects data_array), `0x0087bc14`
(`tag_instance *`, `types/cache.h`), `0x00746f8c` (`void *global_scenario`), `0x006f1d6c`
(game time globals, +0x0c the tick counter), `0x006f1d20` (multiplayer/campaign selector in
the one-sided test), `0x006b0b84` (game globals, +0xa4 a per-team bitmask),
`0x006b0a10` and `0x00686fe8` (recorded animations, see below), `0x00696718` and
`0x006966f8` (constant pointers into `.rdata` at 0x0065c20c and 0x0065c230 handed straight to
the effect and sound creation calls).

`0x00672abc`, `0x00672ac0`, `0x00672ac4` and `0x00672bbc` are **not globals**: reading them
out of the image gives the float literals 0.5, 0.0, 1.0 and 0.0001. Ghidra labels them `DAT_`
because they are the operands of `fld`.

`device_groups` is created by a `data_new`/`game_state_new` call outside this module, so
`k_maximum_device_groups` is not established here and is deliberately absent from the
constants enum.

## Unresolved offsets

1. **`device_group.flags` bit 2** -- set by `device_new` on both groups it allocates (power
   gets `4`, position gets `4 | can_change_only_once`), never set by
   `device_groups_initialize`, and never read anywhere in this module. Named
   `_device_group_object_created_bit` and marked UNSURE in the header. Whoever does the
   `hs` / scripting side (`device_group_set`, `_hs_type_device_group` is 16 in `types/hs.h`)
   should be able to settle it, since that is the most likely reader.
2. **`device_control_data.unknown_218`** (4 bytes) -- untouched here. The obvious candidate
   is the `custom_control_name` int16 at `ScenarioControl` 0x34 plus padding, or a
   last-activation tick stamp, but nothing in range proves either.
3. **`device_light_fixture_data.unknown_218[0x14]`** (20 bytes) -- untouched here.
   `ScenarioLightFixture` carries `ColorRGB color` (0x34), `float intensity` (0x40),
   `float falloff_angle` (0x44) and `float cutoff_angle` (0x48), which is exactly 0x14 bytes,
   so the light fixture runtime record is almost certainly a copy of that run. Not asserted,
   because no function in this module reads it.
4. **`device_data.unknown_1fa` / `unknown_206` / `unknown_212`** -- three int16 holes. They
   are certainly natural alignment before the following float/dword, and nothing ever reads
   them, but that is inference from the layout rather than from an access.
5. **`object + 0xb8`** -- `device_machine_update` reads it as a signed value, bounds-checks it
   against `0 <= x <= 9`, and uses `x + 10` to index a bitmask at `game_globals + 0xa4` (or
   compares it against 1 in the multiplayer path). That is a **team index**, not the
   `name_index` `types/objects.h` documents at 0xb8. One of the two is wrong; the 0..9 bound
   and the friendly-team bitmask favour team index. Flagged for the `objects` owner rather
   than changed here.
6. **`object + 0x4cc` bit 1 and `object + 0x4d4`** -- read by the misfiled recorded-animation
   update and by the elevator rider sweep respectively. Both are in `unit_data` territory
   (`unit_data` starts at object+0x1f4 for bipeds/vehicles, so these are unit fields around
   +0x2d8 / +0x2e0) and belong to `types/units.h`, not here. `0x4d4` is the field the rider
   sweep compares against the machine object index, so it is the rider/parent handle.

## Misattributed functions

### Recorded animations, not devices: 0x44a930, 0x44aa90, 0x44acc0, 0x44ad20

`modules.json` assigns these by address run ("run 0x44a930-0x44c1a0 devices"), and phase 2
named them `device_group_add_device`, `device_groups_update`,
`device_group_has_active_member` and `device_group_find_member`. They are none of those.
The decisive evidence:

* `FUN_0044a930` indexes `*(int *)(scenario + 0x36c)` / `*(int *)(scenario + 0x370)` with a
  stride of **0x40**. Scenario+0x36c is `recorded_animations`, and
  `sizeof(ScenarioRecordedAnimation) == 0x40`. `Scenario.device_groups` is at 0x288 with a
  stride of 0x34, which is what the genuinely device-owned `device_groups_initialize`
  (0x44c220) uses. The two blocks are not the same block.
* The fields it copies out of that record are `+0x20` (`version`), `+0x22`
  (`unit_control_data_version`), `+0x24` (`length_of_animation`) and `+0x38`
  (`recorded_animation_event_stream.pointer`, i.e. `TagDataOffset` 0x2c + 0x0c). Every one is
  a named `ScenarioRecordedAnimation` field.
* `(&PTR_PTR_00686fe8)[version - 1]` selects one of four entries at 0x00686fe8, which are
  `{0x686fe0, 0x686fe0, 0x686fe0, 0x686fd8}`; each target is a two-function vtable,
  `0x686fd8 = {0x44a550, 0x44a590}` and `0x686fe0 = {0x44a890, 0x44a8b0}`. Those four bodies
  live in the preceding run, which `modules.json` assigns to `cutscene` at 0.35 confidence
  with the evidence "table-driven struct pack/unpack, short delta quantisation" -- that is
  the unit-control-data codec.
* `device_groups_update` (0x44aa90) validates each record's object against
  `1 << object_header.type & 3`, that is biped or vehicle only, and feeds the decoded frame
  into `FUN_005639f0` in the units module. Device groups are not restricted to units, and no
  device function anywhere else in the module touches `0x006b0a10`.

Suggested names: `recorded_animation_start` (0x44a930),
`recorded_animations_update` (0x44aa90), `recorded_animation_object_is_playing` (0x44acc0),
`recorded_animation_find_by_object` (0x44ad20).

Their playback record is 0x64 bytes in the `data_array` at **0x006b0a10**, and is recorded
here so it is not lost. It is deliberately **not** defined in `types/devices.h`:

```
0x00  int16   identifier            datum salt
0x02  int16   unknown_02            alignment
0x04  uint32  object_index          datum_index of the unit being driven
0x08  int16   ticks_remaining       from ScenarioRecordedAnimation.length_of_animation 0x24,
                                    decremented each tick
0x0a  uint16  flags                 bit 0 finished / pending delete, bits 2,3,4 read by the
                                    update; the caller ORs extra bits in at creation
0x0c  int32   frame_index           0 at start, incremented after each decode
0x10  void   *event_stream          ScenarioRecordedAnimation event stream pointer (rec 0x38)
0x14  uint8   control_data[0x40]    the decoded unit control data, second argument to both
                                    vtable entries
0x54  uint8   decoder_state[0x0c]   first argument to both vtable entries
0x60  int16   codec_index           ScenarioRecordedAnimation.version - 1, indexes 0x00686fe8
0x62  int16   unknown_62            alignment
                                    total 0x64
```

### 0x44ad80 -- a bare `ret`

One byte, three callers, no globals. It is a no-op table entry (phase 2 guessed the same).
No types.

### Everything else is genuinely devices

The remaining 14 functions (0x44adf0, 0x44ae30, 0x44b0a0, 0x44b5d0, 0x44b720, 0x44ba10,
0x44bd70, 0x44bea0, 0x44bf90, 0x44c090, 0x44c0c0, 0x44c130, 0x44c1a0, 0x44c220) all reach
either `0x0087abf0` or a `Device`-derived tag, and every offset they touch is accounted for
in this header or in an existing one.

## Naming corrections worth carrying into phase 5

| address | phase-2 name | suggested |
|---|---|---|
| 0x44adf0 | `device_maybe_change_power_state` | `device_control_activate` (guards on `triggers_when == touched_by_player`) |
| 0x44b5d0 | `device_apply_initial_open_flag` | `device_machine_melee_attacked` (tests `opened_by_melee_attack`) |
| 0x44ba10 | `device_get_change_function_values` | `device_compute_function_values` (fills `object.function_in_values`) |
| 0x44c090 | `device_control_maybe_update` | `device_control_touched` |
| 0x44c0c0 | `device_can_change_position` | keep; the evidence text has the lock test inverted |
| 0x44c130 | `device_frontfacing` | keep (CEA PDB hint plus the "front" marker string); the object it fetches is the control, not a unit |
