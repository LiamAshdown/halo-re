# math module: type recovery notes

Header: `types/math.h`. Smoke test: `out/phase4/math_smoke.c`
(`gcc -fsyntax-only -I types out/phase4/math_smoke.c`, clean, and clean again with
`-std=c99 -Wall -Wextra`).

Sources: `out/phase4/math_functions.md`, `out/phase2/math/00.md`, `out/phase2/math/01.md`,
full decompiles via `python tools/pack.py 0xADDR`, raw disassembly bytes and direct reads of
`bin/halo.exe` .rdata/.data (PE VA -> file offset: .rdata 0x63a000 -> 0x23a000,
.data 0x676000 -> 0x276000) for every constant table, and the `WaveFunction` / `FunctionType`
tag enums already in `types/tags.h`. Everything below is evidence unless it says "unresolved"
or "guess".

Note on sizes: like `types/memory.h`, structs that contain pointers only measure to the
documented size under a 32-bit data organization. `sizeof(sphere_mesh)` is 0x1c on a 64-bit
host compiler and 0x14 on the target, which is the size `GlobalAlloc(0, 0x14)` proves.

## real_vector2d / real_point2d (0x08), real_vector3d / real_point3d (0x0c)

- `vector2d_normalize_with_length` @0x4018e0 and `vector2d_normalize` @0x4cd2e0 touch exactly
  `[0]` and `[1]`; `vector3d_length` @0x401960, `vector3d_normalize_with_length` @0x401990,
  `vector3d_distance` @0x4088b0 and `vector3d_normalize` @0x4cd320 touch `[0]`, `[1]`, `[2]`.
- `sphere_mesh_interpolate_vertex` @0x4ca9a0 and `sphere_point_table_init` @0x4cd0e0 both
  stride the point array by 0x0c, and `movsx eax,[esi+0xc]` / `lea eax,[eax+eax*2]` /
  `shl eax,2` at 0x4cd0f6 is a literal `count * 12`.
- `polygon2d_point_inside_tolerance` @0x4cad80 and `polygon2d_point_inside_margin` @0x4cae60
  stride their vertex array by 8 (`in_ECX + index*8`, second float at `+4`), which fixes the
  2D size.
- Layout is identical to `Vector2D` / `Point2D` / `Vector3D` / `Point3D` in `types/tags.h`;
  declared separately so `math.h` parses standalone without colliding with those names.
- Unresolved: nothing.

## real_plane2d (0x0c) and real_plane3d (0x10)

- `polygon2d_clip_to_plane` @0x4caff0: `(*param_3 * p[0] + p[1] * param_3[1]) - param_3[2]`,
  so three floats with the distance last and *subtracted*.
- `polygon3d_clip_to_plane` @0x4cb380: `(p[0]*pl[0] + p[1]*pl[1] + p[2]*pl[2]) - pl[3]`.
  `plane3d_intersect_three` @0x4cf040 uses `param_1[3]`, `unaff_EBX[3]`, `unaff_EDI[3]` as the
  three distances and `[0..2]` as the three normals; `plane3d_intersect_pair_to_line`
  @0x4cf1e0 does the same with two planes.
- `matrix4x3_transform_plane` @0x4cbf10 reads `in_EDX[3]` and writes `in_EAX[3]`, confirming
  0x10 for the 3D plane.
- Unresolved: nothing.

## real_quaternion (0x10)

- `quaternion_normalize` @0x4cdb20 writes `{0, 0, 0, 1}` on a degenerate quaternion, which is
  what fixes element 3 as the scalar part (w) rather than element 0.
- `quaternion_multiply` @0x4cdbf0 spills four floats when the destination aliases an operand;
  `quaternion_lerp` @0x4cdcc0 blends four; `quaternion_to_axis_angle` @0x4cdb90 hands
  `[0..2]` to `vector3d_normalize_with_length` and uses `[3]` as the `fpatan` adjacent side.
- `quaternion_from_matrix4x3` @0x4cbc00 / `quaternion_from_matrix3x3` @0x4cc780 build the
  vector part into a 3-float local and write `param_1[3]` last.
- Unresolved: nothing.

## real_euler_angles3d (0x0c)

