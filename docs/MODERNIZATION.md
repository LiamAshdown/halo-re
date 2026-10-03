# Modernisation loop: standing orders (read this first every tick)

The user's directive, verbatim intent: a 5-minute loop that NEVER STOPS and NEVER ASKS QUESTIONS. Decide, act, log. Targets:

1. Remove `extern "C"` and the extern declarations of methods/functions between modules.
2. Use OOP; where a service locator is the right shape, use it (singletons for engine-wide services).
3. Use modern design patterns; there must be no raw C code left.
4. Use enum flags (`enum class` with a bit-flag operator header) instead of magic numbers and strings.
5. Methods that were never reversed: reverse them and implement them.

## Hard rules (never violate)

- NEVER start `halo_rebuilt.exe` or any smoke test (user order). Verification is: the CMake build links, `tools/check_module_symbols.py`
  where it still applies, `tools/modernization_census.py` does not get worse, the stale-address scans stay clean.
- Work on branch `master` of this checkout; commit locally with small commits (end the message with
  `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`). Do not push, do not force, do not delete user files.
- Agents work in worktrees under `.claude/worktrees` from the current master HEAD; the loop merges their branches into master,
  runs the build, and removes the worktree and branch afterwards. Never leave master with a failing build: if a merge breaks the build, fix it
  at once or `git revert` it.
- Behaviour must not change except where a "reverse and implement" task requires it. Keep arithmetic order, float semantics and
  random-draw order.
- Comments: method docblocks only (one or two short paragraphs, `@address` tag), no inline commentary.
- No exceptions, no RTTI in engine code (project decision); no third-party libs except those named in docs/CPP_ARCHITECTURE.md.

## Measuring progress

`python tools/modernization_census.py` prints per module: `extern_c`, `extern_decl`, `shims`, `hex_literals`, `raw_casts`, `macros`, `gotos` and a
`score` (lower is better). Baseline at the start of the loop (2026-10-03): total score 32,319, shims 3,791, extern "C" 3,988, extern decls 13,067,
magic hex literals 12,133, raw casts 14,598. Each tick records the new total in the log below. Pick work from the highest-score modules, but
respect the dependency order: a module's C entry points (shims in `<module>_c_api.cpp`) can only go once its callers and the data tables
(`standalone/data/*.c`, `tables.c`) stop using the C names.

## The technical path (do these in order; each step builds green before the next)

A. **C++-ify the standalone layer.** `standalone/*.c`, `standalone/data/*.c` and `standalone/generated/*.c` become `.cpp` (namespaces, `constexpr`
   tables, no `halo_code_` aliases). Drop `standalone/generated/code_entries.c`, the `cp_trap_*` stubs and `image_bindings.c`: function pointers in tables name
   the real C++ functions (`&halo::<module>::...`) or `nullptr` for functions that are not part of the game (CRT/D3DX/GameSpy leftovers); the loader's
   "jump into original code" diagnostic goes away with them.
B. **Per module M (bottom-up by dependency; math, memory, cseries, cache, ... last: ai, game, interface):**
   1. public header `include/halo/M/api.hpp` (or the existing class headers) declares every function other modules call, in `namespace halo::M`;
   2. every other module `#include`s that header and calls the C++ API; its local `extern ...;` declarations of M's functions are deleted;
   3. the tables that point at M's functions use the C++ names; the shims in `M_c_api.cpp` are deleted; `extern "C"` disappears from M's files;
   4. globals M owns become members of one service object (`halo::M::Globals` or a service singleton reached through `halo::services::...`),
      not loose variables declared `extern` in many files;
   5. record/state classes get real member functions; dispatch tables become interfaces + registries (the registries that exist beside the old
      tables become the only dispatch);
   6. magic numbers: field offsets (`*(uint32_t *)(p + 0x1c)`) become named struct members (`types/*.h` or new headers); flags become
      `enum class` with `halo/core/flags.hpp` operators; string literals that act as identifiers become enums or constants.
C. **Reversing**: find unreversed code (`grep -rn "UNSURE\|FUN_[0-9a-f]\{6\}\|not implemented\|TODO\|stub" src include`, names starting `FUN_`/`unknown_`/
   `DAT_`, `cp_trap_*` stubs for functions that are actually part of the game). Use `bin/halo.exe`, `out/functions.json` and the Ghidra project (read-only)
   for disassembly; rewrite the function in readable C++, name it and its fields, and add it. Never commit retail bytes.

## Agent roles (spawn with the Agent tool, subagent general-purpose, model sonnet, isolation worktree; at most 4 at a time)

- `API` agent: step B for one module (the module name and its caller list are in the task).
- `FLAGS` agent: step B.6 for one module.
- `REVERSE` agent: step C for one module or one family of functions.
- `STANDALONE` agent: step A.

Use docs/MODERNIZATION_BRIEF.md as the brief template. After spawning, fast-forward the worktree to master yourself
(`git -C .claude/worktrees/agent-<id> merge --ff-only master`; if the agent already created the two lead-owned files, delete them first).

