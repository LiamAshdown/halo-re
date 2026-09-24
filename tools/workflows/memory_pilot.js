export const meta = {
  name: 'halo-memory-pilot',
  description: 'Pilot of Phase 3+4 on the memory module: types header (Opus), clean-C rewrite of 61 functions (2x Sonnet), compile gate + review (Opus)',
  phases: [
    { title: 'Types', detail: 'Opus writes types/memory.h from the decompiled code', model: 'opus' },
    { title: 'Rewrite', detail: 'two Sonnet agents rewrite the memory functions into src/memory/', model: 'sonnet' },
    { title: 'Review', detail: 'Opus runs the compile gate, fixes, and spot-checks against the code', model: 'opus' },
  ],
}
const ROOT = 'C:\\Users\\Liam-\\halo-re'
const SPLIT = (args && args.split) || '4d0930'

const common = `Repo: ${ROOT}. Target: retail Halo PC halo.exe 1.0.10, Bungie's Blam engine, plain C, MSVC 7.1, x86.
Module: memory (61 functions, 0x4cf810..0x4d3980). Inputs: out/phase4/memory_functions.md (function list with names and one-line summaries),
out/phase2/memory/00.md and 01.md (per-function packs: strings, globals, callees, Ghidra C truncated at 120 lines),
python tools/pack.py 0xADDR (full Ghidra C for one function), out/phase2/results/memory_00.json (naming evidence incl. struct offsets),
types/tags.h (existing tag structs and the primitive typedefs at its top).
Header rules (Ghidra's CParser ingests types/*.h with no preprocessor): no #include, no #ifndef/#define guards, no comments containing unbalanced quotes;
#pragma pack(push,1) / pack(pop) is fine; redeclare the primitive typedefs exactly as the first lines of types/tags.h do; use only C89/C99 constructs.
Blam conventions: snake_case, structs named s_<thing> or <thing>_t are NOT used; use plain "struct data_array"-style typedefs like  typedef struct data_array {...} data_array;
Datum handles are 32-bit (uint16 index low, uint16 salt high). data_array header layout is well known: char name[32]; int16 maximum_count; int16 size; ... signature 'd@t@' ... int16 next_index, last_index, actual_count; ... void *data.`

phase('Types')
const types = await agent(`${common}
Task: write types/memory.h defining every struct, enum, and constant the memory module's functions operate on: the data_array/datum system,
growable_array, bit_stream, byte_stream, circular_buffer ('circ' magic), crc32 table, block_list allocator (ASCII-magic sentinel blocks), heap allocator
(bucketed free lists), the LRU cache (cache_new/allocate_block/evict_entry), data_packet_group / packet header, and struct_definition field descriptors.
Method: read the two pack files and the naming evidence, then use python tools/pack.py on the functions that touch each struct to pin down every field
offset from the arithmetic (e.g. *(short *)(p + 0x22)). Every struct must have every byte accounted for: name unknown fields pad_XX or unknown_XX with the
offset, and end each struct with a comment giving its total size. Where a global (DAT_xxxxxx) is the head of one of these structures, add a comment
line  // global 0x006xxxxx: <type> <name>  so the next pass can label it.
Also write out/phase4/memory_types_notes.md: for each struct, which functions established which fields and any offsets you could not resolve.
Return a short summary of the structs defined and the unresolved offsets.`, { label: 'types/memory.h', phase: 'Types', model: 'opus' })

phase('Rewrite')
function rewrite(range, label) {
  return agent(`${common}
types/memory.h now exists (read it first) and must be used for every field access. Also read out/phase4/memory_types_notes.md.
Task: rewrite the memory-module functions with addresses ${range} into clean, readable C, one file per function at src/memory/<name>.c
(name = the function's current Ghidra name; if it is still FUN_xxxxxx, choose a proper Blam-style name and use it, and record it in the file header).
For each function: run  python tools/pack.py 0xADDR  to get the FULL Ghidra C (the batch file is truncated), then rewrite it.
Rules:
- Preserve semantics exactly: same control flow, same arithmetic, same memory writes, same call order. No invented behaviour, no "improvements".
- Replace raw offsets with fields from types/memory.h; replace DAT_ globals with named externs declared at the top of the file as  extern <type> <name>; // 0x006xxxxx
- Calls to other engine functions use their current names; declare them with a prototype above (extern), noting register args in a comment when Ghidra shows in_EAX/in_ECX/unaff_ESI style arguments:  // blam-cc: arg0 in EAX, arg1 in ECX  and expose them as normal C parameters in the order EAX, ECX, EDX, EBX, ESI, EDI, then stack.
- File header comment: address, size, confidence in the name and in the rewrite (0..1), evidence, and the register convention.
- Append the original Ghidra output at the end of the file inside  #if 0 ... #endif  for diffing.
- Mark anything you are unsure of with  // UNSURE: <why>.
- Every file starts with  #include "tags.h"  and  #include "memory.h"  (the compile gate adds -I types).
After writing all files run  python tools/build_check.py memory  and fix compile errors in YOUR files until they pass (do not edit types/memory.h; if a type is missing, add a local  // TYPES-GAP:  comment and a local typedef, and list it in your summary).
Return a summary: files written, gate result, TYPES-GAP items, UNSURE count.`, { label, phase: 'Rewrite', model: 'sonnet' })
}
const rewrites = await parallel([
  () => rewrite(`0x4cf810 up to and including 0x${SPLIT}`, 'rewrite A'),
  () => rewrite(`above 0x${SPLIT} up to 0x4d3980`, 'rewrite B'),
])

phase('Review')
const review = await agent(`${common}
Two Sonnet agents rewrote the memory module into src/memory/*.c against types/memory.h. Their summaries:
${JSON.stringify(rewrites.filter(Boolean), null, 1)}
Tasks, in order:
1. Run  python tools/build_check.py memory . Fix every remaining compile error (you may edit types/memory.h and any src/memory file). Re-run until clean.
2. Fold any TYPES-GAP local typedefs from the .c files back into types/memory.h and delete the local copies.
3. Spot-check 8 functions (pick the largest and the most UNSURE-marked) line by line against  python tools/pack.py 0xADDR  Ghidra output; fix semantic drift.
4. Consistency pass across the module: same struct used for the same global everywhere, extern declarations agree between files, names agree with symbols/functions.txt.
5. Write src/memory/README.md: what the module contains, the struct layouts in one table each, known gaps, and the list of functions with their rewrite confidence.
Return: gate result, number of files, number of fixes made, and the top 5 open questions for the hook-verification phase.`, { label: 'review', phase: 'Review', model: 'opus' })

return { types, rewrites, review }