- `matrix4x3_from_euler_angles` @0x4cba10 takes three scalars; `FUN_004cdde0` @0x4cdde0 calls
  it as `(*in_EAX, in_EAX[1], in_EAX[2])`, i.e. from one 3-float object, which is what makes
  it a struct at all.
- Field *order* (yaw, pitch, roll) is taken from the `Euler3D` definition in `types/tags.h`
  and is consistent with the code: `param_3` drives the roll pair that ends up in the
  forward/left rows and `param_2` drives `in_EAX[7] = -sin(param_2)`, the pitch term of the
  up row.
- `real_euler_angles2d` (0x08) has no function of its own in this module; it is included for
  completeness and matches `Euler2D` in `types/tags.h`. **Guess, not evidence.**

## real_matrix3x3 (0x24)

- `matrix3x3_transpose` @0x4cc500 and `matrix3x3_multiply` @0x4cc5f0 copy exactly 9 floats
  (the alias spill loop in multiply is `iVar1 = 9`).
- `quaternion_from_matrix3x3` @0x4cc780 indexes `in_ECX[i*3 + j]` and reads the diagonal at
  `[0]`, `[4]`, `[8]` -- row major, three rows of three.
- Rows are the basis vectors: `matrix3x3_inverse_transform_vector` @0x4cc710 computes
  `out[0] = m[0]*v[0] + m[3]*v[1] + m[6]*v[2]`, the transpose, which is only an inverse for
  an orthonormal forward/left/up basis.
- `FUN_004cc560` @0x4cc560 writes `[0..2]` from one input vector, `[3..5]` from the cross
  product, `[6..8]` from the other input -- a basis builder, so the three rows are named
  forward / left / up in that order.
- Identical layout to `Matrix { float m[3][3]; }` in `types/tags.h`.
- Unresolved: nothing.

## real_matrix4x3 (0x34) -- the most heavily confirmed struct in the module

- Size: `matrix4x3_inverse` @0x4cb7a0 zero-fills 13 floats on the degenerate path
  (`for (iVar6 = 0xd; ...)`), and `matrix4x3_multiply` @0x4cc0d0 spills 13 floats when the
  destination aliases either operand. 13 * 4 = 0x34.
- `+0x00` is a *uniform scale*, not a matrix element: `matrix4x3_transform_point` @0x4cbde0
  and `matrix4x3_transform_vector` @0x4cbe50 both short-circuit `if (*param_1 != 1.0)` and
  pre-multiply the input by it; `matrix4x3_inverse` returns an all-zero matrix when it is 0
  and writes `1.0 / scale`; `matrix4x3_multiply` sets `*out = *a * *b`.
- `+0x04..+0x0f` forward, `+0x10..+0x1b` left, `+0x1c..+0x27` up:
  `matrix4x3_transform_point` computes `out.x = p.i*m[1] + p.j*m[4] + p.k*m[7] + m[10]`, so
  `m[1]`, `m[4]`, `m[7]` are the *x components* of three rows, making `m[1..3]` one row.
  `matrix4x3_transform_normal` @0x4cbec0 reads them as `+0x04/+0x10/+0x1c` for the first
  output component, which is the same statement in byte offsets.
  `matrix4x3_inverse_transform_normal` @0x4cc080 reads `+0x04/+0x08/+0x0c` for the first
  output component -- the transpose -- confirming the rows from the other direction.
- `+0x28..+0x33` position: `FUN_004cbd60` @0x4cbd60 writes `+0x28`, `+0x2c`, `+0x30` after
  calling the 3x3 builder, and `FUN_004cbd90` @0x4cbd90 unpacks forward from `+0x04..+0x0c`,
  up from `+0x1c..+0x24` and position from `+0x28..+0x30`.
  `matrix4x3_inverse_transform_point` @0x4cbf80 subtracts `m[10..12]` before rotating.
- `quaternion_from_matrix4x3` @0x4cbc00 indexes `*(float *)(in_ECX + 4 + (i*3 + j)*4)`,
  i.e. a 3x3 block starting at byte +4, with the diagonal at +0x04, +0x14, +0x24. Independent
  confirmation of both the +4 base and the row-major 3x3.
