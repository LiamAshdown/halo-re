# `src/units` &mdash; the Blam unit layer

Retail Halo PC `halo.exe` 1.0.10, plain C, MSVC 7.1, x86. Module range
**0x5579e0 .. 0x575e30**, 249 functions, 114767 bytes.

This directory holds one `.c` file per rewritten function, named after the function it
contains. Every file follows the house style set by `src/memory`: a header comment giving the
address, the size, a name confidence, a rewrite confidence and the evidence behind both;
`extern` declarations carrying the address of each global and callee; the rewritten body; and
the original Ghidra decompilation verbatim inside a trailing `#if 0 / #endif` block so any
claim in the file can be checked against what the decompiler actually produced.

The types live in `types/units.h`. That header is ingested by Ghidra's CParser, so it carries
no `#include`, no include guards and no preprocessor beyond `#pragma pack(push, 1)`. Layout
regressions are caught by `out/phase4/units_smoke.c`, which carries 20 negative-array-size
assertions over the struct sizes and the individual field offsets this module depends on;
build it with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/units_smoke.c`.
The compile gate for the sources is `python tools/build_check.py units`.

## What the module contains

A *unit* is the object-hierarchy layer that sits between a plain object and a biped or a
vehicle: the thing that can be controlled, seated, armed and spoken to. Concretely the module
owns

* **the control-input path** &mdash; `unit_control_data` records pushed in by the player, the
  AI and the network layer, the cached server-side copy at unit 0x478, and the per-tick
  aiming / looking / facing servos that turn a desired direction into a current one;
* **movement** &mdash; the two biped integrators (`biped_integrate_movement` and
  `biped_integrate_movement_with_collision`), the turn-in-place solver
  (`biped_update_facing`), the up-vector leveller and the skeleton ground-adjust cluster that
  plants a biped's feet on uneven ground;
* **seating and inventory** &mdash; seat lookup and entry, the four-slot weapon inventory, the
  two grenade counts, the held-equipment handle;
* **the animation state machine** &mdash; the base animation state (`asleep`, `alert`,
  `stand`, `crouch`, `flee`, `flaming`), the overlay slots, the aiming and looking bound
  boxes taken from the animation graph, and the seat / weapon label matching that selects
  which graph rows apply;
* **dialogue** &mdash; the `unit_speech` queue, its four countdowns and the sound handle;
* **the network columns** &mdash; the create / update / delta encoders and their apply paths;
* **the two concrete extensions**, `biped_data` and `vehicle_data`, both of which start at
  object + 0x4cc.

### What is in this directory, and what is not

The module is **complete**: all 249 entries in `out/phase4/units_functions.md` are accounted
for &mdash; 231 rewritten as files here, 18 skipped because their types and behaviour belong to
another module (the full list, with the reason for each, is under "Misattributed functions").
One `.c` file per function, one function per file, no duplicate addresses; `tools/build_check.py
units` reports 231 ok / 0 failed and `out/phase4/units_smoke.c` still passes its layout
assertions.

What the directory does **not** contain is a working reimplementation. The confidence columns in
the function table at the bottom are the honest measure: 135 of the 231 files sit at rewrite
confidence 0.35 or below, and there are 1003 `UNSURE` annotations across the directory. Read the
files as annotated transcriptions with the right shape and the right memory writes, not as code
to compile into a game.

## The one piece of binary evidence everything hangs on

`object_type_definition` rows are read straight out of `.data`. The 12-pointer table at
**0x0069bfdc** points at rows whose `+0x04` is the big-endian group tag and whose `+0x08` is
the runtime object size:

| row | name | group | object_size | chain |
|---|---|---|---|---|
| `0x0069b360` | object | `obje` | `0x1f4` | object |
| `0x0069b428` | unit | `unit` | `0x4cc` | object, unit |
| `0x0069b4f0` | biped | `bipd` | `0x550` | object, unit, biped |
| `0x0069b5b8` | vehicle | `vehi` | `0x5c0` | object, unit, vehicle |

So `unit_data` is exactly 0x2d8 bytes at object + 0x1f4, `biped_data` is 0x84 bytes at
object + 0x4cc and `vehicle_data` is 0xf4 bytes at the same place. Every size in the header is
checked against those three numbers rather than inferred from the highest offset seen. The
same rows give each type's per-tick update column at `+0x34`, which is what settles the
misattributions below. The full derivation is in `out/phase4/units_types_notes.md`.

## Struct layouts

#### `unit_control_data` &mdash; 0x40 bytes

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x000` | `0x01` | `int8_t` | `animation_state` | -> unit 0x2a6, the seat / overlay command 0x565420 switches on |
| `0x001` | `0x01` | `int8_t` | `aiming_speed` | -> unit 0x288 |
| `0x002` | `0x02` | `uint16_t` | `control_flags` | -> unit 0x208, unit_control_flags |
| `0x004` | `0x02` | `int16_t` | `weapon_index` | -> unit 0x2f4 (desired weapon) when not -1 |
| `0x006` | `0x02` | `int16_t` | `grenade_index` | -> unit 0x31d (desired grenade) when not -1 |
| `0x008` | `0x02` | `int16_t` | `zoom_level` | -> unit 0x321; 0x55b110 reads it back at 0x480 |
| `0x00a` | `0x02` | `int16_t` | `unknown_0a` | never read by this module |
| `0x00c` | `0x0c` | `real_vector3d` | `throttle` | -> unit 0x278 |
| `0x018` | `0x04` | `float` | `primary_trigger` | -> unit 0x284 |
| `0x01c` | `0x0c` | `real_vector3d` | `facing_vector` | -> unit 0x224; 0x55b110 copies 0x494 to 0x4ac |
| `0x028` | `0x0c` | `real_vector3d` | `aiming_vector` | -> unit 0x230 |
| `0x034` | `0x0c` | `real_vector3d` | `looking_vector` | -> unit 0x254 |

#### `unit_speech` &mdash; 0x30 bytes

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x000` | `0x02` | `int16_t` | `priority` | compared against the priority table at 0x0065e94c |
| `0x002` | `0x02` | `int16_t` | `scream_type` | -1 in the 0x561030 construction |
| `0x004` | `0x04` | `datum_index` | `sound_tag` | handed to 0x00543ce0 to start the line |
| `0x008` | `0x02` | `int16_t` | `delay_ticks` | copied to the 0x3f8 countdown |
| `0x00a` | `0x02` | `int16_t` | `lipsync_ticks` | copied to the 0x3fc countdown |
| `0x00c` | `0x02` | `int16_t` | `tail_ticks` | copied to the 0x3fe countdown |
| `0x00e` | `0x02` | `int16_t` | `unknown_0e` |  |
| `0x010` | `0x04` | `int32_t` | `unknown_10` | -1 in the 0x561030 construction |
| `0x014` | `0x02` | `int16_t` | `unknown_14` | -1 |
| `0x016` | `0x02` | `int16_t` | `ai_line_index` | passed to ai_communication_record_line_played |
| `0x018` | `0x02` | `int16_t` | `unknown_18` | -1 |
| `0x01a` | `0x01` | `int8_t` | `suppress_line_record` | 0x561030 skips the line bookkeeping when set |
| `0x01b` | `0x01` | `int8_t` | `unknown_1b` |  |
| `0x01c` | `0x14` | `uint8_t[0x14]` | `unknown_1c` | zeroed by every construction, never read back |

#### `unit_recent_damage` &mdash; 0x10 bytes

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x000` | `0x04` | `int32_t` | `tick` | game tick the slot was last touched, -1 = empty |
| `0x004` | `0x04` | `float` | `damage` | accumulated damage from this source |
| `0x008` | `0x04` | `datum_index` | `responsible_unit` | matched first when merging into an existing slot |
| `0x00c` | `0x04` | `datum_index` | `responsible_player` | indexes the player data_array at 0x0087a480 |

#### `unit_animation_overlay` &mdash; 0x4 bytes

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x000` | `0x02` | `int16_t` | `animation_index` | -1 when the slot is free |
| `0x002` | `0x02` | `int16_t` | `frame` | the frame 0x004d4dd0 / 0x004d4f90 blends |

#### `biped_movement_solver_data` &mdash; 0xc8 bytes

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x000` | `0x04` | `datum_index` | `object_index` | the object being moved |
| `0x004` | `0x04` | `uint32_t` | `flags` | biped_movement_solver_flags; both integrators clear only the low 16 bits before filling it |
| `0x008` | `0x0c` | `real_point3d` | `start_position` | out: the position the solve started from; 0x55cfd0 diffs result_position against it to build the melee lunge ray |
| `0x014` | `0x0c` | `real_vector3d` | `facing` | object.forward, or unit_data.desired_facing_vector on the player-physics path |
| `0x020` | `0x0c` | `real_vector3d` | `aiming` | the unit_data.aiming_vector of the *live* object, or object.forward for a simple_creature Unit |
| `0x02c` | `0x0c` | `real_vector3d` | `velocity` | object.velocity on entry |
| `0x038` | `0x04` | `float` | `height_change` | how far the collision pill grew or shrank this tick; 0x55bea0 always passes 0 |
| `0x03c` | `0x0c` | `real_vector3d` | `movement_delta` | the per-tick displacement, from the frame_info of the animation or from the player physics block |
| `0x048` | `0x04` | `float` | `unknown_48` | 0 normally, 1.0 on the "frozen" shortcut |
| `0x04c` | `0x04` | `float` | `maximum_acceleration` | 0.0053333333 by default, FLT_MAX when the animation drives the velocity directly |
| `0x050` | `0x04` | `float` | `airborne_acceleration` | GlobalsPlayerInformation.airborne_acceleration / 30 |
| `0x054` | `0x04` | `float` | `pill_height` | first output of unit_get_crouch_height_offset |
| `0x058` | `0x04` | `float` | `pill_radius` | second output of it (Biped.collision_radius) |
| `0x05c` | `0x04` | `float` | `unknown_5c` | FLT_MAX, or 0.1 for a freshly grounded actor |
| `0x060` | `0x04` | `float` | `unknown_60` | 0, or 0.5 in that same case |
| `0x064` | `0x04` | `float` | `cosine_maximum_slope_angle` | Biped tag 0x4d0 |
| `0x068` | `0x04` | `float` | `negative_sine_downhill_falloff_angle` | Biped tag 0x4d4 |
| `0x06c` | `0x04` | `float` | `negative_sine_downhill_cutoff_angle` | Biped tag 0x4d8 |
| `0x070` | `0x04` | `float` | `downhill_velocity_scale` | Biped tag 0x364 |
| `0x074` | `0x04` | `float` | `sine_uphill_falloff_angle` | Biped tag 0x4dc |
| `0x078` | `0x04` | `float` | `sine_uphill_cutoff_angle` | Biped tag 0x4e0 |
| `0x07c` | `0x04` | `float` | `uphill_velocity_scale` | Biped tag 0x370 |
| `0x080` | `0x0c` | `real_vector3d` | `ground_normal` | biped_data.ground_normal on entry, rewritten on exit (the callers copy all four dwords back) |
| `0x08c` | `0x04` | `uint32_t` | `ground_plane` | biped_data.unknown_520 |
| `0x090` | `0x04` | `datum_index` | `ground_surface_index` | biped_data.ground_surface_index on entry |
| `0x094` | `0x04` | `uint32_t` | `unknown_94` | neither integrator touches it |
| `0x098` | `0x04` | `uint32_t` | `unknown_98` | neither integrator touches it |
| `0x09c` | `0x04` | `datum_index` | `result_surface_index` | out: -1 when the solve ended airborne; otherwise stored in biped_data.unknown_4d4 and the 0x4d3 countdown is reloaded with 60 |
| `0x0a0` | `0x01` | `uint8_t` | `result_flags` | out: biped_movement_solver_result_flags |
| `0x0a1` | `0x03` | `uint8_t[3]` | `unknown_a1` | alignment |
| `0x0a4` | `0x04` | `datum_index` | `result_ground_surface_index` | out: the new biped_data.ground_surface_index |
| `0x0a8` | `0x04` | `uint32_t` | `unknown_a8` |  |
| `0x0ac` | `0x0c` | `real_point3d` | `result_position` | out: the solved position |
| `0x0b8` | `0x0c` | `real_vector3d` | `result_velocity` | out: the solved velocity |
| `0x0c4` | `0x04` | `float` | `result_impact_speed` | out: the landing speed the fall-damage and footstep-effect paths consume |

