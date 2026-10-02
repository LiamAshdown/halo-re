# C++ conversion: architecture and coding standards (draft for review)

Status: PLAN, decisions recorded in section 11 (2026-10-03). No code has been changed for it yet; the globals-to-C integration build is green.

## 1. What we are converting (measured 2026-10-03)

- 5,090 `.c` files under `src/`, one engine function each, about 700k lines, plus about 30k lines of `types/*.h`.
- Biggest modules (files / lines): ai 625 / 92k, hs 625 / 30k, networking 556 / 58k, interface 537 / 63k, game 521 / 63k, gamespy 379 / 14k, objects 271 / 31k, units 260 / 41k, rasterizer 239 / 45k, sound 169 / 35k.
- Platform surface: Direct3D 9 / D3DX in 40 files, DirectSound/EAX in about 24, DirectInput in about 25, Winsock/GameSpy networking, Win32 threads/files in about 54. Bink and Vorbis are DLLs.
- The code is a decompilation: many functions take arguments in registers (`// blam-cc:` lines), globals live in fixed layouts, and every struct that touches a map file, a saved game, a network packet or the cache is a fixed binary layout.

Consequence: this is an incremental, behaviour-preserving refactor of a 700k-line engine, not a rewrite. Anything that changes behaviour or layout needs proof, and the decompiled functions are still the specification until we have our own tests.

## 2. Hard constraints (never negotiable)

1. Binary formats do not change: `.map` cache files, tag structs in `types/tags.h`, saved games, network message layouts, `fx.bin`. Every such struct stays standard-layout, no virtuals, no non-trivial members, and keeps a `static_assert(sizeof(...))` plus `offsetof` checks for known fields.
2. Game state memory is owned by the engine's own pools (`data_array`, game_state allocator), not by `std::` containers. Boost/STL containers are for tool code, UI scratch and caches, never for state that is saved, networked or checksummed.
3. Determinism: simulation code keeps its x87/float semantics and RNG draw order. No `-ffast-math`, no reordering of random draws, no replacing math primitives with library versions without a difftest.
4. 32-bit Windows (x86, MSVC) stays the only target until the project is verified; 64-bit comes later as its own project because pointers inside saved structs are 4 bytes.
5. One conversion step lands per branch with the build green and the standalone exe booting a level. No big-bang.

## 3. Language standard and toolchain

- C++20 (MSVC 2022, `/std:c++20`). Rationale: concepts, `std::span`, `std::bit_cast`, designated initialisers, `constexpr` tables; no modules, no coroutines, no ranges in hot code.
- Compiler flags: keep `/Od`-shape parity during the conversion (current build comment says it is for a 1:1 shape with the original); move to `/O2` only per module after difftest passes. `/W4`, warnings as errors per converted module.
- Exceptions: OFF (`/EHs-c-`) in engine code. Errors are returned (`std::expected` is C++23, so use a small `result<T>` type until we move up). RTTI OFF. Boost components used must be header-only or compile without exceptions.
- Build: CMake (already present) with one static library per module (`halo_ai`, `halo_hs`, ...) instead of the single `halo_game` object library. `vcpkg` manifest for third-party libs (pinned baseline).
- Formatting: `clang-format` (config checked in), `clang-tidy` with a short curated check set. Both advisory at first, enforced per module once converted.

## 4. Target source layout

```
include/halo/<module>/...   public headers per module (what other modules may include)
src/<module>/               implementation, one .cpp per cohesive group (not one per function)
platform/                   all OS and API specifics, behind small interfaces
  window/  input/  audio/  filesystem/  net/  time/  threads/  crash/
render/
  core/                     renderer-agnostic: draw lists, materials, passes, sort keys
  backend_d3d9/             today's rasterizer code, behind the Renderer interface
  backend_gl/               OpenGL backend (section 6)
third_party/                vendored or vcpkg shims (GameSpy stays vendored, it is old C)
tests/                      unit, golden-data and difftest
tools/                      Python and C++ dev tools (unchanged location)
```

Module dependency rule (checked by a CMake link graph, one-way only):

