# physics types notes

Module: physics, 80 functions, 0x4ffde0..0x50b530. Header: `types/physics.h`.
Smoke file: `out/phase4/physics_smoke.c`.

Check command (the 64-bit default pass is clean, and `-m32` additionally exercises the
size/offset assertions on the structs that hold pointers, which are 4 bytes on the target):

    cd C:\Users\Liam-\halo-re
    C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -std=gnu99 -Wall -I types out\phase4\physics_smoke.c
    C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -std=gnu99 -Wall -I types out\phase4\physics_smoke.c

Both return 0.

---

## 1. Types reused, not redefined

| type | lives in | why |
|---|---|---|
| `ModelCollisionGeometryBSP` and its eight sub-blocks | `types/tags.h` | every BSP descent here indexes the tag block through its own TagReflexive pointer offsets; nothing contradicts the tag definition |
| `ModelCollisionGeometry`, `…Material`, `…Node` | `types/tags.h` | 0x504e10 / 0x504e90 / 0x504f60 / 0x505330 read only named fields |
| `Physics`, `PhysicsMassPoint`, `PhysicsPoweredMassPoint`, `PointPhysics` | `types/tags.h` | confirmed field by field, see section 4 |
| `ScenarioStructureBSP`, `…Leaf`, `…CollisionMaterial`, `…BreakableSurface` | `types/tags.h` | the three tag offsets 0xa8 / 0xe4 / 0x170 and the strides 0x10 / 0x14 / 0x30 all land exactly |
| `GlobalsMaterial` (stride 0x374, +0x94 `ground_friction_scale`) | `types/tags.h` | 0x507cc0 reads +0x94, +0x98, +0x9c, +0xa0, +0xa4 as the five friction scales |
| `DamageEffect` (+0x1d0/0x1d4/0x1d8, per-material table at +0x200) | `types/tags.h` | 0x4ffde0 |
| `collision_result` (0x50) | `types/projectiles.h` | already owned there; this module is the *producer* (0x505880) but redeclaring it would clash. The refinements it justifies are listed in section 6 and at the bottom of `types/physics.h`. |
| `object`, `damage_data`, `bsp_leaf_reference` | `types/objects.h` | |
| `projection_axis_pair` / `k_projection_axes` at 0x0065c29c | `types/math.h` | every 3D-to-2D projection in the module indexes it with `((component > 0) + axis*2)*4` |
| `real_bounds` | NOT reused | 0x50b290/0x50b4d0 read the pair upper-first (`range[0] - range[1]` is the span), the opposite of `real_bounds`, so `physics_scalar_range` is declared locally |

---

## 2. Structs defined, and what established each field

### `collision_bsp_sphere_query` (0x228) and `collision_bsp_sphere_result` (0x1010)

* Size 0x228 from the stack frame of **0x501980** (`local_228` .. and `FUN_00501a10(local_228, 0)`).
* 0x08 `breakable_surfaces`, 0x0c `center`, 0x10 `radius` are the three arguments 0x501980
  stores (`local_220`, `local_21c`, `local_218`).
* 0x00 `bsp`, 0x04 `breakable_surface_count`, 0x14 `result` are read by **0x501a10**
  (`*param_1`, `param_1[1]` as a short in 0x501d20, `param_1[5]`) but Ghidra lost the stores in
  0x501980 — they arrive in registers. UNRESOLVED only in the sense of "who writes them"; what
  they hold is unambiguous from the readers. Callers confirm the argument order:
  0x505540 passes `breakable_surface_globals + 1 + bsp_index*0x20`, 0x506440 the same plus
  the sphere centre and `radius + 0.0625`.
* 0x18 `plane_count` / 0x1c `planes[128]` from 0x501a10 (`param_1[6]`, `param_1[param_1[6]+7]`).
  128 entries is forced: 0x1c + 128*4 = 0x21c, exactly where the projection cache starts.
* 0x21c/0x21e/0x220/0x224 from 0x501a10 (`*(short *)(param_1 + 0x87)`,
  `*(bool *)((int)param_1 + 0x21e)`, `param_1[0x88]`, `param_1[0x89]`) and re-read by 0x501c90
  and 0x501d20.
