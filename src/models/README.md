# `models` — animation sampling, node transforms, markers and model rendering

Retail Halo PC `halo.exe` 1.0.10, `0x4d4810 .. 0x4d797d` (29 functions, about 12.3 KB of code),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/models.h`, with the original decompile preserved verbatim at the
bottom of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py models` gives **29 ok, 0 failed**.

## What the module contains

Despite the name, most of the range is the **animation sampler**. It reads a `ModelAnimations`
tag (`types/tags.h`) and produces one `real_orientation` (rotation, translation, scale) per
node. The rest turns orientations into node matrices, looks up markers and animations by name,
and draws a model.

| Family | Functions | What it does |
|---|---|---|
| frame sampling | `0x4d4810`, `0x4d4a80`, `0x4d4dd0`, `0x4d4f90`, `0x4d51a0`, `0x4d53f0`, `0x4d57d0`, `0x4d5c00` | base (type 0), replacement (type 2) and overlay (type 1) samplers, plain, weighted and fractional frame; the 2D aiming screen blend |
| root motion | `0x4d4850`, `0x4d49b0`, `0x4d4a00` | frame info distance sum, root node matrix, root translation delta between two frames |
| compressed codec | `0x4d6b10`, `0x4d6b60`, `0x4d6cf0`, `0x4d6e80`, `0x4d6330`, `0x4d6380` | keyframe time search; per node rotation, translation and scale curves; int16x4 and 48 bit quaternion decoders |
| animation graph | `0x4d48d0`, `0x4d6280`, `0x4d6ab0` | advance an `animation_state` by one frame, weighted random pick along a `next_animation` chain, animation index by name |
| node transforms | `0x4d7610`, `0x4d69e0`, `0x4d6880`, `0x4d7690`, `0x4d6440` | bind pose, blend two orientation arrays, node matrices over graph nodes or model nodes, two bone IK |
| markers | `0x4d77c0`, `0x4d7850` | marker group index by name (binary search), fill `object_marker` records |
| rendering | `0x4d6fc0`, `0x4d72a0` | `render_model` builds node matrices, picks an LOD and fills a `rasterizer_model_draw_context`; `model_render_parts` walks regions, permutations and parts in three passes |

How the samplers fit together:

```
render / units / objects callers
  animation_get_frame_orientations (base pose, type 0)     -> real_orientation[64]
  animation_replace_frame_orientations (type 2)             overwrite masked nodes
  animation_overlay_*_frame_orientations[_weighted] (type 1) multiply / add / scale onto them
  animation_aiming_screen_blend (type 1, 2D grid of frames)
     each per node value comes from one of three streams, chosen per node by three bit masks:
       compressed codec   -> animation_node_get_{rotation,translation,scale}
                               -> animation_keyframe_time_search, animation_quaternion48_decode
       uncompressed frame -> frame_data + frame_size * frame (int16x4 rotation, point3d, float)
       default data       -> default_data (same encoding, nodes whose bit is clear)
  animation_graph_nodes_build_matrices / model_nodes_build_matrices -> real_matrix4x3[64]
  render_model -> model_render_parts -> rasterizer (0x52b050, 0x52b180, 0x531350)
```

## Struct layouts

All of these live in `types/models.h`. The tag records (`ModelAnimations`,
`ModelAnimationsAnimation`, `GBXModel`, `ModelNode`, `ModelMarker`, `ModelRegionPermutation`,
`GBXModelGeometryPart` and so on) are in `types/tags.h` and are not repeated here. Offsets are
byte offsets; `#pragma pack(push,1)` is in force.

### `real_orientation`, size `0x20`

The element of every node array in the module (64 of them in the `0x800` byte stack arrays).

| Off | Type | Field |
|---|---|---|
| `0x00` | `real_quaternion` | `rotation` (i, j, k, w) |
| `0x10` | `real_point3d` | `translation` |
| `0x1c` | `float` | `scale` |

### `animation_state`, size `0x04`

The ESI block of `animation_state_advance`, embedded in objects at `+0x0d0` and in the first
person weapon record.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `animation_index` |
| `0x02` | `int16_t` | `frame_index` |

### `animation_quaternion48`, size `0x06`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint16_t[3]` | `packed` (four 12 bit fields, each shifted to 16 bits and scaled by 1/32767) |

### `animation_compressed_header`, size `0x2c`

At `frame_data.pointer + offset_to_compressed_data`. Every field is a dword offset from the
start of the header. The rotation keyframe headers follow at `+0x2c`.

