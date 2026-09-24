# `structures` - the structure BSP: clusters, portals, visibility, detail objects, fog

Retail Halo PC `halo.exe` 1.0.10, `0x53f150 .. 0x555330`, plain C / MSVC 7.1 / x86. 47 files for
one function each, rewritten from the Ghidra decompilation against `types/structures.h`, with the
original decompile preserved verbatim at the bottom of each file inside `#if 0 ... #endif` for
diffing.

Gate: `python tools/build_check.py structures` -> **47 ok, 0 failed**.

Ghidra lists 48 entry points in the range. One of them (`0x53f150`) is not a structures function
and is deliberately absent; see "Misattributed functions". 47 + 1 = 48.

This module barely owns any structure of its own. Everything it reads hangs off one resident tag,
the `ScenarioStructureBSP` whose tag data pointer lives at `0x00746f9c`, and `types/tags.h` already
carries that tag and all 40-odd of its sub-blocks. What the module *does* own is the per-frame
visibility state derived from that tag, plus the detail-object game-state block. The reflexive
offsets and strides every walk in here uses are listed in the header comment of
`types/structures.h` and corroborated in `out/phase4/structures_types_notes.md`.

**Read the confidence columns before trusting a file.** Ghidra recovers this module badly: most of
the interesting functions pass their real arguments in registers that the decompiler drops
entirely, and several of its rendered expressions are byte-offset arithmetic that reads as scalar
arithmetic. Every file whose confidence is 0.7 or better was checked instruction by instruction
against `objdump -d -M intel bin/halo.exe` and quotes the instructions that fix its layout.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| cluster reference lists | `0x551e30`-`0x552020` | `cluster_partition_new` and the add/remove pair that chain `object_cluster_reference` records into both a cluster's list of objects and an object's list of clusters |
| small BSP lookups | `0x5520b0`-`0x552210` | the leaf-map vertex counting debug pass, `structure_surface_material_locate` (surface index -> owning lightmap + material), and the triangular cluster sound-distance matrix |
| detail objects | `0x552260`-`0x552780` | the `0xa430` game-state allocation, the per-frame render-list build (the largest function in the module), and the `lower_bound` / `upper_bound` pair over the global detail-object cell grid |
| picked-polygon debug | `0x5527f0`-`0x5528f0` | refresh and draw for the `bsp_poly` debug visualisation Chimera also signatures |
| debug surface draw | `0x552980`-`0x552d60` | three sibling "query surfaces, lock a geometry buffer, gather, submit, enumerate" routines plus the two leaf-face gatherers and the geometry-buffer submit helper |
| leaf-face enumeration | `0x552de0` | `structure_leaf_faces_for_each`: the four-callback walk every drawer above drives |
| runtime decals | `0x5530d0` | per-object switch-group bitset transitions, and the decal spawn that follows a leaving object |
| node bounds | `0x553380` | `bsp3d_node_bounds_decompress`: byte-quantised per-axis bounds against a parent rectangle |
| camera position | `0x553490` | resolve the camera's leaf, cluster and sky |
| mirrors | `0x553560` | search the visible clusters' mirrors for one the clipped view polygon still reaches |
| per-frame visibility | `0x5537c0`-`0x553c40` | the top-level reset + flood + expansion pass, the two expansion variants (subcluster boxes vs. per-cluster surface runs), and the cluster-set surface collector |
| BSP query | `0x553d80`-`0x554260` | `structure_bsp_query_surfaces` and the recursive `bsp3d_node_query_recursive` / `structure_bsp_leaf_query` pair, with the two generic box classifiers they lean on |
| portal flood | `0x554420`-`0x554b00` | visible-object collection, the camera visibility pass, the recursive portal flood, the portal projection / clip / sphere tests, and the 2D bounds expansion |
| cluster flood fill | `0x554cb0`-`0x554e30` | the radius seed and the two flood fills (radius, and caller-predicate) |
| surface resolution | `0x554fa0`-`0x555190` | find a leaf's surface on an accepted plane whose triangle contains a point, and the stepping walk that drives it |
| fog | `0x555270`-`0x555330` | resolve a cluster's Fog tag through the fog plane / region / palette chain, and build the fog environment record from it |

