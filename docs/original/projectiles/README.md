# `projectiles` - the projectile branch of the object hierarchy

Retail Halo PC `halo.exe` 1.0.10, `0x4bda60 .. 0x4c1070`, plain C / MSVC 7.1 / x86. 17 files for
12,047 bytes of code; one function per file, rewritten from its Ghidra decompilation against
`types/projectiles.h`, with the original decompile preserved verbatim at the bottom inside
`#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py projectiles` -> **17 ok, 0 failed** (full repo: 890 ok, 0
failed).

Ghidra lists 22 entry points in the range. Five of them are **not** projectile functions (or are
not functions at all) and are deliberately absent; see "Misattributed functions" below.
17 + 5 = 22.

Read the confidence column in the function table before trusting a file. This module decompiles
badly. Six of the seventeen functions decompile as `undefined FUN_...(void)` with **no recovered
arguments at all**, and several more recover only some: the object index arrives in `EAX`, `EBX`,
`ECX` or `EDI` depending on the function, and a long list of callees take pointers in
`ECX`/`EDX`/`ESI`/`EDI` that Ghidra prints as empty argument lists. Every file records its own
register convention in a `blam-cc:` line, and the verification pass re-derived those (and the
semantic fixes listed below) from `objdump -d -M intel bin/halo.exe` rather than from the
decompilation.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| detonation networking | `0x4bda60`-`0x4bdb40` | message `0x30`: the sender forces `network_role = 3`, the receiver repositions, detonates, raises the state and deletes |
| per-tick integrator | `0x4bdc00` | `projectile_update`: contrail retirement, the arming / deceleration-delay / detonation timers, guided steering with a periodic "wander", velocity decay against the tag damage range, gravity, the range limit, up to ten collision sub-steps, the flyby-sound listener, the rotation spin, and the terminal detonate/delete |
| collision response | `0x4bf0f0`-`0x4bf390` | `projectile_request_state` (the state word is only ever raised), the attach message pair `0x33`, and `projectile_response` - the whole `ProjectileResponse` machine against `ProjectileMaterialResponse` |
| per-tick helpers | `0x4c0180`-`0x4c0450` | the rotation axis/sine/cosine, the four tag function-in values, the two deceleration solvers, and the three-ray collision sweep |
| detonation | `0x4c0670` | `projectile_detonate`: the super-combining sibling sweep, the attached-detonation damage, and the tag / material detonation effects |
| creation + delta networking | `0x4c0b10`-`0x4c1070` | message `0x1e` (creation) and the delta codec pair driven off the type row's message index |

Projectiles are an `object` extension, not a separate record. The `object_type_definition` row at
`0x0069b9a0` fixes it: the `projectile` row's `object_size` is `0x2b0`, its subdefinition chain is
`{object, projectile}`, and the `object` row's size is `0x1f4` - so `projectile_data` starts at
object `0x1f4` and is exactly `0xbc` bytes. It therefore **overlaps `item_data`**, which is why
these fields live in `types/projectiles.h` and not in `types/items.h`; do not mix the two.
`k_projectile_data_offset` and `k_projectile_object_size` in `types/projectiles.h` are those two
constants.

Two network hash tables run through this module, the same pair `src/items` documents and under
the same names. `object_pooled_node_globals` (`0x00687130`) and `network_message_table_b`
(`0x00687558`) are parallel; `+0x0c` off each is the `hash_table` `hash_table_get` turns an object
`datum_index` into a small hash with, and `+0x28` off each is the `datum_index` array the receiver
turns that hash back through. The senders and receivers pair up exactly: `creating_object_hash`
goes through `0x00687130` and lands in `object_placement_data.role`, and `owner_hash` goes through
`0x00687558` and lands in `object_placement_data.owner_linkage`.
`object_pooled_node_globals_006870d8` (`0x006870d8`) is a *different* global - the pooled-node
block `FUN_004e9cd0` / `FUN_004e9d40` bind and unbind objects in.

