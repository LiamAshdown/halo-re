# C++ conventions for the halo-re conversion (v1, binding for all module conversions)

Read docs/CPP_ARCHITECTURE.md for the why. This file is the how. "Modern C++" here means real object-oriented design
and design patterns, applied inside two hard limits: binary layouts must not change, and every original C symbol must
keep existing (`tools/check_module_symbols.py` enforces the second).

## 1. What a converted module looks like

```
include/halo/<module>/<topic>.hpp   public API: classes, interfaces, enums, free functions in namespace halo::<module>
src/<module>/<topic>.cpp            implementation, a few cohesive files per module (not one file per function)
src/<module>/<module>_c_api.cpp     the C shims: one `extern "C"` function per original symbol, forwarding to the C++ API
types/<module>.h                    kept as the compatibility header other (unconverted) modules include; it may include
                                    the new headers; it must keep exporting every type name other modules use
docs/original/<module>/             the old author notes and the `#if 0` decompile blocks, moved verbatim
```

Procedure for one module (do it in this order, commit after each step that builds):
1. Baseline: `symbols/exports/<module>.txt` already exists. Run `python tools/check_module_symbols.py check <module>`; it must
   pass before you start.
2. Types first. Record structs that live in data arrays or are serialised become classes/structs with member functions:
   same members in the same order, same size, standard-layout, trivially copyable, no virtual functions, no non-static
   members added, no constructors that run on pool memory. Add `static_assert(sizeof(T) == N)` and `offsetof` checks.
   Operations whose first parameter is that record become member functions (`actor_kill(actor *a)` -> `void actor::kill()`).
3. Behaviour objects. Where the original uses function-pointer tables or switch-on-type dispatch (actor type procs,
   object type definitions, message-delta field types, game engine definitions, hs function tables, HUD widgets,
   rasterizer effects) introduce an abstract interface and concrete classes registered in a registry (Strategy/Factory/
   Template Method). These objects are created at start-up in static storage; they are never stored in saved or networked
   memory. Dispatch must call the same code in the same order as the original.
4. Module state. Each module's loose globals gather into one `struct <module>_globals` with a single accessor in
   `halo::<module>`; the fixed-address data objects defined in standalone/data/*.c stay as they are (C definitions) and are
   referenced through `extern "C"` declarations. Do not move or rename a global that standalone/data/*.c defines.
5. Resources. Files, handles, locks, COM objects, scratch buffers get RAII wrappers (`halo::platform` where OS-specific).
   Never introduce exceptions, RTTI, `new`/`delete` in simulation code, or `std::shared_ptr`.
6. C shims. For every original function symbol keep an `extern "C"` function with the identical name, parameter list and
   return type (calling convention too: `__stdcall`/`__fastcall` markers stay), whose body only forwards. This is what
   the link tables and every unconverted module call.
7. Delete the old `.c` files, move their notes to docs/original/<module>/, then run in this order:
   `python tools/check_module_symbols.py check <module>` (symbols identical),
   `python tools/cxx_probe.py` is not needed for `.cpp` files; compile errors show in the check above.
8. Do not run the full CMake build or the exe; the lead does that per wave.

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
New C++ symbols in `namespace halo::<module>` are allowed (the symbol check accepts mangled `?...@halo@@` additions); the
original C names must all still exist, and no other new symbol may appear. Classes may therefore live in a normal
namespace, be defined in cohesive .cpp files and be used by other modules. (items/effects were converted earlier with
anonymous namespaces to satisfy the stricter first version of the check; that is fine but not required.)

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

- `python tools/check_module_symbols.py check <module>` must report 0 missing, 0 extra, 0 failing files.
- Every converted file compiles with the project flags (the check compiles them).
- The lead builds everything and runs the exe after each wave; if a wave breaks the link or the smoke test the offending
  module is reverted.
- Pure modules additionally get a byte-for-byte equivalence test (see tests/ and the math pilot notes below).

## 6. Pitfalls found so far

- A header that declares functions or variables without `extern "C"` makes them C++-mangled: wrap the declarations in
  `extern "C"` (see types/bink.h, types/vorbisfile.h, src/networking/message_delta_codec.h). A header declaring globals that
  standalone/data/*.c defines must use `extern "C"` too.
- Never declare `_CxxThrowException`, `memcpy`-family or other runtime functions yourself in C++; include the real header.
- `this` is a keyword: it was renamed `self`/`self_`.
- A file that starts with `#if 0` around all of its code: code inside conditional blocks does not count when placing
  `extern "C"` (tools/cxx_wrap_linkage.py handles that for the old files).

## 7. Math pilot findings

(The math pilot appends its notes here when it merges.)