## Structures

Types this module defines live in `types/structures.h`. Tag blocks are in `types/tags.h` and
`data_array` / `datum_index` / `bit_stream` in `types/memory.h`; nothing is redefined here.
`structure_bsp_aabb` was considered and rejected: the 6-float box every test in this module passes
around is exactly `real_rectangle3d` from `types/math.h`, so that is what the files use.

### `polygon2d` - size `0x804`

Every portal, mirror and view polygon in the module. Callers allocate it on the stack; the clip
routines write the count and the point array through two separate pointers 4 bytes apart, which is
what pins the header.

| Offset | Type | Field |
|---|---|---|
| `0x000` | `int16_t` | `point_count` - `-1` back from `polygon2d_clip_to_planes` means "unchanged" |
| `0x002` | `int16_t` | `unknown_02` - alignment, never read |
| `0x004` | `real_point2d[0x100]` | `points` |

### `structure_bsp_leaf_map` - size `0x1c`

The tail of `ScenarioStructureBSP`, from tag `+0x26c` to its end at `+0x288`.

| Offset | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `unknown_00` - the tag's `_pad_26c`, never read |
| `0x04` | `TagReflexive` | `leaves` - `ScenarioStructureBSPGlobalMapLeaf`, stride `0x18` |
| `0x10` | `TagReflexive` | `portals` - `ScenarioStructureBSPGlobalLeafPortal`, stride `0x18` |

### `structure_bsp_cluster_surface_run` - `0x0c` header plus `surface_count` int32

The no-subcluster encoding of a cluster's `surface_indices` block, walked only by
`structure_bsp_expand_visible_clusters_by_plane`.

| Offset | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `unknown_00` - only consumed by the elided `render_frustum_classify_point_side_planes` calls |
| `0x04` | `int32_t` | `unknown_04` - same |
| `0x08` | `int32_t` | `surface_count` - that many int32 surface indices follow |

### `cluster_reference_group` - size `0x0c`

The three-global group `cluster_partition_new` fills through ESI. `types/objects.h` documents both
instances by address (`0x008603c0` noncollideable, `0x008603d0` collideable) but never named the
record; it is folded into `types/structures.h` by this pass.

| Offset | Type | Field |
|---|---|---|
| `0x00` | `datum_index *` | `cluster_first` - `datum_index[k_maximum_clusters]`, one chain head per cluster |
| `0x04` | `data_array *` | `cluster_object_references` - pool named `"cluster <name> reference"`: each cluster's chain of referencing objects |
| `0x08` | `data_array *` | `object_cluster_references` - pool named `"<name> cluster reference"`: each object's chain of referenced clusters |

Element type is `object_cluster_reference` (`types/objects.h`, stride `0x0c`), which is what the
`mov ebx,0xc` before both `game_state_new` calls confirms. Note that `types/objects.h`'s global
name for the third slot (`*_cluster_partition`) predates this and is a misnomer: it is a
`data_array`, not a partition.

### `structure_bsp_visible_cluster` - size `0x1a0`, array of `0x80` at `0x007c3390`

| Offset | Type | Field |
|---|---|---|
| `0x000` | `int16_t` | `cluster_index` |
| `0x002` | `int16_t` | `unknown_02` - alignment |
| `0x004` | `real_bounds` | `screen_bounds_x` - seeded from `*0x00696744`, grown by `polygon2d_bounds_expand` |
| `0x00c` | `real_bounds` | `screen_bounds_y` |
| `0x014` | `uint8_t[0x18c]` | the per-cluster clipped view frustum, written by the render module; nothing here reads it |

### `structure_bsp_mirror_result` - size `0x1c`

