# Brief template for modernisation agents (fill the {{...}} fields)

You are modernising part of the halo-re engine (C++20 already, but written in a C-like style). Read docs/MODERNIZATION.md first (goals, rules,
technical path), then docs/CPP_CONVENTIONS.md for style. Your branch starts at the current master; edit only the files named in your task plus new
files under include/halo/ and src/.

ROLE: {{ROLE}}   MODULE / SCOPE: {{SCOPE}}

Rules that apply to every role
- Never run the game or any smoke test. Verify with a build of just what you touched: compile your files with the project flags (use
  `python tools/check_module_symbols.py check <module>` for the compile gate where the symbol sets still apply; when your task removes C symbols on
  purpose, update `symbols/exports/<module>.txt` accordingly in the same commit) and, for cross-module changes, a full
  `cmake --build build/<your own dir> --config Release --parallel` in your worktree (configure it first with
  `cmake -S . -B build/<dir> -G "Visual Studio 17 2022" -A Win32 -DHALO_REGENERATE=OFF`). The build must link.
- Behaviour does not change (arithmetic order, float semantics, random draws, evaluation order) except when you implement a previously unreversed
  function.
- Comments: docblocks of one or two short paragraphs with an `@address` line; no inline commentary.
- Commit small, green commits on your branch (`git add` specific files; message `modernise <module>: <what>`; end with
  `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`). Do not push. Do not touch files outside your scope.
- Never stop early and never ask questions: pick the best option, note it in the commit message, continue. When your scope is done, take the next
  most valuable item in the same module and continue until your budget is nearly used, then finish with a short report (what changed, what is left,
  census numbers for your module before/after: `python tools/modernization_census.py`).

Role details
- API: for your module, give every function other modules call a C++ declaration in `include/halo/<module>/api.hpp` (namespace halo::<module>), make all
  callers include it and call it, delete the callers' local `extern ...;` declarations of that module, delete the module's `extern "C"` shims and
  `extern "C"` blocks, and update the tables in standalone/data (they are C++ files by now, or convert them) to reference the C++ functions. Where a
  caller declared a different signature than the definition, fix the call site (cast or adapt) instead of keeping the mismatch. Global variables
  owned by the module move into one service object (`halo::<module>::Globals`, reached through an accessor) and the `extern` declarations of them
  elsewhere are replaced by the accessor.
- FLAGS: replace magic numbers with named members/constants: offsets into records become struct fields (reconstruct the struct from the access
  patterns; keep sizes with `static_assert`), bit masks become `enum class ... : uint32_t` flags using `include/halo/core/flags.hpp`
  (create it if missing: `operator|`, `&`, `~`, `has(flags, bit)`), identifier-like strings become enums. Keep the generated code identical.
- REVERSE: find unreversed code in your scope (names `FUN_xxxxxx`, `unknown_*`, `DAT_*`, `UNSURE` notes, stubs, `cp_trap_*` entries that belong to the game),
  disassemble it from `bin/halo.exe` (read-only) or `out/functions.json`, and implement it in readable C++ with real names. Do not copy decompiler
  output verbatim: restructure into clear code, keep the exact behaviour. Register it where the engine needs it (tables, callers).
- STANDALONE: convert `standalone/*.c`, `standalone/data/*.c`, `standalone/generated/*.c` to C++ and remove the address/trap/alias machinery as described in
  docs/MODERNIZATION.md step A.
