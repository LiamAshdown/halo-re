# items module: type recovery notes

Header: `types/items.h`. Smoke test: `out/phase4/items_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/items_smoke.c
```

It passes, and it carries negative-array assertions for every struct size plus ~70 individual
`offsetof` assertions written as *object* offsets (`0x22c + offsetof(weapon_data, heat) == 0x23c`
and so on), so a field reordering fails the build instead of passing silently. The other six
phase-4 smoke files still compile unchanged.

## The binary evidence everything hangs on

### 1. object_type_definition rows -> every struct size

The 12-pointer table at **0x0069bfdc** (PE image base 0x400000, `.data` VA 0x676000 -> file
offset 0x276000). Each row is `{char *name, uint32 group, int16 object_size, ...}`:

| row | name | group | object_size | chain (`subdefinitions[]` at +0x80) |
|---|---|---|---|---|
| 0x0069b360 | object | `obje` | 0x1f4 | object |
| 0x0069b680 | **item** | `item` | **0x22c** | object, item |
| 0x0069b748 | **weapon** | `weap` | **0x340** | object, item, weapon |
| 0x0069b810 | **equipment** | `eqip` | **0x294** | object, item, equipment |
| 0x0069b8d8 | **garbage** | `garb` | **0x244** | object, item, garbage |
| 0x0069b9a0 | projectile | `proj` | 0x2b0 | object, projectile |

So `item_data` is exactly 0x38 bytes at object+0x1f4, and `weapon_data` (0x114),
`equipment_data` (0x68) and `garbage_data` (0x18) all start at object+0x22c. No size in the
header is inferred from the highest offset seen.

### 2. The same rows fix three unknown fields in `types/objects.h`

`object_type_definition` currently documents `unknown_0a` / `unknown_0c` / `unknown_10`. The
bytes and their one consumer say:

```
+0x0a int16  scenario_placement_offset    byte offset of a TagReflexive inside the Scenario tag
+0x0c int16  scenario_palette_offset      byte offset of the matching palette TagReflexive
+0x0e int16  scenario_placement_size      stride of one placement record
+0x10 int32  network_delta_message_type   index into the network message group, -1 = none
```

Raw values, checked against `types/tags.h`:

| row | +0x0a | +0x0c | +0x0e | +0x10 | matches |
|---|---|---|---|---|---|
| weapon | 0x0270 | 0x027c | 0x005c | 3 | `Scenario.weapons` 0x270, `weapon_palette` 0x27c, `sizeof(ScenarioWeapon)` 0x5c |
| equipment | 0x0258 | 0x0264 | 0x0028 | 2 | `Scenario.equipment` 0x258, `equipment_palette` 0x264, `sizeof(ScenarioEquipment)` 0x28 |
| biped | 0x0228 | 0x0234 | 0x0078 | 4 | `Scenario.bipeds` 0x228, `biped_palette` 0x234, `sizeof(ScenarioUnit)` 0x78 |
| item / garbage | -1 | -1 | -1 | -1 | no scenario block, no network delta |

The consumer for +0x0a/+0x0c/+0x0e is the scenario placement sweep at **0x4f3ba0** (Ghidra
currently calls it `objects_update_control_bindings`, which does not match its body -- it is the
scenario object spawn pass), which computes
`piVar11 = (int *)(*(short *)(row + 10) + scenario_tag_data)` and then walks `piVar11[0]`
records of `*(short *)(row + 0x0e)` bytes out of `piVar11[1]`. The consumer for +0x10 is
`message_delta_encode_message`: `FUN_004bc0f0`, `FUN_004c0f30` and `FUN_004c5f10` all pass
`*(int *)(object_type_definitions[object->type] + 0x10)` as the message type, which is also how
those three functions were attributed to equipment / projectile / weapon respectively.

This is also what makes `FUN_004c1350` legible: its `param_2` is a `ScenarioWeapon`, and
`+0x48`, `+0x4a`, `+0x4c` are `rounds_reserved`, `rounds_loaded` and `flags`
(`initially_at_rest`, `obsolete`, `does_accelerate`).

Also worth carrying back to `types/objects.h`: `object_type_definition.category` at +0x04 is
documented there as "0 = delete immediately, 3 = delete recursively". As `units` already noted,
it is the big-endian group tag.

### 3. The blur permutation table

`0x006961b8` is two `char *` read straight out of `.data` -> `{"~primary-blur",
"~secondary-blur"}` (targets 0x0066b1e0 and 0x0066b1d0). `weapon_update` indexes it by trigger
index and hands the name to `object_set_permutation_by_name`, gated on
`weapon_trigger_state.firing_rate` crossing `WeaponTrigger.blurred_rate_of_fire` (0x14). That
pairing is what identifies the float at trigger+0x10.

