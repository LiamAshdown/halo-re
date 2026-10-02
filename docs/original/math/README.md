# `math` — Blam scalar, vector, matrix, quaternion and geometry primitives

Retail Halo PC `halo.exe` 1.0.10, `0x401050 .. 0x4cf7a0` (21,580 bytes of code, 99 functions),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/math.h`, with the original decompile preserved verbatim at the
bottom of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py math` → **99 ok, 0 failed**.

Seven of the functions (`0x401050`–`0x4088b0`) sit far below the rest of the module; they are the
handful of leaf helpers the linker pulled forward into the hot part of `.text`. Nine more
(`0x4052c0`–`0x43c400`) sit inside the AI module's address range: generic helpers (a cross
product, an integer RNG, two qsort comparators, four small 2D/3D geometry tests) that LTCG placed
next to their AI callers. The other 83 are contiguous from `0x4ca4b0`.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| random numbers | `0x401050`, `0x4019f0`, `0x405320`, `0x4cd070`–`0x4cd1b0` | one 32-bit LCG, two streams, plus a `QueryPerformanceCounter`-based seeder |
| qsort comparators | `0x405360`, `0x433c70` | `float_compare_ascending` and `object_sort_by_flag_then_distance`, both `__cdecl (const void *, const void *)` |
| AI-range geometry helpers | `0x4052c0`, `0x414910`, `0x43b2f0`, `0x43c340`–`0x43c400` | cross product, horizontal cone test, point-on-segment, radius test, 2D ray/circle, circle tangents |
| vector / point leaf math | `0x401930`–`0x4088b0`, `0x4cd2e0`–`0x4cdd40` | length, normalize, lerp, project, angle-between, rotate-about-axis, quaternions |
| sphere mesh | `0x4ca4b0`–`0x4ca9a0` | subdivided octahedron; feeds the 1026-entry quasi-uniform direction table |
| 2D/3D polygon clipping and hull | `0x4caa40`–`0x4cb740` | Sutherland–Hodgman clip, gift-wrapping hull, containment tests |
| `real_matrix4x3` | `0x4cb7a0`–`0x4cc4ff` | build / invert / multiply / transform, with a scalar, an SSE and a 3DNow! multiply |
| `real_matrix3x3` and quaternions | `0x4cc500`–`0x4cc8cf`, `0x4cdb20`–`0x4cddd0` | the rotation-only counterparts and the quaternion conversions |
| periodic and transition functions | `0x4cc8d0`–`0x4cd06f` | twelve wave tables and six easing tables, 1024 bytes each |
| ray / segment / plane queries | `0x4cde30`–`0x4cf35f` | sphere and cylinder intersection, segment-to-segment distance, plane intersections |
| angular servos | `0x4cd950`, `0x4cf360`, `0x4cf530` | seek-toward helpers with bounded velocity and acceleration |

Nothing here allocates from the Blam heap: the sphere mesh and the wave tables call `GlobalAlloc`
directly, and everything else works on caller-supplied storage or the stack.

## Struct layouts

All of these live in `types/math.h`; the tables below are the summary. `#pragma pack(push,1)` is
in force, so every offset is exact. `real` is `float`.

### Geometric primitives

| Type | Size | Fields |
|---|---|---|
| `real_vector2d` | `0x08` | `i`, `j` |
| `real_point2d` | `0x08` | `x`, `y` |
| `real_vector3d` | `0x0c` | `i`, `j`, `k` |
| `real_point3d` | `0x0c` | `x`, `y`, `z` |
| `real_plane2d` | `0x0c` | `real_vector2d normal` @ `0x00`, `real d` @ `0x08`; the plane is `i*x + j*y == d` |
| `real_plane3d` | `0x10` | `real_vector3d normal` @ `0x00`, `real d` @ `0x0c` |
| `real_quaternion` | `0x10` | `i`, `j`, `k`, `w` (scalar part last — `quaternion_normalize` writes `{0,0,0,1}` for a degenerate input) |
| `real_euler_angles2d` | `0x08` | `yaw`, `pitch` |
| `real_euler_angles3d` | `0x0c` | `yaw`, `pitch`, `roll` (order fixed by `euler_angles_to_basis_vectors` @ `0x4cdde0`) |
| `real_bounds` | `0x08` | `lower`, `upper` |
| `real_rectangle3d` | `0x18` | `real_bounds x`, `y`, `z` |

