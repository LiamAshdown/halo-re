# Agent brief for converting engine modules to object-oriented C++20 (template)

Used by the lead (and the overnight supervising loop) to start conversion agents. Replace the {{...}} fields.

---

You are converting part of the halo-re engine from C-style code (compiled as C++20, one function per .c file) into modern,
object-oriented C++20. Several agents work in parallel on different modules or file lists. The user's requirement: real
C++ design (classes with member functions, interfaces and design patterns where behaviour is dispatched through tables,
RAII, namespaces), not just a rename.

YOUR SCOPE: {{SCOPE}}

FIRST: run `git log --oneline -1` and confirm your branch contains commit c54d5e1f (docs/CPP_CONVENTIONS.md and
tools/check_module_symbols.py must exist in your tree). If not, STOP and report; do not continue on a stale base.
Then read docs/CPP_CONVENTIONS.md in full (binding; section 1 is the procedure) and skim docs/CPP_ARCHITECTURE.md
sections 2, 4, 8 and 12.

HARD RULES (violating any of them breaks the game; the lead builds and runs everything after you finish)
1. Every original external symbol of your module(s) must still exist with the identical name, signature and calling
   convention. Gate: `python tools/check_module_symbols.py check <module>` (baselines in symbols/exports/) must report
   0 missing / 0 extra / 0 failing files at every commit you make. The new C++ API sits behind thin `extern "C"` shim
   functions with the original names (link tables, standalone/data/*.c and other modules call those names).
2. Binary layouts do not change: records in data arrays, tag structs, saved/network structs stay standard-layout with
   identical size and offsets. Member functions are fine; virtuals, new data members and non-trivial constructors or
   destructors on those records are not. static_assert sizes/offsets.
3. Behaviour does not change: keep bodies, evaluation order, float arithmetic and random-draw order exactly. Move code, do
   not rewrite algorithms. If you see an apparent bug, leave it and mention it in your report.
4. Edit ONLY the files in your scope, new files under include/halo/<module>/, src/<module>/ (new .cpp), and
   docs/original/<module>/. If several agents share your module, do NOT edit shared struct definitions in
   types/<module>.h (merge conflicts): put your C++ in new headers as view/facade/behaviour classes around the existing
   records (composition, Strategy/State/Command/Factory classes) and keep the original records untouched. If you own the
   whole module you may add member functions to the record structs in types/<module>.h.
5. Do not touch standalone/, harness/, CMakeLists.txt, tools/, symbols/ or standalone/data/*.c.
6. No exceptions, no RTTI, no new/delete in game-state paths, no std::shared_ptr; keep game memory in the existing
   allocators. Libraries only as the conventions allow.
7. Comments: ONLY a docblock of one or two short paragraphs per function/class (plus one `@address 0x00xxxxxx` line for
   functions that came from the original). No inline commentary, no commented-out code. Move the old long author notes and
   `#if 0` decompile blocks verbatim into docs/original/<module>/ so nothing is lost.
8. Do not run the full CMake build or halo_rebuilt.exe (shared machine). The check script is your compile gate; run it
   before each commit, not after every edit (it compiles the whole module).
9. Commit after every green check (git add specific files; message `C++ OOP <module>: <what>`; end with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`). Partial conversion is fine and safe at any green commit
   (unconverted .c files keep working); finish cohesive topic groups completely. Do not push; do not merge anything.

DESIGN: group functions by the record they operate on (the type of the first pointer parameter, or the global they manage).
For each group make a class (or a view class when you do not own the record) with member functions, in a few cohesive .cpp
files (not one per function). Where the original dispatches via function-pointer tables or switch-on-type, introduce an
interface plus registry. Module globals gather into one struct with an accessor; fixed-address globals defined in
standalone/data/*.c stay referenced via `extern "C"`. Public headers go in include/halo/<module>/*.hpp (namespace
halo::<module>); types/<module>.h stays as the compatibility header and must keep every type name other modules use (grep
before renaming or moving anything). Shims keep the original function names; new C++ method names are short snake_case.

Finish when your scope is converted or your budget runs low, with everything committed and green. Final reply (max 25
lines): per module/list: files before/after, classes/patterns introduced, check results, what is left unconverted and why,
bugs noticed, header needs for other agents.