#### `unit_data` &mdash; 0x2d8 bytes, at object + 0x1f4

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x1f4` | `0x04` | `datum_index` | `actor_index` | -1 when the unit has no AI; 0x569bf0 and 0x55e2d0 test it, 0x568610 clears it |
| `0x1f8` | `0x04` | `datum_index` | `swarm_actor_index` | 0x568610 turns it into (index & 0xffff) * 0x724 against the actor data_array at 0x00880360, which is what fixes it as an actor handle rather than an object one |
| `0x1fc` | `0x04` | `datum_index` | `swarm_next_unit_index` | never read in this module; the only dword left between the two actor handles and the flags word |
| `0x200` | `0x04` | `uint32_t` | `unknown_200` | untouched here (the 0x200 accesses in the melee and drop paths are on the *item* being dropped, not on the unit) |
| `0x204` | `0x04` | `uint32_t` | `flags` | unit_flags |
| `0x208` | `0x04` | `uint32_t` | `control_flags` | unit_control_flags, zero-extended from unit_control_data.control_flags |
| `0x20c` | `0x02` | `int16_t` | `update_tick_counter` | unit_update increments it every tick and resets it when this unit wins the staggered expensive-update slot |
| `0x20e` | `0x01` | `int8_t` | `unknown_20e` |  |
| `0x20f` | `0x01` | `int8_t` | `unknown_20f` | -1 when unset; 0x565420 sign-extends it into a seat / animation index |
| `0x210` | `0x04` | `int32_t` | `unknown_210` | set by 0x563b20, counted down by unit_update |
| `0x214` | `0x04` | `uint32_t` | `unknown_214` | set by 0x563b20; ORed into control_flags and tested for bit 0x800 by unit_update |
| `0x218` | `0x04` | `datum_index` | `controlling_player` | indexes the player data_array at 0x0087a480 (stride 0x200, unit handle at +0x34) |
| `0x21c` | `0x02` | `int16_t` | `unknown_21c` |  |
| `0x21e` | `0x02` | `int16_t` | `emotion_animation_index` | unit_scripting_set_emotion_animation writes the model region index it looked up here; 0x563b50 prefers it over the graph default |
| `0x220` | `0x04` | `uint32_t` | `unknown_220` |  |
| `0x224` | `0x0c` | `real_vector3d` | `desired_facing_vector` | unit_control_data.facing_vector |
| `0x230` | `0x0c` | `real_vector3d` | `desired_aiming_vector` | unit_control_data.aiming_vector |
| `0x23c` | `0x0c` | `real_vector3d` | `aiming_vector` | the current aim; 0x5696f0 returns it and unit_release_thrown_grenade launches along it |
| `0x248` | `0x0c` | `real_vector3d` | `aiming_velocity` | seeded from global_origin3d by unit_update |
| `0x254` | `0x0c` | `real_vector3d` | `desired_looking_vector` | unit_control_data.looking_vector |
| `0x260` | `0x0c` | `real_vector3d` | `looking_vector` | 0x56bc80 and 0x56c100 cone-test against it |
| `0x26c` | `0x0c` | `real_vector3d` | `looking_velocity` | seeded from global_origin3d by unit_update |
| `0x278` | `0x0c` | `real_vector3d` | `throttle` | unit_control_data.throttle |
| `0x284` | `0x04` | `float` | `primary_trigger` | unit_control_data.primary_trigger |
| `0x288` | `0x01` | `int8_t` | `aiming_speed` | unit_control_data.aiming_speed |
| `0x289` | `0x01` | `int8_t` | `melee_state` | unit_melee_state |
| `0x28a` | `0x01` | `int8_t` | `melee_damage_countdown` | 0x56fc80 and 0x56fd40 reload it with 10 and tick it down between melee damage pulses |
| `0x28b` | `0x01` | `int8_t` | `unknown_28b` | countdown; 0x5705a0 seeds it with a random stun duration, unit_update decrements it |
| `0x28c` | `0x01` | `int8_t` | `unknown_28c` | countdown decremented by unit_update; both seat-teardown paths require it to be 0 |
| `0x28d` | `0x01` | `int8_t` | `throwing_grenade_state` | unit_throwing_grenade_state |
| `0x28e` | `0x02` | `int16_t` | `throwing_grenade_counter` | unit_begin_throw_grenade zeroes it, unit_update increments it |
| `0x290` | `0x02` | `int16_t` | `throwing_grenade_duration` | unit_release_thrown_grenade divides the counter by it to get the throw fraction |
| `0x292` | `0x02` | `int16_t` | `unknown_292` |  |
| `0x294` | `0x04` | `datum_index` | `throwing_grenade_projectile` | the grenade object attached to the hand, -1 once released |
| `0x298` | `0x02` | `uint16_t` | `animation_state_flags` | unit_animation_state_flags |
| `0x29a` | `0x02` | `int16_t` | `animation_instance` | the animation instance unit_try_set_animation_state allocated, -1 when none; 0x563b50 needs it before it will blend the aiming overlay |
| `0x29c` | `0x02` | `int16_t` | `unknown_29c` | -1 when unset; 0x563b50 requires it for the seat / turret overlay |
| `0x29e` | `0x02` | `int16_t` | `unknown_29e` |  |
| `0x2a0` | `0x01` | `int8_t` | `animation_definition_index` | index into the unit block of the animation graph (tag data + 0x0c count, + 0x10 address, stride 100); -1 when the unit has none |
| `0x2a1` | `0x01` | `int8_t` | `animation_weapon_index` | index into the weapons of that block at +0x5c, stride 0xbc |
| `0x2a2` | `0x01` | `int8_t` | `animation_weapon_type_index` | index into the types of that weapon at +0xb4, stride 0x3c |
| `0x2a3` | `0x01` | `int8_t` | `animation_state` | unit_animation_state |
| `0x2a4` | `0x01` | `int8_t` | `unknown_2a4` | 0x563b50 refuses the aiming overlay unless this is 0; 0x565420 clears it |
| `0x2a5` | `0x01` | `int8_t` | `unknown_2a5` | 0x566410 raises it to the command it started |
| `0x2a6` | `0x01` | `int8_t` | `seat_command` | unit_control_data.animation_state |
| `0x2a7` | `0x01` | `int8_t` | `base_animation_state` | unit_base_animation_state |
| `0x2a8` | `0x01` | `int8_t` | `emotion_animation_frame` | -1 when idle; 0x563b50 plays the emotion animation of the graph at this frame |
| `0x2a9` | `0x01` | `int8_t` | `unknown_2a9` | alignment; never read |
| `0x2aa` | `0x0c` | `unit_animation_overlay[3]` | `overlays` | 0x2ae 0x2b2, index then frame in each pair |
| `0x2b6` | `0x01` | `int8_t` | `aiming_bounds_valid` | 0x563b50 sets it after filling aiming_bounds |
| `0x2b7` | `0x01` | `int8_t` | `looking_bounds_valid` | 0x563b50 sets it after filling looking_bounds |
| `0x2b8` | `0x10` | `float[4]` | `aiming_bounds` | -yaw, +yaw, -pitch, +pitch, each an int16 frame count from the graph times its float scale; 0x5697a0 clamps a direction into this box |
| `0x2c8` | `0x10` | `float[4]` | `looking_bounds` | the same four for looking, taken from the graph unit block at +0x20..+0x36 |
| `0x2d8` | `0x08` | `uint8_t[8]` | `unknown_2d8` | untouched by this module |
| `0x2e0` | `0x04` | `float` | `illumination` | unit_calculate_luminosity: the 0.299 / 0.587 / 0.114 luma of the sampled lighting, or the value of the parent when attached |
| `0x2e4` | `0x04` | `float` | `attached_light_luminosity` | object_sum_attached_light_luminance result |
| `0x2e8` | `0x04` | `float` | `animation_blend_weight` | 0x563b50 blends animation 10 of the graph unit block by it; unit_update decays it toward 0 each tick |
| `0x2ec` | `0x04` | `uint32_t` | `unknown_2ec` |  |
| `0x2f0` | `0x02` | `int16_t` | `vehicle_seat_index` | index into the Unit tag seats block of the parent (stride 0x11c), -1 when not seated |
| `0x2f2` | `0x02` | `int16_t` | `current_weapon_index` | slot in weapons[], -1 when unarmed |
| `0x2f4` | `0x02` | `int16_t` | `desired_weapon_index` | unit_control_data.weapon_index; unit_ready_desired_weapon consumes it |
| `0x2f6` | `0x02` | `int16_t` | `unknown_2f6` |  |
| `0x2f8` | `0x10` | `datum_index[4]` | `weapons` | the inventory; 0x56d660 returns the first -1 |
| `0x308` | `0x10` | `int32_t[4]` | `weapon_ready_ticks` | zeroed when a weapon is picked up; 0x56dba0 picks the lowest when choosing a replacement |
| `0x318` | `0x04` | `datum_index` | `equipment_object_index` | the object currently held in the hand (0x56d1a0 attaches it, 0x56d2c0 and 0x56d300 release it), -1 when empty |
| `0x31c` | `0x01` | `int8_t` | `current_grenade_index` | unit_get_current_grenade_index returns it |
| `0x31d` | `0x01` | `int8_t` | `desired_grenade_index` | unit_control_data.grenade_index |
| `0x31e` | `0x02` | `int8_t[2]` | `grenade_counts` | unit_get_grenade_count indexes it; 0x55b110 restores both bytes at once from 0x52c |
| `0x320` | `0x01` | `int8_t` | `zoom_level` | -1 when not zoomed |
| `0x321` | `0x01` | `int8_t` | `desired_zoom_level` | unit_control_data.zoom_level; 0x5659c0 and 0x565a70 force it back to -1 |
| `0x322` | `0x01` | `int8_t` | `unknown_322` | tick counter clamped at 0x7f, reset by unit_update; 0x55e2d0 flees above 120 |
| `0x323` | `0x01` | `int8_t` | `aiming_change` | written by unit_update from the same block |
| `0x324` | `0x04` | `datum_index` | `driver_unit_index` | the child object in the first tracked seat; 0x56ce30 recomputes it and unit_update copies the control input of this occupant into itself |
| `0x328` | `0x04` | `datum_index` | `gunner_unit_index` | the child object in the second tracked seat |
| `0x32c` | `0x04` | `datum_index` | `last_parent_object_index` | the object this unit was last seated in, recorded by every detach path |
| `0x330` | `0x04` | `int32_t` | `last_seat_change_tick` | game time at that detach |
| `0x334` | `0x02` | `int16_t` | `unknown_334` |  |
| `0x336` | `0x02` | `int16_t` | `unknown_336` | copied from the UnitSeat record at +0x3a when the unit leaves the seat (0x568610, 0x568cb0) |
| `0x338` | `0x04` | `float` | `driver_seat_power` | a 0..1 scalar the vehicle lean, thruster and ground-effect routines all multiply by |
| `0x33c` | `0x04` | `float` | `gunner_seat_power` | vehicle_update tests it against 0 with bit 8 |
| `0x340` | `0x04` | `float` | `integrated_light_power` | 0..1 ramp, unit_update steps it by 1/24 down and 1/6 up |
| `0x344` | `0x04` | `float` | `unknown_344` | 0..1; unit_update steps it by 1/900 up and 1/3600 down; packed into the network update |
| `0x348` | `0x04` | `float` | `flashlight_ramp` | 0..1 ramp, 1/24 down and 1/12 up; zeroed by 0x5659c0 and 0x565a70 |
| `0x34c` | `0x0c` | `real_point3d` | `unknown_34c` | cached look reference point; 0x56e820 diffs it frame to frame and 0x570cb0 shifts it by the movement delta of the parent |
| `0x358` | `0x0c` | `real_vector3d` | `unknown_358` | the delta of that point on the previous frame |
| `0x364` | `0x0c` | `float[3]` | `animation_controls_smoothed` | unit_update runs 0.7 * old + 0.3 * new; 0x563b50 drives three graph animations by them |
| `0x370` | `0x0c` | `float[3]` | `animation_controls` | the raw 0..1 values 0x56e820 computes |
| `0x37c` | `0x04` | `float` | `unknown_37c` | 0..1, stepped by 1/120 in unit_update and reduced by damage in 0x5674a0 |
| `0x380` | `0x04` | `float` | `unknown_380` | 0..1, stepped by 1/90 in unit_update |
| `0x384` | `0x04` | `datum_index` | `dialogue_tag_index` | the unit_dialogue tag 0x560d00 walks (records of stride 0x10 at tag data + 0x1c) |
| `0x388` | `0x30` | `unit_speech` | `current_speech` | the line being played |
| `0x3b8` | `0x30` | `unit_speech` | `pending_speech` | the line queued behind it; 0x561620 promotes it through 0x560f20 when the current one ends |
| `0x3e8` | `0x02` | `int16_t` | `unknown_3e8` | countdown reloaded with 0x16 by 0x561620 |
| `0x3ea` | `0x02` | `int16_t` | `unknown_3ea` | decremented each time 0x3e8 expires |
| `0x3ec` | `0x02` | `int16_t` | `unknown_3ec` | countdown, 0x561140 reloads it with 0x1e |
| `0x3ee` | `0x02` | `int16_t` | `unknown_3ee` | countdown, 0x561140 reloads it with 0x3c |
| `0x3f0` | `0x04` | `uint32_t` | `unknown_3f0` | 0x560d00 returns it to its caller unchanged |
| `0x3f4` | `0x01` | `int8_t` | `speech_started` | 0x561620 sets it once the sound was started |
| `0x3f5` | `0x01` | `int8_t` | `speech_lipsync_stopped` | set once the lipsync countdown hit 0 |
| `0x3f6` | `0x01` | `int8_t` | `speech_finished` | set once the duration countdown hit 0 |
| `0x3f7` | `0x01` | `int8_t` | `unknown_3f7` | alignment |
| `0x3f8` | `0x02` | `int16_t` | `speech_delay_ticks` | loaded from unit_speech.delay_ticks |
| `0x3fa` | `0x02` | `int16_t` | `speech_duration_ticks` | 0x560f20 computes it from the length of the sound tag at +0x84 (times 30, divided by 1000), or 0x2d when there is no sound |
| `0x3fc` | `0x02` | `int16_t` | `speech_lipsync_ticks` | loaded from unit_speech.lipsync_ticks |
| `0x3fe` | `0x02` | `int16_t` | `speech_tail_ticks` | loaded from unit_speech.tail_ticks |
| `0x400` | `0x04` | `datum_index` | `speech_sound_handle` | the handle 0x00543ce0 returned, -1 when idle |
| `0x404` | `0x02` | `int16_t` | `unknown_404` | passed as the second argument of 0x0042be40 |
| `0x406` | `0x02` | `int16_t` | `unknown_406` | countdown; 0x5674a0 reloads it with 0x2d |
| `0x408` | `0x04` | `float` | `unknown_408` | damage accumulator, raised by 0x5674a0 and consumed by unit_update |
| `0x40c` | `0x04` | `datum_index` | `unknown_40c` | the object 0x5674a0 recorded as responsible |
| `0x410` | `0x04` | `int32_t` | `unknown_410` | stored by 0x5705a0, read back by 0x570720 |
| `0x414` | `0x04` | `float` | `idle_turn_angle` | 0x570650 seeds it from the current heading plus a random offset, 0x570840 wanders it |
| `0x418` | `0x04` | `float` | `idle_turn_offset` | the second, tighter angle of the wander |
| `0x41c` | `0x04` | `int32_t` | `unknown_41c` | game tick stamp taken by 0x562030 and by both seat-teardown paths |
| `0x420` | `0x02` | `int16_t` | `unknown_420` | countdown, unit_update fires on the 0 edge |
| `0x422` | `0x02` | `int16_t` | `unknown_422` | unit_update checks it against the network predicted-state flag |
| `0x424` | `0x04` | `float` | `stun_amount` | 0..1 stun meter; 0x5674a0 raises it and unit_update and the movement solvers scale velocity by 1 - stun_movement_penalty * this. Confirmed: both integrators ... |
| `0x428` | `0x02` | `int16_t` | `unknown_428` | countdown, raised by 0x5674a0 |
| `0x42a` | `0x02` | `int16_t` | `ai_communication_count` | 0x568230 counts hits and broadcasts once the count reaches 3 (5 for a player) |
| `0x42c` | `0x04` | `int32_t` | `ai_communication_tick` | tick of the last hit; the count resets after 0x78 ticks |
| `0x430` | `0x40` | `unit_recent_damage[4]` | `recent_damage` | the four-slot damage cache |
| `0x470` | `0x04` | `uint32_t` | `unknown_470` |  |
| `0x474` | `0x01` | `int8_t` | `unknown_474` | set when the incoming control word carried bits 0x2800, cleared once the delta is sent |
| `0x475` | `0x01` | `int8_t` | `unknown_475` | set by the network create and update paths and by the scripted spawn |
| `0x476` | `0x02` | `int8_t[2]` | `unknown_476` | alignment |
| `0x478` | `0x40` | `unit_control_data` | `saved_control` | the server-side copy 0x5639f0 block-moves |
| `0x4b8` | `0x01` | `int8_t` | `unknown_4b8` | 1 when unknown_4bc holds a valid value |
| `0x4b9` | `0x03` | `int8_t[3]` | `unknown_4b9` | alignment |
| `0x4bc` | `0x04` | `int32_t` | `unknown_4bc` | the source identifier of the control record; unit_update reads it back |
| `0x4c0` | `0x0c` | `uint8_t[12]` | `unknown_4c0` | untouched by this module |

#### `biped_data` &mdash; 0x84 bytes, at object + 0x4cc

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x4cc` | `0x04` | `uint32_t` | `flags` | bit 0 = grounded (0x55ecf0 sets it, 0x560800 and 0x569b30 test it), bit 1 = jumping (0x559fa0 sets 0 and 1 together), bit 4 = the 0x55bea0 landing latch, bit... |
| `0x4d0` | `0x01` | `int8_t` | `unknown_4d0` | frame counter 0x55eb90 advances |
| `0x4d1` | `0x01` | `int8_t` | `unknown_4d1` | the frame count it is compared against, loaded by 0x55eaa0 |
| `0x4d2` | `0x01` | `int8_t` | `movement_state` | biped_update maps the animation state onto 0 (standing), 1 (moving) or 2 (other); unit_update_facing and 0x560410 branch on it |
| `0x4d3` | `0x01` | `int8_t` | `unknown_4d3` | countdown reloaded with 0x3c (60 ticks) by the movement solvers every tick that last_ground_surface_index is refreshed |
| `0x4d4` | `0x04` | `datum_index` | `last_ground_surface_index` | the supporting surface the movement solver last reported; both integrators (0x55bea0 and 0x55cfd0) store it and reload the 0x4d3 countdown with 60, or clear ... |
| `0x4d8` | `0x04` | `datum_index` | `ground_surface_index` | the supporting surface 0x560630 found, -1 when airborne; 0x560800 refuses to level the up-vector without it |
| `0x4dc` | `0x04` | `datum_index` | `cached_surface_index` | -1 when unset; 0x55ab30 uses it as the cached look-at result |
| `0x4e0` | `0x0c` | `real_point3d` | `cached_position` | the cached look-at point 0x55ab30 refreshes |
| `0x4ec` | `0x04` | `int32_t` | `cached_tick` | game tick that cache was last refreshed |
| `0x4f0` | `0x04` | `datum_index` | `previous_cached_surface_index` | the previous value of cached_surface_index |
| `0x4f4` | `0x04` | `datum_index` | `melee_target_index` | the object 0x55cfd0 hands to 0x56ff40 when melee_state is 3 |
| `0x4f8` | `0x04` | `int32_t` | `last_flee_reaction_tick` | tick stamp; 0x55e190 and 0x55e2d0 rate-limit their reactions to once every 15 ticks |
| `0x4fc` | `0x04` | `datum_index` | `unknown_4fc` | the target 0x55e0a0 is tracking |
| `0x500` | `0x01` | `int8_t` | `unknown_500` | how many ticks that target has been held; 0x55e0a0 saturates it at 0xf1 |
| `0x501` | `0x01` | `int8_t` | `unknown_501` | ticks in the current grounded state, clamped at 0x7f by biped_update |
| `0x502` | `0x01` | `int8_t` | `unknown_502` | the same counter for the second flag bit |
| `0x503` | `0x01` | `int8_t` | `unknown_503` | latch 0x560410 toggles at the seat angle limit |
| `0x504` | `0x01` | `int8_t` | `unknown_504` | ticks without a target lock (0x55ec90) |
| `0x505` | `0x01` | `int8_t` | `unknown_505` | biped_update decays it by a quarter each tick |
| `0x506` | `0x01` | `int8_t` | `unknown_506` | the value unknown_505 is compared against |
| `0x507` | `0x01` | `int8_t` | `unknown_507` | alignment |
| `0x508` | `0x02` | `int16_t` | `unknown_508` | 0x55eaa0 stores a 0/1 comparison result here and 0x55eb90 turns it into a trigger id |
| `0x50a` | `0x02` | `int16_t` | `unknown_50a` |  |
| `0x50c` | `0x04` | `float` | `crouch_fraction` | 0..1; the movement solvers step it by the crouch_camera_velocity of the Biped tag (0x4cc), unit_get_camera_position and 0x55a2e0 blend the standing and crouc... |
| `0x510` | `0x04` | `float` | `bank_angle` | angle; 0x560800 takes its cos and sin |
| `0x514` | `0x0c` | `real_vector3d` | `ground_normal` | the supporting plane normal 0x560630 caches |
| `0x520` | `0x04` | `uint32_t` | `unknown_520` | written by 0x560630 alongside the normal |
| `0x524` | `0x01` | `uint8_t` | `ground_adjust_iteration` | 0x557a90 increments it up to 0x7f |
| `0x525` | `0x01` | `uint8_t` | `ground_adjust_iteration_limit` | 0x55ad00 seeds it with 0x14; the solver stops once the iteration reaches it |
| `0x526` | `0x01` | `uint8_t` | `unknown_526` | set by the network create and scripted spawn |
| `0x527` | `0x01` | `uint8_t` | `network_update_sequence` | 0x55b5f0 rejects an update whose sequence is behind this one |
| `0x528` | `0x01` | `uint8_t` | `network_delta_sequence` | 0x55b440 increments it per delta sent and wraps it at 0xff |
| `0x529` | `0x03` | `uint8_t[3]` | `unknown_529` | alignment |
| `0x52c` | `0x02` | `int16_t` | `network_grenade_counts` | both grenade counts as one int16; 0x55b110 copies it into unit_data.grenade_counts |
| `0x52e` | `0x02` | `int16_t` | `unknown_52e` |  |
| `0x530` | `0x04` | `float` | `network_body_vitality` | 0x55b110 copies it into object 0xe0 |
| `0x534` | `0x04` | `float` | `network_shield_vitality` | 0x55b110 writes object 0xe4 as this times 3 |
| `0x538` | `0x01` | `int8_t` | `network_shield_stunned` | 0x55b110 turns it into object 0x104 |
| `0x539` | `0x03` | `int8_t[3]` | `unknown_539` | alignment |
| `0x53c` | `0x01` | `int8_t` | `network_baseline_valid` | 0x55b5f0 sets it when it snapshots the block |
| `0x53d` | `0x03` | `int8_t[3]` | `unknown_53d` | alignment |
| `0x540` | `0x02` | `int16_t` | `baseline_grenade_counts` | the snapshot 0x55b5f0 keeps of 0x52c |
| `0x542` | `0x02` | `int16_t` | `unknown_542` |  |
| `0x544` | `0x04` | `float` | `baseline_body_vitality` | snapshot of 0x530 |
| `0x548` | `0x04` | `float` | `baseline_shield_vitality` | snapshot of 0x534 |
| `0x54c` | `0x01` | `int8_t` | `baseline_shield_stunned` | snapshot of 0x538 |
| `0x54d` | `0x03` | `int8_t[3]` | `unknown_54d` | alignment |

