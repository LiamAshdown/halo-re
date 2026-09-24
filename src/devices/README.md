# `devices` — Blam device machines, controls and light fixtures

Retail Halo PC `halo.exe` 1.0.10, `0x44adf0 .. 0x44c220` (4,200 bytes of code, 14 functions),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/devices.h`, with the original decompile preserved verbatim at the
bottom of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py devices` → **14 ok, 0 failed**. Type smoke test:
`gcc -fsyntax-only -I types out/phase4/devices_smoke.c` → passes (negative-array size assertions
plus ~45 `offsetof` assertions written as *object* offsets).

`modules.json` gives this module the address run `0x44a930 .. 0x44c220`, 19 functions. Five of
those are not devices and no file was written for them; see **Misattributed functions** below.

## What the module contains

The device layer of the object hierarchy. Three object types — `device_machine` (7),
`device_control` (8), `device_light_fixture` (9) — share one `device_data` extension at
`object + 0x1f4` and one global value table, and everything here is about moving values between
those two.

| Family | Functions | What it is |
|---|---|---|
| the group table | `device_groups_initialize`, `device_new`, `device_group_set_value`, `device_group_set_value_immediate` | create and write the shared `device_group` slots |
| per-tick state machines | `device_update_change_values`, `device_machine_update` | interpolate object values toward group values; automatic door open/close; elevator rider transport |
| activation | `device_control_touched`, `device_control_activate`, `device_change_power_state`, `device_machine_melee_attacked` | a player touching a switch, or melee-ing a door, ends up writing a group value |
| predicates and outputs | `device_can_change_position`, `device_frontfacing`, `device_compute_function_values`, `device_play_state_change_effect` | lock test, front-marker side test, model function inputs, effect/sound dispatch |

### The one thing worth understanding before reading any file

Each device carries **two** links into the same group table: a `power_group` and a
`position_group`. `device_update_change_values` copies `group[power_group].value` into
`device_data.power` and `group[position_group].value` into `device_data.position`, each at a rate
the `Device` tag sets. The group index is a shared namespace, so **one group can be a control's
`position_group` and a machine's `power_group` at the same time**, and that is how a switch powers
a machine:

```
player touches control
  device_control_touched            object.type == device_control ?
    device_control_activate         DeviceControl.triggers_when == touched_by_player ?
      device_change_power_state     picks 0.0 / 1.0 / call_value from DeviceControl.type,
                                    writes it to the CONTROL's position_group N
        device_group_set_value      group[N].value = v, then walks every device object and
                                    fires Device.repowered / Device.depowered on each whose
                                    POWER group is N  -- i.e. on the machine
          ... next tick ...
        device_update_change_values machine: group[N] -> device_data.power
                                    control: group[N] -> device_data.position (visual throw)
        device_compute_function_values  power / position -> object.function_in_values -> animation
```

That is why `device_change_power_state` reads `object + 0x204` (`position_group`) despite its
name, and why `device_group_set_value` keys its broadcast on `power_group` despite being handed a
position group. Neither is a transcription error. The `control.position_group ==
machine.power_group` wiring itself lives in the scenario, outside this module, so it is recorded
as a hypothesis in both files' headers.

Doors are the other half. `device_machine_update` drives `position_group` directly (proximity
scan to open, `door_open_time_ticks` to close) and the open/close/opened/closed pair is played by
`device_update_change_values`, not by the group setter.

## Struct layouts

All of these live in `types/devices.h`; the tables below are the summary. `#pragma pack(push,1)`
is in force. Every size comes from the `object_type_definition` rows reached through the pointer
table at `0x0069bfdc` (`"machine"` `0x228`, `"control"` `0x21c`, `"light_fixture"` `0x22c`), not
from the highest offset seen.

### `device_group` — size `0x08`, one element of the `data_array` at `0x0087abf0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` — the datum salt `datum_new` writes; no device function reads it |
| `0x02` | `uint16_t` | `flags` (below) |
| `0x04` | `float` | `value`, always clamped to `0.0 .. 1.0` by both setters |

