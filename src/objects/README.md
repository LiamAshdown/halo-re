# `objects` - the Blam object system

Retail Halo PC `halo.exe` 1.0.10, `0x4088e0 .. 0x4ffda0` (65,913 bytes of code, 233 files for the
252 functions Ghidra lists in the range), plain C / MSVC 7.1 / x86. Every file in this directory
is one function, rewritten from its Ghidra decompilation against `types/objects.h`, with the
original decompile preserved verbatim at the bottom of the file inside `#if 0 ... #endif` for
diffing.

Gate: `python tools/build_check.py objects` -> **233 ok, 0 failed**.

This is the largest and least settled module recovered so far. Read the confidence column in the
function table before trusting any single file: about a quarter of the module sits at 0.3 or
below, and those files are close transliterations of Ghidra's output rather than understood code.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| damage | `0x4ed440`-`0x4f0510` | `damage_data`, the shield/body damage split, region destruction, damage effects |
| lights | `0x4f0a20`-`0x4f3410` | the `light` data_array, attached and positioned lights, lightmap sampling, the transient light table |
| object type definitions | `0x4f3680`-`0x4f4760` | the 12-entry vtable table at `0x0069bfdc` and the broadcast / query / override helpers that walk it |
| object lifecycle | `0x4f47c0`-`0x4f5f70` | `objects_initialize` / `_update` / `_dispose`, `object_new`, the three deletion paths, cluster membership |
| object accessors | `0x4f6000`-`0x4f7b00` | markers and node transforms, attachment, position and orientation, the iterator, sphere and cluster queries |
| object storage | `0x4f7d50`-`0x4f7e50` | the `memory_pool`-backed variable-length object record |
| per-frame update | `0x4f7ef0`-`0x4fa9b0` | `object_update`, bounding radius and node evaluation, functions and change colours, attachments, GC, the memory dump |
| widgets | `0x4faa20`-`0x4ffda0` | antenna, flag (cloth), glow, plus the light-volume and lightning render hooks and the `widget` datum list |

The object record is variable length: `object` below is only the common `0x1f4`-byte header that
every biped / vehicle / weapon / ... extends, plus three trailing arrays reached through
`object_block_reference` fields at `0x1e8`, `0x1ec` and `0x1f0`.

The five widget types are driven by a read-only table at `0x0069c010` that was read straight out
of `bin/halo.exe` rather than inferred; it is what fixed the widget order and corrected a whole
run of inherited names (see "Misattributed functions").

## Struct layouts

All of these live in `types/objects.h`; the tables below are the summary, and offsets are byte
offsets from the struct base under `#pragma pack(push,1)`. Types this module uses but does not
own -- `data_array`, `datum_index`, `memory_pool` (`types/memory.h`), `real_point3d`,
`real_vector3d`, `real_matrix4x3` (`types/math.h`), `tag_instance` (`types/cache.h`) and every
tag structure (`types/tags.h`) -- are not redefined here.

### `object_header` - size `0x0c`, element of the `0x800`-slot object `data_array` at `0x008603b0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` - datum salt, 0 marks the slot empty |
| `0x02` | `uint8_t` | `flags` - `object_header_flags` |
| `0x03` | `uint8_t` | `type` - `object_type`, tested as `1 << type` against a type mask |
| `0x04` | `int16_t` | `cluster_index` - cached BSP cluster, -1 at create |
| `0x06` | `int16_t` | `block_size` - bytes of pool storage behind `data` |
| `0x08` | `object *` | `data` - the record in the objects `memory_pool` |

### `object` - the common `0x1f4` header; selected fields, the full list is in the header

| Off | Type | Field |
|---|---|---|
| `0x000` | `datum_index` | `definition_tag` - the `Object` tag |
| `0x004` | `int32_t` | `network_role` - `object_delete` dispatches on 0 versus 3 |
| `0x010` | `uint32_t` | `flags` - `object_flags` |
| `0x014` | `int32_t` | `cluster_stamp` - compared against `0x008603cc` |
| `0x05c` | `real_point3d` | `position` |
| `0x068 / 0x08c` | `real_vector3d` | `velocity` / `angular_velocity` |
| `0x074 / 0x080` | `real_vector3d` | `forward` / `up` |
| `0x098 / 0x09c` | `int32_t / int16_t` | `location_leaf_index` / `location_cluster_index` - a `bsp_leaf_reference` in all but name |
| `0x0a0 / 0x0ac` | `real_point3d / float` | `bounding_center` / `bounding_radius` |
| `0x0b0 / 0x0b4` | `float / int16_t` | `scale` / `type` |
| `0x0d8..0x0e4` | `float x4` | `maximum_body_vitality`, `maximum_shield_vitality`, then the two fractions |
| `0x0f0` | `datum_index` | `damage_owner` |
| `0x10c..0x120` | `datum_index x4, uint8_t` | `placement_id`, `next_tracked_object`, `next_object`, `first_child_object`, `parent_object`, `parent_marker_index` |
| `0x123 / 0x124 / 0x134` | `uint8_t / float[4] / float[4]` | `function_valid_flags`, `function_in_values`, `function_out_values` |
| `0x144 / 0x14c` | `int8_t[8] / datum_index[8]` | `attachment_types` / `attachment_handles` |
| `0x16c` | `datum_index` | `first_widget` |
| `0x174 / 0x176` | `uint16_t` | `destroyed_region_flags` / `forced_shader_permutation` |
| `0x178 / 0x180` | `uint8_t[8]` | `region_vitality` / `region_permutations` |
| `0x1b8` | `ColorRGB[4]` | `change_colors` |
| `0x1e8 / 0x1ec / 0x1f0` | `object_block_reference` | `node_function_values`, `node_function_defaults`, `nodes` (`real_matrix4x3`, stride `0x34`) |

### `object_block_reference` - size `0x04`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `size` in bytes |
| `0x02` | `int16_t` | `offset` from the object base |

### `object_type_definition` - size `0xc4` as far as this module reaches; 12 pointers at `0x0069bfdc`

| Off | Type | Field |
|---|---|---|
| `0x00` | `char *` | `name` |
| `0x04` | `int32_t` | `category` - 0 delete immediately, 3 delete recursively |
| `0x08` | `int16_t` | `object_size` - the runtime block size |
| `0x14..0x20` | `void *` | `initialize`, `dispose`, `reset`, `flush` |
| `0x24..0x5c` | `void *` | the notify / query vtable column, one helper per slot in `0x4f3e30..0x4f4760` |
| `0x64..0x7c` | `void *` | the override column (`object_type_override_*`) |
| `0x80` | `object_type_definition *[16]` | `subdefinitions`, first null ends the array |
| `0xc0` | `object_type_definition *` | `next`, chain head at `0x008603dc` |

### `object_placement_data` - size `0x88`

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `definition_tag` |
| `0x04` | `uint32_t` | `flags` |
| `0x08 / 0x0c` | `uint32_t` | `owner_linkage` / `role` |
| `0x14 / 0x16` | `int16_t` | `name_index` / `permutation_group` |
| `0x18` | `real_point3d` | `position` |
| `0x24` | `float` | `height_above_origin` - `position += height * up`, NOT a scale |
| `0x28 / 0x34 / 0x40 / 0x4c` | `real_vector3d` | `velocity`, `forward`, `up`, `angular_velocity` |
| `0x58` | `real_vector3d[4]` | `network_vectors` |

