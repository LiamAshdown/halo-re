# `physics` — Blam collision queries, collision proxies and rigid-body physics

Retail Halo PC `halo.exe` 1.0.10, `0x4ffde0 .. 0x50b530` (80 Ghidra functions), plain C /
MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/physics.h`, with the original decompile preserved verbatim at the
bottom of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py physics` → **77 ok, 0 failed** (and `python
tools/build_check.py` → 1040 ok, 0 failed for the whole tree).

77 of the 80 functions are here. The other three are misattributed and belong to other modules —
see [Misattributed functions](#misattributed-functions).

## What the module contains

Four layers, bottom up. Nothing here allocates; every query is handed caller stack.

| Layer | Range | What it is |
|---|---|---|
| collision BSP queries | `0x501340`–`0x503350` | sphere, segment and swept-sphere ("pill") recursive descents of a `ModelCollisionGeometryBSP`, plus the 2D helpers that project a surface onto an axis pair and clip against its edge loop |
| the `physics_model` | `0x503360`–`0x504bb0` | a scratch list of sphere / pill / polygon collision proxies built from whatever geometry a query touched, and the point and ray tests that run against it |
| object collision | `0x504e10`–`0x5067b0` | per-collision-node queries against an object's own BSPs, the cluster walks that find which objects a segment could reach, and the world-level entry point everything else calls |
| object physics | `0x5074b0`–`0x50b530` | the per-tick force / torque integration of an object carrying a `Physics` tag, one mass point at a time, plus `point_physics` for particles and other massless movers |
| breakable surfaces | `0x4ffde0`–`0x4fff20` | damage and reset of the per-BSP breakable-surface bit vector and health array |

The world-level entry point is `collision_test_movement_segment` (`0x505880`). Everything that
moves in the game reaches the collision BSP through it:

```
collision_test_movement_segment            0x505880   flags select which of the three tests run
  collision_bsp_query_segment_init         0x502060     structure BSP, bit 0x20
    collision_bsp_query_segment_node_recursive
  (fog-plane / water-surface test)                      bit 0x40, writes collision_result type 0
  object_collision_test_ray_nearby_chain   0x5055b0     bit 0x80, per cluster then per object
    object_collision_context_test_segment  0x504f60       per collision node, segment
    object_collision_context_test_pill     0x5050b0       per collision node, swept sphere
  (unstick)                                             bit 0x100000, backs the point out of solid

physics_model_build_from_sphere_query      0x506440   the proxy-list counterpart
  collision_bsp_query_sphere_init          0x501980
  physics_shape_build_proxies_from_query   0x503d90     vertices -> spheres, edges -> pills,
                                                        surfaces -> polygons
  collision_gather_nearby_object_shapes    0x5061c0     the same for nearby objects

object_physics_tick                        0x507840   Physics.radius > 0 picks the single-pass path
  object_physics_tick_single_pass          0x509e80     fused force + integrate + collide
  object_physics_compute_mass_point_forces 0x507cc0     the general multi-mass-point path
    object_physics_mass_point_resolve_ground_contact 0x507ac0
    object_physics_blend_friction_axes     0x507c00
  object_physics_integrate_and_test_at_rest 0x5097e0
  object_physics_handle_nearby_object_impacts 0x508a10
    object_physics_check_impact_damage     0x508b70
    object_physics_resolve_mass_point_overlap 0x5090c0
```

## Struct layouts

All of these live in `types/physics.h`; the tables below are the summary. Offsets are byte
offsets from the struct base, `#pragma pack(push,1)` is in force, and pointer fields are 4 bytes
on the target. Types that already existed elsewhere are **not** redeclared here:
`ModelCollisionGeometry*` and `Physics` / `PhysicsMassPoint` / `PointPhysics` come from
`types/tags.h`, `collision_result` from `types/projectiles.h`, `object` / `damage_data` /
`bsp_leaf_reference` from `types/objects.h`, `data_array` / `datum_index` from `types/memory.h`,
and `k_projection_axes` from `types/math.h`.

### `collision_bsp_sphere_query` — size `0x228`

| Off | Type | Field |
|---|---|---|
| `0x000` | `void *` | `bsp` — `ModelCollisionGeometryBSP *` |
| `0x004` | `int16_t` | `breakable_surface_count` — a leaf index at or above this counts as already broken |
| `0x006` | `int16_t` | `unknown_006` — padding, never touched |
| `0x008` | `uint32_t *` | `breakable_surfaces` — bit vector, one bit per breakable surface |
| `0x00c` | `void *` | `center` — `real_point3d *` |
| `0x010` | `float` | `radius` |
| `0x014` | `void *` | `result` — `collision_bsp_sphere_result *` |
| `0x018` | `int32_t` | `plane_count` — depth of the split-plane stack |
| `0x01c` | `int32_t[128]` | `planes` — plane index, bit 31 set when the sphere is on the back side |
| `0x21c` | `int16_t` | `projection_axis` |
| `0x21e` | `uint8_t` | `projection_sign` |
| `0x220` | `float` | `projected_center_i` |
| `0x224` | `float` | `projected_center_j` |

The 128-entry plane stack is forced: `0x1c + 128*4 = 0x21c`, exactly where the projection cache
starts.

### `collision_bsp_sphere_result` — size `0x1010`

| Off | Type | Field |
|---|---|---|
| `0x000` | `int32_t` | `surface_count` |
| `0x004` | `int32_t[256]` | `surfaces` |
| `0x404` | `int32_t` | `edge_count` |
| `0x408` | `int32_t[256]` | `edges` |
| `0x808` | `int32_t` | `vertex_count` |
| `0x80c` | `int32_t[256]` | `vertices` |
| `0xc0c` | `int32_t` | `leaf_count` |
| `0xc10` | `int32_t[256]` | `leaves` |

Every list is append-with-dedupe and silently drops entries past 256.
`collision_bsp_query_sphere_init` reports "found something" from `surface_count` and `edge_count`
only — not from `vertex_count`.

### `collision_bsp_segment_query` — size `0x28`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `flags` — `collision_bsp_segment_flags` |
| `0x04` | `void *` | `bsp` |
| `0x08` | `int16_t` | `breakable_surface_count` |
| `0x0a` | `int16_t` | `unknown_0a` |
| `0x0c` | `uint32_t *` | `breakable_surfaces` |
| `0x10` | `void *` | `origin` — `real_point3d *` |
| `0x14` | `void *` | `delta` — `real_vector3d *`, the whole swept segment |
| `0x18` | `void *` | `result` — set by the caller in ECX, not by `0x502060` |
| `0x1c` | `int32_t` | `last_leaf` — -1 before the first plane crossing |
| `0x20` | `uint8_t` | `last_leaf_type` — `collision_bsp_leaf_type` |
| `0x24` | `int32_t` | `crossing_plane` |

### `collision_bsp_segment_result` — size `0x418`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `t` — fraction of `delta` consumed, clamped into 0..1 at init |
| `0x04` | `void *` | `plane` — `real_plane3d *`, straight into the BSP plane block |
| `0x08` | `int32_t` | `surface_index` |
| `0x0c` | `int32_t` | `plane_index` — `surface->plane`, sign bit set on a back-face hit |
| `0x10` | `uint8_t` | `surface_flags` |
| `0x11` | `int8_t` | `breakable_surface_index` |
| `0x12` | `int16_t` | `material_index` — into `ScenarioStructureBSP.collision_materials` |
| `0x14` | `int32_t` | `leaf_count` |
| `0x18` | `int32_t[256]` | `leaves` — every leaf crossed, in order; overflow overwrites `leaves[255]` |

### `collision_bsp_pill_query` — size `0x22c`

| Off | Type | Field |
|---|---|---|
| `0x000` | `void *` | `bsp` |
| `0x004` | `void *` | `origin` |
| `0x008` | `void *` | `delta` |
| `0x00c` | `float` | `radius` |
| `0x010` | `void *` | `result` — caller ECX |
| `0x014` | `int32_t` | `plane_count` |
| `0x018` | `int32_t[128]` | `planes` — same encoding as the sphere query |
| `0x218` | `int16_t` | `projection_axis` |
| `0x21a` | `uint8_t` | `projection_sign` |
| `0x21c` | `float` | `projected_origin_i` |
| `0x220` | `float` | `projected_origin_j` |
| `0x224` | `float` | `projected_delta_i` |
| `0x228` | `float` | `projected_delta_j` |

### `collision_bsp_pill_result` — size `0x420`

| Off | Type | Field |
|---|---|---|
| `0x000` | `float` | `t` — deepest contact fraction so far; init writes the clamped maximum here |
| `0x004` | `float` | `plane_i` |
| `0x008` | `float` | `plane_j` |
| `0x00c` | `float` | `plane_k` |
| `0x010` | `float` | `plane_d` — `FLT_MAX` marks "edge contact, not a face" |
| `0x014` | `int32_t` | `surface_index` |
| `0x018` | `int16_t` | `unknown_018` — nothing in the module writes it |
| `0x01a` | `int16_t` | `material_index` |
| `0x01c` | `int32_t` | `leaf_count` |
| `0x020` | `int32_t[256]` | `leaves` |

### `collision_bsp_boundary_clip` — size `0x18` (two `collision_bsp_boundary_hit`, `0x0c` each)

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `enter.t` |
| `0x04` | `int32_t` | `enter.edge_index` — held in a float register, read back as an index |
| `0x08` | `int32_t` | `enter.surface_index` — the surface on the far side of the edge |
| `0x0c` | `float` | `exit.t` |
| `0x10` | `int32_t` | `exit.edge_index` |
| `0x14` | `int32_t` | `exit.surface_index` |

Both halves are seeded to -`FLT_MAX` / +`FLT_MAX`; the caller treats `exit.t < enter.t` as
"no overlap".

### `physics_model` — size `0xac08`

| Off | Type | Field |
|---|---|---|
| `0x0000` | `int16_t` | `sphere_count` — the three counts are indexed as `counts[shape_type]` |
| `0x0002` | `int16_t` | `pill_count` |
| `0x0004` | `int16_t` | `shape_count` |
| `0x0006` | `int16_t` | `unknown_0006` |
| `0x0008` | `physics_model_sphere[256]` | `spheres`, stride `0x1c` |
| `0x1c08` | `physics_model_pill[256]` | `pills`, stride `0x28` |
| `0x4408` | `physics_model_shape[256]` | `shapes`, stride `0x68` |

The size is pinned twice over: the bases and strides the point/ray tests use to walk back from a
hit index, and the `0xac08` bytes of stack `0x507170` and `0x507ac0` each reserve for one.

### `physics_model_sphere` — size `0x1c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `object_index` — -1 for the world |
| `0x04` | `int32_t` | `surface_index` — -1 when `object_index` is set |
| `0x08` | `uint8_t` | `surface_flags` |
| `0x09` | `int8_t` | `breakable_surface_index` |
| `0x0a` | `int16_t` | `material_type` — `0xffff` when unknown |
| `0x0c` | `float` | `center_x` |
| `0x10` | `float` | `center_y` |
| `0x14` | `float` | `center_z` — the pair's second sphere sits at `center_z - height` |
| `0x18` | `float` | `radius` |

### `physics_model_pill` — size `0x28`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `object_index` |
| `0x04` | `int32_t` | `surface_index` |
| `0x08` | `uint8_t` | `surface_flags` |
| `0x09` | `int8_t` | `breakable_surface_index` |
| `0x0a` | `int16_t` | `material_type` |
| `0x0c`..`0x14` | `float` | `origin_x/y/z` |
| `0x18`..`0x20` | `float` | `extent_i/j/k` — `origin + extent` is the far end of the axis |
| `0x24` | `float` | `radius` |

### `physics_model_shape` — size `0x68`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `object_index` |
| `0x04` | `int32_t` | `surface_index` |
| `0x08` | `uint8_t` | `surface_flags` |
| `0x09` | `int8_t` | `breakable_surface_index` |
| `0x0a` | `int16_t` | `material_type` |
| `0x0c`..`0x14` | `float` | `plane_i/j/k` — the supporting plane |
| `0x18` | `float` | `plane_d` — pushed out by `thickness * plane_k` for a downward-facing source |
| `0x1c` | `float` | `thickness` — a point is inside while `0 <= distance < thickness` |
| `0x20` | `int16_t` | `projection_axis` |
| `0x22` | `uint8_t` | `projection_sign` |
| `0x23` | `uint8_t` | `unknown_23` — padding |
| `0x24` | `int32_t` | `vertex_count` |
| `0x28` | `float[8][2]` | `vertices` — the boundary projected onto the surviving axis pair |

`0x5038a0` does **not** clamp the vertex count it copies out of the surface edge loop, so 8 is
the layout's limit, not an enforced one.

### `physics_model_contact` — size `0x2c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `t` — penetration depth (point test) or segment fraction (ray test) |
| `0x04`..`0x0c` | `float` | `point_x/y/z` — the ray test writes `origin + delta` on a miss |
| `0x10`..`0x18` | `float` | `plane_i/j/k` — separating normal, pointing at the mover |
| `0x1c` | `float` | `plane_d` |
| `0x20` | `uint32_t` | `object_index` — copied out of the proxy that won |
| `0x24` | `int32_t` | `surface_index` |
| `0x28` | `uint8_t` | `surface_flags` |
| `0x29` | `int8_t` | `breakable_surface_index` |
| `0x2a` | `int16_t` | `material_type` |

`0x5067b0` keeps an array of these, one per slide iteration, at this same `0x2c` stride, and
`physics_point_walk_state` is deliberately layout-compatible with its first four floats.

### `object_collision_context` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `object_index` |
| `0x04` | `ModelCollisionGeometry *` | `definition` |
| `0x08` | `uint8_t *` | `region_permutations` — the object's own `region_permutations` array |
| `0x0c` | `void *` | `nodes` — the object's node matrix array, `real_matrix4x3` at stride `0x34` |

### `object_node_collision_result` — size `0x420`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `node_index` |
| `0x02` | `int16_t` | `region_index` — `node->region` |
| `0x04` | `int16_t` | `permutation_index` — clamped to `node->bsps.count - 1` |
| `0x06` | `int16_t` | `unknown_06` |
| `0x08` | `collision_bsp_segment_result` | `segment` |

Both the segment test (`0x504f60`) and the pill test (`0x5050b0`) write this record, which is
why the pill variant cannot keep its whole `0x420`-byte `collision_bsp_pill_result` here and
stores only the fraction.

### `object_physics_context` — size `0x3c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `object_index` |
| `0x04` | `void *` | `definition` — `Physics *` |
| `0x08` | `float` | `scale` — always 1.0 here; the embedded `real_matrix4x3` starts at this field |
| `0x0c`..`0x14` | `float` | `forward_i/j/k` |
| `0x18`..`0x20` | `float` | `left_i/j/k` |
| `0x24`..`0x2c` | `float` | `up_i/j/k` |
| `0x30`..`0x38` | `float` | `position_x/y/z` — `M * (-Physics.center_of_mass)` |

Every callee that takes "the matrix" is handed `&context->scale` (`context + 2` dwords in the
decompiles), not `&context->forward_i`.

### `object_physics_ray_result` — size `0x14`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `t` |
| `0x04`..`0x0c` | `float` | `plane_i/j/k` |
| `0x10` | `float` | `plane_d` |

### `mass_point_state` — size `0x130`

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint32_t` | `flags` — `mass_point_flags` |
| `0x004`..`0x00c` | `float` | `position_x/y/z` — definition position transformed to world |
| `0x010`..`0x018` | `float` | `forward_i/j/k` |
| `0x01c` | `float[3]` | `unknown_01c` — zeroed, never written; where a `left` vector would sit |
| `0x028`..`0x030` | `float` | `up_i/j/k` |
| `0x034` | `int32_t` | `leaf_index` — -1 outside the BSP |
| `0x038` | `int16_t` | `cluster_index` |
| `0x03c`..`0x044` | `float` | `offset_x/y/z` — position minus object position, the torque arm |
| `0x048`..`0x050` | `float` | `velocity_i/j/k` — object velocity + angular_velocity × offset |
| `0x054`..`0x05c` | `float` | `tangential_velocity_i/j/k` |
| `0x060`..`0x06c` | `float` | `resting_plane_i/j/k/d` — seeded from `0x0069c53c` |
| `0x070` | `int16_t` | `material_type` — `0xffff` when nothing was hit |
| `0x074` | `float` | `ground_depth` — positive means the sphere is dug in |
| `0x078` | `uint32_t` | `unknown_078` — zeroed, never written |
| `0x07c` | `float` | `water_depth` |
| `0x080` | `float` | `ground_normal_magnitude` — the spring + damper reaction |
| `0x084`..`0x08c` | `float` | `ground_normal_force_i/j/k` |
| `0x090` / `0x09c` / `0x0a8` | `float[3]` | `ground_friction_force` / `_parallel` / `_perpendicular` |
| `0x0b4` | `float` | `buoyancy_magnitude` — volume × tag water_density, faded in by depth |
| `0x0b8`..`0x0c0` | `float` | `buoyancy_force_i/j/k` — always `(0, 0, magnitude)` |
| `0x0c4` / `0x0d0` / `0x0dc` | `float[3]` | `water_friction_force` / `_parallel` / `_perpendicular` |
| `0x0e8` / `0x0f4` / `0x100` | `float[3]` | `air_friction_force` / `_parallel` / `_perpendicular` |
| `0x10c`..`0x114` | `float` | `powered_force_i/j/k` |
| `0x118`..`0x120` | `float` | `total_force_i/j/k` |
| `0x124`..`0x12c` | `float` | `torque_i/j/k` — `offset × total_force` |

Each friction block is the nine-float layout `object_physics_blend_friction_axes` expects:
blended result first, then the two candidate axes it mixes.

### `powered_mass_point_state` — size `0x60`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `ground_friction` — `PhysicsPoweredMassPointFlags` bit `0x01` |
| `0x04` | `float` | `water_friction` — bit `0x02` |
| `0x08` | `float` | `air_friction` — bit `0x04` |
| `0x0c` | `float` | `water_lift` — bit `0x08` |
| `0x10` | `float` | `air_lift` — bit `0x10` |
| `0x14` | `float` | `thrust` — bit `0x20` |
| `0x18` | `float` | `antigrav` — bit `0x40` |
| `0x1c` | `uint8_t[0x10]` | `unknown_1c` — untouched by this module |
| `0x2c` | `float` | `matrix_scale` — start of the `real_matrix4x3` `0x507840` writes |
| `0x30` | `float[3][3]` | `matrix` — transposed in place after `matrix4x3_from_quaternion` |
| `0x54` | `float[3]` | `matrix_position` |

A branch only runs when its flag is set **and** the matching scalar is non-zero, and `0x507cc0`
reads them in flag-bit order.

### `breakable_surface_globals` — size `0x4204` (pointer at `0x006b8d78`)

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint8_t` | `initialized` — every entry point bails when 0 |
| `0x001` | `uint32_t[16][8]` | `active` — one bit per breakable surface, set while intact. **Unaligned on purpose** |
| `0x201` | `uint8_t[3]` | `unknown_201` — padding |
| `0x204` | `float[16][256]` | `health` — crossing zero clears the bit and fires the break effect |

The bit vector starting at offset 1 is why every reader writes
`base + 1 + (bsp_index * 8 + (surface >> 5)) * 4`. The `0x203`-byte gap is what fixes
`k_maximum_structure_bsps` at 16 — inferred from the gap, not read from an allocation.

### `physics_scalar_range` — size `0x08`, `physics_scalar_rates` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `physics_scalar_range.upper` — read **upper first**; the span is `range[0] - range[1]` |
| `0x04` | `float` | `physics_scalar_range.lower` |
| `0x00` | `float` | `physics_scalar_rates.maximum_positive` |
| `0x04` | `float` | `physics_scalar_rates.maximum_negative` |
| `0x08` | `float` | `physics_scalar_rates.acceleration_positive` |
| `0x0c` | `float` | `physics_scalar_rates.acceleration_negative` |

`real_bounds` in `types/math.h` is **not** reused: it reads lower-first, the opposite order.

### `object_physics_tick_accumulator` — size `0x18`, `physics_point_walk_state` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `real_vector3d` | `object_physics_tick_accumulator.torque` — **torque comes first** |
| `0x0c` | `real_vector3d` | `object_physics_tick_accumulator.force` |
| `0x00` | `float` | `physics_point_walk_state.t` — remaining step fraction |
| `0x04` | `real_point3d` | `physics_point_walk_state.position` |

Both were file-local typedefs until the phase-4 integration pass folded them into
`types/physics.h`; so was `collision_test_movement_segment_flags`.

### Enums

`physics_constants`, `collision_bsp_leaf_type`, `collision_bsp_segment_flags`,
`collision_test_movement_segment_flags`, `physics_model_shape_type`, `mass_point_flags` and
`point_physics_result_flags`. `collision_test_movement_segment_flags` is the one worth quoting,
because it is `0x505880`'s whole API surface on top of the low five `collision_bsp_segment_flags`
face-side bits:

| Bit | Meaning |
|---|---|
| `0x20` | apply the structure-BSP segment clip result to the caller's `collision_result` |
| `0x40` | also test the destination cluster's fog plane for a water-surface crossing |
| `0x80` | also walk nearby objects' collision |
| `0xfff00` | object-type test mask forwarded to `0x5055b0`; defaults to "every type" when zero |
| `0x100000` | after a hit, nudge the result point back out of solid geometry |

### What this module adds to `collision_result` (`types/projectiles.h`, size `0x50`)

`0x505880` is the producer every other consumer reads, so its stores name fields
`projectiles.h` had to leave as unknowns. These are documented at the bottom of
`types/physics.h` rather than edited into `projectiles.h`, which another module owns:

| Off | `projectiles.h` | What this module writes |
|---|---|---|
| `0x00` | `type` | `0` is also produced here, by the fog-plane test — which is why `point_physics` treats 0 as "hit water" |
| `0x04` | `unknown_04[8]` | `int32_t` **first** leaf of the segment walk |
| `0x08` | (same) | `int16_t` cluster of that leaf |
| `0x0c` | `leaf` | the **last** leaf of the walk (the endpoint leaf) |
| `0x10` | (same) | its cluster |
| `0x30` | `unknown_30` | the `d` of the contact plane; `0x24`..`0x33` is one `real_plane3d` |
| `0x3c` | `unknown_3c` | `int16_t region_index` |
| `0x3e` | `marker_index` | collision **node** index, not a marker index |
| `0x40` | `unknown_40` | `int16_t permutation_index`; `0x42` stays untouched |
| `0x48` | `unknown_48` | `int32_t plane_index`, sign bit set on a back-face hit |
| `0x4d` | `unknown_4d` | the surface's `breakable_surface` index |
| `0x4e` | `unknown_4e` | `int16_t` index into `ScenarioStructureBSP.collision_materials` |

The two leaf pairs at `0x04`/`0x08` and `0x0c`/`0x10` are distinct records. Conflating them was
one of the drift bugs the integration pass fixed.

## Misattributed functions

Three of the module's 80 addresses are not physics and are deliberately not rewritten here
(`out/phase4/physics_types_notes.md` section 5):

| Address | Size | Where it belongs |
|---|---|---|
| `0x500090` | 4764 | **effects.** Picks a random sample position inside a broken surface and spawns the break particle effect: drives the cseries LCG at `0x00719cd4` (`x*0x19660d + 0x3c6ef35f`), reads `GlobalsBreakableSurfaceParticleEffect`, calls `color_interpolate` and the particle spawner at `0x006267f0`. Only its two entry conditions are physics state, and `breakable_surface_globals` already covers those. |
| `0x5066e0` | 121 | **math.** `point3d_project_onto_line`, already named; projects a point onto an infinite line. |
| `0x507430` | 123 | **math.** Clamped inverse lerp of a value between two references, returning 0 or 1 outside. One caller here, one in `0x509e80`. |

Two more are flagged but **are** rewritten here, because nothing else claims them yet:

* `0x501470` `physics_shape_forward_call_helper` — a two-line thunk into `FUN_0044d860` (math)
  that returns its second argument unchanged. No types of its own.
* `0x50b290` / `0x50b2f0` / `0x50b370` / `0x50b460` / `0x50b4d0` — five scalar-range helpers whose
  only callers are outside the module slice. A wrapping range plus asymmetric
  accelerate/decelerate limits looks like turret or seat aiming rather than rigid-body physics.
  `physics_scalar_range` and `physics_scalar_rates` live in `types/physics.h` so the module
  compiles; ownership is **UNSURE** and they should move if the unit/vehicle pass claims them.

Phase 2 named `0x507610`, `0x507cc0` and `0x5097e0` `antenna_*`. That is wrong — all three
resolve the object's `Physics` tag (Object tag `+0x8c`) and iterate `Physics.mass_points` at
stride `0x80`, never the `antenna` widget in `types/objects.h`, and the dispatch in `0x5061c0`
and `0x505350` reaches them for `object_type == 1` (vehicle) under mask bit `0x400000`. The
renames are recorded, with that evidence, in `symbols/agent_phase4_physics.txt`.

## Known gaps

**Hidden register arguments are the dominant problem in this module.** Ghidra recovers the stack
arguments of nearly every call here, but the collision and physics helpers pass their output
pointer, their matrix and often their whole input point in registers, and MSVC 7.1's register
allocation defeats the decompiler at most of those call sites. Everything reconstructed that way
carries an `UNSURE` comment naming the callee; there are 204 of them across the 77 files. The
worst offenders, by call site:

* `physics_shape_add_vertex_proxy` / `_add_edge_proxy` (`0x503a60` / `0x503ae0`) are called from
  `physics_shape_build_proxies_from_query` with **no** visible arguments at all, not even the loop
  index. The surface loop is the one third of `0x503d90` Ghidra decompiled properly.
* `matrix4x3_transform_point`, `matrix4x3_inverse_transform_point`, `vector3d_cross_product` and
  `vector3d_normalize_with_length` show only their stack argument (the matrix, or the output
  pointer) at every call site in this module. Every physics file now declares them with the
  canonical `src/math` prototype and reconstructs the register operands at the call, with an
  `UNSURE` on each.
* `object_set_position_and_relink` (`0x4f5350`) is called from `0x508b70` with a single literal
  `0`, which cannot be its real signature.

**Things this module reads but does not own, and cannot name:**

* The fog / atmosphere chase in `0x505880`: `ScenarioStructureBSP` `+0x138` (cluster, stride
  `0x68`) → `+0x17c` fog plane → `+0x188` fog region → `+0x194` fog palette → the loaded Fog tag's
  `+0x74`, which is presumably a world-space water altitude. Kept as raw offsets.
* The Globals sub-tag chase in `0x508b70`: `Globals + 0x18c` then `+0x68` (impact damage effect)
  and `+0x58` (breakable-surface damage effect).
* `GlobalsMaterial + 0x2d4`, read by `0x4ffde0` through `FUN_0053e7c0` as the divisor of the
  breakable-surface damage. Almost certainly the breakable-surface vitality, but the field is
  inside a `_pad` run in `types/tags.h`.
* `object_flags` bits `0x02` / `0x04` / `0x08` / `0x10`, written by `0x5097e0` and `0x509e80` from
  the mass-point ground / water contact tallies. Not named in `types/objects.h`; kept as raw hex.
  Note that `0x04` and `0x08` are both driven by the **same** tally (the water-contact count), and
  `0x10` is set only when *every* mass point reported it.

**Cross-module corrections worth folding back** (flagged, not applied, because another module
owns the header):

* `types/objects.h` `damage_data.unknown_4c` is the **collision material type** of the surface
  being damaged — `0x4ffde0` uses it as the index into the `DamageEffect` per-material table at
  `+0x200`.
* `types/units.h` `vehicle_data.unknown_508` / `unknown_514` are `real_vector3d
  accumulated_force` / `accumulated_torque`: `0x507840` reads six consecutive floats there, adds
  them to the force and torque it is about to integrate, then zeroes all six. This **conflicts**
  with `biped_data.ground_normal` at `0x514`; since `0x507840` is only reached through the object
  `Physics` tag, the vehicle reading is the likely one, and the two extensions overlap at `0x4cc`
  so both can be true.
* `types/projectiles.h` `collision_result` — see the table above.

**Global declarations that disagree across modules.** Physics is internally consistent (one
declaration per address, verified), but eight of the globals it reads are declared with a
different name or type by another module — `0x0069672c`, `0x0069c52c`, `0x0069e8d8`, `0x006b8d78`,
`0x006f1d6c`, `0x00746f98`, `0x00746f9c`, `0x00746fa0`. Physics reads them as
`global_reference_vector_0069672c`, `k_physics_gravity`, `global_structure_bsp_index`,
`breakable_surface_state`, `game_time_globals`, `structure_collision_bsp`,
`structure_bsp_tag_data` and `game_globals`, with the struct types
`types/physics.h` proves. `0x0069e8d8` and `0x006b8d78` in particular are read as a BSP index and
a breakable-surface block here but as a local-player index and a light table by `src/objects`,
which cannot both be right; resolving that needs a cross-module pass.

**Unresolved offsets** (`physics_types_notes.md` section 7): `collision_bsp_sphere_query 0x006`,
`collision_bsp_segment_query 0x0a`/`0x21`..`0x23`, `collision_bsp_pill_query 0x21b`,
`collision_bsp_pill_result 0x018`, `mass_point_state 0x01c`..`0x027` and `0x078`,
`powered_mass_point_state 0x1c`..`0x2b`, `physics_model_shape 0x23`.

## Functions

`size` is the Ghidra byte size. `name` is the confidence in the *name*, `rw` the confidence in
the *rewrite*, `?` the number of `UNSURE` annotations in the file (excluding the `#if 0` block).

| Address | File | Size | name | rw | ? |
|---|---|---|---|---|---|
| `0x4ffde0` | `breakable_surface_apply_damage` | 305 | 0.50 | 0.60 | 3 |
| `0x4fff20` | `breakable_surface_damage_in_blast_radius` | 356 | 0.50 | 0.55 | 2 |
| `0x501340` | `bsp2d_node_find_leaf` | 78 | 0.35 | 0.60 | 0 |
| `0x5013a0` | `bsp3d_node_find_leaf` | 92 | 0.45 | 0.70 | 0 |
| `0x501400` | `collision_bsp_surface_get_vertices` | 112 | 0.45 | 0.65 | 1 |
| `0x501470` | `physics_shape_forward_call_helper` | 47 | 0.15 | 0.30 | 2 |
| `0x5014a0` | `collision_bsp_surface_test_point_side_2d` | 252 | 0.30 | 0.45 | 1 |
| `0x5015a0` | `collision_bsp_surface_closest_edge_point_2d` | 576 | 0.35 | 0.40 | 1 |
| `0x5017f0` | `collision_bsp_surface_clip_line_2d` | 393 | 0.30 | 0.50 | 1 |
| `0x501980` | `collision_bsp_query_sphere_init` | 130 | 0.40 | 0.50 | 1 |
| `0x501a10` | `collision_bsp_query_sphere_node_recursive` | 632 | 0.40 | 0.50 | 1 |
| `0x501c90` | `collision_bsp_query_sphere_leaf_edge_recursive` | 136 | 0.30 | 0.45 | 1 |
| `0x501d20` | `collision_bsp_query_sphere_collect_geometry` | 830 | 0.40 | 0.40 | 2 |
| `0x502060` | `collision_bsp_query_segment_init` | 216 | 0.50 | 0.40 | 1 |
| `0x502140` | `collision_bsp_query_segment_node_recursive` | 787 | 0.50 | 0.35 | 1 |
| `0x502460` | `collision_bsp_surface_test_point_leaf` | 407 | 0.35 | 0.30 | 1 |
| `0x502600` | `collision_bsp_surface_test_point_2d` | 299 | 0.35 | 0.45 | 1 |
| `0x502730` | `collision_bsp_query_pill_init` | 110 | 0.30 | 0.40 | 1 |
| `0x5027a0` | `collision_bsp_query_pill_node_recursive` | 1458 | 0.35 | 0.30 | 1 |
| `0x502d60` | `collision_bsp_query_pill_leaf_recursive` | 261 | 0.30 | 0.45 | 1 |
| `0x502e70` | `collision_bsp_query_pill_leaf_test_surface` | 474 | 0.30 | 0.30 | 1 |
| `0x503050` | `physics_shape_pill_sweep_test_point` | 569 | 0.40 | 0.35 | 1 |
| `0x503290` | `physics_shape_sphere_sweep_test_ray` | 203 | 0.30 | 0.35 | 1 |
| `0x503360` | `physics_shape_vertex_to_sphere` | 291 | 0.35 | 0.55 | 0 |
| `0x503490` | `physics_shape_edge_to_pill_and_quad` | 1028 | 0.40 | 0.30 | 1 |
| `0x5038a0` | `physics_shape_surface_to_polygon` | 437 | 0.40 | 0.50 | 0 |
| `0x503a60` | `physics_shape_add_vertex_proxy` | 126 | 0.30 | 0.25 | 2 |
| `0x503ae0` | `physics_shape_add_edge_proxy` | 355 | 0.30 | 0.20 | 1 |
| `0x503c50` | `physics_shape_add_surface_proxy` | 307 | 0.30 | 0.20 | 2 |
| `0x503d90` | `physics_shape_build_proxies_from_query` | 283 | 0.35 | 0.30 | 2 |
| `0x503ec0` | `physics_shape_sphere_test_point` | 199 | 0.40 | 0.55 | 0 |
| `0x503f90` | `physics_shape_pill_test_point` | 391 | 0.40 | 0.45 | 1 |
| `0x504120` | `physics_shape_polygon_test_point` | 318 | 0.40 | 0.50 | 0 |
| `0x504260` | `physics_shape_test_point` | 455 | 0.40 | 0.50 | 1 |
| `0x504430` | `physics_shape_sphere_test_ray` | 396 | 0.50 | 0.55 | 0 |
| `0x5045c0` | `physics_shape_pill_test_ray` | 784 | 0.50 | 0.35 | 1 |
| `0x5048d0` | `physics_shape_polygon_test_ray` | 731 | 0.50 | 0.40 | 1 |
| `0x504bb0` | `physics_shape_test_ray` | 603 | 0.50 | 0.50 | 1 |
| `0x504e10` | `object_collision_context_build` | 117 | 0.50 | 0.45 | 0 |
| `0x504e90` | `object_collision_context_test_point` | 202 | 0.40 | 0.40 | 1 |
| `0x504f60` | `object_collision_context_test_segment` | 314 | 0.40 | 0.35 | 2 |
| `0x5050b0` | `object_collision_context_test_pill` | 321 | 0.40 | 0.35 | 6 |
| `0x505200` | `object_collision_context_gather_sphere_shapes` | 296 | 0.35 | 0.30 | 3 |
| `0x505330` | `model_collision_geometry_resolve_material_type` | 28 | 0.40 | 0.60 | 0 |
| `0x505350` | `object_collision_test_nearby_chain` | 305 | 0.40 | 0.30 | 0 |
| `0x505490` | `object_collision_test_cluster_group` | 172 | 0.30 | 0.20 | 5 |
| `0x505540` | `physics_point_refresh_leaf` | 102 | 0.30 | 0.20 | 3 |
| `0x5055b0` | `object_collision_test_ray_nearby_chain` | 714 | 0.40 | 0.15 | 3 |
| `0x505880` | `collision_test_movement_segment` | 1972 | 0.35 | 0.50 | 5 |
| `0x506040` | `collision_test_movement_pill` | 371 | 0.35 | 0.40 | 3 |
| `0x5061c0` | `collision_gather_nearby_object_shapes` | 613 | 0.35 | 0.35 | 7 |
| `0x506440` | `physics_model_build_from_sphere_query` | 659 | 0.35 | 0.45 | 1 |
| `0x5067b0` | `physics_model_slide_along_contacts` | 1257 | 0.30 | 0.35 | 8 |
| `0x506fb0` | `physics_sweep_capsule_step` | 277 | 0.30 | 0.30 | 4 |
| `0x5070d0` | `physics_point_walk_toward_target` | 143 | 0.30 | 0.45 | 0 |
| `0x507170` | `physics_point_find_clear_position` | 695 | 0.35 | 0.35 | 2 |
| `0x5074b0` | `object_physics_context_build` | 220 | 0.40 | 0.35 | 6 |
| `0x507590` | `object_physics_test_point_against_mass_points` | 124 | 0.35 | 0.35 | 2 |
| `0x507610` | `object_physics_test_ray_against_mass_points` | 374 | 0.50 | 0.30 | 4 |
| `0x507790` | `object_physics_add_mass_point_shapes` | 168 | 0.40 | 0.35 | 2 |
| `0x507840` | `object_physics_tick` | 503 | 0.40 | 0.35 | 5 |
| `0x507a40` | `physics_resolve_material_type` | 114 | 0.40 | 0.45 | 0 |
| `0x507ac0` | `object_physics_mass_point_resolve_ground_contact` | 306 | 0.40 | 0.35 | 4 |
| `0x507c00` | `object_physics_blend_friction_axes` | 188 | 0.35 | 0.30 | 5 |
| `0x507cc0` | `object_physics_compute_mass_point_forces` | 3403 | 0.50 | 0.30 | 8 |
| `0x508a10` | `object_physics_handle_nearby_object_impacts` | 351 | 0.40 | 0.30 | 5 |
| `0x508b70` | `object_physics_check_impact_damage` | 1355 | 0.40 | 0.35 | 14 |
| `0x5090c0` | `object_physics_resolve_mass_point_overlap` | 1573 | 0.35 | 0.30 | 6 |
| `0x5096f0` | `object_physics_mass_point_update_orientation` | 227 | 0.35 | 0.30 | 1 |
| `0x5097e0` | `object_physics_integrate_and_test_at_rest` | 1692 | 0.45 | 0.30 | 17 |
| `0x509e80` | `object_physics_tick_single_pass` | 5129 | 0.35 | 0.20 | 19 |
| `0x50b290` | `physics_scalar_advance_and_wrap` | 86 | 0.30 | 0.40 | 1 |
| `0x50b2f0` | `physics_scalar_move_toward_target` | 127 | 0.30 | 0.30 | 2 |
| `0x50b370` | `physics_clamp_value_to_spring_range` | 227 | 0.35 | 0.30 | 1 |
| `0x50b460` | `physics_scalar_step_to_target_clamped` | 110 | 0.30 | 0.30 | 2 |
| `0x50b4d0` | `physics_scalar_approach_direction` | 86 | 0.35 | 0.40 | 1 |
| `0x50b530` | `point_physics_tick` | 1195 | 0.45 | 0.45 | 7 |

The 77 names are registered in `symbols/agent_phase4_physics.txt` (with each file's own `name
confidence`, not a flat 0.8) and merged into `symbols/functions.txt` by
`tools/merge_symbols.py`. None of the 77 previously appeared in `symbols/functions.txt` except
the three `antenna_*` entries the table above replaces.

### Lowest-confidence rewrites, in order

`object_physics_tick_single_pass` (0x509e80, rw 0.20), `object_collision_test_ray_nearby_chain`
(0x5055b0, 0.15), `physics_shape_add_edge_proxy` / `_add_surface_proxy` (0.20),
`object_collision_test_cluster_group` (0x505490, 0.20), `physics_point_refresh_leaf` (0x505540,
0.20), `object_physics_integrate_and_test_at_rest` (0x5097e0, 0.30) and
`physics_shape_add_vertex_proxy` (0.25). All seven are limited by hidden register arguments
rather than by control flow: the flow in each is straightforward, and the risk is in *what* each
callee is being handed.

### Semantic corrections made by the phase-4 integration pass

Every one of these was found by re-deriving a file line by line against
`python tools/pack.py 0xADDR`; each is documented in the file's own header comment.

| File | What was wrong |
|---|---|
| `object_physics_check_impact_damage` (0x508b70) | eight separate drifts: a fused impulse-vector/scratch-float, a wrong margin expression and its type, a broken five-float aliasing, `position` used where the decompile reads `bounding_center`, a dropped `out_position.z` adjustment, the wrong object's unit extension read for the responsible party, a Globals sub-tag offset reconstructed from the wrong base, and the material scale indexed through the wrong tag and written to the wrong `damage_data` field |
| `point_physics_tick` (0x50b530) | the bounce loop's two exits had been collapsed into one, so the post-loop block that copies `collision_result.leaf` and `.point` into `*out_leaf` and `*position` was dropped entirely — a tick with no collision never moved the point |
| `object_physics_tick_single_pass` (0x509e80) | the at-rest test had its velocity and angular-velocity thresholds swapped and was missing both delta terms |
| `object_physics_integrate_and_test_at_rest` (0x5097e0) | the angular velocity was scaled by the friction term instead of by `remaining_t` |
| `physics_model_slide_along_contacts` (0x5067b0) | the no-hit exit read `contact->plane` (0x10) where the decompile reads `contact->point` (0x04) — a pointer-offset error; and the edge direction had been fused with the slide direction, which the division consumes before it is overwritten |
| `collision_test_movement_segment` (0x505880) | the first-leaf pair (0x04/0x08) and last-leaf pair (0x0c/0x10) were aliased onto the same struct field, so 0x04/0x08 were never written; and the fog-plane material write went to 0x4e instead of `material_type` at 0x34 |
| `collision_bsp_query_sphere_init` (0x501980) | the "found something" test read `vertex_count` (0x808) where the decompile reads `edge_count` (0x404) |
| `physics_shape_build_proxies_from_query` (0x503d90) | `param_1` was guessed to be a material type; both call sites pass a `ModelCollisionGeometryBSP`, which meant `0x506440` was handing a BSP pointer to a material-index parameter |
| `object_physics_add_mass_point_shapes` (0x507790) | the matrix argument was `&context->forward_i`, one field past the `&context->scale` the decompile passes |
| `object_physics_context_build` (0x5074b0) | the centre-of-mass translation was stored unrotated instead of being transformed by the matrix |
| several | Ghidra renders a float comparison as `x < K != (x == K)`, which is `x <= K`, not `x < K`; the at-rest and unstick tests are boundary-inclusive |

### Cross-file consistency

Verified mechanically after the pass: every `extern` function declaration that appears in more
than one physics file is now byte-identical across those files, and every global `extern` has one
declaration per address inside the module. Signatures for shared math helpers match
`src/math/*.c`; `collision_bsp_query_segment_init`, `collision_bsp_query_sphere_init`,
`collision_bsp_query_pill_init`, `physics_shape_build_proxies_from_query` and
`object_collision_context_gather_sphere_shapes` are declared once and copied verbatim into each
caller, instead of the four different `FUN_xxxxxxxx` shapes the first pass produced.
`collision_bsp_query_pill_init` is declared `uint8_t`, not `void`: it has no explicit return
expression, but nothing clobbers EAX between its recursive call and its epilogue and its caller
reads the value as a `char`.
