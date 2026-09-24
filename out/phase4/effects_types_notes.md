# effects module: type recovery notes

Header: `types/effects.h` (976 lines). Smoke test: `out/phase4/effects_smoke.c`, built with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/effects_smoke.c
```

It passes. It carries negative-array size assertions for all 24 structs plus ~190 `offsetof`
assertions: the runtime records, the tag offsets every attribution rests on, the `object` fields
the module reaches through, and three span identities (`k_decal_layers * k_decal_grid_clusters *
4 == 0x2800`, `0x1c + 8 * 0x10 == 0x9c`, `0x58 + 4 * 0x40 == 0x158`) so a field reordering fails
the build instead of passing silently. The other eleven phase-4 smoke files still compile
unchanged (`cache`, `devices`, `game`, `hs`, `items`, `math`, `objects`, `physics`,
`projectiles`, `tags`, `units` all re-verified).

The smoke file includes `tags.h`, `memory.h`, `math.h`, `objects.h`, `effects.h` — a superset of
the three the task named, matching what `projectiles_smoke.c` already does, because the header
reuses `bsp_leaf_reference` / `object_marker` / `object` from `objects.h` and
`real_point3d` / `real_vector3d` / `real_matrix4x3` from `math.h` rather than redefining them.

---

## The binary evidence everything hangs on

### 1. The two `game_state_new` calls fix the contrail and decal tables

`contrails_initialize` 0x44c8b0 and `decals_initialize` 0x44df90 are the only two initializers in
the range, and they are the only place a capacity appears in the binary:

| call | name | capacity | element stride seen at every call site |
|---|---|---|---|
| 0x44c8b0 | `"contrail"` | 0x100 | 0x44 |
| 0x44c8b0 | `"contrail point"` | 0x400 | 0x38 |
| 0x44df90 | `"decals"` | 0x800 | 0x38 |

`decals_initialize` also sets `data_array.valid` (+0x25) to 1 directly, carves `0x280c` bytes out
of the game-state cursor at 0x006e2dcc, folds that size into the CRC at 0x006e2dd4, and stores the
block pointer at 0x006b0ad8. `0x280c` is exactly `5 * 0x200 * 4 + 4 + 4 + 4`, which is what fixed
`decal_grid` as a `[layer][cluster]` head table and not an `(x, y)` spatial hash — the phase-2
summaries call it a "spatial grid", which is wrong.

### 2. Three .rdata dispatch tables fix three enum widths

Read out of `.rdata` (PE image base 0x400000, `.rdata` VA 0x63a000 → file offset 0x23a000):

| address | entries | procedures | matches |
|---|---|---|---|
| 0x0065743c | 2 | 0x4552a0, 0x4554d0 | `ParticleSystemSystemUpdatePhysics` (2 values) |
| 0x00657444 | 3 | 0x455310, 0x4554e0, 0x455610 | `ParticleSystemParticleCreationPhysics` (3) |
| 0x00657450 | 1 | 0x455350 | `ParticleSystemParticleUpdatePhysics` (1) |

The call sites are `particle_system_update` 0x4544f0 (`ParticleSystem.system_update_physics` at
tag +0x48), `particle_system_spawn` 0x453b10 (`ParticleSystemTypeStates.particle_creation_physics`
at +0xb0) and `particle_system_update` again (`particle_update_physics` at +0xb2).

**All six procedures are inside this address range and Ghidra created no function for any of
them.** They own `particle_system_particle` 0x1c..0x40 (position, `unknown_28`, direction), which
is why those three fields are the weakest part of that struct.

### 3. `decal_type_parameters` at 0x006573f8

Four rows of 0x10 bytes, `{40.0, 110.0, 1.5, 1}` three times then `{10.0, 10.0, 1.5, 0}`, indexed
by `DecalType` (scratch / splatter / burn / painted_sign). `decal_flood_surfaces` 0x44e730 reads
+0x00 through the `0.017453292` degrees-to-radians literal as the maximum angle it will wrap a
decal across, and +0x08 as the radius multiplier for the `ray_intersects_sphere_test` that bounds
the flood.

### 4. `effect.tint_source` is a closure, not a vector

The decompiler emits two mutually exclusive readings: `effect_set_placement` 0x451600 copies three
dwords from a caller vector into +0x30/+0x34/+0x38, while `effect_spawn_particles` 0x451f90 calls
`+0x34` as a procedure. The raw code settles it. `effect_set_placement` at 0x0045164b clears only
+0x34 and +0x38 on the null path, leaving +0x30 alone, and 0x004526ca is:

```
4526ca: mov  eax,[edi+0x34]
4526cd: test eax,eax
4526cf: je   4526e9            ; fall back to global_origin3d_pointer
4526d1: mov  ecx,[edi+0x30]    ; the closure data
4526d4: push ecx
4526d5: lea  edx,[esp+0x88]    ; the spawn position
4526dc: push edx
4526dd: lea  ecx,[esp+0x60]    ; the out colour
4526e1: push ecx
4526e2: call eax
```

so the triple is `{void *data; void (*proc)(ColorRGB*, real_point3d*, void*); uint32_t unknown;}`.
Both fields are declared `uint32_t` in the header so the 0x0c size holds under a 64-bit host
compiler; the comment gives the real signature.

### 5. `decal.definition_index` at +0x2c is the tag, not an object

Ghidra prints `*(uint *)(iVar11 + 0x2c) = param_1;` after it has already reused `param_1` as a
loop counter, which reads as an object handle. The raw code at 0x004502b4 is
`mov edx,[ebp+0x8]; mov [esi+0x2c],edx`, and `[ebp+0x8]` is the same value 0x0044edee resolves tag
data from (`and esi,0xffff; shl esi,5; mov ebx,[esi+edx+0x14]` with `edx == 0x0087bc14`). So
+0x2c holds the `deca` tag datum index. There is no object handle in a `decal`: the object-attached
list is rehashed by re-probing `decal.position` in `decal_rehash_object_decals` 0x44e000.

### 6. `particle_system_particle.position` at +0x1c

`lea edx,[edi+0x1c]` at 0x00454ccc is the point handed to `matrix4x3_transform_point`.

### 7. The weather instance array is one element

The next referenced global after 0x006b0ae4 is 0x006b0b80, which is 0x006b0ae4 + 0x9c. Inside the
instance, `0x9c - 0x1c == 8 * 0x10`, which is the per-particle-type block count.

### 8. The ambient noise grid is exactly 0x900 bytes

It starts at 0x00746284 and the next global, the weather palette count, is at 0x00746b84.
`0x900 == 3 * 8 * 8 * 0x0c`. `ambient_color_randomize` 0x53fa70 walks it with an outer loop of 8
stepping 0x60 (a row) and an inner loop of 3 stepping 0x300 (a band), and `ambient_color_sample`
0x53fc80 indexes it flat as `((hash & 0x3f) + band * 0x40) * 3` floats, which is the same layout
read two ways.

### 9. `player_effect` is driven by `ContinuousDamageEffect`, not `DamageEffect`

This is the single most useful correction in the batch. `player_effect_apply_continuous_damage`
0x4567c0 reads its tag at +0x00/+0x04 (a distance falloff pair), +0x24, +0x28, +0x44, +0x48, +0x5c
and +0x60. Against `DamageEffect` +0x24 is the screen-flash `type`/`priority` short pair, which
cannot be a float multiplier. Against `ContinuousDamageEffect` every one lands on a named field:

| tag offset | `ContinuousDamageEffect` field | destination in `player_effect` |
|---|---|---|
| 0x00, 0x04 | `radius[2]` | the `1 - (d - r0) / (r0 - r1)` falloff |
| 0x24 | `low_frequency_vibrate_frequency` | +0xcc |
| 0x28 | `high_frequency_vibrate_frequency` | +0xd0 |
| 0x44 | `camera_shaking_random_translation` | +0xd4 |
| 0x48 | `camera_shaking_random_rotation` | +0xd8 |
| 0x5c | `camera_shaking_wobble_period` | `periodic_function_evaluate(now / period)` |
| 0x60 | `camera_shaking_wobble_weight` | `(1 - w) + result * w` |

### 10. `0x006f1884` is a pointer, not the array

Every access is `index * 0xec + DAT_006f1884` or `*(short *)(DAT_006f1884 + 0xfc)`, i.e. the
global holds an address. With one local player on PC the record occupies 0x000..0x0ec and the
fields at 0x0ec..0x124 are the scripted whole-screen effect the Hs `player_effect_set_*` family
drives, which is why they are singletons rather than per-player.

### 11. Correcting the tag offsets the phase-2 packs implied

Two `tags.h` offsets I had to re-derive because the code disagreed with my first reading:

- `EffectEvent.parts` is at **0x2c** and `particles` at **0x38**, not 0x30/0x3c.
  `object_change_color_evaluate` 0x4529d0 reads the part count at `+0x2c` with stride 0x68, and
  `effect_update` 0x451a30 reads the particle count at `+0x38` with stride 0xe8.
- `FUN_00450fa0` appears to read `Effect + 0x04` as a float radius, which would collide with
  `loop_start_event`. It does not: the base is a `short *`, so `*(float *)(local_20 + 4)` is byte
  offset 8, i.e. `maximum_damage_radius`. No correction needed.

---

## Which function established which field

### `contrail` (0x44, table 0x0087abec)

| field | offset | established by |
|---|---|---|
| `identifier` | 0x00 | datum header convention |
| `flags` | 0x02 | `contrail_new` 0x44c910 sets bit 0; `contrail_update` 0x44cb50 sets/clears it against `object.function_valid_flags` |
| `definition_index` | 0x04 | every function: `(*(uint*)(rec+4) & 0xffff) * 0x20 + 0x14 + tag_index_base` |
| `object_index` | 0x08 | `contrail_new` (from ECX), `contrail_advance` 0x44ca60 writes -1 to detach |
| `attachment_index` | 0x0c | `contrail_generate_points` 0x44d020 uses it as `Object tag +0x144 + idx * 0x48 + 0x10` for `object_get_node_local_transform` |
| `scale_function_index` | 0x0e | `contrail_new`: `ObjectAttachment.primary_scale (+0x30) - 1` |
| `scale` | 0x10 | `contrail_new` / `contrail_update`: `object.function_out_values[scale_function_index]`, 1.0 when -1 |
| `sequence_index` / `frame_index` | 0x14 / 0x16 | `contrail_next_sequence` 0x44ced0, against `Contrail.first_sequence_index` +0x40, `sequence_count` +0x42 and `BitmapSequence.bitmap_count` +0x22 |
| `texture_offset_u` / `_v` | 0x18 / 0x1c | `contrail_update`, from `Contrail.texture_animation_u` +0x24 and `_v` +0x28 with scale-flag bits 8 and 9 |
| `generation_timer` | 0x20 | `contrail_points_due` 0x44cf80, reloaded with `1 / Contrail.point_generation_rate` (+0x04, scale-flag bit 0) |
| `animation_timer` | 0x24 | `contrail_update` against `1 / Contrail.animation_rate` (+0x2c, scale-flag bit 5); zeroed by `contrail_next_sequence` |
| `accumulated_delta_time` | 0x28 | `contrail_advance` adds, `contrail_update` subtracts and zeroes |
| `point_count[4]` | 0x2c | `contrail_generate_points` increments, `contrail_age_points` 0x44d470 decrements |
| `first_point[4]` | 0x34 | `contrail_new`, `contrail_generate_points`, `contrail_delete` 0x44cad0, `contrail_age_points` |

### `contrail_point` (0x38, table 0x0087abe8)

`contrail_generate_points` 0x44d020 writes every field at create; `contrail_age_points` 0x44d470
owns the state machine. The `flags` bits were fixed by which branch reads
`ContrailPointState.duration` (+0x00) versus `transition_duration` (+0x08) and which scale-flag
bits guard each (bits 0/1 for duration, 2/3 for the transition — exactly the
`ContrailPointStateScaleFlags` order in `tags.h`). `location` at 0x14 is a `bsp_leaf_reference`
because `contrail_refresh_lightmap` 0x44cda0 rewrites it with the `bsp3d_node_find_leaf` +
`ScenarioStructureBSPLeaf.cluster` pair `objects.h` already documents.

### `decal` (0x38, table 0x0087abe4) and `decal_grid` (0x280c, 0x006b0ad8)

`decal_place` 0x44edc0 writes position, `creation_game_time`, `sequence_index`, `lifetime`,
`decay_time`, `color`, `alpha`, `triangle_count` and `definition_index`; `decal_new` 0x44dd90 and
`decal_link` 0x44dd30 own `flags`, `cluster_index`, `layer` and the list links; `decal_update_fade`
0x44dc30 recomputes `alpha`; `decal_delete` 0x44e3c0 and `decal_rehash_object_decals` 0x44e000
unlink and relink.

`cluster_index` / `layer` rather than `(cell_x, cell_y)`: `decal_rehash_object_decals` sets +0x04
from `bsp3d_node_find_leaf` → `ScenarioStructureBSPLeaf.cluster`, and `decal_new` sets +0x06 from
`Decal.layer` (tag +0x04); the index is `layer * 0x200 + cluster`, and `decal_evict_object_decals`
0x44e310 loops the first subscript 0..4, which is exactly `DecalLayer`.

### `effect` (0xfc, table 0x0087abdc) and `effect_location_marker` (0x3c, table 0x0087abe0)

| field | offset | established by |
|---|---|---|
| `flags` | 0x02 | `effect_update` 0x451a30 (bits 0, 1, 3, 4), `effect_stop` 0x450b20 (bits 2, 3, 5), `effect_new_on_object` 0x4507a0 (bit 6) |
| `definition_index` | 0x04 | `effect_new` 0x451500 stores its **first** argument here and uses it as a tag index, so the Ghidra signature `(object_index, definition_id)` is reversed |
| `unknown_08`, `unknown_0a` | 0x08, 0x0a | written only by 0x4506d0; no reader in the batch |
| `change_color_index` | 0x0c | 0x4506d0, read by `effect_update` as `object.change_colors` (+0x1b8) |
| `location` | 0x10 | `effect_update` copies `object.location_leaf_index` / `location_cluster_index` (+0x98/+0x9c) as one pair; 0x450980 probes it |
| `color` | 0x18 | `effect_set_placement` 0x451600 and 0x450980; consumed by `effect_event_apply` 0x452cf0 for the `pctl` branch with alpha forced to 1.0 |
| `velocity` | 0x24 | `effect_update` copies `object.velocity` (+0x68); `effect_event_apply` adds it to the random part velocity |
| `tint_source` | 0x30 | `effect_set_placement`, called by `effect_spawn_particles` (see evidence 4) |
| `object_index` | 0x3c | all four creation wrappers; `effect_update` deletes the effect once the object is gone |
| `creator_object_index` | 0x40 | `effect_new` second argument; `effect_event_apply` passes it to `object_new` as the owner and into `damage_data` |
| `a_scale` / `b_scale` | 0x44 / 0x48 | `effect_property_random_value` 0x451290 reads +0x44 for the A bit-set and +0x48 for the B bit-set of every `EffectPartScalesValues` |
| `first_person_weapon_index` | 0x4c | 0x4506d0 etc. from 0x4926f0; indexes 0x006b2d98 with stride 0x1ea0 |
| `event_index` .. `previous_event_fraction` | 0x4e..0x5c | `effect_start_event` 0x451660 and `effect_update`; `event_duration` is `random_real_range_seeded(EffectEvent.duration_bounds)` at tag +0x10 |
| `location_markers[32]` | 0x5c | the `for (0x20) *p = -1` loop in all four wrappers; indexed by `EffectPart.location` |
| `particle_counts[32]` | 0xdc | `effect_update`, one byte per `EffectEvent.particles` entry, rolled from `EffectParticle.count` at +0x6c |

`effect_location_marker.transform` is proved by the 13-dword copy in `effect_marker_new` 0x4517d0,
whose source pointer is pre-incremented by two shorts before the first store and therefore starts
at `object_marker.transform` (+0x04). Consumers agree: `object_change_color_evaluate` 0x4529d0
reads +0x0c..0x18 and +0x24..0x30 as basis rows and +0x30..0x3c as the position, which is exactly
`real_matrix4x3` laid at +0x08.

### `particle_system` (0x158, table 0x0087abd4)

`particle_system_new_at_point` 0x453600, `particle_system_new_on_marker` 0x4536f0,
`particle_system_new_type_states` 0x4538b0, `particle_system_update` 0x4544f0,
`particle_system_render` 0x454bf0, `particle_system_delete` 0x453f60.
`ambient_color` at 0x48 is the `object_sample_ambient_lightmap_point` output (0x453600 passes
`rec + 0x20` as the point and `rec + 0x48` as the destination), and 0x454bf0 multiplies the per
particle colour by +0x48/+0x4c/+0x50, so it is three floats and 0x54 is separate.

The ten multipliers in `particle_system_type_state` 0x0c..0x34 are one 10-float copy from
`ParticleSystemTypeStates + 0x34`, which pins them to `scale_multiplier`,
`animation_rate_multiplier`, `rotation_rate_multiplier`, `color_multiplier` (4),
`radius_multiplier`, `minimum_particle_count`, `particle_creation_rate` in order. Each one is then
conditionally scaled by the system scale, and the guarding `ParticleSystemType.flags` bit for each
matches the `ParticleSystemTypeFlags` bitfield order in `tags.h` exactly (bit 9
`tint_by_effect_color` → the colour, bit 11 → `minimum_particle_count`, 12 → rate, 13 → scale,
14 → animation rate, 15 → rotation rate). That agreement is the strongest confirmation in the
batch that the block is laid out as written.

### `particle` (0x70, table 0x0087abd0)

`particle_new` 0x455740 writes every field. `color` at 0x60 is `ColorARGB` because the RGB triple
at 0x64..0x70 is what the ambient and diffuse samples multiply, gated on `Particle.flags` bit 9
(`self_illuminated`) and bit 6 (`tint_from_diffuse_texture`). `sequence_state` at 0x0e is the
0→1→2→3→4 walk in `particle_next_sequence` 0x455e60 over `Particle.first_sequence_index` +0x98,
`initial_sequence_count` +0x9a, `looping_sequence_count` +0x9c and `final_sequence_count` +0x9e.
`last_update_tick` at 0x10 is compared against 0x007c3100 with a 0x10-tick window in
`particles_update` 0x455b60, which is a staleness cull rather than a lifetime.

### `weather_instance` (0x9c, 0x006b0ae4), `weather_particle` (0x54, table 0x0087abcc), `weather_particle_system_state` (0x20, 0x00746b88)

`weather_instance_activate` 0x457e20, `weather_instance_deactivate` 0x457f00,
`weather_instance_adjust_count` 0x457fc0, `weather_particle_new` 0x458070,
`weather_instance_update` 0x458420, `weather_particle_update` 0x458630,
`weather_update_local_player` 0x458a90, `weather_update` 0x53f5c0.

Every `weather_particle` field maps onto a named `WeatherParticleSystemParticleType` field, which
is what makes this struct confident despite the low phase-2 confidence scores: `target_count` ←
`particle_count` +0xa4, `field_extent` ← `fade_out_end_distance` +0x30, `acceleration` ←
`acceleration_magnitude` +0xcc with `acceleration_change_rate` +0xd4 and
`acceleration_turning_rate` +0xd8, `radius` ← `particle_radius` +0xfc, `animation_rate` ← +0x104,
`rotation_rate` ← +0x10c, `alpha` ← the two `color_*_bound` alphas +0x134/+0x144, `color` ←
`color_interpolate` over the same pair with the flags at +0x20 selecting HSV, and
`sequence_index` ← the `sprite_bitmap` tag at +0x194 (tag id +0x1a0).

`color_interpolate` 0x43f6a0 writes **three** floats, not four, proved by `effect_spawn_particles`
computing the alpha separately into the slot below its output. That is what separates
`weather_particle.alpha` (0x34) from `weather_particle.color` (0x38..0x44) and keeps `radius` at
0x44.

### `player_effect` (0xec) and `player_effect_globals` (0x124)

`player_effect_clear_dead_players` 0x456730 (zeroes 0x3b dwords = 0xec),
`player_effect_apply_continuous_damage` 0x4567c0, `player_effect_apply_at_object` 0x456900,
`player_effect_set_screen_flash` 0x4578a0 (14 dwords into +0x18),
`player_effect_set_camera_impulse` 0x4579b0 (13 floats into +0x50),
`player_effect_set_camera_shake` 0x457d50 (18 floats into +0x84),
`player_effect_build_screen_flash` 0x457000, `player_effect_fade_damage_indicators` 0x457220,
`player_effect_mark_damage_direction` 0x456cf0, `player_effect_build_camera_shake_matrix`
0x457390, `player_effect_send_network_update` 0x456bc0.

---

## Unresolved offsets

Named `unknown_XX` in the header, listed here so the next pass knows where to look.

| struct | offset | what is known |
|---|---|---|
| `contrail` | 0x03 | never read; almost certainly padding after the byte `flags` |
| `contrail_point` | 0x10 | not written at create and not read by any of the 124 functions. The renderer (`contrail_draw_segment_blended` 0x511b40 and around it) is the only candidate reader |
| `decal` | 0x19 | never written by `decal_place`; may be padding beside `sequence_index` |
| `decal` | 0x1a | written as literal 0 by `decal_place` at 0x00450240 |
| `decal` | 0x1b | a byte taken from a local that `decal_place` sets to 0 on one path (0x0044f3ac) and to a short read out of a BSP surface/lightmap table on another (0x0044f381). Probably the lightmap or shader index the rasterizer needs |
| `decal` | 0x29 | never written |
| `decal_type_parameters` | 0x04 | 110.0 for three types, 10.0 for painted_sign; no reader inside the range. Likely a second angle used by the renderer |
| `effect` | 0x08, 0x0a | written only by `effect_new_at_texture_coordinate` 0x4506d0 from its two explicit arguments; no reader in the batch. The phase-2 summary guesses a 2D texture coordinate |
| `effect` | 0x0e | never written |
| `effect_tint_source` | 0x08 | third dword of the descriptor; cleared alongside `proc` but never read |
| `particle_system` | 0x02 | never written |
| `particle_system` | 0x54 | no writer or reader found |
| `particle_system_particle` | 0x28..0x34 | filled by one of the three missing creation-physics procedures at 0x00657444. Declared `real_point3d unknown_28` on the strength of the stride, not the semantics |
| `particle` | 0x3c..0x48 | copied straight from `particle_creation_data + 0x1c`; no reader in the range |
| `particle` | 0x54, 0x58 | copied from `particle_creation_data + 0x40` and `+ 0x44`; no reader |
| `particle_creation_data` | 0x0b, 0x0f | the two bytes beside the three flag bytes; never read |
| `particle_creation_data` | size | 0x5c is a lower bound: `particle_new` reads up to `[0x16]` = +0x58. The block is a caller stack temporary and the caller layout in `effect_spawn_particles` 0x451f90 is not contiguous with the locals Ghidra names, so the base is only pinned by the reads |
| `weather_instance` | 0x10, 0x14 | +0x10 is copied from 0x007c3344 and handed to `FUN_0053ed60` as a sample point **and** to the render submit as the field origin, which are incompatible widths (4 bytes cannot be a `real_point3d`). +0x14 is 2 bytes from 0x007c3348. One of the two readings is a decompiler artefact and I could not settle which |
| `weather_instance` | 0x16, 0x1b | never written |
| `weather_particle` | 0x02, 0x2a | padding |
| `weather_particle_system_state` | array length | only the 0x20 stride is proved. The header declares 8 entries on the strength of the `Scenario` weather palette maximum and the 8 per-type blocks in `weather_instance`; nothing between 0x00746b88 and the next referenced global bounds it |
| `player_screen_flash` | 0x02, 0x04..0x10, 0x14..0x24 | inside the 14-dword copy; only `type`, `duration`, `intensity` and `color` have readers |
| `player_camera_impulse` | 0x04, 0x1c..0x34 | inside the 13-float copy; only `duration`, the magnitude pair and `intensity` have readers |
| `player_camera_shake` | 0x04..0x20, 0x24, 0x2c..0x48 | inside the 18-float copy; only `duration`, `unknown_20` (also scaled by 30) and `intensity` have readers |
| `player_effect_globals` | 0x0ff | padding |
| `decal_projection` | 0x57 | padding after the `normal_positive` bool |

Two scratch buffers are deliberately **not** typed:

- The polygon accumulator `decal_flood_surfaces` 0x44e730 takes as its second argument, addressed
  at +0x5000 (a vertex count), +0x5002 (1024 per-surface counts) and +0x5802 (a surface count).
  It is a stack temporary inside `decal_place` 0x44edc0 (which reserves ~0x21000 bytes via
  `__chkstk`), and the vertex stride disagrees between the two functions that touch it — 0x10 in
  `decal_place`, and the surrounding arithmetic in 0x44e730 implies 0x14. Resolving it needs the
  rasterizer side (`FUN_0051a770`, `rasterizer_decals_initialize` 0x51a6a0).
- The `object_marker` array `effect_rebuild_markers` 0x451710 reserves 0x6c0 bytes for, which is
  `0x10 * sizeof(object_marker)` and needs no new type.

---

## Misattributed functions

### Ghidra names that are wrong

Ghidra (or the phase-2 pass) named seven functions `particle_system_*` that in fact operate on the
`effe` table at 0x0087abdc, not the `pctl` table at 0x0087abd4. The giveaway is the element stride
(0xfc versus 0x158) and the tag layout they read (`Effect.events` at +0x34 versus
`ParticleSystem.particle_types` at +0x5c):

| address | Ghidra name | what it is |
|---|---|---|
| 0x450630 | `particle_system_try_and_get` | `effect_try_and_get` |
| 0x451290 | `particle_system_property_random_value` | `effect_property_random_value` |
| 0x451500 | `particle_system_new` | `effect_new` — and its two arguments are reversed: argument 1 is the `effe` tag index, not an object index |
| 0x450be0 | `particle_system_delete_450be0` | `effect_delete` |
| 0x451a30 | `particle_system_update` | `effect_update` |
| 0x451f90 | `particle_system_spawn_particles` | `effect_spawn_particles` (one `EffectEvent`, not a `pctl` system) |
| 0x453f60 | `particle_system_delete_453f60` | the real `particle_system_delete` |

`chimera__contrail_update` 0x44cb50 and `chimera__decal_table` 0x44e2b0 are Chimera signature
names for `contrail_update` and `decals_update_fade`; the prefix is an artefact of the signature
source, not part of the retail symbol.

`FUN_004529d0` is named `object_change_color_evaluate` in the phase-2 results, and that is right
in spirit but it is reached only from `effect_update`, so it belongs to the effect event pipeline
rather than to `objects`.

### Functions in this range that belong to other modules

Types skipped for all of these; they need nothing beyond `tags.h` and `math.h`.

| address | belongs to | note |
|---|---|---|
| 0x44d820 `vector3d_major_axis_index` | math | generic; already the shape `math.h` documents for 0x4ce8c0 |
| 0x44d860 | math | back-solves the third coordinate through `k_projection_axes`, which `math.h` already owns |
| 0x44d8e0 `vector3d_scalar_triple_product` | math | generic |
| 0x44d950 | math | `plane2d_from_points`; returns NULL on a degenerate segment |
| 0x44d9e0 | math | `plane3d_from_point_and_normal` |
| 0x44da20 `plane3d_negate` | math | generic |
| 0x44da60 `color_real_to_argb_pack` | math or cseries | generic quantise-and-pack |
| 0x44dad0 | structures | fetches an indexed BSP plane, negating it for a negative index |
| 0x44db30 | bitmaps or structures | builds a lightmap uv rectangle from a `ScenarioStructureBSPLightmap` material row |
| 0x453290 `transition_function_evaluate` | effects, but it is the `EffectDistributionFunction` CDF, not a general easing curve | values 0..5 evaluate to 1, step, x, x², 1-(1-x)², smoothstep, which is exactly the cumulative distribution of `effectdistributionfunction_start`..`_buildup_and_falloff`. Note `math.h` already has a different `transition_function_evaluate` at 0x4ccac0 |
| 0x453330 | players | "is any local player within 10 world units"; reads the player globals at 0x0087a478 and the camera positions at 0x006ac6d0 |
| 0x456730..0x457d50 (15 functions) | a `player_effect` module (players / camera / damage feedback) | `ContinuousDamageEffect` and `DamageEffect` screen and rumble feedback per local player. Types defined in `types/effects.h` anyway because no other header owns them; move them out when that module is written |
| 0x458990 | render | builds a camera-facing matrix from the render globals at 0x007c3114..0x007c321c; no effects state at all |
| 0x4588e0 | math | adds signed noise to a vector; takes everything in registers |
| 0x458b50 | structures | finds up to 8 `Scenario` trigger-volume-like regions within a radius, reading the scenario at 0x00746f9c +0x1c0/+0x1c4 |
| 0x53f860, 0x53f940, 0x53fa70, 0x53fc80 | effects, but they serve the sky and marker colour path rather than any of the six subsystems | the ambient noise grid; `types/effects.h` keeps them because 0x53f940 blends against the weather wind state this module owns |

### Functions Ghidra never created, inside 0x44c800..0x53fc80

Found through the three `.rdata` dispatch tables. All six are particle physics and all six own
fields I could only type by stride:

| address | column | role |
|---|---|---|
| 0x4552a0 | `0x0065743c[0]` | system update physics, default |
| 0x4554d0 | `0x0065743c[1]` | system update physics, explosion |
| 0x455310 | `0x00657444[0]` | particle creation physics, default |
| 0x4554e0 | `0x00657444[1]` | particle creation physics, explosion |
| 0x455610 | `0x00657444[2]` | particle creation physics, jet |
| 0x455350 | `0x00657450[0]` | particle update physics, default |

They are the best next target in this module: between them they resolve
`particle_system_particle` 0x1c..0x40 and confirm whether `unknown_28` is a previous position, an
acceleration or an axis.
