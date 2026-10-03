# C++ conversion: math pilot findings (for folding into docs/CPP_CONVENTIONS.md)

The math module (124 functions, `src/math/*.c` one function per file) was converted end to end as the pilot. This
file records what was actually done, how it was verified, and the recipe and pitfalls the other modules should
reuse. Everything here is consistent with CPP_CONVENTIONS.md v1; where the pilot needed a rule v1 does not have, it
is marked **proposed**.

## 1. Result

| | before | after |
|---|---|---|
| sources | 124 `src/math/*.c` (+ README) | 13 `src/math/*.cpp`: 12 area files + `math_c_api.cpp` |
| public headers | none (every caller wrote its own `extern`) | `include/halo/math/`: `math.hpp` (umbrella), `math_types.hpp`, 12 area headers, `value_types.hpp`, `glm_interop.hpp`, `math_globals.h`, `math_c_api.h` |
| author notes / decompiles | in the `.c` files | `docs/original/math/<function>.c.txt` (each original file verbatim) + `README.md` |
| C symbols | 124 | 124, identical names and signatures (`tools/check_c_abi_symbols.py math`) |
| behaviour | | byte-identical on 46,042 calls (`tests/math/run_difftest.py`) |

Area files: `real_vector`, `rotation`, `matrix`, `quaternion`, `geometry`, `polygon`, `periodic_functions`,
`interpolation`, `random`, `sphere_mesh`, `utility`, `math_initialize`. Functions keep their original names inside
`namespace halo::math` and are ordered by original address inside each file.

## 2. Layering of a converted module

1. **Implementation** (`src/<module>/<area>.cpp`, `namespace halo::<module>`): the engine functions under their
   original names, with references instead of pointers where a parameter is only dereferenced (`const` when never
   written). Bodies are the original arithmetic, term for term, unless a section below says otherwise.
2. **C ABI** (`src/<module>/<module>_c_api.cpp`, prototypes in `include/halo/<module>/<module>_c_api.h`): one
   `extern "C"` wrapper per original symbol with the original signature, whose body only forwards
   (`return halo::math::f(*p, x);`). Generated, never edited by hand except for enum casts
   (`static_cast<halo::math::periodic_function_type>(type)`).
3. **C++ API for new code**: value types with members and operators, classes (Strategy registries,
   `random_stream`), glm interop. All of it forwards to layer 1, so it is bit-exact by construction and is checked
   against layer 2 by `tests/math/math_api_test.cpp`.
4. **Engine globals**: declared once, `extern "C"`, in `include/halo/<module>/<module>_globals.h`; still defined by
   standalone/data / globals.asm, never by the module.

## 3. Types (one definition, C-compatible, value semantics in C++)

- The single definition of every type stays in `types/<module>.h`, because `standalone/data/*.c` compiles as C and
  includes it, and so do the tools. The C++ API reaches the same types through `using ::real_vector3d;` in
  `namespace halo::math`.
- **proposed:** C++ behaviour is added *inside* the struct under `#ifdef __cplusplus`: member functions, `static
  constexpr` factories (`identity()`), and hidden-friend operators. No data members, constructors, default member
  initialisers or virtuals, so the struct stays a standard-layout, trivially copyable aggregate (`{0}`, memcpy and
  designated/brace init keep working everywhere). Members whose bodies need engine functions are declared in the
  struct and defined in `include/halo/<module>/value_types.hpp`.
- Every type gets `static_assert`s for size, `offsetof` of every field, and `is_engine_pod_v<T>` (standard
  layout + trivially copyable + trivially default constructible + aggregate) in `math_types.hpp`.
- Enums: the C enums stay unscoped in `types/math.h` (their enumerators are used as integers by effects, shaders,
  units, game, ... and the selector is stored as `int16_t`). The C++ API uses scoped mirrors
  (`periodic_function_type`, `transition_function_type`, `enum class : int16_t`) whose values are pinned to the C
  enumerators with `static_assert`; the C wrappers `static_cast`. Constant enums used as integers by other modules
  (`k_random_multiplier`, ...) are not redeclared in the namespace: a namespaced constant with the same name would
  silently change the type of every expression in the module that uses it.

## 4. Comments

Docblock only (CPP_CONVENTIONS.md section 4). The pilot's generator writes the docblock from
`tools/cxx_work/<module>_docblocks.txt` (one `name | summary` line per function, written by hand once), adds
`Original register convention: ...` from the file's `blam-cc:` line and the `@address` tag. Docblocks sit on the
declarations in the public headers; definitions in the `.cpp` carry none. All inline comments are stripped from the
bodies; nothing is lost because every original file is copied verbatim to `docs/original/<module>/`.

## 5. Float exactness (how it was protected)

- Bodies were moved mechanically: only `p->` / `*p` on converted parameters change. No expression was reordered,
  fused or "simplified"; `(real)sqrt((double)x)` stays exactly that (the same CRT function, the same widening).
