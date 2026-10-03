// ui_carousel_slot_compare_valid_first  (Ghidra: FUN_004a7630, renamed)
// address 0x4a7630, size 38 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.8
// evidence: phase-4 summary "qsort comparator that sorts valid (non -1) carousel entries
// before empty ones."
// register convention: Ghidra recognized both parameters directly; no unresolved registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// qsort-style comparator over two int32_t* carousel-slot ids: entries that are not -1 sort
// before entries that are -1; two entries of the same "validity" compare equal.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t ui_carousel_slot_compare_valid_first(const int32_t *a, const int32_t *b)
{
    if (*a == -1) {
        if (*b != -1) {
            return 1;
        }
    } else if (*b == -1) {
        return -1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a7630):

undefined4 FUN_004a7630(int *param_1,int *param_2)

{
  if (*param_1 == -1) {
    if (*param_2 != -1) {
      return 1;
    }
  }
  else if (*param_2 == -1) {
    return 0xffffffff;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
