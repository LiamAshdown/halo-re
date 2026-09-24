# `items` - items, equipment and weapons

Retail Halo PC `halo.exe` 1.0.10, `0x4bbb50 .. 0x4c6340`, plain C / MSVC 7.1 / x86. 76 files for
24,455 bytes of code; one function per file, rewritten from its Ghidra decompilation against
`types/items.h`, with the original decompile preserved verbatim at the bottom inside
`#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py items` -> **76 ok, 0 failed**.

Ghidra lists 101 entry points in the range. 25 of them are **not** items-module functions and are
deliberately absent; see "Misattributed functions" below. 76 + 25 = 101.

Read the confidence column in the function table before trusting a file. The whole networking
corner of this module (`0x4c25a0`, `0x4c3530`, `0x4c3870`, `0x4c4ac0`, `0x4bbe20`, `0x4c5c10`,
`0x4c6070`) decompiles with **no recovered arguments at all** - every value arrives in a register
or in an untyped stack buffer - so those seven files were re-derived from
`objdump -d -M intel bin/halo.exe` rather than from the decompilation. Each one quotes the
instructions that fix its layout in its header comment.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| equipment | `0x4bbb50`-`0x4bc0f0` | the pickup sound pair, and the equipment creation / network-update codecs |
| item physics | `0x4bc5c0`-`0x4bd740` | `item_update` (the airborne sweep, bounce and come-to-rest), `item_accelerate`, rotation and ground alignment, the holder/effective-position accessors |
| weapon per-tick | `0x4c1530` | `weapon_update`: heat, age, charge, the ready timer, the per-magazine recharge and reload machine, the per-trigger effect switch |
| weapon accessors | `0x4c24d0`-`0x4c3100` | label, ammo and zoom queries, HUD ammo state, the small predicates unit code asks before melee / grenades / swap |
| trigger state machine | `0x4c3190`-`0x4c3eb0` | ready-to-fire, fire-or-reload, the reload / chamber cycle, and the nine trigger effect states |
| firing | `0x4c3f10`-`0x4c4c40` | `weapon_fire_trigger` and `trigger_create_projectiles`, plus the shot-finish and effect helpers |
| state setters | `0x4c54e0`-`0x4c58c0` | barrel spread, the animation-driven state word, ammo counts, the battery fraction |
| weapon networking | `0x4c5a50`-`0x4c6340` | the weapon creation / network-update codecs |

Items are an `object` extension, not a separate record. `item_data` starts at object `0x1f4` and
is shared by every weapon, equipment and garbage object; the type-specific block starts at object
`0x22c` and is one of `weapon_data`, `equipment_data` or `garbage_data`. `k_item_data_offset` and
`k_item_extension_offset` in `types/items.h` are those two constants.

Three network hash tables run through this module and are worth knowing before reading any of the
codec files. `object_pooled_node_globals` (`0x00687130`) and `network_message_table_b`
(`0x00687558`) are two parallel tables; `+0x0c` off each is the `hash_table` that
`hash_table_get` turns an object `datum_index` into a small hash with, and `+0x28` off each is the
`datum_index` array the receiver turns that hash back through. The senders and receivers pair up
exactly: `parent_hash` goes through `0x00687130` and lands in `object_placement_data.role`, and
`owner_hash` goes through `0x00687558` and lands in `object_placement_data.owner_linkage`.

## Struct layouts

All of these live in `types/items.h`. Offsets are byte offsets from the struct base under
`#pragma pack(push,1)`, and every offset in these tables is checked mechanically against the
header with an `offsetof` probe. Types this module uses but does not own -- `data_array`,
`datum_index` (`types/memory.h`), `real_point3d`, `real_vector3d`, `real_matrix4x3`
(`types/math.h`), `tag_instance` (`types/cache.h`), `object`, `object_marker`,
`object_placement_data` (`types/objects.h`) and every tag structure (`types/tags.h`) -- are not
redefined here.