## Structs defined

| struct | size | where | fixed by |
|---|---|---|---|
| `item_data` | 0x38 | object + 0x1f4 | the `item` row object_size 0x22c |
| `garbage_data` | 0x18 | object + 0x22c | the `garbage` row object_size 0x244 |
| `equipment_data` | 0x68 | object + 0x22c | the `equipment` row object_size 0x294 |
| `equipment_network_state` | 0x24 | equipment 0x248 and 0x270 | 0x4bbc90 reads nine floats, 0x4bbe20 writes them; two copies land exactly on 0x294 |
| `weapon_data` | 0x114 | object + 0x22c | the `weapon` row object_size 0x340 |
| `weapon_trigger_state` | 0x28 | weapon 0x260, two of them | every accessor spells the base as `(uint32 *)object + i*10 + 0x98` or `(uint8 *)object + i*0x28 + 0x260`; `item_has_active_state` reads 0x261 and 0x289 |
| `weapon_magazine_state` | 0x0c | weapon 0x2b0, two of them | `item_add_ammunition` indexes `0x2b6 + i*0x0c` and returns `0x2b0 + i*0x0c`; `item_has_active_state` reads 0x2b0 and 0x2bc |
| `weapon_network_state` | 0x2c | weapon 0x2e4 and 0x314 | `FUN_004c6070` block-copies 11 dwords from 0x2e4; two copies plus their valid bytes land exactly on 0x340 |
| `weapon_hud_magazine_state` | 0x0a | output of 0x4c29d0 | the code writes `out + i*10 + 0x0c .. +0x14`, and the six fields sum to 10 |
| `weapon_hud_ammo_state` | 0x20 | output of 0x4c29d0 | 0x0c + 2*0x0a |
| `equipment_creation_message` | 0x58 | stack | 0x4bbc90 builds it, 0x4bbe20 consumes it |
| `weapon_creation_message` | 0x58 | stack | 0x4c5a50 builds it, 0x4c5c10 consumes it |
| `weapon_magazine_ammo_message` | 0x0a | stack | 0x4c3470 / 0x4c37b0 / 0x4c4a00 encode, 0x4c3530 / 0x4c3870 / 0x4c4ac0 decode |
| `weapon_ammo_pickup_message` | 0x08 | stack | 0x4c2510 |

Enums: `item_constants`, `item_flags`, `weapon_flags`, `weapon_control_flags`, `weapon_state`,
`weapon_trigger_effect_state`, `weapon_trigger_state_flags`, `weapon_magazine_state_enum`.

## Which function established which field

### item_data (object 0x1f4 .. 0x22c)

* **0x1f4 flags.** `item_set_holder` (0x4bcfc0) is the proof for bits 0x01 and 0x02: it reads
  the word once, computes `(flags & ~0x40) | 1`, and then either `| 3` when the holder's
  `unit_data.controlling_player` (unit 0x218) is set or `(flags & ~0x42) | 1` when it is -1.
  `item_compute_rotation` (0x4bd500) owns bit 0x04 -- sets it when the object angular velocity
  is nonzero, clears it and forces sine/cosine to 0/1 otherwise. `item_update` (0x4bc5c0) sets
  bit 0x08 when the "ground point" marker test reports type 2 (a structure surface) and bit
  0x10 when it reports an object, and `item_accelerate` (0x4bd080) clears 0x08 when a real
  impulse arrives. Bit 0x20 comes from the scenario record in 0x4c1350 and is the early-out in
  `item_accelerate`. Bit 0x40 is only ever *cleared* (0x4bbb50, 0x4bcfc0) -- unresolved.
* **0x1f8 detonation_countdown.** `item_update` counts it down at the very end, and at zero it
  plays an effect through 0x4507a0 and calls `object_delete`. `FUN_004bd450` seeds it exactly
  once (`if (*(short *)(item + 0x1f8) == 0)`) and is called from `item_accelerate` when the
  Item tag has `destroyed_by_explosions` and from `weapon_update` when the Weapon tag has
  `detonates_when_dropped` (0x400) and the weapon has no parent. `FUN_004bcf50` iterates
  `_object_mask_item` and returns true if any item has it > 0.
* **0x1fa resting_surface_index / 0x1fc resting_bsp_index.** Written as a pair in the
  `local_120[0] == 2` branch of `item_update`: `0x1fa` takes the surface index the marker test
  returned and `0x1fc` takes the int16 global at 0x0069e8d8. The re-wake test reads them back
  together: `(flags & 8) && 0x1fa != -1 && 0x1fc == DAT_0069e8d8`, then indexes
  `*(int *)(DAT_00746f98 + 0x40) + 8 + resting_surface_index*0x0c` and re-accelerates when that
  byte has bit 0x08.
