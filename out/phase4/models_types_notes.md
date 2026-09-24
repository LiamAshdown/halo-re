# models module: type recovery notes

Header: `types/models.h`. Smoke test: `out/phase4/models_smoke.c`. Build it with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/models_smoke.c
```

The test passes. It checks the size of all 6 new structs and every field offset in them. It also checks about 90 offsets in the `types/tags.h` records that this module reads, re-derived from this module's arithmetic.

The smoke file includes `tags.h`, `memory.h`, `math.h` and `models.h`. `math.h` is one more than the task asked for. It is needed because `real_orientation` reuses `real_quaternion` / `real_point3d` and the smoke test uses `real_matrix4x3`. I also compiled every `types/*.h` together, with and without `models.h`. Both runs give 377 errors, all from other headers and none in `models.h`, so the new header adds no name clashes.

Evidence came from the Ghidra C (`tools/pack.py`) and from `objdump -d -M intel` of `bin/halo.exe`. Every register convention below was read from the callee prologue and at least one call site.

The range is misnamed as a whole. About two thirds of it is the animation sampler, which reads ModelAnimations tags and outputs per-node orientations. The phase 2 names that say "vertex", "vertices", "region" or "normal" are wrong (see the last section).

---

## Records reused, not redefined

| type | owner | models evidence |
|---|---|---|
| `ModelAnimationsAnimation` 0xb4 | tags.h | 0x4d4a80 and siblings (EDI/ESI/ECX = animation) read type +0x20, frame_count +0x22, frame_size +0x24, frame_info_type +0x26, node_list_checksum +0x28, node_count +0x2c, loop_frame_index +0x2e, key/second key +0x34/+0x36, next_animation +0x38, flags +0x3a (bit 0 compressed), sound +0x3c, sound_frame_index +0x3e, main_animation_index +0x42, relative_weight +0x44 (cumulative threshold in 0x4d6280), frame_info.pointer +0x54, the three node bit masks +0x5c (translation) / +0x6c (rotation) / +0x7c (scale), offset_to_compressed_data +0x88, default_data.pointer +0x98 and frame_data.pointer +0xac. All of these match tags.h. The masks are read as `mask[node >> 5]`, so only the first two dwords of each 16-byte mask area are used (64 nodes). |
| `ModelAnimations` 0x80 | tags.h | 0x4d48d0 reads sound_references.pointer +0x58 (stride 0x14, tag_id +0x0c) and animations.pointer +0x78. 0x4d6ab0 reads animations +0x74/+0x78. 0x4d6880 reads nodes +0x68/+0x6c (stride 0x40). The vehicle caller of 0x4d5c00 (0x571974) reads vehicles +0x24/+0x28. |
| `ModelAnimationsAnimationGraphNode` 0x40 | tags.h | 0x4d6880 does a breadth-first walk over next_sibling +0x20 and first_child +0x22, and uses parent +0x24 as the parent matrix index. |
| `ModelAnimationsFrameInfo*` | tags.h | 0x4d4850 steps frame_info by 2, 3 or 4 floats for frame_info_type 1, 2 or 3, sums the first float (dx), and snapshots the sum at key_frame_index. |
| `GBXModel` 0xe8 | tags.h | flags +0x00: bit 1 adds context flag 0x100 and bit 2 adds 0x200 (0x4d6fc0). node_list_checksum +0x04 (0x4d4a80, 0x4d6fc0). Cutoffs +0x08..+0x18 (0x4d6fc0). base_map_u/v_scale +0x30/+0x34 are copied to context +0xc4/+0xc8. Reflexives used: markers +0xac/+0xb0 (0x4d77c0 binary search, 0x4d7850), nodes +0xb8/+0xbc (0x4d4a80 count check, 0x4d6fc0, 0x4d7610, 0x4d7690), regions +0xc4/+0xc8, geometries +0xd4, shaders +0xe0 (0x4d72a0). |
| `ModelNode` 0x9c | tags.h | 0x4d7610 reads default_translation +0x28 and default_rotation +0x34. 0x4d6fc0 multiplies each node matrix by the real_matrix4x3 at +0x68 (tags.h splits it as scale / Matrix / translation). 0x4d7690 walks +0x20/+0x22/+0x24 the same way 0x4d6880 walks the graph nodes. |
| `ModelMarker` 0x40, `ModelMarkerInstance` 0x20 | tags.h | 0x4d77c0 binary-searches the names with `__stricmp`, so the block must be sorted. 0x4d7850 reads instance region +0x00, permutation +0x01, node +0x02, translation +0x04 and rotation +0x10. |
| `ModelRegion` 0x4c, `ModelRegionPermutation` 0x58 | tags.h | 0x4d72a0 uses region permutations.pointer +0x44 and the geometry index at permutation +0x40 + lod*2. |
| `GBXModelGeometry` 0x30, `GBXModelGeometryPart` 0x84 | tags.h | 0x4d72a0 reads parts +0x24/+0x28. From each part it reads flags +0x00 (bit 0 skips the part; bit 1 calls set_up_node_parts 0x526cf0 with part +0x6c), shader_index +0x04, prev/next filthy +0x06/+0x07, centroid_primary_node +0x08 and centroid +0x14 (transformed by that node matrix before FUN_0052b180), the triangle block +0x44 / count +0x48 and the vertex block +0x54. |
| `ModelShaderReference` 0x20, `Shader`, `ShaderModel` | tags.h | 0x4d72a0 resolves shader.tag_id +0x0c and uses permutation +0x10 when the forced permutation argument is 0. It reads Shader.shader_type +0x24 and, for model shaders, ShaderModel flags +0x28 bit 3 (alpha_blended_decal). |
| `object_marker` 0x6c | objects.h | 0x4d7850 writes node_index +0x00 (remapped through the optional int16 table). It builds +0x04 from the instance quaternion and translation with matrix4x3_from_quaternion (EDX = record + 4), and writes +0x38 = node_matrix[node] * (+0x04). **Correction for objects.h:** in mirrored mode 0x4d7850 negates +0x48, +0x4c and +0x50. That is `node_transform.left` at 0x38 + 0x10. The objects.h comment says the negated components are at 0x24/0x28/0x2c, which is wrong. That comment also calls +0x04 an identity and +0x38 a copy of the node matrix. That is only true on the objects fallback path. On the models path, +0x04 is the node-relative marker matrix and +0x38 is the world marker matrix. |
| `rasterizer_model_draw_context` | rasterizer.h | 0x4d6fc0 is the producer that rasterizer.h lacked. It builds the whole context at ebp-0xdc..ebp-0x11, which is **exactly 0xcc bytes**: +0x00 flags; +0x04 argument 9 (render_object 0x50f044 passes the object datum, the sky 0x511124 passes 0); +0x08 node matrix array (the 64-entry array at ebp-0xddc); +0x0c GBXModel node count; +0x10 render_lighting (0x1d dwords from argument 5); **+0x84 argument 3 = ColorRGB change_colors[4]\* and +0x88 argument 4 = float function_values[4]\*** (rasterizer.h `unknown_84[2]`); +0x8c ten dwords from argument 8 (render_model_effect / rasterizer_geometry_group_parameters); +0xb4 argument 6 (centre point); **+0xc0 argument 7 = bounding radius** (`unknown_c0`); **+0xc4/+0xc8 = GBXModel base_map_u_scale / base_map_v_scale** (`unknown_c4` / `unknown_c8`). The context is passed to 0x4d72a0 as `&context.node_matrices` (rasterizer_node_matrices). |
| `render_model_effect` 0x28 | render.h | This is argument 8 of 0x4d6fc0. When it is NULL the zero block at 0x006b7f18 is used. |
| `transparent_geometry_group_link` 0x0c | rasterizer.h | This is the first 0x0c bytes of `model_part_group_link`. The +0x0a word that rasterizer.h marks "not written" is written by 0x4d72a0 itself (linked_part_index). |

---

## Structs and enums defined in models.h

### real_orientation (0x20)
* **Layout source:** 0x4d7610 writes all eight dwords one at a time: rotation from ModelNode +0x34, translation from +0x28, and 1.0f at +0x1c.
* **Readers:** 0x4d6880 and 0x4d7690 pass the element to matrix4x3_from_quaternion 0x4cbad0 (ECX), then copy +0x1c to matrix.scale and +0x10..+0x18 to matrix.position.
* **Other writers:** 0x4d4a80 writes each field from the stream. 0x4d4f90 applies quaternion_multiply at +0x00, addition at +0x10 and multiplication at +0x1c. 0x4d51a0 / 0x4d57d0 do the weighted versions. 0x4d69e0 blends with a lerp.
* **Array size:** the stack arrays in 0x4d49b0 (0x800 bytes) and 0x4d4a00 (2 x 0x800) hold 64 elements, which gives `k_maximum_nodes_per_model`.
* **Unresolved:** none. The name follows Blam convention (hint only).

### animation_state (0x04)
* This is the ESI block of 0x4d48d0. +0x00 is the animation index (stride 0xb4). 0x4d48d0 increments +0x02, compares it and rewrites it.
* It is embedded in object +0x0d0 / +0x0d2 (objects.h `animation_index` / `animation_frame`). The first person weapon caller 0x49324e passes `ebp+0x16`.
* **Unresolved:** none.

### animation_state_advance_result (enum, return of 0x4d48d0)
* 0 is an ordinary frame. 1 means a key frame was hit. 2 is the last frame of a non-looping animation. 3 means the next animation was picked through 0x4d6280 (DX = main_animation_index +0x42) and the frame reset to 0. 4 means the animation looped to `min(loop_frame_index, frame_count-1)`. The caller 0x493256 tests 1 and 2.
* When EBX is non-null, 0x4d48d0 also writes the sound tag id there. It does this only if sound +0x3c != -1 and sound_frame_index +0x3e equals the frame *before* the increment. Otherwise it writes -1.

### animation_random_stream (enum)
* This is the stack argument of 0x4d48d0 and 0x4d6280. Value 1 steps 0x00719cd0 and any other value steps 0x00719cd4. It uses the LCG 0x19660d/0x3c6ef35f and scales the high 16 bits by 1/65535.

### animation_quaternion48 (0x06)
* **Decoder:** 0x4d6380 (ECX = source, ESI = out). The bit shuffle is written out in the header.
* **Stride 6:** `lea x,[i+i*2]` then `[base+x*2]` in 0x4d6b60 for the defaults (indexed by node) and for the keyframes.
* **Unresolved:** none. The phase 2 name `model_vertex_unpack_compressed_normal` is wrong, because this is animation rotation data.

### animation_compressed_header (0x2c + inline headers)
* **Rotation** (0x4d6b60): `[h+0x00]` is the keyframe times offset, `[h+0x04]` the defaults offset, `[h+0x08]` the keyframes offset. The keyframe headers are inline at `h+0x2c+i*4`.
* **Translation** (0x4d6cf0): +0x0c headers, +0x10 times, +0x14 defaults (12 bytes each, indexed by node), +0x18 keyframes.
* **Scale** (0x4d6e80): +0x1c headers, +0x20 times, +0x24 defaults, +0x28 keyframes (4 bytes each).
* **Keyframe header format:** count is `& 0xfff` and first index is `>> 12`. A count of 0 means the default value is used.
* **Base address:** `frame_data.pointer + offset_to_compressed_data`, used by 0x4d6b60 / 0x4d6cf0 / 0x4d6e80 (ECX = animation).
* **Quirks, recorded as observed:**
  * 0x4d6e80 indexes both the scale header array and the scale default array with the same DX value (the running scale index from its callers). Rotation and translation index their defaults by node instead.
  * In 0x4d4a80's compressed path, a node whose scale bit is clear gets 1.0 and is not read from the defaults.
* **Unresolved:** whether the scale-default indexing is a bug or means the scale defaults are stored per animated node.

### animation_aiming_screen (0x18)
* **Consumer:** 0x4d5c00. When yaw is positive it divides by +0x04, otherwise by +0x00. It clamps the column to [-(+0x08), (+0x0a)-1]. Pitch works the same way with +0x10/+0x0c and +0x14/+0x16. The grid has `(+0x08 + +0x0a + 1) * (+0x14 + +0x16 + 1)` frames and must fit in frame_count. The animation must be an overlay (type 1).
* **Producers:** the tag blocks ModelAnimationsAnimationGraphUnitSeat +0x20, ModelAnimationsAnimationGraphWeapon +0x60 and ModelAnimationsAnimationGraphVehicleAnimations +0x00. The smoke test checks all three. Callers: 0x563f46 / 0x56408c (unit code, which also scales unit +0x2b8..+0x2d4 from the same fields) and 0x571974 (vehicle, pitch 0).
* **Unresolved:** none.

### model_level_of_detail (enum)
* 0x4d6fc0 starts at lod 4 and walks down until `cutoff[lod] <= pixels`, reading `GBXModel +0x08 + lod*4`. 0x4d72a0 uses `permutation +0x40 + lod*2`. The console global 0x006893e8 overrides the result (-1 means off) and is clamped to 0..4.
* **Naming conflict:** tags.h (invader names) calls +0x08 `super_high_detail_cutoff` and +0x40 `super_low`, but the engine pairs them under the same index. The enum follows the geometry names, and the code meaning is that +0x08 is the minimum pixel size at which anything is drawn. `types/tags.h` was left untouched.

### model_render_flags (enum, argument 11 of 0x4d6fc0 / argument 6 of 0x4d72a0)
* **Bit 0:** context |= 0x1f, and no part group links are recorded.
* **Bit 1:** immediate mode. It skips the cutoff test, calls chimera__rasterizer_set_model_skinning directly, stores the context pointer at 0x0071d260 and sets 0x0071d265. 0x4d72a0 then runs only pass 0 and draws through FUN_00531350. The caller 0x511124 (sky) sets it.
* **Bit 2:** context |= 0x40. 0x50f044 sets it from object_render_data +0x09.
* **Bit 3:** context |= 0x80. The first person caller 0x49266a passes 8.
* **Unresolved:** the meaning of bit 0 and of the 0x1f it sets.

### model_render_pass (enum)
* Shader types 3..11 are the only ones drawn.
* **Pass 0:** environment and model shaders. It uses FUN_0052b050, or FUN_00531350 when immediate.
* **Pass 1:** model shaders with ShaderModel flag bit 3, drawn by FUN_0052b050.
* **Pass 2:** types 5..11 (transparent) through FUN_0052b180. The shader type test at 0x4d73dc also admits type 1 (effect), but the earlier 2 < type < 12 filter means type 1 never reaches it.

### model_part_group_link (0x10)
* This is the local array of 0x4d72a0 at esp+0x38, 32 records.
* **Filled by FUN_0052b180 (EAX = record):** +0x00, +0x04 and +0x08 (see rasterizer.h).
* **Written by 0x4d72a0:** +0x0a = the part's next_filthy_part_index (sign-extended byte +0x07) and +0x0c = the part index. A record is kept only when group_index != -1, fewer than 32 records are held, flag bit 0 is clear, and one of the part's filthy indices is positive.
* **Second loop:** it matches +0x0a against the other records' +0x0c and links the groups: `*this.next_group_index = other.group_index` and `*other.previous_group_index = this.group_index`.
* **Unresolved:** +0x0e is never touched.

### model_constants
* 64 nodes: see real_orientation. The same limit shows up in the 0xd00-byte matrix array in 0x4d6fc0 and the int16[64] walk queue in 0x4d6880 / 0x4d7690.
* 5 levels of detail, 32 links and 3 passes, as above.
* Keyframe mask and shift: see animation_compressed_header.
* `0x0769c097` is the node_list_checksum that 0x4d6fc0 compares against GBXModel +0x04. When it matches and global_scenario +0x3e bit 0 is set, the first person flag 0x007c0478 is set for the duration of the call. The meaning of scenario +0x3e bit 0 is unresolved.

---

## Globals

| address | declared as | evidence |
|---|---|---|
| 0x006894b4 | `uint8_t animation_compressed_data_enabled` (owned) | The value is 1. Nothing in the image writes it or holds a pointer to it. It is read only by the eight samplers 0x4d4810..0x4d5c00. |
| 0x006b7f08 | `float model_render_default_function_values[4]` (owned) | 0x4d6fc0 argument 4 default. It is 0x10 bytes and ends exactly at 0x006b7f18. |
| 0x006b7f18 | `render_model_effect model_render_default_effect` (owned) | Argument 8 default. It is 0x28 bytes and ends exactly at 0x006b7f40. |
| 0x006b7f40 | `uint8_t model_render_default_region_permutations[8]` (owned) | Argument 2 default. The length 8 comes from object +0x180. **Unresolved:** 0x20 bytes lie before the next global. |
| 0x006b7f60 | `ColorRGB model_render_default_change_colors[4]` (owned) | Argument 3 default. The length 4 comes from object +0x1b8. The end is not bounded by another reference. |
| 0x007c0478 | `uint8_t model_render_first_person` (written here) | 0x4d6fc0 is the only writer. FUN_0052b180 and 0x52b340 read it into transparent_geometry_group.first_person. It sits in rasterizer .bss, so ownership could reasonably move to rasterizer.h. |

These are referenced here but owned elsewhere: 0x0087bc14 (cache.h), 0x00719cd0 / 0x00719cd4 (math.h), 0x00696664 (the matrix4x3_multiply pointer), 0x007c3178 (frustum world_to_view, copied into every node matrix when 0x4d6fc0 gets a NULL matrix array), 0x006893e8 (console LOD override, in the debug toggle range rasterizer.h claims), 0x00746f8c, 0x007c1220, 0x0069c689, 0x006893f2, 0x0071d260 and 0x0071d265. The .rdata constants used are 0x00672bd0 = 1/32767, 0x00672b84 = 1/65535, 0x00672ff4 = 0.98, 0x00672ac0 = 0.0, 0x00672ac4 = 1.0 and 0x00672af8 = 1.0 (double, the fmod divisor).

---

## Register conventions (confirmed in objdump)

| addr | registers | stack |
|---|---|---|
| 0x4d4810 | ECX animation | int16 frame; returns the frame data pointer |
| 0x4d4850 | ECX animation | float *dx_to_key_frame, float *dx_total |
| 0x4d48d0 | EAX graph tag index, ESI animation_state*, EBX int32 *sound_tag_id (may be NULL) | random stream |
| 0x4d49b0 | EAX real_matrix4x3 *out, ECX frame, EDI animation | GBXModel* |
| 0x4d4a00 | ECX frame, EDX animation, EBX real_vector3d *out | GBXModel* |
| 0x4d4a80 | EDI animation, EAX GBXModel* (NULL skips the checksum and node count test) | int16 frame, real_orientation* |
| 0x4d4dd0 / 0x4d4f90 | ESI animation | int16 frame, real_orientation* |
| 0x4d51a0 | EDI animation | int16 frame, float weight, real_orientation* |
| 0x4d53f0 | EDI animation | float frame, real_orientation* |
| 0x4d57d0 | EDI animation | float frame, float weight, real_orientation* |
| 0x4d5c00 | EDI animation | animation_aiming_screen*, float yaw, float pitch, real_orientation* |
| 0x4d6280 | EAX graph tag index, DX first animation | random stream; returns int16 |
| 0x4d6330 | ECX int16[4] source, EAX real_quaternion *out | |
| 0x4d6380 | ECX animation_quaternion48*, ESI real_quaternion *out | |
| 0x4d6440 | | four real_matrix4x3* (the first is copied to the fourth at exit) |
| 0x4d6880 | EAX graph tag index, ECX real_point3d *root_position | real_matrix4x3 *out, real_orientation* |
| 0x4d69e0 | EAX real_orientation *in_out, CX node count | real_orientation *other, int16 step, int16 steps |
| 0x4d6ab0 | EAX graph tag index, EBX name | returns int16 animation index or -1 |
| 0x4d6b10 | EAX count, BX frame | uint16 *times |
| 0x4d6b60 | ECX animation | float frame, int16 rotation index, int16 node, real_quaternion *out |
| 0x4d6cf0 | ECX animation | float frame, int16 translation index, int16 node, real_point3d *out |
| 0x4d6e80 | ECX animation, DX scale index | float frame, float *out |
| 0x4d6fc0 | EAX model tag index, ECX real_matrix4x3 *node_matrices (NULL allowed) | pixels, region permutations, change colours, function values, render_lighting*, centre, radius, render_model_effect*, arg9, forced shader permutation, model_render_flags |
| 0x4d72a0 | | GBXModel*, region permutations, rasterizer_node_matrices*, lod, forced permutation, flags |
| 0x4d7610 | ESI GBXModel* | real_orientation* |
| 0x4d7690 | EAX real_point3d *root_position | GBXModel*, real_matrix4x3 *out, real_orientation* |
| 0x4d77c0 | EAX model tag index | char *name; returns int16 or -1 |
| 0x4d7850 | ECX model tag index, EAX char *name | uint8 *region_permutations (NULL = all), int16 *node_remap (NULL = identity), real_matrix4x3 *node_matrices, bool mirrored, object_marker *out, int16 maximum; returns the count |

---

## Misnamed or misattributed functions

No function in the range is library code, and none belongs to another module's data. Two things may need a module decision:

* 0x4d6fc0 and 0x4d72a0 are the model render path. They only write rasterizer state and could be moved to the render module. Their types are kept here because they are the only users of model_render_flags, model_render_pass and model_part_group_link.
* 0x4d6440 (two-bone IK) only does matrix4x3 arithmetic and could be moved to the math module.

Names that are wrong or misleading (current name -> suggested):

| addr | current / phase 2 | suggested | why |
|---|---|---|---|
| 0x4d4810 | FUN / model_get_frame_vertex_pointer | animation_get_frame_data | It returns `frame_data + frame*frame_size`, or the compressed base. |
| 0x4d4850 | FUN / model_vertex_weight_sum | animation_get_frame_info_distance | It sums frame_info dx, not vertex weights. |
| 0x4d48d0 | FUN / model_region_permutation_enumerate_next | animation_state_advance | It steps an animation_state and has nothing to do with regions. |
| 0x4d49b0 | FUN | animation_get_root_node_matrix | |
| 0x4d4a00 | model_animation_get_frame_delta | animation_get_root_translation_delta | This one is close to correct. |
| 0x4d4a80 | FUN | animation_get_frame_orientations | |
| 0x4d4dd0 | FUN | animation_replace_frame_orientations | It requires type 2 (replacement). |
| 0x4d4f90 | FUN | animation_overlay_frame_orientations | It requires type 1 (overlay). |
| 0x4d51a0 | FUN | animation_overlay_frame_orientations_weighted | |
| 0x4d53f0 | model_vertices_get_interpolated_frame | animation_overlay_interpolated_frame_orientations | It works on orientations, not vertices. |
| 0x4d57d0 | FUN | animation_overlay_interpolated_frame_orientations_weighted | |
| 0x4d5c00 | model_vertices_bilinear_interpolate_2d_frame | animation_aiming_screen_blend | |
| 0x4d6280 | FUN | animation_choose_random_permutation | Blam name, hint only. |
| 0x4d6330 | FUN | animation_quaternion16_decode | |
| 0x4d6380 | model_vertex_unpack_compressed_normal | animation_quaternion48_decode | This is not a vertex normal. |
| 0x4d6880 | model_nodes_calculate_world_transforms | animation_graph_nodes_build_matrices | It walks the **animation graph** nodes, not the model nodes. |
| 0x4d6ab0 | model_get_region_index_by_name | animation_graph_find_animation_by_name | **Wrong object:** it searches ModelAnimations.animations (+0x74/+0x78, stride 0xb4), not model regions. |
| 0x4d6b10 | FUN | animation_keyframe_time_search | |
| 0x4d6b60 / 0x4d6cf0 / 0x4d6e80 | model_node_get_interpolated_* | animation_node_get_rotation / _translation / _scale | These are compressed-codec curves. |
| 0x4d6fc0 | FUN | model_render | |
| 0x4d72a0 | FUN | model_render_parts | |
| 0x4d7690 | FUN (summary: "root rotation from an external override") | model_nodes_build_matrices | The summary is wrong. This is 0x4d6880 over GBXModel nodes (stride 0x9c) with the root position in EAX, and there is no override. |

Every function in the range was covered by this pass. None was left out.