### `damage_data` - size `0x54`

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `damage_effect_tag` |
| `0x04` | `uint32_t` | `flags` |
| `0x08 / 0x0c` | `datum_index` | `responsible_player` / `responsible_object` |
| `0x10` | `int16_t` | `team_index` |
| `0x14 / 0x18` | `int32_t / int16_t` | `location_leaf_index` / `location_cluster_index` |
| `0x1c` | `real_point3d` | `epicentre` |
| `0x28` | `real_point3d` | `origin` |
| `0x34` | `real_vector3d` | `direction` - UNSURE, only ever written from outside this module |
| `0x40 / 0x44` | `float` | `random_blend` / `multiplier` |

### `damage_effect_vector_block` - size `0x3c`, UNSURE

| Off | Type | Field |
|---|---|---|
| `0x00..0x30` | `real_vector3d[5]` | `vector0`..`vector4`, paired positionally with the strings "normal", "incident", "negative incident", "reflection", "gravity" |

### `bsp_leaf_reference` - size `0x08`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `leaf_index` - `bsp3d_node_find_leaf` (`0x5013a0`) result, -1 when outside the BSP |
| `0x04` | `int16_t` | `cluster_index` - `ScenarioStructureBSPLeaf.cluster`, at `+0x08` of a `0x10`-byte leaf |
| `0x06` | `int16_t` | padding, never read |

### `object_placement_cursor` - size `0x08`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t *` | `cluster_globals` - `&cluster_first` of the applicable three-global group |
| `0x04` | `datum_index` | `next_reference` - `placement_id`, then each `object_cluster_reference.next_reference` |

### `object_cluster_reference` - size `0x0c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` |
| `0x02` | `int16_t` | unknown |
| `0x04` | `datum_index` | `object_index` |
| `0x08` | `datum_index` | `next_reference` |

### `object_iterator` - size `0x0c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `type_mask` |
| `0x04` | `uint8_t` | `flags_mask` |
| `0x06` | `int16_t` | `index` |
| `0x08` | `datum_index` | `handle` |

### `object_globals` - size `0x98`, game state at `0x006b8cbc`

| Off | Type | Field |
|---|---|---|
| `0x01` | `uint8_t` | `collecting_in_clusters` |
| `0x08` | `datum_index` | `first_tracked_object` |
| `0x0c / 0x4c` | `uint32_t[16]` | `cluster_pvs_previous` / `cluster_pvs_current` (0x200 clusters max) |
| `0x90 / 0x94` | `int16_t` | `ambient_cluster_mode` / `ambient_cluster_index` |

### `object_marker` - size `0x6c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `node_index` |
| `0x04` | `real_matrix4x3` | `transform` - identity on the fallback path |
| `0x38` | `real_matrix4x3` | `node_transform` - 13 dwords copied out of the object node array |

### `object_memory_dump_record` - size `0x18`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x04` | `datum_index / int16_t` | `definition_tag` / `type` |
| `0x06 / 0x08` | `int16_t / int32_t` | `maximum_size` / `total_size`, the qsort key |
| `0x0c..0x16` | `int16_t x6` | `count`, `active_count`, `garbage_count`, `dead_count`, `outside_map_count`, `at_rest_count` |

### `object_statistics` - size `0x08`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x02` | `int16_t` | `count` / `active_count` |
| `0x04` | `float` | `pool_fullness_fraction` - `1 - free/0x200000`; the divisor is a hard-coded 2 MiB |

### `object_shield_impulse_result` - UNSURE, size inferred from two writes

| Off | Type | Field |
|---|---|---|
| `0x04` | `float` | `shield_damage_dealt` |
| `0x08` | `uint8_t` | `depleted_this_call` |

### `hash_table` - size `0x18`; generic engine code this module happens to own

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t` | `initialized` |
| `0x04` | `int32_t` | `bucket_count` |
| `0x08` | `hash_bucket *` | `buckets`; `hash_bucket` is `{int32_t count; hash_node *first;}`, size `0x08` |
| `0x0c` | `int32_t` | `entry_count` |
| `0x10` | `hash_node *` | `freelist`; `hash_node` is `{key, value, next}`, size `0x0c` |
| `0x14` | `hash_node_block *` | `blocks` - 600-byte `GlobalAlloc`s of 50 nodes each |

### `light` - size `0x7c`, `0x380`-slot data_array at `0x00860b14`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x02` | `int16_t / uint16_t` | `identifier` / `flags` (`light_flags`) |
| `0x04` | `datum_index` | `definition_tag` |
| `0x0c` | `int32_t` | `creation_tick` |
| `0x10` | `datum_index` | `next_light` |
| `0x2c` | `datum_index` | `owner_object` |
| `0x30 / 0x3c / 0x48` | `real_point3d, real_vector3d` | `position`, `direction`, `up` in world space |
| `0x54` | `float` | `radius` |
| `0x58 / 0x5c / 0x5e` | `int32_t / int16_t` | `marker_link`, `marker_index`, `marker_index_secondary` |
| `0x60 / 0x6c` | `real_point3d / real_vector3d` | `local_position` / `local_direction`; `0x5c..0x77` is really a union, left flattened |

### `light_transient` - size `0x28`, 8 slots at `0x008609cc`

| Off | Type | Field |
|---|---|---|
| `0x00` | `void *` | `definition` |
| `0x04` | `real_point3d` | `position` |
| `0x18` | `uint32_t` | `color`, packed ARGB |
| `0x20` | `int16_t` | `slot_index` |
| `0x23` | `uint8_t` | `intensity`, the scalar argument rounded into 0..255 |

### `widget` - size `0x0c`, `0x40`-slot data_array at `0x00860398`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x02` | `int16_t` | `identifier` / `type` (`object_widget_type`) |
| `0x04` | `datum_index` | `instance` |
| `0x08` | `datum_index` | `next_widget` |

### `widget_type_definition` - size `0x28`, 5 read-only rows at `0x0069c010`, read out of `bin/halo.exe`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x04` | `uint32_t / int32_t` | `group_tag` (`flag`, `ant!`, `glw!`, `mgs2`, `elec`) / `flag` |
| `0x08..0x14` | `void *` | `initialize`, `dispose`, `dispose_clear_flag`, `reset` |
| `0x18 / 0x1c` | `void *` | `new_instance` / `delete_instance` |
| `0x20 / 0x24` | `void *` | `update`, null for glow, light volume and lightning / `render` |

### `antenna` - size `0x2bc`, 12 slots at `0x008603ac`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x05` | `int16_t / uint8_t` | `identifier` / `degenerate` |
| `0x08 / 0x0c` | `datum_index` | `definition_tag` / `object_index` |
| `0x10` | `real_point3d` | `previous_marker_position` |
| `0x1c` | `antenna_vertex[21]` | 20 tag vertices plus the trailing tip; element is `{position, velocity, texture_scale}`, size `0x20` |

