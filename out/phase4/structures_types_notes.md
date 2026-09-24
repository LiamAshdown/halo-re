# structures module: type recovery notes

Header: `types/structures.h`. Smoke gate: `out/phase4/structures_smoke.c`, checked with

    C:\Users\Liam-\halo-re> C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/structures_smoke.c
    C:\Users\Liam-\halo-re> C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -I types out/phase4/structures_smoke.c

Both pass. This host gcc really does produce 4-byte pointers under `-m32`, so every `PTRS32`-gated
size (including `detail_object_frame` == 0x5210, `detail_object_globals` == 0xa430 and
`structure_fog_environment` == 0x4c) is machine-checked rather than hand-checked.

## The headline: this module barely owns any structure

All 48 functions are readers of one resident tag, the `ScenarioStructureBSP` whose tag data pointer
lives at 0x00746f9c. `types/tags.h` already carries that tag and all 40-odd of its sub-blocks, and
every walk in the module indexes them through exactly the `TagReflexive` pointer offsets and strides
that header declares. So the useful output of this pass is mostly *confirmation of tags.h* plus the
handful of runtime records the module builds on top of it. The offset table in the header comment
lists all 18 confirmed reflexive offsets; the strides that confirm them are:

| tag block | stride the code uses | tags.h size |
|---|---|---|
| `ScenarioStructureBSPCluster` | 0x68 | 0x68 |
| `ScenarioStructureBSPLeaf` | 0x10, cluster at +0x08 | 0x10 |
| `ScenarioStructureBSPSurfaceReference` | 0x08, node at +0x04 | 0x08 |
| `ScenarioStructureBSPSurface` | 0x06 | 0x06 |
| `ScenarioStructureBSPLightmap` | 0x20, materials at +0x14 | 0x20 |
| `ScenarioStructureBSPMaterial` | 0x100 | 0x100 |
| `ScenarioStructureBSPSubcluster` | 0x24, surface_indices at +0x18 | 0x24 |
| `ScenarioStructureBSPMirror` | 0x40 | 0x40 |
| `ScenarioStructureBSPClusterPortal` | 0x40 | 0x40 |
| `ScenarioStructureBSPFogPlane` | 0x20 | 0x20 |
| `ScenarioStructureBSPFogRegion` | 0x28, fog at +0x24 | 0x28 |
| `ScenarioStructureBSPFogPalette` | 0x88 | 0x88 |
| `ScenarioStructureBSPBackgroundSoundPalette` | 0x74 | 0x74 |
| `ScenarioStructureBSPSoundEnvironmentPalette` | 0x50 | 0x50 |
| `ScenarioStructureBSPRuntimeDecal` | 0x10 | 0x10 |
| `ScenarioStructureBSPDetailObjectData` | 0x40 | 0x40 |
| `ScenarioStructureBSPGlobalDetailObjectCell` | 0x20 | 0x20 |
| `ScenarioStructureBSPGlobalMapLeaf` / `GlobalLeafPortal` | 0x18 | 0x18 |
| `ModelCollisionGeometryBSP3DNode` | 0x0c | 0x0c |

Two tag blocks get *new* field meanings out of this pass, both for fields invader still calls
padding:

- `ScenarioStructureBSPRuntimeDecal` is confirmed field by field by 0x005530d0: `decal_type` at
  +0x0c is read as a byte index into the scenario decal palette, and `yaw`/`pitch` at +0x0e/+0x0f
  are signed bytes turned into a direction with the scales 0.02473695 and 0.012368475 radians per
  count (i.e. 2*pi/254 and pi/254).
- `ShaderEnvironment` +0x30c and +0x310 (`_pad_30c[8]`) are two runtime floats 0x00553560 copies
  into its mirror result whenever the mirror shader is a `shadertype_environment`.

## Structs defined, and what established each field

### `structure_bsp_leaf_map` (0x1c)
The tail of `ScenarioStructureBSP`, from +0x26c to the end of the tag at +0x288.

- 0x005520b0 is handed this address in ECX and reads `leaves.pointer` at +0x08 and
  `portals.pointer` at +0x14. 0x005527f0 bounds-checks the same two blocks straight off the tag
  base at +0x270 (`leaf_map_leaves.count`) and +0x27c/+0x280 (`leaf_map_portals`). The only base
  that satisfies both is tag+0x26c, and a 0x1c-byte struct there ends exactly at the tag size
  0x288, which is what makes the split believable rather than arbitrary.
- The leading dword (`ScenarioStructureBSP._pad_26c`) is never read; it stays `unknown_00`.