A `real_plane2d` is almost never passed in by a caller: both `polygon2d_points_classify` and
`polygon2d_clip_to_planes` build every plane they use from two **points**, through
`plane2d_from_points` @ `0x44d950` (out of this module, `ECX` = out plane, `EAX` = `a`,
`EDX` = `b`): `normal = (a.y - b.y, b.x - a.x)`, normalized, `d = normal · b`, returning NULL when
the points are within `0.0001` of each other.

### `real_matrix3x3` — size `0x24`

| Off | Type | Field |
|---|---|---|
| `0x00` | `real_vector3d` | `forward` |
| `0x0c` | `real_vector3d` | `left` |
| `0x18` | `real_vector3d` | `up` |

The rows are the basis vectors; `matrix3x3_inverse_transform_vector` computes
`out.i = m[0]*v.i + m[3]*v.j + m[6]*v.k`, i.e. the transpose, which is the inverse for an
orthonormal basis.

### `real_matrix4x3` — size `0x34`

| Off | Type | Field |
|---|---|---|
| `0x00` | `real` | `scale` — uniform; `0` marks the matrix invalid, and `matrix4x3_transform_point` skips the pre-multiply when it is exactly `1` |
| `0x04` | `real_vector3d` | `forward` |
| `0x10` | `real_vector3d` | `left` |
| `0x1c` | `real_vector3d` | `up` |
| `0x28` | `real_point3d` | `position` |

Thirteen floats: `matrix4x3_inverse` zero-fills 13 on a degenerate matrix and `matrix4x3_multiply`
spills 13 when the destination aliases an operand. The scale is kept out of the rotation rows, so
those stay orthonormal.

### `sphere_mesh` — size `0x14`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `subdivisions` (`n`; arrives in AX) |
| `0x02` | `int16_t` | `unknown_02` — never written |
| `0x04` | `real_point3d *` | `points`, `point_count` unit vectors |
| `0x08` | `int16_t *` | `indices`, run-length-encoded triangle strips |
| `0x0c` | `int16_t` | `point_count` = `4*(n-2)*(n-1) + 12n - 6` |
| `0x0e` | `int16_t` | `triangle_count` = `8*n*n` |
| `0x10` | `int16_t` | `strip_count` = `8*n` |
| `0x12` | `int16_t` | `unknown_12` — never written |

Each row of a face emits a run length (`2*row + 1`) followed by that many point indices.
`sphere_point_table_init` @ `0x4cd0e0` calls the generator with `n = 16`, giving 1026 points and
2048 triangles. Scratch tables: `sphere_mesh_edge_cache` (`int16_t[8][8]`, `0x80` bytes, keyed by
the two base vertex indices) and `sphere_mesh_face_cache` (`(n+1)*(n+1)` `int16_t`, indexed
`[(n+1)*row + column]`), both filled with `0xffff` and freed before returning.

### `periodic_function_table` — size `0x400`

1024 bytes, one per wave. `periodic_function_tables[12]` at `0x006b7aa8`,
`transition_function_tables[6]` at `0x006b7ad8`, `periodic_functions_initialized` at `0x006b7af0`.

How an entry is produced is **not** simply `value * 255`, and this rewrite corrects the earlier
reading:

* **periodic tables** (`periodic_function_build_table` @ `0x4ccdb0`) sample the wave at 1024
  phases while tracking the running minimum and maximum, then emit
  `(value - minimum) / (maximum - minimum) * 255`. The two *slide* waves (types 6 and 7) are
  exempted by the same `(1 << type) & 0xc0` mask the evaluator uses, because they are already in
  `[0,1)`. A range of exactly zero skips the division, which is what keeps the constant tables
  (`_periodic_function_one`, `_periodic_function_zero`) from dividing by zero.
* **transition tables** (`periodic_function_build_transition_table` @ `0x4cccb0`) are not
  normalized: entry `i` is `curve(i / 1023) * 255`, and the evaluator samples at `phase * 1023`,
  special-casing index `0x3ff` so the lerp never reads `samples[1024]`.