### `flag` - size `0x16bc`, 2 slots at `0x008603a8`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x02` | `int16_t / uint8_t` | `identifier` / `invalid` |
| `0x08 / 0x0c` | `datum_index` | `object_index` / `definition_tag` |
| `0x10` | `real_point3d` | `previous_marker_position` |
| `0x1c` | `flag_vertex[225]` | row-major cloth grid; element is `{position, previous_position}`, size `0x18` |
| `0x1534` | `int16_t[196]` | `cell_split_codes`, one quad-diagonal code per cell |

### `glow` - at least `0x258`; the array at `0x008603a0` is created outside this module, so the stride is not proven

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x02 / 0x04` | `int16_t / uint8_t / int16_t` | `identifier`, `disabled`, `marker_count` (max 5) |
| `0x08` | `object_marker[5]` | `markers` |
| `0x224 / 0x228` | `datum_index / int16_t` | `definition_tag` / `particle_count` |
| `0x22a / 0x234 / 0x238` | `int16_t[5], float, float[5]` | `marker_order`, `total_length`, `cumulative_length` |
| `0x24c` | `int16_t` | `spawn_count` |
| `0x250 / 0x254` | `glow_particle *` | `first_particle` / `last_particle` |

### `glow_particle` - at least `0x64`; same caveat as `glow`

| Off | Type | Field |
|---|---|---|
| `0x00 / 0x04` | `int16_t / datum_index` | `identifier` / `handle` |
| `0x28` | `float` | `t`, position along the marker chain |
| `0x38 / 0x44` | `float[3]` | `base_color` / `render_color` |
| `0x50 / 0x52` | `int16_t` | `age` / `lifetime` |
| `0x54 / 0x58` | `uint32_t / float` | `flags` / `fade` |
| `0x5c / 0x60` | `glow_particle *` | `next` / `previous` |

### `object_zone_light_table` - size `0x4204`, at `0x006b8d78`

| Off | Type | Field |
|---|---|---|
| `0x0000` | `uint8_t` | `valid` |
| `0x0001` | `uint32_t[16][8]` | `membership`, a 256-bit set per viewer, deliberately unaligned |
| `0x0204` | `float[16][256]` | `weights` |

## Conventions used in these files

* **File header.** Every file opens with the function name, `address`, `size`, `name confidence`,
  `rewrite confidence`, an `evidence:` list naming the exact struct fields, globals and callees it
  relies on, and a `register convention:` / `blam-cc:` line. Anything not established is marked
  `// UNSURE:` at the point of use. 589 such annotations remain across the module (excluding the `#if 0` blocks).
* **Gate.** `tools/build_check.py` is a 64-bit host `gcc -fsyntax-only` pass. It catches undeclared
  identifiers, wrong field names and type mismatches. It does **not** validate 32-bit layout, and
  `-Wint-to-pointer-cast` warnings on `TagReflexive.pointer`-style fields are expected.
* **`extern` declarations.** One declaration per symbol, agreeing with the definition in its own
  module. Where two call sites of the same foreign address show incompatible argument lists,
  the symbol is declared with an **empty parameter list** (`extern void f();`) rather than a
  fabricated prototype - that asserts nothing and stays compatible with the canonical signature.
  This applies to `FUN_00450870`, `FUN_004507a0`, `FUN_00505880`, `FUN_004f9b70`, `__ftol`,
  `block_list_compact`, `render_billboard_quad_build`, and to `vector3d_cross_product`,
  `matrix4x3_transform_point`, `matrix4x3_from_forward_up` and `matrix4x3_multiply_procedure` in
  the three files whose call sites show fewer operands than the math module's real signature.
* **Register conventions.** MSVC 7.1 gave this module a lot of custom register calling
  conventions, and Ghidra models many of them as unsourced `in_EAX` / `unaff_ESI` / `extraout_ECX`
  values. Where a convention was settled by disassembling `bin/halo.exe` directly, the file header
  says so and quotes the instruction addresses.
* **Compiler clones.** Ten addresses in this range are zero-caller compiler clones or outlined
  fragments of a neighbouring function. They are kept as comment-only stub files (header plus the
  `#if 0` decompile, no translated body) so that the address stays accounted for without a second,
  unverifiable copy of the same logic.

## Misattributed functions

The inherited Ghidra / PDB / Chimera names are wrong for a long run of this module. Every
correction below is recorded in `symbols/agent_phase4_objects.txt` with its evidence.

### The glow widget, inherited as "lightning"

The five-row `widget_type_definition` table at `0x0069c010` was read out of `bin/halo.exe`
(PE VA `0x676000` maps to file offset `0x276000`). Row 2 is `glw!` with its render hook at
`0x4fcdb0`, which re-attributes fourteen functions:

| address | inherited name | what it actually is |
|---|---|---|
| `0x4fcdb0` | `lightning_instance_update` | `glow_render_dispatch` |
| `0x4fce80` | `lightning_update` | `glow_update` |
| `0x4fd3a0` | `lightning_segment_compute_fade` | `glow_particle_compute_fade` |
| `0x4fd420` | `lightning_segment_compute_color` | `glow_particle_compute_color` |
| `0x4fd4a0` | `lightning_segment_compute_position` | `glow_particle_compute_position` |
| `0x4fd650` | `lightning_segment_advance_time` | `glow_particle_advance_time` |
| `0x4fd830` | `lightning_bolt_build_chain` | `glow_chain_build` |
| `0x4fd8e0` | `lightning_segment_new` | `glow_particle_new` |
| `0x4fdb20` | `lightning_segment_spawn` | `glow_particle_spawn` |
| `0x4fdde0` | `lightning_shard_datum_new` | `glow_particle_datum_new` |
| `0x4fde40` | `lightning_segment_reposition` | `glow_particle_reposition` |
| `0x4fe570` | `lightning_render` | `glow_render` |
| `0x4fe900` | `light_volume_new` | `light_volume_render` (row 3 render column) |
| `0x4ff010` | `light_volume_render` | `lightning_render` (row 4 render column) |

Corroboration for the glow half: every tag offset those functions read lands inside `Glow` in
`types/tags.h` and means the right thing there (`glow_flags` `0x28`, the distance bounds
`0x84..0x90`, `particle_size_bounds` `0xa0`, the colour bounds `0xb8..0xd0`, the trailing-particle
`t` bounds `0x108`/`0x10c`). None of them makes sense against `Lightning`, which is `0x108` bytes
in total. Consequently `0x008603a0` and `0x008603a4` are the glow instance and glow particle
arrays, not lightning arrays.

### `object_delete` was shadowed by a clone

`0x4f5bd0` is the real `object_delete`: 58 bytes, 27 callers, reading `object.network_role` and
dispatching to `object_delete_unparented` then `object_delete_recursive`. `object_apply_damage`
inlines the identical sequence at its tail. It had no file, because a zero-caller compiler clone
at `0x4f94e0` inherited the same Ghidra name and took `object_delete.c`. The clone is now
`object_update_functions_clone_4f94e0.c` and `0x4f5bd0` has been written.

Note for `out/phase4/objects_types_notes.md`: that file lists `0x4f5bd0` under
"`object_type_definition` ... category at `+0x04`". The `+0x04` dereference is off
`object_header.data`, so the field is `object.network_role`, not the definition's category.
`objects_delete_unparented_of_type_mask` `0x4f47c0` is the one that reads a definition `+0x04`.

### Other renames made by this review pass