| Offset | Type | Field |
|---|---|---|
| `0x00` | `real_plane3d` | `plane` - copied from `ScenarioStructureBSPMirror.plane` |
| `0x10` | `float` | `unknown_10` - `ShaderEnvironment` `+0x30c`, zero for other shader types |
| `0x14` | `float` | `unknown_14` - `ShaderEnvironment` `+0x310` |
| `0x18` | `int16_t` | `cluster_index` |
| `0x1a` | `int16_t` | `unknown_1a` - never written |

### `structure_fog_environment` - size `0x4c`

| Offset | Type | Field |
|---|---|---|
| `0x00` | `uint16_t` | `fog_flags` - low word of `Fog.flags` |
| `0x02` | `uint8_t` | `flags` - bit 0: the fog came from the sky tag, not a cluster |
| `0x03` | `uint8_t` | `unknown_03` |
| `0x04` | `uint8_t[0x18]` | never read or written by this module - some other producer owns it |
| `0x1c` | `int16_t` | `plane_mode` - `structure_fog_plane_mode` |
| `0x1e` | `int16_t` | `unknown_1e` |
| `0x20` | `real_plane3d` | `plane` - only when `plane_mode` is `_structure_fog_plane_bounded` |
| `0x30` | `float` x3 | `color_red` / `color_green` / `color_blue` - `Fog.color` (`Fog +0x78`) |
| `0x3c` | `float` | `maximum_density` - `Fog +0x58` |
| `0x40` | `float` | `opaque_distance` - `Fog +0x60` |
| `0x44` | `float` | `opaque_depth` - `Fog +0x68` |
| `0x48` | `void *` | `screen_parameters` - `&Fog.flags_1`, i.e. fog tag data `+0x84` |

### detail objects: `detail_object_globals` - size `0xa430` at `*0x0072277c`

| Offset | Type | Field |
|---|---|---|
| `0x0000` | `detail_object_frame[2]` | `frames` - `0x5210` each; only `[0]` is ever written here |
| `0xa420` | `ScenarioStructureBSPGlobalZReferenceVector` | `default_z_reference` - seeded `{0, 0, 1.0, 0}` |

`detail_object_frame` (`0x5210`): `detail_object_batch batches[0x20][0x1b]` at `0x0000`,
`detail_object_layer_batches layers[0x20]` at `0x5100`, `detail_object_render_list render_list` at
`0x5200`, then the cached render cell (`cell_x` / `cell_y` / `cell_z` / `valid`) at `0x5208`.

| Offset | Type | Field (`detail_object_batch`, size `0x18`) |
|---|---|---|
| `0x00` | `int32_t` | `first_instance` - running sum of `instance_count` |
| `0x04` | `int32_t` | `instance_count` |
| `0x08` | `int16_t` | `cell_x` |
| `0x0a` | `int16_t` | `cell_y` |
| `0x0c` | `float` | `cell_z` - `cell_z + offset_z * (1/255)` |
| `0x10` | `uint32_t` | `unknown_10` - never written |
| `0x14` | `void *` | `z_reference` - into the tag's z-reference vectors, or `default_z_reference` |

`detail_object_layer_batches` (`0x08`): `batches` pointer, `batch_count`, `layer_index`.
`detail_object_render_list` (`0x08`): `layers` pointer, `layer_count`, `unknown_06`.
`detail_object_cell_key` (`0x08`): the three `int16_t` cell coordinates the two binary searches
compare lexicographically, plus one zeroed, never-compared `int16_t`.

### Enums

`structure_bsp_overlap` (`_none` / `_partial` / `_contained` = 0 / 1 / 2) is the shared return of
`aabb_overlap_classify` and `frustum_planes_classify_box`; `2` means "no clipping needed", which is
why `bsp3d_node_query_recursive` skips both tests once it inherits it and drops its plane count to
zero. `structure_fog_plane_mode` (`_none` / `_bounded` / `_unbounded`) is
`structure_fog_environment.plane_mode`. `structure_bsp_constants` carries the capacities
(`k_maximum_visible_clusters` `0x80`, `k_maximum_visible_surfaces` `0x4000`, and the rest).