Both quantizers finish with `__ftol` (`0x6391b4`) and clamp to `0..255`.

The twelve `periodic_function` cases line up with the tag-side `WaveFunction` enum, and the six
`transition_function` cases with `FunctionType`; the disassembly settles both orders. The four
`_variable_period` / `spark` cases (3, 5, 7, 11) are exactly the ones that read the noise-warped
phase `noise[i] * 28` instead of the linear phase `i * 28/1024`, and the six easing curves are
literally `t`, `t^0.5`, `t^0.25`, `t^2`, `t^4`, `(sin(t*pi - pi/2) + 1) / 2`.

### Other tables

| Global | Type | Notes |
|---|---|---|
| `0x0065c190` | `real_point3d k_octahedron_vertices[6]` | `(0,0,1) (0,1,0) (1,0,0) (0,-1,0) (-1,0,0) (0,0,-1)` |
| `0x0065c1d8` | `int16_t k_octahedron_faces[8][3]` | walked as 8 triples from `0x65c1da` using `[edi-2] [edi] [edi+2]` |
| `0x0065c208`.. | identity / negative-identity matrices, basis vectors, origin, identity quaternion, null rectangle | reached through the pointer table at `0x006966d0` |
| `0x0065c29c` | `projection_axis_pair k_projection_axes[6]` | `{2,1} {1,2} {0,2} {2,0} {1,0} {0,1}`, indexed `[dominant_axis*2 + (component > 0)]` |
| `0x0069665c`, `0x00696668` | `int16_t[3]` `{1,2,0}` | the quaternion-extraction next-index tables, duplicated once per function |
| `0x00696664` | `void (*matrix4x3_multiply_procedure)(...)` | default `0x4cc0d0` scalar, SSE `0x4cc250`, 3DNow! `0x4cc3a0` |
| `0x00719cd0`, `0x00719cd4` | `random_seed` | the deterministic (`random_seed_global`) and non-deterministic (`local_random_seed`) LCG streams |
| `0x007196f4` | `int32_t safe_mode` | shell command-line BOOL (`types/shell.h`); `math_initialize` keeps the scalar matrix multiply when it is set |
| `0x00721e90`, `0x00721e94` | `char **shell_argv`, `int32_t shell_argc` | read by `math_initialize` for `-noSSE`; owned by the shell module |

### Records borrowed from other headers

| Type | Header | Size | Used by |
|---|---|---|---|
| `ai_nearby_actor_candidate` | `types/ai.h` | `0x0c` | `object_sort_by_flag_then_distance` @ `0x433c70`: `actor_index` @ `0x00`, `distance_squared` @ `0x04` (secondary key, ascending), `is_type_9` @ `0x08` (primary key, clear before set), `pad[3]` |

The comparator's rewriter first declared a private copy of this layout as a `TYPES-GAP`. It was not
folded into `types/math.h`, because `types/ai.h` already defines the same 12-byte record for the
only caller (`ai_object_process_nearby_actors` @ `0x433cc0`), and one layout should have one
definition. The comparator now includes `ai.h` and takes `const void *`.
| `0x006b7af4`, `0x006b7af8` | `real_point3d *`, `int16_t` | the 1026-entry sphere point table and its count |

## Misattributed functions

1. **`0x4cf530` is not `vector3d_random_point_in_cone`.** Renamed to
   `vector3d_rotate_toward_with_acceleration`. The function contains no randomness at all — it
   never touches either seed global and has no RNG callee. It is a bounded-acceleration angular
   servo: the speed it wants is `min(sqrt(2 * acceleration * angle_remaining), maximum_velocity)`
   carried on the axis `direction × target_direction`, the stored angular velocity moves toward
   that goal by at most `acceleration` per call, and the result is applied as a rotation. Its only
   caller, `FUN_005625b0` (the unit aiming update), passes unit-tag fields scaled by `1/30` and
   `1/900`, i.e. per-tick velocity and acceleration, and its sibling early-out does exactly what
   this function's own degenerate path does.
2. **`0x4cc3a0` is 3DNow!, not SSE.** Already flagged in `math_types_notes.md` and renamed here to
   `matrix4x3_multiply_3dnow`: the body is full of `0f 0f` escape opcodes and `math_initialize`
   selects it with `cpu_get_type(0x1a)`. The real SSE routine is `0x4cc250`, selected with
   `cpu_get_type(0x1d)`, and it is *outside* this module's function list.