### `item_data` - size `0x38`, at object `0x1f4`; shared by weapon, equipment and garbage

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `flags` - `item_flags` |
| `0x04` | `int16_t` | `detonation_countdown` - ticks; at zero the object is deleted |
| `0x06` | `int16_t` | `resting_surface_index` - into the surface table at `0x00746f98 + 0x40`, stride `0x0c` |
| `0x08` | `int16_t` | `resting_bsp_index` - stamped from `0x0069e8d8`; a mismatch invalidates the rest |
| `0x0a` | `int16_t` | `unknown_1fe` |
| `0x0c` | `datum_index` | `ignore_object_index` - the object the sweep skips; forced to -1 on rest |
| `0x10` | `int32_t` | `held_game_time` - refreshed every tick while in an inventory |
| `0x14` | `datum_index` | `resting_object_index` - the object the item is lying on |
| `0x18` | `real_point3d` | `contact_point` - in the resting object's node space |
| `0x24` | `real_vector3d` | `rotation_axis` - **dual use**: the normalized angular velocity while moving, the ground normal once at rest |
| `0x30` | `float` | `rotation_sine` |
| `0x34` | `float` | `rotation_cosine` |

`item_flags`: `0x01` in inventory, `0x02` held by a player, `0x04` rotation valid, `0x08` at rest
on a structure surface, `0x10` at rest on an object, `0x20` does not accelerate, `0x40`
unresolved (cleared by `item_set_holder` and `equipment_pickup_play_sound`, never set or tested).

### `weapon_data` - size `0x114`, at object `0x22c`

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint32_t` | `flags` - `weapon_flags` |
| `0x004` | `uint16_t` | `control_flags` - `weapon_control_flags`, rebuilt every tick by `unit_update` |
| `0x006` | `uint16_t` | `unknown_232` |
| `0x008` | `float` | `primary_trigger` - the analog pull, for tag triggers with flag `0x200` |
| `0x00c` | `int8_t` | `state` - `weapon_state`; `weapon_set_state` maps it onto an animation index |
| `0x00d` | `int8_t` | `unknown_239` |
| `0x00e` | `int16_t` | `action_ticks` - ready / put-away animation; triggers are ignored while positive |
| `0x010` | `float` | `heat` - 0..1 |
| `0x014` | `float` | `age` - 0..1; the one weapon field carried in the network state |
| `0x018` | `float` | `charged_fraction` - suppresses heat decay while nonzero |
| `0x01c` | `float` | `ready_timer` - decrements by 1/24 per tick |
| `0x020` | `uint32_t` | `unknown_24c` |
| `0x024` | `datum_index` | `tracked_object_index` - required by trigger effect state 5 |
| `0x028` | `uint32_t` | `unknown_254` |
| `0x02c` | `uint32_t` | `unknown_258` |
| `0x030` | `int16_t` | `alternate_shots_loaded` |
| `0x032` | `int16_t` | `unknown_25e` |
| `0x034` | `weapon_trigger_state[2]` | `triggers` - object `0x260`, stride `0x28` |
| `0x084` | `weapon_magazine_state[2]` | `magazines` - object `0x2b0`, stride `0x0c` |
| `0x09c` | `uint32_t` | `unknown_2c8` |
| `0x0a0` | `datum_index` | `overheat_effect_handle` |
| `0x0a4` | `int32_t` | `last_fire_game_time` |
| `0x0a8` | `int16_t[2]` | `predicted_rounds_unloaded` - stride **2**, not the magazine stride |
| `0x0ac` | `int16_t[2]` | `predicted_rounds_loaded` |
| `0x0b0` | `uint32_t` | `unknown_2dc` |
| `0x0b4` | `uint8_t` | `network_state_valid` |
| `0x0b5` | `uint8_t` | `network_baseline_index` |
| `0x0b6` | `uint8_t` | `network_sequence` - wraps `0xff` -> 0 |
| `0x0b8` | `weapon_network_state` | `network_state` - object `0x2e4` |
| `0x0e4` | `uint8_t` | `last_update_valid` |
| `0x0e8` | `weapon_network_state` | `last_update_state` - a verbatim copy of the last accepted update |

### `weapon_trigger_state` - size `0x28`, `weapon_data.triggers[i]`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int8_t` | `idle_ticks` - saturates at `0x7f`; **signed**, and compared as a signed char |
| `0x01` | `int8_t` | `effect_state` - `weapon_trigger_effect_state` |
| `0x02` | `int16_t` | `effect_state_ticks` - ticked down while nonzero; -1 parks the state |
| `0x04` | `uint32_t` | `flags` - `weapon_trigger_state_flags` |
| `0x08` | `uint16_t` | `firing_effect_used_mask` |
| `0x0a` | `uint16_t` | `firing_effect_index` - the barrel the next round uses |
| `0x0c` | `int16_t` | `firing_effect_rounds` |
| `0x0e` | `int16_t` | `unknown_0e` |
| `0x10` | `float` | `firing_rate` - 0..1 spin-up; crossing `blurred_rate_of_fire` swaps the blur permutation |
| `0x14` | `float` | `ejection_port_recovery` - decayed by tag `ejection_port_recovery_rate` (`0xf4`) |
| `0x18` | `float` | `illumination_recovery` - decayed by tag `illumination_recovery_rate` (`0xf0`) |
| `0x1c` | `float` | `error` - accuracy bloom; `trigger_create_projectiles` reads it for the spread |
| `0x20` | `datum_index` | `effect_handle` |
| `0x24` | `int8_t` | `empty_ticks` - **signed**; past 10 an empty client weapon asks the host to reload |
| `0x25` | `uint8_t[3]` | `unknown_25` |