Stride 8 is fixed by the uniform `*(int *)(0x0087abf0 + 0x34) + index * 8 + field` in all seven
functions that touch it, and every index is scaled **unsigned** (`movzx`). Devices address groups
by raw `int16` slot index, never by `datum_index` — `-1` / `0xffff` means "no group".

| Bit | Name | Set by |
|---|---|---|
| 0 | `can_change_only_once` | `device_groups_initialize` (from `ScenarioDeviceGroup.flags` bit 0), `device_new` (from `ScenarioDeviceFlags` bit 2) |
| 1 | `changed` | `device_group_set_value`, on every accepted change |
| 2 | `object_created` — **UNSURE** | `device_new` only, on both groups it allocates; never read in this module |

The **locked** predicate everywhere is *both* bits 0 and 1 set.
`out/phase2/results/devices_00.json` records this backwards for `0x44c0c0`; the code refuses the
change in that case.

### `device_placement_data` — size `0x08`, the shared slice at scenario placement `+0x28`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `power_group`, `-1` means allocate a fresh group |
| `0x02` | `int16_t` | `position_group`, same |
| `0x04` | `uint32_t` | `flags` — `ScenarioDeviceFlags` |

`device_new` is handed a pointer straight at this slice, not at a whole placement record.
`ScenarioMachine`, `ScenarioControl` and `ScenarioLightFixture` all carry the same three fields at
the same offsets, which is what makes one function serve all three.

### `device_data` — size `0x24`, at `object + 0x1f4 .. 0x218` (offsets below are object offsets)

| Off | Type | Field |
|---|---|---|
| `0x1f4` | `uint32_t` | `flags` (below) |
| `0x1f8` | `int16_t` | `power_group` |
| `0x1fa` | `int16_t` | `unknown_1fa` — never read; alignment |
| `0x1fc` | `float` | `power`, the interpolated copy of `group[power_group].value` |
| `0x200` | `float` | `power_change`, signed rate |
| `0x204` | `int16_t` | `position_group` |
| `0x206` | `int16_t` | `unknown_206` — never read; alignment |
| `0x208` | `float` | `position` |
| `0x20c` | `float` | `position_change`, signed rate |
| `0x210` | `int16_t` | `delay_ticks`, counts up against `Device.delay_time_ticks`, reset to 0 once settled |
| `0x212` | `int16_t` | `unknown_212` — never read; alignment |
| `0x214` | `uint32_t` | `type_flags` — `ScenarioMachineFlags` for a machine, `ScenarioControlFlags` for a control, unused for a light fixture. Filled outside this module |

`flags` at `0x1f4`: bit 0 `position_reversed` (`ScenarioDeviceFlags` bit 3), bit 1
`not_usable_from_any_side` (bit 4), bit 2 `position_changed` — pure runtime bookkeeping, raised
whenever a cached value moves and cleared at the end of `device_machine_update` after it relinks
the object.

`type_flags` at `0x214`, for a **machine**: bit 0 `does_not_operate_automatically` (gates the
whole proximity scan), bit 1 `one_sided`, bit 2 `never_appears_locked`, bit 3
`opened_by_melee_attack`. For a **control**, only bit 0 `usable_from_both_sides`, which makes
`device_frontfacing` skip its marker test.

### `device_machine_data` — size `0x34`, object size `0x228`

| Off | Type | Field |
|---|---|---|
| `0x1f4` | `device_data` | `device` |
| `0x218` | `int32_t` | `ticks_since_fully_open` — incremented only while `machine_type == door` and `position == 1.0`, compared against `DeviceMachine.door_open_time_ticks` (tag `0x320`) to close; reset to `0` when the door leaves 1.0 and to `-3` when the proximity scan reopens it |
| `0x21c` | `real_point3d` | `last_elevator_position` — the `elevator_node` world position at the end of the previous tick; the delta is what riders get translated by |

### `device_control_data` — size `0x28`, object size `0x21c`

| Off | Type | Field |
|---|---|---|
| `0x1f4` | `device_data` | `device` |
| `0x218` | `uint32_t` | `unknown_218` — no function in this module touches it |