* **0x200 ignore_object_index.** `item_update` passes it as the second argument to the swept
  collision test at 0x401a20 and forces it to -1 in the branch that parks the item; the
  `puVar1[0x80] != 0xffffffff` test in `item_accelerate` is the other reader.
* **0x204 held_game_time.** The last statement of `item_update`:
  `if (flags & 1) item->held_game_time = *(int *)(0x006f1d6c + 0x0c)`.
* **0x208 resting_object_index / 0x20c contact_point.** Also the `item_update` resting branch:
  `puVar1[0x82] = local_e8` (the object the marker test hit), then
  `object_get_node_marker_address` + `matrix4x3_inverse_transform_point` into
  `puVar1 + 0x83`. On the following ticks the same pair is read back with the forward
  transform and handed to `item_align_to_normal_and_point`.
* **0x218 rotation_axis / 0x224 rotation_sine / 0x228 rotation_cosine.** Two writers, mutually
  exclusive on object flag 0x20: `item_compute_rotation` writes the normalized object angular
  velocity plus `fsin`/`fcos` of its magnitude, but only `if ((object->flags & 0x20) == 0)`;
  `item_update` writes the ground normal into the same three floats in the branch that has just
  set object flag 0x20. Readers use it both ways -- `vector3d_rotate_about_axis(0x224, 0x228)`
  around 0x218, and `item_align_to_normal_and_point(item + 0x86, ...)`. This is a genuine
  overlay in the original code, not a decompiler artefact.

### weapon_data (object 0x22c .. 0x340)

* **0x22c flags.** `weapon_update` owns 0x01 and 0x02 (heat crossing
  `overheated_threshold` 0x350 sets them, falling below `heat_recovery_threshold` 0x34c clears
  both with `& 0xfffffffc`); `weapon_fire_trigger` sets 0x04 for a `weapon_type == 3` secondary
  shot and `weapon_update` consumes it on overheat; `FUN_004c3530` sets 0x08 and 0x4c3870 /
  0x4c4ac0 / `weapon_trigger_begin_reload` clear it.
* **0x230 control_flags / 0x234 primary_trigger.** Established from the *caller*. The function
  Ghidra calls `item_set_permutation` (0x4c2990) writes `int16 -> 0x230` and
  `float(FUN_004ccac0(arg)) -> 0x234`, and its only caller is `unit_update` (0x5625b0), which
  builds the int16 out of its own `unit_control_flags` (`0x10 -> 0x01`, `0x800 -> 0x02`,
  `0x1000 -> 0x04`, `0x400 -> 0x08`, two predicates -> `0x10`, a melee test -> `0x40`) or sets
  it to exactly `0x20` when the weapon is not the unit's current one, and passes
  `unit + 0x284` as the float. `weapon_update` reads 0x230 as a ushort (bits 0x02, 0x04, 0x10,
  and `& 0x26`), and `weapon_trigger_ready_to_fire` (0x4c3190) substitutes 0x234 for the
  per-trigger firing rate when the tag trigger has flag 0x200. That is the whole reason the
  function had to be renamed; see "Misattributed / misnamed functions".
* **0x238 state.** `weapon_set_state` (0x4c5670) is the only writer
  (`*(char *)(object + 0x238) = (char)new_state`) and its switch maps the value onto an
  animation index, which names the eleven states. `weapon_force_settled_state` (0x4c5630)
  treats 7, 8 and 10 as persistent; `FUN_004c2f30` treats 5..10 as busy.
* **0x23a action_ticks.** `weapon_ready` (0x4c2840) writes
  `weapon_get_first_person_animation_time(0, -1)` here; `weapon_update` decrements it while
  positive and refuses every trigger while `action_ticks >= 1`.
* **0x23c heat / 0x240 age.** `weapon_fire_trigger` adds `heat_generated_per_round` (trigger
  0xb8) and `age_generated_per_round` (0xbc), clamping both at 1.0; `weapon_update` subtracts
  `heat_loss_rate/30` (tag 0x35c) scaled by `age_heat_recovery_penalty` (0x440).
  `weapon_set_loaded_ammo_fraction` writes `1.0 - fraction` into 0x240 for a weapon with no
  magazines but a trigger that has `age_generated_per_round`, which is what identifies it as
  the battery meter. `weapon_build_hud_ammo_state` reports both.
* **0x244 charged_fraction.** Written only in the charged case of `weapon_update`:
  `1.0 - (effect_state_ticks/30) / WeaponTrigger.charged_time`. It is also the gate on heat
  decay (`if (weapon->charged_fraction == 0.0) { decay }`), and `weapon_update` zeroes it at
  the top of every tick.