#### `vehicle_data` &mdash; 0xf4 bytes, at object + 0x4cc

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x4cc` | `0x02` | `uint16_t` | `flags` | bit 0 = over the blur_speed of the Vehicle tag (0x318), bit 2 = has ground contact, bit 3 = hovering, bit 4 = controls were active this tick; vehicle_update ... |
| `0x4ce` | `0x02` | `int16_t` | `unknown_4ce` | vehicle_update reloads it with 0xf while the controls move; unit_update_recoil_decay counts it down and fires on the 0 edge |
| `0x4d0` | `0x01` | `uint8_t` | `airborne_ticks` | 0x575640 increments it while off the ground and 0x5756f0 folds it into the blend weight |
| `0x4d1` | `0x01` | `uint8_t` | `unknown_4d1` | vehicle_update clears it with unknown_4d2 |
| `0x4d2` | `0x01` | `uint8_t` | `unknown_4d2` | counter vehicle_update raises while 0x4d1 is 1 or 2 and clears past 0x1d |
| `0x4d3` | `0x01` | `uint8_t` | `landing_ticks` | 0x575640 bumps it when ground contact resumes |
| `0x4d4` | `0x04` | `float` | `forward_velocity` | divided by the Vehicle tag field maximum_forward_speed (0x2f8) or maximum_reverse_speed (0x2fc) |
| `0x4d8` | `0x04` | `float` | `sideways_velocity` | divided by maximum_left_slide (0x330) or maximum_right_slide (0x334) |
| `0x4dc` | `0x04` | `float` | `turning_velocity` | divided by maximum_left_turn (0x308) or maximum_right_turn (0x30c) |
| `0x4e0` | `0x04` | `float` | `wheel_rotation` | 0x572cd0 accumulates forward_velocity into it and wraps it at wheel_circumference (0x310) |
| `0x4e4` | `0x04` | `float` | `left_wheel_rotation` | 0x572b60 accumulates forward minus turning |
| `0x4e8` | `0x04` | `float` | `right_wheel_rotation` | 0x572b60 accumulates forward plus turning |
| `0x4ec` | `0x04` | `float` | `ground_lean` | 0..1, rate-limited to 0.1 per tick by the hover routines 0x5738b0 and 0x5739a0 |
| `0x4f0` | `0x04` | `float` | `ground_contact_fraction` | 0..1; 0x573100 and 0x573f60 ease it toward the speed fraction and the thruster effects scale by it |
| `0x4f4` | `0x14` | `uint8_t[20]` | `contact_point_traction` | one wear byte per physics mass point; 0x575170 reads and rewrites entry i, 0xff meaning full traction. UNRESOLVED: the array bound is the mass point count, n... |
| `0x508` | `0x04` | `uint32_t` | `unknown_508` | zeroed by 0x570b00 |
| `0x50c` | `0x04` | `uint32_t` | `unknown_50c` | zeroed by 0x570b00 |
| `0x510` | `0x04` | `uint32_t` | `bank_angle` | zeroed by 0x570b00 |
| `0x514` | `0x04` | `uint32_t` | `unknown_514` | zeroed by 0x570b00 |
| `0x518` | `0x04` | `uint32_t` | `unknown_518` | zeroed by 0x570b00 |
| `0x51c` | `0x04` | `uint32_t` | `unknown_51c` | zeroed by 0x570b00 |
| `0x520` | `0x04` | `uint32_t` | `active_marker_mask` | one bit per hover / contact marker; 0x575e30 averages the positions of the set ones |
| `0x524` | `0x01` | `uint8_t` | `unknown_524` | cleared by 0x5724d0 after a film snapshot |
| `0x525` | `0x01` | `uint8_t` | `unknown_525` | the scripted spawn seeds it with 1 |
| `0x526` | `0x01` | `uint8_t` | `unknown_526` | read into the film snapshot |
| `0x527` | `0x01` | `uint8_t` | `network_update_sequence` | 0x5724d0 increments it and wraps it at 0xff |
| `0x528` | `0x01` | `uint8_t` | `network_delta_sequence` | base of the delta record 0x5724d0 encodes |
| `0x529` | `0x83` | `uint8_t[0x83]` | `unknown_529` | untouched by this module |
| `0x5ac` | `0x04` | `int32_t` | `network_update_tick` | game tick of the last seat change or network update; vehicle_update rate-limits on it |
| `0x5b0` | `0x02` | `int16_t` | `cinematic_facing_index` | 0x570de0 indexes the cinematic direction table of the scenario with it |
| `0x5b2` | `0x0e` | `uint8_t[0xe]` | `unknown_5b2` | untouched by this module |

#### `unit_scale_request` &mdash; 0x8 bytes, UNSURE

The second argument of `unit_apply_scale_change` (0x562030). Nothing in this module constructs
one, so the caller that does may show it to be longer.

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x00` | `0x04` | `float` | `scale` | written to `object.body_vitality` when > 0 |
| `0x04` | `0x04` | `uint32_t` | `flags` | bit 0 = run the seat / grenade teardown |