| address | inherited name | now | why |
|---|---|---|---|
| `0x4f94e0` | `object_delete` | `object_update_functions_clone_4f94e0` | clone of `0x4f92f0`'s loop; shadowed the real `object_delete` |
| `0x4f93b0` | `object_delete_to_network` | `object_update_functions_clone_4f93b0` | same clone |
| `0x4f9540` | `object_reconnect_to_map` | `object_update_functions_clone_4f9540` | same clone |
| `0x4f84e2` | `objects_initialize_for_new_map_mod_processed_bsps` | `object_recalculate_bounding_radius_clone_4f84e2` | clone of `0x4f8310` |
| `0x4f8834` | `objects_update__object_in_player_pvs_nop1` | `object_recalculate_bounding_radius_clone_4f8834` | clone of `0x4f8310` |
| `0x4f8a70` | `object_reset` | `object_recalculate_bounding_radius_clone_4f8a70` | clone of `0x4f8310`; also collided with `objects_reset` `0x4f4bb0` |
| `0x4f8cb0` | `object_placement_data_new` | `object_set_position_network_clone_4f8cb0` | clone of `0x4f8bd0`; also collided with `object_placement_data_initialize` |
| `0x4efea0` | `mdp_decode_stateless_iterated` | `object_damage_notify_and_impulse_fragment_4efea0` | 8-byte tail fragment of `0x4efcf0` |
| `0x4efea8` | `switchD_004efdc8::caseD_0` | `object_damage_notify_and_impulse_fragment_4efea8` | outlined switch case of `0x4efcf0` |
| `0x4eff10` | `mdp_decode_incremental_iterated` | `object_damage_notify_and_impulse_fragment_4eff10` | outlined default case of `0x4efcf0` |
| `0x6391b4` | `FUN_006391b4` | `__ftol` | the disassembly is the MSVC 7.1 CRT `_ftol2` body |

Carried over from the earlier pass and still standing: `0x4f0250` is `object_damage_effect_dispatch`
(not `mdpi_encode`), `0x4f29c0` is `object_light_clear_dirty_flag` (not `chimera__light_table`),
`0x4f8207` is `object_function_evaluate_input`, `0x4fa280` is
`object_type_definitions_collect_by_flag_bits` (not a second `object_get_orientation`),
`0x4fb3e0` is `antenna_render_wire`, and `0x4ee5e0` really is `object_apply_damage`.

## Defects found and fixed by this review pass

These were real semantic drifts in the first-pass rewrites, each confirmed against
`python tools/pack.py` and, where Ghidra itself had elided an instruction, against
`objdump -d -M intel bin/halo.exe`.

1. **The BSP leaf -> cluster lookup, four files.** `structure_bsp_globals + 0xe4` holds a *pointer*
   to the `ScenarioStructureBSPLeaf` array, and the leaf index is masked with `0x7fffffff` before
   it is scaled by `0x10`. `objects_recompute_cluster_membership` (two sites),
   `object_set_cluster_and_parent`, `objects_set_ambient_cluster_override` and
   `object_collect_local_player_relevant_objects` all folded `0xe4` into a byte offset and dropped
   the mask, so they indexed a table that does not exist at an index that could be negative.
   (`antenna_apply_marker_delta`, `flag_pole_get_marker_positions` and
   `object_light_recompute_transform` already had it right.)
2. **`object_test_in_atmosphere_zone` overflowed a local.** It declared
   `object_get_root_parent_placement`'s out block as a single `int32_t *` and passed `&pair`, but
   the callee writes two dwords through that pointer - so the second write went past the local and
   the subsequent `pair[1]` read a global instead of the chain cursor. Fixed by giving the out
   block the real type (`object_placement_cursor`, now in `types/objects.h`).
3. **`object_test_in_atmosphere_zone`'s PVS bit test** read from `bsp_cluster_pvs_source` instead of
   `bsp_cluster_pvs_source + 0x18`, six dwords early. The same file also missed the
   `data_array.data` (`+0x34`) step when walking the cluster-reference chain.
4. **Six functions returned `int` for a value that only exists in `AL`.** Ghidra shows
   `CONCAT31(garbage, AL)` (or `bool`) for `object_datum_consume_pending_flag`,
   `object_is_delete_pending`, `object_type_definitions_query_0x28`,
   `object_type_definitions_query_0x34`, `object_restore_full_body_vitality` and
   `object_shield_recharge_start`; all six now return `uint8_t`, matching the correction already
   recorded for `object_type_definitions_query_0x44`.
5. **`object_delete_recursive`'s calling convention was wrong.** It was documented as EAX/ECX
   register arguments; the entry is `push ebx / mov ebx,[esp+8]` and the sibling flag is read at
   `0x4f5a09` as `mov al,[esp+0x18]`, so it is plain cdecl. Its callers agree.
6. **Four call sites lost a register argument.** `object_update_functions` and
   `object_update_change_colors` (EAX, from `object_new_with_datum_role_control`),
   `object_set_permutation_by_name` (EAX, from `object_destroy_region`) and
   `object_for_each_light_attachment` (EAX, from `object_delete_recursive`) were all called bare or
   short; the disassembly identifies the missing operand in each case.
7. **`object_get_root_object_index` was declared as taking an out-pointer.** It takes the object
   index in ECX; Ghidra had mis-attributed a neighbouring `push ecx` (which belongs to the
   following `FUN_00505880` call) to it.
8. **Two calls passed uninitialised or invented pointers.** `antenna_apply_marker_delta` and
   `flag_pole_get_marker_positions` passed `&scratch` (never written) to the leaf probe; the
   disassembly shows the real operands are `marker.node_transform.position` and
   `marker_positions[0]`.
9. **`FUN_004e9d40`'s arguments.** Both call sites set EAX to the literal `0x006870d8` and ESI to
   the object index; it had been declared as taking the object index alone.

Also folded in: the five file-local `typedef`s the first pass left behind (`antenna_marker_ref`,
`flag_marker_ref`, `light_leaf_reference`, `object_statistics`, `damage_effect_vector_block`) now
live in `types/objects.h`, and the first three collapsed into the single `bsp_leaf_reference` -
they are the same record, and `object.location_leaf_index` / `.location_cluster_index` is a fourth
copy of it.

## Known gaps

* **Twenty functions in the range have no file**, by design. Three are the hash table
  (`0x4f0530`, `0x4f05e0`, `0x4f0620`) - generic engine code whose *types* are declared in
  `types/objects.h` because nothing else owns them, but whose *bodies* were left for whichever
  module claims them. Three are interpolation helpers (`0x4f06d0`, `0x4fca60`, `0x4fcb00`) that
  belong with `types/math.h`. Four sample BSP lightmaps (`0x4f0730`, `0x4f0900`, `0x4f1ef0`,
  `0x4f2550`). Six read the packed control words at `0x006f1cec`/`0x006f1ce8` and belong to the
  input or game module (`0x4f3680`, `0x4f3700`, `0x4f37d0`, `0x4f3890`, `0x4f39d0`, `0x4f3ad0`).
  Three are Ghidra artefacts with no independent body (`0x4f3ef0`, `0x4f8dc0`, `0x4f9990`).
* **`object` `0x022..0x05b` and `0x188..0x1b7`** - 0x3a and 0x30 bytes that no function in this
  module reads or writes. The two largest holes in the record.