### Shared callback typedefs

`structure_lightmap_begin_callback`, `structure_material_callback`,
`structure_lightmap_end_callback` and `structure_transparent_material_callback` are the four
function pointers `structure_leaf_faces_for_each` drives. They were duplicated verbatim in five
files and are now declared once in `types/structures.h`. The callees themselves live in the render
module; the signatures are pinned by the argument counts the original pushes, not by any callee
this module owns.

## Globals

`types/structures.h` carries the full annotated list at the bottom of the file. The ones worth
knowing before reading any file:

| Address | Type | Name |
|---|---|---|
| `0x00746f9c` | `ScenarioStructureBSP *` | `structure_bsp` - the resident tag; every walk starts here (read, not owned) |
| `0x00746f90` | `ModelCollisionGeometryBSP *` | `global_globals` - the structure's collision BSP (read, not owned; see the gap list) |
| `0x007c3344`/`48`/`4d`/`4e` | | the camera's leaf, cluster, has-sky flag and sky index |
| `0x007c3350` | `uint32_t[0x10]` | `cluster_visible_bits` - one bit per cluster, ending exactly where the visible list begins |
| `0x007c3390` | `structure_bsp_visible_cluster[0x80]` | `visible_clusters`, count at `0x007d0390` |
| `0x007d0394` | `uint32_t[0x1000]` | `surface_visible_bits` |
| `0x00850394` | `int16_t` | `visible_surface_count`, indices at `0x00850398` (`int32_t[0x4000]`) |
| `0x006e3af8` | `uint32_t *` | `flood_recursion_bits` - a pointer to the caller's `0x40`-byte stack bitset, not a bitset |
| `0x006e3afc` | `int16_t[0x200]` | `cluster_visible_index` - cluster -> visible slot |
| `0x006e3f01`/`04`/`08` | | the cluster flood stamps, which `types/objects.h` and `types/physics.h` both read; this module is the writer |
| `0x0072277c` | `detail_object_globals *` | `detail_objects` |

Four global widths were corrected in this pass against the binary's own access widths:
`visible_surface_count` and `geometry_buffer_warning` (`0x0069fa48`) and
`current_structure_bsp_index` (`0x0069e8d8`) are `int16_t`, `fog_plane_vector_valid`
(`0x006e3ae0`) is `uint8_t`, and `k_plane_side_epsilon` (`0x00672c00`) is a `double`, not a float.

## Misattributed functions

- **`0x0053f150` belongs to the sound module, not here.** It sits `0x12ce0` bytes below the rest of
  the module and calls its immediate neighbour `0x0053ec30`. What it does is rate-limit the
  *listener's* `SoundEnvironment`: the 18 dwords it copies are exactly `sizeof(SoundEnvironment)`
  (`0x48`) and the 12 values it clamps are that tag's 12 reals from `room_intensity` (`+0x08`) to
  `hf_reference` (`+0x34`), with the last clamp being `600.0` per tick - Hz, which clinches it. No
  file is written for it. It is also absent from `symbols/agent_phase4_structures.txt`. It does
  still establish three offsets this module records: the BSP's background sound palette
  (`+0x1fc`/`+0x200`) and sound environment palette (`+0x20c`), and the Fog tag's
  `background_sound.tag_id` / `sound_environment.tag_id`.
- **Generic geometry helpers that merely live in this range**, kept because the code is here but
  candidates for a later math pass: `aabb_overlap_classify` (`0x5541b0`),
  `frustum_planes_classify_box` (`0x554260`), `polygon2d_bounds_expand` (`0x554a90`), and the
  STL-shaped `lower_bound` / `upper_bound` pair (`0x552710` / `0x552780`), which are specialised to
  the `0x20`-byte detail-object cell and its three-`int16_t` key.