`platform` and `math/memory/cseries` (foundation) -> `cache/tags/scenario/structures` (data) -> `objects/physics/units/items/projectiles/effects/devices` (world) -> `ai/hs/game/cutscene/camera/saved_games` (simulation) -> `render/interface/sound/input` (presentation) -> `main/shell` (app).
Simulation modules never include render/sound/input headers; they publish events or read state through interfaces. This is the rule that makes the OpenGL work possible without touching gameplay code.

## 5. Conversion strategy, phase by phase

Each phase has an exit test. Order is chosen so risk stays low and value accrues early.

| Phase | Work | Exit test |
|---|---|---|
| 0 | Finish globals-to-C (the 379 overlapping `EQU` objects), delete all `.asm`, clean CMake build with retail files hidden | clean build + boots a level (in progress) |
| 1 | Compile everything as C++ unchanged (`extern "C"`, `/TP` per module). Fix what C++ rejects: implicit `void*` casts, designated init order, `new`/`class` identifiers, const-correctness, uninitialised jumps over initialisation | all modules build as C++ with `/W3`; exe boots |
| 2 | Per-module headers: `types/*.h` split into `include/halo/<module>/*.h`, enums become `enum class` with `static_assert` on size, typedef'd structs become `struct`, macros become `constexpr` / `inline`. Introduce namespaces `halo::<module>` and keep `extern "C"` shims only where the register-convention thunks need them | module builds with namespace; no behaviour change |
| 3 | Merge one-function-per-file into cohesive `.cpp` files per subsystem (generated mechanically; keep function order). Keep the decompile-comment blocks as docs or move to `docs/original/` | file count drops 5,090 -> a few hundred; link table unchanged |
| 4 | Introduce types: `handle<T>` (datum index with salt), `fixed_string<N>`, `span`-based table access, `real_vector3d` operators, `rect`/`point` value types, RAII wrappers for files/locks. Only layout-safe types here | difftest of affected functions identical |
| 5 | Replace Win32 and COM calls with `platform/` interfaces, per section 7 | each platform area ships behind an interface with the old implementation as default |
| 6 | Renderer abstraction (section 6) | D3D9 backend renders identically (golden screenshots) |
| 7 | OpenGL backend | passes the same golden screenshots within tolerance |
| 8 | Optional: modern idioms inside simulation modules (algorithms, ranges), unit tests per module, `/O2` | determinism replay test (section 9) |

Do not start phase N+1 for a module until phase N is green for it. Different modules can be at different phases.

## 6. Rendering and OpenGL

Today: `src/rasterizer` (239 files) is a Direct3D 9 back end driven by fixed-function state and 122 compiled effects in `fx.bin` (pixel-shader 1.x era, converted to `fx_2_0` by `tools/convert_fx.py` because the 2003 D3DX is no longer linked). `render` builds draw lists, `interface` draws HUD and menus through the rasterizer.

Plan:

1. Define `halo::render::Renderer`, a thin interface in `render/core` exposing only what the engine uses: create/destroy textures, vertex/index buffers, render targets, shaders (by effect id), set render state blocks, draw primitives, present, query caps. Derive it from the existing rasterizer calls (count the real surface first: list every `IDirect3DDevice9` method actually called).
2. Move the current code behind it as `backend_d3d9`. This is the reference; the OpenGL backend is judged against it.
3. OpenGL backend: OpenGL 4.5 core. Libraries: `glad` (loader), `SDL3` or `GLFW` (window, GL context, input events), `glm` is NOT used for engine math (the engine's own math stays; it must match the original float behaviour), but used inside the backend if convenient.
4. Shaders: write the 122 effects as GLSL by hand/with a generator from the decoded `fx_2_0` state data (`convert_fx.py` already parses techniques, passes, states). Expect a table-driven translator for the fixed-function and ps1.x subset, plus hand work for the odd effects (water, lens flares, fog, lightmaps). This is the largest single risk; start with a spike on 3 effects before committing.
5. Coordinate conventions: D3D clip space and texel-centre rules differ from GL. Put the fix in one place (backend) with `glClipControl` and a half-pixel convention test, never in engine code.
6. Verification: a deterministic "render capture" harness that replays a fixed camera and game tick, and compares frames from both backends (PSNR/diff image). Golden images are stored in `tests/golden/` and are regenerated only on purpose.
7. Keep D3D9 selectable at runtime (`--renderer=d3d9|gl`) until GL reaches parity; remove D3D only if you decide to.


## 7. Libraries: what to adopt, what not

| Need | Choice | Why / limits |
|---|---|---|
| Window, GL context, input events, gamepad | SDL3 (alternative: GLFW) | replaces `shell` window code and DirectInput; map to the engine's input_globals, keep bindings semantics |
| Audio | miniaudio or OpenAL Soft (+ libvorbis/ogg) | DirectSound/EAX are gone on modern Windows; EAX reverb needs a mapping, accept a parity gap and document it |
| Video | Bink DLL stays (binkw32) | closed format; no replacement. Wrap behind `VideoPlayer` |
| Networking sockets | Boost.Asio (or plain Winsock behind `platform/net`) | engine protocol is custom UDP plus GameSpy. Must stay packet-compatible with retail if interop is a goal; Asio gives us timers/async but exceptions must be off (`error_code` API). Optional, not required for phase 5 |
| GameSpy | keep vendored C (`src/gamespy`, 379 files) | master-server services are mostly dead; leave it as a library, not converted, until you decide a replacement master server |
| Containers/utilities | Boost (headers only): `boost::container::static_vector`/`small_vector`, `boost::intrusive`, `boost::circular_buffer`, `boost::endian`, `boost::crc` | only for non-state data. Prefer `std::` where equal. Do not use `boost::shared_ptr` or `std::shared_ptr` in the engine; ownership is explicit pools and RAII |
| Strings/format | `std::format` (C++20) or `fmt` | engine uses fixed `char[N]` and wide strings in data; keep those types for layout, use format for logging and debug text |
| Logging | `spdlog` (header-only mode) behind a `LOG_*` macro | replaces `halo_standalone.log` plumbing; one sink at start-up |
| Tests | Catch2 v3 (or doctest) + golden-data helpers | section 9 |
| Config/CLI | `CLI11` or `Boost.Program_options` | replaces ad-hoc `-devmode` parsing; Boost.PO needs RTTI so prefer CLI11 |
| Math | keep the engine's `real_*` math in `math/` | must stay bit-identical; no GLM/Eigen in simulation |
| Compression | zlib (already linked) | unchanged |

Rule: every dependency goes through vcpkg, is pinned, and is wrapped by a project interface so it can be swapped. No library types in public module headers except inside `platform/` and `render/backend_*`.

## 8. Coding standards

Naming and style
- Types `snake_case` is what the engine uses today; the C++ code keeps `snake_case` for engine types and functions (it keeps the original names greppable) and uses `PascalCase` only for new platform/backend class names. Constants `k_name`; enum class values `snake_case`. Namespaces `halo::<module>`. Files `snake_case.cpp/.hpp`.
- No `using namespace` in headers. `#pragma once`. Include order: own header, `halo/` headers, third-party, std.
- One public header per module exposes the module API; internals live in `src/<module>/internal/` and are not includable from other modules.

Language usage
- Prefer `struct` of data plus free functions in `halo::<module>`; use classes only for RAII wrappers and platform/backend objects with real invariants.
- No virtual functions in anything stored in game state. Virtuals are allowed in platform/backend interfaces (a handful of interfaces, created once).
- `enum class` with explicit underlying type for every enum; `static_assert(sizeof(T) == N)` after every serialised struct; `offsetof` checks for fields that code or data depend on.
- `constexpr` for tables where possible; mutable global state is declared in one `*_globals.cpp` per module and accessed through the module's header.
- Ownership: raw non-owning pointers/spans are fine; owning raw pointers only inside pool/allocator code. No `new`/`delete` outside memory modules.
- No exceptions, no RTTI. `assert`-style checks through `HALO_ASSERT`; fatal errors through one `halo::fatal()` that logs and calls the existing crash reporter.
- Casts: `static_cast`/`reinterpret_cast`/`std::bit_cast` only; a `reinterpret_cast` needs a comment naming the layout it assumes.
- Decompilation notes: keep the original address in a comment on every converted function (`// original 0x00xxxxxx`) so `tools/` and difftests can keep mapping; never delete the notes while a function is unverified.

Reviews and automation
- CI gate per PR: build (x86 Release) with retail files hidden, `clang-format --dry-run`, `clang-tidy` on changed files, unit tests, size assertions, boot test.
- Commit style: one subsystem per commit, message prefix `module:`; mechanical changes (rename, move, reformat) are separate commits from behaviour changes so `git blame` and review stay usable.

## 9. Verification (how we know nothing broke)

1. Struct layout: `static_assert` for every serialised struct, and a generated test that compares `offsetof` tables against `types/` golden data (`tools/struct_offsets.py` exists).
2. Function equivalence: the existing difftest/harness (`harness/`, `tools/emu_difftest.py`) for pure functions during phases 1 to 4.
3. Determinism replay: record inputs for a fixed campaign segment and a multiplayer match; replay and compare per-tick checksums of game state. This is the main regression net for phases 4, 5 and 8.
4. Golden frames for rendering (section 6).
5. Boot/smoke: standalone exe starts, loads a map, runs N ticks without "MISSING FUNCTION" in the log.
6. Save/load and network compatibility: round-trip a saved game through the new build and through retail; compare packet captures for a short session.

## 10. Risks (ranked)

1. OpenGL shader translation of the 122 effects (long tail of odd effects). Mitigation: spike first, hand-write the hard ones, keep D3D9 backend.
2. Hidden layout dependence in global tables (the 379 remaining overlapping objects from globals-to-C). Mitigation: finish phase 0 with explicit layout checks before touching types.
3. Behavioural drift from "cleanup" in decompiled code. Mitigation: mechanical edits only until a module has replay coverage.
4. Scale: 5,090 files. Mitigation: scripted conversions (phase 1 to 3 are mostly mechanical) and per-module progress tracking in a table in this document.
5. Third-party churn (SDL, GL, audio, EAX parity). Mitigation: interfaces in `platform/`, pinned vcpkg versions.

## 11. Decisions (answered 2026-10-03)

1. C++20. Confirmed.
2. Non-Windows support IS a goal. Consequences: SDL3 for window/input/GL context (not GLFW-only Win32 paths); every OS API goes through `platform/` with a Windows and a POSIX implementation; no MSVC-only constructs in new code (`__declspec`, SEH, inline `__asm`, `#pragma comment`) outside `platform/windows/`; the 7 files with inline `__asm` and `harness/x87_shims.c` need portable replacements, with bit-exact float behaviour proven by difftest; GCC/Clang added to CI next to MSVC. Keep the 32-bit target for the first pass (struct layouts and 4-byte pointers inside saved data), so a Linux build starts as `-m32`; a later 64-bit project replaces in-struct pointers with handles/offsets.
3. Direct3D 9 stays until the OpenGL backend matches it (golden frames pass), then it is removed. The `Renderer` interface and the D3D9 backend stay in the tree until then.
4. GameSpy stays as is (vendored C, not converted, not wrapped beyond compiling as a C library with `extern "C"`).
5. `ai` and `hs` are converted in the first pass. Because they are the biggest, behaviour-critical modules and replay tests do not exist yet, build the determinism replay harness (section 9, item 3) BEFORE phase 2 starts on them, and convert them mechanically (phases 1 to 3 only) with the difftest harness as the gate; no behavioural cleanups until replay coverage exists.
6. Audio: miniaudio (plus libvorbis/ogg for Vorbis data). EAX reverb has no direct equivalent; document the parity gap and use miniaudio's node graph for a simple reverb as a later task.

## 12. Standards added 2026-10-03

- Comments: a method docblock of one or two short paragraphs per function (what it does, and any non-obvious contract such as register convention, units or caller assumptions), with one `@address 0x00xxxxxx` tag line. No inline commentary, no evidence or UNSURE essays. The existing long notes and the `#if 0` original-decompile blocks move to `docs/original/<module>/` verbatim.
- glm: used for matrix and vector math in the new `halo::<module>` APIs, the camera/render code and the OpenGL backend. Stored and serialised types (`real_matrix3x3`, `real_matrix4x3` and similar) keep their layout and size and are converted at the boundary. A function's body switches to glm only after its byte-for-byte output test matches the original build, because simulation math must stay bit-exact; where glm changes the bits the original arithmetic stays and the reason is recorded in `docs/CPP_CONVENTIONS.md`.