`weapon_trigger_state_flags`: `0x01` not pulled this tick, `0x02` was pulled last tick, `0x04` the
latch itself, `0x10` blur permutation applied, `0x20` a charging effect is playing. Bits `0x02`
and `0x04` are the edge detector for tag trigger flag `0x10` (a latching trigger).

### `weapon_magazine_state` - size `0x0c`, `weapon_data.magazines[i]`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `state` - `weapon_magazine_state_enum`: idle / reloading / chamber-pending / chambering |
| `0x02` | `int16_t` | `state_ticks` |
| `0x04` | `int16_t` | `state_ticks_total` |
| `0x06` | `int16_t` | `rounds_unloaded` - the reserve |
| `0x08` | `int16_t` | `rounds_loaded` - in the magazine |
| `0x0a` | `int16_t` | `rounds_recharged_accumulator` - **renamed from `unknown_0a` this pass**; see below |

`rounds_recharged_accumulator` was documented as unused. It is not: the `weapon_update` recharge
loop adds the tag magazine's `rounds_recharged % 30` to it every tick and moves one whole round
into `rounds_loaded` whenever it passes 29. Without it, a battery magazine whose
`rounds_recharged` is under 30 would never refill at all.

### `weapon_network_state` - size `0x2c` / `equipment_network_state` - size `0x24`

| Off | Type | Field (weapon) | | Off | Type | Field (equipment) |
|---|---|---|---|---|---|---|
| `0x00` | `real_point3d` | `position` | | `0x00` | `real_point3d` | `position` |
| `0x0c` | `real_vector3d` | `velocity` | | `0x0c` | `real_vector3d` | `velocity` |
| `0x18` | `uint8_t[0x0c]` | `unknown_18` - very likely a vestigial angular velocity | | `0x18` | `real_vector3d` | `angular_velocity` |
| `0x24` | `int16_t[2]` | `rounds_unloaded` | | | | |
| `0x28` | `float` | `age` | | | | |

### `equipment_data` - size `0x68` / `garbage_data` - size `0x18`, both at object `0x22c`

`garbage_data` is entirely unresolved: its two vtable columns (`0x4bc490`, `0x4bc510`) were Ghidra
misses; cleanup pass 1 rewrote them (`garbage_new` / `garbage_update`), which show the first word is a
random despawn countdown. `equipment_data` is `0x18` unresolved bytes, then
`network_state_valid` (`0x18`), `network_baseline_index` (`0x19`), `network_sequence` (`0x1a`),
`network_state` (`0x1c`), `last_update_valid` (`0x40`) and `last_update_state` (`0x44`). The tail
from `0x40` is laid out by analogy with `weapon_data` and the projectile record and has no reader
in the export, because the equipment update applier (`0x4bc250`) was another Ghidra miss; it is now
`equipment_apply_network_update.c` (cleanup pass 1) and confirms the layout.

### Network message records