* **0x248 ready_timer.** `weapon_update` decrements it by `0.041666668` (1/24) per tick while
  positive, and skips the decrement when the holder's *Object* tag has flag 0x800000; in that
  case `unit_update` writes the field directly through `FUN_004c2b20`. UNSURE: nothing in this
  module reads the value, so "ready_timer" is inferred from `Weapon.ready_time` and from the
  weapon-ready state machine rather than proved.
* **0x250 tracked_object_index.** `weapon_update` scrubs it (`if (weapon->tracked_object_index
  != -1 && object_try_and_get(0xffffffff) == 0) = -1`) and trigger effect state 5 requires it
  to be live; `FUN_004c3eb0` resets it to -1 together with the trigger state.
* **0x25c alternate_shots_loaded.** `weapon_fire_trigger` increments it for
  `secondary_trigger_mode` 3 or 4 and both it and `weapon_update` bound it with
  `Weapon.maximum_alternate_shots_loaded` (0x32e).
* **0x260 triggers[2].** See below.
* **0x2b0 magazines[2].** See below.
* **0x2cc overheat_effect_handle.** `weapon_update` stores the return of `FUN_004c48a0` here
  the tick the weapon overheats, and `FUN_00450b20(1)` is called on it when the heat recovers;
  `weapon_put_away` (0x4c28f0) deletes the particle system it names and sets it to -1. It is
  therefore a looping sound / effect / particle-system datum.
* **0x2d0 last_fire_game_time.** `weapon_fire_trigger`:
  `weapon->last_fire_game_time = *(int *)(0x006f1d6c + 0x0c)`.
* **0x2d4 / 0x2d8 predicted_rounds_*.** `FUN_004c3530` writes `0x2d4 + i*2` and `0x2d8 + i*2`
  from an incoming message and sets flag 0x08; `weapon_trigger_begin_reload` copies both arrays
  into `magazines[i].rounds_unloaded` / `.rounds_loaded` when it runs client-side. Two int16
  each, ending at 0x2dc.
* **0x2e0..0x340 the network pair.** `weapon_apply_network_update` (0x4c6070) is the proof: it
  block-copies 11 dwords out of 0x2e4 into a local, block-copies them back under the
  `*(char *)(record + 6)` condition, and then copies the same local into 0x314 after setting
  `0x310 = 1`. 0x2e4 + 0x2c == 0x310 and 0x314 + 0x2c == 0x340 == the weapon object_size, so
  the tail is exactly two snapshots plus their valid bytes. `weapon_build_creation_message`
  names position (0x2e4), velocity (0x2f0), `rounds_unloaded[2]` (0x308) and `age` (0x30c)
  inside the snapshot.

### weapon_trigger_state (0x28, weapon 0x260 + i*0x28)

`weapon_update` is most of the proof; it is the only function that touches every field.

* **0x00 idle_ticks.** `if ((char)trigger->idle_ticks < 0x7f) ++`, and
  `weapon_trigger_ready_to_fire` compares `idle_ticks + 1.0` against
  `30.0 / lerp(maximum_rate_of_fire[0], [1], firing_rate)`.
* **0x01 effect_state / 0x02 effect_state_ticks.** Always written as a pair, by
  `weapon_trigger_effect_set_state` (0x4c49c0) or one of 0x4c3e40 (0/0), 0x4c3e70 (7/-1),
  0x4c48f0 (0/0), 0x4c3bc0 (3/random), 0x4c3c60 (1/random), 0x4c3d00 (6/random or 0/0),
  0x4c4b50 (8/0). `weapon_update` decrements the counter while nonzero and switches on the
  state; that switch is what named the nine states in the header. The two entry conditions come
  from `FUN_004c3280`: state 2 when `WeaponTrigger.charging_time` (0x48) is positive, state 1
  when `overload_time` (0xc4) is.
* **0x04 flags.** `weapon_update`: bit 0x01 is set on every tick the trigger is not pulled and
  cleared by the last statement of `weapon_fire_trigger`; bits 0x02 and 0x04 are the edge
  detector and latch used only when the tag trigger has flag 0x10; bit 0x10 tracks whether the
  blur permutation is applied; `FUN_004c3280` sets bit 0x20 when it starts a charging effect
  and `weapon_fire_trigger` tests it.
* **0x08 firing_effect_used_mask / 0x0a firing_effect_index / 0x0c firing_effect_rounds.**
  `weapon_fire_trigger`: it compares the ushort at 0x08 against
  `(1 << WeaponTrigger.firing_effects.count) - 1` and resets it to 0 when every effect has
  been used, advances the ushort at 0x0a past the already-used bits (randomizing the start when
  the tag trigger has flag 0x02), seeds the int16 at 0x0c from the chosen effect record's int16
  at +0x10, and decrements it per shot.