### `device_light_fixture_data` — size `0x38`, object size `0x22c`

| Off | Type | Field |
|---|---|---|
| `0x1f4` | `device_data` | `device` |
| `0x218` | `uint8_t[0x14]` | `unknown_218` — untouched here; `ScenarioLightFixture` carries a `ColorRGB`, an `intensity` and two cone angles at `0x34 .. 0x48`, which is exactly `0x14` bytes |

### Tag-side offsets, confirmed field for field

Nothing tag-side was redefined; `types/tags.h` already had it all. These are the offsets this
module actually loads, each verified against the disassembly during the phase-4 review:

| Offset | Field | Read by |
|---|---|---|
| `0x17c` | `Device.device_flags` (bit 0 `position_loops`) | `device_update_change_values` (the `wrap` argument) |
| `0x198` | `Device.device_a_in .. device_d_in` | `device_compute_function_values`, four `int16` selectors |
| `0x1ac` / `0x1bc` | `Device.open.tag_id` / `close.tag_id` | `device_update_change_values`, mid-flight reversal |
| `0x1cc` / `0x1dc` | `Device.opened.tag_id` / `closed.tag_id` | `device_update_change_values`, settle |
| `0x1ec` / `0x1fc` | `Device.depowered.tag_id` / `repowered.tag_id` | `device_group_set_value` |
| `0x218` | `Device.delay_effect.tag_id` | `device_update_change_values`, first delay tick |
| `0x21c` | `Device.automatic_activation_radius` | `device_machine_update`, guarded by `>= 0.0001` |
| `0x274` .. `0x28c` | the seven `inverse_*` rate fields and `delay_time_ticks` | `device_update_change_values`, `device_compute_function_values`, `device_machine_update` |
| `0x290` | `DeviceMachine.machine_type` / `DeviceControl.type` | the two switches in the module |
| `0x292` | `DeviceMachine.machine_flags` (bit 2 elevator) / `DeviceControl.triggers_when` | `device_machine_update`, `device_control_activate` |
| `0x294` | `DeviceControl.call_value` | `device_change_power_state`, case 3 |
| `0x2ea` | `DeviceMachine.elevator_node` | `device_machine_update`, scaled **signed** by `0x34` |
| `0x2f4` / `0x304` / `0x314` | `DeviceControl.on.tag_id` / `off.tag_id` / `deny.tag_id` | `device_change_power_state` |
| `0x320` | `DeviceMachine.door_open_time_ticks` | `device_machine_update`, signed compare |

`Device` is `0x290` on top of `Object` (`0x17c`), so `DeviceMachine.machine_type` and
`DeviceControl.type` necessarily share `0x290`. That pair is the strongest single confirmation in
the module: `device_change_power_state` switches on it with cases 0..3 and reads a float at
`0x294` in case 3, which is `DeviceType {toggle_switch, on_button, off_button, call_button}` plus
`DeviceControl.call_value`; `device_machine_update` switches on the same offset against 0 and 2,
which is `MachineType {door, platform, gear}`.

## Globals

Owned:

```
0x0087abf0  data_array *device_groups   elements are device_group, stride 8
```

The `data_new` / `game_state_new` call that creates it is outside this module, so
`k_maximum_device_groups` is not established here.

Read but not owned: `0x008603b0` (`data_array *object_data`), `0x0087bc14`
(`tag_instance *tag_instances`), `0x00746f8c` (`Scenario *global_scenario`), `0x006f1d6c`
(game time globals, `+0x0c` the tick counter), `0x006f1d20` (multiplayer/campaign selector),
`0x006b0b84` (game globals, `+0xa4` a per-team bitmask), `0x00696718` / `0x006966f8` (two
constant pointers into `.rdata` at `0x0065c20c` and `0x0065c230`, handed to the sound creation
routine without being dereferenced).

`0x00672abc`, `0x00672ac0`, `0x00672ac4` and `0x00672bbc` are **not globals** — reading them out
of the image gives the float literals `0.5`, `0.0`, `1.0` and `0.0001`. Ghidra labels them `DAT_`
because they are `fld` / `fcomp` operands.

