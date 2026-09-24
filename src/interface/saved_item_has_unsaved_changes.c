// saved_item_has_unsaved_changes  (Ghidra: FUN_00495ea0, unnamed)
// address 0x495ea0, size 163 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Determines whether the currently-selected saved
// map or variant has unsaved changes by comparing its [working copy against its disk copy]";
// reuses saved_item_select.c's selected_saved_item/saved_item_working_copy/saved_item_disk_copy,
// whose 0x1ffc (map) and 0x98 (variant, game_variant sized) byte spans match this function's
// 0x7ff/0x26 dword compare counts exactly.
// register convention: none (void).
// UNSURE: the dword-by-dword compare loop is reproduced as memcmp, which is exactly equivalent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern int32_t selected_saved_item;             // 0x00714e7c
extern uint8_t saved_item_disk_copy[0x1ffc];     // 0x00716e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80

// Returns true if a saved item is selected and its working copy differs from its on-disk copy
// (map: the full 0x1ffc byte record; variant: just the 0x98 byte game_variant portion).
uint8_t saved_item_has_unsaved_changes(void)
{
    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            return memcmp(saved_item_disk_copy, saved_item_working_copy,
                          sizeof(saved_item_disk_copy)) != 0;
        }
        if ((selected_saved_item & 0xf) == 1) {
            return memcmp(saved_item_disk_copy, saved_item_working_copy, sizeof(game_variant)) != 0;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x495ea0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_00495ea0(void)

{
  int iVar1;
  int *piVar2;
  int *piVar3;
  bool bVar4;

  bVar4 = false;
  if (_DAT_00714e7c != 0xffffffff) {
    if ((_DAT_00714e7c & 0xf) == 0) {
      iVar1 = 0x7ff;
      bVar4 = true;
      piVar2 = &DAT_00716e7c;
      piVar3 = &DAT_00714e80;
      do {
        if (iVar1 == 0) break;
        iVar1 = iVar1 + -1;
        bVar4 = *piVar2 == *piVar3;
        piVar2 = piVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (bVar4);
      bVar4 = !bVar4;
    }
    else if ((_DAT_00714e7c & 0xf) == 1) {
      iVar1 = 0x26;
      bVar4 = true;
      piVar2 = &DAT_00716e7c;
      piVar3 = &DAT_00714e80;
      do {
        if (iVar1 == 0) break;
        iVar1 = iVar1 + -1;
        bVar4 = *piVar2 == *piVar3;
        piVar2 = piVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (bVar4);
      return !bVar4;
    }
  }
  return bVar4;
}
#endif
