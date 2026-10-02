// game_engine_rasterize_message  (Ghidra: game_engine_rasterize_message, already named)
// address 0x462a80, size 11 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Stub with no body in this retail build; presumably an
// on-screen game-engine message rasterizer that was compiled out or disabled"). Zero callers.

// CORRECTED against disassembly 0x462a80..0x462a8a (2026-09-30): this is NOT a function. The 11 bytes are the shared
//   epilogue (`pop edi/esi/ebp/ebx; add esp, 0x260; ret`, AL = the caller frame's result byte) of the function that ends at
//   0x462a8a (the game variant lookup at ~0x4629xx: its `jmp`-less fall-through target at 0x462a7c/0x462a80 returns
//   through here). Nothing calls it; entering it as a function pops four registers and 0x260 bytes off the caller's
//   stack, which is why the difftest process dies. It must never be hooked or replaced: it belongs in
//   harness/stub_address_only.txt (and the `opensauce-ce-exact-entry` name at 0x462a80 in symbols/functions.txt is
//   a CE address transplanted onto a retail mid-function label).

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void game_engine_rasterize_message(void)
{
}

#if 0
Original Ghidra decompilation (0x462a80), from tools/pack.py 0x462a80:

void game_engine_rasterize_message(void)

{
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
