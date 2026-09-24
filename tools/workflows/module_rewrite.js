export const meta = {
  name: 'halo-module-rewrite',
  description: 'Phase 3+4 for one engine module: types header (Opus 5.5), clean-C rewrite (2x Sonnet), compile gate + review (Opus 5.5)',
  phases: [
    { title: 'Types', detail: 'session model (Opus 5.5) writes types/<module>.h from the decompiled code' },
    { title: 'Rewrite', detail: 'two Sonnet agents rewrite the module into src/<module>/', model: 'sonnet' },
    { title: 'Review', detail: 'session model (Opus 5.5) runs the compile gate, fixes, and spot-checks against the code' },
  ],
}
const ROOT = 'C:\\Users\\Liam-\\halo-re'
const MOD = args.module
const SPLIT = args.split
const FIRST = args.first
const LAST = args.last
const BATCHES = args.batches.join(', ')
const N = args.functions
const RF = args.rangeFirst || FIRST
const RL = args.rangeLast || LAST
const SKIP_TYPES = !!args.skipTypes
const ONLY_A = args.onlyA || null
const ONLY_B = args.onlyB || null

const common = `Repo: ${ROOT}. Target: retail Halo PC halo.exe 1.0.10, Bungie's Blam engine, plain C, MSVC 7.1, x86.
Module: ${MOD} (${N} functions, 0x${FIRST}..0x${LAST}). Inputs: out/phase4/${MOD}_functions.md (function list with names and one-line summaries),
${BATCHES} (per-function packs: strings, globals, callees, Ghidra C truncated at 120 lines), python tools/pack.py 0xADDR (full Ghidra C for one function),
out/phase2/results/${MOD}_*.json (naming evidence incl. struct offsets), types/tags.h (tag structs + primitive typedefs), and the headers already
written for other modules under types/ (memory.h defines data_array/datum_index/bit_stream/etc.; reuse those types, never redefine them).
src/memory/ is the house style to follow (file header, extern declarations with addresses, #if 0 Ghidra block at the end).
Header rules (Ghidra's CParser ingests types/*.h with no preprocessor): no #include, no #ifndef/#define guards; #pragma pack(push,1)/pack(pop) is fine;
redeclare the primitive typedefs exactly as the first lines of types/tags.h do; only C89/C99 constructs; a comment must not contain an unbalanced quote.
Blam conventions: snake_case; plain  typedef struct thing {...} thing;  Datum handles are 32-bit (uint16 index low, uint16 salt high).
Compiler facts (from the PE Rich header): cl 13.10.3077 (VS .NET 2003), and the engine objects were built with LTCG (/GL). The register-passed arguments (EAX/ECX/EDX/ESI/EDI) are custom conventions LTCG invented per function at link time, so they vary per function: always confirm them from the call sites and the callee prologue in objdump, never assume a house convention.
Never write a placeholder or stub file for a real function; if a function cannot be finished, leave it unwritten and list it in the summary.`

phase('Types')
const types = SKIP_TYPES ? 'types/' + MOD + '.h already exists from an earlier session; reused' : await agent(`${common}
Task: write types/${MOD}.h defining every struct, enum, and constant this module's functions operate on, and the globals they own.
Method: read the function list and the packs, then use python tools/pack.py on the functions that touch each struct to pin every field offset from
the arithmetic (e.g. *(short *)(p + 0x22)). Where the binary itself carries a layout (a byte-swap definition, a constructor that fills a struct, a
tag block whose definition already exists in types/tags.h), prefer that evidence and say so. Every struct must have every byte accounted for: name
unknown fields unknown_XX with the offset, and end each struct with a comment giving its total size. For each owning global add a comment line
// global 0x006xxxxx: <type> <name>. If a function turns out to belong to another module or to library code, note it and skip its types.
Also write out/phase4/${MOD}_types_notes.md: for each struct, which functions established which fields, unresolved offsets, misattributed functions.
Check the header with:  ${ROOT.replace(/\\\\/g, '\\\\')}  ->  C:\\msys64\\ucrt64\\bin\\gcc.exe -fsyntax-only -I types <a smoke .c that includes tags.h, memory.h and ${MOD}.h>.
Return a short summary of the structs defined, the unresolved offsets, and any misattributed functions.`, { label: `types/${MOD}.h`, phase: 'Types' })