- Two phase-4 one-line summaries are wrong and were corrected in the files:
  `0x553380` is node-bounds decompression, not a colour unpack; `0x555270` resolves a **Fog** tag,
  not a sound environment. `0x553560` searches **mirrors** (stride `0x40` off `cluster->mirrors`),
  not faces or materials. `0x5540c0` collects **surface** indices from one leaf, not cluster
  indices.

## Known gaps

1. **`0x00746f90` vs `0x00746f98`.** This module proves `0x00746f90` is a
   `ModelCollisionGeometryBSP *`: it is the ECX argument of `bsp3d_node_find_leaf` at `0x553e4a`,
   and `0x554b9f` reads its `+0x10` as `planes.pointer` and indexes it by `plane_index * 0x10`.
   Seven files in `src/effects`, `src/objects` and `src/hs` already call that same address
   `global_globals` (as `void *`); this module keeps the name for cross-module agreement and only
   refines the type. It is a **different** global from `structure_collision_bsp` at `0x00746f98`,
   whose `+0x40 surfaces.pointer` `src/effects` and `src/items` pin. Nothing in the binary writes
   either one with a direct `mov [abs], reg` except the two teardown paths that zero both, so which
   producer fills them, and whether they ever hold the same value, is unresolved.
2. **`collision_result +0x48`.** `structure_bsp_resolve_position_to_surface` masks it with
   `0x7fffffff` and hands it to `structure_bsp_leaf_find_material_surface` as the accepted plane,
   which compares it against `collision_bsp->bsp3d_nodes[reference.node].plane`. So `+0x48` (still
   `unknown_48` in `types/projectiles.h`) is the BSP3D **plane index** of the hit, with bit 31 as
   the flip bit. Recorded here rather than edited into that header from this module.
3. **`structure_bsp_visible_cluster +0x014..+0x19f`** (`0x18c` bytes) is the per-cluster clipped
   frustum. `structure_bsp_camera_visibility_pass` fills it through
   `render_camera_compute_frustum_bounds` and `chimera__render_camera_build_frustum`; the layout
   belongs to the render/camera module.
4. **`structure_fog_environment +0x04..+0x1b`** (`0x18` bytes) is never touched here. Most likely
   the atmospheric-fog part of the record, owned by whatever also feeds the renderer.
5. **`structure_bsp_cluster_surface_run.unknown_00` / `.unknown_04`** are consumed only by the
   three `render_frustum_classify_point_side_planes` calls whose arguments Ghidra lost to registers.
6. **`surface_visible_bits` capacity.** `0x005537c0` clears only `((surfaces.count + 0x1f) >> 5)`
   dwords and the matching *local* bitset in `0x00553d80` is a `0x4000`-byte stack buffer, so
   `0x4000` is the real capacity - but the next global that can be placed is `0x00850394`, leaving
   `0x007d4394..0x00850393` as a hole no function here touches. Do not assume the bitset is that
   large.
7. **`0x0072278c runtime_decals_suppressed`** is a pointer whose target byte `0x005530d0` clears on
   every exit path; the owning block is probably the decal globals in `types/effects.h`.
8. **`0x007c3100..0x007c3170`** is the render camera block, read but not owned. What this module
   pins: `+0x08` a render window index tested against `-1`, `+0x14` the camera position, `+0x20` its
   forward vector, `+0x54` the portal-visibility tolerance, and `+0x68` the projection context
   `structure_bsp_portal_project` transforms portal vertices through (its `+0x10` is the
   `real_matrix4x3`). A byte at `+0x24` relative to the position pointer flips the projection
   winding; whether it belongs to the same record is unsettled.
9. Three out-of-module callee contracts remain opaque: `FUN_0050ddc0` (fills a four-float screen
   bounds from a camera), `0x4ce8c0` (the triangle/point test at the bottom of
   `structure_bsp_leaf_find_material_surface`, whose last two arguments this module only forwards),
   and `vector3d_projection_band_test` in `cluster_flood_fill_with_predicate`.

## Functions

`nc` is the file's own name confidence, `rc` its rewrite confidence, `U` the number of `UNSURE`
notes left in the file. Names are also in `symbols/agent_phase4_structures.txt`.