* **`object` `0x008`, `0x00c`, `0x0bc`, `0x0c8`, `0x108`, `0x170`** are written at create (mostly
  to -1) and never read here. `0x020` is touched only by
  `objects_update_player_visibility_masks`, so its width and meaning rest on one site.
* **`object` `0x178 region_vitality`** is a 0..255 accumulator compared against the region
  `damage_threshold`, but the quantity it accumulates is still unsettled. Now that `0x006391b4` is
  identified as `__ftol`, the byte is the truncation of some float - which float is the open part.
* **`object_type_definition` past `0xc4`** cannot be decided: the table at `0x0069bfdc` holds
  pointers, not inline records. Slots `0x60` and `0x78` are inside the vtable range but no function
  here calls them.
* **`glow` and `glow_particle` strides are lower bounds**, not proofs - their `data_array`s are
  created by `glow_initialize` `0x4fcbb0`, outside this module.
* **`light` `0x5c..0x77` is really a union**; the attached and positioned constructors write
  overlapping field sets. It is left flattened.
* **`damage_data` `0x34..0x3f`** is read by the knockback path but only ever written by callers
  outside this module, so the direction interpretation is unverified.
* **The object control binding tables** (`0x008603e0..0x008603fc`, `0x00860430`/`0x00860434`,
  `0x00860480`/`0x00860484`) are parallel arrays with bases 4 to 8 bytes apart, not one struct. No
  type is claimed; the addresses are recorded as globals only.
* **The light volume and lightning instance records** (`0x006b8d70`, `0x006b8d74`) are allocated
  outside this module and reached through a hash-slot lookup, so no layout is claimed.
* **Cross-module `extern` disagreements remain** for eleven symbols where `src/hs`, `src/math`,
  `src/memory` or `src/cache` declare the same address differently from `src/objects`:
  `FUN_00450870`, `FUN_00450980`, `FUN_00474db0`, `FUN_004f6080`, `FUN_005013a0`,
  `object_apply_damage`, `object_for_each_light_attachment`, `object_get_position`,
  `object_iterator_next`, `object_set_permutation_by_name`, `object_try_and_get`. In every case
  the `src/objects` declaration matches the definition in this module and the `src/hs` one does
  not; reconciling them means editing `src/hs`, which is out of this module's scope.
* **Ghidra sometimes elides instructions the binary really executes.** The `and eax,0x7fffffff`
  in the leaf lookup is present in every disassembly and absent from every decompile. Treat
  `tools/pack.py` output as a starting point, not as ground truth, for any arithmetic that looks
  suspiciously simple.

## Functions

`nc` is the file's own name confidence, `rc` its rewrite confidence, `?` the number of `UNSURE`
annotations it carries. `rc = n/a` marks the ten comment-only stubs (clones and outlined
fragments) that were deliberately not translated.