`equipment_creation_message` and `weapon_creation_message` are both `0x58` bytes and share their
first `0x18` bytes exactly: `definition_tag` `0x00`, `object_hash` `0x04`, `name_index` `0x08`,
`owner_hash` `0x0c`, `parent_hash` `0x10`, `object_flags` `0x14`. Then both carry `position`
`0x18`, `forward` `0x24`, `up` `0x30`, `velocity` `0x3c`. Equipment continues with
`angular_velocity` `0x48` and `baseline_index` `0x54`; weapon continues with `baseline_index`
`0x48`, `rounds_unloaded[2]` `0x4a`, `age` `0x50` and `rounds_loaded[2]` `0x54`.

`weapon_magazine_ammo_message` (`0x0a`) is one record shape for three message types -
`0x2b` reload-begin, `0x2d` reload-end, `0x2e` reload-cancel: `object_hash` `0x00`,
`magazine_index` `0x04`, `rounds_unloaded` `0x06`, `rounds_loaded` `0x08`. The magazine index is
sign-extended at every use (`movsx`), so it is a signed `int16_t`.

`weapon_ammo_pickup_message` (`0x08`, type `0x2c`): `object_hash` `0x00`, `magazine_index` `0x04`,
`rounds` `0x06`.

`weapon_network_update_header` is the networking module's per-update sub-record, reached through
`update_record[0x11]`: `baseline_index` `0x04`, `sequence` `0x05`, `force_baseline` `0x06`. It is
folded into `types/items.h` so the module shares one spelling, and belongs in a `types/network.h`
when that module is written.

## Misattributed functions

25 of the 101 Ghidra entry points in `0x4bbb50 .. 0x4c6340` are not items-module functions. None
of them has a file here, and none is named in `symbols/agent_phase4_items.txt`.

**22 projectile functions.** These call `object_try_and_get` with mask `0x20`
(`_object_mask_projectile`) and read Projectile-tag fields, not Item ones. `projectile_data`
overlaps `item_data`, so folding them into `types/items.h` would produce a wrong header; they
belong in a future `types/projectiles.h`.

`0x4bda60` `0x4bdb40` `0x4bdc00` `0x4be1b0` `0x4beb30` `0x4bee20` `0x4beec0` `0x4bef80`
`0x4bf0f0` `0x4bf120` `0x4bf1c0` `0x4bf390` `0x4c0180` `0x4c0250` `0x4c0310` `0x4c03f0`
`0x4c0450` `0x4c0670` `0x4c0b10` `0x4c0ca0` `0x4c0f30` `0x4c1070`

**1 shell/main function.** `0x4c62d0` is a real function but belongs to shell/main.

**2 Ghidra artifacts.** `0x4c62f0` and `0x4c6340` are not functions at all; they are mid-body
labels inside `0x4c62d0` that Ghidra promoted to entry points.

### Renames away from `symbols/functions.txt`

Six names inherited from the phase-2 pass were wrong, and five of them are the same finding: at
these addresses `object_try_and_get`'s type mask is `4` (`_object_mask_weapon`), not the item
mask, so the function is a weapon function and the `item_` prefix misleads.

