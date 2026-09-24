export const meta = {
  name: 'halo-missed-functions',
  description: 'Cleanup pass 1: rewrite real functions Ghidra never created (4x Sonnet rewriters by module group, Opus 5.5 review)',
  phases: [
    { title: 'Rewrite', detail: 'four Sonnet agents, each on a group of modules', model: 'sonnet' },
    { title: 'Review', detail: 'session model (Opus 5.5) runs the compile gate, fixes, and spot-checks against objdump' },
  ],
}
const ROOT = 'C:\\Users\\Liam-\\halo-re'
const GROUPS = args.groups   // [{label, items: [{addr, module}]}]

const common = `Repo: ${ROOT}. Target: retail Halo PC halo.exe 1.0.10, Bungie's Blam engine, plain C, MSVC 7.1 (cl 13.10.3077), x86.
This is cleanup pass 1: real functions that Ghidra never created during auto-analysis. They were found by the phase-4 types agents
(through vtables, dispatch tables and call sites) and have just been created in the Ghidra project under placeholder names missed_XXXXXX.
Per-module context: out/phase4/<module>_types_notes.md (explains what each missed function is and why it was found), types/<module>.h,
the module's existing files in src/<module>/ (house style: file header with address/size/confidence/evidence/register convention,
extern declarations with addresses, #if 0 block at the end with the original Ghidra output).
Tools: python tools/pack.py 0xADDR (full Ghidra C for one function, now available for these addresses), objdump:
C:\\msys64\\ucrt64\\bin\\objdump.exe -d -M intel --start-address=0xA --stop-address=0xB bin/halo.exe
Compiler facts: the engine objects were built with LTCG (/GL), so register-passed arguments (EAX/ECX/EDX/ESI/EDI) are custom per function:
confirm them from the callee prologue and the call sites or table that reach it, never assume. Many of these functions are only reached
through a function-pointer table, so their convention is often plain __cdecl/__stdcall or __thiscall-like; check.
Blam conventions: snake_case; datum handles are 32-bit (uint16 index low, uint16 salt high). Replace placeholder missed_XXXXXX with a
proper Blam-style name justified by the notes and the code.
Never write a placeholder or stub file for a real function; if one cannot be finished, leave it unwritten and list it in the summary.`

phase('Rewrite')
function rewrite(g) {
  const list = g.items.map(i => `${i.addr} (${i.module})`).join(', ')
  return agent(`${common}
Task: rewrite EXACTLY these functions and no others, one file per function at src/<module>/<name>.c: ${list}
For each: read the module's types notes entry for it, run python tools/pack.py on the address, check it against objdump (Ghidra's first
decompile of a freshly created function is often incomplete: verify the function's real end and its arguments), then rewrite it.
Rules: preserve semantics exactly (same control flow, arithmetic, memory writes, call order), use fields from the module headers, name
globals as externs with their address, mark uncertainty with // UNSURE:, append the Ghidra output inside #if 0 ... #endif.
Every file starts with #include "tags.h" then the headers it needs, in dependency order (the gate adds -I types).
After writing, run python tools/build_check.py <module> for each module you touched and fix your own files until they pass.
Also append one line per function you wrote to symbols/agent_phase4_missed.txt:  0xADDR name func 0.8 missed-pass <short evidence>
Return a summary: files written (address -> path -> name), gate results, TYPES-GAP items, UNSURE count, functions left unwritten and why.`,
  { label: `rewrite ${g.label}`, phase: 'Rewrite', model: 'sonnet' })
}
const rewrites = await parallel(GROUPS.map(g => () => rewrite(g)))

phase('Review')
const review = await agent(`${common}
Four Sonnet agents rewrote the missed functions. Their summaries:
${JSON.stringify(rewrites.filter(Boolean), null, 1)}
Full assignment: ${JSON.stringify(GROUPS)}
Tasks, in order:
0. Write any assigned function a rewriter left unwritten, same rules.
1. Run python tools/build_check.py for every module touched; fix all compile errors. Then run the full-tree gate (python tools/build_check.py).
2. Spot-check at least 12 of the new files line by line against objdump, prioritising the largest and the table-dispatched ones whose
   calling convention was inferred. Known failure modes: dropped register arguments, pointer strides on typed pointers, signed vs unsigned
   shifts and compares, NaN-unsafe float compares, a function's real end differing from Ghidra's first guess.
3. Make sure callers elsewhere in src/ that already declared these functions under another name or signature agree with the new files;
   fix the extern declarations in those callers.
4. Update each touched module's src/<module>/README.md with the new functions.
Return: gate results, number of files, fixes made, and anything still unresolved.`, { label: 'review', phase: 'Review' })

return { rewrites, review }
