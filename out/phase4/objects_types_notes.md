# objects module type recovery notes

Header: `types/objects.h`. Syntax gate: `out/phase4/objects_smoke.c`, checked with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/objects_smoke.c` (clean).
All 26 struct sizes were also compiled and compared against the documented 32-bit sizes
(host size minus 4 per pointer); every one matches.

The smoke file includes `math.h` in addition to `tags.h` and `memory.h`, because `objects.h`
uses `real_point3d` / `real_vector3d` / `real_matrix4x3` from `types/math.h` rather than
redefining them. `hs.h` sets the precedent of naming math.h as a dependency.

## Tag-side layouts: verified, not redefined

Every tag structure this module reads already exists in `types/tags.h` and each one was
checked against the arithmetic before deciding not to redefine it:

| tags.h struct | proved by | offsets checked |
|---|---|---|
| `Object` (0x17c) | `object_new_with_datum_role_control` 0x4f54b0, `object_update_change_colors` 0x4f9110, `object_create_attachments` 0x4f9750, `widget_new` 0x4ffa80, `object_update_functions` 0x4f92f0 | object_type 0x00, flags 0x02, scales_change_colors 0x24, model TagID 0x34, animation_graph TagID 0x44, collision_model TagID 0x7c, creation_effect TagID 0xac, forced_shader_permutation_index 0x13e, attachments 0x140/0x144, widgets 0x14c/0x150, functions 0x158/0x15c, change_colors 0x164/0x168 |
| `ObjectAttachment` (0x48) | `object_create_attachments` stride 0x48, group tag at +0, marker/scale shorts at +0x30, +0x34 | |
| `ObjectWidget` (0x20) | `widget_new` stride 0x20, group tag at +0, reference TagID at +0xc | |
| `ObjectFunction` (0x168) | `object_update_functions` stride 0x168 | flags 0x00, period 0x04, scale_period_by 0x08, wobble fields 0x10/0x14, square_wave_threshold 0x18, step_count 0x1c, bounds 0x28/0x2c, turn_off_with 0x36, scale_by 0x38, inverse_* 0x13c/0x140/0x144 |
| `ObjectChangeColors` (0x2c) | `object_update_change_colors` stride 0x2c, darken_by 0x00, scale_by 0x02, colours at 0x08 | |
| `GBXModel` (0xe8) | `object_set_permutation_by_name` 0x4f6c60, `object_copy_default_node_transforms` 0x4f6b70 | nodes count 0xb8, regions 0xc4/0xc8 stride 0x4c |
| `ModelRegion` (0x4c) / `ModelRegionPermutation` (0x58) | `object_permutation_find_matching_group` 0x4f8d80, `object_regions_initialize_permutations` 0x4f8dd0 | permutations 0x40/0x44, permutation flags 0x20 bit 0, permutation_number 0x24 |
| `ModelCollisionGeometry` (0x298) / `ModelCollisionGeometryRegion` (0x54) | `object_initialize_shield_stun_thresholds` 0x4ed440, `object_destroy_region` 0x4f02d0, `object_regions_reset_permutation_lock` 0x4f03e0, `object_apply_body_damage` 0x4ef2a0, `object_apply_shield_damage` 0x4ef820 | maximum body vitality 0x08, maximum shield vitality 0xcc, shield damage fields 0xf0/0xf4, material index 0xd2, low shield threshold 0x184, regions 0x240/0x244 stride 0x54; region flags 0x20 (bits 0x02, 0x10, 0x20, 0x40, 0x80, 0x100), damage_threshold 0x28, permutations count 0x48 |
| `Antenna` (0xd0) / `AntennaVertex` (0x80) | `antenna_new` 0x4faa90 | bitmaps TagID 0x2c, vertices 0xc4/0xc8 stride 0x80; vertex length 0x24, sequence_index 0x28, offset 0x74 |
| `Flag` (0x60) | `flag_new` 0x4fb540, `flag_cloth_init_shape_constraints` 0x4fb770, `flag_pole_get_marker_positions` 0x4fc020 | shape 0x04, width 0x0c, height 0x0e, blue shader TagID 0x50, attachment_points 0x54/0x58 |
| `Glow` (0x154) | `glow_update` 0x4fce80, `glow_particle_new` 0x4fd8e0, `glow_particle_compute_color` 0x4fd420, `glow_chain_build` 0x4fd830 | marker name 0x00, number_of_particles 0x20, glow_flags 0x28 (bits 0x02/0x04/0x08/0x20), distance bounds 0x84..0x90, particle size bounds 0xa0/0xa4, colour bounds 0xb8..0xd0, trailing particle fields 0x108/0x10c |
| `Light` (0x160) | `object_lights_update_all` 0x4f0cf0, `lights_apply_spot_falloff` 0x4f1780 | cutoff angle 0x14, intensity 0x24 region, duration 0xf4 |

`DamageEffect`, `LightVolume` and `Lightning` are referenced but only through offsets in the
0x1c4..0x1dc and 0xa4 ranges, which are inside their documented sizes; nothing contradicted
`tags.h`.

## Structs defined in types/objects.h

### `object_header` (0x0c) and the object data_array

Established by `object_try_and_get` 0x4f6ec0 (identifier at +0x00, type byte at +0x03 tested
as `1 << type`, data pointer at +0x08), `object_iterator_next` 0x4f6f20 (flags byte at +0x02
masked, stride 0x0c from `psVar4 + 6` on a `short *`), `object_new_with_datum_role_control`
0x4f54b0 (stamps flags 0x44, writes the type byte, sets +0x04 to -1),
`object_block_data_new` 0x4f7d50 and `object_block_data_grow` 0x4f7e50 (block size at +0x06),
`objects_update` 0x4f4e90 (the +0x04 cluster index is what the PVS bitset is indexed with),
`object_dump_accumulate_stats` 0x4fa3d0 (+0x06 is the size the memory dump totals).

Array: `objects_initialize` 0x4f4ad0, `game_state_new("object", 0x800)`.

### `object` (0x1f4 common header)

Almost every field came out of one of four functions; the rest are named `unknown_XX`.

| offsets | established by |
|---|---|
| 0x000, 0x004, 0x008, 0x00c, 0x010, 0x014, 0x018 | `object_new_with_datum_role_control` 0x4f54b0 writes all of them at create |
| 0x020 | `objects_update_player_visibility_masks` 0x4f4880 only. Marked UNSURE. |
| 0x05c position | `object_get_position` 0x4f6900, `object_get_world_matrix` 0x4f6a20 |
| 0x068 velocity, 0x08c angular velocity | `object_get_root_object_velocities` 0x4f6aa0 returns exactly these two triples |
| 0x074 forward, 0x080 up | `object_get_orientation` 0x4f6970, `object_set_velocity_and_orientation` 0x4f51c0 |
| 0x098 leaf, 0x09c cluster | `object_set_cluster_and_parent` 0x4f5c30, `objects_recompute_cluster_membership` 0x4f7570; `damage_apply_area_effect` 0x4edd30 passes the same 8-byte pair out of a `damage_data` |
| 0x0a0 bounding centre, 0x0ac radius | `object_find_in_sphere` 0x4f6fe0 does the sphere test with exactly 0xa0..0xac; `object_get_center_of_mass_and_scale` 0x4088e0 copies 0xa0..0xac |
| 0x0b0 scale | every `object_recalculate_bounding_radius*` variant scales the radius by it |
| 0x0b4 type | `object_new_with_datum_role_control` copies the Object tag object_type into it; `object_find_in_sphere` tests `1 << *(int16 *)(obj+0xb4)` |
| 0x0b8 name index | written from `object_placement_data` +0x14, read back into a new placement by `object_placement_data_initialize` 0x4f53a0 |
| 0x0ba render cache slot | `object_reserve_render_cache_slot` 0x4f9ac0, `object_release_render_cache_slot` 0x4f9b00 |
| 0x0c0, 0x0c4 | `object_placement_data_initialize` reads 0xc0 off the current object; the constructor writes both |
| 0x0cc graph, 0x0d0 index, 0x0d2 frame | `object_start_animation` 0x4fa8d0, `object_animation_get_frames_remaining` 0x4fa9b0 |
| 0x0d6 node function count | `object_copy_default_node_transforms` 0x4f6b70, `object_offset_node_translation` 0x4f6c10 |
| 0x0d8, 0x0dc maxima and 0x0e0, 0x0e4 fractions | `object_initialize_shield_stun_thresholds` 0x4ed440 seeds 0xd8 from ModelCollisionGeometry+0x08 and 0xdc from +0xcc, then sets 0xe0 / 0xe4 to 1.0 when the corresponding maximum is positive. `object_apply_body_damage` 0x4ef2a0 and `object_apply_shield_damage` 0x4ef820 divide by the maxima and subtract from the fractions. |
| 0x0e8, 0x0ec, 0x0f4, 0x0f8, 0x0fc, 0x100, 0x104, 0x106 | `object_apply_shield_damage` 0x4ef820 and `object_update_vitality_and_regeneration` 0x4ed510 |
| 0x0f0 | `object_clear_references_to_object` 0x4f73e0 clears any object whose 0xf0 points at the dying object |
| 0x10c | `object_get_root_parent_placement` 0x4f5f70 returns it |
| 0x110 | `object_list_membership_set` 0x4f7450, the list head is `object_globals` +0x08 |
| 0x114, 0x118, 0x11c, 0x120 | `object_attach_to_object` 0x4f6440 (cycle check up 0x11c, then writes 0x11c and 0x120), `object_snap_to_parent_marker_and_detach` 0x4f6610 (clears 0x11c to -1 and 0x120 to 0xff), `object_get_root_object_index` 0x4f6fb0, `object_remove_from_sibling_list` 0x4f8fe0 |
| 0x123, 0x124, 0x134 | `object_update_functions` 0x4f92f0 writes `obj[0x4d + i]` = 0x134 + 4i and sets or clears bit i of the byte at 0x123; `object_function_get_value` 0x4f6e70 reads exactly that pair. The scale-by lookup `obj[selector + 0x48]` is 0x120 + 4*selector, which covers 0x124..0x143, that is four "in" values then the same four "out" values the function loop writes. |
| 0x144, 0x14c | `object_create_attachments` 0x4f9750 writes the type byte at `0x144 + i` and the handle at `obj[0x53 + i]` = 0x14c + 4i; `object_delete_attachments` 0x4f9900 reads both back |
| 0x16c | `widget_new` 0x4ffa80 (`obj[0x5b]`) and `widget_delete_all` 0x4ffbe0 |
| 0x174 | `object_destroy_region` 0x4f02d0 uses it as the already-destroyed bitmask (`obj[0x5d]` as a `ushort`) |
| 0x176 | constructor copies Object tag +0x13e into it |
| 0x178 | `object_apply_body_damage` 0x4ef2a0 writes `obj + 0x178 + region` and tests it against the region damage_threshold |
| 0x180 | `object_set_permutation_by_name` 0x4f6c60 and `object_regions_initialize_permutations` 0x4f8dd0 write `obj + 0x180 + region`; `object_regions_reset_permutation_lock` 0x4f03e0 forces the same bytes to 0 or 1; `object_get_node_local_transform` 0x4f6080 passes `obj + 0x180` to `model_markers_get_by_name` as the permutation table |
| 0x1b8 | `object_update_change_colors` 0x4f9110 writes `obj[0x6e + 3i]` = 0x1b8 + 12i for four entries |
| 0x1e8, 0x1ec, 0x1f0 | `object_block_data_grow` 0x4f7e50 writes `{size, offset}` at the field offset it is given; the constructor calls it with 0x1f0 (node_count * 0x34), 0x1ec and 0x1e8 (node_count * 0x20 each). `object_get_node_marker_address` 0x4f6000 computes `obj + *(int16 *)(obj+0x1f2) + i*0x34` and `object_copy_default_node_transforms` 0x4f6b70 copies from `obj + *(int16 *)(obj+0x1ee)` to `obj + *(int16 *)(obj+0x1ea)`. |

The node array element is `real_matrix4x3` (0x34) because `object_get_node_local_transform`
copies exactly 13 dwords out of it and builds the identity fallback by writing 1.0 at +0x04,
+0x18 and +0x28 of the destination, which is the scale plus the 3x3 diagonal.

### `object_placement_data` (0x88)

`object_placement_data_initialize` 0x4f53a0 zeroes 0x22 dwords and seeds every default;
`object_new_with_datum_role_control` 0x4f54b0 consumes it field by field. The scalar at 0x24
is a height along the up vector, not a scale: the constructor does
`position += placement[9] * up`.

### `damage_data` (0x54)

`damage_data_initialize` 0x4ed990 zeroes 0x15 dwords and writes the sentinels at 0x08, 0x0c,
0x10, 0x18 and 0x4c plus the two 1.0 multipliers at 0x40 and 0x44.
`damage_apply_area_effect` 0x4edd30 passes `&dd[5]` (0x14) as the search location and
`&dd[7]` (0x1c) as the sphere centre, which is what fixed the location pair at 0x14/0x18 and
the epicentre at 0x1c. `object_damage_apply_line_of_sight` 0x4eddb0 uses `&dd[10]` (0x28) as
the ray origin. `object_apply_damage` 0x4ee5e0 reads 0x08 against the player data_array, 0x0c
with the unit type mask, 0x10 as a team index against a ten-team bitfield, 0x40 as the random
blend weight and 0x44 as the final multiplier (which it divides by the seated-biped count
when a vehicle spreads damage).

### `object_iterator` (0x0c)

`object_iterator_next` 0x4f6f20 only. The int16 index at +0x06 overlaps the high half of the
dword the function loads for the flags mask, which is why the flags mask is a single byte at
+0x04 with one byte of padding.

### `object_globals` (0x98)

Size is exact: `objects_initialize` 0x4f4ad0 reserves 0x98 bytes of game state and stores the
base at 0x006b8cbc. Fields from `object_collect_in_clusters` 0x4f7180 (+0x01),
`object_list_membership_set` 0x4f7450 (+0x08), `objects_update` 0x4f4e90 (the two 0x40-byte
PVS bitsets at +0x0c and +0x4c, sized by `(cluster_count + 0x1f) >> 5` dwords, so 16 dwords
is the 0x200-cluster maximum), `objects_set_ambient_cluster_override` 0x4f79d0 and
`objects_get_ambient_cluster` 0x4f7a50 (+0x90, +0x94).

### `object_type_definition` (0xc4 as far as this module reaches)

`object_type_definition_chain_build` 0x4f3db0 gives the sub-definition array at +0x80 (16
slots, first null ends it), the chain link at +0xc0 and the initialize hook at +0x14.
`object_dump_write` 0x4fa490 gives the name pointer at +0x00. `object_new_with_datum_role_control`
gives the runtime object size at +0x08. `objects_delete_unparented_of_type_mask` 0x4f47c0 and
`object_delete` 0x4f5bd0 give the category at +0x04 (0 means delete immediately, 3 means
delete recursively). The vtable columns 0x14..0x7c are each pinned by exactly one broadcast
or override helper in the 0x4f3e30..0x4f4760 run, named in the header.

### `object_cluster_reference` (0x0c)

`object_collect_in_clusters` 0x4f7180: the per-cluster head table gives a handle, and each
element has the object handle at +0x04 and the next reference at +0x08 inside a 0x0c stride.

### `object_marker` (0x6c)

`object_get_node_local_transform` 0x4f6080 builds it: node index at +0x00, an identity
`real_matrix4x3` at +0x04, then 13 dwords copied out of the object node array at +0x38.
`glow_update` 0x4fce80 walks an array of them with stride 0x6c and reads the position of each
at entry+0x60, which is the translation of the second matrix.

### `object_memory_dump_record` (0x18)

`objects_dump_memory` 0x4fa500 qsorts two arrays with stride 0x18;
`object_dump_compare_by_total_size` 0x4fa3a0 sorts on +0x08; `object_dump_accumulate_stats`
0x4fa3d0 and `object_dump_write` 0x4fa490 together fix every counter against the printed
column header `number (active) [garbage/dead/outside/at-rest] maxsize totsize`.

### `hash_table` family

`hash_table_set_or_remove` 0x4f0530, `hash_table_get` 0x4f05e0 and `hash_table_grow_freelist`
0x4f0620. The 600-byte node pool divided by the 0x3c-per-iteration unroll gives 50 nodes of
0x0c bytes per block. This is generic engine code that happens to live in the objects module;
it is declared in `objects.h` because no other recovered module owns it.

### `light` (0x7c)

Stride proved by `(index & 0xffff) * 0x7c` in `light_new_attached` 0x4f0af0 and
`object_light_clear_dirty_flag` 0x4f29c0. Note that the array maximum is 0x380 and the stride
is 0x7c; the phase 2 evidence for `lights_initialize` reported 0x380 as a stride, which is
wrong. `game_state_new(name, maximum_count)` takes the element size in a register, so 0x380
is the count.

### `light_transient` (0x28), `widget` (0x0c), `widget_type_definition` (0x28)

`light_transient_add` 0x4f1600 writes eight parallel-array bases 4 bytes apart with a common
stride of 0x28. `widget_new` 0x4ffa80 and `widget_delete_all` 0x4ffbe0 give the widget datum.
`widget_type_definition` was read out of `bin/halo.exe` rather than inferred (see below).

### `antenna` (0x2bc), `flag` (0x16bc)

Strides are exact from the `* 700` and `* 0x16bc` datum arithmetic in `antenna_new` 0x4faa90
and `flag_new` 0x4fb540. The antenna vertex array is 21 entries because
`(0x2bc - 0x1c) / 0x20 = 21`, and the constructor writes one extra tip vertex past the tag
vertex count. The flag cloth grid is 225 entries because `0x1c + 225*0x18 = 0x1534`, which is
exactly where `flag_cloth_set_region_split_flags` 0x4fb840 starts writing the per-cell split
codes, and `(0x16bc - 0x1534) / 2 = 196` codes.

### `glow` (at least 0x258) and `glow_particle` (at least 0x64)

`glow_update` 0x4fce80, `glow_chain_build` 0x4fd830, `glow_particle_new` 0x4fd8e0,
`glow_particle_spawn` 0x4fdb20, `glow_particle_reposition` 0x4fde40, `glow_render` 0x4fe570.
The marker array is 5 entries because `glow_update` asks `object_get_node_local_transform`
for at most 5 and `0x08 + 5*0x6c = 0x224`, exactly where the tag handle lives.

### `object_zone_light_table` (0x4204)

`zone_light_table_initialize` 0x4ffd40 and `zone_light_table_test_bit` 0x4ffda0.

## Unresolved offsets

- `object` 0x022..0x05b (0x3a bytes) and 0x188..0x1b7 (0x30 bytes): no function in this
  module reads or writes any byte of either range. They are the largest holes in the record.
- `object` 0x008, 0x00c, 0x0bc, 0x0c8, 0x108, 0x170: written at create (mostly to -1) but
  never read anywhere in this module.
- `object` 0x020: only `objects_update_player_visibility_masks` touches it, so the width
  (16-bit) and the meaning are both from a single site. Marked UNSURE in the header.
- `object` 0x0b6, 0x09e, 0x0d4: padding by position, not by proof.
- `object` 0x178 `region_vitality[8]`: `object_apply_body_damage` 0x4ef2a0 stores a byte per
  region and then tests `region->damage_threshold < byte * (1/255)`, so it is a 0..255 damage
  accumulator. The producer of the byte is `FUN_006391b4`, which this pass did not identify,
  so the exact quantity (fraction of region vitality versus a tick stamp) is not settled.
  The adjacent array at 0x180 is unambiguous: `object_set_permutation_by_name` 0x4f6c60 and
  `object_regions_initialize_permutations` 0x4f8dd0 write permutation indices into it, and
  `object_regions_reset_permutation_lock` 0x4f03e0 forces it to 0 or 1 for destroyable
  multi-permutation regions.
- `damage_data` 0x34..0x3f: read by the knockback path but only ever written by callers
  outside this module, so the direction interpretation is UNSURE. 0x12, 0x1a, 0x48, 0x4e and
  0x50 are never touched.
- `light` 0x14..0x2b, 0x48..0x53, 0x62..0x77 and 0x78: the two constructors write an
  overlapping set of fields from 0x5c onwards (three marker shorts in the attached form
  versus a short plus two vectors in the positioned form), so 0x5c..0x77 is really a union
  and is left flattened.
- `object_type_definition`: whether the record continues past 0xc4 cannot be decided, because
  the table at 0x0069bfdc holds pointers rather than inline records. Slots 0x60 and 0x78 are
  inside the vtable range but no function in this module calls them.
- `glow` and `glow_particle`: their data_arrays are created by `glow_initialize` 0x4fcbb0,
  which is outside this module, so neither stride is proven. The sizes in the header are
  lower bounds from the highest offset actually touched.
- The object control binding tables at 0x008603e0..0x008603fc, 0x00860430/0x00860434 and
  0x00860480/0x00860484 are parallel arrays whose bases sit 4 to 8 bytes apart, so they are
  not one struct. `object_control_binding_table_update_a` 0x4f3890 and `_update_b` 0x4f39d0
  index them through register arguments Ghidra lost, and both fall through to jump tables
  (`PTR_LAB_004f39bc`, `PTR_LAB_004f3abc`). No struct is declared; the addresses are recorded
  as globals only.
- `object_control_word_field_extract` 0x4f3680 pulls six different 3-bit fields (shifts 4, 7,
  10, 0x10, 0x13, 0xd) out of the packed words at 0x006f1cec / 0x006f1ce8. Those words belong
  to the input or game-engine module, so the bitfield is not defined here.
- The light volume and lightning instance records (0x006b8d70, 0x006b8d74) are allocated
  outside this module; only the render hooks are here, and they reach the instance through a
  hash-slot lookup, so no layout is claimed for either.

## Misattributed functions

### The glow widget, wrongly named lightning

The five-row table at `0x0069c010` was read straight out of `bin/halo.exe` (PE VA 0x676000
maps to file offset 0x276000, so 0x0069c010 is at 0x0029c010). Its rows are:

```
row 0  'flag'  flag=1  init 0x4fb4d0 dispose 0x4fb4f0 clear 0x4fb510 reset 0x4fb520
               new 0x4fb540 delete 0x4fb970 update 0x4fba00 render 0x4fb980