* **0x10 firing_rate.** `weapon_update` climbs it by `firing_acceleration_rate` (0xf8, clamped
  at 1.0) while pulled and falls by `firing_deceleration_rate` (0xfc); crossing
  `blurred_rate_of_fire` (0x14) swaps the permutation named at 0x006961b8;
  `weapon_trigger_ready_to_fire` interpolates the rate of fire with it; `FUN_004c3280` tests
  `> 0.0` and `FUN_004c3d00` zeroes it.
* **0x14 ejection_port_recovery / 0x18 illumination_recovery.** `weapon_fire_trigger` sets each
  to 1.0 when the corresponding tag time (0xa4 / 0xa8) is positive, and `weapon_update` decays
  them by the matching rates (0xf4 / 0xf0). `FUN_004c5580` independently confirms 0x14: it
  writes 1.0 into `object + 0x274` for `weapon_state == 3` and into `object + 0x29c` for
  `weapon_state == 4`, which is trigger 0 and trigger 1 at the same +0x14, and gates both on
  `ejection_port_recovery_time > 0` and the tag trigger flag bit 31.
* **0x1c error.** `weapon_update` climbs it by `error_acceleration_rate` (0x100) while the
  trigger is pulled or the effect state is 4 or 6, and falls by `error_deceleration_rate`
  (0x104); `trigger_create_projectiles` reads it for the spread.
* **0x20 effect_handle.** `FUN_004c3280` stores the `weapon_play_trigger_tag_effect` return
  here, and `weapon_update` stops it through `FUN_00450b20(1)` and sets -1.
* **0x24 empty_ticks.** Only the state-0 arm of `weapon_update`:
  `cVar = (char)trigger->empty_ticks + 1; store; if (cVar > 10) { ask the host to reload;
  empty_ticks = 0; }`.
* 0x0e and 0x25..0x27 are unresolved; see below.

### weapon_magazine_state (0x0c, weapon 0x2b0 + i*0x0c)

* **0x00 state / 0x02 state_ticks / 0x04 state_ticks_total.** `weapon_trigger_begin_reload`
  writes all three: `state = 1`, then `state_ticks = state_ticks_total =
  weapon_get_first_person_animation_time(0, i)`. `weapon_update` decrements `state_ticks` and
  dispatches on `state` (1 -> finish the reload through 0x4c3900/0x4c3a20, 2 -> start the
  chamber through 0x4c3b00, 3 with ticks 0 -> back to 0). `FUN_004c4b50` compares
  `state_ticks * 2` against a fresh animation length.
* **0x06 rounds_unloaded / 0x08 rounds_loaded.** `item_add_ammunition` adds to 0x2b6 and
  returns `0x2b0 + i*0x0c`, which is what fixes the stride. `weapon_set_ammo_counts` clamps
  0x2b6 with `WeaponMagazine.rounds_reserved_maximum` (tag +0x08) and then 0x2b8 with the
  reserve; `weapon_set_loaded_ammo_fraction` scales 0x2b8 by
  `rounds_loaded_maximum` (tag +0x0a) and moves the difference out of 0x2b6;
  `item_transfer_ammunition` moves rounds from a source item's 0x2b6 into the target's 0x2b6;
  `FUN_004c3900` / `FUN_004c3a20` move `rounds_reloaded` (tag +0x18) worth from 0x2b6 to 0x2b8,
  and zero 0x2b8 first when the tag magazine has flag 0x01.
* 0x0a is unresolved.

### equipment_data (object 0x22c .. 0x294)

`equipment_build_creation_message` (0x4bbc90) reads `0x245` and the nine floats at
`0x248/0x24c/0x250`, `0x254/0x258/0x25c`, `0x260/0x264/0x268`;
`equipment_create_from_creation_message` (0x4bbe20) writes the same nine, sets `0x244 = 1`,
`0x245` from the message and `0x246 = 0`, and then copies `0x254..0x25c` into `object->velocity`
(0x68) and `0x260..0x268` into `object->angular_velocity` (0x8c), which is what names the three
vectors. `equipment_build_network_update` (0x4bc0f0) reads `0x245` and `0x246`, points the
delta encoder at `0x248`, and post-increments `0x246` with a 0xff -> 0 wrap.

## Unresolved offsets