### `structure_bsp_cluster_surface_run` (0x0c header + n int32)
- 0x00553a70 walks `cluster->surface_indices` as runs, not as the flat int32 list tags.h declares:
  it consumes three dwords, takes the third as the run length, then walks that many surface
  indices. 0x005537c0 picks this function only when `clusters[0].subclusters.count == 0`, so the
  run encoding is the no-subcluster form of the block.
- The two leading dwords are only ever consumed by the three
  `render_frustum_classify_point_side_planes` calls the inner loop makes per surface, whose
  arguments Ghidra lost to registers. **Unresolved:** `unknown_00`, `unknown_04`.

### `structure_bsp_visible_cluster` (0x1a0), array of 0x80 at 0x007c3390
- `cluster_index` at 0x00: written by `camera_cluster_portal_flood_recursive` (0x005545d0) as
  `(&DAT_007c3390)[visible_index * 0xd0] = cluster`, and read the same way by 0x00553920,
  0x00553a70 and 0x00554420. Ghidra types the base as int16, so 0xd0 units == 0x1a0 bytes.
- `screen_bounds_x` / `screen_bounds_y` at 0x04..0x13: the same function copies four dwords from
  `*0x00696744` into +0x04..+0x13 on first visit, and `polygon2d_bounds_expand` (0x00554a90) then
  writes exactly `[0]=min x, [1]=max x, [2]=min y, [3]=max y` over that block. Two `real_bounds`
  from types/math.h.
- **Unresolved:** 0x014..0x19f (0x18c bytes). 0x005544f0 makes a second pass over the finished
  list calling `render_camera_compute_frustum_bounds` and `chimera__render_camera_build_frustum`
  once per entry; nothing in this module reads the result, so the per-cluster clipped frustum
  layout belongs to the render/camera module. The count 0x80 is pinned by
  0x007d0390 - 0x007c3390 == 0xd000 == 0x80 * 0x1a0.

### `structure_bsp_mirror_result` (0x1c)
The out-block 0x00553560 fills through its third argument.

- 0x00..0x0f plane: four dwords copied straight from `ScenarioStructureBSPMirror` +0x00.
- 0x10, 0x14: `ShaderEnvironment` +0x30c / +0x310 when the mirror shader's `shader_type` (Shader
  +0x24) is 3 (`shadertype_environment`), zero otherwise.
- 0x18 `cluster_index`: `*(short *)(param_3 + 6)`.
- **Unresolved:** 0x1a..0x1b. Nothing writes them; the size is inferred as 4-byte aligned 0x1a.
  Note the phase-2 summary for 0x00553560 called this a *face/material* search. It is not: the
  stride is 0x40 off `cluster->mirrors` (+0x50/+0x54), so it searches **mirrors**, and what it
  returns is the mirror plane, not a material.

### `structure_fog_environment` (0x4c)
The out-block 0x00555330 fills through ESI, built from the Fog tag 0x00555270 resolves.

- 0x00 `fog_flags` from `Fog.flags` low word; 0x02 bit 0 set on the sky-tag fallback path.
- 0x1c `plane_mode`: 1 when `cluster.fog` had bit 0x8000 (a real fog plane), 2 when the cluster is
  wholly inside the fog region, 0 when there is no fog.
- 0x20..0x2f plane, copied from `ScenarioStructureBSPFogPlane.plane` (+0x04).
- 0x30/0x34/0x38 from `Fog.color` (+0x78), 0x3c from `Fog.maximum_density` (+0x58), 0x40 from
  `Fog.opaque_distance` (+0x60), 0x44 from `Fog.opaque_depth` (+0x68).
- 0x48 pointer to `&Fog.flags_1` (fog tag data + 0x84), the screen-layer block.
- **Unresolved:** 0x04..0x1b (0x18 bytes). No function in this module reads or writes them, and
  0x00555330 only zeroes +0x00, +0x1c and +0x48 on entry, so some other producer owns that span.
  Most likely candidate is whatever also fills the atmospheric-fog part of the record for the
  renderer. UNSURE.

The lookup chain 0x00555270 implements is worth recording because it is not obvious from tags.h:
`cluster.fog` (+0x02) is 0xffff for none; with bit 0x8000 set the low 15 bits index `fog_planes`
and that plane's `front_region` names the fog region, otherwise the low 15 bits index `fog_regions`
directly. The region's `fog` (+0x24) indexes `fog_palette`, whose dependency at +0x20 is the Fog
tag. On the sky path it instead returns `Sky.indoor_fog_screen.tag_id` (Sky +0xa4), which is also a
fog tag -- that is what confirms the return type.