- The build is MSVC x86 /Od with SSE2 scalar float code: each float operation rounds to float, so two expressions
  are bit-identical exactly when they perform the same operations on the same operands with the same association.
  Commutativity is free (`a*b == b*a`, `a+b == b+a`), associativity is not.
- glm is used only where its formula has the original association (section 6) and only after the difftest proved it.
- `glm_interop.hpp` must be included only by `.cpp` files: glm pulls `<cmath>`, which adds float overloads
  (`fabs(float)`, `fmod(float, float)`, `sin(float)`) to the global namespace. Every CRT call in such a file passes an
  explicit `(double)` argument so overload resolution still picks the original double function (two calls in
  `real_vector.cpp` had float arguments and were changed to explicit casts).
- Constants: `#define K 6.2831855f` became `constexpr real k_two_pi = 6.2831855f;` (same value, same type).
- x87-specific code: the module has none left of its own; `__ftol` and `ROUND` stay external (harness/x87_shims.c) and
  are called exactly as before. `matrix4x3_multiply_sse` and `_3dnow` are scalar C reproductions of the SIMD routines
  and keep their symbols.

## 6. glm (third_party/glm 1.0.1, header-only, `GLM_FORCE_PURE`)

Interop: `to_glm(v)`, `vector_from_glm`, `point_from_glm`, `to_glm(real_matrix3x3)` (zero-copy `std::bit_cast`:
the struct's forward/left/up rows are glm's columns), `rotation_to_glm(real_matrix4x3)`, `matrix4x3_from_glm`,
`to_glm_affine` (a `glm::mat4` with the scale folded in, for new rendering code), quaternions by value
(`glm::quat::wxyz`; glm stores w separately).

glm-based engine functions (byte-identical to the original in the difftest):
`matrix4x3_transform_point`, `_transform_vector`, `_transform_normal`, `_transform_plane` (normal part),
`matrix4x3_inverse_transform_point`, `_inverse_transform_vector`, `_inverse_transform_normal`,
`matrix3x3_inverse_transform_vector`, `matrix3x3_transpose`, `matrix3x3_from_forward_up`, `matrix4x3_from_forward_up`,
`matrix4x3_multiply_3dnow`, `real_matrix4x3_rotation_from_forward`, `real_matrix4x3_rotation_rebuild_orthonormal`
(their cross products), `vector3d_magnitude_squared`, `vector3d_scalar_triple_product`, `point3d_add_scaled`,
`vector3d_lerp`, `plane3d_from_point_and_normal`, `plane3d_negate` (20 functions).
Why they match: glm's `mat3 * vec3` is `(m[0][r]*x + m[1][r]*y) + m[2][r]*z`, `vec3 * mat3` is the row dot in i, j, k
order, `mat3 * mat3` sums the three products left to right, `dot` is `(x*x' + y*y') + z*z'`, `cross` is the textbook
formula; these are exactly the orders the original code (and the 3DNow! routine) use.

Kept original (glm would change bits; tried, difftest failed or the order differs):
- every sum written k, j, i or in another order: `vector3d_length`, `vector3d_normalize*`, `vector3d_distance*`,
  `matrix4x3_from_quaternion`, `quaternion_*`, `matrix3x3_multiply`, `matrix4x3_multiply` and `_sse` (term order
  `u*k + f*i + l*j` per component; tried, 990 lines differed), the transform-plane distance term;
- `vector3d_cross_product`: same formula as `glm::cross(b, a)`, but the original writes `out.i` before reading the
  inputs again, so an aliased call (`out == a`) gives different bits (tried, failed on the alias case);
- anything with x87-era special cases, clamps, NaN-ordered comparisons or table lookups (polygon, ray, segment,
  periodic, servo code): no glm equivalent, original kept.

glm's index `assert`s compile to `_wassert` calls with absolute source paths in this NDEBUG-less build;
`glm_interop.hpp` includes glm with `NDEBUG` pushed on, then restores `assert`.

## 7. OOP and patterns used in math

- Value types: vectors, points, planes, quaternions, 3x3 and 4x3 matrices with members and operators (section 3).
- Strategy + registry: `periodic_wave` (twelve waves; jitter and wander share one) and `transition_curve` (six
  curves; the four power curves are one class with an exponent) replace the switches in the table builders. The
  objects are `const` statics with constant initialisation; out-of-range selectors return `nullptr` and the builder
  keeps the previous sample, exactly as the original switch's `default` did.