| Address | Name | Bytes | nc | rc | U |
|---|---|---|---|---|---|
| `0x551e30` | `cluster_partition_new` | 197 | 0.85 | 0.85 | 2 |
| `0x551f00` | `cluster_reference_add_within_radius` | 279 | 0.6 | 0.6 | 0 |
| `0x552020` | `cluster_reference_remove_all` | 137 | 0.6 | 0.7 | 0 |
| `0x5520b0` | `structure_leaf_portal_vertex_count_debug` | 85 | 0.35 | 0.55 | 1 |
| `0x552110` | `structure_surface_material_locate` | 243 | 0.4 | 0.55 | 1 |
| `0x552210` | `cluster_sound_distance_lookup` | 68 | 0.75 | 0.8 | 0 |
| `0x552260` | `detail_objects_globals_allocate` | 93 | 0.6 | 0.75 | 0 |
| `0x5522d0` | `detail_objects_update_render_list` | 1066 | 0.6 | 0.4 | 5 |
| `0x552710` | `detail_object_cell_lower_bound` | 107 | 0.4 | 0.7 | 0 |
| `0x552780` | `detail_object_cell_upper_bound` | 107 | 0.4 | 0.7 | 0 |
| `0x5527f0` | `structure_picked_polygon_refresh` | 244 | 0.4 | 0.55 | 1 |
| `0x5528f0` | `structure_picked_polygon_draw` | 130 | 0.4 | 0.55 | 3 |
| `0x552980` | `structure_debug_draw_surfaces_in_box` | 221 | 0.4 | 0.35 | 5 |
| `0x552a60` | `structure_debug_draw_surfaces_in_box_alt` | 221 | 0.4 | 0.35 | 5 |
| `0x552b40` | `structure_debug_draw_surfaces_simple` | 180 | 0.5 | 0.6 | 3 |
| `0x552c20` | `structure_leaf_faces_gather_masked` | 192 | 0.4 | 0.6 | 0 |
| `0x552cf0` | `structure_leaf_faces_gather_list` | 95 | 0.4 | 0.5 | 2 |
| `0x552d60` | `chimera__bsp_poly_movsx_2` | 118 | 0.55 | 0.5 | 3 |
| `0x552de0` | `structure_leaf_faces_for_each` | 629 | 0.4 | 0.35 | 2 |
| `0x5530d0` | `structure_decals_update_switch_transitions` | 672 | 0.4 | 0.35 | 5 |
| `0x553380` | `bsp3d_node_bounds_decompress` | 265 | 0.35 | 0.75 | 1 |
| `0x553490` | `render_camera_update_leaf_and_cluster` | 197 | 0.4 | 0.55 | 0 |
| `0x553560` | `structure_bsp_mirror_query` | 577 | 0.55 | 0.55 | 1 |
| `0x5537c0` | `structure_bsp_cluster_visibility_update` | 345 | 0.6 | 0.55 | 1 |
| `0x553920` | `structure_bsp_expand_visible_clusters_by_subcluster` | 322 | 0.6 | 0.5 | 1 |
| `0x553a70` | `structure_bsp_expand_visible_clusters_by_plane` | 452 | 0.6 | 0.5 | 1 |
| `0x553c40` | `structure_bsp_collect_surfaces_in_clusters` | 308 | 0.55 | 0.55 | 1 |
| `0x553d80` | `structure_bsp_query_surfaces` | 387 | 0.6 | 0.5 | 1 |
| `0x553f10` | `bsp3d_node_query_recursive` | 428 | 0.75 | 0.8 | 1 |
| `0x5540c0` | `structure_bsp_leaf_query` | 218 | 0.55 | 0.5 | 1 |
| `0x5541b0` | `aabb_overlap_classify` | 175 | 0.9 | 0.95 | 1 |
| `0x554260` | `frustum_planes_classify_box` | 434 | 0.85 | 0.85 | 0 |
| `0x554420` | `structure_bsp_collect_visible_objects` | 200 | 0.55 | 0.5 | 1 |
| `0x5544f0` | `structure_bsp_camera_visibility_pass` | 217 | 0.55 | 0.5 | 1 |
| `0x5545d0` | `camera_cluster_portal_flood_recursive` | 632 | 0.7 | 0.7 | 2 |
| `0x554850` | `structure_bsp_portal_project` | 354 | 0.7 | 0.8 | 1 |
| `0x5549c0` | `structure_bsp_portal_test_and_project` | 86 | 0.5 | 0.55 | 1 |
| `0x554a20` | `structure_bsp_points_within_band` | 99 | 0.5 | 0.5 | 1 |
| `0x554a90` | `polygon2d_bounds_expand` | 94 | 0.85 | 0.8 | 1 |
| `0x554b00` | `structure_bsp_portal_sphere_test` | 420 | 0.55 | 0.45 | 3 |
| `0x554cb0` | `structure_bsp_cluster_flood_seed` | 94 | 0.55 | 0.6 | 1 |
| `0x554d10` | `cluster_flood_fill_within_radius` | 281 | 0.6 | 0.55 | 1 |
| `0x554e30` | `cluster_flood_fill_with_predicate` | 364 | 0.5 | 0.4 | 1 |
| `0x554fa0` | `structure_bsp_leaf_find_material_surface` | 485 | 0.65 | 0.75 | 1 |
| `0x555190` | `structure_bsp_resolve_position_to_surface` | 219 | 0.6 | 0.7 | 1 |
| `0x555270` | `structure_bsp_resolve_fog_tag` | 181 | 0.6 | 0.6 | 1 |
| `0x555330` | `structure_bsp_build_fog_environment` | 387 | 0.6 | 0.55 | 2 |