## Log (append one line per tick: time, census total, what changed, what is running)

- 2026-10-03: loop created. Census total 32,319. Image removal done (standalone/data/tables.c), build 11 green.
- 2026-10-03 tick 0: census total 32,319; spawned STANDALONE a8033ae8c66b841c2, REVERSE interface a3ee9b552d4fbf34d, REVERSE networking+game ad4029e97d8eb3ba8, FLAGS small modules a09eab90139d34ab9 (creates include/halo/core/flags.hpp). Build 11 green (retail image removed).
- tick 1: merged REVERSE interface (a3ee9b552d4fbf34d: globals documented in include/halo/interface/engine_state.hpp, 16 files lost local externs); build green. Census: TOTAL 878 334564 3991 13056 3791 12131 14598 437 721 32310. Running: STANDALONE, REVERSE networking+game, FLAGS small modules. Open: rasterizer/main/render/saved_games/networking still declare the same globals under old names (next API/STANDALONE work).
- tick 2: merged STANDALONE (standalone layer is C++; code_entries/image_bindings/trap stubs gone; 120 CRT/D3DX table targets nulled; the single extern "C" block for tables is standalone/data/code_refs.hpp with 1337 declarations = the API worklist). Build green. Spawned API math+memory+cache a91048e42cf398036, API sound+input+physics a69fd7a30e019fce7. Running also: REVERSE networking+game ad4029e97d8eb3ba8, FLAGS small modules a09eab90139d34ab9.
- tick 3: merged REVERSE networking+game (FUN_ gone from code; globals named in include/halo/networking/{browser_state,net_state}.hpp and include/halo/game/legacy_globals.hpp; clear_flag_by_id now matches retail, no 0x50 store) and FLAGS small modules (include/halo/core/{flags,datum,tag_groups}.hpp; 11 modules; hex literals 13->2 cutscene, 45->4 devices, 115->14 projectiles, 27->0 scenario). One merge conflict (scenario_data.cpp spawned_item) resolved. Build green. Census total 32,208 (extern_c 3,994, extern_decl 13,033, shims 3,791, hex 11,867, raw casts 14,574). Running: API math+memory+cache a91048e42cf398036, API sound+input+physics a69fd7a30e019fce7, FLAGS structures/render/saved_games/main/shell/units/objects a38888c65aa8af6f1, REVERSE objects/units/hs/ai/saved_games/render/rasterizer/effects/items af36659c4c35d8dab.
- tick 4: merged API math+memory+cache+structures and API sound+input+physics+items+effects (include/halo/<m>/api.hpp + globals() accessors; ~400 conflicts scripted via build/dbg/resolve_conflicts.py + resolve2.py + fixinc.py, ~30 manual call-site fixes). Build green. Census total 29,177 (extern_c 3,420, extern_decl 11,207, shims 3,162, hex 11,868, raw casts 14,576). Running: FLAGS a38888c65aa8af6f1, REVERSE af36659c4c35d8dab; spawning API cseries/camera/cutscene/devices/dialogs and API projectiles/scenario/bitmaps/models/text/shaders.
- tick 5: merged API cseries/camera/cutscene/devices/dialogs (fast-forward, build green after cmake reconfigure). Census total 28925. Running: FLAGS a38888c65aa8af6f1, REVERSE af36659c4c35d8dab, API projectiles/scenario/bitmaps/models/text/shaders a84fffde2fac781b6. Leftovers: sound/state.hpp, shell/application.cpp, render/objects.cpp still declare cseries/camera globals.
- tick 6: merged REVERSE (objects/units/hs/ai/saved_games/render/rasterizer/effects/items: ~1k unknown_/DAT_/FUN_ names resolved, new headers hs/script_globals, main/main_globals_fields, saved_games/globals, rasterizer/globals). 24 conflicts via build/dbg/resolve3.py (HEAD + rename map). Build green. Census 28948. Running: FLAGS a38888c65aa8af6f1, API projectiles/scenario/... a84fffde2fac781b6, API rasterizer/render/saved_games/main/shell a72c10963883a51cd. Next REVERSE targets: networking net1_bandwidth renderer_unknown_*, game/interface old link names (unit_noop_569670, unknown_0071d1fa, main_globals_byte_*), console_debug_toggle_*, ai unknown_350..5d4.
- tick 7: merged FLAGS (structures/render/saved_games/main/shell/units/objects: flag enums, offsets->named fields, 140 tag-bitfield enums; 68 conflicts via build/dbg/resolve4.py + fixglob.py + argfix.py (auto ptr->ref arg fixer)). structure_bsp_view.cpp and structure_clusters.cpp took master's version (FLAGS work there to be redone). Build green. Census 28240. Running: API projectiles/scenario/... a84fffde2fac781b6, API rasterizer/render/saved_games/main/shell a72c10963883a51cd, REVERSE game/interface/networking aa9167e2251b442d0.
