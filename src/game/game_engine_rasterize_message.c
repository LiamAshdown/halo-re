// game_engine_rasterize_message  (Ghidra: game_engine_rasterize_message, already named)
// address 0x462a80, size 11 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Stub with no body in this retail build; presumably an
// on-screen game-engine message rasterizer that was compiled out or disabled"). Zero callers.

#include "tags.h"

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