The float pool this module reads its thresholds from, all verified against the image:
`0x00672ac0` = `0.0`, `0x00672abc` = `0.5`, `0x00672ac4` = `1.0`, `0x00672ac8` = `30.0`,
`0x00672acc` = `1/30`, `0x00672bbc` = `0.0001` (the at-rest squared speed), `0x00672c94` = `0.3`
(the ground-normal test), `0x00672f20` = `0.99`, `0x00696140` = `0.2` (the network position
tolerance) and `0x0069c52c` = `0.00356518` (gravity, world units per tick squared).

## Struct layouts

All of these live in `types/projectiles.h`. Offsets are byte offsets from the struct base under
`#pragma pack(push,1)` and are checked mechanically against the header with an `offsetof` probe.
Types this module uses but does not own - `data_array`, `datum_index` (`types/memory.h`),
`real_point3d`, `real_vector3d` (`types/math.h`), `tag_instance` (`types/cache.h`), `object`,
`object_header`, `object_type_definition`, `object_placement_data`, `bsp_leaf_reference`,
`damage_data` (`types/objects.h`), `unit_data` (`types/units.h`) and every tag structure
(`types/tags.h`) - are not redefined here.

### `projectile_data` - size `0xbc`, at object `0x1f4`

The struct's own offsets are struct-relative; the "obj" column is the object-relative offset the
decompiled module uses, and is what every other file in the repo quotes.

| Off | obj | Type | Field |
|---|---|---|---|
| `0x00` | `0x1f4` | `uint8_t[0x38]` | `unknown_1f4` - overlaps `item_data`; no function in this module reads it |
| `0x38` | `0x22c` | `uint32_t` | `flags` - `projectile_flags` |
| `0x3c` | `0x230` | `int16_t` | `state` - `projectile_state`, raised only, by `projectile_request_state` |
| `0x3e` | `0x232` | `int16_t` | `material_response_index` - index into the tag's `projectile_material_response` |
| `0x40` | `0x234` | `datum_index` | `ignore_object_index` - the object the sweep skips |
| `0x44` | `0x238` | `datum_index` | `tracked_object_index` - the guidance target |
| `0x48` | `0x23c` | `int32_t` | `contrail_attachment_index` - index into `object.attachment_handles` |
| `0x4c` | `0x240` | `float` | `detonation_timer` - `0..1`; at `1.0` the state is raised to detonating |
| `0x50` | `0x244` | `float` | `detonation_timer_rate` - `1/(seconds*30)` |
| `0x54` | `0x248` | `float` | `arming_timer` - `0..1`, advanced every tick |
| `0x58` | `0x24c` | `float` | `arming_timer_rate` - `1/(Projectile.arming_time*30)` |
| `0x5c` | `0x250` | `float` | `distance_travelled` - world units, against `Projectile.maximum_range` |
| `0x60` | `0x254` | `float` | `deceleration_delay` - `0..1` ramp; decay is skipped below `1.0` |
| `0x64` | `0x258` | `float` | `deceleration_delay_rate` |
| `0x68` | `0x25c` | `float` | `deceleration` - world units per tick squared |
| `0x6c` | `0x260` | `float` | `deceleration_end_range` |
| `0x70` | `0x264` | `real_vector3d` | `rotation_axis` - normalized `object.angular_velocity` |
| `0x7c` | `0x270` | `float` | `rotation_sine` |
| `0x80` | `0x274` | `float` | `rotation_cosine` |
| `0x84` | `0x278` | `uint8_t` | `thrown_grenade` - gates the detonation broadcast |
| `0x85` | `0x279` | `uint8_t` | `network_state_valid` |
| `0x86` | `0x27a` | `uint8_t` | `network_baseline_index` |
| `0x87` | `0x27b` | `uint8_t` | `network_sequence` - wraps, reset to 0 when it reaches `0xff` |
| `0x88` | `0x27c` | `projectile_network_state` | `network_state` - the outgoing baseline |
| `0xa0` | `0x294` | `uint8_t` | `last_update_valid` |
| `0xa1` | `0x295` | `uint8_t[3]` | `pad_295` |
| `0xa4` | `0x298` | `projectile_network_state` | `last_update_state` |

### `projectile_network_state` - size `0x18`

| Off | Type | Field |
|---|---|---|
| `0x00` | `real_point3d` | `position` |
| `0x0c` | `real_vector3d` | `velocity` |

### `collision_result` - size `0x50`

