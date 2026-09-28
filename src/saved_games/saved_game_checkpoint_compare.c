// saved_game_checkpoint_compare  (Ghidra: saved_game_checkpoint_compare, already named)
// address 0x538e30, size 64 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// "checkpoint_file_entry" note: "kind, primary sort key (larger first)" and
// "checkpoint_sort_newest_first: qsort direction for equal kinds". Standard qsort comparator
// signature (Ghidra-recognized void* pointers as int).
// register convention: __cdecl (Ghidra-recognized), a and b are the recognized parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t checkpoint_sort_newest_first; // 0x0069e7e8


int32_t saved_game_checkpoint_compare(const checkpoint_file_entry *a, const checkpoint_file_entry *b)
{
    int32_t result;
    int32_t time_result;

    if (a->kind == b->kind) {
        time_result = CompareFileTime((const FILETIME *)a->last_write_time, (const FILETIME *)b->last_write_time);
        result = -time_result;
        if (checkpoint_sort_newest_first == 0) {
            return time_result;
        }
    } else {
        result = (a->kind < b->kind) * 2 - 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x538e30):

LONG saved_game_checkpoint_compare(int param_1,int param_2)

{
  LONG LVar1;
  int iVar2;

  if (*(int *)(param_1 + 0x24) == *(int *)(param_2 + 0x24)) {
    LVar1 = CompareFileTime((FILETIME *)(param_1 + 0x1c),(FILETIME *)(param_2 + 0x1c));
    iVar2 = -LVar1;
    if (DAT_0069e7e8 == '\0') {
      return LVar1;
    }
  }
  else {
    iVar2 = (uint)(*(int *)(param_1 + 0x24) < *(int *)(param_2 + 0x24)) * 2 + -1;
  }
  return iVar2;
}
#endif