- `FUN_004cdde0` @0x4cdde0 builds one on the stack and copies `local_38..local_30` (base+4,
  forward) and `local_20..local_18` (base+0x1c, up) out -- a third independent confirmation.
- `matrix4x3_from_axis_angle` @0x4cb880, `matrix4x3_from_euler_angles` @0x4cba10 and
  `matrix4x3_from_quaternion` @0x4cbad0 all write `[0] = 1.0f` and zero `[10..12]`,
  i.e. scale 1 and no translation.
- Unresolved: nothing.

## real_bounds (0x08) / real_rectangle3d (0x18)

- Weakest struct in the header. Evidence is the .rdata constant at 0x0065c284:
  six floats `+FLT_MAX, -FLT_MAX, +FLT_MAX, -FLT_MAX, +FLT_MAX, -FLT_MAX`, pointed at twice
  from the global pointer table (0x00696744 and 0x00696748) -- the classic empty/null
  bounding box -- plus `FUN_004cf360` @0x4cf360, which clamps and optionally wraps a tracked
  value into `[param_4, param_5]` (`param_5 - param_4` is used as the wrap period).
- **Unresolved: no function in this module takes a `real_bounds *` or `real_rectangle3d *`.**
  The 24 bytes at 0x0065c284 are contiguous and end exactly where the projection axis table
  begins, which is the only thing bounding the struct.

## sphere_mesh (0x14)

Established entirely by `sphere_mesh_generate` @0x4ca4b0, which does `GlobalAlloc(0, 0x14)`
and therefore pins the size exactly.

- `+0x00` `int16 subdivisions` = `in_AX`. `sphere_point_table_init` @0x4cd0e0 passes 16
  (`b8 10 00 00 00` = `mov eax, 0x10` at 0x4cd0ec).
- `+0x02` **unresolved**: never written and never read. `GlobalAlloc` with flags 0 does not
  zero, so this is uninitialized alignment padding ahead of the pointer at +0x04.
- `+0x04` `real_point3d *points`: allocated `point_count * 0xc` bytes; the six octahedron
  seed vertices are copied into it 12 bytes at a time; `sphere_mesh_interpolate_vertex`
  @0x4ca9a0 writes `points[index*0xc + 0/4/8]`; `sphere_point_table_init` copies 12 bytes per
  entry out of `*(int *)(mesh + 4)`.
- `+0x08` `int16 *indices`: allocated `triangle_count * 8` bytes and written by
  `sphere_mesh_build_face` @0x4ca5f0 as `*(short *)(*(int *)(mesh + 8) + cursor*2)`.
- `+0x0c` `int16 point_count` = `8*(n-2)*(n-1)/2 - 6 + 12*n` (1026 for n = 16). Confirmed
  twice: it is the multiplier for the 12-byte point allocation, and `sphere_point_table_init`
  reads it with `movsx eax, word ptr [esi+0xc]` before the `* 12`.
- `+0x0e` `int16 triangle_count` = `8*n*n` (2048 for n = 16); the multiplier for the
  `<< 3` index allocation.
- `+0x10` `int16 strip_count`: zeroed by generate, incremented once per row by
  `sphere_mesh_build_face` (`unaff_ESI[8] = unaff_ESI[8] + 1`) right where that row writes its
  run length (3, 5, 7, ... = 2*row+1) into the index buffer. So `indices` is a run list, not a
  flat triangle list.
- `+0x12` **unresolved**: never written, tail padding to 0x14.
- **Unresolved, structural:** the number of int16 actually written into `indices` lives in a
  stack local in `sphere_mesh_generate` (`&local_8`, passed to `sphere_mesh_build_face` as
  `param_4`) and is never stored in the header. A consumer of a `sphere_mesh` cannot recover
  the index length from the struct; it is only bounded by `triangle_count * 4`.

### sphere_mesh_edge_cache (0x80) and sphere_mesh_face_cache

- Edge cache: `GlobalAlloc(0, 0x80)` in `sphere_mesh_generate`, filled with 0xffffffff by a
  32-iteration dword loop (= 64 int16). `sphere_mesh_get_edge_point` @0x4ca8c0 indexes it as
  `hi + lo*8` after sorting the two base vertex indices, so it is `int16 [8][8]` keyed by the
  6 octahedron vertices (8 is the padded stride).
