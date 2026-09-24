export const meta = {
  name: 'halo-orphan-functions',
  description: 'Cleanup pass 4: rewrite engine functions no module rewrote (4x Sonnet rewriters by module group, Opus 5.5 review)',
  phases: [
    { title: 'Rewrite', detail: 'four Sonnet agents, each on a group of modules', model: 'sonnet' },
    { title: 'Review', detail: 'session model (Opus 5.5) runs the compile gate, fixes, and spot-checks against objdump' },
  ],
}
const ROOT = 'C:\\Users\\Liam-\\halo-re'
const GROUPS = args.groups   // [{label, items: [{addr, module, name, size}]}]

const common = `Repo: ${ROOT}. Target: retail Halo PC halo.exe 1.0.10, Bungie's Blam engine, plain C, MSVC 7.1 (cl 13.10.3077), x86.
This is cleanup pass 4: orphan functions. python tools/coverage_audit.py found engine functions (modules.json assigns them a module) that
no src file defines. Most were skipped because the module that owned the address range judged them out of place (misattributed to it,
really another module's code) and noted that in its src/<module>/README.md or out/phase4/<module>_types_notes.md, and no other pass
picked them up. Grep src/*/README.md and out/phase4/*_notes.md for the address first: that note usually says what the function is and
which module it really belongs to. The "module" given with each address is only modules.json's guess.
House style (see any existing src/<module>/*.c): file header with name, "// address 0xADDR, size N bytes", name confidence and rewrite
confidence, evidence, register convention with a "// blam-cc:" line; extern declarations with their addresses; // UNSURE: markers; the
original Ghidra output inside a final #if 0 ... #endif block.
Tools: python tools/pack.py 0xADDR (full Ghidra C for one function), objdump:
C:\\msys64\\ucrt64\\bin\\objdump.exe -d -M intel --start-address=0xA --stop-address=0xB bin/halo.exe
Compiler facts: the engine objects were built with LTCG (/GL), so register-passed arguments (EAX/ECX/EDX/ESI/EDI) are custom per function:
confirm them from the callee prologue and its call sites, never assume. The shell 0x57b..0x57d range is the hardware-requirements code
(hwreq), which is C++ using MSVC 7.1 std::map / std::string instantiations and EH: rewrite those as plain C with the layout they use.
Blam conventions: snake_case; datum handles are 32-bit (uint16 index low, uint16 salt high). Replace FUN_XXXXXX with a proper Blam-style
name justified by the code; keep an existing descriptive Ghidra name when the code supports it.
Some addresses may turn out not to be functions of their own (a label inside another function, a jump-table target, a fragment already
written as part of its parent). Write no file for those; record each with the evidence in out/phase4/orphans_notes.md instead.
Never write a placeholder or stub file for a real function; if one cannot be finished, leave it unwritten and list it in the summary.`

phase('Rewrite')
function rewrite(g) {
  const list = g.items.map(i => `${i.addr} ${i.name} (${i.size} bytes, modules.json: ${i.module})`).join('\n')
  return agent(`${common}
Task: handle EXACTLY these addresses and no others:
${list}
For each: find the notes about it, run python tools/pack.py on it and check it against objdump (confirm the real start and end, the
arguments and the return), decide the module it belongs to (the src/<module>/ directory it goes in), then rewrite it there as one file.
Rules: preserve semantics exactly (same control flow, arithmetic, memory writes, call order), use the types from types/*.h, name globals
as externs with their address, mark uncertainty with // UNSURE:.
Every file starts with #include "tags.h" then the headers it needs, in dependency order (the gate adds -I types).
After writing, run python tools/build_check.py <module> for each module you touched and fix your own files until they pass.
Append one line per function you wrote to symbols/agent_phase4_orphans.txt:  0xADDR name func <name confidence> orphan-pass <short evidence>
(same format as symbols/agent_phase4_missed.txt; create the file with a one-line # comment header if it does not exist).
Return a summary: files written (address -> path -> name), addresses judged not-a-function (and why), gate results, TYPES-GAP items,
UNSURE count, and anything left unwritten and why.`,
  { label: `rewrite ${g.label}`, phase: 'Rewrite', model: 'sonnet' })
}
const rewrites = await parallel(GROUPS.map(g => () => rewrite(g)))

phase('Review')
const review = await agent(`${common}
Four Sonnet agents handled the orphan functions. Their summaries:
${JSON.stringify(rewrites.filter(Boolean), null, 1)}
Full assignment: ${JSON.stringify(GROUPS)}
Tasks, in order:
0. Handle any assigned address a rewriter left unhandled, same rules.
1. Run python tools/build_check.py for every module touched; fix all compile errors. Then run the full-tree gate (python tools/build_check.py).
2. Spot-check at least 12 of the new files line by line against objdump, prioritising the largest and the ones whose calling convention
   was inferred. Known failure modes: dropped register arguments, pointer strides on typed pointers, signed vs unsigned shifts and
   compares, NaN-unsafe float compares, a function's real end differing from Ghidra's first guess.
3. Check every not-a-function verdict in out/phase4/orphans_notes.md against objdump.
4. Callers elsewhere in src/ that declared these functions as FUN_XXXXXX or under another name/signature: make them agree with the new
   files (python tools/propagate_names.py --apply handles names on extern lines that carry the address; fix signatures by hand).
5. Update each touched module's src/<module>/README.md with the new functions.
6. Run python tools/coverage_audit.py and python tools/naming_report.py and report their summaries.
Return: gate results, number of files, fixes made, audit summary, and anything still unresolved.`, { label: 'review', phase: 'Review' })

return { rewrites, review }