#### `unit_network_update_record` &mdash; 0x19 bytes, UNSURE past 0x08

The wire record of the biped health / grenade / shield network column, confirmed from both ends:
`unit_submit_periodic_network_update` (0x55b440) builds it, `unit_apply_network_health_update`
(0x55b5f0) reads the same four bytes back in the same order.

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x00` | `0x04` | `int32_t` | `hash_key` | `hash_table_get` result, 0 when absent |
| `0x04` | `0x01` | `uint8_t` | `update_sequence` | `biped_data.network_update_sequence` (0x527) |
| `0x05` | `0x01` | `uint8_t` | `delta_sequence` | `biped_data.network_delta_sequence` (0x528) |
| `0x06` | `0x01` | `uint8_t` | `is_full_update` | receiver takes the baseline block only when non-zero |
| `0x07` | `0x01` | `int8_t` | `shield_recharging` | receiver applies `shield_vitality` only when this is 1 |
| `0x08` | `0x04` | `int32_t` | `timestamp_milliseconds` | `QueryPerformanceCounter * 1000 / frequency` |
| `0x0c` | `0x02` | `int16_t` | `grenade_counts` | both counts as one int16 &mdash; UNSURE |
| `0x0e` | `0x02` | `int16_t` | `unknown_0e` | never written by the sender &mdash; UNSURE |
| `0x10` | `0x04` | `float` | `body_vitality` | UNSURE |
| `0x14` | `0x04` | `float` | `shield_vitality` | `shield_vitality / 3` or the cached network value &mdash; UNSURE |
| `0x18` | `0x01` | `uint8_t` | `shield_stunned` | `object.shield_stun_ticks > 0` &mdash; UNSURE |

#### `unit_network_control_packet` &mdash; 0x24 bytes, UNSURE

What `unit_apply_network_control_update` (0x566c90) receives in EAX. The payload is bit-packed by
a `message_delta` field definition that lives outside this module, so it is carried as an opaque
window rather than invented field by field.

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x00` | `0x04` | `int32_t *` | `kind_ptr` | itself a pointer; `*kind_ptr == 0` selects the control-update path |
| `0x04` | `0x20` | `uint8_t[32]` | `payload` | bit-packed &mdash; UNSURE, see that file's header |

#### `unit_object_anchor` &mdash; overlay of the first 0x28 bytes of `object`

`types/objects.h` cuts 0x019..0x021 as `unknown_019[7]` + `player_visibility_mask` because the
objects module never reads them; `unit_recalculate_position` (0x558eb0) reads and writes a
12-byte point at 0x01c, straddling that boundary. Declared here as an overlay so `objects.h`
does not have to be re-cut from the units side.

| offset | size | type | field | notes |
|---|---|---|---|---|
| `0x00` | `0x1c` | `uint8_t[0x1c]` | `before_01c` | placeholder: `object.definition_tag` .. `unknown_019` tail |
| `0x1c` | `0x0c` | `real_point3d` | `cached_anchor_point` | compared against `object.position` (0x05c) &mdash; UNSURE name |

## Known gaps

### Types

* **`vehicle_data` past 0x520.** `0x570b00` (the vehicle reset column) zeroes 0x4cc..0x4f8 and
  0x508..0x520 but *not* 0x4fc..0x507, and nothing in the processed range reads those three
  dwords. `contact_point_traction` is declared as 20 bytes to keep the record
  byte-complete; that length is an assumption, flagged in the header.
* **The biped / vehicle overlap.** Both extensions start at object + 0x4cc, so the same
  literal offset in a decompile means two different fields. Assignment was done by asking
  which update tree reaches the function. Three ranges stayed genuinely ambiguous and were
  resolved by majority evidence rather than proof: 0x4d0..0x4d3, 0x508..0x520 and
  0x524..0x528. See the notes file.
* **`biped_movement_solver_data` fields 0x48, 0x5c, 0x60, 0x94, 0x98, 0xa8.** The two
  integrators pin the offsets and the values written, but the consumer is `0x55efd0` in the
  physics module, which has not been processed, so what the solver *does* with them is
  unknown. Likewise `result_flags` bit 0x10.
* **`unit_data` unresolved offsets** are listed one by one in
  `out/phase4/units_types_notes.md`; the header names them `unknown_XX` rather than guessing.
  The most load-bearing of them are 0x210/0x214 (a countdown plus a flags word ORed into
  `control_flags`), 0x404..0x428 (the damage / stun bookkeeping `unit_update` consumes) and
  0x338/0x33c (two 0..1 scalars the vehicle lean code multiplies by).
* **Both TYPES-GAP items the rewriters flagged are closed, not open.** `biped_update.c`
  reinterprets the tail of a `ModelNode` (`scale` at +0x68, the rotation `Matrix` at +0x6c,
  `translation` at +0x90) as a `real_matrix4x3`; that is not an approximation &mdash;
  `real_matrix4x3` is exactly `{float scale; forward; left; up; position}` = 0x34 bytes, the
  same 52 bytes in the same order, and the ground-adjust cluster indexes the object node array
  with the same 0x34 stride. `unit_refresh_anchor_position`'s cached anchor point at
  object + 0x1c is declared in `types/units.h` as the `unit_object_anchor` overlay rather than
  by re-cutting `object` from the units side. Neither needs a local typedef, and none is left
  in this directory.
* Raw tag offsets still appear in a handful of files where the field belongs to a tag this
  module does not otherwise touch &mdash; the Weapon tag's zoom rows (`+0x3da`, `+0x4ac`,
  `+0x4bc`, `+0x4d0`) in `unit_update.c` are the main cluster.

### Register-implicit calls

The single biggest source of uncertainty in this module. MSVC 7.1 passed many of these
arguments in registers, and Ghidra binds only the stack operands, so a call that really took
three arguments often shows up with one or none. Every such site carries an `// UNSURE`
comment naming what could not be bound. Three math helpers were resolved by reading the
callee's own decompilation and are now declared identically everywhere in this directory:

| helper | address | convention |
|---|---|---|
| `vector3d_normalize_with_length` | `0x401990` | vector in ECX, normalized in place, original length returned |
| `vector3d_cross_product` | `0x4052c0` | `*out = stack_operand x ecx_operand`; out in EAX |
| `vector3d_rotate_about_axis` | `0x4cd820` | vector in EAX rotated in place about the axis in ECX by the pushed (sin, cos) |

Where a call site cannot supply the full operand list, the declaration is deliberately left
unprototyped (`extern void vector3d_cross_product();`) with that convention restated, so no
two files in this directory assert contradictory prototypes for the same symbol.

The integration pass closed every disagreement it could close mechanically: all 231 files now
name the same address the same way, and wherever the address has a definition under `src/` the
`extern` carries that definition's exact signature, or the recovered shape plus a comment giving
the real one. What remains is 13 addresses with **no definition anywhere in `src/` yet**, where
two call sites in this module recovered different subsets of the same register-passed list and
there is no third party to arbitrate:

| address | declared as | argument counts seen |
|---|---|---|
| `0x4052c0` | `vector3d_cross_product` | 0 and 3 (the 0-arg form is the documented unprototyped convention above) |
| `0x42be40` | `FUN_0042be40` | 4 and 6 |
| `0x42d340` | `ai_communication_broadcast` | 1 and 7 |
| `0x4507a0` | `FUN_004507a0` | 1 and 6 |
| `0x4726f0` | `FUN_004726f0` | 0 and 1 |
| `0x4c2c70` | `FUN_004c2c70` | 0 and 1 |
| `0x4d6280` | `FUN_004d6280` | return typed `int16_t` in five files and `int32_t` in five others |
| `0x4d6ab0` | `model_get_region_index_by_name` | 0 and 2 |
| `0x4e1a80` | `FUN_004e1a80` | 6 and 7 |
| `0x4e6f20` | `player_update_history_free_all` | 0 and 1 |
| `0x4e9d40` | `FUN_004e9d40` | 0 and 1 |
| `0x504f60` | `FUN_00504f60` | 2 and 5 |
| `0x505880` | `FUN_00505880` | 3 and 5, and four different return spellings |

These want a disassembly-level pass over the callee prologues, not more decompiler reading.

### Foreign functions this module calls but does not own

`0x55efd0` (the object movement / collision slide solver) is the important one: both biped
integrators are thin wrappers around it, so their rewrite confidence is capped by it.
`0x564ae0` (the bounded angular servo), `0x504e10` / `0x504f60` / `0x505880` (the collision
trace chain), `0x56ff40` (the melee damage application), `0x46fe10` (the difficulty-scaled
globals lookup) and `0x428270` (an actor-side predicate) are the rest.

## Misattributed functions

Fixed by the `object_type_definition` vtables:

* **`0x5590a0` is `biped_update`, not `unit_update`** &mdash; it is the biped row's `+0x34`
  column. The real per-tick unit update is the unit row's `+0x34` column, **`0x5625b0`**,
  which Ghidra called `FUN_005625b0`. Both files carry the corrected name.
* **`0x55b7c0` is `biped_update_facing`, not `unit_update_facing`** (this pass). Every tag
  field it reads is a Biped field that does not exist on a bare Unit &mdash;
  `moving_turning_speed` 0x2f0, `biped_flags` 0x2f4, `bank_angle` / `bank_apply_time` /
  `bank_decay_time` 0x324/0x328/0x32c, `pitch_ratio` 0x330, `angular_velocity_maximum` /
  `angular_acceleration_maximum` 0x344/0x348 and `cosine_stationary_turning_threshold` 0x4c8
  &mdash; and it branches on `biped_data.movement_state`. Its only caller is `biped_update`.
* **`0x5756f0`** is the vehicle row's `+0x38` column, so its inherited name
  `unit_calculate_animation_controls` should be `vehicle_calculate_animation_controls`; it
  only ever reads `vehicle_data`. Rewritten under the corrected name, and
  `symbols/agent_phase4_units.txt` carries it at 0.70 so the merge outranks the phase-2 spelling.
* **`0x570b00`** is the vehicle row's `+0x50` reset hook, not a generic "clears control state"
  helper. Rewritten as `vehicle_reset_state`.

Inherited names that are simply wrong (types unaffected, the name misleads):

| address | inherited name | what it actually is |
|---|---|---|
| `0x565a70` | `unit_update` | 50 bytes duplicating the tail of `0x5659c0`: clears the weapon-switch flags and the 0x348 timer. Rewritten as `unit_clear_weapon_switch_state`. |
| `0x56c440` | `unit_get_camera_position` | conditionally detaches the unit from its seat; the real camera getter is `0x568f80`, which also carries the name |
| `0x565040` | `unit_scripting_set_current_vitality` | a cross-product / clamp / transform vector helper; belongs in `math` |
| `0x5738b0` | `unit_get_custom_animation_time` | hovering-vehicle lift/turn physics; the real getter is `0x5701b0` |
| `0x5739a0` | `unit_start_user_animation` | the same hovering-vehicle physics; the real one is `0x5702a0` |
| `0x571b40` | `unit_throw_grenade_release` | accumulates weighted per-marker forces for a vehicle; grenade release is `0x56e440` |
| `0x55e83a` | `biped_new_from_network_unit_grenade_count_mod` | one INT3 byte of padding |
| `0x55e9ff` | `biped_build_update_delta_unit_grenade_count_mod1` | a real 153-byte impulse applier entered mid-instruction; the name is unrelated. The file keeps the misleading name per the naming rule and says so. |
| `0x569450`, `0x56d070` | `unit_animation_set_state`, `unit_inventory_get_weapon` | 1-byte and 9-byte stubs in this build |

Phase-2 candidate names that did not survive reading the body, renamed with the evidence in
each file header: `0x560c70` / `0x560cb0` (guessed "permutation" lookups, actually resolve
`UnitUnitHudInterface.hud.tag_id`), `0x560f20` (guessed "set_animation_state", actually
commits the speech queue), `0x561990` / `0x561a00` (guessed "permutation" selection, actually
pick a `UnitDialogueVariant`).

Functions inside the module range whose *types* belong to another module and which were
therefore listed rather than rewritten: `0x55efd0` (object movement / collision, physics);
`0x564580`, `0x564840`, `0x564990`, `0x564ae0`, `0x5579e0`, `0x558860`, `0x55eed0`,
`0x5658f0`, `0x565040`, `0x572a90` (pure math &mdash; ramp solvers and vector / basis
helpers); `0x561cb0`, `0x561d50`, `0x561e60`, `0x561ab0` (generic datum-table walkers,
objects/memory); `0x5724d0` (the saved-film transform recorder); `0x56eb90` (a six-string
table lookup); `0x55e83a` (one byte of padding).

## Corrections made in the phase-4 review pass

* `unit_get_crouch_height_offset` (`0x55a2e0`) read the Biped tag's **camera** heights
  (0x400 / 0x404) and `crouch_transition_time` (0x408) where the binary reads the
  **collision** heights (0x424 / 0x428) and `collision_radius` (0x42c). It computes the
  collision pill, not a camera offset. Fixed, and the same wrong field appeared once more in
  `unit_find_placement_position`. `out/phase4/units_types_notes.md` still carries the
  original mislabelling; `types/units.h` has been corrected.