## Misattributed functions

Five of the 19 addresses `modules.json` assigns to this module are not devices. No file was
written for any of them, and `types/devices.h` deliberately defines none of their types.

1. **`0x44a930`, `0x44aa90`, `0x44acc0`, `0x44ad20` are recorded-animation playback.**
   `0x44a930` indexes `Scenario.recorded_animations` (count `0x36c`, pointer `0x370`, stride
   `0x40` = `sizeof(ScenarioRecordedAnimation)`) — `Scenario.device_groups` is at `0x288` with
   stride `0x34`, a different block — and copies that record's `version` `0x20`,
   `unit_control_data_version` `0x22`, `length_of_animation` `0x24` and event stream pointer
   `0x38`. `0x44aa90` validates each record's object against `1 << object_header.type & 3` (biped
   or vehicle only) and feeds the decoded frame to `FUN_005639f0` in `units`; device groups are
   not restricted to units, and no device function anywhere touches their `data_array` at
   `0x006b0a10`. Their `0x64`-byte playback record is documented in
   `out/phase4/devices_types_notes.md` and is not in `types/devices.h`. `symbols/functions.txt`
   previously carried `device_groups_update` for `0x44aa90`; the phase-4 correction to
   `recorded_animations_update` is registered in `symbols/agent_phase4_devices.txt`.
2. **`0x44ad80` is a bare `ret`** — one byte, three callers, no globals, a no-op table entry.

## Known gaps

1. **`object + 0xb8` is read as a team index, not `name_index`.** `device_machine_update`'s
   one-sided door test does `mov ax,WORD PTR [esi+0xb8]`, bounds-checks it signed against
   `0 <= x <= 9`, and uses `x + 10` to index a bitmask at `game_globals + 0xa4` (or compares it
   against 1 on the multiplayer path). `types/objects.h` documents `name_index` at that offset.
   The width and signedness of the read are disassembly-confirmed; which field lives there is
   not. The file keeps the raw offset so the mismatch stays visible. Belongs to the `objects`
   owner.
2. **`object + 0x4d4` is the rider's parent handle**, compared against the machine's own object
   index by the elevator sweep. That is `unit_data` territory (`unit_data` starts at
   `object + 0x1f4`, so this is around unit `+0x2e0`) and is not yet in `types/units.h`, so it
   stays a raw offset here.
3. **`UnitFlags` bit 14 has no named constant.** `device_machine_update` rejects a nearby object
   when `tag_data + sizeof(Object) & 0x4000` is set, which `types/tags.h` spells
   `cannot_open_doors_automatically` in a bitfield comment rather than an enum, so the mask is
   written literally. This is what identifies the block as the automatic door-opening scan rather
   than a generic collision sweep.
4. **`device_group.flags` bit 2 is set but never read** in this module. `device_new` writes it on
   both groups it allocates and nothing here reads it back. The `hs` scripting side
   (`device_group_set`, `_hs_type_device_group` is 16 in `types/hs.h`) is the most likely reader.
5. **`device_control_data.unknown_218`** (4 bytes) and **`device_light_fixture_data.unknown_218`**
   (20 bytes) are untouched here. For the control the obvious candidate is
   `ScenarioControl.custom_control_name` (`int16` at `0x34`) plus padding, or a last-activation
   tick stamp; for the light fixture, the `ColorRGB` / `intensity` / two-cone-angle run at
   `ScenarioLightFixture 0x34 .. 0x48`. Neither is asserted.
6. **`0x006f1d20` is read at two widths across the repo.** `device_machine_update` reads all 32
   bits (`mov edx,DWORD PTR ds:0x6f1d20; test edx,edx` at `0x44b290`); five files in `src/items`
   and `src/objects` declare the same address as `uint8_t network_predicted_state_flag`. Both are
   correct transcriptions of their own call site. Likewise `0x006b0b84` is `void *game_globals`
   here, `uint32_t *g_006b0b84` in `src/objects`, and its `+0xa4` run is `friendly_fire_matrix`
   in `src/units`.