3. **`0x4cd070` `random_seed_generate` is mostly platform code.** It is `rand()` XOR two
   `QueryPerformanceCounter` readings scaled by the frequency at `0x006ac8f8`; the only part that
   belongs to this module is the seed it hands to `sphere_point_table_init`.
4. **`0x44d950` and `0x44d8e0` are not in this module** but are called from it.
   `0x44d950` is `plane2d_from_points` (documented above); `0x44d8e0` is
   `vector3d_scalar_triple_product`. Both are still `FUN_*` in `symbols/functions.txt`; they are
   declared locally in the files that use them rather than renamed here.
5. **Four out-of-module CRT helpers** are called through the x87 stack, which is why Ghidra shows
   them with no arguments: `0x628cca` is `_CIfmod` (`x` in `ST(1)`, `y` in `ST(0)`), `0x6283c0` is
   `_CIpow` (base in `ST(1)`, exponent in `ST(0)`), `0x628140` is `acos`, and `0x6391b4` is
   `__ftol`.

## Known gaps

1. **`sphere_mesh_get_face_point` / `get_edge_point` / `interpolate_vertex` (`0x4ca7c0`–
   `0x4ca9a0`) are the weakest cluster.** The top of the chain is now settled — the
   next-point-index counter is a stack slot in `sphere_mesh_generate` seeded to 6, passed in `EAX`
   to `sphere_mesh_build_face`, which does `mov ebx,eax` so it is `EBX` from there down — but the
   three leaf functions themselves have not been read against the disassembly, and their
   argument lists are still partly inferred.
2. **`segment3d_distance_squared_to_segment` (`0x4cdef0`) and `segment3d_within_radius_of_segment`
   (`0x4ceae0`), the two largest functions in the module, are still at 0.3–0.35.** Their
   non-parallel branches call `vector3d_scalar_triple_product` with only one visible argument, and
   `point3d_distance_squared_to_segment` with none. The parallel branches are transcribed exactly;
   the non-parallel `s`/`t` are reconstructed from the standard closest-point formula.
3. **`triangle_point_barycentric_2d` (`0x4ce8c0`) and `real_seek_toward_clamped` (`0x4cf360`)** are
   large, control-flow-heavy and were only checked at the return-value level (both are byte
   returns; see below). Their bodies are still literal transliterations of a decompile full of
   `CONCAT2`/`NAN()` artifacts.
4. **`ray_intersects_cylinder` (`0x4ce4e0`) is at 0.3** and `ray_intersects_sphere_test`
   (`0x4ce6c0`) at 0.4; both have elided callee arguments.
5. **`polygon2d_clip_to_plane` / `polygon3d_clip_to_plane` (`0x4caff0`, `0x4cb380`)** are correct
   in outline but their "leftover byte" copy loops and near-duplicate-vertex rejection were not
   re-derived from the disassembly. Their caller `polygon2d_clip_to_planes` now has been.
6. **`vector3d_projection_band_test` (`0x4cef90`) is arithmetically exact but semantically open.**
   Every instruction is accounted for, yet nothing in this module names its four scalars; the
   shape of the quadratic suggests a swept-sphere or cone-vs-segment test.
7. **Unused struct fields**: `sphere_mesh::unknown_02` and `::unknown_12` are never written by any
   function in the range.
8. **`bit_vector_or` (`0x4cb760`) has a 0.40 name.** The body is unambiguous (word-wise OR of one
   fixed-size bit array into another) but nothing in this module fixes the element count or the
   owner of the arrays.

9. **The nine AI-range helpers are small and mostly settled, with three open points.**
   `point3d_within_horizontal_cone` (`0x414910`) uses its `reference` vector unnormalized.
   `path_find_closest_point_on_segment` (`0x43b2f0`) computes `t` from `start - point`, which is
   the textbook parameter negated, so it lands on the end-point path whenever the point projects
   inside the segment. `vector2d_tangent_edge_directions` (`0x43c400`) clamps `extent/distance`
   only from above. All three are confirmed in the disassembly and kept as compiled. What is still
   open is whether the callers mean it.