* `unit_update_aiming_overlay_angles` (`0x563b50`) gated the looking-overlay branch on
  `base_animation_state` (0x2a7). The decompile reads `puVar2[0xa7]`, a *dword index*, i.e.
  object + 0x29c &mdash; `unknown_29c`, which the notes already describe as the seat / turret
  overlay gate. Fixed.
* `unit_set_or_test_seat_and_weapon_label` (`0x5651e0`) was declared in five caller files with
  three arguments and a `uint32_t` return; its definition takes the unit index in EAX first
  and returns a byte. Declarations and call sites aligned.


### Corrections made in the integration pass over the whole module

The second half of the module (0x566de0..0x575e30) was rewritten by two agents working in
parallel; the integration pass re-read the largest and most `UNSURE`-dense files against
`python tools/pack.py 0xADDR` and found several errors that were **systematic**, because the
"eject a unit from its vehicle seat" block is duplicated seven times across the module and every
copy inherited the same misreadings. All of them are fixed:

* **`Object` tag + 0x34 is `model.tag_id`, not `attachments`.** 25 sites in 12 files read it as
  `attachments.definition`, which is `Object` + 0x148. The companion test `puVarN[4] & 1` is the
  **object's** `flags` word (object + 0x10), not a tag field, and the paired
  `puVarN[4] &= 0xfffffffe` write was missing entirely at 10 of those sites. Confirmed against
  the canonical copy at 0x56c640 and independently against `biped_update.c`, which had it right.
* **`k_unit_data_offset` was being added to offsets that are already object-absolute.** 27 sites
  in 12 files spelled `unit_data.vehicle_seat_index` as `*(int16_t *)(obj + k_unit_data_offset +
  0x2f0)`, i.e. object + 0x4e4, which is past the end of `unit_data` (it ends at 0x4cc) and lands
  inside `biped_data` / `vehicle_data`. The same mistake hit 0x2a0, 0x2a1, 0x2a3 and 0x218. All
  are now named field accesses through a correctly based `unit_data *`.
* **The seat-exit bookkeeping was attributed to the wrong object.** In all seven copies Ghidra
  writes `last_parent_object_index` / `last_seat_change_tick` and the *first* driver/gunner clear
  pair on the **exiting unit** (`puVar3` / `puVar15` / `puVar2` / `puVar8`) and only the *second*
  pair on the **vehicle** (`local_8` / `puVar5` / `puVar11` / `puVar9`); the `animation_state !=
  0x25` guard is the vehicle's. Six of the seven had this inverted, and the value written into
  `last_parent_object_index` was the unit's own index rather than the parent object's.
* **The node-transform snapshot came from the wrong tag.** Five files sourced the three dwords
  copied into the exiting unit's node block from `animation_graph.tag_id` and then applied
  `+ 0x34` a second time; the binary reads `model.tag_id`'s tag data at `+ 0xbc`.
* **A control-flow error in `unit_apply_damage_effects` (0x5674a0).** `LAB_00567a9a`'s own
  `if (role != 1) goto LAB_00567b07` exit was missing, and the else-branch's copy of that exit
  jumped *into* the controlling-player block instead of past it, so the player-history reset ran
  on dedicated servers.
* **`unit_release_transient_state` (0x568610) had one level of indirection too many** in its
  seat-exit block: Ghidra's `puVar2` is the unit itself and `local_c` is `object.parent_object`,
  but the rewrite read the grandparent's `last_parent_object_index` and then re-fetched. The
  block is restructured against the decompilation.
* **Globals that hold pointers were declared as arrays.** 0x00687130, 0x0087a478 and 0x00746fa0
  are read as `*(T *)(DAT + off)`, i.e. the stored value is the base; declaring them
  `uint8_t name[]` makes the base the *address of the slot*. Fixed, and
  `unit_apply_network_control_update`'s missing inner dereference of
  `PTR_DAT_00687130 + 0x28` with it (it also had the offset as `0x28 * 4`).
* **One name and one type per global address.** 0x00696714/18/20 had four spellings between
  them, and `network_predicted_state_flag` named *two different addresses* (0x006f1d20 and
  0x0071c419) in different files. All 231 files now agree, and the
  `types/math.h` spellings win where they apply.
* **Externs now carry their definition's real signature.** Every `extern` in the directory whose
  address has a definition somewhere in `src/` uses that definition's name (58 addresses were
  still spelled `FUN_00XXXXXX`), and its exact signature wherever the recovered argument count
  matched (166 declarations). Where Ghidra recovered fewer arguments than the function takes, the
  declaration keeps the recovered shape and carries a `// real signature (file.c): ...; Ghidra
  recovered N of M args at this call site` comment rather than inventing the missing values (158
  declarations). 28 explicit casts were added at call sites where the aligned signature exposed a
  pointer-type mismatch.

Two whole-directory scans for the remaining known failure modes came back clean: no file reads a
signed byte in Ghidra while using only unsigned types in the rewrite, and every `continue` in the
directory sits inside a `for` loop whose header carries the advance step that the corresponding
`goto LAB_xxx` targeted.

## Functions

All 231 rewritten functions, in address order. *name conf* is how sure the name is, *rewrite
conf* is how sure the body is; both are each file's own header values. *UNSURE* counts the
`UNSURE` annotations in the rewritten half of the file (1003 across the directory). The rewrite
confidence distribution is 28 files at >= 0.6, 68 at 0.4-0.55, 112 at 0.2-0.35 and 23 at <= 0.15
&mdash; the last group is dominated by shared tail fragments Ghidra split out of larger callers,
whose operands all arrive as untraceable `unaff_*` / `in_stack_*` registers.