### detail objects: `detail_object_batch` (0x18), `detail_object_layer_batches` (0x08), `detail_object_render_list` (0x08), `detail_object_frame` (0x5210), `detail_object_globals` (0xa430), `detail_object_cell_key` (0x08)
The whole layout falls out of arithmetic in 0x00552260 (which reserves the block) and 0x005522d0
(which fills it), and the sizes chain with no slack:

- batch stride 0x18: the indexed store is `base + (count_in_layer + layer * 0x1b) * 0x18`.
- 0x20 layers (one bit per layer in `cell.valid_layers_flags`) x 0x1b batches x 0x18 == 0x5100,
  which is exactly where the 8-byte per-layer descriptors start (`base + 0x5100 + i * 8`), and the
  per-layer batch pointer advances by 0x288 == 0x1b * 0x18 per layer.
- 0x20 descriptors x 8 == 0x100, so the two-member render-list header lands at 0x5200 and the
  cached cell at 0x5208..0x520f, ending the frame at 0x5210.
- 0xa420 / 0x5210 == 2 exactly, and 0xa420 is where 0x00552260 seeds the `{0, 0, 1.0f, 0}` quad
  that 0x005522d0 uses as the fallback `z_reference` when `detail_objects.z_reference_vectors` is
  empty -- i.e. a default `ScenarioStructureBSPGlobalZReferenceVector`. 0x5210 * 2 + 0x10 == 0xa430,
  the exact reservation size.
- **Unresolved / UNSURE:** only `frames[0]` is ever written here (the block base is used directly).
  `frames[2]` is an inference from `0xa420 == 2 * 0x5210`; the second frame has no store anywhere in
  this module. Also `detail_object_batch.unknown_10` is never written.

`detail_object_cell_key` is the key the two binary searches take: 0x00552710 returns the first cell
not less than it and 0x00552780 the first cell greater, comparing the three int16 lexicographically
over the 0x20-byte cell stride. Together they bracket one (x, y) column, which is why the caller
sweeps `cell_z` by +/-1 around it. That also independently confirms
`ScenarioStructureBSPGlobalDetailObjectCell`'s first six bytes.

### `polygon2d` (0x804)
Not strictly owned here -- `types/math.h` owns `polygon2d_clip_to_planes`,
`polygon2d_clip_to_plane`, `polygon2d_points_classify` and `polygon3d_clip_to_plane` -- but math.h
declares no record for the polygon itself, and this module cannot be described without one. Size is
pinned three ways: 0x00554850 writes the count at +0x00 and projected `real_point2d` from +0x04
onwards; every caller stack buffer is 0x804 == 4 + 0x100 * 8 (0x00553560 has three of them,
0x005545d0 and 0x00554b00 one each); and 0x100 is the literal vertex limit handed to
`polygon2d_clip_to_planes`. If math.h later grows its own `polygon2d`, delete this one.

### `structure_bsp_overlap` and `structure_fog_plane_mode` enums
`structure_bsp_overlap` is the shared 0/1/2 return of `aabb_overlap_classify` (0x005541b0) and
`frustum_planes_classify_box` (0x00554260). Confirmed by three independent callers that take the
`min` of the two results and by `bsp3d_node_query_recursive`, which skips both tests entirely once
the inherited value is 2.

## Misattributed functions

