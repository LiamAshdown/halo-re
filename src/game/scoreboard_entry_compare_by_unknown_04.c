// scoreboard_entry_compare_by_unknown_04  (Ghidra: FUN_0045cbc0; named per
// out/phase4/game_functions.md, "Ascending qsort comparator on a single 4-byte field of the
// scoreboard entry struct.")
// address 0x45cbc0, size 32 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: types/game.h scoreboard_entry::unknown_04 (0x04, "never compared" by the live
//   sort path -- this function has zero callers per pack.py, i.e. it is dead code in this
//   build, which is consistent with that note).
// register convention: __cdecl, two scoreboard_entry pointers (Ghidra's own recognized stack
//   parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

// Ascending qsort comparator on scoreboard_entry::unknown_04. Unreferenced in this build.
uint32_t scoreboard_entry_compare_by_unknown_04(const scoreboard_entry *a, const scoreboard_entry *b)
{
    if (b->unknown_04 < a->unknown_04) {
        return 0xffffffff;
    }
    return (uint32_t)(a->unknown_04 < b->unknown_04);
}

#if 0
Original Ghidra decompilation (0x45cbc0), from tools/pack.py 0x45cbc0:

uint FUN_0045cbc0(int param_1,int param_2)

{
  if (*(int *)(param_2 + 4) < *(int *)(param_1 + 4)) {
    return 0xffffffff;
  }
  return (uint)(*(int *)(param_1 + 4) < *(int *)(param_2 + 4));
}
#endif
