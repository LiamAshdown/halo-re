// game_engine_free_custom_variant_cache  (Ghidra: game_engine_free_custom_variant_cache,
// already named)
// address 0x4638b0, size 111 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/game_functions.md ("Releases the dynamically allocated recent/custom
// game-variant cache and resets its bookkeeping globals to empty"); types/game.h
// game_variant_history/_count/_capacity/_current (0x00687b0c/0x10/0x14/0x18); each history
// entry is 0xa4 bytes (game_variant_history_entry), with path/name (its own two GlobalAlloc'd
// pointers) at +0x00/+0x04.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_variant_history_entry *game_variant_history; // 0x00687b0c
extern uint32_t game_variant_history_count;              // 0x00687b10
extern uint32_t game_variant_history_capacity;            // 0x00687b14
extern int32_t game_variant_history_current;              // 0x00687b18

extern void *__stdcall GlobalFree(void *handle); // Win32

void game_engine_free_custom_variant_cache(void)
{
    if (game_variant_history != 0) {
        uint32_t i;
        for (i = 0; i < game_variant_history_count; i++) {
            GlobalFree(game_variant_history[i].path);
            GlobalFree(game_variant_history[i].name);
        }
        GlobalFree(game_variant_history);
    }
    game_variant_history = 0;
    game_variant_history_capacity = 0;
    game_variant_history_count = 0;
    game_variant_history_current = -1;
}

#if 0
Original Ghidra decompilation (0x4638b0), from tools/pack.py 0x4638b0:

void game_engine_free_custom_variant_cache(void)

{
  HGLOBAL pvVar1;
  int iVar2;
  uint uVar3;

  uVar3 = 0;
  if (DAT_00687b0c != (HGLOBAL)0x0) {
    if (DAT_00687b10 != 0) {
      iVar2 = 0;
      do {
        pvVar1 = DAT_00687b0c;
        GlobalFree(*(HGLOBAL *)(iVar2 + (int)DAT_00687b0c));
        GlobalFree(*(HGLOBAL *)((int)pvVar1 + iVar2 + 4));
        uVar3 = uVar3 + 1;
        iVar2 = iVar2 + 0xa4;
      } while (uVar3 < DAT_00687b10);
    }
    GlobalFree(DAT_00687b0c);
  }
  DAT_00687b0c = (HGLOBAL)0x0;
  DAT_00687b14 = 0;
  DAT_00687b10 = 0;
  DAT_00687b18 = 0xffffffff;
  return;
}
#endif