| struct | offset(s) | why it is stuck |
|---|---|---|
| `item_data` | 0x1fe (int16) | nothing in the module reads or writes it |
| `item_data` | flags bit 0x40 | only ever cleared (0x4bbb50, 0x4bcfc0); no setter and no test anywhere in the export |
| `garbage_data` | 0x22c..0x244 (all 0x18 bytes) | the garbage row only fills vtable +0x28 (0x4bc490) and +0x34 (0x4bc510) and Ghidra recovered neither as a function, so no code that touches garbage_data exists in `out/halo_decompiled.c` |
| `equipment_data` | 0x22c..0x244 (0x18 bytes) | same reason: 0x4bba90, 0x4bbae0, 0x4bbc30, 0x4bc070, 0x4bc250 and 0x4bc420 are all in the address range and all missed by Ghidra's function recovery. A scan of every function that calls `object_try_and_get(8)` found only 0x4bc0f0. |
| `equipment_data` | 0x26c..0x294 | laid out (valid byte + pad + a second `equipment_network_state`) purely by analogy with the weapon and projectile tails, both of which have that shape and both of which end exactly on their type's object_size the way this one does. Marked UNSURE in the header. The applier that would prove it, 0x4bc250, is one of Ghidra's misses. |
| `weapon_data` | 0x232, 0x239, 0x24c, 0x254, 0x258, 0x25e, 0x2c8, 0x2dc | no reader or writer in the whole export. A per-function scan for anything that references a weapon anchor (0x2b6/0x2b8/0x23c/0x4f0/0x4fc) together with one of these offsets came back empty. |
| `weapon_data` | 0x248 `ready_timer` | the decrement and the external write are both proved, the *meaning* is not; nothing reads the value in this module |
| `weapon_trigger_state` | 0x0e, 0x25..0x27 | untouched |
| `weapon_magazine_state` | 0x0a | untouched |
| `weapon_network_state` | 0x18..0x24 (12 bytes) | inside the 11-dword block copy in 0x4c6070, so it is definitely part of the record, but neither the creation message nor the update message carries it and nothing reads it by name. `equipment_network_state` has `angular_velocity` in the same slot, so that is the likely identity. |
| `weapon_flags` | bit 0x04 | set for a `Weapon.weapon_type == 3` secondary shot and consumed on overheat; the exact meaning of weapon_type 3 was not chased down |
| `weapon_trigger_effect_state` | state 4 | only `weapon_update` reads it, and only to drop to out-of-ammo or idle; no setter found |
| `object_type_definition` | nothing new | +0x0a/+0x0c/+0x0e/+0x10 are now resolved (above); the rest of the row is `types/objects.h` business |

## Misattributed functions

### Not items at all: 22 projectile functions