| Off | Field | Points at |
|---|---|---|
| `0x00` | `rotation_keyframe_times` | `uint16[]` |
| `0x04` | `rotation_defaults` | `animation_quaternion48[node count]` |
| `0x08` | `rotation_keyframes` | `animation_quaternion48[]` |
| `0x0c` | `translation_keyframe_headers` | `uint32[]` |
| `0x10` | `translation_keyframe_times` | `uint16[]` |
| `0x14` | `translation_defaults` | `real_point3d[node count]` |
| `0x18` | `translation_keyframes` | `real_point3d[]` |
| `0x1c` | `scale_keyframe_headers` | `uint32[]` |
| `0x20` | `scale_keyframe_times` | `uint16[]` |
| `0x24` | `scale_defaults` | `float[]` (indexed by the scale index, not the node) |
| `0x28` | `scale_keyframes` | `float[]` |

A keyframe header is `count (bits 0..11) | first_index << 12`.

### `animation_aiming_screen`, size `0x18`

The first stack argument of `animation_aiming_screen_blend`. It matches
`ModelAnimationsAnimationGraphUnitSeat +0x20`, `...Weapon +0x60` and `...VehicleAnimations +0x00`.

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `right_yaw_per_frame` (divisor for yaw <= 0) |
| `0x04` | `float` | `left_yaw_per_frame` (divisor for yaw > 0) |
| `0x08` | `uint16_t` | `right_frame_count` |
| `0x0a` | `uint16_t` | `left_frame_count` |
| `0x0c` | `float` | `down_pitch_per_frame` |
| `0x10` | `float` | `up_pitch_per_frame` |
| `0x14` | `uint16_t` | `down_pitch_frame_count` |
| `0x16` | `uint16_t` | `up_pitch_frame_count` |

### `model_part_group_link`, size `0x10`

