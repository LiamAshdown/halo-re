# C++ conversion report (2026-10-03, branch `cxx-phase1`)

## Where the project stands

- Every engine source file except GameSpy (kept as vendored C) is now C++20 under `src/<module>/*.cpp` (565 files, about 312k
  lines) with 322 public headers in `include/halo/<module>/`. Before: 5,090 one-function `.c` files. `src/*/*.c` outside
  `src/gamespy` is empty.
- The standalone exe builds, links and runs: seven integration builds in a row (each with the retail binary and the Ghidra
  exports hidden from the build) linked and ran for 2 minutes with no exception and no "MISSING FUNCTION" in
  `halo_standalone.log`.
- There are no `.asm` files left anywhere in the repository. The former `globals.asm` (2,087 fixed-address globals) is C data in
  `standalone/data/*.c`; `code_entries.asm` and `bridges.asm` are `standalone/generated/code_entries.c` and
  `standalone/bridges.cpp`; the data image is `standalone/image/halo_image_*.c`; the hook harness trampoline is
  `harness/difftest_call.c` (naked inline asm). A handful of source files still contain inline `__asm` blocks (cpu id, x87).
- Nothing is on `master` or pushed. All of it is on branch `cxx-phase1` (146 commits ahead of master).

## How each module was converted

Rules (docs/CPP_CONVENTIONS.md, binding): every original C symbol still exists through a thin `extern "C"` shim with the original
name and signature (`tools/check_module_symbols.py check <module>` proves it; baselines in `symbols/exports/`), record layouts and
sizes are unchanged, function bodies were moved without changes to arithmetic or order, comments are method docblocks only, and the old
notes and `#if 0` decompile blocks live in `docs/original/<module>/` (4,392 files).

Design that exists today:
- View/handle classes (`UnitView`, `ObjectRef`, `ActorView`, `ChannelView`, ...) with member functions, in `namespace halo::<module>`.
- Real interfaces: `AudioDevice`/`DirectSoundDevice` and `EaxBackend` hierarchy (sound), `RenderDevice`/`D3D9Device` (rasterizer, about 770
  Direct3D call sites go through it), `SettingsStore`/`CrashReporter`/`HardwareProbe` with Win32 implementations (shell),
  `GameEngineSlots` with `SlayerEngine`/`OddballEngine`/`RaceEngine` (game), `ActorMode`/`ActorTypeBehavior` registries (ai).
- Strategy/Command/State: message-delta `FieldCodec` registry, client/server message handler registries and a client state machine
  (networking), `ServerCommand` registry for the `sv_*` commands, server browser sort strategies, hs command groups, chat line source
  strategy and waypoint visitor (interface), dialogue condition / platoon condition / aim solver strategies (ai), text encoding strategy.
- Math (the pilot) uses glm for 20 functions, proven bit-identical by a 46,042-call old-vs-new comparison (`tests/math`).

## What is verified, and what is not

Verified: link, symbol-set equality per module, start-up and the first level frames for 2 minutes, math bit-exactness.
Not verified: rendering, audio, input and AI/hs behaviour beyond "it runs". The conversion moved bodies unchanged, so equivalence is by
construction, but there is no golden-frame test, no determinism replay and no per-function difftest for the converted modules yet.
Risky areas to test by playing: the rasterizer `RenderDevice` rewrite, sound (`audio_device()` routing, two small changes in
`channel_set_parameters`/`sound_initialize`), the `globals` conversion (below), and the shell registry code.

## Known gaps and findings

1. The retail data image is still copied by the loader. About 20 tables (hs function definitions, message-delta definitions, rasterizer effect
   and shader tables, hs enum definitions, campaign levels, object type definitions, game engine definitions, ...) still hold raw pointers
   into unnamed read-only data in that image. Fully removing it means giving every such target its own C object and retargeting the
   pointer. Scanners: `tools/scan_stale_addresses.py`, `tools/scan_data_stale_pointers.py`, `tools/scan_stale_into_globals.py`. An experiment
   (loader leaves the old range unmapped) crashes at start-up, which is how the stale uses were found; two real ones were fixed
   (frustum/camera globals referenced by raw address; keystone locale format declared as pointer but defined as array).
2. `DAT_00000050` (a store to absolute address 0x50 in `network_machine_clear_flag_by_id`) is now a 1-byte variable; the original faulted.
3. Behaviour changes made by agents and worth review: `tree_erase_range` now passes the root to `tree_destroy_subtree` (the old call had a
   missing argument), `DirectSoundDevice::channel_set_parameters` takes 3 arguments (2 ignored ones dropped), `sound_initialize` goes through
   `audio_device()`. Several original bugs were deliberately left alone and are listed in the agents' reports (for example the swapped
   `RegCreateKeyExA` arguments, the inverted gain clamps in `sound_set_master_gain`, a stack overflow in `server_browser_open`).
4. Docblocks: many functions only have a generic one-line docblock plus `@address`. The full old notes are in `docs/original`.
5. Registries (hs command tables, `WidgetEventRegistry`, ...) exist beside the original data tables but are not consumed by the engine yet;
   the dispatch tables are fixed C data in `standalone/data`.
6. `harness/gen_hooks.py` / `harness/gen_link.py` and the regenerate tools still scan `src/*/*.c` for `blam-cc` comments and address
   comments; they no longer see converted modules. The standalone link tables are committed and unaffected.
7. The 390 formerly overlapping fixed-address globals use ordered linker sections (`.geq$...`) to keep their original relative layout; the
   layout is only as exact as the agent's analysis (checker `tools/globals_check_eq.py` has not been run).

## Next steps (recommended order)

1. Play it: campaign level start to finish, a multiplayer match, menus, sound. Compare with the retail build.
2. Build a render golden-frame harness and a determinism replay (docs/CPP_ARCHITECTURE.md section 9) before further refactors.
3. Remove the old data image (finding 1) and then the fixed 0x630000 reservation in `standalone/loader.c`.
4. Docblock pass, then the design pass that actually drives the registries and removes the shims where nothing outside needs the C name.
5. OpenGL backend behind `RenderDevice`, miniaudio behind `AudioDevice` (decisions in docs/CPP_ARCHITECTURE.md section 11).
6. Decide when to merge `cxx-phase1` to `master` and push.

## How to build and check

    cmake -S . -B build/cxx -G "Visual Studio 17 2022" -A Win32 -DHALO_REGENERATE=OFF
    cmake --build build/cxx --config Release --parallel
    python tools/check_module_symbols.py check <module> ...      # symbol equality against symbols/exports/
    python tools/scan_data_stale_pointers.py                       # stale old-address dwords in the linked data
The status ledger with every merge, build and decision is `docs/CONVERSION_STATUS.md`.
