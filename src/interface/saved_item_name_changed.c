// saved_item_name_changed  (Ghidra: FUN_00495c90, unnamed)
// address 0x495c90, size 82 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Checks whether the display name of the
// currently-selected saved map or variant differs from its on-d[isk copy]."; reuses
// saved_item_select.c's selected_saved_item/saved_item_working_copy/saved_item_disk_copy.
// register convention: none (void).
// UNSURE: the exact field roles at working_copy+2/disk_copy+2 (map name, 0xc wide chars) and
// working_copy+0/disk_copy+0 (variant name, 0x18 wide chars) are inferred from "display name"
// in the summary; the map/game_variant record layouts are not otherwise recovered here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <wchar.h>

extern int32_t selected_saved_item;             // 0x00714e7c
extern uint8_t saved_item_disk_copy[0x1ffc];     // 0x00716e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80

// Returns 1 if the currently-selected saved map or variant's name differs from the name stored
// in its on-disk copy, 0 if they match (or nothing is selected).
int32_t saved_item_name_changed(void)
{
    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            if (wcsncmp((wchar_t *)(saved_item_working_copy + 2),
                        (wchar_t *)(saved_item_disk_copy + 2), 0xc) != 0) {
                return 1;
            }
        } else if ((selected_saved_item & 0xf) == 1) {
            if (wcsncmp((wchar_t *)saved_item_working_copy, (wchar_t *)saved_item_disk_copy,
                        0x18) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x495c90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00495c90(void)

{
  int iVar1;

  if (_DAT_00714e7c != 0xffffffff) {
    if ((_DAT_00714e7c & 0xf) == 0) {
      iVar1 = _wcsncmp((wchar_t *)((int)&DAT_00714e80 + 2),(wchar_t *)((int)&DAT_00716e7c + 2),0xc);
      if (iVar1 != 0) {
        return 1;
      }
    }
    else if (((_DAT_00714e7c & 0xf) == 1) &&
            (iVar1 = _wcsncmp((wchar_t *)&DAT_00714e80,(wchar_t *)&DAT_00716e7c,0x18), iVar1 != 0))
    {
      return 1;
    }
  }
  return 0;
}
#endif
