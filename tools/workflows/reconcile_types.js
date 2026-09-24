export const meta = {
  name: 'halo-types-reconcile',
  description: 'Cleanup pass 3: fold cross-module header corrections into types/*.h and fix affected sources (Opus 5.5: collect, 3 fixers, review)',
  phases: [
    { title: 'Collect', detail: 'one agent builds the correction checklist from every types notes file' },
    { title: 'Fix', detail: 'three agents, each owning a disjoint set of headers' },
    { title: 'Review', detail: 'full-tree gate, cross-checks, smoke files' },
  ],
}
const ROOT = 'C:\\Users\\Liam-\\halo-re'
const common = `Repo: ${ROOT}. Decompilation of retail Halo PC halo.exe 1.0.10 (Bungie Blam engine, plain C, MSVC 7.1 cl 13.10.3077, LTCG).
Layout: types/*.h (one header per module, no #include, no guards, parsed by Ghidra's CParser in alphabetical order), src/<module>/*.c
(one rewritten function per file; code above the final "#if 0", preserved Ghidra output below it: never edit below the final #if 0),
out/phase4/<module>_types_notes.md (each module's types agent notes, including corrections it found for OTHER modules' headers but was not
allowed to apply), out/phase4/<module>_smoke.c (compile-time size/offset checks per header), tools/build_check.py (gcc -fsyntax-only gate).
Evidence tools: python tools/pack.py 0xADDR; C:\\msys64\\ucrt64\\bin\\objdump.exe -d -M intel --start-address=0xA --stop-address=0xB bin/halo.exe;
raw .data/.rdata bytes can be read from bin/halo.exe (PE, image base 0x400000).
Rule: a correction is applied only if the binary supports it. If two notes disagree, decide from the disassembly and say why.`

const ITEM_SCHEMA = {
  type: 'object',
  properties: {
    items: { type: 'array', items: { type: 'object', properties: {
      id: { type: 'string' }, header: { type: 'string' }, kind: { type: 'string' },
      description: { type: 'string' }, evidence: { type: 'string' }, source_notes: { type: 'string' }, symbols: { type: 'string' } },
      required: ['id', 'header', 'kind', 'description', 'evidence', 'source_notes', 'symbols'] } },
  },
  required: ['items'],
}

phase('Collect')
const collected = await agent(`${common}
Task: read EVERY out/phase4/*_types_notes.md and build one checklist of all corrections a module's types agent found for a header OTHER than its
own (look for sections like "corrections for other headers", "problems found in other modules", "findings for other headers", "one thing for
other headers", conflicts, and misnamed globals). Also include these known items:
- 0x0087ac06 is declared int32_t console_verbosity in interface.h and int16_t network_statistics_level in networking.h; the binary only uses it as
  one byte (cseries notes).
- src/ai/actor_movement_test_obstacle_ray.c and src/ai/ai_search_evaluate_edge_cost.c declare 0x43d790 path_find_trace_cluster_boundary_from_vertex
  several times under alias names with different argument lists; settle its real signature from objdump and make both files use the one real
  declaration (header: ai.h has no prototype; treat the target as src only, header "src").
- Headers that reference types defined in headers that sort AFTER them alphabetically (cutscene.h, camera.h, effects.h, input.h, interface.h and
  others noted): list each such dependency (header "ApplySymbols-order") so the fix step can make Ghidra's CParser parse them in dependency order.
- out/phase4/interface_smoke.c no longer compiles (stale names/sizes); header "interface.h".
Deduplicate. Give each item an id (R01, R02, ...), the header it changes (e.g. objects.h), a kind (offset|size|type|name|signature|parse-order|smoke),
the concrete change, the evidence cited by the notes (addresses, instructions), the notes file(s) it came from, and the symbols/globals affected.
Write the list to out/phase4/reconciliation.md as well. Do not change any header or source file yourself.`, { label: 'collect', phase: 'Collect', schema: ITEM_SCHEMA })

// split by header into 3 disjoint buckets, balanced by item count
const byHeader = {}
for (const it of collected.items) (byHeader[it.header] = byHeader[it.header] || []).push(it)
const buckets = [[], [], []], sizes = [0, 0, 0]
for (const h of Object.keys(byHeader).sort((a, b) => byHeader[b].length - byHeader[a].length)) {
  const i = sizes.indexOf(Math.min(...sizes)); buckets[i].push(h); sizes[i] += byHeader[h].length
}
log(`collected ${collected.items.length} corrections across ${Object.keys(byHeader).length} headers; buckets ${JSON.stringify(sizes)}`)

phase('Fix')
function fixer(headers, idx) {
  const items = headers.flatMap(h => byHeader[h])
  return agent(`${common}
You own ONLY these headers (and the src files affected by them): ${headers.join(', ')}. Other agents are fixing other headers in parallel;
do not edit any other types/*.h.
Corrections to apply (verify each against the binary first; skip and explain any the binary does not support):
${JSON.stringify(items, null, 1)}
For each applied correction:
1. Edit the header. Keep every byte accounted for and every struct's size comment right; update that header's out/phase4/<module>_smoke.c
   checks so they still pass (run gcc on it: C:\\msys64\\ucrt64\\bin\\gcc.exe -fsyntax-only -I types out/phase4/<module>_smoke.c, and with -m32).
2. Find every src/*/*.c that uses the affected field, global or signature (grep for the old name and for the raw offset in comments) and fix it
   above the final #if 0, so the code reads the right bytes with the right width and sign. Record a one-line note in the file header:
   "reconciled: <id> <what changed>".
3. For a renamed global, update every extern declaration of it across src/ to the one agreed name and type.
For parse-order items: make scripts/ApplySymbols.java parse types/*.h in dependency order instead of alphabetical (a fixed ordered list at the
top of the script, then any remaining headers alphabetically), and list the order in the script comment.
After all items: run python tools/build_check.py for every module whose files you touched until clean.
Return: items applied (id, what changed, files touched), items skipped and why, gate results.`, { label: `fix ${idx + 1}: ${headers.join(' ')}`, phase: 'Fix' })
}
const fixes = await parallel(buckets.filter(b => b.length).map((b, i) => () => fixer(b, i)))

phase('Review')
const review = await agent(`${common}
Three agents applied cross-module header corrections from out/phase4/reconciliation.md. Their reports:
${JSON.stringify(fixes.filter(Boolean), null, 1)}
Tasks:
1. Run the full-tree gate: python tools/build_check.py. Fix every failure.
2. Compile every out/phase4/*_smoke.c (64-bit and -m32) and fix any that fail.
3. Also compile all types/*.h together in the dependency order recorded in scripts/ApplySymbols.java (one .c that includes them all) and
   fix any duplicate or conflicting definitions.
4. For 10 applied corrections that changed an offset, size, width or signedness, open two affected src files each and confirm against objdump
   that the new code reads the right bytes.
5. Mark each item in out/phase4/reconciliation.md as applied, skipped (with reason) or open.
Return: gate result, smoke results, combined-header result, number applied/skipped/open, and the open items.`, { label: 'review', phase: 'Review' })

return { collected: collected.items.length, fixes, review }