The collision module's output record. `projectile_collision_test` fills it, `projectile_response`
consumes it, and `projectile_update` owns the buffer.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `type` - `collision_result_type` (2 = structure surface, 3 = object, water surface has its own value) |
| `0x02` | `int16_t` | `unknown_02` |
| `0x04` | `uint8_t[8]` | `unknown_04` |
| `0x0c` | `bsp_leaf_reference` | `leaf` - forwarded to `object_set_cluster_and_parent` and to the breakable-surface damage call |
| `0x14` | `float` | `t` - fraction of the swept segment consumed; the integrator uses `1 - t` |
| `0x18` | `real_point3d` | `point` - contact point; also the impact-noise origin |
| `0x24` | `real_vector3d` | `normal` - `normal.k > 0.3` is the ground test |
| `0x30` | `float` | `unknown_30` - never read by this module |
| `0x34` | `int16_t` | `material_type` - seeds `projectile_data.material_response_index` |
| `0x36` | `int16_t` | `unknown_36` |
| `0x38` | `datum_index` | `object_index` - the object hit, `-1` for a structure surface |
| `0x3c` | `int16_t` | `unknown_3c` - `object_apply_damage`'s fourth argument |
| `0x3e` | `int16_t` | `marker_index` - the attachment marker, and `object_apply_damage`'s third argument |
| `0x40` | `uint32_t` | `unknown_40` - never read by this module |
| `0x44` | `int32_t` | `surface_index` - forwarded to `FUN_004ffde0` |
| `0x48` | `uint32_t` | `unknown_48` - never read by this module |
| `0x4c` | `uint8_t` | `surface_flags` - bit `0x08` = the surface can be broken |
| `0x4d` | `uint8_t` | `unknown_4d` - packed into the low half of `FUN_004ffde0`'s first argument |
| `0x4e` | `int16_t` | `unknown_4e` - `object_apply_damage`'s fifth argument |

### `projectile_creation_message` - size `0x54`, message `0x1e`

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `definition_tag` |
| `0x04` | `int32_t` | `object_hash` |
| `0x08` | `int16_t` | `name_index` |
| `0x0a` | `uint8_t[2]` | `pad_0a` - never written by the sender |
| `0x0c` | `int32_t` | `owner_hash` - through `player_network_id_table`, lands in `object_placement_data.owner_linkage` |
| `0x10` | `int32_t` | `creating_object_hash` - through `object_network_id_table`, lands in `object_placement_data.role` |
| `0x14` | `real_point3d` | `position` - from `network_state.position` |
| `0x20` | `real_vector3d` | `forward` |
| `0x2c` | `real_vector3d` | `up` |
| `0x38` | `real_vector3d` | `velocity` - from `network_state.velocity` |
| `0x44` | `real_vector3d` | `angular_velocity` |
| `0x50` | `uint8_t` | `baseline_index` |
| `0x51` | `uint8_t[3]` | `pad_51` |

### `projectile_detonation_message` - size `0x10`, message `0x30`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `object_hash` |
| `0x04` | `real_point3d` | `position` |

### `projectile_attach_message` - size `0x0a`, message `0x33`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `object_hash` - the projectile |
| `0x04` | `int32_t` | `parent_hash` - the object it stuck to |
| `0x08` | `int16_t` | `parent_marker_index` |

### `projectile_network_update_header` - size `0x07`

The sub-record at `update_record + 0x44` (i.e. `update_record[0x11]`) in the generic
message-delta record.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `object_hash` |
| `0x04` | `uint8_t` | `baseline_index` |
| `0x05` | `uint8_t` | `sequence` |
| `0x06` | `uint8_t` | `is_delta` |

### Enumerations

`projectile_flags` (`projectile_data.flags`, object `0x22c`): `0x01` rotation_valid, `0x02`
tracer, `0x04` hit_ground, `0x08` attached, `0x10` at_rest, `0x20` detonation_timer_started,
`0x40` super_detonation_counted, `0x80` super_detonation.

`projectile_definition_flags` (the `ProjectileFlags` bitfield at `Projectile` tag `0x17c`, added
to `types/projectiles.h` this pass because `types/tags.h` carries the names only in a comment):
`0x01` oriented_along_velocity, `0x02` ai_must_use_ballistic_aiming, `0x04`
detonation_max_time_if_attached, `0x08` has_super_combining_explosion, `0x10`
combine_initial_velocity_with_parent, `0x20` random_attached_detonation_time, `0x40`
minimum_unattached_detonation_time.