* The result block comes from **0x501980** (four counters zeroed at 0x000, 0x404, 0x808, 0xc0c),
  **0x501a10** (leaves at 0xc0c/0xc10), **0x501d20** (vertices at 0x808/0x80c, edges at
  0x404/0x408, surfaces at 0x000/0x004) and is consumed in that same word order by
  **0x503d90** (`unaff_EDI[0x202]`, `unaff_EDI[0x101]`, `*unaff_EDI`).

### `collision_bsp_segment_query` (0x28) and `collision_bsp_segment_result` (0x418)

* 0x28 from the frame of **0x502060** (`local_28[4]` .. `local_4`).
* Field map from **0x502140**: `*param_1` flags, `[1]` bsp, `[2]` breakable count (short),
  `[3]` bit vector, `[4]` origin, `[5]` delta, `[6]` result, `[7]` last leaf, `[8]` leaf type
  byte, `[9]` crossing plane. 0x502060 writes 0x04, 0x08, 0x0c, 0x10, 0x14, 0x1c, 0x20, 0x24;
  0x00 (flags) and 0x18 (result) come in registers, and 0x505880 supplies both.
* Result field map from the hit store at the end of 0x502140 (+0x00 t, +0x04 plane pointer,
  +0x08 surface, +0x0c plane index, +0x10 flags, +0x11 breakable, +0x12 material) and the leaf
  append (`result + 0x14` count, `result + 0x18 + n*4`, overflow written to `result + 0x414`,
  which fixes the array at 256). **0x505880** independently declares `int local_404[257]` at
  exactly result+0x14, which closes the struct at 0x418.
* The flag bits are the five tests at the top of the leaf branch in 0x502140 (`uVar15 & 1`,
  `& 2`, `& 4`, `& 8` against surface flag 0x02, `& 0x10` against surface flag 0x08).

### `collision_bsp_pill_query` (0x22c) and `collision_bsp_pill_result` (0x420)

* 0x22c from the frame of **0x502730** (`local_22c[4]`, `FUN_005027a0(local_22c, 0)`).
* Field map from **0x5027a0**: `*param_1` bsp, `[1]` origin, `[2]` delta, `[3]` radius,
  `[4]` result, `[5]` plane count, `[6..]` plane stack, `0x218` axis, `0x21a` sign,
  `[0x87]`..`[0x8a]` the projected origin and direction, which **0x502d60** reads back as the
  2D line it walks the leaf plane tree with.
* Result: +0x00 t, +0x04..0x13 plane, +0x14 surface, +0x1a material, +0x1c leaf count,
  +0x20 leaves; **0x502e70** writes plane.d = FLT_MAX (0x7f7fffff) for an edge contact, and the
  overflow store at +0x41c is leaves[255], so the size is 0x420. **0x506040** declares
  `int local_404[257]` at result+0x1c, which agrees.

### `collision_bsp_boundary_clip` (0x18)

From **0x5017f0** only: `in_ECX[0..5]` seeded to -FLT_MAX / -NAN / -NAN / +FLT_MAX / -NAN /
-NAN, then written as (t, edge index, far surface index) twice. The two index fields are moved
through float registers, which is why Ghidra types them as `float`.

### `physics_model_sphere` / `_pill` / `_shape` / `physics_model` / `physics_model_contact`

* Bases and strides from **0x504260** and **0x504bb0**, which both walk back from a winning
  index with `base + i*0x1c + 8`, `base + 0x1c08 + i*0x28`, `base + 0x4408 + i*0x68`.
* Counts: both query functions index `(short *)(param_1 + type*2)`, so the three counts are
  int16 at 0x00/0x02/0x04 and 0x06 is padding.
* Sphere fields from **0x503360**, pill from **0x503490**, shape from **0x5038a0**. The shape
  vertex array runs 0x28..0x68, i.e. 8 `Point2D`, and 0x5038a0 stores `vertex_count` straight
  from the surface edge-loop walk without clamping it.
* Total 0xac08 is confirmed twice by the stack reservations in **0x507170** and **0x507ac0**
  (`undefined1 local_ac08[44036]`).