Ghidra's module seeding put one contiguous run, 0x4bbb50..0x4c62f0, in `items`, and the
evidence string in `modules.json` already admits it ("cea-pdb item_update, **projectile_detonate**,
trigger_create_projectiles"). Splitting the 101 functions by the mask each one passes to
`object_try_and_get` and by which tag offsets it reads gives:

| class | count | addresses |
|---|---|---|
| item (`_object_mask_item` 0x1c, Item tag) | 8 | 0x4bc5c0, 0x4bcf50, 0x4bcfc0, 0x4bd080, 0x4bd450, 0x4bd500, 0x4bd5d0, 0x4bd740 |
| equipment (mask 0x008, Equipment tag 0x31c) | 5 | 0x4bbb50, 0x4bbbd0, 0x4bbc90, 0x4bbe20, 0x4bc0f0 |
| **projectile (mask 0x020, Projectile tag)** | **22** | 0x4bda60, 0x4bdb40, 0x4bdc00, 0x4be1b0, 0x4beb30, 0x4bee20, 0x4beec0, 0x4bef80, 0x4bf0f0, 0x4bf120, 0x4bf1c0, 0x4bf390, 0x4c0180, 0x4c0250, 0x4c0310, 0x4c03f0, 0x4c0450, 0x4c0670, 0x4c0b10, 0x4c0ca0, 0x4c0f30, 0x4c1070 |
| weapon (mask 0x004, Weapon tag) | 63 | 0x4c12b0 .. 0x4c6070 |
| shell / main | 3 | 0x4c62d0, 0x4c62f0, 0x4c6340 |

What makes the projectile call unambiguous:

* `FUN_004bc0f0` calls `object_try_and_get(8)` (equipment) while `FUN_004c0f30` and
  `FUN_004c1070` call `object_try_and_get(0x20)` (projectile) and `FUN_004c5f10` calls
  `object_try_and_get(4)` (weapon). Those are the three copies of the same network-update
  routine, one per type.
* `item_update_function_values` (0x4c0250) reads four int16 at **tag + 0x184**. In the Item tag
  0x184 is `scale`; in the Projectile tag 0x184..0x18a is `projectile_a_in..d_in`, which is
  exactly what a function-value updater wants, and it writes the four floats at
  `object + 0x124` (`object.function_in_values`). It also divides by `tag + 0x1c8`
  (`Projectile.maximum_range`).
* `FUN_004beec0` tests `tag + 0x17c` bit 1 and reads `tag + 0x1cc` and `tag + 0x1e4`:
  `Projectile.air_gravity_scale` and `initial_velocity`. `FUN_004c03f0` divides by
  `tag + 0x1e4` / `tag + 0x1e8` (`initial_velocity` / `final_velocity`). `FUN_004bf1c0` reads
  the `timer[2]` pair at `tag + 0x1bc`/`0x1c0`. `FUN_004bf390` reads `tag + 0x230`
  (`impact_damage.tag_id`). None of those offsets exist in the Item tag (0x248 and 0x258 are
  `material_effects` and `collision_sound`, which is what `item_update` actually reads), and
  `tag + 0x254`/`0x264` do not exist in the Projectile tag (size 0x24c).
* `item_detonate` (0x4c0670) is the function the CEA hint list names `projectile_detonate`.

Because `projectile_data` is 0xbc bytes at object+0x1f4 it **overlaps `item_data`**, so folding
the projectile fields into `types/items.h` would produce a wrong struct. They are recorded here
instead, for whoever writes `types/projectiles.h`:

```
projectile_data, object 0x1f4 .. 0x2b0 (0xbc bytes, "projectile" row object_size 0x2b0)
  0x22c uint32  flags            bit 0x01 set/cleared by 0x4c0180 with the rotation axis;
                                 bit 0x02 tested by 0x4bdc00 (attachment teardown) and by
                                 0x4c0250 (function input 3); bits 0x08/0x20 gate the
                                 integrator in 0x4bdc00; bit 0x40 marks a fragment already
                                 scattered (0x4bf1c0, 0x4c0670); bit 0x80 set by 0x4bf1c0
                                 once six sibling fragments exist
  0x230 int16   high-water mark written by FUN_004bf0f0 (currently misnamed
                                 item_update_max_permutation_reached) and forced to >= 1 by
                                 0x4bdc00
  0x23c int32   attachment slot index; 0x4bdc00 and 0x4c0670 use it to reach
                                 object.attachment_handles (object 0x14c + index*4) and
                                 contrail_delete the handle there
  0x240 float   value, 0x244 float rate   (0x4bdc00 does 0x240 += 0x244; 0x4bf1c0 seeds
                                 0x244 = 1/(random_range(timer[0], timer[1]) * 30) and zeroes
                                 0x240 and 0x248 -- so 0x240 is the detonation progress)
  0x248 float   value, 0x24c float rate   (0x4bdc00 does 0x248 += 0x24c)
  0x250 float   distance travelled; 0x4c0250 divides it by Projectile.maximum_range
  0x254 float   value, 0x258 float rate   (0x4bdc00 does 0x254 += 0x258; 0x4c0310 seeds
                                 0x258 = air/water_damage_range[0] / initial_velocity)
  0x25c float   damage scale from FUN_004c03f0(air|water_damage_range[0..1])
  0x260 float   copied from Projectile.air|water_damage_range[1]
  0x264 real_vector3d rotation_axis, 0x270 float sine, 0x274 float cosine
                                 (FUN_004c0180, the exact analogue of item_compute_rotation)
  0x279 uint8   network_state_valid    (0x4c0ca0 sets 1)
  0x27a uint8   network_baseline_index
  0x27b uint8   network_sequence       (0x4c0f30 increments, wraps 0xff -> 0)
  0x27c real_point3d position, 0x288 real_vector3d velocity   (projectile_network_state, 0x18)
  0x294 uint8   last_update_valid      (0x4c1070 sets 1)
  0x298 projectile_network_state last_update_state   -> ends exactly at 0x2b0
message types: 0x1e creation, 0x30 (0x4bda60) and 0x33 (0x4bf120) notifications
```

`FUN_004be1b0`, currently named `resolution_list_add_resolution`, is the projectile motion and
collision integrator; the name looks inherited from an unrelated symbol and does not match the
code (it reads `Projectile.final_velocity`, `air_gravity_scale`, `water_gravity_scale` and
`maximum_range` and calls `projectile_detonate`).

### Not items: the 0x4c62d0 trio

`FUN_004c62d0` (32 bytes) is the real function: it copies four dwords out of
0x00696554..0x00696560 into 0x006b70a8.., stamps the window class name `"halo"` and `"(  "`,
seeds two -1 int16s, and then scans `argv` for `-console`. `weapon_prevents_grenade_throwing`
(0x4c62f0) and `weapon_get_first_person_animation_time` (0x4c6340) are *not functions* -- they
are mid-body labels of the same routine that Ghidra promoted, which is why both decompile as a
repeat of the `-console` scan with `in_ECX` / `unaff_ESI` inputs. All three belong in
`main`/`shell` (0x4c6390 next door is `chimera__exec_init`), and both inherited names must be
removed: the real owners are in this module (below).

### Renames this pass establishes

| address | current name | should be | evidence |
|---|---|---|---|
| 0x4c1530 | `item_update_triggers` | `weapon_update` | it is the **weapon** row's vtable +0x34, and the item row's +0x34 is 0x4bc5c0 (already `item_update`) |
| 0x4c2990 | `item_set_permutation` | `weapon_set_control_flags` | writes control_flags+primary_trigger; only caller is `unit_update`, which builds the word from `unit_control_flags`. Nothing about permutations. |
| 0x4c2f80 | `FUN_004c2f80` | `weapon_get_first_person_animation_time` | reads `Weapon.first_person_animations` (tag 0x478 = the TagID of 0x46c), walks the animation graph's 0xb4-stride animation block and returns a frame count; `weapon_ready` and `weapon_trigger_begin_reload` store the result as a tick countdown. The name currently sits on 0x4c6340. |
| 0x4c2f30 | `FUN_004c2f30` | `weapon_prevents_grenade_throwing` | returns `(Weapon.weapon_flags >> 6) & 1` -- bit 6 is literally `prevents_grenade_throwing` -- OR-ed with `weapon_state` in 5..10. The name currently sits on 0x4c62f0. |
| 0x4c2ee0 | `FUN_004c2ee0` | `weapon_prevents_melee_attack` | `(weapon_flags >> 9) & 1` = `prevents_melee_attack`, OR trigger 0 charging/charged |
| 0x4c2ea0 | `FUN_004c2ea0` | `weapon_must_be_readied` | `(weapon_flags >> 3) & 1` = `must_be_readied` |
| 0x4c24d0 | `FUN_004c24d0` | `weapon_get_label` | returns `tag + 0x30c` = `Weapon.label`, or the empty string at 0x0065512c |
| 0x4c12b0 | `FUN_004c12b0` | `weapon_trigger_get_average_damage` | `Weapon.triggers` -> `WeaponTrigger.projectile` -> `Projectile.impact_damage` and `attached_detonation_damage` -> the `DamageEffect` bounds at 0x1d4/0x1d8, averaged |
| 0x4c2d70 | `FUN_004c2d70` | `weapon_get_zoom_magnification` | `Weapon.zoom_levels` 0x3da and `zoom_magnification_range` 0x3dc/0x3e0 |
| 0x4c2cf0 | `FUN_004c2cf0` | `weapon_get_next_zoom_level` | same two tag fields, wraps against `zoom_levels - 1` |
| 0x4c2c70 | `FUN_004c2c70` | `weapon_is_out_of_ammo` | `age < 1.0` plus both magazine counters |
| 0x4c2ad0 | `FUN_004c2ad0` | `weapon_is_reloading` | `magazines[0].state == 1` |
| 0x4c3070 | `item_has_active_state` | `weapon_has_active_state` | reads both triggers' effect_state, both magazines' state and `weapon_data.state` |
| 0x4c30c0 | `FUN_004c30c0` | `weapon_triggers_idle` | the same three, minus the magazines |
| 0x4c2b20 | `FUN_004c2b20` | `weapon_set_ready_timer` | single store to weapon 0x248; UNSURE, inherits the uncertainty about that field |
| 0x4c25a0 | `item_add_ammunition` | `weapon_add_ammunition` | `object_try_and_get(4)` |
| 0x4c2610 | `item_transfer_ammunition` | `weapon_transfer_ammunition` | walks the Weapon tag magazine block |
| 0x4bf0f0 | `item_update_max_permutation_reached` | a projectile function | writes projectile 0x230 |
| 0x4c0250 | `item_update_function_values` | `projectile_update_function_values` | `Projectile.projectile_a_in..d_in` |
| 0x4c0670 | `item_detonate` | `projectile_detonate` | matches the CEA hint for the `"gravity"` string |
| 0x4c1070 | `item_apply_network_update` | `projectile_apply_network_update` | `object_try_and_get(0x20)`, projectile 0x279..0x2b0 |
| 0x4be1b0 | `resolution_list_add_resolution` | a projectile integrator | see above |
| 0x4c62f0, 0x4c6340 | `weapon_prevents_grenade_throwing`, `weapon_get_first_person_animation_time` | delete both; not functions | fragments of 0x4c62d0 |

Ghidra also missed 15 real function entries in and just below this address range, every one of
them a column of the item / weapon / equipment / garbage vtable rows: 0x4bba90, 0x4bbae0, 0x4bbc30,
0x4bc070, 0x4bc250, 0x4bc420, 0x4bc460, 0x4bc490, 0x4bc510, 0x4bc580, 0x4c1420, 0x4c2110,
0x4c59f0, 0x4c5e80, 0x4c6290. They are worth creating by hand before the clean-C pass, because
five of them are the only readers of the two unresolved blocks above.