10. **Out-of-module callers disagree with some of these definitions.** Their externs were written
   before the definitions existed: `random_int_range` in `src/ai/actor_squad_action_execute.c`
   (one `int32_t` argument), `FUN_00414910`, `FUN_0043c340`, `FUN_0043c380`, `FUN_0043c400` and
   `path_find_closest_point_on_segment(void)` in `src/ai/`, and eleven incomplete `vector3d_cross_product`
   externs in `src/objects/`, `src/physics/` and `src/units/` (nine take no arguments, two take only `out`).
   `src/saved_games/` still calls `0x007196f4` `no_simd_matrix_multiply_flag`, a name this module
   used to give it; it is `safe_mode`. None of them was edited from this
   module.

## Return-value widths

Fourteen functions return a **byte**, not an `int`: the result is written with `mov al,1` / `xor al,al`
(or `mov al,cl`) and never zero-extended, so the upper 24 bits of `EAX` are garbage. They are
declared `uint8_t` here — `polygon2d_point_inside_tolerance`, `polygon2d_point_inside_margin`,
`vector3d_rotate_toward`, `ray_intersects_sphere`, `ray_intersects_cylinder`,
`ray_intersects_sphere_test`, `triangle_point_barycentric_2d`, `vector3d_projection_band_test`,
`plane3d_intersect_three`, `plane3d_intersect_pair_to_line`, `real_seek_toward_clamped`,
`lerp_find_threshold_byte`, `point3d_within_horizontal_cone` and `ray2d_intersect_circle_distance`.
`segment3d_within_radius_of_segment` and `point3d_within_radius` are the booleans that really do
write all of `EAX` (`mov eax,1` / `xor eax,eax`), and are declared `int`. Several `int16_t` returns (`polygon2d_*`, `sphere_mesh_get_*`) likewise
only fill `AX`.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite, `U` counts `UNSURE`
markers in the file. A `rw` of 0.85 or better means the function was checked instruction by
instruction against `objdump -d` output, not just against the Ghidra decompile.

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x401050` | `random_real_range` | 55 | 0.70 | 0.85 | 0 |
| `0x4018e0` | `vector2d_normalize_with_length` | 68 | 0.80 | 0.85 | 0 |
| `0x401930` | `point3d_add_scaled` | 41 | 0.60 | 0.85 | 0 |
| `0x401960` | `vector3d_length` | 33 | 0.80 | 0.85 | 0 |
| `0x401990` | `vector3d_normalize_with_length` | 87 | 0.80 | 0.85 | 0 |
| `0x4019f0` | `random_real` | 41 | 0.70 | 0.85 | 0 |
| `0x4088b0` | `vector3d_distance` | 41 | 0.80 | 0.85 | 0 |
| `0x4052c0` | `vector3d_cross_product` | 81 | 0.80 | 0.85 | 0 |
| `0x405320` | `random_int_range` | 49 | 0.80 | 0.85 | 1 |
| `0x405360` | `float_compare_ascending` | 45 | 0.80 | 0.90 | 0 |
| `0x414910` | `point3d_within_horizontal_cone` | 128 | 0.40 | 0.75 | 3 |
| `0x433c70` | `object_sort_by_flag_then_distance` | 71 | 0.50 | 0.85 | 0 |
| `0x43b2f0` | `path_find_closest_point_on_segment` | 192 | 0.60 | 0.85 | 2 |
| `0x43c340` | `point3d_within_radius` | 64 | 0.60 | 0.85 | 0 |
| `0x43c380` | `ray2d_intersect_circle_distance` | 125 | 0.50 | 0.80 | 1 |
| `0x43c400` | `vector2d_tangent_edge_directions` | 163 | 0.40 | 0.80 | 2 |
| `0x4ca4b0` | `sphere_mesh_generate` | 317 | 0.70 | 0.85 | 0 |
| `0x4ca5f0` | `sphere_mesh_build_face` | 450 | 0.75 | 0.70 | 1 |
| `0x4ca7c0` | `sphere_mesh_get_face_point` | 248 | 0.60 | 0.40 | 2 |
| `0x4ca8c0` | `sphere_mesh_get_edge_point` | 211 | 0.60 | 0.50 | 1 |
| `0x4ca9a0` | `sphere_mesh_interpolate_vertex` | 154 | 0.60 | 0.50 | 1 |
| `0x4caa40` | `polygon2d_points_classify` | 153 | 0.60 | 0.90 | 0 |
| `0x4caae0` | `polygon2d_convex_hull_build` | 651 | 0.55 | 0.40 | 1 |
| `0x4cad80` | `polygon2d_point_inside_tolerance` | 210 | 0.60 | 0.70 | 0 |
| `0x4cae60` | `polygon2d_point_inside_margin` | 128 | 0.55 | 0.75 | 0 |
| `0x4caee0` | `polygon2d_clip_to_planes` | 261 | 0.60 | 0.85 | 0 |
| `0x4caff0` | `polygon2d_clip_to_plane` | 898 | 0.60 | 0.50 | 1 |
| `0x4cb380` | `polygon3d_clip_to_plane` | 958 | 0.55 | 0.50 | 1 |
| `0x4cb740` | `uint32_log2_floor` | 22 | 0.50 | 0.85 | 0 |
| `0x4cb760` | `bit_vector_or` | 54 | 0.40 | 0.60 | 0 |
| `0x4cb7a0` | `matrix4x3_inverse` | 212 | 0.90 | 0.85 | 0 |
| `0x4cb880` | `matrix4x3_from_axis_angle` | 230 | 0.65 | 0.75 | 0 |
| `0x4cb970` | `matrix4x3_from_forward_up` | 147 | 0.45 | 0.70 | 0 |
| `0x4cba10` | `matrix4x3_from_euler_angles` | 179 | 0.55 | 0.70 | 0 |
| `0x4cbad0` | `matrix4x3_from_quaternion` | 290 | 0.75 | 0.80 | 0 |
| `0x4cbc00` | `quaternion_from_matrix4x3` | 337 | 0.70 | 0.75 | 0 |
| `0x4cbd60` | `matrix4x3_from_forward_up_position` | 35 | 0.40 | 0.50 | 1 |
| `0x4cbd90` | `matrix4x3_extract_forward_up_position` | 72 | 0.40 | 0.60 | 0 |
| `0x4cbde0` | `matrix4x3_transform_point` | 109 | 0.90 | 0.90 | 0 |
| `0x4cbe50` | `matrix4x3_transform_vector` | 100 | 0.65 | 0.90 | 0 |
| `0x4cbec0` | `matrix4x3_transform_normal` | 76 | 0.90 | 0.90 | 0 |
| `0x4cbf10` | `matrix4x3_transform_plane` | 102 | 0.60 | 0.75 | 0 |
| `0x4cbf80` | `matrix4x3_inverse_transform_point` | 140 | 0.65 | 0.75 | 0 |
| `0x4cc010` | `matrix4x3_inverse_transform_vector` | 104 | 0.60 | 0.75 | 0 |
| `0x4cc080` | `matrix4x3_inverse_transform_normal` | 76 | 0.55 | 0.80 | 0 |
| `0x4cc0d0` | `matrix4x3_multiply` | 384 | 0.90 | 0.90 | 0 |
| `0x4cc3a0` | `matrix4x3_multiply_3dnow` | 352 | 0.85 | 0.60 | 1 |
| `0x4cc500` | `matrix3x3_transpose` | 94 | 0.70 | 0.85 | 0 |
| `0x4cc560` | `matrix3x3_from_forward_up` | 129 | 0.45 | 0.75 | 0 |
| `0x4cc5f0` | `matrix3x3_multiply` | 277 | 0.75 | 0.85 | 0 |
| `0x4cc710` | `matrix3x3_inverse_transform_vector` | 111 | 0.50 | 0.80 | 0 |
| `0x4cc780` | `quaternion_from_matrix3x3` | 327 | 0.70 | 0.75 | 0 |
| `0x4cc8d0` | `periodic_function_tables_init` | 135 | 0.65 | 0.75 | 0 |
| `0x4cc960` | `periodic_function_tables_free` | 75 | 0.65 | 0.85 | 0 |
| `0x4cc9b0` | `periodic_function_evaluate` | 264 | 0.90 | 0.90 | 0 |
| `0x4ccac0` | `transition_function_evaluate` | 238 | 0.65 | 0.90 | 0 |
| `0x4ccbb0` | `periodic_function_build_noise_table` | 250 | 0.40 | 0.55 | 1 |
| `0x4cccb0` | `periodic_function_build_transition_table` | 229 | 0.60 | 0.90 | 0 |
| `0x4ccdb0` | `periodic_function_build_table` | 644 | 0.80 | 0.90 | 0 |
| `0x4cd070` | `random_seed_generate` | 110 | 0.60 | 0.60 | 0 |
| `0x4cd0e0` | `sphere_point_table_init` | 131 | 0.55 | 0.70 | 0 |
| `0x4cd170` | `random_real_range_seeded` | 57 | 0.60 | 0.75 | 0 |
| `0x4cd1b0` | `vector3d_randomize_direction` | 289 | 0.75 | 0.90 | 0 |
| `0x4cd2e0` | `vector2d_normalize` | 62 | 0.65 | 0.80 | 0 |
| `0x4cd320` | `vector3d_normalize` | 81 | 0.65 | 0.80 | 0 |
| `0x4cd380` | `vector3d_cross_product_length` | 108 | 0.65 | 0.80 | 0 |
| `0x4cd3f0` | `math_initialize` | 134 | 0.80 | 0.65 | 0 |
| `0x4cd480` | `vector2d_angle_between` | 101 | 0.60 | 0.65 | 0 |
| `0x4cd4f0` | `vector3d_angle_between_4cd4f0` | 238 | 0.60 | 0.60 | 0 |
| `0x4cd5e0` | `vector3d_angle_between_4cd5e0` | 131 | 0.55 | 0.55 | 1 |
| `0x4cd670` | `vector3d_build_perpendicular` | 137 | 0.55 | 0.65 | 0 |
| `0x4cd700` | `vector3d_rotate_about_axis_perpendicular` | 137 | 0.40 | 0.65 | 0 |
| `0x4cd790` | `vector3d_rotate_pair_in_plane` | 133 | 0.40 | 0.65 | 0 |
| `0x4cd820` | `vector3d_rotate_about_axis` | 145 | 0.75 | 0.80 | 0 |
| `0x4cd8c0` | `vector3d_lerp` | 57 | 0.70 | 0.85 | 0 |
| `0x4cd900` | `real_lerp_clamped` | 71 | 0.55 | 0.80 | 0 |
| `0x4cd950` | `vector3d_rotate_toward` | 217 | 0.45 | 0.55 | 1 |
| `0x4cda30` | `vector3d_project_onto_unit_axis` | 82 | 0.60 | 0.75 | 0 |
| `0x4cda90` | `vector3d_project_onto_axis` | 143 | 0.60 | 0.75 | 0 |
| `0x4cdb20` | `quaternion_normalize` | 111 | 0.85 | 0.85 | 0 |
| `0x4cdb90` | `quaternion_to_axis_angle` | 94 | 0.85 | 0.65 | 1 |
| `0x4cdbf0` | `quaternion_multiply` | 202 | 0.85 | 0.75 | 0 |
| `0x4cdcc0` | `quaternion_lerp` | 117 | 0.70 | 0.80 | 0 |
| `0x4cdd40` | `quaternion_rotate_vector` | 153 | 0.80 | 0.65 | 0 |
| `0x4cdde0` | `euler_angles_to_basis_vectors` | 66 | 0.60 | 0.90 | 0 |
| `0x4cde30` | `point3d_distance_squared_to_segment` | 191 | 0.60 | 0.75 | 0 |
| `0x4cdef0` | `segment3d_distance_squared_to_segment` | 1187 | 0.60 | 0.35 | 1 |
| `0x4ce3a0` | `ray_intersects_sphere` | 320 | 0.70 | 0.55 | 0 |
| `0x4ce4e0` | `ray_intersects_cylinder` | 469 | 0.50 | 0.30 | 1 |
| `0x4ce6c0` | `ray_intersects_sphere_test` | 263 | 0.55 | 0.40 | 2 |
| `0x4ce7d0` | `ray_intersect_sphere_distance` | 240 | 0.65 | 0.60 | 0 |
| `0x4ce8c0` | `triangle_point_barycentric_2d` | 544 | 0.45 | 0.35 | 2 |
| `0x4ceae0` | `segment3d_within_radius_of_segment` | 1198 | 0.50 | 0.30 | 1 |
| `0x4cef90` | `vector3d_projection_band_test` | 175 | 0.45 | 0.85 | 0 |
| `0x4cf040` | `plane3d_intersect_three` | 410 | 0.60 | 0.60 | 2 |
| `0x4cf1e0` | `plane3d_intersect_pair_to_line` | 379 | 0.50 | 0.60 | 1 |
| `0x4cf360` | `real_seek_toward_clamped` | 457 | 0.40 | 0.30 | 1 |
| `0x4cf530` | `vector3d_rotate_toward_with_acceleration` | 615 | 0.55 | 0.85 | 0 |
| `0x4cf7a0` | `lerp_find_threshold_byte` | 108 | 0.50 | 0.90 | 0 |

Twenty-four of these names were first established by this rewrite and are registered in
`symbols/agent_phase4_math.txt`, which `tools/merge_symbols.py` merges into
`symbols/functions.txt`. Twenty-one replaced a bare `FUN_xxxxxxxx`. Three replaced an existing but
wrong phase-2 name: `0x4cc3a0`, `0x4cf530`, and `0x414910` (was `actor_is_direction_within_forward_cone`,
though it touches no actor state). The four added in the phase-4 gate review (`0x414910`, `0x43c340`, `0x43c380`,
`0x43c400`) are in the per-source file but `functions.txt` has not been re-merged yet. The other 75
already matched `symbols/functions.txt` exactly, and a scripted check confirms every file in this directory is named after the symbol its
address carries.

## Verification method

Ghidra's decompile of this module is unusually lossy, because almost every argument here travels
either in a register (the Blam custom convention) or on the x87 stack. Where a rewrite needed a
value Ghidra had dropped, it was recovered from

```
objdump -d --start-address=0xADDR --stop-address=0xEND -M intel bin/halo.exe
```

plus direct reads of `.rdata` / `.data` constants out of `bin/halo.exe` by virtual address. Every
header comment that says **VERIFIED** lists the exact instruction addresses it relies on, so the
claim can be rechecked without redoing the analysis. That pass cut the module's `UNSURE` count
from 53 to 25 and turned up nine substantive semantic corrections, all documented in the file
headers.

## Phase 4 gate review (99-function pass)

A final review read all nine AI-range files against `objdump` and made these fixes. Each one is
also noted in the header of the file it touches.

| File | Fix |
|---|---|
| `random_int_range.c` | multiply and shift were signed (`sar`); the binary does `imul` then `shr`, so the product is now shifted as `uint32_t`. Any range above `0x8000` could sign-fill before the fix. |
| `point3d_within_horizontal_cone.c` | both late tests are `test ah,0x41 / jne`, which means `<=` or unordered. The draft returned 1 on `dot == threshold` and on a NaN dot. Now `!(dot > threshold)` and `!(length > 0)`. |
| `path_find_closest_point_on_segment.c` | an unordered `t` (zero-length segment, `0/0`) goes to the lerp path in the binary; the draft sent it to the end-point path. Sums also re-ordered to the FPU trace. |
| `point3d_within_radius.c` | squared-distance sum re-ordered to the FPU trace (`dx² + dz²`, then `+ dy²`). |
| `vector2d_tangent_edge_directions.c` | both components of each output are now computed before either is stored, as in the binary, so an output that aliases `direction` behaves the same. |
| `object_sort_by_flag_then_distance.c` | the local `TYPES-GAP` struct was replaced by `ai_nearby_actor_candidate` (`types/ai.h`), and the parameters are now `const void *` to match the extern in `src/ai/`. |
| `float_compare_ascending.c` | parameters changed to `const void *` to match the extern in `src/ai/actor_squad_action_execute.c`. |
| `ray_intersects_cylinder.c` | the local extern for `ray_intersects_sphere` returned `int`; the definition returns `uint8_t` (AL only). The local `hit` is now `uint8_t` too. |
| `math_initialize.c` | globals renamed to the shell module names: `0x007196f4` is `safe_mode`, and `0x00721e90`/`0x00721e94` are `shell_argv`/`shell_argc`. |