* The two provenance words: **0x5061c0** calls `FUN_00503360(..., object_index, 0xffffffff, 0,
  0xff)`, and **0x503c50** passes the surface index in the same second slot but forces it to -1
  when an object index is present. So 0x00 is the object handle (-1 for the world) and 0x04 is
  the collision BSP surface (-1 for an object).
* `physics_model_contact` (0x2c) is the common output of 0x504260 (depth) and 0x504bb0
  (fraction + point); **0x5067b0** keeps an array of them at the same 0x2c stride
  (`sVar5 * 0x2c + param_6`), and reads `+0x10` of entry n as the plane it slides along.

### `object_collision_context` (0x10)

From **0x504e10**: object handle, `ModelCollisionGeometry` from Object tag +0x7c, `object +
0x180` (region permutations, `types/objects.h` names it), `object + object->nodes.offset`
(the int16 at object +0x1f2). **0x5055b0** reserves 16 bytes for one (`local_46c[16]`).

### `object_node_collision_result` (0x420)

From **0x504f60** (`*out = node index; out[1] = node->region; out[2] = permutation;
*(undefined4 *)(out + 4) = 0x7f7fffff` i.e. the t of the embedded segment result at +0x08) and
the way **0x5055b0** reads the whole thing back (`local_420`, `local_41e`, `local_41c`, then
`local_418` .. `local_406` which is exactly a `collision_bsp_segment_result` at +0x08).

### `object_physics_context` (0x3c)

From **0x5074b0**: object handle, `Physics` from Object tag +0x8c, then a `real_matrix4x3` at
+0x08 with the scale forced to 1.0. Every consumer treats it as a matrix4x3:
**0x5090c0** does `p[3]*x + p[6]*y + p[9]*z + p[0xc]`, and **0x507cc0**, **0x507790** call
`matrix4x3_transform_point(param_1 + 2)`. Size 0x3c is pinned by `local_45c[60]` in
**0x5055b0**.

### `object_physics_ray_result` (0x14)

From **0x507610**: `{float t; real_plane3d plane;}`, t seeded to FLT_MAX, the plane rotated
back to world at the end. 0x5055b0 reads it as `local_480, local_47c, local_478, local_474,
local_470`.

### `mass_point_state` (0x130)

The stride is the memset at the top of **0x507cc0**
(`(*(int *)(local_10 + 0x74) * 0x130) >> 2` dwords), and the last write is the torque at word
index 0x4b, i.e. bytes 0x12c..0x12f, which closes the record exactly.
Word index -> field, all from 0x507cc0 unless noted:

| words | bytes | field | source |
|---|---|---|---|
| 0 | 0x000 | flags | 0x507cc0 sets bits 1, 2, 8, 0x10; **0x507ac0** sets/clears bit 4 |
| 1..3 | 0x004 | position | `matrix4x3_transform_point(param_1 + 2)` of `PhysicsMassPoint.position` |
| 4..6 | 0x010 | forward | tag +0x44 rotated |
| 7..9 | 0x01c | UNRESOLVED | zeroed by the memset, never written; the slot a `left` vector would occupy |
| 10..0xc | 0x028 | up | tag +0x50 rotated |
| 0xd | 0x034 | leaf_index | `FUN_005013a0` |
| 0xe | 0x038 | cluster_index | structure BSP leaves +0x08 |
| 0xf..0x11 | 0x03c | offset | position - object->position |
| 0x12..0x14 | 0x048 | velocity | object angular_velocity x offset, + object velocity |
| 0x15..0x17 | 0x054 | tangential_velocity | velocity minus the resting-plane normal component |
| 0x18..0x1b | 0x060 | resting_plane | **0x507ac0**, seeded from the constant at 0x0069c53c |
| 0x1c | 0x070 | material_type | **0x507ac0** via **0x507a40** |
| 0x1d | 0x074 | ground_depth | **0x507ac0** |
| 0x1e | 0x078 | UNRESOLVED | zeroed, never written |
| 0x1f | 0x07c | water_depth | `FUN_0053ee00` |
| 0x20 | 0x080 | ground_normal_magnitude | spring (ground_depth / ground_depth tag) minus damper |
| 0x21..0x23 | 0x084 | ground_normal_force | magnitude * plane normal |
| 0x24..0x2c | 0x090 | ground friction: force, parallel, perpendicular | **0x507c00** writes `[0..2]` from `[3..5]*s1 + [6..8]*s2` |
| 0x2d | 0x0b4 | buoyancy_magnitude | `mass / density * Physics.water_density * fade` |
| 0x2e..0x30 | 0x0b8 | buoyancy_force | always `(0, 0, magnitude)` |
| 0x31..0x39 | 0x0c4 | water friction triple | same 9-float shape |
| 0x3a..0x42 | 0x0e8 | air friction triple | same 9-float shape |
| 0x43..0x45 | 0x10c | powered_force | water lift, air lift, thrust, antigrav all accumulate here |
| 0x46..0x48 | 0x118 | total_force | the six-term sum at the end of the loop |
| 0x49..0x4b | 0x124 | torque | offset x total_force |