- Face cache: `sphere_mesh_build_face` allocates `(n+1)*(n+1)` int16 and fills it with 0xffff;
  `sphere_mesh_get_face_point` @0x4ca7c0 indexes `(n+1)*row + column`. Size depends on `n`, so
  it is declared as a flexible one-element array with the size in a comment.

## periodic_function / transition_function tables

- `periodic_function_tables_init` @0x4cc8d0 allocates **12** tables of `0x400` bytes from
  `&DAT_006b7aa8` and calls `periodic_function_build_table` @0x4ccdb0 with the index, then
  **6** tables of `0x400` from `&DAT_006b7ad8` calling `FUN_004cccb0` @0x4cccb0.
  `periodic_function_tables_free` @0x4cc960 frees 12 then 6, same order.
- The 12-case switch in `periodic_function_build_table` matches `WaveFunction` in
  `types/tags.h` one for one: case 0 constant 1.0, case 1 constant 0.0, case 2
  `cos(t*2pi)` (cosine), case 3 the same cosine driven by the cumulative noise table from
  `FUN_004ccbb0` (cosine variable period), cases 4/5 a triangle wave (diagonal wave),
  cases 6/7 the raw ramp (slide), case 8 a raw LCG draw (noise), cases 9/10 a sum of cosines
  (jitter/wander), case 0x0b squares its sample (spark). **This tag-side definition is the
  evidence for the order**, per the "prefer the binary's own layout" rule -- the table is not
  self-describing.
- The 6-case switch in `FUN_004cccb0` matches `FunctionType` (linear, early, very early, late,
  very late, cosine): case 0 is the identity ramp and case 5 is `sin(t*pi - pi/2)`.
- Table entries are `value * 255`: every evaluator multiplies the byte by 0.003921569.
  `periodic_function_evaluate` @0x4cc9b0 masks the index with 0x3ff and lerps
  `table[i]`/`table[(i+1) & 0x3ff]`; `FUN_004ccac0` @0x4ccac0 does the same at an explicit
  [0,1] phase against the 6-table set.
- `periodic_function_evaluate` tests `(1 << type) & 0xc0` -- types 6 and 7, the two slides --
  and wraps the interpolated value modulo 1 for those, which is why they are sawtooths.
- **Unresolved:** the `* 25.6` time scale in `periodic_function_evaluate` is
  `1024 / 40`, i.e. one table period per 40 ticks (1.33 s at 30 tps), but nothing in the
  module states the unit; it is an inference from the table size.

## projection_axis_pair (0x04)

- `FUN_004ce8c0` @0x4ce8c0 picks the largest-magnitude component of the triangle normal, then
  indexes `(&DAT_0065c29c)[((component > 0) + axis*2) * 4]` and reads two int16 from it
  (`DAT_0065c29c`/`DAT_0065c29e`, and the module also references `DAT_0065c2ac`,
  `DAT_0065c2ae`, `DAT_0065c2b0`, `DAT_0065c2b2`, i.e. the last two entries).
- Read out of .rdata: `{2,1} {1,2} {0,2} {2,0} {1,0} {0,1}` -- for each dominant axis the two
  surviving axes, swapped by sign so the projected winding is preserved.
- The table is 0x0065c29c..0x0065c2b4 and ends exactly where `bit_mask_clear` (documented in
  `types/memory.h` at 0x0065c2b4) begins, which bounds it at 6 entries.

## quaternion next-index tables

- `quaternion_from_matrix4x3` @0x4cbc00 reads `*(short *)(&DAT_0069665c + i*2)`;
  `quaternion_from_matrix3x3` @0x4cc780 reads `*(short *)(&DAT_00696668 + i*2)`. Both tables
  read `{1, 2, 0}` in .data. Two copies of the same constant, one per function, with an int16
  of padding after each (0x00696662, 0x0069666e).

## globals owned by the module

All contiguous, no gaps:

| address | type | name | evidence |
| --- | --- | --- | --- |
| 0x00719cd0 | `uint32` | `random_seed_global` | `random_real` @0x4019f0, `random_real_range` @0x401050; forced to 0x20f3f660 by `periodic_function_tables_init` |
| 0x00719cd4 | `uint32` | `local_random_seed` | written by `sphere_point_table_init` from `random_seed_generate` @0x4cd070; read by ~40 call sites elsewhere |
| 0x006b7aa8 | `uint8 *[12]` | `periodic_function_tables` | `periodic_function_tables_init`, 12-iteration loop of 4-byte slots |
| 0x006b7ad8 | `uint8 *[6]` | `transition_function_tables` | same function, 6-iteration loop; 0x006b7aa8 + 12*4 == 0x006b7ad8 |
| 0x006b7af0 | `uint8` | `periodic_functions_initialized` | set 1 / cleared on allocation failure; every evaluator early-outs on it |
| 0x006b7af4 | `real_point3d *` | `sphere_point_table` | `sphere_point_table_init`; 12-byte stride in `FUN_004cd1b0` |
| 0x006b7af8 | `int16` | `sphere_point_table_count` | `= mesh->point_count`, read as a short in `FUN_004cd1b0` and elsewhere |
| 0x00696664 | fn ptr | `matrix4x3_multiply_procedure` | `math_initialize` @0x4cd3f0 |
| 0x0069665c | `int16[3]` | `k_quaternion_next_index_matrix4x3` | `{1,2,0}` in .data |
| 0x00696668 | `int16[3]` | `k_quaternion_next_index_matrix3x3` | `{1,2,0}` in .data |
| 0x0065c190 | `real_point3d[6]` | `k_octahedron_vertices` | .rdata; `sphere_mesh_generate` copies 6 * 12 bytes |
| 0x0065c1d8 | `int16[8][3]` | `k_octahedron_faces` | .rdata; 8 iterations of a 3-short stride |
| 0x0065c29c | `projection_axis_pair[6]` | `k_projection_axes` | .rdata, see above |

### Read-only constants in .rdata at 0x0065c208..0x0065c29c

Read straight out of the image; contiguous, and pointed at by the .data pointer table at
0x006966d0..0x0069674c so the compiler cannot constant-fold them:
`global_identity_matrix4x3` 0x0065c208 (scale 1, rotation I, position 0),
`global_identity_matrix3x3` / `global_forward3d` 0x0065c20c, `global_left3d` 0x0065c218,
`global_up3d` 0x0065c224, `global_origin3d` 0x0065c230,
`global_negative_identity_matrix4x3` 0x0065c240, `global_backward3d` 0x0065c244,
`global_right3d` 0x0065c250, `global_down3d` 0x0065c25c,
`global_identity_quaternion` 0x0065c274, `global_null_rectangle3d` 0x0065c284.

- **Unresolved:** only 0x00696714 (`global_origin3d`) is dereferenced from inside this module,
  by `vector3d_random_point_in_cone` @0x4cf530. The pointer table repeats itself -- there is a
  five-entry group at 0x006966d0, another at 0x006966e4, then two identical seven-entry groups
  (origin, forward, left, up, backward, right, down) at 0x006966f8 and 0x00696714, and
  0x00696730 and 0x00696734 both point at `global_up3d`. The *values* are certain; which slot
  each source symbol owns is not. Only the 0x00696714 group is named in the header.

## globals read but owned elsewhere

- 0x006ac8f8 / 0x006ac8fc: the 64-bit `QueryPerformanceFrequency` result, divided into both
  counter readings by `random_seed_generate` @0x4cd070. Timing/system module.
- 0x00721e90 / 0x00721e94: `argv` / `argc`, scanned by `math_initialize` for `-noSSE`.
  Startup module.
- 0x007196f4: a flag `math_initialize` requires to be zero before it will install any SIMD
  matrix multiply. Meaning unknown, owner unknown.
- 0x0063a0b0 / 0x0063a0bc / 0x0063a0ac: the `GlobalAlloc` / `GlobalFree` /
  `QueryPerformanceCounter` import thunks.
- Every `DAT_00672xxx` in the phase-2 packs (0x00672abc, 0x00672ac0, 0x00672ac4, 0x00672b84,
  0x00672bd8, ...) is in .rdata and is an MSVC floating-point literal (`1.0f`, `0.0f`,
  `0.0001f`, `1/65536`, `FLT_MAX`, ...), not a program global. None are named in the header.