| Address | New name | Was |
|---|---|---|
| `0x4c1530` | `weapon_update` | `item_update_triggers` |
| `0x4c25a0` | `weapon_add_ammunition` | `item_add_ammunition` |
| `0x4c2610` | `weapon_transfer_ammunition` | `item_transfer_ammunition` |
| `0x4c2990` | `weapon_set_control_flags` | `item_set_permutation` |
| `0x4c3070` | `weapon_has_active_state` | `item_has_active_state` |
| `0x4bd500` | `item_compute_rotation` | `item_compute_ground_alignment_rotation` (a different finding: it computes the rotation axis/sine/cosine for any spinning item; only `item_update`'s caller is about ground alignment) |

## Known gaps

1. **`trigger_create_projectiles` (`0x4c4c40`) is missing its autoaim / magnetism block.** This is
   a behavioural gap, not a naming gap: on this path a weapon with autoaim fires perfectly
   straight and does not apply its per-target spread. The block resolves the holder's autoaim
   target by hand-walking the object `data_array`, consults `actor_data` (`0x00880360`, stride
   `0x724`, the `int16` at `+0x5f2` against 4), and calls `FUN_005658f0` / `FUN_0040f7e0` /
   `FUN_004593b0` to produce three values the function then uses: `spread_gain`, `error_bias` and
   `projectile_type_index`. It needs the units and ai headers plus three unresolved signatures.
   The file's header comment marks exactly where it sits in the `#if 0` block.
2. **No `player` type.** `item_get_effective_position` reads `player + 0x34` through a raw offset
   because the players module has not been written; it is used there as an object `datum_index`.
3. **`garbage_data` and the front of `equipment_data`** (`0x22c .. 0x244`) have no reader anywhere
   in the export, because the garbage and equipment vtable columns are Ghidra misses. The sizes
   come from `object_type_definition`, and nothing else about those bytes is known.
4. **`FUN_004ec590` / `FUN_004ec670` / `FUN_004e9cd0` / `FUN_004ec600` / `FUN_004ec590`** are the
   networking module's message-delta entry points. `FUN_004ec590`'s destination-in-ECX convention
   is established here from disassembly; the rest of that module's shape is not.
5. **`weapon_apply_network_update`'s `update_record`** is a networking record whose dword at
   `+0x00` is a *pointer* to the mode word, not the mode word. Only that double indirection and
   the header at `+0x44` are established; the rest of the record is unmapped.
6. **`object_set_position_and_relink`** takes a third argument at `item_update`'s call site (the
   sweep record's `+0x0c` block) that the two-argument declaration used across the repo cannot
   express. The same block is `FUN_00453490`'s third argument, which is why it is guessed to be a
   leaf/cluster location pair.
7. **`0x0069672c`** (a vector scaled by the per-tick gravity constant when a resting surface goes
   away) and **`0x00687558`** are still unnamed; the repo carries them as
   `global_reference_vector_0069672c` and `network_message_table_b`.

## Functions

Rewrite confidence is the file's own header value. `U` is the number of `UNSURE` markers in the
rewritten body (not counting the `#if 0` block).

| Address | Function | Bytes | Name | Rewrite | U |
|---|---|---|---|---|---|
| `0x4bbb50` | `equipment_pickup_play_sound` | 114 | 0.55 | 0.70 | 2 |
| `0x4bbbd0` | `equipment_definition_play_pickup_sound` | 83 | 0.60 | 0.80 | 1 |
| `0x4bbc90` | `equipment_build_creation_message` | 398 | 0.60 | 0.55 | 3 |
| `0x4bbe20` | `equipment_create_from_creation_message` | 578 | 0.60 | 0.75 | 5 |
| `0x4bc0f0` | `equipment_build_network_update` | 338 | 0.60 | 0.40 | 3 |
| `0x4bc5c0` | `item_update` | 2444 | 0.75 | 0.60 | 30 |
| `0x4bcf50` | `item_any_detonating` | 95 | 0.55 | 0.60 | 0 |
| `0x4bcfc0` | `item_set_holder` | 183 | 0.60 | 0.65 | 0 |
| `0x4bd080` | `item_accelerate` | 961 | 0.70 | 0.40 | 7 |
| `0x4bd450` | `item_detonation_timer_start` | 169 | 0.50 | 0.65 | 2 |
| `0x4bd500` | `item_compute_rotation` | 207 | 0.55 | 0.70 | 0 |
| `0x4bd5d0` | `item_align_to_normal_and_point` | 364 | 0.70 | 0.55 | 0 |
| `0x4bd740` | `item_get_effective_position` | 122 | 0.50 | 0.55 | 2 |
| `0x4c12b0` | `weapon_trigger_get_average_damage` | 152 | 0.45 | 0.60 | 1 |
| `0x4c1350` | `weapon_new_from_placement` | 208 | 0.50 | 0.60 | 1 |
| `0x4c1530` | `weapon_update` | 2981 | 0.55 | 0.55 | 4 |
| `0x4c24d0` | `weapon_get_label` | 59 | 0.50 | 0.75 | 0 |
| `0x4c2510` | `weapon_notify_ammo_pickup` | 135 | 0.35 | 0.45 | 1 |
| `0x4c25a0` | `weapon_add_ammunition` | 106 | 0.60 | 0.80 | 2 |
| `0x4c2610` | `weapon_transfer_ammunition` | 553 | 0.50 | 0.55 | 2 |
| `0x4c2840` | `weapon_ready` | 161 | 0.35 | 0.40 | 5 |
| `0x4c28f0` | `weapon_put_away` | 154 | 0.35 | 0.40 | 3 |
| `0x4c2990` | `weapon_set_control_flags` | 60 | 0.50 | 0.55 | 1 |
| `0x4c29d0` | `weapon_build_hud_ammo_state` | 250 | 0.45 | 0.70 | 0 |
| `0x4c2ad0` | `weapon_is_reloading` | 69 | 0.45 | 0.60 | 1 |
| `0x4c2b20` | `weapon_set_ready_timer` | 32 | 0.40 | 0.60 | 2 |
| `0x4c2b40` | `weapon_trigger_projectile_collision_test` | 153 | 0.30 | 0.30 | 1 |
| `0x4c2be0` | `weapon_trigger_projectile_time_fraction` | 140 | 0.30 | 0.45 | 2 |
| `0x4c2c70` | `weapon_is_out_of_ammo` | 118 | 0.40 | 0.40 | 2 |
| `0x4c2cf0` | `weapon_get_next_zoom_level` | 126 | 0.40 | 0.50 | 0 |
| `0x4c2d70` | `weapon_get_zoom_magnification` | 219 | 0.40 | 0.30 | 2 |
| `0x4c2e50` | `weapon_clamp_zoom_fov` | 71 | 0.35 | 0.40 | 1 |
| `0x4c2ea0` | `weapon_must_be_readied` | 55 | 0.40 | 0.70 | 0 |
| `0x4c2ee0` | `weapon_prevents_melee_attack` | 79 | 0.40 | 0.60 | 1 |
| `0x4c2f30` | `weapon_prevents_grenade_throwing` | 79 | 0.40 | 0.60 | 0 |
| `0x4c2f80` | `weapon_get_first_person_animation_time` | 225 | 0.45 | 0.45 | 1 |
| `0x4c3070` | `weapon_has_active_state` | 80 | 0.50 | 0.75 | 0 |
| `0x4c30c0` | `weapon_triggers_idle` | 60 | 0.45 | 0.75 | 0 |
| `0x4c3100` | `weapon_trigger_get_charge_fraction` | 132 | 0.35 | 0.65 | 0 |
| `0x4c3190` | `weapon_trigger_ready_to_fire` | 233 | 0.40 | 0.40 | 1 |
| `0x4c3280` | `weapon_trigger_fire_or_reload` | 485 | 0.35 | 0.40 | 4 |
| `0x4c3470` | `weapon_notify_reload_begin` | 183 | 0.35 | 0.35 | 1 |
| `0x4c3530` | `weapon_predict_ammo` | 121 | 0.50 | 0.80 | 0 |
| `0x4c35b0` | `weapon_trigger_begin_reload` | 510 | 0.50 | 0.35 | 3 |
| `0x4c37b0` | `weapon_notify_reload_step` | 183 | 0.35 | 0.35 | 1 |
| `0x4c3870` | `weapon_apply_ammo_correction` | 138 | 0.50 | 0.80 | 0 |
| `0x4c3900` | `weapon_magazine_reload_tick` | 284 | 0.40 | 0.35 | 1 |
| `0x4c3a20` | `weapon_magazine_reload_tick_predicted` | 212 | 0.40 | 0.35 | 0 |
| `0x4c3b00` | `weapon_magazine_begin_chamber` | 181 | 0.40 | 0.30 | 2 |
| `0x4c3bc0` | `weapon_trigger_become_charged` | 156 | 0.40 | 0.30 | 3 |
| `0x4c3c60` | `weapon_trigger_continue_burst` | 147 | 0.35 | 0.30 | 1 |
| `0x4c3d00` | `weapon_trigger_enter_recovery` | 223 | 0.35 | 0.35 | 1 |
| `0x4c3de0` | `weapon_trigger_handle_empty` | 89 | 0.35 | 0.50 | 0 |
| `0x4c3e40` | `weapon_trigger_effect_clear` | 48 | 0.45 | 0.80 | 0 |
| `0x4c3e70` | `weapon_trigger_effect_set_out_of_ammo` | 49 | 0.45 | 0.80 | 0 |
| `0x4c3eb0` | `weapon_trigger_reset_tracking` | 88 | 0.40 | 0.75 | 0 |
| `0x4c3f10` | `weapon_fire_trigger` | 2226 | 0.55 | 0.30 | 11 |
| `0x4c47d0` | `weapon_play_trigger_tag_effect` | 198 | 0.50 | 0.30 | 3 |
| `0x4c48a0` | `weapon_stop_object_effect` | 80 | 0.35 | 0.25 | 2 |
| `0x4c48f0` | `weapon_trigger_finish_shot` | 71 | 0.35 | 0.75 | 0 |
| `0x4c4940` | `weapon_reload_recovery_finish` | 116 | 0.30 | 0.30 | 2 |
| `0x4c49c0` | `weapon_trigger_effect_set_state` | 55 | 0.55 | 0.80 | 0 |
| `0x4c4a00` | `weapon_notify_reload_cancel` | 183 | 0.35 | 0.35 | 1 |
| `0x4c4ac0` | `weapon_apply_ammo_correction_and_resync` | 139 | 0.50 | 0.80 | 0 |
| `0x4c4b50` | `weapon_reset_triggers` | 233 | 0.50 | 0.55 | 1 |
| `0x4c4c40` | `trigger_create_projectiles` | 2193 | 0.85 | 0.20 | 0 |
| `0x4c54e0` | `weapon_trigger_barrel_spread_offset` | 158 | 0.35 | 0.40 | 1 |
| `0x4c5580` | `weapon_set_state_indicator_flags` | 164 | 0.30 | 0.50 | 0 |
| `0x4c5630` | `weapon_force_settled_state` | 58 | 0.40 | 0.75 | 0 |
| `0x4c5670` | `weapon_set_state` | 381 | 0.55 | 0.55 | 3 |
| `0x4c5820` | `weapon_set_ammo_counts` | 147 | 0.50 | 0.55 | 1 |
| `0x4c58c0` | `weapon_set_loaded_ammo_fraction` | 284 | 0.50 | 0.55 | 0 |
| `0x4c5a50` | `weapon_build_creation_message` | 433 | 0.40 | 0.40 | 2 |
| `0x4c5c10` | `weapon_create_from_creation_message` | 610 | 0.60 | 0.75 | 4 |
| `0x4c5f10` | `weapon_build_network_update` | 346 | 0.35 | 0.40 | 3 |
| `0x4c6070` | `weapon_apply_network_update` | 537 | 0.35 | 0.75 | 6 |

`trigger_create_projectiles` carries 0 `UNSURE` markers only because its uncertainty is stated as
prose in the file header rather than as inline markers; it is the least complete file here.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x4bba90` | `equipment_new` | 68 | 0.6 | 0.75 | 2 |  |
| `0x4bbae0` | `equipment_new_from_placement` | 104 | 0.6 | 0.7 | 2 |  |
| `0x4bbc30` | `equipment_send_creation` | 91 | 0.6 | 0.8 | 2 |  |
| `0x4bc070` | `equipment_network_baseline_take` | 118 | 0.65 | 0.8 | 0 |  |
| `0x4bc250` | `equipment_apply_network_update` | 461 | 0.65 | 0.75 | 7 | review: angular_velocity store added |
| `0x4bc420` | `equipment_is_old_enough` | 57 | 0.6 | 0.8 | 2 |  |
| `0x4bc460` | `item_stamp_age_timestamp` | 38 | 0.4 | 0.8 | 1 |  |
| `0x4bc490` | `garbage_new` | 124 | 0.6 | 0.8 | 2 |  |
| `0x4bc510` | `garbage_update` | 100 | 0.6 | 0.85 | 2 |  |
| `0x4bc580` | `item_new` | 60 | 0.6 | 0.85 | 3 |  |
| `0x4c1420` | `weapon_new` | 258 | 0.6 | 0.75 | 1 |  |
| `0x4c2110` | `weapon_update_function_values` | 886 | 0.75 | 0.7 | 6 | review: weapon_data read from the weapon, only the output goes to the root parent; firing_on returns the firing rate; NaN-faithful maxima |
| `0x4c59f0` | `weapon_send_creation` | 91 | 0.6 | 0.85 | 1 |  |
| `0x4c5e80` | `weapon_network_baseline_take` | 131 | 0.65 | 0.8 | 0 |  |
| `0x4c6290` | `weapon_is_old_enough` | 57 | 0.6 | 0.8 | 2 |  |

Gate: `python tools/build_check.py items` clean after the cleanup review.