`projectile_state` (`projectile_data.state`, object `0x230`): 0 flying, 1 detonating,
2 disappearing.

`collision_result_type` and `projectile_constants` (the message ids, the collision masks, the
super-combine thresholds and `k_projectile_maximum_collisions_per_tick = 10`) are in the same
header.

## Functions

`name` is the file's own name confidence, `rw` its rewrite confidence, `U` the number of
`UNSURE:` annotations left in the live code.

| Address | File | name | rw | U |
|---|---|---|---|---|
| `0x4bda60` | `projectile_send_detonation.c` | 0.75 | 0.70 | 0 |
| `0x4bdb40` | `projectile_detonation_message_apply.c` | 0.75 | 0.85 | 0 |
| `0x4bdc00` | `projectile_update.c` | 0.85 | 0.70 | 22 |
| `0x4bf0f0` | `projectile_request_state.c` | 0.90 | 0.95 | 0 |
| `0x4bf120` | `projectile_send_attach.c` | 0.80 | 0.75 | 0 |
| `0x4bf1c0` | `projectile_attach_apply.c` | 0.80 | 0.80 | 1 |
| `0x4bf390` | `projectile_response.c` | 0.85 | 0.65 | 3 |
| `0x4c0180` | `projectile_compute_rotation.c` | 0.80 | 0.90 | 0 |
| `0x4c0250` | `projectile_update_function_values.c` | 0.80 | 0.90 | 0 |
| `0x4c0310` | `projectile_compute_deceleration.c` | 0.70 | 0.90 | 0 |
| `0x4c03f0` | `projectile_deceleration_from_range.c` | 0.70 | 0.90 | 0 |
| `0x4c0450` | `projectile_collision_test.c` | 0.70 | 0.80 | 1 |
| `0x4c0670` | `projectile_detonate.c` | 0.70 | 0.60 | 17 |
| `0x4c0b10` | `projectile_send_creation.c` | 0.70 | 0.85 | 2 |
| `0x4c0ca0` | `projectile_create_from_network.c` | 0.70 | 0.75 | 5 |
| `0x4c0f30` | `projectile_build_network_update.c` | 0.70 | 0.80 | 3 |
| `0x4c1070` | `projectile_apply_network_update.c` | 0.80 | 0.85 | 5 |

`projectile_update` and `projectile_detonate` still carry most of the module's uncertainty, but
for different reasons: `projectile_update`'s remaining `UNSURE`s are all about the *guided-steering
"wander" block* and its three opaque foreign callees (`FUN_0046fe10`, `FUN_00569280`,
`FUN_00545460`) plus the `FUN_00543d80` sound bundle, while the integrator itself is now derived
instruction-by-instruction. `projectile_detonate`'s are about the two effect spawns through the
12-argument `FUN_00450980` and the four opaque objects-module calls the super-detonation relink
makes (`FUN_004f6610`, `FUN_004f7b70`).

### Corrections the verification pass made

Each of these is a behaviour change, not a comment change, and each is cited in the file at the
line it fixes.