### `powered_mass_point_state` (0x60)

Stride 0x60 from **0x507cc0** (`sVar5 * 0x60 + param_2`). The seven leading floats are read at
`[0]`..`[6]` and each one is gated on the matching `PhysicsPoweredMassPointFlags` bit
(0x01 ground_friction, 0x02 water_friction, 0x04 air_friction, 0x08 water_lift, 0x10 air_lift,
0x20 thrust, 0x40 antigrav) - seven flags, seven scalars, read in bit order.
The matrix at +0x2c is written by **0x507840**: `matrix4x3_from_quaternion` then an in-place
3x3 transpose (it swaps +8/+0x10, +0xc/+0x1c, +0x18/+0x20 relative to the matrix base, which is
exactly m01/m10, m02/m20, m12/m21). 0x2c + 0x34 = 0x60 closes the record.
0x1c..0x2b (0x10 bytes) is UNRESOLVED - nothing in this module reads it.

### `breakable_surface_globals` (0x4204)

* 0x000 `initialized`: `*DAT_006b8d78 != '\0'` guards 0x4ffde0, 0x4fff20 and 0x500090.
* 0x001 bit vector: every access is `base + 1 + (bsp_index*8 + (surface >> 5))*4`, and
  **0x505540** / **0x506440** hand `base + 1 + bsp_index*0x20` straight to the BSP queries as
  the `breakable_surfaces` argument, so the per-BSP stride is 0x20 = 256 bits.
* 0x204 health: `base + 0x204 + (bsp_index*0x100 + surface)*4`.
* `k_maximum_structure_bsps = 16` is INFERRED from the 0x203-byte gap (16 * 0x20 = 0x200 plus
  3 bytes of padding). It is the only value that fits; it is not read out of an allocation
  site, because the allocation is not in this module.

### `physics_scalar_range` (0x08) and `physics_scalar_rates` (0x10)

From **0x50b290** (`in_ECX[1] <= v <= *in_ECX`, wrap by `*in_ECX - in_ECX[1]`) and
**0x50b370** (`in_EDX[0..3]` as max-positive, max-negative, accel-positive, accel-negative).
See section 5 - ownership of these five functions is uncertain.

---

## 3. Enums

* `collision_bsp_segment_flags` - the five tests in 0x502140.
* `collision_bsp_leaf_type` - `(leaf->flags & 1) + 1`, or 3 for a -1 child, in 0x502140.
* `physics_model_shape_type` - the `sVar2 == 0 / 1 / 2` dispatch in 0x504260 and 0x504bb0.
* `mass_point_flags` - the five set/clear pairs in 0x507cc0 and 0x507ac0.
* `point_physics_result_flags` - `local_88` in 0x50b530: 1 air, 2 water, 4 collided, 8 water
  surface.

---

## 4. Tag offsets this module proves

`PhysicsMassPoint` (stride 0x80): +0x20 powered index, +0x28 relative_mass, +0x2c mass,
+0x30 relative_density, +0x34 density, +0x38 position, +0x44 forward, +0x50 up,
+0x5c friction_type, +0x60/+0x64 friction scales, +0x68 radius. 0x507cc0 uses +0x2c as the
force scale and `+0x2c / +0x34` (mass over density, i.e. volume) as the buoyancy term.