The local array of 32 in `model_render_parts`. The first `0x0c` bytes are the
`transparent_geometry_group_link` that `0x52b180` fills through EAX.

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `previous_group_index` (an `int16_t *` into the new group) |
| `0x04` | `uint32_t` | `next_group_index` (an `int16_t *`) |
| `0x08` | `int16_t` | `group_index`, -1 when nothing was built |
| `0x0a` | `int16_t` | `linked_part_index` (the part's `next_filthy_part_index`, sign extended) |
| `0x0c` | `int16_t` | `part_index` |
| `0x0e` | `int16_t` | `unknown_0e`, never touched |

### `rasterizer_model_draw_context`, as `render_model` fills it

This struct is defined in `types/rasterizer.h`. `render_model` is its producer.

| Off | Source |
|---|---|
| `0x00` | flags: `0x200` ignore_skinning, `0x1f` render flag bit 0, `0x40` outside fog plane, `0x80` frustum z, `0x100` parts_have_local_nodes |
| `0x04` | `object_index` |
| `0x08` / `0x0c` | local node matrix array / node count (`rasterizer_node_matrices`, handed to `model_render_parts`) |
| `0x10` | `render_lighting` copy (0x74 bytes) |
| `0x84` / `0x88` | `change_colors` / `function_out_values` pointers (`unknown_84[2]` in rasterizer.h) |
| `0x8c` | `render_model_effect` copy (0x28 bytes) |
| `0xb4` | `bounding_center` copy; NULL means `node_matrices + 0x28` (the root node position) |
| `0xc0` | `bounding_radius` |
| `0xc4` / `0xc8` | `GBXModel` `base_map_u_scale` / `base_map_v_scale` |

## Register conventions

The build used LTCG, so each function has its own convention. All of these were read from the
call sites and prologues in objdump.

| Function | Registers | Stack |
|---|---|---|
| `animation_get_frame_data` | ECX animation | frame |
| `animation_get_frame_info_distance` | ECX animation | dx_to_key_frame, dx_total |
| `animation_state_advance` | EAX graph tag, ESI state, EBX sound_tag_id out | random stream |
| `animation_get_root_node_matrix` | EAX out, ECX frame, EDI animation | model |
| `model_animation_get_frame_delta` | ECX frame, EDX animation, EBX out | model |
| `animation_get_frame_orientations` | EAX model, EDI animation | frame, out |
| `animation_replace_frame_orientations`, `animation_overlay_frame_orientations` | ESI animation | frame, out |
| `animation_overlay_frame_orientations_weighted` | EDI animation | frame, weight, out |
| `animation_overlay_interpolated_frame_orientations` | EDI animation | frame (float), out |
| `animation_overlay_interpolated_frame_orientations_weighted` | EDI animation | frame, weight, out |
| `animation_aiming_screen_blend` | EDI animation | screen, yaw, pitch, out |
| `animation_choose_random_permutation` | EAX graph tag, DX first animation; returns AX | stream |
| `animation_quaternion16_decode` | ECX source, EAX out | |
| `animation_quaternion48_decode` | ECX source, ESI out | |
| `model_ik_solve_two_bone` | | target, middle (the root joint), end (the middle joint), out_end (the effector) |
| `animation_graph_nodes_build_matrices` | EAX graph tag, ECX root position | out, orientations, forward, up |
| `model_nodes_blend_transforms` | EAX in_out, CX node count | other, step, steps |
| `animation_graph_find_animation_by_name` | EAX graph tag, EBX name; returns AX | |
| `animation_keyframe_time_search` | EAX count, BX frame; returns AX | times |
| `animation_node_get_rotation`, `_translation` | ECX animation | frame, index, node, out |
| `animation_node_get_scale` | ECX animation, DX scale index | frame, out |
| `render_model` | EAX model tag, ECX node matrices | 11 arguments, see the file |
| `model_render_parts` | | model, region_permutations, node_matrices, lod, forced permutation, flags |
| `model_nodes_get_default_transforms` | ESI model | out |
| `model_nodes_build_matrices` | EAX root position, ECX forward | model, out, orientations, up |
| `model_marker_group_index_from_name` | EAX model tag; returns AX | name |
| `model_markers_get_by_name` | ECX model tag, EAX name; returns AX | region_permutations, node_remap, node_matrices, mirrored, out, maximum |

## Known gaps and quirks (all reproduced, not fixed)

1. **No misattributed functions.** Every function in `0x4d4810..0x4d7850` is engine code of this
   module; no library code was found.
2. **Tag fields that the code reads as signed.** `types/tags.h` declares `frame_count`,
   `frame_size`, `node_count`, `key_frame_index`, `sound`, `parent_node_index`, `shader_index`,
   `centroid_primary_node` and the others as `uint16_t`. The binary loads all of them with
   `movsx`. The rewrites cast to `int16_t` at each use and say so in a comment. The keyframe
   *times* are the exception: the three curve evaluators compare them unsigned (`movzx`), while
   `0x4d6b10` compares them signed.
3. **A frame past the last keyframe spins.** The curve evaluators special case frames below the
   first keyframe and exactly equal to the last one. Any later frame goes to `0x4d6b10`, which
   loops forever for it. Callers are expected never to pass one.
4. **Compressed rotation ignores the fraction.** In `0x4d53f0` and `0x4d57d0`, the compressed
   rotation curve is sampled at the floored base frame. Translation and scale are sampled at the
   fractional frame. `0x4d57d0` floors `frame` directly, while `0x4d53f0` floors `fabs(frame)`.
5. **Two compressed data selectors can disagree.** In `0x4d4a80`, the frame base pointer requires
   `animation_compressed_data_enabled` (`0x6894b4`), but the codec selector does not when
   `offset_to_compressed_data == 0`. Nothing ever clears `0x6894b4`, so this never happens in
   practice.
6. **Scale defaults are indexed by the scale index.** `0x4d6e80` indexes the scale default table
   with the running scale index, not with the node index. It is unclear whether this is a bug or
   the data layout.
7. **`animation_aiming_screen_blend` ignores scale.** It walks the uncompressed stream with only
   the rotation and translation masks. A node with an animated scale would desynchronise the four
   frame cursors.
8. **`render_model` with `node_matrices == NULL` and `bounding_center == NULL`** reads the default
   center from address `0x28`. This happens exactly as in the original.
9. **Remaining `UNSURE` markers.** There are two, both in `render_model.c` and both about names of
   globals owned by other modules: `0x007c3178` `render_camera_world_to_view` and `0x006893f2`.
10. **Out of date declarations in other modules.** These were not edited, because they are
    outside this module:
    - `0x4d53f0` is still declared under its old name `model_vertices_get_interpolated_frame` by
      `src/render/render_sky.c`, `src/units/unit_throw_grenade_release.c`,
      `src/units/unit_update_aiming_overlay_angles.c` and the two
      `src/objects/object_recalculate_bounding_radius*.c` files.
    - `0x4d5c00` is declared as `model_vertices_bilinear_interpolate_2d_frame` in
      `unit_update_aiming_overlay_angles.c`.
    - `0x4d6880` is declared as `model_nodes_calculate_world_transforms` in
      `first_person_weapon_update_animation_controls.c`.
    - `0x4d6ab0` is declared as `model_get_region_index_by_name` in three `src/units` and
      `src/objects` files.
    - `0x4d6280` is declared as `FUN_004d6280` with guessed one argument prototypes in about 15
      files in `src/units` and `src/ai`.
    - Several prototypes differ from the definitions here: `model_markers_get_by_name`,
      `model_ik_solve_two_bone`, `model_nodes_get_default_transforms`,
      `model_nodes_blend_transforms` and `model_animation_get_frame_delta`.
    - `src/rasterizer` names `0x007c0478` `rasterizer_rendering_first_person`; this module owns
      it as `model_render_first_person`.

## Functions and rewrite confidence

`name` is confidence in the symbol name. `rw` is confidence in the C rewrite after the review
pass, in which every function was checked against objdump. `U` counts `UNSURE` markers.

| Address | Size | Function | name | rw | U |
|---|---|---|---|---|---|
| `0x4d4810` | 52 | `animation_get_frame_data` | 0.5 | 0.85 | 0 |
| `0x4d4850` | 123 | `animation_get_frame_info_distance` | 0.5 | 0.85 | 0 |
| `0x4d48d0` | 218 | `animation_state_advance` | 0.5 | 0.85 | 0 |
| `0x4d49b0` | 72 | `animation_get_root_node_matrix` | 0.5 | 0.85 | 0 |
| `0x4d4a00` | 118 | `model_animation_get_frame_delta` | 0.55 | 0.85 | 0 |
| `0x4d4a80` | 847 | `animation_get_frame_orientations` | 0.5 | 0.85 | 0 |
| `0x4d4dd0` | 440 | `animation_replace_frame_orientations` | 0.5 | 0.85 | 0 |
| `0x4d4f90` | 520 | `animation_overlay_frame_orientations` | 0.5 | 0.8 | 0 |
| `0x4d51a0` | 583 | `animation_overlay_frame_orientations_weighted` | 0.5 | 0.85 | 0 |
| `0x4d53f0` | 967 | `animation_overlay_interpolated_frame_orientations` | 0.5 | 0.8 | 0 |
| `0x4d57d0` | 1052 | `animation_overlay_interpolated_frame_orientations_weighted` | 0.5 | 0.8 | 0 |
| `0x4d5c00` | 1660 | `animation_aiming_screen_blend` | 0.5 | 0.8 | 0 |
| `0x4d6280` | 164 | `animation_choose_random_permutation` | 0.45 | 0.85 | 0 |
| `0x4d6330` | 77 | `animation_quaternion16_decode` | 0.5 | 0.9 | 0 |
| `0x4d6380` | 182 | `animation_quaternion48_decode` | 0.5 | 0.85 | 0 |
| `0x4d6440` | 1079 | `model_ik_solve_two_bone` | 0.65 | 0.8 | 0 |
| `0x4d6880` | 327 | `animation_graph_nodes_build_matrices` | 0.5 | 0.8 | 0 |
| `0x4d69e0` | 203 | `model_nodes_blend_transforms` | 0.5 | 0.8 | 0 |
| `0x4d6ab0` | 81 | `animation_graph_find_animation_by_name` | 0.6 | 0.85 | 0 |
| `0x4d6b10` | 66 | `animation_keyframe_time_search` | 0.5 | 0.85 | 0 |
| `0x4d6b60` | 386 | `animation_node_get_rotation` | 0.55 | 0.85 | 0 |
| `0x4d6cf0` | 390 | `animation_node_get_translation` | 0.55 | 0.85 | 0 |
| `0x4d6e80` | 320 | `animation_node_get_scale` | 0.55 | 0.85 | 0 |
| `0x4d6fc0` | 731 | `render_model` | 0.55 | 0.8 | 2 |
| `0x4d72a0` | 849 | `model_render_parts` | 0.55 | 0.75 | 0 |
| `0x4d7610` | 114 | `model_nodes_get_default_transforms` | 0.55 | 0.85 | 0 |
| `0x4d7690` | 280 | `model_nodes_build_matrices` | 0.5 | 0.8 | 0 |
| `0x4d77c0` | 144 | `model_marker_group_index_from_name` | 0.55 | 0.85 | 0 |
| `0x4d7850` | 302 | `model_markers_get_by_name` | 0.5 | 0.8 | 0 |

The names are recorded in `symbols/agent_phase4_models.txt`, together with seven module owned
globals. Run `tools/merge_symbols.py` to fold them into `symbols/functions.txt`.