| File | Was | Is |
|---|---|---|
| `projectile_update.c` | after a collision response, the settle tail ran when the projectile *was* attached | inverted: `0x4be577 test al,8 / je LAB_004be5c1` runs the tail when it is **not** attached |
| `projectile_update.c` | the post-collision velocity rescale divided by the range check's average speed (`fStack_130`) | it divides by `fStack_13c` (`0x4be4ef fdiv [esp+0x34]`), now a separate `speed_after_decay` local |
| `projectile_update.c` | the velocity-floor blend scaled its first term by `final_velocity * 0.99` | `0x4be1ce fmul [esi+0x1e8]`: the first term uses the unscaled `final_velocity` |
| `projectile_update.c` | `projectile_detonate(index, first_collision, 0)` | the third argument is `remaining_fraction` (`0x4bead1 mov edx,[esp+0x18]`) |
| `projectile_update.c` | `FUN_0042c610(index, 0, 1, impact_noise)` | five arguments, and the second is `&hit.point` (the buffer is `collision_result + 0x18`) |
| `projectile_update.c` | `FUN_0044ca60(0)` | `FUN_0044ca60(0, (1 - remaining) / 30)`, with the contrail handle in `EDI` |
| `projectile_response.c` | the velocity-noise rescale dropped the pre-normalization length | the scale is `|velocity| + uniform(-vn, +vn)`; without the length the speed collapsed to the noise |
| `projectile_response.c` | `damage_data.unknown_4c` and `.location_cluster_index` left at 0 | both start at `-1`, and `unknown_4c` is **read back** after `object_apply_damage` |
| `projectile_response.c` | `fade_out = dd.multiplier` (`+0x44`) | `0x4bf5b6` reads `damage_data + 0x48` |
| `projectile_response.c` | the chosen effect tag was stored in an `int16_t` | it is a full `datum_index`; truncation silently broke every response effect |
| `projectile_response.c` | the effect scale was the clamped speed fraction for every `scale_effects_by` | only values 0 and 1 assign it (and only they clamp); anything else leaves it at `1.0` |
| `projectile_response.c` | the breakable-surface record was built and dropped | it rides in `EBX` (`0x4bf8d2 lea ebx,[esp+0xa4]`), and also carries the surface's material index and its `ProjectileMaterialResponse` row |
| `projectile_response.c` | `alignment_score` used `-velocity_noise * r` | `+velocity_noise * r - velocity_noise` |
| `projectile_response.c` | `vector3d_project_onto_axis()` with no arguments | the canonical four (`ECX` parallel out, `EDX` axis, `ESI` v, `EDI` perpendicular out); this also settles which output `parallel_friction` scales |
| `projectile_collision_test.c` | the second offset ray was computed and never passed | `FUN_00401a20` takes two **points** in `ECX`/`EAX` and subtracts them itself |
| `projectile_collision_test.c` | returned `uint32_t` | `AL` only (`xor al,al` / `mov al,1`), and the caller tests a byte |
| `projectile_apply_network_update.c` | the local was a *pre-decode snapshot* restored afterwards | it is the decoder's **out-parameter** (`0x4c1127 lea ecx,[esp+0x10]`); everything applied is the decoded state |
| `projectile_detonate.c` | `FUN_0042c610()` with no arguments | `(object_index, &position_block[0], 2, tag->detonation_noise, 1)` |
| `projectile_detonate.c` | the super-detonation relink reused the effect position block | it has its own point buffer (`local_90`) |
| `projectile_detonate.c` | `damage_data.unknown_4c` left at 0 | starts at `-1` |
| `projectile_create_from_network.c` | `FUN_004e9cd0(object_hash)` | `(pooled_node_globals, new_object_index, object_hash)`, the globals being the literal `0x006870d8` |

`src/items/weapon_apply_network_update.c` (`0x4c6070`) still carries the pre-decode-snapshot
misreading this pass corrected for `0x4c1070`. It is the same code shape and almost certainly the
same bug; it was left alone because it is outside this module.

## Misattributed functions

Five of Ghidra's 22 entry points in the range are absent from `src/projectiles/`:

| Address | Ghidra name | Why it is not here |
|---|---|---|
| `0x4be1b0` | `resolution_list_add_resolution` | not a function: a label inside `projectile_update` that Ghidra promoted. Zero callers, no prologue, and both decompilations print the same `LAB_004be5ad` / `LAB_004be5c1`. Fully absorbed into `projectile_update.c` |
| `0x4beb30` | `FUN_004beb30` | AI ballistic aiming: the swept/gravity firing-solution solver. Takes a `Projectile` **tag** pointer and plain vectors, never an object index |
| `0x4bee20` | `FUN_004bee20` | AI ballistic aiming: the straight-line solver |
| `0x4beec0` | `FUN_004beec0` | AI ballistic aiming: picks `0x4beb30` when the tag has `ai_must_use_ballistic_aiming` and a positive `air_gravity_scale`, else `0x4bee20` |
| `0x4bef80` | `FUN_004bef80` | a generic `object_apply_impulse_and_spin`-shaped helper: touches only `object.velocity`, `object.angular_velocity` and object flag `0x20`, plus `0x4c0180`. Owns nothing in `projectile_data` |

