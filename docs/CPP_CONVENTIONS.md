# C++ conventions for the halo-re conversion (v1, binding for all module conversions)

Read docs/CPP_ARCHITECTURE.md for the why. This file is the how. "Modern C++" here means real object-oriented design
and design patterns, applied inside two hard limits: binary layouts must not change, and every original C symbol must
stay reachable at its original address (the link tables in standalone/data/link/ bind them).

## 1. What a converted module looks like

```
include/halo/<module>/<topic>.hpp   public API: classes, interfaces, enums, free functions in namespace halo::<module>
src/<module>/<topic>.cpp            implementation, a few cohesive files per module (not one file per function)
include/halo/<module>/api.hpp       every function other modules call, declared once in namespace halo::<module>
include/halo/<module>/vars.hpp      halo::<module>::vars(): the address table of the engine variables the module owns
types/<module>.h                    kept as the compatibility header other (unconverted) modules include; it may include
                                    the new headers; it must keep exporting every type name other modules use
docs/original/<module>/             the old author notes and the `#if 0` decompile blocks, moved verbatim
```

Procedure for one module (do it in this order, commit after each step that builds):
1. Baseline: the full build links before you start.
2. Types first. Record structs that live in data arrays or are serialised become classes/structs with member functions:
   same members in the same order, same size, standard-layout, trivially copyable, no virtual functions, no non-static
   members added, no constructors that run on pool memory. Add `static_assert(sizeof(T) == N)` and `offsetof` checks.
   Operations whose first parameter is that record become member functions (`actor_kill(actor *a)` -> `void actor::kill()`).
3. Behaviour objects. Where the original uses function-pointer tables or switch-on-type dispatch (actor type procs,
   object type definitions, message-delta field types, game engine definitions, hs function tables, HUD widgets,
   rasterizer effects) introduce an abstract interface and concrete classes registered in a registry (Strategy/Factory/
   Template Method). These objects are created at start-up in static storage; they are never stored in saved or networked
   memory. Dispatch must call the same code in the same order as the original.
4. Module state. Engine variables at fixed addresses are published by their owner in `halo::<module>::vars()` (the interface
   module's accessor is `halo::ui::vars()`); a file that uses one binds a typed reference with
   `static auto &X = halo::link::ref<T>(halo::<owner>::vars().X);` (include/halo/core/link.hpp). State the module
   allocates itself lives in a service singleton reached through an accessor (`halo::<module>::globals()`). Never write
   an `extern` declaration of a variable or function in a .cpp file.
5. Resources. Files, handles, locks, COM objects, scratch buffers get RAII wrappers (`halo::platform` where OS-specific).
   Never introduce exceptions, RTTI, `new`/`delete` in simulation code, or `std::shared_ptr`.
6. API headers. Every function another module calls is declared in `include/halo/<module>/api.hpp`; callers include it and
   call `halo::<module>::name(...)`. The only `extern "C"` code left is the GameSpy boundary (src/cseries/cseries_api.cpp,
   src/memory/memory_c_api.cpp), the vendored GameSpy sources and standalone/data/link/*.
7. Move the old notes to docs/original/<module>/ and make sure the full build links.
8. Never run the exe; verify with `cmake --build build/<dir> --config Release --parallel`.

## 2. Design patterns to use (and where they are unsafe)

Use: RAII, Strategy, Factory + registry, Template Method, State (ai behaviours), Observer/event dispatch for gameplay
events, Command (hs script functions), Iterator/range adapters over data arrays, value types with operators for vectors,
`std::span` for table access, `std::array` for fixed tables, `enum class` with underlying types, `constexpr` tables,
`[[nodiscard]]`, `noexcept` where nothing can throw, `static_assert`.
Not allowed: anything that changes memory layout of pooled/serialised data, hidden allocation in per-tick code,
reordering of floating-point arithmetic or random draws (the simulation is bit-exact sensitive), changing evaluation
order of side effects, replacing CRT/math calls with different library functions.
glm: use for matrix/vector math in new rendering, camera and backend code; in simulation code only where the output is
proven bit-identical.

### Linkage of the C++ API
All engine functions are normal C++ functions in `namespace halo::<module>`, declared in the module's api.hpp. There is no C
symbol layer: the link tables in standalone/data/link/ reference the C++ functions and the vars.hpp address tables.

## 3. Naming and style

snake_case for functions, variables, namespaces and engine types (original names stay greppable); PascalCase for new
class names that are patterns/interfaces (e.g. `ActorBehavior`, `ObjectFactory`); `k_` prefix for constants;
`m_`-free members (plain names). Namespace `halo::<module>`. Headers use `#pragma once`. 4-space indent, no tabs.
Include order: own header, halo headers, third-party, standard library.

## 4. Comments (strict)

One docblock per function/class/interface: one or two short paragraphs saying what it does and any non-obvious
contract (units, register-convention origin, caller assumptions), with a single `@address 0x00xxxxxx` line for functions
that came from the original. No inline commentary, no evidence/UNSURE essays, no commented-out code, no restating the
code. Longer notes live in docs/original/<module>/.

## 5. Verification summary

- The full Release build compiles and links.
- `python tools/modernization_census.py` reports 0 for extern_c and extern_decl outside the GameSpy boundary.
- The lead builds everything and runs the exe after each wave; if a wave breaks the link or the smoke test the offending
  module is reverted.
- Pure modules additionally get a byte-for-byte equivalence test (see tests/ and the math pilot notes below).

## 6. Pitfalls found so far

- Do not add `extern` declarations in .cpp files: declare functions in api.hpp and bind variables through vars() + `link::ref`.
- Never declare `_CxxThrowException`, `memcpy`-family or other runtime functions yourself in C++; include the real header.
- `this` is a keyword: it was renamed `self`/`self_`.

## 7. Math pilot findings

(The math pilot appends its notes here when it merges.)