phase('Rewrite')
function rewrite(range, label) {
  return agent(`${common}
types/${MOD}.h now exists (read it first) and must be used for every field access. Also read out/phase4/${MOD}_types_notes.md.
Task: rewrite the ${MOD}-module functions with addresses ${range} into clean, readable C, one file per function at src/${MOD}/<name>.c
(name = the function's current Ghidra name; if it is still FUN_xxxxxx, choose a proper Blam-style name, use it, and record it in the file header).
Skip functions the types notes mark as misattributed (library code); list them in your summary instead.
For each function: run  python tools/pack.py 0xADDR  to get the FULL Ghidra C (the batch file is truncated), then rewrite it.
Rules:
- Preserve semantics exactly: same control flow, same arithmetic, same memory writes, same call order. No invented behaviour, no "improvements".
  Watch pointer arithmetic: Ghidra scales by the pointed-to type, so  p + n  on a short* is n*2 bytes. Watch signed vs unsigned widening.
- Replace raw offsets with fields from the headers; replace DAT_ globals with named externs declared at the top:  extern <type> <name>; // 0x006xxxxx
- Calls to other engine functions use their current names, declared above with a prototype (extern). Register-passed arguments (in_EAX, in_ECX,
  unaff_ESI ...) become normal C parameters in the order EAX, ECX, EDX, EBX, ESI, EDI, then stack, with a  // blam-cc:  comment stating the mapping.
- File header comment: address, size, name confidence and rewrite confidence (0..1), evidence, register convention.
- Append the original Ghidra output at the end inside  #if 0 ... #endif.
- Mark anything uncertain with  // UNSURE: <why>.
- Every file starts with  #include "tags.h"  then the module headers it needs ("memory.h", "${MOD}.h", ...); the gate adds -I types.
After writing all files run  python tools/build_check.py ${MOD}  and fix compile errors in YOUR files until they pass (do not edit types/*.h; if a
type is missing, add a local typedef with a  // TYPES-GAP:  comment and list it in your summary).
Return a summary: files written, gate result, TYPES-GAP items, UNSURE count, skipped functions.`, { label, phase: 'Rewrite', model: 'sonnet' })
}
const rewrites = await parallel([
  () => rewrite(ONLY_A ? `EXACTLY these addresses and no others: ${ONLY_A.join(', ')} (large functions earlier sessions deferred; take them one at a time, disassembly first)` : `0x${RF} up to and including 0x${SPLIT}`, 'rewrite A'),
  () => rewrite(ONLY_B ? `EXACTLY these addresses and no others: ${ONLY_B.join(', ')} (large functions earlier sessions deferred; take them one at a time, disassembly first)` : `above 0x${SPLIT} up to and including 0x${RL}`, 'rewrite B'),
])

phase('Review')
const review = await agent(`${common}
Two Sonnet agents rewrote the ${MOD} module functions in 0x${RF}..0x${RL} into src/${MOD}/*.c (other ranges of the module may already exist there from an earlier session; review those files only for consistency) against types/${MOD}.h. Their summaries:
${JSON.stringify(rewrites.filter(Boolean), null, 1)}
Tasks, in order:
0. If a rewriter summary lists in-range functions it left unwritten (budget), write those files yourself first, same rules as the rewriters (python tools/pack.py for the full Ghidra C, preserve semantics, #if 0 block at the end).
1. Run  python tools/build_check.py ${MOD} . Fix every remaining compile error (you may edit types/${MOD}.h and any src/${MOD} file). Re-run until clean.
2. Fold any TYPES-GAP local typedefs back into types/${MOD}.h and delete the local copies.
3. Spot-check at least 8 functions (the largest and the most UNSURE-marked) line by line against  python tools/pack.py 0xADDR ; fix semantic drift.
   Known failure modes from the memory pilot: pointer-stride errors on typed pointers, goto targets that skip a shared advance step, signed chars
   widened as unsigned, a char return tested only in AL.
4. Consistency pass: same struct for the same global everywhere, extern declarations agree between files and with src/memory, names agree with
   symbols/functions.txt. Any renames you make: append lines  0xADDR name func 0.8 agent-phase4 <evidence>  to symbols/agent_phase4_${MOD}.txt.
5. Write src/${MOD}/README.md: what the module contains, struct layouts in one table each, known gaps, list of functions with rewrite confidence.
Return: gate result, number of files, number and nature of fixes made, misattributed functions, and the top 5 open questions for hook verification.`, { label: 'review', phase: 'Review' })

return { module: MOD, types, rewrites, review }
