# halo-re

A C++20 reimplementation of Halo PC (1.0.10) built from our own readable source. The executable runs against an installed
copy of Halo PC for its game data (maps, bitmaps.map, sounds.map, the bink and vorbis DLLs); it does not read the retail
`halo.exe`. GameSpy is vendored C source compiled into the engine.

## Layout

| Path | Contents |
|---|---|
| `src/<module>/` | The engine, one directory per module (ai, game, objects, units, rasterizer, networking, ...). `src/gamespy` is the vendored GameSpy C code. |
| `include/halo/<module>/` | Public headers per module: `api.hpp` (the `halo::<module>` functions other modules call), `globals()`/`vars()` accessors, flag enums, record layouts. |
| `include/halo/core/` | Shared building blocks: `flags.hpp` (enum class flag helpers), `datum.hpp`, `link.hpp`, `libm.hpp`, `x87.hpp`. |
| `types/` | Engine record, tag and map structure headers. |
| `standalone/` | The loader, the readable data tables the game reads (`data/`), and the link names of engine variables (`data/link/`). |
| `tools/` | `map_inspect/` (offline .map reader and validator), `convert_fx.py` (shader conversion), `modernization_census.py`, `stride_audit.py`, disassembly helpers. |
| `docs/` | `CPP_ARCHITECTURE.md`, `CPP_CONVENTIONS.md`, `MODERNIZATION.md` (history and log), `MODERNIZATION_BRIEF.md`. |
| `scripts/` | Ghidra Java scripts used for reverse engineering. |
| `third_party/glm/` | GLM, used for matrix maths. |

## Build

Needs Visual Studio 2022 (x86 C++ tools) and the DirectX SDK (June 2010).

    cmake -S . -B build/cxx -G "Visual Studio 17 2022" -A Win32
    cmake --build build/cxx --config Release --parallel

Outputs in `build/cxx/Release/`: `halo_rebuilt.exe` and `map_inspect.exe`.

The game needs the converted shader file `override\shaders\fx.bin` next to the exe. Produce it from your install with
`python tools/convert_fx.py` (see the top of `CMakeLists.txt` for `HALO_FX_OVERRIDE` and `HALO_FOLDER`).

## Run

    build\cxx\Release\halo_rebuilt.exe -window

The loader finds the Halo install at `HALO_FOLDER` (default `C:\Program Files (x86)\Microsoft Games\Halo`). Diagnostics are
written to `halo_standalone.log` next to the exe. Setting `HALO_HEAPCHECK=1` makes the main loop validate the process heap
each frame and log the first point where it is found corrupt.

## Inspect map files

    map_inspect --check "C:\Program Files (x86)\Microsoft Games\Halo\MAPS"
    map_inspect <file.map> [--tags]
    map_inspect --data <bitmaps.map|sounds.map> [--list]

## Conventions

See `docs/CPP_CONVENTIONS.md`. In short: comments are method docblocks only (one or two paragraphs, `@address` for the
original function address), no raw C linkage outside the GameSpy boundary, flags are `enum class` types, and cross-module
calls go through `halo::<module>` API headers.
