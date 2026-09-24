# units module: type recovery notes

Header: `types/units.h`. Smoke test: `out/phase4/units_smoke.c`, built with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/units_smoke.c` (passes; the
file carries negative-array size assertions for all seven structs, so a layout regression
fails the build rather than passing silently).

## The one piece of binary evidence everything hangs on

`object_type_definition` rows are read straight out of `.data` (PE image base 0x400000,
`.data` VA 0x676000 -> file 0x276000). The 12-pointer table at **0x0069bfdc** points at rows
whose `+0x04` is the *big-endian group tag* and whose `+0x08` is the runtime `object_size`.
Each row's `subdefinitions[]` at `+0x80` is the inheritance chain:

| row | name | group | object_size | chain |
|---|---|---|---|---|
| 0x0069b360 | object | `obje` | 0x1f4 | object |
| 0x0069b428 | unit | `unit` | **0x4cc** | object, unit |
| 0x0069b4f0 | biped | `bipd` | **0x550** | object, unit, biped |
| 0x0069b5b8 | vehicle | `vehi` | **0x5c0** | object, unit, vehicle |
| 0x0069b680 | item | `item` | 0x22c | object, item |
| 0x0069b748 | weapon | `weap` | 0x340 | object, item, weapon |

So `unit_data` is exactly 0x2d8 bytes at object+0x1f4, `biped_data` is 0x84 bytes at
object+0x4cc and `vehicle_data` is 0xf4 bytes at object+0x4cc. Every "size" comment in the
header is checked against these three numbers, not inferred from the highest offset seen.

Two side effects worth carrying back:

* **`types/objects.h` correction.** `object_type_definition.category` at +0x04 is documented
  there as "0 = delete immediately, 3 = delete recursively". The bytes say otherwise: it is
  the group tag stored big-endian (`bipd`, `vehi`, `weap`, `obje`, `unit`, `item`, ...).
  The `+0x08` `object_size` field is correct as documented.
* The vtable columns identify the per-type update entry points (see "Misattributed
  functions").

## Structs defined

| struct | size | where | fixed by |
|---|---|---|---|
| `unit_control_data` | 0x40 | passed in, and cached at unit 0x478 | 0x5639f0 block-moves 0x10 dwords into 0x478 and then unpacks each field; 0x55b110 reads 0x480 and 0x494..0x49c back out of that same copy |
| `unit_speech` | 0x30 | unit 0x388 and unit 0x3b8 | 0x560f20 block-moves 0xc dwords into either slot; 0x561030 builds one on the stack field by field |
| `unit_recent_damage` | 0x10 | unit 0x430, four of them | 0x568230 scans `iVar4 + 0x434` with `pfVar3 += 4` four times and indexes `sVar*0x10 + 0x430` |
| `unit_animation_overlay` | 0x04 | unit 0x2aa, three of them | 0x563b50 reads index/frame pairs at 0x2aa/0x2ac, 0x2ae/0x2b0, 0x2b2/0x2b4 |
| `unit_data` | 0x2d8 | object + 0x1f4 | the `unit` row's object_size 0x4cc |
| `biped_data` | 0x84 | object + 0x4cc | the `biped` row's object_size 0x550 |
| `vehicle_data` | 0xf4 | object + 0x4cc | the `vehicle` row's object_size 0x5c0 |

Enums: `unit_constants`, `unit_base_animation_state`, `unit_animation_state`, `unit_flags`,
`unit_control_flags`, `unit_animation_state_flags`, `unit_throwing_grenade_state`,
`unit_melee_state`.

## Which function established which field

### unit_control_data (0x40)

`0x5639f0` is the whole proof. As a server it does
`for (i=0x10; i; i--) *dst++ = *src++;` from the record into unit+0x478, then:

```
unit 0x2a6 = *(byte *)(rec + 0x00)      unit 0x288 = *(byte *)(rec + 0x01)
unit 0x208 = *(ushort *)(rec + 0x02)    unit 0x2f4 = *(short *)(rec + 0x04)  (if != -1)
unit 0x31d = *(byte *)(rec + 0x06)      unit 0x321 = *(byte *)(rec + 0x08)  (if 0x06 != -1)
unit 0x278/0x27c/0x280 = rec[3..5]      unit 0x284 = rec[6]
unit 0x224/0x228/0x22c = rec[7..9]      unit 0x230/0x234/0x238 = rec[10..12]
unit 0x254/0x258/0x25c = rec[13..15]
```

`0x55b110` independently confirms the cached copy: it writes `0x480 = (short)unit 0x321`
(= saved_control.zoom_level) and copies `0x494 -> 0x4ac`, `0x498 -> 0x4b0`, `0x49c -> 0x4b4`
(= facing_vector -> looking_vector inside the cached record).

### unit_data

* **0x1f4 / 0x1f8 actor handles.** `0x568610` clears both; it turns 0x1f8 into
  `(v & 0xffff) * 0x724 + *(int *)(0x00880360 + 0x34)`, i.e. the actor data_array, which is
  what makes it an actor handle and not an object handle. `0x569bf0` and `0x56bdc0` test
  both together ("has no AI and is not in a swarm"). 0x1fc is the only dword left before
  the flags word and is untouched here.
* **0x204 flags / 0x208 control flags.** 36 and 34 sites respectively; every named bit in
  the header cites its function. `unit_update` (0x5625b0) ORs `driver->control_flags & 0x3f`
  and `& 0x7c00` into its own, and shifts the word right by 13 for the grenade action.
* **0x20c** incremented once per tick by `unit_update` and reset when the unit wins the
  staggered slot arbitrated through the pointer at 0x006ef910.
* **0x210 / 0x214** written as a pair by `0x563b20`; `unit_update` decrements 0x210 and
  tests 0x214 for bit 0x800.
* **0x218 controlling_player.** `0x5590a0`, `0x55bea0`, `0x568230` all index the player
  data_array at 0x0087a480 (stride 0x200) with it.
* **0x21e emotion_animation_index.** `unit_scripting_set_emotion_animation` (0x569cf0)
  writes the `model_get_region_index_by_name` result here; `0x563b50` prefers it over the
  animation graph default.
* **0x224..0x284, the aiming block.** `unit_update` initialises it: `0x230 = 0x224`,
  `0x254 = 0x224`, `0x240 = 0x234`, `0x260 = 0x254`, and seeds 0x248 and 0x26c from
  `global_origin3d` (0x00696714). That pairing is what separates desired/current/velocity:
  0x224 desired facing, 0x230 desired aiming, 0x23c current aiming, 0x248 aiming velocity,
  0x254 desired looking, 0x260 current looking, 0x26c looking velocity. `0x5696f0` returns
  0x23c..0x244 and `unit_release_thrown_grenade` launches along it; `unit_update_facing`
  dots 0x224 against the object basis at 0x74.
* **0x289 melee_state.** set to 1 and 4 by `0x569a20`, cleared by `unit_cause_melee_damage`,
  `unit_melee_attack_scan` and `unit_can_see_point`, tested `== 3` by `0x55cfd0` and `== 4`
  by `0x56fc80`.
* **0x28d..0x294 grenade throw.** `unit_begin_throw_grenade` sets 0x28d = 1 and zeroes
  0x28e; `unit_throw_grenade_move_to_hand` sets 2 then 3 and stores the spawned projectile
  at 0x294; `unit_release_thrown_grenade` divides 0x28e by the int16 at 0x290 to get the
  throw fraction and clears 0x294.
* **0x2a0/0x2a1/0x2a2/0x2a7, the animation indices.** `unit_set_or_test_seat_and_weapon_label`
  (0x5651e0) walks the animation graph's unit block (tag data +0x0c count, +0x10 address,
  stride 100), its weapons at +0x5c (stride 0xbc) and their types at +0xb4 (stride 0x3c),
  and stores the three indices at 0x2a0, 0x2a1, 0x2a2. It then `__stricmp`s the seat label
  against the six-entry table at **0x0069fde4** and stores the match at 0x2a7. Reading that
  table out of `.data` gives the enum verbatim: `asleep, alert, stand, crouch, flee,
  flaming`. `0x56c2f0` indexes it to name the state, `0x56eb90` is the reverse lookup.
* **0x2b8 / 0x2c8, the aim and look bound boxes.** `0x563b50` fills four floats at each as
  `-(float)(int16) * float` / `+(float)(int16) * float` pairs taken from the graph weapon
  record and from the graph unit record at +0x20..+0x36, setting the 0x2b6 / 0x2b7 validity
  bytes; `0x5697a0` picks one box by a flag and clamps a world direction into it.
* **0x2e0 / 0x2e4.** `unit_calculate_luminosity` writes the 0.299/0.587/0.114 luma into
  0x2e0 and `object_sum_attached_light_luminance` into 0x2e4, or copies both from the
  parent object when attached.
* **0x2f0 vehicle_seat_index.** Everywhere it is multiplied by 0x11c against the Unit tag's
  seats block at tag+0x2e4/0x2e8 (`UnitSeat` in `types/tags.h` is 0x11c, which matches).
* **0x2f2/0x2f4/0x2f8/0x308, the inventory.** `unit_get_weapon_object_index` (0x569970)
  reads `0x2f8 + index*4`; `0x56d660` finds the first -1 over four slots;
  `unit_drop_inventory_weapons` walks all four; `0x56d400` zeroes `0x308 + slot*4` on
  pickup and `0x56dba0` picks the slot with the lowest 0x308 value. `unit_drop_current_weapon`
  clears `0x2f8 + 0x2f2*4` and sets 0x2f2 = -1.
* **0x318 equipment_object_index.** `0x56d1a0` attaches an object here (refusing when one is
  already held), `0x56d2c0` and `0x56d300` release it, both seat-teardown paths clear it.
* **0x31c/0x31d/0x31e grenades.** `0x56d160` adds a delta to `*(char *)(0x31e + type)` and
  records the type in both 0x31c and 0x31d; `unit_get_grenade_count` (0x56e030) reads
  `0x31e + CX`; `0x5699a0` cycles the type index wrapping at 1; `unit_drop_grenades` walks
  the pair; `0x55b110` restores both bytes as one int16 from biped 0x52c.
* **0x320/0x321 zoom.** `unit_update` compares them and acts on the difference; `0x55b110`
  writes the cached control record's zoom (0x480) from 0x321.
* **0x324/0x328 seat occupants.** `0x56ce30` recomputes both by scanning the child list;
  `unit_update` reads the occupant at 0x324 and copies its control flags and aiming vectors
  into itself, which is what makes it the driver rather than an arbitrary child.
* **0x384..0x400, dialogue.** `0x560d00` arbitrates a new line against the priority tables
  at 0x0065e94c / 0x0065e964 and walks the dialogue tag at 0x384 through the fallback chain
  at 0x0065e7a8; `0x560f20` commits by block-moving 0xc dwords into 0x388 (playing) or
  0x3b8 (queued), then seeds 0x3f8/0x3fc/0x3fe from the record's 0x390/0x392/0x394 and
  computes 0x3fa from the sound tag's length at +0x84; `0x561620` runs all four countdowns,
  starts the sound into 0x400 and sets the 0x3f4/0x3f5/0x3f6 latches.
* **0x42a..0x470, damage bookkeeping.** `0x568230`, in full.
* **0x474..0x4bc, the cached control record.** `0x5639f0` and `0x55b440`.

### biped_data

`biped_update` (0x5590a0), the two movement solvers (0x55bea0, 0x55cfd0) and the four
network columns (0x55aed0, 0x55b3d0, 0x55b440, 0x55b5f0) supply all of it.

* 0x4cc flags: `0x55ecf0` sets bit 0, `0x559fa0` sets bits 0|1, `0x55ad00` sets bit 5 and
  `0x55ad70` clears it, `0x55bea0`/`0x55cfd0` toggle bits 0 and 4.
* 0x4d2 movement_state: `biped_update` switches the animation state onto 0/1/2 and both
  `unit_update_facing` and `0x560410` branch on the result.
* 0x50c crouch_fraction: the movement solvers step it by the **Biped tag's** 0x4cc
  (`crouch_camera_velocity`), and `unit_get_camera_position` (0x568f80) and `0x55a2e0` blend
  Biped tag 0x424/0x428 (standing/crouching camera height) with it.
* 0x514 ground_normal and 0x520: written together by `0x560630`; `0x560800` uses the normal
  and the angle at 0x510 to level the up-vector.
* 0x524/0x525: `0x557a90` increments 0x524 and stops once it reaches 0x525; `0x55ad00`
  seeds 0x525 with 0x14.
* 0x527..0x54f: the network block. `0x55b440` sends a delta from 0x52c and bumps 0x528;
  `0x55b5f0` rejects a stale 0x527, applies 0x52c/0x530/0x534/0x538 and snapshots them into
  0x540/0x544/0x548/0x54c behind the 0x53c latch; `0x55b110` copies 0x52c into the unit's
  grenade counts, object 0xe0 from 0x530, object 0xe4 from `0x534 * 3` and object 0x104
  from `0x538 == 1`. That 0x52c..0x54f pair of 16-byte blocks plus the 0x53c latch lands
  exactly on 0x550, independently confirming the biped object size.

### vehicle_data

`vehicle_update` (0x570ee0), the reset column `0x570b00` (it zeroes 0x4cc, 0x4ce, 0x4d0..
0x4d3, then every dword from 0x4d4 to 0x4f8 and from 0x508 to 0x520) and
`unit_calculate_animation_controls` (0x5756f0, the vehicle row's +0x38 column) supply it.
0x5756f0 divides each field by a named Vehicle tag field, which is what names them:

```
0x4d4 / maximum_forward_speed 0x2f8, maximum_reverse_speed 0x2fc  -> forward_velocity
0x4d8 / maximum_left_slide 0x330, maximum_right_slide 0x334       -> sideways_velocity
0x4dc / maximum_left_turn 0x308, maximum_right_turn 0x30c         -> turning_velocity
0x4e0 / wheel_circumference 0x310                                 -> wheel_rotation
0x4e4, 0x4e8 / wheel_circumference 0x310                          -> left/right wheel
```

`0x572cd0` accumulates forward_velocity into 0x4e0 and wraps at 0x310 (single-axis steering);
`0x572b60` accumulates `forward -/+ turning` into 0x4e4 and 0x4e8 (dual axis).
`vehicle_update` compares `|0x4d4|` against `blur_speed` (tag 0x318) and sets flags bit 0.
`0x575170` reads and rewrites `*(byte *)(0x4f4 + i)` per physics mass point.
`0x575e30` treats 0x520 as a marker bitmask. `0x5ac` is stamped with the game tick by every
seat transition and rate-limits the vehicle network update.

## Unresolved offsets

**unit_data** — named `unknown_XX` in the header, listed here with what is known:

| offset | size | what is known |
|---|---|---|
| 0x1fc | 4 | never read by this module; the third handle of the actor/swarm triple |
| 0x200 | 4 | never read; the 0x200 accesses in `unit_drop_object_from_hand` and `0x56e820` are on the dropped *item* and on tag data respectively, not on the unit |
| 0x20e, 0x20f | 1+1 | 0x20f is -1 when unset and `0x565420` sign-extends it into an index |
| 0x210, 0x214 | 4+4 | both written by `0x563b20`; 0x210 counts down, 0x214 is a flags word ORed into control_flags |
| 0x21c | 2 | never read |
| 0x220 | 4 | never read |
| 0x28b, 0x28c | 1+1 | countdowns; 0x28b is seeded by the stun path |
| 0x292 | 2 | never read |
| 0x29c, 0x29e | 2+2 | 0x29c is -1 when unset and gates the seat/turret overlay |
| 0x2a4, 0x2a5, 0x2a9 | 1 each | 0x2a4 must be 0 for the aiming overlay; 0x2a5 is a high-water command value |
| 0x2d8 | 8 | untouched by the whole module |
| 0x2ec | 4 | untouched |
| 0x2f6 | 2 | untouched |
| 0x322, 0x323 | 1+1 | 0x322 is a tick counter clamped at 0x7f that the flee test compares against 120 |
| 0x334 | 2 | untouched; 0x336 next to it is copied from `UnitSeat + 0x3a` |
| 0x338, 0x33c | 4+4 | 0..1 scalars the vehicle lean/thruster code multiplies by; which gameplay quantity they are is not settled |
| 0x340, 0x344, 0x348, 0x37c, 0x380 | 4 each | five independent 0..1 ramps `unit_update` steps at 1/24, 1/900, 1/24, 1/120 and 1/90 |
| 0x34c, 0x358 | 12+12 | a cached point and its per-frame delta (`0x56e820`, `0x570cb0`) |
| 0x3e8..0x3ee | 2 each | four dialogue-adjacent countdowns |
| 0x3f0 | 4 | `0x560d00` hands it back to its caller and nothing else reads it |
| 0x404..0x428 | — | damage/stun bookkeeping written by `0x5674a0` and consumed by `unit_update`; the units are not pinned |
| 0x470 | 4 | never read |
| 0x4c0 | 12 | never read; the tail padding of the unit record |

Inside `unit_speech` (0x30) only 0x00..0x1b is ever touched; 0x1c..0x2f is zeroed on every
construction and never read back. `0x0e`, `0x10`, `0x14` and `0x18` are constructed as
0 / -1 / -1 / -1 and are otherwise opaque.

**biped_data** — 0x4d4, 0x4d0/0x4d1 (a frame counter and its limit), 0x4dc/0x4e0/0x4ec/0x4f0
(a look-at cache whose consumer is outside this module), 0x500..0x507 (eight independent
byte counters), 0x508/0x50a, 0x510, 0x520, 0x526, 0x52e, 0x542.

**vehicle_data** — the biggest gap. `0x570b00` zeroes 0x4cc..0x4f8 and 0x508..0x520 but
*not* 0x4fc..0x507, and nothing in this module reads those three dwords, so whether
`contact_point_traction` really runs to 0x508 is not proven. The header declares it as
`uint8_t contact_point_traction[20]` to keep the record byte-complete and flags the
assumption inline. 0x529..0x5ab (0x83 bytes) and 0x5b2..0x5bf (0xe bytes) are untouched by
every function in the module; the vehicle row's remaining columns (0x571f20, 0x572410,
0x5726e0, 0x572a30) would be the place to look.

**Overlap hazard.** `biped_data` and `vehicle_data` both start at 0x4cc, so the same
literal offset in the decompiler output means two different fields. Assignment was done by
asking which of the two update trees reaches the function. The three that stayed genuinely
ambiguous and were resolved by majority evidence rather than proof:

* **0x4d0..0x4d3** — read as bytes by both trees. Biped: `0x55eaa0`/`0x55eb90` (frame
  counter and limit), `biped_update` (movement_state at 0x4d2), `0x55bea0` (0x4d3 countdown
  reloaded with 0x3c). Vehicle: `vehicle_update` (0x4d1/0x4d2), `0x575640` (0x4d0 airborne
  and 0x4d3 landing counters), `0x5738b0`/`0x5739a0` (0x4d0).
* **0x508..0x520** — biped keeps a ground normal at 0x514 and a short at 0x508; the vehicle
  reset zeroes the same dwords. The header puts the ground normal in `biped_data` and
  leaves the vehicle side unknown except the 0x520 mask.
* **0x524..0x528** — the network sequence bytes 0x526/0x527/0x528 appear in both trees with
  the same meaning (`0x55b440` for bipeds, `0x5724d0` for vehicles), but 0x524/0x525 are the
  ground-adjust iteration pair on bipeds and something else on vehicles. Both spellings are
  in the header, one per struct.

A third hazard worth recording: **tag offsets collide with object offsets** throughout this
module's decompilation. `tag + 0x2f4` is `Biped.biped_flags`, `tag + 0x2e4` is
`Unit.seats`, `tag + 0x2f8` is `Vehicle.maximum_forward_speed`, `tag + 0x4cc` is
`Biped.crouch_camera_velocity` and `tag + 0x4e4/0x4e6/0x4e8` are the Biped pelvis/head node
indices and contact points -- all of which also look like plausible unit/biped object
offsets. Every field above was checked for which base the pointer came from before being
written down. Nothing in `types/tags.h` needed to change; every tag-side offset this module
touches already lands on an existing field.

## Misattributed functions

Fixed by the object_type_definition vtables:

* **0x5590a0 is `biped_update`, not `unit_update`.** It is the `biped` row's +0x34 column.
  The real per-tick unit update is the `unit` row's +0x34 column, **0x5625b0**, currently
  `FUN_005625b0`. Renaming these two is the single highest-value correction in the module.
* **0x5756f0 is the `vehicle` row's +0x38 column**, so its current name
  `unit_calculate_animation_controls` should be `vehicle_calculate_animation_controls`; it
  only ever reads `vehicle_data`.
* **0x570b00 is the `vehicle` row's +0x50 column** (the reset/initialise hook), not a
  generic "clears control state" helper.
* The `unit` row's other columns are **0x561fe0** (+0x14 initialize), **0x562020** (+0x1c
  reset), **0x562180** (+0x28), **0x563860** (+0x38), **0x56f0f0** (+0x3c), **0x56f1c0**
  (+0x40 region damage), **0x563b50** (+0x48), **0x5643f0** (+0x4c) and **0x561030** (+0x58).
  Five of those (0x561fe0, 0x562020, 0x562180, 0x563860, 0x5643f0, 0x56f0f0, 0x56f1c0) are
  **not in this module's function list** even though they are unit methods -- `modules.json`
  has them somewhere else. They should be pulled into `units`.
* The `biped` row's +0x38, +0x50 and +0x54 columns are **0x559e40, 0x559f10 and 0x559f70**,
  and `out/functions.json` has no function at any of those three addresses -- Ghidra never
  split them out of `biped_update`'s tail. 0x559f10 is the biped reset hook, i.e. the
  function that would have pinned `biped_data`'s initial values the way 0x570b00 pins the
  vehicle's. Worth forcing a function there.

Wrong pre-existing names found while reading (types unaffected, but the names mislead):

* **0x565a70 `unit_update`** — 50 bytes that duplicate the tail of `0x5659c0`; it clears the
  weapon-switch flags and the 0x348 timer. Nothing to do with updating a unit.
* **0x56c440 `unit_get_camera_position`** — conditionally calls `0x56c640` to detach the unit
  from its seat. The real camera-position function is **0x568f80** (also carrying that name).
* **0x565040 `unit_scripting_set_current_vitality`** — a cross-product/clamp/transform vector
  helper. Belongs in `math`, and its types were skipped.
* **0x5738b0 `unit_get_custom_animation_time`** — hovering-vehicle lift/turn physics. The real
  getter is **0x5701b0**.
* **0x5739a0 `unit_start_user_animation`** — the same hovering-vehicle physics, entered with a
  known target direction. The real one is **0x5702a0**.
* **0x571b40 `unit_throw_grenade_release`** — accumulates weighted per-marker forces for a
  vehicle; it reads `Vehicle.wheel_circumference` (tag 0x310) and `vehicle_data` 0x4f4.
  Grenade release is **0x56e440**.
* **0x55e83a `biped_new_from_network_unit_grenade_count_mod`** — one INT3 byte of padding.
* **0x55e9ff `biped_build_update_delta_unit_grenade_count_mod1`** — a real 153-byte impulse
  applier, but the entry address is mid-instruction and the name is unrelated.
* **0x569450 `unit_animation_set_state`** (1 byte) and **0x56d070 `unit_inventory_get_weapon`**
  (9 bytes) are stubs in this build.
* **0x569670** is a 118-byte no-op whose only remaining trace is its global references.

Functions in this module whose types belong elsewhere and were therefore skipped:

* **0x55efd0** (5157 bytes) — the generic object movement/collision slide solver. It happens
  to be reached from the biped movement path but operates on objects and BSP geometry, not
  on units. Belongs with physics/collision.
* **0x564580, 0x564840, 0x564990** — a bounded acceleration/velocity ramp solver and its
  evaluator, working entirely on a caller-supplied profile buffer. Pure math.
* **0x564ae0, 0x5579e0, 0x558860, 0x55eed0, 0x5658f0, 0x572a90** — vector/basis helpers
  (rotate toward, orthonormal rebuild, plane projection, blended clamp length). Pure math;
  `types/math.h` already covers what they touch.
* **0x561cb0, 0x561d50, 0x561e60, 0x561ab0** — generic datum-table linked-list walkers that
  take a callback. Objects/memory, not units.
* **0x5724d0** — the saved-film/replay transform recorder. It writes `vehicle_data` 0x524
  and 0x527, which is why the field is documented here, but the record it encodes belongs to
  the replay module.
* **0x56eb90** — a six-string table lookup against 0x0069fde4; it is the inverse of
  `unit_base_animation_state` and needs no type of its own.