| Address | File | Size | nc | rc | ? |
|---|---|---|---|---|---|
| `0x4088e0` | `object_get_center_of_mass_and_scale` | 59 | 0.6 | 0.85 |  |
| `0x4ed440` | `object_initialize_shield_stun_thresholds` | 198 | 0.7 | 0.8 |  |
| `0x4ed510` | `object_update_vitality_and_regeneration` | 1152 | 0.5 | 0.35 | 10 |
| `0x4ed990` | `damage_data_initialize` | 52 | 0.6 | 0.9 |  |
| `0x4ed9d0` | `object_restore_full_body_vitality` | 80 | 0.35 | 0.6 |  |
| `0x4eda20` | `object_set_health_frozen_flag` | 231 | 0.35 | 0.45 | 6 |
| `0x4edb10` | `object_set_shield_depleted_flag` | 136 | 0.4 | 0.55 | 2 |
| `0x4edba0` | `object_shield_recharge_start` | 101 | 0.75 | 0.6 | 2 |
| `0x4edc10` | `object_children_recurse_prune` | 101 | 0.6 | 0.55 |  |
| `0x4edc80` | `object_delete_teardown` | 166 | 0.35 | 0.5 | 4 |
| `0x4edd30` | `damage_apply_area_effect` | 119 | 0.5 | 0.55 | 2 |
| `0x4eddb0` | `object_damage_apply_line_of_sight` | 1294 | 0.5 | 0.3 | 6 |
| `0x4ee2e0` | `object_get_controlling_player_index` | 135 | 0.5 | 0.55 | 2 |
| `0x4ee370` | `object_throttled_multiplayer_sound_event` | 67 | 0.25 | 0.4 | 3 |
| `0x4ee3c0` | `object_notify_pickup_or_refresh_probe` | 260 | 0.3 | 0.3 | 4 |
| `0x4ee4d0` | `object_apply_shield_charge_and_notify` | 260 | 0.3 | 0.25 | 11 |
| `0x4ee5e0` | `object_apply_damage` | 2939 | 0.75 | 0.2 | 34 |
| `0x4ef160` | `object_hash_clear_flag_bit3` | 149 | 0.5 | 0.4 | 3 |
| `0x4ef200` | `object_hash_set_flag_bit3` | 149 | 0.5 | 0.4 | 3 |
| `0x4ef2a0` | `object_apply_body_damage` | 1403 | 0.6 | 0.25 | 13 |
| `0x4ef820` | `object_apply_shield_damage` | 976 | 0.6 | 0.25 | 5 |
| `0x4efbf0` | `object_queue_pickup_denied_event` | 136 | 0.3 | 0.25 | 6 |
| `0x4efc80` | `object_apply_linked_impulse` | 112 | 0.3 | 0.25 | 9 |
| `0x4efcf0` | `object_damage_notify_and_impulse` | 432 | 0.5 | 0.25 | 12 |
| `0x4efea0` | `object_damage_notify_and_impulse_fragment_4efea0` | 8 | 0.1 | n/a |  |
| `0x4efea8` | `object_damage_notify_and_impulse_fragment_4efea8` | 104 | 0.1 | n/a |  |
| `0x4eff10` | `object_damage_notify_and_impulse_fragment_4eff10` | 199 | 0.1 | n/a |  |
| `0x4efff0` | `object_dispatch_effect_notify` | 20 | 0.2 | 0.3 |  |
| `0x4f0010` | `damage_effect_new_at_location` | 640 | 0.9 | 0.25 | 4 |
| `0x4f0250` | `object_damage_effect_dispatch` | 56 | 0.2 | 0.3 |  |
| `0x4f02d0` | `object_destroy_region` | 268 | 0.75 | 0.7 | 4 |
| `0x4f03e0` | `object_regions_reset_permutation_lock` | 132 | 0.7 | 0.6 |  |
| `0x4f0a20` | `lights_initialize` | 115 | 0.7 | 0.45 | 6 |
| `0x4f0aa0` | `lights_dispose_all` | 76 | 0.55 | 0.5 | 4 |
| `0x4f0af0` | `light_new_attached` | 217 | 0.75 | 0.55 | 3 |
| `0x4f0bd0` | `light_delete` | 51 | 0.35 | 0.75 |  |
| `0x4f0c10` | `light_new_positioned` | 215 | 0.75 | 0.4 | 4 |
| `0x4f0cf0` | `object_lights_update_all` | 1591 | 0.5 | 0.15 | 25 |
| `0x4f133c` | `object_lights_update_all_continued` | 694 | 0.2 | 0.1 | 10 |
| `0x4f1600` | `light_transient_add` | 253 | 0.7 | 0.55 | 6 |
| `0x4f1700` | `light_collect_object_references` | 125 | 0.3 | 0.4 |  |
| `0x4f1780` | `lights_apply_spot_falloff` | 453 | 0.6 | 0.35 | 9 |
| `0x4f1950` | `lights_apply_spot_falloff_specular` | 474 | 0.35 | 0.3 | 9 |
| `0x4f1b30` | `object_sum_attached_light_luminance` | 226 | 0.45 | 0.45 |  |
| `0x4f1c20` | `object_sample_total_lighting_at_point` | 571 | 0.35 | 0.3 | 3 |
| `0x4f1e60` | `object_sample_ambient_lightmap_point` | 144 | 0.3 | 0.2 | 8 |
| `0x4f20b0` | `object_sample_ambient_lighting` | 892 | 0.9 | 0.75 | 4 |
| `0x4f2430` | `object_gather_light_list` | 280 | 0.3 | 0.25 | 5 |
| `0x4f29c0` | `object_light_clear_dirty_flag` | 58 | 0.7 | 0.65 | 3 |
| `0x4f2a00` | `object_light_recompute_transform` | 674 | 0.6 | 0.6 | 4 |
| `0x4f2d50` | `object_lights_refresh_transforms` | 157 | 0.7 | 0.6 |  |
| `0x4f2df0` | `object_lights_gather_nearest` | 503 | 0.75 | 0.4 | 1 |
| `0x4f2ff0` | `object_build_effect_parameter_block` | 1049 | 0.3 | 0.2 | 8 |
| `0x4f3410` | `object_color_clamp_to_intensity` | 172 | 0.4 | 0.65 |  |
| `0x4f3ba0` | `objects_update_control_bindings` | 521 | 0.3 | 0.35 | 6 |
| `0x4f3db0` | `object_type_definition_chain_build` | 120 | 0.8 | 0.85 |  |
| `0x4f3e30` | `object_type_definitions_notify_0x24` | 106 | 0.6 | 0.75 | 1 |
| `0x4f3ea0` | `object_type_definitions_query_0x28` | 80 | 0.6 | 0.8 |  |
| `0x4f3f20` | `object_type_definitions_notify_two_args_0x2c` | 110 | 0.6 | 0.8 |  |
| `0x4f3f90` | `object_type_definitions_notify_0x30` | 104 | 0.6 | 0.75 |  |
| `0x4f4000` | `object_type_definitions_query_0x34` | 115 | 0.6 | 0.8 |  |
| `0x4f4080` | `object_type_definitions_notify_0x38` | 104 | 0.6 | 0.75 |  |
| `0x4f40f0` | `object_type_definitions_notify_0x3c` | 106 | 0.6 | 0.75 |  |
| `0x4f4160` | `object_type_definitions_notify_region_damage` | 111 | 0.55 | 0.7 | 1 |
| `0x4f41d0` | `object_type_definitions_query_0x44` | 115 | 0.6 | 0.8 |  |
| `0x4f4250` | `object_type_definitions_notify_two_args_0x48` | 110 | 0.6 | 0.8 |  |
| `0x4f42c0` | `object_type_definitions_notify_0x4c` | 106 | 0.6 | 0.75 |  |
| `0x4f4330` | `object_type_definitions_notify_0x50` | 104 | 0.6 | 0.75 |  |
| `0x4f43a0` | `object_type_definitions_notify_0x54` | 104 | 0.6 | 0.75 |  |
| `0x4f4410` | `object_type_definitions_notify_0x5c` | 104 | 0.6 | 0.75 |  |
| `0x4f4480` | `object_type_definitions_notify_0x58` | 111 | 0.6 | 0.75 |  |
| `0x4f44f0` | `object_type_override_get_0x64` | 97 | 0.6 | 0.7 |  |
| `0x4f4560` | `object_type_override_call_0x68` | 74 | 0.6 | 0.7 |  |
| `0x4f45b0` | `object_type_override_call_0x6c` | 103 | 0.6 | 0.75 |  |
| `0x4f4620` | `object_type_override_call_0x70` | 87 | 0.6 | 0.65 | 1 |
| `0x4f4680` | `object_type_override_call_0x70_release_node` | 46 | 0.3 | 0.4 | 5 |
| `0x4f46b0` | `object_datum_consume_pending_flag` | 78 | 0.45 | 0.55 | 1 |
| `0x4f4700` | `object_type_override_call_0x74` | 88 | 0.6 | 0.75 |  |
| `0x4f4760` | `object_type_override_call_0x7c` | 83 | 0.6 | 0.75 |  |
| `0x4f47c0` | `objects_delete_unparented_of_type_mask` | 149 | 0.75 | 0.6 | 1 |
| `0x4f4880` | `objects_update_player_visibility_masks` | 588 | 0.75 | 0.3 | 6 |
| `0x4f4ad0` | `objects_initialize` | 212 | 0.9 | 0.75 | 3 |
| `0x4f4bb0` | `objects_reset` | 261 | 0.9 | 0.75 | 4 |
| `0x4f4cc0` | `objects_flush_dirty_state` | 237 | 0.85 | 0.6 | 2 |
| `0x4f4db0` | `objects_dispose` | 217 | 0.9 | 0.7 |  |
| `0x4f4e90` | `objects_update` | 592 | 0.7 | 0.4 | 6 |
| `0x4f50f0` | `object_mark_pending_delete` | 57 | 0.7 | 0.75 |  |
| `0x4f5130` | `object_clear_pending_delete_flag` | 33 | 0.7 | 0.8 |  |
| `0x4f5160` | `object_reset_velocity_and_wake` | 92 | 0.4 | 0.6 |  |
| `0x4f51c0` | `object_set_position_and_orientation` | 248 | 0.55 | 0.8 | 1 |
| `0x4f52c0` | `object_set_position_and_recalculate` | 139 | 0.3 | 0.45 | 3 |
| `0x4f5350` | `object_set_position_and_relink` | 66 | 0.3 | 0.5 | 1 |
| `0x4f53a0` | `object_placement_data_initialize` | 190 | 0.85 | 0.75 |  |
| `0x4f5460` | `object_new` | 67 | 0.35 | 0.3 | 2 |
| `0x4f54b0` | `object_new_with_datum_role_control` | 1308 | 0.9 | 0.55 | 25 |
| `0x4f59d0` | `object_delete_recursive` | 197 | 0.75 | 0.7 |  |
| `0x4f5aa0` | `object_delete_unparented` | 171 | 0.4 | 0.5 | 4 |
| `0x4f5b50` | `object_delete_by_pooled_node_id` | 117 | 0.3 | 0.4 | 6 |
| `0x4f5bd0` | `object_delete` | 58 | 0.70 | 0.75 |  |
| `0x4f5c10` | `object_is_delete_pending` | 27 | 0.7 | 0.85 |  |
| `0x4f5c30` | `object_set_cluster_and_parent` | 432 | 0.4 | 0.5 | 6 |
| `0x4f5de0` | `object_unlink_cluster_or_notify_parent` | 172 | 0.5 | 0.45 | 5 |
| `0x4f5f00` | `object_resolve_collideable_reference` | 57 | 0.3 | 0.5 |  |
| `0x4f5f70` | `object_get_root_parent_placement` | 137 | 0.75 | 0.35 | 3 |
| `0x4f6000` | `object_get_node_marker_address` | 41 | 0.85 | 0.8 |  |
| `0x4f6030` | `object_get_attachment_marker_name` | 74 | 0.3 | 0.45 |  |
| `0x4f6080` | `object_get_node_local_transform` | 252 | 0.85 | 0.5 | 3 |
| `0x4f6180` | `object_reorient_relative_to_marker` | 358 | 0.55 | 0.75 |  |
| `0x4f62f0` | `object_recompute_basis_from_marker_delta` | 332 | 0.4 | 0.75 | 2 |
| `0x4f6440` | `object_attach_to_object` | 451 | 0.85 | 0.8 |  |
| `0x4f6610` | `object_snap_to_parent_marker_and_detach` | 453 | 0.85 | 0.65 |  |
| `0x4f67e0` | `object_set_in_pvs_pass_flag` | 106 | 0.35 | 0.6 | 1 |
| `0x4f6850` | `object_set_collision_enabled` | 163 | 0.4 | 0.55 | 2 |
| `0x4f6900` | `object_get_position` | 107 | 0.9 | 0.85 |  |
| `0x4f6970` | `object_get_orientation` | 167 | 0.9 | 0.85 |  |
| `0x4f6a20` | `object_get_world_matrix` | 127 | 0.9 | 0.7 |  |
| `0x4f6aa0` | `object_get_root_object_velocities` | 105 | 0.85 | 0.75 |  |
| `0x4f6b10` | `object_get_root_location` | 81 | 0.4 | 0.65 | 1 |
| `0x4f6b70` | `object_copy_default_node_transforms` | 146 | 0.85 | 0.6 | 1 |
| `0x4f6c10` | `object_offset_node_translation` | 68 | 0.85 | 0.6 | 1 |
| `0x4f6c60` | `object_set_permutation_by_name` | 240 | 0.8 | 0.55 | 1 |
| `0x4f6d60` | `object_solve_two_bone_ik_to_marker` | 265 | 0.25 | 0.3 | 5 |
| `0x4f6e70` | `object_function_get_value` | 70 | 0.85 | 0.75 |  |
| `0x4f6ec0` | `object_try_and_get` | 92 | 0.85 | 0.85 |  |
| `0x4f6f20` | `object_iterator_next` | 129 | 0.85 | 0.75 |  |
| `0x4f6fb0` | `object_get_root_object_index` | 43 | 0.85 | 0.85 |  |
| `0x4f6fe0` | `object_find_in_sphere` | 400 | 0.85 | 0.5 | 5 |
| `0x4f7180` | `object_collect_in_clusters` | 477 | 0.8 | 0.55 | 1 |
| `0x4f7370` | `object_refresh_local_player_render_cache` | 72 | 0.2 | 0.3 | 4 |
| `0x4f73c0` | `object_lookup_table_get` | 28 | 0.8 | 0.85 |  |
| `0x4f73e0` | `object_clear_references_to_object` | 112 | 0.85 | 0.45 | 2 |
| `0x4f7450` | `object_list_membership_set` | 150 | 0.85 | 0.6 |  |
| `0x4f74f0` | `object_sweep_refresh_cluster_membership` | 123 | 0.35 | 0.5 |  |
| `0x4f7570` | `objects_recompute_cluster_membership` | 353 | 0.85 | 0.45 | 5 |
| `0x4f76e0` | `object_test_in_atmosphere_zone` | 607 | 0.3 | 0.45 | 11 |
| `0x4f7950` | `objects_get_statistics` | 120 | 0.35 | 0.55 | 1 |
| `0x4f79d0` | `objects_set_ambient_cluster_override` | 115 | 0.85 | 0.55 | 3 |
| `0x4f7a50` | `objects_get_ambient_cluster` | 125 | 0.85 | 0.55 | 1 |
| `0x4f7ad0` | `object_notify_predicted_resources_if_valid` | 37 | 0.25 | 0.4 | 1 |
| `0x4f7b00` | `object_notify_children_recursive` | 99 | 0.35 | 0.5 |  |
| `0x4f7b70` | `object_reposition_to_spawn_location` | 195 | 0.3 | 0.3 | 3 |
| `0x4f7c40` | `object_nudge_position_by_velocity` | 270 | 0.25 | 0.25 | 3 |
| `0x4f7d50` | `object_block_data_new` | 130 | 0.85 | 0.6 |  |
| `0x4f7de0` | `object_block_data_free` | 105 | 0.5 | 0.6 |  |
| `0x4f7e50` | `object_block_data_grow` | 156 | 0.85 | 0.55 | 2 |
| `0x4f7ef0` | `object_update` | 465 | 0.85 | 0.45 | 7 |
| `0x4f8207` | `object_function_evaluate_input` | 347 | 0.4 | 0.15 | 6 |
| `0x4f82b0` | `object_recalculate_bounding_radius_recursive` | 96 | 0.85 | 0.7 |  |
| `0x4f8310` | `object_recalculate_bounding_radius` | 485 | 0.85 | 0.15 | 11 |
| `0x4f84e2` | `object_recalculate_bounding_radius_clone_4f84e2` | 850 | 0.1 | n/a |  |
| `0x4f8834` | `object_recalculate_bounding_radius_clone_4f8834` | 553 | 0.1 | n/a |  |
| `0x4f8a70` | `object_recalculate_bounding_radius_clone_4f8a70` | 148 | 0.1 | n/a |  |
| `0x4f8b10` | `object_notify_node_array_if_animated` | 82 | 0.3 | 0.5 | 2 |
| `0x4f8b70` | `object_apply_network_placement` | 86 | 0.3 | 0.5 | 1 |
| `0x4f8bd0` | `object_set_position_network` | 234 | 0.8 | 0.3 | 2 |
| `0x4f8cb0` | `object_set_position_network_clone_4f8cb0` | 176 | 0.2 | n/a |  |
| `0x4f8d80` | `object_permutation_find_matching_group` | 64 | 0.4 | 0.6 | 1 |
| `0x4f8dd0` | `object_regions_initialize_permutations` | 284 | 0.85 | 0.5 | 2 |
| `0x4f8ef0` | `object_get_first_region_probability_group` | 93 | 0.35 | 0.55 |  |
| `0x4f8f50` | `object_refresh_region_permutations` | 143 | 0.35 | 0.4 | 2 |
| `0x4f8fe0` | `object_remove_from_sibling_list` | 69 | 0.85 | 0.55 | 1 |
| `0x4f9030` | `object_delete_4f9030` | 210 | 0.65 | 0.6 | 4 |
| `0x4f9110` | `object_update_change_colors` | 480 | 0.85 | 0.5 |  |
| `0x4f92f0` | `object_update_functions` | 185 | 0.85 | 0.4 | 4 |
| `0x4f93b0` | `object_update_functions_clone_4f93b0` | 314 | 0.1 | n/a |  |
| `0x4f94e0` | `object_update_functions_clone_4f94e0` | 86 | 0.1 | n/a |  |
| `0x4f9540` | `object_update_functions_clone_4f9540` | 337 | 0.1 | n/a |  |
| `0x4f96a0` | `object_set_scale_and_refresh_nodes` | 70 | 0.25 | 0.3 | 2 |
| `0x4f96f0` | `object_disconnect_from_map` | 39 | 0.6 | 0.7 | 1 |
| `0x4f9750` | `object_create_attachments` | 412 | 0.85 | 0.45 | 7 |
| `0x4f9900` | `object_delete_attachments` | 214 | 0.65 | 0.4 | 5 |
| `0x4f9a20` | `object_for_each_light_attachment` | 146 | 0.85 | 0.55 | 2 |
| `0x4f9ac0` | `object_reserve_render_cache_slot` | 54 | 0.85 | 0.7 |  |
| `0x4f9b00` | `object_release_render_cache_slot` | 99 | 0.85 | 0.5 | 2 |
| `0x4f9b70` | `object_get_or_build_render_permutation` | 234 | 0.3 | 0.2 | 6 |
| `0x4f9c60` | `objects_garbage_collection` | 1149 | 0.9 | 0.2 | 6 |
| `0x4fa0f0` | `object_tree_collect_matching` | 168 | 0.85 | 0.75 |  |
| `0x4fa1a0` | `object_collect_local_player_relevant_objects` | 233 | 0.25 | 0.3 | 4 |
| `0x4fa280` | `object_type_definitions_collect_by_flag_bits` | 272 | 0.2 | 0.1 | 3 |
| `0x4fa3a0` | `object_dump_compare_by_total_size` | 35 | 0.6 | 0.85 |  |
| `0x4fa3d0` | `object_dump_accumulate_stats` | 185 | 0.65 | 0.6 |  |
| `0x4fa490` | `object_dump_write` | 111 | 0.8 | 0.65 |  |
| `0x4fa500` | `objects_dump_memory` | 725 | 0.85 | 0.25 | 4 |
| `0x4fa8d0` | `object_start_animation` | 214 | 0.55 | 0.3 | 4 |
| `0x4fa9b0` | `object_animation_get_frames_remaining` | 101 | 0.55 | 0.4 | 2 |
| `0x4faa20` | `antennas_initialize` | 28 | 0.65 | 0.85 |  |
| `0x4faa40` | `antennas_dispose` | 18 | 0.55 | 0.75 |  |
| `0x4faa60` | `antennas_clear_disposing_flag` | 10 | 0.6 | 0.85 |  |
| `0x4faa70` | `antennas_reset_data_pointer` | 20 | 0.5 | 0.7 |  |
| `0x4faa90` | `antenna_new` | 487 | 0.55 | 0.35 | 4 |
| `0x4fad20` | `antennas_update` | 235 | 0.55 | 0.4 | 1 |
| `0x4fae10` | `antenna_update_physics` | 943 | 0.6 | 0.35 | 6 |
| `0x4fb1c0` | `antenna_apply_marker_delta` | 378 | 0.55 | 0.7 | 1 |
| `0x4fb3e0` | `antenna_render_wire` | 228 | 0.4 | 0.2 | 7 |
| `0x4fb4d0` | `flags_initialize` | 28 | 0.7 | 0.8 |  |
| `0x4fb4f0` | `flags_dispose` | 18 | 0.6 | 0.75 | 2 |
| `0x4fb510` | `flags_clear_disposing_flag` | 10 | 0.5 | 0.8 |  |
| `0x4fb520` | `flags_reset_data_pointer` | 20 | 0.45 | 0.75 |  |
| `0x4fb540` | `flag_new` | 391 | 0.55 | 0.55 | 2 |
| `0x4fb6d0` | `flag_cloth_mark_border_cells` | 159 | 0.5 | 0.5 | 1 |
| `0x4fb770` | `flag_cloth_init_shape_constraints` | 207 | 0.75 | 0.5 |  |
| `0x4fb840` | `flag_cloth_stamp_region_split_flags` | 290 | 0.45 | 0.55 |  |
| `0x4fba00` | `flags_update` | 210 | 0.55 | 0.5 | 3 |
| `0x4fbae0` | `flag_cloth_update` | 1340 | 0.5 | 0.3 | 6 |
| `0x4fc020` | `flag_pole_get_marker_positions` | 805 | 0.7 | 0.4 | 2 |
| `0x4fc350` | `flag_render` | 1775 | 0.55 | 0.15 | 10 |
| `0x4fcdb0` | `glow_render_dispatch` | 199 | 0.7 | 0.55 | 2 |
| `0x4fce80` | `glow_update` | 1300 | 0.7 | 0.2 | 12 |
| `0x4fd3a0` | `glow_particle_compute_fade` | 121 | 0.7 | 0.55 |  |
| `0x4fd420` | `glow_particle_compute_color` | 128 | 0.7 | 0.6 |  |
| `0x4fd4a0` | `glow_particle_compute_position` | 431 | 0.7 | 0.25 | 1 |
| `0x4fd650` | `glow_particle_advance_time` | 469 | 0.7 | 0.35 | 1 |
| `0x4fd830` | `glow_chain_build` | 166 | 0.7 | 0.5 |  |
| `0x4fd8e0` | `glow_particle_new` | 572 | 0.7 | 0.4 |  |
| `0x4fdb20` | `glow_particle_spawn` | 699 | 0.7 | 0.3 | 1 |
| `0x4fdde0` | `glow_particle_datum_new` | 84 | 0.7 | 0.6 | 1 |
| `0x4fde40` | `glow_particle_reposition` | 1832 | 0.7 | 0.15 | 1 |
| `0x4fe570` | `glow_render` | 259 | 0.7 | 0.5 | 1 |
| `0x4fe740` | `object_attachment_get_blended_marker` | 443 | 0.75 | 0.3 | 1 |
| `0x4fe900` | `light_volume_render` | 326 | 0.7 | 0.3 | 6 |
| `0x4fea50` | `curve_apply_exponent` | 33 | 0.35 | 0.7 |  |
| `0x4fef40` | `antenna_tip_jitter` | 197 | 0.4 | 0.3 | 4 |
| `0x4ff010` | `lightning_render` | 2476 | 0.7 | 0.1 | 11 |
| `0x4ff9d0` | `widgets_initialize` | 56 | 0.9 | 0.85 |  |
| `0x4ffa10` | `widgets_dispose` | 46 | 0.85 | 0.85 | 2 |
| `0x4ffa50` | `widgets_dispose_clear_flag` | 42 | 0.7 | 0.8 |  |
| `0x4ffa80` | `widget_new` | 334 | 0.85 | 0.55 |  |
| `0x4ffbe0` | `widget_delete_all` | 122 | 0.85 | 0.55 |  |
| `0x4ffc60` | `widget_list_has_flag` | 64 | 0.8 | 0.7 |  |
| `0x4ffca0` | `widget_list_notify` | 106 | 0.8 | 0.55 |  |
| `0x4ffd10` | `widgets_update_all` | 39 | 0.85 | 0.8 |  |
| `0x4ffd40` | `zone_light_table_initialize` | 90 | 0.75 | 0.85 |  |
| `0x4ffda0` | `zone_light_table_test_bit` | 52 | 0.75 | 0.6 |  |
