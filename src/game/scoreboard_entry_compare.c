// scoreboard_entry_compare  (Ghidra: FUN_0045cbe0; named per out/phase4/game_functions.md,
// "Multi-field ascending qsort comparator used to fully order scoreboard entries when the
// primary sort key ties.")
// address 0x45cbe0, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/game.h scoreboard_entry (key_0 0x08, key_1 0x0c, key_2 0x10, key_3 0x14);
//   game_engine_build_sorted_player_list (0x45cc90) hands qsort this element layout and
//   "compares dwords 2..5 of adjacent entries to detect ties", i.e. exactly key_0..key_3.
// register convention: __cdecl, two scoreboard_entry pointers (Ghidra's own recognized stack
//   parameters).
//
// UNSURE: key_2's comparison direction is inverted relative to key_0/key_1/key_3 (ascending
// instead of descending) -- transcribed exactly as decompiled, not "corrected".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t scoreboard_entry_compare(const scoreboard_entry *a, const scoreboard_entry *b)
{
    if (b->key_0 < a->key_0) {
        return -1;
    }
    if (b->key_0 <= a->key_0) {
        if (b->key_1 < a->key_1) {
            return -1;
        }
        if (b->key_1 <= a->key_1) {
            if (a->key_2 <= b->key_2) {
                if (a->key_2 < b->key_2) {
                    return -1;
                }
                if (b->key_3 < a->key_3) {
                    return -1;
                }
                if (b->key_3 <= a->key_3) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45cbe0), from tools/pack.py 0x45cbe0:

undefined4 FUN_0045cbe0(int param_1,int param_2)

{
  if (*(int *)(param_2 + 8) < *(int *)(param_1 + 8)) {
    return 0xffffffff;
  }
  if (*(int *)(param_2 + 8) <= *(int *)(param_1 + 8)) {
    if (*(int *)(param_2 + 0xc) < *(int *)(param_1 + 0xc)) {
      return 0xffffffff;
    }
    if (*(int *)(param_2 + 0xc) <= *(int *)(param_1 + 0xc)) {
      if (*(int *)(param_1 + 0x10) <= *(int *)(param_2 + 0x10)) {
        if (*(int *)(param_1 + 0x10) < *(int *)(param_2 + 0x10)) {
          return 0xffffffff;
        }
        if (*(int *)(param_2 + 0x14) < *(int *)(param_1 + 0x14)) {
          return 0xffffffff;
        }
        if (*(int *)(param_2 + 0x14) <= *(int *)(param_1 + 0x14)) {
          return 0;
        }
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