7. **`device_control_activate` reads its object id from an inherited register.**
   `device_change_power_state` takes its object id in EBX and its fallback float in ECX, and
   `device_control_activate`'s own 62 bytes set neither — both are whatever a still-higher,
   undiscovered caller left there. The rewrite passes `object_id` explicitly (which reproduces the
   effect) and `0.0f` for the float (which cannot be known). The float only matters for a
   malformed `DeviceControl.type`; see gap 8.
8. **`device_change_power_state`'s switch default is reachable.** `cmp eax,3; ja 0x44aebe` at
   `0x44ae7a` forwards the caller's ECX float straight to `device_group_set_value` for any
   `DeviceControl.type` outside 0..3, including a negative one. Unreachable for well-formed tag
   data only.
9. **The two creation routines the effect dispatcher tail-calls are outside this module.**
   `device_play_state_change_effect` calls `0x4507a0` (effect) with `EAX = object_index`,
   `ECX = tag_id` and six stack arguments — `(object_index, -1, device_data.position,
   device_data.power, 0, 0)` — and `0x543ce0` (sound) with the two `.rdata` constants in EAX/ECX
   and four stack arguments `(tag_id, -1, 1.0f, 0)`. The values passed are disassembly-confirmed;
   only the parameter *names* in the `extern` declarations are guesses.
10. **`device_machine_update`'s final block is a literal self-assignment.** `0x44b592-0x44b5b0`
    reads `object.position` and writes it back to the same address; the actual effect of the block
    is the relink the two surrounding `object_unlink_cluster_or_notify_parent` /
    `object_set_cluster_and_parent` calls perform. Preserved as-is rather than "corrected".
11. **`0x86868686` is dropped.** Both group setters build an `object_iterator` on the stack and
    the original also writes `0x86868686` into the 4 bytes past the `0x0c`-byte struct. Every
    other module in this repo drops that store (it appears only inside `#if 0` blocks elsewhere),
    so `devices` follows the house convention. If `object_iterator` turns out to be `0x10` bytes
    with a signature field, six modules need the same correction, not just this one.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite, `U` counts distinct
`UNSURE` issues in the file (not marker lines — several are referenced twice, once in the header
and once inline).

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x44adf0` | `device_control_activate` | 62 | 0.80 | 0.70 | 2 |
| `0x44ae30` | `device_change_power_state` | 234 | 0.50 | 0.85 | 0 |
| `0x44b0a0` | `device_machine_update` | 1320 | 0.55 | 0.75 | 3 |
| `0x44b5d0` | `device_machine_melee_attacked` | 68 | 0.80 | 0.90 | 0 |
| `0x44b720` | `device_update_change_values` | 749 | 0.50 | 0.85 | 0 |
| `0x44ba10` | `device_compute_function_values` | 501 | 0.80 | 0.85 | 0 |
| `0x44bd70` | `device_group_set_value` | 292 | 0.55 | 0.85 | 1 |
| `0x44bea0` | `device_group_set_value_immediate` | 227 | 0.50 | 0.90 | 0 |
| `0x44bf90` | `device_new` | 251 | 0.60 | 0.90 | 0 |
| `0x44c090` | `device_control_touched` | 45 | 0.80 | 0.90 | 0 |
| `0x44c0c0` | `device_can_change_position` | 102 | 0.80 | 0.85 | 0 |
| `0x44c130` | `device_frontfacing` | 112 | 0.70 | 0.85 | 0 |
| `0x44c1a0` | `device_play_state_change_effect` | 126 | 0.60 | 0.80 | 1 |
| `0x44c220` | `device_groups_initialize` | 111 | 0.50 | 0.90 | 0 |

Ten of the fourteen are disassembly-verified end to end (`objdump -d -M intel bin/halo.exe`),
which is how the open/close, opened/closed, repowered/depowered and on/off/deny tag-id selections
were settled — Ghidra elides every one of those register arguments. The five names first
established or corrected by this rewrite are registered in `symbols/agent_phase4_devices.txt`
and merged into `symbols/functions.txt` by `tools/merge_symbols.py`; the other nine already
matched `symbols/functions.txt` exactly.
