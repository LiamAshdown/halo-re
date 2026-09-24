// hs_object_orient  (already named, but per src/hs/README.md this is a misnomer)
// address 0x48ab80, size 9 bytes
// name confidence: 0.1 (the inherited Ghidra name is confirmed wrong: src/hs/README.md,
//   "Misattributed functions" #4: "`fld dword [esp+4]` / `jmp 0x6391b4`. A CRT float thunk,
//   despite the inherited Ghidra name hs_object_orient.")
// rewrite confidence: 0.7 (trivial 2-instruction thunk, fully confirmed by the README's own
//   disassembly read)
// evidence: src/hs/README.md (quoted above); __ftol (0x6391b4) is already used throughout this
//   codebase as `ROUND` (e.g. src/ai/actor_danger_update_reaction.c: "Ghidra ROUND(): a bare
//   x87 fistp, i.e. round-to-nearest-even").
// register convention: cdecl, one stack float argument, tail-jumped straight into `ROUND`.
// blam-cc: hs_object_orient(float x) -- cdecl, identical to ROUND(x)
// UNSURE: kept as its own file (rather than deleted / merged) because the task's address list
// names it explicitly; it is not genuinely hs-module code.

#include "tags.h"
#include "math.h"

extern int32_t ROUND(float x); // 0x6391b4, __ftol, MSVC round-to-nearest helper

int32_t hs_object_orient(float x)
{
    return ROUND(x);
}

#if 0
Original Ghidra decompilation (0x48ab80):

void hs_object_orient(void)

{
  __ftol();
  return;
}
#endif