## Semantic fixes made in the reconciliation pass

For the record, and because these are the shapes of mistake to look for in the files that were not
line-checked:

- `bsp3d_node_query_recursive`: its output argument was modelled as an `int32_t` byte offset rather
  than an `int32_t *`, dropping the element stride (`lea edx,[ebx+ecx*4]` rendered by Ghidra as
  `iVar3 + (short)iVar11 * 4`); its three "unknown forwarded" parameters are the query box, the
  plane count and the plane array; and its two child-visit conditions were swapped with one
  boundary inverted.
- `structure_bsp_portal_project`: a whole register parameter (EDX, the source vertex array) was
  missing, and the transform loop was calling `matrix4x3_transform_point` with one argument instead
  of its real `EAX -> out, EDX -> point, stack -> m` convention.
- `camera_cluster_portal_flood_recursive`: there are two local `polygon2d` buffers, not one; the
  subject and clip polygons of `polygon2d_clip_to_planes` were the wrong way round; and the clip
  destination was the struct base instead of the point array (a 4-byte shift on every output point).
- `structure_bsp_mirror_query`: its first two parameters were swapped, and its clip call had the
  same subject/clip and destination problems.
- `structure_bsp_resolve_position_to_surface`: `0x505880` is
  `collision_test_movement_segment`, not the invented three-argument "jitter generator"; the whole
  loop reads differently as a result.
- `structure_bsp_leaf_find_material_surface`: the triangle-vertex gather was missing entirely,
  including the fact that the vertex source and stride depend on the material's
  `rendered_vertices_type` (`compressed_vertices` at stride `0x20` for type 1,
  `uncompressed_vertices` at stride `0x38` for types 0 and `0xc`); and its "outer cell / inner
  cell" pair is really the lightmap index / material index pair.
- The three `structure_debug_draw_surfaces_*` siblings: their trailing arguments are a
  `cluster_count` / `cluster_indices` pair, not a `box` / `is_box` pair, and the `simple` variant
  has four stack parameters plus an ECX plane count rather than three stack parameters.
- `structure_bsp_points_within_band`: its register convention is `EDX -> points, SI -> point_count`,
  not `EAX -> points, EDX -> point_count`.