`Physics`: +0x08 mass, +0x0c centre of mass, +0x1c gravity_scale, +0x20 ground_friction,
+0x24 ground_depth, +0x2c/+0x30 ground normal k1/k0, +0x38 water_friction, +0x3c water_depth,
+0x40 water_density, +0x48 air_friction, +0x5c inertial matrix block, +0x68 powered mass
points, +0x74 mass points. 0x5097e0 uses `*(int *)(Physics + 0x60) + 0x24` - that is
`inertial_matrix_and_inverse.pointer` plus 0x24 - as the INVERSE inertia matrix, so the block
really is two `Matrix` rows of 0x24 each.

`Object` tag: +0x7c collision_model tag id, +0x8c physics tag id (both are the TagID inside
a `TagDependency` that starts 12 bytes earlier).

`DamageEffect`: 0x4ffde0 reads +0x1d0 damage_lower_bound, randomises between +0x1d4 and +0x1d8
(`damage_upper_bound[2]`), and scales by `*(float *)(tag + 0x200 + material_type*4)`, which is
the `dirt / sand / stone / snow / wood / …` per-material table. 0x4fff20 uses +0x04 as the
effect radius.

`damage_data` (types/objects.h): 0x4ffde0 reads +0x40 (`random_blend`, already named) and
+0x4c, which `types/objects.h` currently calls `unknown_4c`. **It is the collision material
type of the surface being damaged** - it is used as the index into the DamageEffect
per-material table at +0x200. Worth folding back into `types/objects.h`.

---

## 5. Misattributed or foreign functions

* **0x5066e0 `point3d_project_onto_line`** - a pure math helper (project a point onto an
  infinite line). Belongs in the math module; already named, no types of its own.
* **0x507430** - a clamped inverse lerp of a value between two references, returning 0 or 1
  outside. Pure math, one caller inside this module and one in 0x509e80. No types.
* **0x501470** - two-line thunk that calls `FUN_0044d860` (math module) and returns its second
  argument unchanged. No types.
* **0x500090** (4764 bytes) - this is an **effects** function, not physics. It picks a random
  sample position inside a broken surface and spawns the break particle effect: it drives the
  cseries random seed at 0x00719cd4 with the LCG `x*0x19660d + 0x3c6ef35f`, reads the
  `GlobalsBreakableSurfaceParticleEffect` block out of the Globals tag, and calls
  `color_interpolate` and the particle spawner at 0x006267f0. Only its two entry conditions
  (the breakable-surface bit vector and the health array) are physics state, and those are
  covered by `breakable_surface_globals`. No effect/particle structs are declared here.
* **0x50b290 / 0x50b2f0 / 0x50b370 / 0x50b460 / 0x50b4d0** - five scalar-range helpers with one
  caller each, and the callers are outside the module slice. They move a float toward a target
  inside a range with optional wraparound, gated by a four-scalar rate record. The shape
  (a wrapping range plus asymmetric accelerate/decelerate limits) matches turret or seat aiming
  rather than rigid-body physics. `physics_scalar_range` and `physics_scalar_rates` are
  declared in `types/physics.h` so the module compiles, but ownership is UNSURE and they should
  move if the unit/vehicle pass claims them.
* **0x507590 / 0x507610 / 0x507790 / 0x507840 / 0x507ac0 / 0x507c00 / 0x507cc0 / 0x508a10 /
  0x508b70 / 0x5090c0 / 0x5096f0 / 0x5097e0 / 0x509e80** - phase 2 named several of these
  `antenna_*`. **That is wrong.** They all resolve the object Physics tag (Object tag +0x8c) and
  iterate `Physics.mass_points`, never the `antenna` widget in `types/objects.h`. The dispatch
  in 0x5061c0 and 0x505350 reaches them for `object_type == 1` (vehicle) under mask bit
  0x400000. Suggested renames: `object_physics_*` / `mass_point_*`. `antenna_test_ray_against_
  vertex_spheres` (0x507610) is `object_physics_test_ray_against_mass_points`;
  `antenna_object_compute_vertex_forces` (0x507cc0) is `object_physics_compute_mass_point_
  forces`; `antenna_object_integrate_and_test_rest` (0x5097e0) is
  `object_physics_integrate_and_test_at_rest`.
* **0x509e80** (5129 bytes) is the alternate single-mass-point path. 0x507840 picks it when
  `Physics.radius > 0.0`; the multi-mass-point path runs when the radius is 0 or less. Its
  local state is the same `mass_point_state` shape held in stack locals, so no extra struct.