| address | file / function | bytes | name conf | rewrite conf | UNSURE | what it does |
|---|---|---|---|---|---|---|
| `0x557a90` | `biped_ground_adjust_step` | 235 | 0.4 | 0.55 | 1 | Caches current node positions and drives the ground-adjustment constraint solve and node-basis update for a unit, u... |
| `0x557b80` | `biped_ground_adjust_solve_node` | 1143 | 0.4 | 0.4 | 6 | Computes and validates a ground/support contact position for a single skeleton node against its collision marker, r... |
| `0x558000` | `biped_ground_adjust_solve` | 2123 | 0.4 | 0.35 | 12 | Iteratively solves per-node ground-contact and bone-length distance constraints for a unit's skeleton (leg/foot gro... |
| `0x558a20` | `biped_ground_adjust_apply_node_rotations` | 787 | 0.4 | 0.3 | 3 | Applies small per-node rotations to align a unit's skeleton nodes to their newly solved ground-adjusted positions,... |
| `0x558eb0` | `unit_recalculate_position` | 494 | 0.4 | 0.55 | 7 | Validates and refreshes a unit's cached attachment/anchor position against its current position, guarding against N... |
| `0x5590a0` | `biped_update` | 3460 | 0.85 | 0.3 | 20 | Top-level per-frame update for a unit object: handles seat/parent attachment changes and drives movement, facing, a... |
| `0x559fa0` | `unit_apply_impulse` | 455 | 0.35 | 0.35 | 4 | Adds a positional impulse to a unit, applying random jitter to its facing basis when the unit has no parent object. |
| `0x55a170` | `unit_compute_marker_offset_position` | 359 | 0.35 | 0.3 | 2 | Computes an offset position for a unit marker/attachment point, selecting between several position-blend modes. |
| `0x55a2e0` | `unit_get_crouch_height_offset` | 173 | 0.4 | 0.75 | 0 | Computes a unit's current vertical (crouch-interpolated) height offset from its object position and tag-defined cro... |
| `0x55a390` | `unit_get_look_origin_and_direction` | 364 | 0.3 | 0.35 | 4 | Computes a look/aim origin and direction from two named model markers (typically eye markers), falling back to a he... |
| `0x55a500` | `unit_find_placement_position` | 1299 | 0.35 | 0.25 | 14 | Searches a scatter pattern of candidate offsets around an object for a collision-free spot and repositions the obje... |
| `0x55aa20` | `unit_test_placement_candidate` | 267 | 0.35 | 0.25 | 3 | Tests a candidate position offset from a base point against level collision and returns it if valid. |
| `0x55ab30` | `biped_get_cached_look_at_position` | 464 | 0.35 | 0.4 | 6 | Periodically refreshes and returns a cached target look-at position for a unit, falling back to raw object position... |
| `0x55ad00` | `unit_reset_ground_adjust_state` | 108 | 0.7 | 0.6 | 2 | Resets a unit's ground-adjustment iteration counter and marks its ground-adjust state dirty when the appropriate ta... |
| `0x55ad70` | `unit_clear_ground_adjust_dirty` | 86 | 0.6 | 0.6 | 1 | Clears the ground-adjust dirty flags previously set by unit_reset_ground_adjust_state, once the corresponding tag f... |
| `0x55add0` | `unit_reset_orientation_and_find_position` | 244 | 0.35 | 0.5 | 0 | Resets a unit's orientation basis to world axes and finds a valid spawn/placement position for it. |
| `0x55aed0` | `unit_build_network_update` | 563 | 0.35 | 0.3 | 4 | Packs a unit's key simulation state (position, orientation, health, animation) into a structure and submits it as a... |
| `0x55b110` | `unit_network_create_update_apply` | 702 | 0.35 | 0.2 | 7 | Handles an incoming network unit-creation update by spawning the unit object and initializing its health, shield, a... |
| `0x55b440` | `unit_submit_periodic_network_update` | 421 | 0.35 | 0.45 | 5 | Builds and submits a periodic network update packet (full or delta) describing the local player's controlled unit. |
| `0x55b5f0` | `unit_apply_network_health_update` | 395 | 0.4 | 0.5 | 6 | Applies a received network health/shield update to the local unit if its sequence number is not stale. |
| `0x55b7c0` | `biped_update_facing` | 1745 | 0.7 | 0.5 | 3 | Turns a unit's facing direction toward its desired heading at a tag-limited turn rate each tick, then rebuilds its... |
| `0x55bea0` | `biped_integrate_movement` | 3577 | 0.6 | 0.45 | 6 | Integrates a unit's per-tick movement from control input and animation markers, resolves collisions, and reports th... |
| `0x55cca0` | `unit_predict_movement_delta` | 783 | 0.35 | 0.3 | 4 | Iterates over unit objects to find one and drives its movement update, apparently used for non-locally-controlled (... |
| `0x55cfd0` | `biped_integrate_movement_with_collision` | 4295 | 0.6 | 0.4 | 12 | Full-featured per-tick unit movement update that additionally resolves BSP collision and applies fall damage. |
| `0x55e0a0` | `biped_update_target_lock_timer` | 237 | 0.35 | 0.35 | 4 | Maintains a per-unit target-lock counter, triggering a reaction once the same target has been tracked for several t... |
| `0x55e190` | `biped_check_evade_reaction` | 313 | 0.3 | 0.3 | 4 | Periodically checks whether a unit should perform an evasive reaction toward a nearby open position, dispatching a... |
| `0x55e2d0` | `unit_evaluate_flee_reaction` | 450 | 0.3 | 0.25 | 5 | Evaluates whether a unit (typically vehicle-mounted) should flee or evade, dispatching one of several reaction codes. |
| `0x55e4a0` | `unit_check_fell_off_level` | 78 | 0.35 | 0.55 | 1 | Detects when a unit has fallen far below the level (Z < -2000) and triggers a kill/respawn handler. |
| `0x55e4f0` | `unit_apply_fall_damage` | 444 | 0.55 | 0.4 | 5 | Applies scaled fall damage to a unit when its downward velocity exceeds the tag-defined safe threshold. |
| `0x55e6b0` | `unit_rotate_basis_about_axis` | 392 | 0.35 | 0.2 | 2 | Rotates a unit's stored forward and right basis vectors about a common axis, keeping them orthonormal. |
| `0x55e840` | `biped_update_idle_basis` | 157 | 0.3 | 0.25 | 3 | Selects between running ground-adjustment or an idle basis-refresh animation state for a unit based on elapsed idle... |
| `0x55e8e0` | `biped_is_idle_eligible` | 82 | 0.4 | 0.55 | 0 | Returns whether a unit has been idle long enough (and in an eligible collision state) to trigger idle behaviors. |
| `0x55e940` | `biped_apply_idle_fidget` | 191 | 0.3 | 0.25 | 3 | Nudges an idle unit with a small randomized impulse to produce idle fidget motion, then refreshes its orientation b... |
| `0x55e9ff` | `biped_build_update_delta_unit_grenade_count_mod1` | 153 | 0.2 | 0.15 | 2 | Applies an externally-supplied (optionally randomized) impulse vector to a unit's acceleration accumulator and refr... |
| `0x55eaa0` | `biped_update_animation_frame_trigger` | 232 | 0.3 | 0.2 | 1 | Checks a unit's animation timing fields against a threshold and updates its animation frame-tracking state if it ha... |
| `0x55eb90` | `biped_advance_frame_counter_trigger` | 143 | 0.3 | 0.4 | 2 | Advances a per-unit frame counter and, once a matching state is reached, fires paired animation/sound trigger events. |
| `0x55ec20` | `biped_trigger_on_velocity_threshold` | 109 | 0.3 | 0.5 | 0 | Fires a paired trigger event when a unit exceeds a small velocity threshold after being still. |
| `0x55ec90` | `unit_track_target_lock_timeout` | 94 | 0.3 | 0.55 | 0 | Tracks how long a unit has gone without a target lock and triggers a follow-up handler once a threshold is exceeded. |
| `0x55ecf0` | `unit_snap_to_min_ground_height` | 469 | 0.3 | 0.3 | 3 | Snaps a unit's position up to its tag-defined minimum ground height if it has sunk below it. |
| `0x560410` | `unit_update_footstep_and_idle_triggers` | 359 | 0.3 | 0.35 | 2 | Fires paired trigger events when a unit's seat/turret angle reaches its tag-defined limits, and tracks an idle time... |
| `0x560590` | `unit_fire_animation_sound_trigger` | 153 | 0.35 | 0.35 | 5 | Fires a numbered unit trigger event (sound/animation cue) if its priority and sound tag validate successfully. |
| `0x560630` | `unit_find_nearest_valid_surface_plane` | 446 | 0.3 | 0.15 | 11 | Scans nearby BSP surfaces to find and cache the closest supporting ground plane for a unit. |
| `0x560800` | `unit_update_up_vector` | 1136 | 0.45 | 0.15 | 5 | Gradually rotates a unit's up-vector toward its target orientation (e.g., ground normal or gravity) each tick, with... |
| `0x560c70` | `unit_get_hud_interface_tag_id` | 52 | 0.3 | 0.5 | 1 | Returns a pointer to the Nth permutation record from a unit's variant table, clamped to the valid range. |
| `0x560cb0` | `unit_get_seat_hud_interface_tag_id` | 73 | 0.3 | 0.5 | 0 | Returns a pointer to the Nth permutation record from a specific seat/node's variant table, clamped to the valid range. |
| `0x560d00` | `unit_animation_change_priority_check` | 539 | 0.4 | 0.25 | 6 | Arbitrates whether a requested unit animation change is allowed to override the current one, based on priority tables. |
| `0x560f20` | `unit_commit_speech` | 264 | 0.4 | 0.35 | 3 | Commits a new animation/sound state into a unit's active-animation slot and computes its playback duration. |
| `0x561030` | `unit_play_default_reaction_sound` | 261 | 0.3 | 0.25 | 3 | Plays a fixed default reaction animation and accompanying scripted sound for a unit, once per trigger. |
| `0x561140` | `unit_choose_combat_reaction_animation` | 854 | 0.35 | 0.2 | 8 | Chooses and plays a context-appropriate combat reaction animation (e.g., noticing, evading, or reacting to a target... |
| `0x5614a0` | `unit_dispatch_reaction_animation` | 41 | 0.35 | 0.15 | 2 | Dispatches to one of several per-unit reaction-animation handler routines via a jump table indexed by a small react... |
| `0x561620` | `unit_update_animation_timers` | 700 | 0.35 | 0.25 | 9 | Advances all of a unit's active animation and cooldown timers each tick and fires the corresponding transition call... |
| `0x561990` | `unit_choose_dialogue_variant` | 104 | 0.3 | 0.4 | 0 | Selects a preferred or fallback random permutation variant for a unit and caches the chosen id. |
| `0x561a00` | `unit_pick_random_dialogue_variant` | 168 | 0.35 | 0.45 | 1 | Randomly selects one matching entry from a permutation table using a simple linear-congruential PRNG. |
| `0x561b80` | `unit_update_vitality_fractions` | 300 | 0.2 | 0.3 | 3 | Updates a unit's normalized movement/turn speed-scale fields based on tag-defined maximum speeds, firing threshold-... |
| `0x561f80` | `unit_current_weapon_is_type` | 86 | 0.5 | 0.4 | 1 | Checks whether a unit's currently active weapon object matches a given object-type index, returning the result in AL. |
| `0x562030` | `unit_apply_scale_change` | 326 | 0.35 | 0.2 | 9 | Updates a unit's size-related field and, on a specific animation-state/flag combination, resets related timers and... |
| `0x562570` | `unit_is_look_target_valid` | 61 | 0.3 | 0.35 | 1 | Boolean gate combining several unit fields, used before performing a head-look/aim update. |
| `0x5625b0` | `unit_update` | 4765 | 0.85 | 0.15 | 37 | Per-tick update of a unit's aiming/head-look direction cones, several countdown timers, and animation-state triggers. |
| `0x5639f0` | `unit_apply_control_block` | 289 | 0.4 | 0.6 | 0 | Copies a block of externally supplied fields into a unit's control-state structure, plus an owner-object index. |
| `0x563b20` | `unit_set_control_countdown` | 42 | 0.3 | 0.5 | 0 | Stores two caller-supplied values into a unit's control-input fields at offsets 0x210/0x214. |
| `0x563b50` | `unit_update_aiming_overlay_angles` | 1352 | 0.4 | 0.15 | 12 | Computes the unit's current aiming pitch/yaw and drives several animation-overlay blend-weight setters from them. |
| `0x5640a0` | `unit_find_weapon_marker_transform` | 533 | 0.35 | 0.15 | 9 | Resolves a named marker on a unit's currently referenced weapon, computing its transform and a secondary derived-na... |
| `0x5642c0` | `unit_get_weapon_marker_indices` | 200 | 0.35 | 0.35 | 2 | Looks up a weapon/grenade marker record and outputs two indices (likely node/permutation) from it via output parame... |
| `0x564390` | `unit_get_animation_frames_remaining` | 94 | 0.4 | 0.5 | 0 | Returns how many frames remain in the unit's current animation and outputs its current animation-state byte. |
| `0x565150` | `unit_seat_index_is_valid` | 137 | 0.4 | 0.4 | 0 | Tests whether a given seat index is valid for a unit's vehicle/type, deferring to the seat/weapon-label lookup rout... |
| `0x5651e0` | `unit_set_or_test_seat_and_weapon_label` | 564 | 0.75 | 0.3 | 1 | Finds a matching seat/weapon-label pair on a unit and, when found, records the corresponding seat and weapon indice... |
| `0x565420` | `unit_update_animation_state_machine` | 1133 | 0.45 | 0.25 | 16 | Per-tick dispatcher that advances a unit's current special-move/seat-transition animation state and applies its sid... |
| `0x5659c0` | `unit_validate_and_clear_weapon_switch` | 176 | 0.35 | 0.25 | 5 | Validates a weapon/label change and then clears the object's transient weapon-switch flags and timer. |
| `0x565a70` | `unit_clear_weapon_switch_state` | 50 | 0.2 | 0.2 | 3 | Shares its entire body with the tail of FUN_005659c0: clears transient weapon-switch object flags and timer fields. |
| `0x565ab0` | `unit_get_active_weapon_scale` | 67 | 0.3 | 0.4 | 2 | Returns a scale/modifier value derived from the unit's currently held weapon, defaulting to 1.0 when unarmed. |
| `0x565b00` | `unit_local_player_weapon_flag_check` | 87 | 0.25 | 0.2 | 1 | Resolves a globally tracked object (likely the local player's unit) and forwards to a per-weapon flag test. |
| `0x565b60` | `unit_current_weapon_has_flag` | 126 | 0.3 | 0.3 | 2 | Tests whether a unit's currently equipped weapon's type tag has a specific flag bit set. |
| `0x565be0` | `unit_animation_state_is_compatible` | 61 | 0.3 | 0.25 | 1 | Tests whether a given animation-state/class combination is compatible, used to gate animation-state transitions. |
| `0x565c60` | `unit_state_is_scripted_animation` | 31 | 0.35 | 0.35 | 1 | Returns whether a given animation-state value belongs to the set of uninterruptible/special-move states. |
| `0x565ca0` | `unit_state_allows_control` | 29 | 0.3 | 0.35 | 1 | Returns whether a given animation state still permits normal unit control input. |
| `0x565da0` | `unit_animation_state_from_seat_type` | 37 | 0.25 | 0.45 | 0 | Maps a seat/action enum value into one of the unit's core animation-state constants. |
| `0x565e00` | `unit_start_seat_overlay_animation_a` | 349 | 0.3 | 0.35 | 0 | Starts one of the unit's seat-control overlay animations, tracked at unit+0x2aa, for a given command value. |
| `0x565f90` | `unit_try_set_animation_state` | 942 | 0.5 | 0.3 | 6 | Central routine that validates and applies a new animation-state value to a unit, allocating the underlying animati... |
| `0x566410` | `unit_start_seat_overlay_animation_b` | 273 | 0.3 | 0.35 | 0 | Starts the unit's second seat-control overlay animation, tracked at unit+0x2ae, for a given command value. |
| `0x566560` | `unit_find_best_seat_to_enter` | 722 | 0.6 | 0.25 | 4 | Finds the nearest valid, unoccupied seat on a target vehicle for a unit to board. |
| `0x566840` | `unit_seat_is_occupied_by_other` | 189 | 0.4 | 0.3 | 3 | Checks whether a vehicle already has another occupant seated, and returns that occupant's index. |
| `0x566910` | `unit_all_seats_unoccupied` | 95 | 0.3 | 0.25 | 2 | Returns whether none of a unit-type's seats currently report the tracked occupancy status. |
| `0x566970` | `unit_enter_vehicle_seat` | 652 | 0.7 | 0.2 | 11 | Places a unit into a specific seat of a target vehicle, updating its parent/seat linkage, transform, and weapon label. |
| `0x566c00` | `unit_broadcast_state_change_event` | 129 | 0.3 | 0.2 | 7 | Queues a networked event/message carrying an optional hash-table-resolved index, used after seat-entry/exit transit... |
| `0x566c90` | `unit_apply_network_control_update` | 322 | 0.35 | 0.1 | 16 | Applies an incoming (likely networked) control/state update to a unit, dispatching by packet type. |
| `0x566de0` | `unit_update_stance_and_jump` | 1548 | 0.45 | 0.3 | 9 | Decides and applies a unit's stand/crouch/jump/land animation-state transition based on its current motion and grou... |
| `0x567400` | `unit_dispatch_seat_overlay_command` | 123 | 0.35 | 0.6 | 1 | Dispatches a seat-control command to the appropriate overlay-animation starter based on its enumerated value. |
| `0x5674a0` | `unit_apply_damage_effects` | 3199 | 0.35 | 0.2 | 43 | Applies the gameplay side effects of damage to a unit, including possible vehicle ejection, animation-state change,... |
| `0x568120` | `unit_exit_vehicle_seat` | 263 | 0.5 | 0.3 | 5 | Removes a unit from its current vehicle seat, resetting its pose and transient state and broadcasting the change. |
| `0x568230` | `unit_record_recent_damage_and_react` | 778 | 0.3 | 0.25 | 8 | Records a recent damage or contact event into a small per-unit cache, used to avoid repeating an associated respons... |
| `0x568540` | `unit_pick_random_spawned_actor_count` | 193 | 0.35 | 0.4 | 2 | Lazily computes and caches a random variant/permutation index for a unit, seeded from the global PRNG. |
| `0x568610` | `unit_release_transient_state` | 1696 | 0.35 | 0.2 | 19 | Releases a unit's transient sound/animation handles and, when seated, performs additional vehicle-seat transition c... |
| `0x568cb0` | `unit_release_transient_state_and_detach` | 671 | 0.4 | 0.3 | 6 | Performs the transient-state cleanup used when a unit fully leaves its seat, also clearing its overlay-animation an... |
| `0x568f50` | `unit_get_primary_eye_marker_position` | 45 | 0.3 | 0.3 | 2 | Retrieves the world position of a fixed named marker via FUN_004f6080 and returns it through the implicit ESI outpu... |
| `0x568f80` | `unit_get_camera_position` | 516 | 0.6 | 0.3 | 3 | Computes a unit's current camera position, blending between standing and crouching heights or using the vehicle sea... |
| `0x569190` | `unit_add_marker_relative_offset` | 232 | 0.3 | 0.25 | 5 | Computes the offset between a given world point and the unit's camera position, accumulating it into an output vector. |
| `0x569280` | `unit_get_secondary_eye_marker_position` | 45 | 0.3 | 0.3 | 2 | Retrieves the world position of a second fixed named marker via FUN_004f6080 and returns it through the implicit ES... |
| `0x5692b0` | `unit_map_action_command_to_animation_state` | 149 | 0.4 | 0.6 | 1 | Maps a scripted/action command enum into the corresponding unit animation-state constant and its priority class. |
| `0x5693a0` | `unit_is_seat_control_available` | 176 | 0.3 | 0.4 | 1 | Reports whether a specific seat control is currently available to the unit, depending on its animation state and se... |
| `0x569450` | `unit_animation_set_state` | 1 | 0.4 | 0.9 | 1 | Effectively a no-op stub (single return instruction) under this name; no observable behavior to confirm or refute it. |
| `0x569470` | `unit_scripted_action_animation_exists` | 184 | 0.35 | 0.3 | 1 | Checks whether the animation corresponding to a given scripted action currently exists for the unit's type. |
| `0x569530` | `unit_try_start_scripted_action_animation` | 319 | 0.4 | 0.3 | 2 | Starts a unit's scripted action animation (e.g. melee/grenade-throw class) if one exists for the requested action. |
| `0x569670` | `unit_noop_569670` | 118 | 0.25 | 0.9 | 1 | Currently a no-op; its referenced globals suggest it once performed unit/tag-table work that has since been inlined... |
| `0x5696f0` | `unit_get_aiming_vector` | 45 | 0.4 | 0.7 | 0 | Returns the unit's stored ground/surface normal vector. |
| `0x569720` | `unit_get_forward_vector_or_marker_normal` | 113 | 0.35 | 0.3 | 2 | Returns a unit's stored direction/offset vector, transformed into world space through its parent object's skeleton... |
| `0x5697a0` | `unit_clamp_direction_to_aim_or_look_bounds` | 451 | 0.4 | 0.25 | 3 | Tests a world-space direction against the unit's aiming or looking angle limits (selected by a flag), clamps it int... |
| `0x569970` | `unit_get_weapon_object_index` | 44 | 0.55 | 0.8 | 0 | Returns the object index of the weapon stored in the unit's inventory slot given by in_CX, or -1 if no slot is sele... |
| `0x5699a0` | `unit_find_next_grenade_type_with_count` | 117 | 0.4 | 0.5 | 0 | Searches forward or backward through the unit's seat/marker usage-count table to find the next label with a non-zer... |
| `0x569a20` | `unit_try_ready_weapon` | 238 | 0.35 | 0.4 | 1 | Checks whether the unit's current weapon animation mode allows a state change and, if so, calls FUN_00565f90 and FU... |
| `0x569b30` | `unit_try_ready_weapon_variant` | 116 | 0.3 | 0.4 | 1 | A variant weapon-mode gate similar to unit_try_ready_weapon, additionally checking unit-type and a flag at offset 0... |
| `0x569bc0` | `unit_get_flag_bit6` | 34 | 0.25 | 0.6 | 0 | Returns a single flag bit (bit 6) from the unit's 0x204 flags dword. |
| `0x569bf0` | `unit_refresh_targeting_flag_and_weapons` | 158 | 0.3 | 0.3 | 3 | Recomputes a control/targeting flag on the unit based on whether it has any active target or seat references, then... |
| `0x569c90` | `unit_is_in_busy_animation_state` | 56 | 0.4 | 0.6 | 0 | Returns whether the unit's current weapon/vehicle-transition mode byte is one of the reserved 'busy' state values. |
| `0x569cf0` | `unit_scripting_set_emotion_animation` | 74 | 0.9 | 0.5 | 3 | Looks up a named emotion animation and stores its index on the unit, logging an error if it isn't found. |
| `0x569d40` | `unit_detach_and_enter_named_seat` | 1340 | 0.35 | 0.2 | 15 | Detaches a unit from its current parent object and, given a target parent and named seat marker, re-attaches/positi... |
| `0x56a290` | `unit_reset_velocity_and_ground_flag` | 119 | 0.3 | 0.5 | 0 | Sets or clears a per-unit boolean flag, resets a cached 3D vector field from a shared default, and clears an additi... |
| `0x56a310` | `unit_find_seats_matching_name_and_flags` | 402 | 0.35 | 0.3 | 3 | Scans the unit's seat/marker definitions for unoccupied entries whose name matches (or is empty) and whose type fla... |
| `0x56a4c0` | `unit_seat_candidates_from_zone_and_enter` | 1595 | 0.3 | 0.2 | 14 | Finds seats on the unit matching a name/flags filter and forcibly detaches whichever child objects currently occupy... |
| `0x56ab10` | `unit_notify_weapon_removed` | 17 | 0.3 | 0.6 | 1 | Calls FUN_00565f90 when the implicit weapon/object index is valid, used as a small guard before weapon-related tear... |
| `0x56ab30` | `unit_notify_weapon_removed_dup` | 17 | 0.25 | 0.6 | 1 | Duplicate of unit_notify_weapon_removed; calls FUN_00565f90 when the implicit index is valid. |
| `0x56ab50` | `unit_detach_child_at_named_seat` | 1848 | 0.4 | 0.2 | 16 | Finds the child object attached to the unit at a seat whose marker name matches param_2 and detaches/repositions it... |
| `0x56b290` | `unit_mark_zone_occupants_flag` | 235 | 0.3 | 0.4 | 0 | Iterates a player/zone-indexed list of seat markers and marks each seated unit's flags field with bit 0x100000. |
| `0x56b380` | `unit_named_seat_occupant_in_zone` | 407 | 0.35 | 0.3 | 0 | Looks up a named seat on the unit and reports whether the object currently occupying it also appears in the zone/li... |
| `0x56b520` | `unit_is_child_seated_at_named_marker` | 196 | 0.35 | 0.4 | 0 | Returns whether a given child object is seated at the named marker on this unit. |
| `0x56b5f0` | `unit_try_exit_controlled_seat` | 1495 | 0.35 | 0.2 | 15 | Attempts to make the currently controlled unit exit its seat, choosing between an animated exit sequence and an imm... |
| `0x56bbd0` | `unit_build_seat_occupant_zone_list` | 170 | 0.35 | 0.4 | 2 | Allocates a new zone/list datum and populates it with references to every seated occupant of the unit. |
| `0x56bc80` | `unit_point_in_front_and_asleep` | 215 | 0.3 | 0.3 | 4 | Tests whether a given world point lies in front of the controlled unit and matches an additional name-based condition. |
| `0x56bd60` | `unit_current_weapon_type_is_2_or_3` | 90 | 0.3 | 0.5 | 1 | Returns whether the unit's currently equipped weapon has type code 2 or 3. |
| `0x56bdc0` | `object_find_next_untargeted` | 276 | 0.3 | 0.4 | 0 | Finds the next object (wrapping around) after a given one that has no active target references and isn't marked for... |
| `0x56bee0` | `object_find_nearest_biped` | 219 | 0.4 | 0.5 | 0 | Finds the nearest object of type 0 (biped) to a given reference object by scanning all objects and comparing positi... |
| `0x56bfc0` | `unit_sample_camera_shake_from_velocity` | 168 | 0.3 | 0.3 | 5 | Samples a value derived from the unit's scaled velocity and, above a 0.95 threshold, bumps an accumulator and trigg... |
| `0x56c070` | `unit_any_dying_or_seat_transition` | 124 | 0.3 | 0.5 | 0 | Scans all units for one that is dead (with a specific sub-state) or currently entering/exiting a seat, returning tr... |
| `0x56c100` | `unit_point_within_look_cone` | 208 | 0.4 | 0.25 | 2 | Returns whether a given world point lies within a cone of half-angle param_1 around the unit's forward direction. |
| `0x56c1d0` | `unit_mark_zone_list_alt_flag` | 278 | 0.3 | 0.4 | 0 | Iterates a zone/player-indexed list of seat markers and sets one of two alternate flag bits on each seated unit dep... |
| `0x56c2f0` | `unit_get_seat_or_state_name` | 115 | 0.4 | 0.6 | 0 | Returns the name of the seat/marker the unit currently occupies, or a default state-name string if it has no parent. |
| `0x56c370` | `unit_dispatch_scripted_event_9` | 133 | 0.3 | 0.25 | 7 | Dispatches a local scripted event (id 9) built from a hashed lookup value and a byte parameter, and on success forw... |
| `0x56c400` | `unit_dispatch_seat_exit_message` | 69 | 0.25 | 0.2 | 6 | Dispatches to either a local seat-exit handler or a network handler depending on a flag read from the object pointe... |
| `0x56c440` | `unit_detach_if_flag_clear` | 29 | 0.3 | 0.7 | 1 | Conditionally detaches the unit from its current parent/seat (via FUN_0056c640) when the flag in_AL is clear; despi... |
| `0x56c470` | `unit_try_start_seat_exit_animation` | 461 | 0.4 | 0.25 | 7 | Attempts to start the controlled unit's seat-exit animation sequence, returning whether the exit was actually initi... |
| `0x56c640` | `unit_detach_from_seat` | 1024 | 0.45 | 0.25 | 11 | Detaches a unit from the parent object/seat it is currently attached to, repositioning it in world space at the sea... |
| `0x56ca40` | `unit_detach_reposition_and_nudge` | 459 | 0.2 | 0.2 | 8 | Smoothly steers the unit's stored aim/look direction toward a target position each tick, updating the corresponding... |
| `0x56cc10` | `unit_is_seat_occupied` | 104 | 0.4 | 0.5 | 0 | Returns whether any object is currently seated at the given parent/seat-index pair. |
| `0x56cc80` | `unit_any_flagged_seat_occupied` | 120 | 0.4 | 0.5 | 0 | Returns whether any of the unit's flagged seats currently has an occupant. |
| `0x56cd10` | `unit_seat_flag_bit2` | 84 | 0.3 | 0.5 | 0 | Returns a specific flag bit (bit 2) from the definition of the unit's seat at index in_CX. |
| `0x56cd70` | `unit_seat_flag_bit3` | 84 | 0.3 | 0.5 | 0 | Returns flag bit 3 from the unit's seat definition at index in_CX. |
| `0x56cdd0` | `unit_seat_flag_bit10` | 84 | 0.3 | 0.5 | 0 | Returns flag bit 10 from the unit's seat definition at index in_CX. |
| `0x56ce30` | `unit_recompute_seat_occupants` | 223 | 0.4 | 0.3 | 0 | Recomputes which child object occupies the unit's primary and secondary tracked seats (fields 0xc9/0xca) by scannin... |
| `0x56d070` | `unit_inventory_get_weapon` | 9 | 0.4 | 0.9 | 0 | Empty stub; performs no operation in this build. |
| `0x56d080` | `unit_try_give_grenade` | 214 | 0.3 | 0.2 | 5 | Attempts to give the unit one more grenade of its currently selected type, up to the type's maximum, updating relat... |
| `0x56d160` | `unit_set_grenade_type_and_count_delta` | 61 | 0.35 | 0.6 | 0 | Adjusts the occupancy/usage count for seat label in_DX by param_1 and records it as the most recently touched label. |
| `0x56d1a0` | `unit_try_select_equipment` | 273 | 0.3 | 0.3 | 7 | Switches the unit's currently selected secondary item (e.g. grenade type) to param_2 if none is already selected, r... |
| `0x56d2c0` | `unit_clear_selected_equipment` | 57 | 0.3 | 0.6 | 0 | Clears the unit's currently selected secondary item field, releasing it first via FUN_0056ed00. |
| `0x56d300` | `unit_release_selected_equipment` | 93 | 0.3 | 0.6 | 2 | Releases the unit's currently selected secondary item by dispatching to a type-specific release routine, then clear... |
| `0x56d360` | `unit_drop_inventory_weapons_except_current` | 158 | 0.35 | 0.5 | 2 | Releases every weapon in the unit's inventory except the currently equipped one, clearing the corresponding slot an... |
| `0x56d400` | `unit_pickup_weapon` | 519 | 0.4 | 0.2 | 13 | Handles a unit picking up a nearby weapon object into a free inventory slot, updating attachment/physics state and... |
| `0x56d610` | `unit_has_weapon_of_type` | 76 | 0.35 | 0.5 | 0 | Returns whether the unit currently carries a weapon of the given type in any inventory slot. |
| `0x56d660` | `unit_find_empty_weapon_slot` | 53 | 0.45 | 0.6 | 0 | Returns the index of the first empty weapon inventory slot, or an invalid index if the unit's inventory is full. |
| `0x56d6a0` | `unit_pick_and_ready_next_weapon` | 63 | 0.3 | 0.6 | 1 | Recomputes which inventory slot should become the unit's next weapon and triggers the weapon-switch routine. |
| `0x56d6e0` | `unit_ready_desired_weapon` | 679 | 0.6 | 0.2 | 15 | Switches the unit from its current weapon to its desired inventory slot (or to being unarmed if none is available),... |
| `0x56d990` | `unit_count_deployed_weapons` | 106 | 0.35 | 0.5 | 0 | Counts how many of the unit's carried weapons are not marked with the 0x10 'undeployed' flag. |
| `0x56da00` | `unit_check_weapon_use_permission` | 113 | 0.35 | 0.4 | 1 | Checks whether the unit's current seat allows using its equipped weapon, consulting an optional scripted permission... |
| `0x56da80` | `unit_lacks_weapon_type_of` | 93 | 0.3 | 0.5 | 1 | Returns whether the unit's inventory does NOT already contain another weapon of the same type as a given reference... |
| `0x56dae0` | `unit_weapon_is_best_of_type` | 189 | 0.4 | 0.3 | 1 | Compares a reference weapon against every weapon the unit carries of the same type, checking whether it is the curr... |
| `0x56dba0` | `unit_find_next_zone_permitted_weapon_slot` | 292 | 0.4 | 0.3 | 0 | Finds the next valid, zone-permitted weapon inventory slot starting from param_1, searching forward or backward dep... |
| `0x56dcd0` | `unit_dispatch_scripted_event_1b` | 221 | 0.3 | 0.25 | 4 | Dispatches a scripted event (id 0x1b) that includes hashed identifiers for both the unit and its currently equipped... |
| `0x56ddb0` | `unit_scripting_set_or_drop_weapon` | 270 | 0.3 | 0.2 | 4 | Script/console-callable helper that resolves weapon-name arguments to object ids and updates or drops the unit's se... |
| `0x56dec0` | `unit_drop_current_weapon` | 260 | 0.55 | 0.3 | 4 | Detaches and drops the unit's current weapon object and selects a replacement desired-weapon slot. |
| `0x56dfd0` | `unit_get_current_weapon_label` | 96 | 0.75 | 0.7 | 0 | Returns a label string for the unit's currently held weapon, or "unarmed" if no weapon is equipped. |
| `0x56e030` | `unit_get_grenade_count` | 44 | 0.5 | 0.8 | 0 | Returns the ammo count for a given grenade-type slot (index in CX) held by the unit. |
| `0x56e060` | `unit_get_current_grenade_index` | 30 | 0.55 | 0.8 | 0 | Returns the index of the unit's currently selected grenade type. |
| `0x56e080` | `unit_begin_throw_grenade` | 472 | 0.5 | 0.2 | 11 | Initiates the unit's grenade-throw sequence: validates the current mode, records timing/aim data, and starts the th... |
| `0x56e280` | `unit_throw_grenade_move_to_hand` | 446 | 0.65 | 0.2 | 10 | Spawns the grenade projectile object for a throw and attaches it to the unit's left-hand marker, advancing the thro... |
| `0x56e440` | `unit_release_thrown_grenade` | 988 | 0.5 | 0.3 | 16 | Detaches the grenade previously attached to the unit's hand, computes its launch velocity, and releases it into the... |
| `0x56e820` | `unit_update_look_delta_controls` | 876 | 0.3 | 0.35 | 2 | Updates a set of clamped 0-1 control values representing the frame-to-frame change of a unit's look/aim reference p... |
| `0x56ebd0` | `unit_set_custom_animation` | 53 | 0.55 | 0.75 | 0 | Sets the unit's active custom animation (graph and index) and resets its playback frame to zero. |
| `0x56ec10` | `unit_reset_light_effect` | 72 | 0.25 | 0.3 | 3 | Performs an unresolved state reset (FUN_004d48d0) and, if a valid effect index is provided, resets its intensity to... |
| `0x56ec60` | `unit_calculate_luminosity` | 157 | 0.6 | 0.45 | 4 | Computes and caches the unit's current light/luminosity value from its RGB color state, or inherits it from a paren... |
| `0x56ed00` | `unit_drop_object_from_hand` | 596 | 0.55 | 0.25 | 14 | Detaches a held object (typically a weapon or grenade) from the unit's hand and releases it into the world with a s... |
| `0x56ef60` | `unit_drop_grenades` | 246 | 0.75 | 0.35 | 3 | Spawns and drops all of the unit's carried grenades of both types into the world (e.g. on death). |
| `0x56f060` | `unit_drop_inventory_weapons` | 141 | 0.75 | 0.5 | 3 | Drops every weapon currently carried in the unit's inventory, clearing each inventory slot as it is dropped. |
| `0x56f210` | `unit_trigger_material_hit_effect` | 183 | 0.4 | 0.4 | 6 | Triggers a material/impact visual effect associated with a given material index or the current unit, used after mel... |
| `0x56f2d0` | `unit_cause_melee_damage` | 626 | 0.85 | 0.45 | 4 | Performs the unit's melee attack: locates the melee marker, resolves the target via a hit test, and applies melee d... |
| `0x56f550` | `unit_melee_attack_scan` | 681 | 0.5 | 0.2 | 11 | Sweeps a small grid of hit tests in front of the unit to find the nearest melee target or surface, then applies dam... |
| `0x56f800` | `unit_can_see_point` | 1151 | 0.55 | 0.1 | 10 | Tests line-of-sight/visibility from the unit to a target point, applying any resulting effects along the trace. |
| `0x56fc80` | `unit_melee_lunge_damage_tick` | 192 | 0.35 | 0.2 | 7 | Applies a periodic melee/lunge damage tick while the unit is in melee sub-state 4, using a swept collision plane test. |
| `0x56fd40` | `unit_exit_seat_end` | 508 | 0.5 | 0.1 | 12 | Checks for and applies collision/crush damage when a unit finishes exiting a vehicle seat. |
| `0x56ff40` | `unit_process_melee_special_interaction` | 512 | 0.3 | 0.2 | 7 | Handles special melee interactions between two units based on weapon-tag flag bits: either applying bonus melee dam... |
| `0x570140` | `unit_detach_from_parent` | 98 | 0.6 | 0.15 | 6 | Detaches the unit from its current parent/attachment object. |
| `0x5701b0` | `unit_get_custom_animation_time_remaining` | 106 | 0.5 | 0.4 | 1 | Returns the number of frames remaining in the unit's currently playing custom animation. |
| `0x570220` | `unit_set_custom_animation_frame` | 124 | 0.5 | 0.35 | 1 | Sets the current playback frame of the unit's active custom animation, if valid. |
| `0x5702a0` | `unit_start_user_animation` | 343 | 0.6 | 0.3 | 4 | Starts (or validates continuation of) a named custom animation on the unit, switching it into custom-animation cont... |
| `0x570400` | `unit_accumulate_clamped_offset` | 89 | 0.3 | 0.6 | 0 | Applies a rate-limited (max 0.3/tick) update to a persistent smoothing value stored on the unit. |
| `0x570460` | `unit_find_weapon_index_with_fixed_flag` | 106 | 0.3 | 0.45 | 0 | Finds the index of the first carried weapon whose tag flags have a specific fixed bit (bit 3) set, or 0xffff if none. |
| `0x5704d0` | `unit_set_throw_aim_direction` | 76 | 0.4 | 0.5 | 1 | Records the unit's grenade-throw aim direction and reference up-vector, unless the unit is currently seated in some... |
| `0x570520` | `unit_find_weapon_index_by_flag` | 116 | 0.4 | 0.5 | 1 | Finds the index of the first carried weapon whose tag flags have the given bit set. |
| `0x5705a0` | `unit_enter_stunned_state` | 175 | 0.45 | 0.4 | 1 | Puts the unit into a disoriented/stunned state: drops its current weapon, sets stun flags, and starts a randomized... |
| `0x570650` | `unit_initialize_random_turn_angle` | 207 | 0.35 | 0.4 | 2 | One-time initialization of the unit's random idle-turn target angle, seeded from its current heading plus a small r... |
| `0x570720` | `unit_update_autoaim_interaction` | 277 | 0.25 | 0.3 | 3 | Updates per-tick interaction flags for the unit and applies an interaction tick against a globally tracked target o... |
| `0x570840` | `unit_update_random_turn_angle` | 644 | 0.45 | 0.3 | 2 | Randomly wanders the unit's idle look/turn angle each tick within clamped bounds and applies the resulting rotation. |
| `0x570ad0` | `unit_get_biped_specific_value` | 46 | 0.25 | 0.5 | 0 | Returns a biped-specific value (delegated to FUN_0055e8e0) only when the unit is a biped; returns 0 for other unit... |
| `0x570b00` | `vehicle_reset_state` | 171 | 0.3 | 0.6 | 0 | Clears a block of the unit's control/animation-tracking state fields, resetting them to zero. |
| `0x570c80` | `unit_get_recently_updated_flag` | 34 | 0.25 | 0.5 | 1 | Tests whether a specific object-flags bit (0x20) is set on the unit, used to gate a network position resync. |
| `0x570cb0` | `unit_propagate_position_delta_to_children` | 184 | 0.4 | 0.45 | 2 | Propagates the unit's positional movement delta to any attached child bipeds/vehicles, updating their cached relati... |
| `0x570d70` | `unit_has_child_of_type5` | 89 | 0.3 | 0.5 | 0 | Returns whether the unit has any attached child object of a specific type (flag bit 5 at 0xb4). |
| `0x570de0` | `unit_set_facing_from_index_table` | 244 | 0.3 | 0.25 | 9 | When a global cinematic/lookup mode is active, orients the unit to face a direction taken from an indexed table. |
| `0x570ee0` | `vehicle_update` | 2531 | 0.85 | 0.15 | 16 | Per-tick update for vehicle-type units: dispatches to per-vehicle-type control/animation calculations, applies exce... |
| `0x571b40` | `unit_throw_grenade_release` | 309 | 0.4 | 0.1 | 5 | Accumulates weighted per-marker force contributions used when releasing a thrown object (e.g. a grenade) from the u... |
| `0x571c70` | `unit_get_tag_flag_bit7` | 55 | 0.3 | 0.4 | 1 | Returns a single boolean flag (bit 7 of the unit tag's flags field) describing an unidentified unit-tag property. |
| `0x571cb0` | `unit_apply_impulse_to_seat` | 286 | 0.4 | 0.4 | 3 | Applies a linear and angular impulse from the unit onto the object it is currently seated in (e.g. a vehicle). |
| `0x571de0` | `unit_predict_aim_target_position` | 296 | 0.3 | 0.35 | 4 | Attempts to compute a projected/predicted aim position in front of the unit for certain sub-types, validating line-... |
| `0x572110` | `unit_spawn_with_starting_weapons` | 760 | 0.3 | 0.2 | 10 | Script/console-callable function that spawns a new unit at a computed placement with an initial orientation/velocit... |
| `0x572b60` | `vehicle_calculate_turret_controls` | 361 | 0.4 | 0.25 | 6 | Computes the dual-axis (pitch/yaw) turret control transform for a vehicle-type unit each tick. |
| `0x572cd0` | `vehicle_calculate_steering_wheel_controls` | 288 | 0.4 | 0.3 | 4 | Computes a single-axis (steering-wheel-style) rotation control transform for a vehicle-type unit each tick. |
| `0x572df0` | `vehicle_calculate_lean_controls` | 779 | 0.35 | 0.15 | 8 | Computes a lean/tilt rotation control transform for a vehicle-type unit based on its current angular velocity. |
| `0x573100` | `vehicle_calculate_ground_lean_controls` | 962 | 0.45 | 0.1 | 7 | Computes ground-hugging lean/roll for a hovering vehicle each tick and triggers its hover/jet thruster particle eff... |
| `0x5734d0` | `vehicle_calculate_wing_flex_controls` | 992 | 0.35 | 0.1 | 16 | Computes per-marker flex/sway transforms (e.g. for wing or control-surface animation) on a flying vehicle-type unit... |
| `0x5738b0` | `vehicle_calculate_hover_turn_controls` | 267 | 0.3 | 0.05 | 3 | Computes hovering-vehicle turn/lift physics each tick and triggers its hover-thruster particle effect; previously m... |
| `0x5739a0` | `vehicle_calculate_hover_lift_toward_target` | 1315 | 0.3 | 0.05 | 3 | Variant of the hovering-vehicle lift/turn physics calculation entered when a target direction is already known, end... |
| `0x573ee0` | `vehicle_calculate_mounted_controls_dispatch` | 119 | 0.4 | 0.3 | 2 | Selects between two ground-contact lean calculations for a mounted/turret-style vehicle unit and triggers its hover... |
| `0x573f60` | `vehicle_calculate_ground_contact_lean` | 1279 | 0.4 | 0.1 | 19 | Computes a ground-contact-relative lean transform for a vehicle when its supporting object's physics type is 2, oth... |
| `0x574460` | `vehicle_calculate_ground_contact_lean_alt` | 798 | 0.35 | 0.1 | 12 | Alternate ground-contact lean calculation for a vehicle, writing its resulting transform into a caller-provided buf... |
| `0x574780` | `unit_update_recoil_decay` | 379 | 0.5 | 0.4 | 2 | Decays the unit's camera/weapon recoil offset toward zero each tick while its recoil countdown timer is active. |
| `0x574900` | `vehicle_create_hover_thruster_effects` | 682 | 0.55 | 0.2 | 5 | Spawns hover/jet-thruster exhaust visual effects at each thruster marker of a vehicle, oriented against the surface... |
| `0x574bc0` | `vehicle_create_hover_thruster_midpoint_effects` | 860 | 0.5 | 0.15 | 5 | Spawns hover-thruster ground-effect visuals positioned at the midpoint between each hover marker and the surface be... |
| `0x574f30` | `unit_update_steering_deviation_effects` | 559 | 0.35 | 0.25 | 4 | Applies a steering-deviation based damage/light-intensity effect while any of the unit's ground-contact markers are... |
| `0x575170` | `unit_update_marker_traction_effects` | 750 | 0.35 | 0.15 | 7 | Updates a per-marker traction/wear value for each of the unit's contact points via surface material tests, triggeri... |
| `0x575460` | `unit_update_marker_skid_effects` | 475 | 0.35 | 0.25 | 2 | Triggers skid/spark effects at each fast-moving ground-contact marker of the vehicle. |
| `0x575640` | `unit_update_ground_contact_counter` | 169 | 0.4 | 0.25 | 1 | Tracks how long the unit has been airborne, resetting the counter and bumping a landing-recovery counter whenever g... |
| `0x5756f0` | `vehicle_calculate_animation_controls` | 1217 | 0.55 | 0.2 | 10 | Evaluates a table of physics-derived control values (speed, turn rate, vertical motion, etc., each normalized to 0-... |
| `0x575c50` | `unit_is_area_clear_of_fast_objects` | 473 | 0.25 | 0.3 | 4 | Checks whether the area around a small set of tracked unit objects is free of other fast-moving objects, likely use... |
| `0x575e30` | `unit_get_average_active_marker_direction` | 365 | 0.3 | 0.15 | 4 | Computes and returns the normalized direction from the unit toward the averaged position of its currently active ma... |

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x559e40` | `biped_update_scale_function_inputs` | 193 | 0.45 | 0.6 | 2 |  |
| `0x559f10` | `biped_reset_state` | 94 | 0.4 | 0.65 | 0 |  |
| `0x559f70` | `biped_clear_ground_surface_references` | 47 | 0.35 | 0.7 | 0 |  |
| `0x561fe0` | `unit_ai_update_stagger_allocate` | 61 | 0.4 | 0.7 | 0 |  |
| `0x562020` | `unit_ai_update_stagger_reset` | 12 | 0.4 | 0.8 | 0 |  |
| `0x562180` | `unit_new` | 987 | 0.5 | 0.45 | 10 | review: added unknown_475 = 0; feign-death roll is random < chance; default team gated on current_game_engine |
| `0x563860` | `unit_update_scale_function_inputs` | 362 | 0.45 | 0.55 | 4 | review: case 3 byte read unsigned (movzx) |
| `0x5643f0` | `unit_update_ik_detail_nodes` | 379 | 0.3 | 0.35 | 5 | review: ECX = &animation_state_flags passed to FUN_00565d60/FUN_00565d00; convention is two stack args |
| `0x56f0f0` | `unit_forget_object_reference` | 203 | 0.4 | 0.75 | 0 | review: weapon-slot helper called as (EAX unit, -1, 0) |
| `0x56f1c0` | `unit_region_damage_reaction` | 67 | 0.35 | 0.6 | 3 | review: callee takes ESI = unit (prototype of the pre-existing callee not yet updated) |

Gate: `python tools/build_check.py units` clean after the cleanup review.