- `random_stream`: a non-owning view of an LCG state (`simulation_random()` = random_seed_global,
  `effect_random()` = effect_random_seed, or a caller's seed). Each draw method reproduces the engine's formula
  and association (`next_real(min, max)` is `(max - min) * high16 * 2^-16 + min`, not `lerp(min, max, next_real())`).
- Not converted to classes: sphere mesh and polygon clipping (array/count interfaces called through register
  conventions; candidates for `std::span` overloads later).

## 8. Verification (run in this order)

1. `python tools/check_c_abi_symbols.py <module>`: C-linkage symbols identical to `symbols/exports/<module>.txt`;
   C++ symbols allowed only in `halo`, `glm`, `std`. **proposed:** use this classification in
   `tools/check_module_symbols.py check`: today it reports every converted module as "extra" (the namespaced
   functions) and "missing" whenever a literal pool disappears (math: `__real@3fd0000000000000`, `@4000...`,
   `@4010...`, the pow exponents now stored in `power_curve` objects), neither of which affects linking.
2. `python tests/math/run_difftest.py`: builds the original module from git (`--ref`, default c54d5e1f) and the
   converted one with the same flags, the same driver, stubs and x87 shims, compares the outputs byte for byte, then
   runs `math_api_test` (the C++ API against the C functions). Also `cmake -DHALO_BUILD_TESTS=ON` + `ctest -C
   Release` (two tests: `math_difftest`, `math_api_test`).
3. Whole tree compiles: `python tools/cxx_probe.py` (for `.c` files; 5045 files, 0 fail after the types/math.h
   change) and the `halo_game` target in a pilot CMake build dir (all 5,437 sources compiled, 0 errors).
4. Sensitivity check of the difftest: reordering one sum (`vector3d_length` i,j,k instead of k,j,i) makes it fail.

Coverage: 120 of 124 functions are called directly. `sphere_mesh_build_face`, `_get_face_point`,
`_get_edge_point` and `_interpolate_vertex` are internal steps that need a half-built mesh; they are covered through
`sphere_mesh_generate` (subdivisions 1..8, every point and every written index compared). `random_seed_generate`
reads QueryPerformanceCounter: the stub sets `performance_frequency` to INT64_MAX so the timer terms are 0 and the
function reduces to its `rand()` term, which is compared.

## 9. Recipe for the next module

1. `git merge cxx-phase1`; baseline gate passes on the unconverted module.
2. Write `GROUPS` / `GROUP_TITLES` for the module in `tools/cxx_convert_module.py` and the summaries in
   `tools/cxx_work/<module>_docblocks.txt`.
3. `python tools/cxx_convert_module.py <module>`; fix what does not compile by adding entries to `PATCHES` (keep the
   output reproducible: `--from-ref <commit>` regenerates from git after the `.c` files are gone); `git rm` the `.c`.
4. Gates: `check_c_abi_symbols.py`, then a difftest (copy tests/math: driver calling every pure export with
   fixed-seed inputs, stubs identical for both builds), then commit the mechanical pass.
5. Hand work on top, re-running both gates after every step: const-correct pointer inputs, scoped enums in the C++
   API, globals header, value-type members, Strategy/registry for switch-on-type, glm where proven identical.
6. Docblocks only; nothing but forwarding in the C wrappers.

## 10. Pitfalls hit

1. **Function addresses inside the namespace.** `matrix4x3_multiply_procedure = matrix4x3_multiply;` inside
   `namespace halo::math` took the address of the C++ function, not the C symbol. Symbols were identical, only the
   difftest caught it. Rule: an address stored in engine data, a table or a callback is always `::name`. The
   converter now does this automatically (and includes the C API header for it).
2. **Member names shadowing parameter names.** A parameter called `up` looks "used" in `out->up`; the converter now
   ignores member accesses when deciding whether a parameter can become a reference.
3. **Disagreeing local externs.** One file declared `bounded_ramp_profile_*` with `uint8_t *` and kept its profiles
   in `uint8_t[0x20]`; merged into one file the real signature wins, so the buffer became the 0x20-byte struct
   (a `PATCHES` entry). Expect this in every module: the one-function files declared their callees independently.
4. **Ambiguous calls.** A TU that sees both the C API header and the namespace (via `using namespace halo::math`)
   gets an ambiguity for every function whose two signatures are identical (all-scalar parameters, e.g.
   `random_real()`). Implementation files include the C API header only where they take a C address (math_initialize.cpp) and never use `using namespace`; tests qualify C calls with `::`.
5. **`<math.h>` is shadowed by types/math.h.** Any header that includes `<cmath>` (glm, `<limits>` users) breaks
   unless `<corecrt_math.h>` is included first; and then float overloads appear (section 5).
6. **Uninitialised bytes in test outputs.** Padding, unwritten tail elements of output arrays and GlobalAlloc'd memory
   differ between two executables; the driver pre-fills every output with 0xcd and prints only written ranges.
7. **Register-convention inputs need their real domain.** `decal_plane_solve_third_axis` reads only the low byte of
   EAX as a 0/1 flag; random 32-bit values index far outside the table and crash both builds. Test inputs must
   respect the documented register contract.
8. **Scratchpad and stash are shared between agents.** Keep tools in a per-task subdirectory.

## 11. Recommended next modules

1. `cseries` / `memory` (foundation, few float paths): exercises the globals header, RAII for allocators and the
   symbol gate on data-heavy code before simulation modules depend on the new headers.
2. `physics` or `objects` geometry helpers: the heaviest users of the math value types, so they show whether the
   member/operator API is enough for real call sites; keep the bodies mechanical and difftest the pure helpers.