row 1  'ant!'  flag=0  init 0x4faa20 dispose 0x4faa40 clear 0x4faa60 reset 0x4faa70
               new 0x4faa90 delete 0x4fac80 update 0x4fad20 render 0x4fac90
row 2  'glw!'  flag=0  init 0x4fcbb0 dispose 0x4fcc00 clear 0x4fcc30 reset 0x0044ad80
               new 0x4fcc50 delete 0x4fcd40 update null   render 0x4fcdb0
row 3  'mgs2'  flag=0  init 0x4fe680 dispose 0x4fe6a0 clear 0x4fe6c0 reset 0x0044ad80
               new 0x4fe6d0 delete 0x4fe720 update null   render 0x4fe900
row 4  'elec'  flag=0  init 0x4fee80 dispose 0x4feea0 clear 0x4feec0 reset 0x0044ad80
               new 0x4feed0 delete 0x4fef20 update null   render 0x4ff010
```

This contradicts the phase 2 naming for a whole run of functions. `0x4fcdb0` is the render
hook of the **glow** widget, not lightning, and the functions it drives are glow functions:

| address | phase 2 name | what it actually is |
|---|---|---|
| 0x4fcdb0 | `lightning_instance_update` | `glow_render_dispatch` (widget type 2 render column) |
| 0x4fce80 | `lightning_update` | `glow_update` |
| 0x4fd3a0 | `lightning_segment_compute_fade` | `glow_particle_compute_fade` |
| 0x4fd420 | `lightning_segment_compute_color` | `glow_particle_compute_color` |
| 0x4fd4a0 | `lightning_segment_compute_position` | `glow_particle_compute_position` |
| 0x4fd650 | `lightning_segment_advance_time` | `glow_particle_advance_time` |
| 0x4fd830 | `lightning_bolt_build_chain` | `glow_chain_build` |
| 0x4fd8e0 | `lightning_segment_new` | `glow_particle_new` |
| 0x4fdb20 | `lightning_segment_spawn` | `glow_particle_spawn` |
| 0x4fdde0 | `lightning_shard_datum_new` | `glow_particle_datum_new` |
| 0x4fde40 | `lightning_segment_reposition` | `glow_particle_reposition` |
| 0x4fe570 | `lightning_render` | `glow_render` |
| 0x4fe900 | `light_volume_new` | `light_volume_render` (widget type 3 render column) |
| 0x4ff010 | `light_volume_render` | `lightning_render` (widget type 4 render column) |

Independent confirmation for the glow half: every tag offset those functions read lands
inside the `Glow` struct in `tags.h` and means the right thing there. `0x4fd830` reads the
flags at tag+0x28 (`GlowFlags`), `0x4fd8e0` randomizes from tag+0x84..0x90
(`min/max_distance_particle_to_object` and their multipliers), tag+0xa0/0xa4
(`particle_size_bounds`) and tag+0xb8..0xd0 (`color_bound_0`, `color_bound_1`), and
`0x4fdb20` reads tag+0x108/0x10c (`trailing_particle_minimum_t` / `maximum_t`). None of those
offsets makes sense against `Lightning` (0x108 total). Correspondingly, `0x008603a0` and
`0x008603a4` are the glow instance and glow particle arrays, not lightning arrays.

### Ghidra-split tails and folded duplicates, no types of their own

These carry no independent structure and were skipped:

- 0x4efea0 `mdp_decode_stateless_iterated`, 0x4efea8 `caseD_0`, 0x4eff10
  `mdp_decode_incremental_iterated`: outlined or ICF-folded pieces of
  `object_damage_notify_and_impulse` 0x4efcf0. The inherited PDB names describe network
  decode logic that is not present.
- 0x4f3ef0 `index_resolution_table_translate`: the shared tail of the vtable +0x28 predicate
  loop at 0x4f3ea0, zero callers, no index table anywhere in it.
- 0x4f8dc0 `object_new_with_role`: 7 bytes. Ghidra attributes the whole body of
  `object_permutation_find_matching_group` 0x4f8d80 to it. It is a thunk or an alternate
  entry point, not a constructor.
- 0x4f9990 `object_get_marker_by_name`: the same attachment-delete switch as
  `object_delete_attachments` 0x4f9900 with a self-loop, and there is no name string or
  comparison in it.
- 0x4f133c: the tail of `object_lights_update_all` 0x4f0cf0.
- 0x4f84e2, 0x4f8834, 0x4f8a70: compiler-cloned variants of
  `object_recalculate_bounding_radius` 0x4f8310; all three end with the same write to
  object+0xac scaled by object+0xb0, and all three have zero recorded callers. Their
  inherited names (`objects_initialize_for_new_map_mod_processed_bsps`,
  `objects_update__object_in_player_pvs_nop1`, `object_reset`) do not match.
- 0x4f93b0, 0x4f94e0, 0x4f9540: byte-identical copies of the per-node
  `periodic_function_evaluate` loop; the inherited names `object_delete_to_network`,
  `object_delete` and `object_reconnect_to_map` do not match a function-value recompute.
- 0x4f8207 `object_types_place_objects_mod_processed_bsps__read`: this is the object function
  input evaluator, and it is the function that fixed the 0x124..0x143 value array.
- 0x4f1ef0 `object_cause_damage`: samples BSP lightmap colour and incident vectors; no damage
  is applied anywhere in it.
- 0x4fa280 `object_get_orientation` (the second one, distinct from 0x4f6970): walks a bitfield
  calling `object_tree_collect_matching`; no orientation math present.
- 0x4f29c0 `chimera__light_table` and 0x4ee5e0 `chimera__apply_damage`: Chimera-derived
  labels, not Bungie identifiers. The second one is genuinely `object_apply_damage`.

### Not objects-module code

- 0x4f0530 / 0x4f05e0 / 0x4f0620 (hash table) and 0x4f06d0 / 0x4fca60 / 0x4fcb00 (barycentric
  and cubic interpolation) are generic helpers with no object in sight. The interpolators
  belong with `types/math.h` if that header is ever extended; they are not declared in
  `objects.h`. The hash table is declared here because nothing else owns it.
- 0x4f0730 / 0x4f0900 / 0x4f1ef0 / 0x4f2550 sample BSP lightmaps and belong to the BSP or
  rendering module; they touch structure_bsp lightmap blocks, not object records, so no types
  for them are defined here.
- 0x4f3680 / 0x4f3700 / 0x4f37d0 / 0x4f3890 / 0x4f39d0 / 0x4f3ad0 read the packed control
  words at 0x006f1cec and 0x006f1ce8, which belong to the input or game module.