- **0x0053f150 belongs to the sound module, not here.** It is 0x12ce0 bytes below the rest of the
  module and calls 0x0053ec30, its neighbour. What it does is interpolate the *listener's*
  `SoundEnvironment` parameters: the 18 dwords it copies are exactly `sizeof(SoundEnvironment)` ==
  0x48, and the 12 values it rate-limits are exactly that tag's 12 reals from `room_intensity`
  (+0x08) to `hf_reference` (+0x34), with per-field clamps 0.03 / 0.03 / 0.3 / 0.1 / 0.03 / 0.03 /
  0.09 / 0.03 / 0.003 / 0.03 / 0.03 / 600.0 per tick -- the 600.0 on the last one is Hz, which
  clinches it. The priority compare is `SoundEnvironment.priority` at +0x04. Per the brief I did
  **not** define its types; instead:
  - `0x00746f94` is a pointer to the sound module's interpolated cache: `uint8_t` latch at +0x30
    (the source fog's `is_water` flag) followed by an 0x48-byte `SoundEnvironment` copy at
    +0x34..+0x7b, which the function hands back to the caller by pointer.
  - `0x0065e508` is `k_default_sound_environment`, the 0x48-byte record used when no cluster
    environment resolves. The globals list in the pack reaches 0x0065e53c == 0x0065e508 + 0x34,
    exactly the last real, which corroborates the size.
  - It does establish structure_bsp offsets +0x1fc/+0x200 (background sound palette) and +0x20c
    (sound environment palette), and the Fog tag's `background_sound.tag_id` at +0x100 and
    `sound_environment.tag_id` at +0x110. Those are recorded in structures.h.

- **Generic geometry helpers that happen to live in this range** (kept, since the code is here, but
  they are not structure-BSP specific and a later math pass may want to move them):
  `aabb_overlap_classify` 0x005541b0, `frustum_planes_classify_box` 0x00554260,
  `polygon2d_bounds_expand` 0x00554a90, and the `lower_bound`/`upper_bound` pair 0x00552710 /
  0x00552780 (which are STL-shaped: `count/2` bisection with the "skip the pivot" remainder
  `count + (-1 - half)`, specialised to the 0x20-byte detail-object cell and its three-int16 key).

- **Nothing else is misplaced.** 0x005527f0..0x00552de0 look like they belong to the renderer, but
  they are the BSP-side debug surface visualisation (picked polygon, leaf-map counting, masked
  leaf-face geometry submission) and they own 0x006e3ad8/0x006e3adc, so they stay.

## Other unresolved offsets

- `0x007d0394 surface_visible_bits`: declared `uint32_t[0x1000]` (0x4000 bytes). 0x005537c0 only
  ever clears `((surfaces.count + 0x1f) >> 5)` dwords, and the matching *local* bitset in
  0x00553d80 is a 0x4000-byte stack buffer, so 0x4000 is the real capacity. But the next global I
  can place is 0x00850394, which is 0x80000 bytes later -- so 0x007d4394..0x00850393 is a hole no
  function in this module touches. Do not assume the bitset is that large.
- `0x0072278c`: a pointer global whose target byte 0x005530d0 clears on every exit path and tests
  before spawning decals. Named `runtime_decals_suppressed`; the owning block is probably the decal
  globals in types/effects.h. UNSURE.
- `0x007c3100..0x007c3170`: the render camera block, read but not owned. What this module pins:
  +0x08 an int32 render target/window index tested against -1, +0x14 the camera position (three
  floats, used both for the detail-object cell and for the portal band test), +0x20 its forward
  vector, +0x54 the portal-visibility tolerance, and +0x68 the projection context 0x00554850
  transforms portal vertices through (its +0x10 is fed to `matrix4x3_transform_point`, and a byte
  at +0x24 relative to the camera flips the winding). The camera module should confirm these.
- `0x006e3afc cluster_visible_index[0x200]`: Ghidra types it as int16 and indexes it by cluster,
  which fits 0x200 clusters ending at 0x006e3efc; 0x006e3efc..0x006e3f00 is then a 4-byte gap
  before the flood stamps. Plausible but not proved.
- `0x006e3af8 flood_recursion_bits`: a pointer, not a bitset. 0x005544f0 sets it to a 0x40-byte
  zeroed stack array and `camera_cluster_portal_flood_recursive` sets the current cluster's bit on
  entry and clears it on exit -- an on-the-recursion-stack marker, distinct from the visible set.

## Globals claimed for this module

See the block at the end of `types/structures.h`. In brief: 0x007c3344..0x007c334e (the camera's
leaf/cluster/sky), 0x007c3350 + 0x007c3390 + 0x007d0390 (visible clusters), 0x007d0394 +
0x00850394 + 0x00850398 (visible surfaces), 0x006e3ad8/0x006e3adc + 0x0069fa40..0x0069fa4c +
0x00724a45/0x00724a46 (picked-polygon debug), 0x006e3ae0..0x006e3aec (the fog plane vector),
0x006e3af0/0x006e3af8/0x006e3afc (portal flood state), 0x006e3f01/0x006e3f04/0x006e3f08 (the
cluster flood stamps that types/objects.h and types/physics.h both already reference as
read-not-owned -- this module is the writer), 0x0072277c (detail objects) and 0x0072278c.

Two sanity checks on those extents: 0x007c3350 is 0x40 bytes long and ends exactly where
0x007c3390 begins, and `visible_surface_indices` at 0x00850398 with 0x4000 int32 entries ends at
0x00860398, just 0x18 bytes short of the object globals at 0x008603b0 (types/objects.h).