## misattributed / miscategorised functions

1. **`matrix4x3_multiply_sse` @0x4cc3a0 is not SSE -- it is AMD 3DNow!** Its body contains 21
   `0f 0f` escape bytes (PFMUL/PFADD; Ghidra renders them as `PackedFloatingMUL` /
   `PackedFloatingADD`) and zero SSE opcodes. The *real* SSE implementation is the unnamed
   function at **0x004cc250** (`f3 0f 10` MOVSS, `0f 16` MOVHPS, `0f c6` SHUFPS, 62 SSE-family
   opcodes, zero 3DNow!), which `math_functions.md` does not list at all -- the 90-function
   list jumps from `matrix4x3_multiply` @0x4cc0d0 (size 384, ends at 0x4cc250) straight to
   0x4cc3a0. `math_initialize` installs 0x004cc250 on `cpu_get_type(0x1d)` and 0x004cc3a0 on
   `cpu_get_type(0x1a)`, in that priority order. Recommended renames:
   0x004cc250 -> `matrix4x3_multiply_sse`, 0x004cc3a0 -> `matrix4x3_multiply_3dnow`.
2. **`FUN_004cb760` @0x4cb760** is a bit-vector OR (`dst[i] = src[i] | a[i]` for
   `(bit_count + 31) >> 5` dwords, with the three buffers addressed as base + two deltas).
   It is a flags/bit-vector utility, not geometry; it defines no math type. Suggested name
   `bit_vector_or`.
3. **`uint32_log2_floor` @0x4cb740** is a generic integer helper (shift-until-1 loop). No math
   type. Fine to keep in the module, but it belongs with the bit utilities above.
4. **`random_seed_generate` @0x4cd070** is mostly platform code: `QueryPerformanceCounter`
   twice, CRT `rand`, `__allmul`, `__alldiv`. Its only math-module output is the seed.
5. **`FUN_004cf7a0` @0x4cf7a0** defines no type: it scans a byte value down from a
   float-to-int conversion looking for the largest byte whose `lerp(lo, hi, byte/255)` is at or
   below a threshold -- the inverse of a periodic/transition table sample, so it is math, just
   structureless.
6. **`FUN_004cf360` @0x4cf360** keeps its state in *two separate float pointers* (value in EDI,
   velocity in ESI) plus a `[param_4, param_5]` range in registers -- there is no struct in the
   binary, so none is declared. Suggested name `real_seek_toward_clamped`.
7. Callees used as evidence that are **outside this module's function list** even though some
   are inside its address range: `vector3d_scalar_triple_product` @0x44d8e0 and `FUN_0044d950`
   @0x44d950 (both called by `segment3d_distance_squared_to_segment`,
   `segment3d_within_radius_of_segment`, `plane3d_intersect_three`, `FUN_004caa40`,
   `polygon2d_clip_to_planes`); CRT/compiler helpers `FUN_00628140` (acos), `FUN_00628cca`,
   `FUN_006391b4` (`__ftol`), `__chkstk` @0x628240, `__stricmp` @0x628d8b, `_rand` @0x6240cf,
   `__allmul` @0x62de80, `__alldiv` @0x639230; and `cpu_get_type` @0x5402a0 (system module).
8. The polygon routines (`polygon2d_convex_hull_build` @0x4caae0,
   `polygon2d_point_inside_tolerance` @0x4cad80, `polygon2d_point_inside_margin` @0x4cae60,
   `polygon2d_clip_to_planes` @0x4caee0, `polygon2d_clip_to_plane` @0x4caff0,
   `polygon3d_clip_to_plane` @0x4cb380) pass `(int16 count, point array, ...)` explicitly and
   never a polygon object, so no `polygon2d` / `polygon3d` struct is declared. Two details
   worth recording:
   - `polygon2d_clip_to_plane`'s `param_5` is an in/out `uint32 *` edge bitmask, one bit per
     output vertex, marking vertices created by the clip.
   - `polygon2d_clip_to_planes` double-buffers through `0x800` dwords of `__chkstk` stack
     (two halves of 0x400 dwords = 512 `real_point2d` each), and `polygon3d_clip_to_plane`
     spills through 1537 floats, which caps the vertex counts those routines can handle.