---

## 6. Cross-module corrections worth folding back

### `types/projectiles.h` -> `collision_result`

0x505880, 0x505ab0-ish tail, 0x5055b0, 0x506040 and 0x50b530 between them name everything
projectiles.h left as an unknown:

| offset | projectiles.h | what this module writes |
|---|---|---|
| 0x04 | `unknown_04[8]` | `int32_t first_leaf` - leaves[0] of the segment walk |
| 0x08 | (same) | `int16_t first_cluster` |
| 0x0c | `leaf` | last leaf of the walk (the endpoint leaf) |
| 0x10 | (same) | its cluster |
| 0x30 | `unknown_30` | the `d` of the contact plane; 0x24..0x33 is one `real_plane3d` |
| 0x3c | `unknown_3c` | `int16_t region_index` (collision node region) |
| 0x3e | `marker_index` | collision **node** index, not a marker index |
| 0x40 | `unknown_40` (uint32) | `int16_t permutation_index`; 0x42 stays untouched |
| 0x48 | `unknown_48` | `int32_t plane_index`, sign bit set on a back-face hit |
| 0x4d | `unknown_4d` | the surface `breakable_surface` index |
| 0x4e | `unknown_4e` | `int16_t` index into `ScenarioStructureBSP.collision_materials` |

Also: `_collision_result_type_water_surface = 0` is not only a projectile thing. 0x505880
produces type 0 from its fog-plane test, and `point_physics` (0x50b530) reads type 0 as
"hit the water surface".

### `types/units.h` -> `biped_data` / `vehicle_data` at 0x508 and 0x514

**0x507840** reads six consecutive floats at `object + 0x508` and `object + 0x514`, adds them
to the force and torque it is about to integrate, and then zeroes all six. So for objects that
reach this path they are `real_vector3d accumulated_force` (0x508) and
`real_vector3d accumulated_torque` (0x514). That matches `vehicle_data.unknown_508` /
`unknown_514` (both currently `uint32_t`, both "zeroed by 0x570b00"), and **conflicts** with
`biped_data.ground_normal` at 0x514. Since 0x507840 is only reached through the object Physics
tag, the vehicle reading is the likely one and `biped_data` is probably right for bipeds - the
two extensions overlap at 0x4cc, so both can be true. Flagging rather than changing.

### `types/objects.h` -> `damage_data.unknown_4c`

It is the collision material type, see section 4.

---

## 7. Unresolved

* `collision_bsp_sphere_query` 0x006, `collision_bsp_segment_query` 0x0a and 0x21..0x23,
  `collision_bsp_pill_query` 0x21b - alignment padding, never touched.
* `collision_bsp_pill_result` 0x018 - two bytes between the surface index and the material
  index that nothing in the module writes.
* `mass_point_state` 0x01c..0x027 and 0x078 - zeroed by the memset, never written. The 0x01c
  triple sits exactly where a `left` basis vector would go between `forward` and `up`.
* `powered_mass_point_state` 0x1c..0x2b - 16 bytes this module never reads.
* `physics_model_shape` 0x23 - one byte of padding after `projection_sign`.
* `k_maximum_structure_bsps = 16` is inferred from the gap in `breakable_surface_globals`, not
  read from an allocation.
* `object_physics_context` translation (0x30..0x3b): 0x5074b0 builds it from
  `object_get_position` and then runs `matrix4x3_transform_point` over the negated
  `Physics.center_of_mass`, and Ghidra attributes the three negation stores to the matrix
  translation slot itself. The net effect (a model-space-to-world matrix) is certain because
  every consumer uses it that way; the exact instruction order is not.
* `GlobalsMaterial + 0x2d4`, read by 0x4ffde0 through `FUN_0053e7c0` as the divisor of the
  breakable-surface damage, is almost certainly the breakable-surface vitality, but the field is
  inside a `_pad` run in `types/tags.h` so it is not named here.
* Globals 0x0069c538, 0x0069c54c and 0x00746f90 are marked UNSURE in the header; the first two
  are float constants whose exact role is inferred from one use each, and 0x00746f90 is part of
  the structure-BSP globals block that another module should own.