## Known gaps

- **The guided-steering "wander" block** in `projectile_update` (`0x4bde40`-`0x4be120`) reaches
  three functions this project has not touched: `FUN_0046fe10` (`0x4bdea5`; reads a word at
  `+0x0e` off the difficulty/skill globals at `0x006b0b80` and returns a float multiplier for the
  turn rate), `FUN_00569280` (`0x4bdf73`, zero visible arguments, called between the proximity
  ramp and the two `periodic_function_evaluate` draws) and `FUN_00545460` (`0x4be687`; turns the
  flyby sound's tag id into a radius). The arithmetic around them is derived; their signatures are
  not.
- **`FUN_00543d80`**, the flyby-sound bundle, is passed a stack block whose shape is guessed by
  analogy with `src/items/item_update.c`. It is written as a local anonymous struct and flagged.
- **`FUN_00450980`**'s trailing four arguments (`0, 0, 0, 1` at every call site in this module)
  and `FUN_00450870`'s nine are taken from the established 12-argument form in
  `src/hs/hs_effect_spawn_at_location.c`; the two `0.0` slots before them are the effect's fade
  in/out pair only by inference.
- **`object_set_cluster_and_parent`'s second argument** in `projectile_update` is a
  `bsp_leaf_reference` the function never initializes - a genuine leftover-stack read in the
  original, reproduced as an uninitialized local.
- **`object` fields `0x018`, `0x01c`, `0x044` and `0x048`** that
  `projectile_apply_network_update` writes are inside `types/objects.h`'s unresolved
  `unknown_019` / `player_visibility_mask` / `unknown_022` region and are still written as raw
  offsets. `out/phase4/projectiles_types_notes.md` calls them an interpolation block.
- **`damage_data.flags` bit `0x08`** is set by three call sites in this module and has no name in
  `types/objects.h`.
- **`projectile_data.unknown_1f4[0x38]`** (object `0x1f4`-`0x22c`) is read by nothing in this
  module. It is the range `item_data` also claims, so whatever initializes it does so elsewhere.
- **`projectile_new` (`0x4bd7c0`), `projectile_notify_object_deleted` (`0x4bf0c0`),
  `projectile_force_detonate` (`0x4c0ac0`), `projectile_network_baseline_take` (`0x4c0ed0`)** and
  **`projectile_is_old_enough` (`0x4c1270`)** are on the projectile function table but outside
  this address range; they were rewritten in cleanup pass 1 (table at the end of this file). `types/projectiles.h` cites them for the field
  semantics they establish (notably the initial state and the in-water probe).

## Types changed outside `types/projectiles.h`

`types/objects.h` gained two names in its `object_flags` enum, folded in from this module's
`TYPES-GAP` locals and from `out/phase4/projectiles_types_notes.md`:

- `_object_in_water_bit = 0x00000010` - `projectile_new` probes the medium and sets or clears it;
  `projectile_compute_deceleration` and `projectile_update`'s gravity pick branch on it, and the
  water overpenetrate response in `projectile_response` toggles it.
- `_object_took_network_update_bit = 0x08000000` - the gate and the set in
  `projectile_apply_network_update` and its weapon sibling.

No local `enum`/`typedef` workarounds remain in `src/projectiles/`.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x4bd7c0` | `projectile_new` | 652 | 0.75 | 0.55 | 6 | review: timer-rate tests follow the x87 compare (unordered divides) |
| `0x4bf0c0` | `projectile_notify_object_deleted` | 48 | 0.7 | 0.85 | 0 |  |
| `0x4c0ac0` | `projectile_force_detonate` | 71 | 0.7 | 0.85 | 0 |  |
| `0x4c0ed0` | `projectile_network_baseline_take` | 90 | 0.7 | 0.85 | 0 |  |
| `0x4c1270` | `projectile_is_old_enough` | 57 | 0.7 | 0.85 | 0 |  |
| `0x571dd0` | `object_type_definition_return_false` | 2 | 0.4 | 0.9 | 0 |  |
| `0x572a80` | `object_type_definition_return_true` | 2 | 0.4 | 0.9 | 0 |  |

Gate: `python tools/build_check.py projectiles` clean after the cleanup review.
