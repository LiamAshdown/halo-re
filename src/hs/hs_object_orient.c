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
// register convention: cdecl, one stack float argument, tail-jumped straight into `__ftol` (truncation).
// blam-cc: hs_object_orient(float x) -- cdecl, identical to (int32_t)x
// UNSURE: kept as its own file (rather than deleted / merged) because the task's address list
// names it explicitly; it is not genuinely hs-module code.

#include "tags.h"
#include "math.h"

// VERIFIED against disassembly 0x48ab80..0x48ab88 and 0x6391b4 (2026-09-30): `fld [esp+4]; jmp 0x6391b4` tail-jumps into the
// CRT __ftol (_ftol2: fistp, then a correction step), which TRUNCATES toward zero. The draft called ROUND (fistp,
// round-to-nearest-even), which returns -3 for -2.7 where the original returns -2. The C cast truncates the same way.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t hs_object_orient(float x)
{
    return (int32_t)x;
}

#if 0
Original Ghidra decompilation (0x48ab80):

void hs_object_orient(void)

{
  __ftol();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
