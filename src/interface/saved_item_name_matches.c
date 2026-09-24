// saved_item_name_matches  (Ghidra: FUN_00495e70, unnamed)
// address 0x495e70, size 38 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/interface_functions.md "Compares a given name string against the
// currently-selected saved map/variant's name, returning a ma[tch flag]"; reuses
// saved_item_select.c's selected_saved_item/saved_item_disk_copy, and
// saved_item_name_changed.c's map-name-at-offset-2 field.
// register convention: name to compare as the recognized stack parameter (param_1).
// UNSURE: the incoming/outgoing `in_EAX` Ghidra shows is decompiler noise (its high 24 bits pass
// straight through unread); rewritten to return a plain uint8_t 0/1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern int32_t selected_saved_item;          // 0x00714e7c
extern uint8_t saved_item_disk_copy[0x1ffc]; // 0x00716e7c

// Returns 1 if a saved item is selected and `name` matches its on-disk map-name field, else 0.
uint8_t saved_item_name_matches(wchar_t *name)
{
    if (selected_saved_item != -1) {
        return wcscmp(name, (wchar_t *)(saved_item_disk_copy + 2)) == 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x495e70):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_00495e70(wchar_t *param_1)

{
  uint in_EAX;
  uint uVar1;
  int iVar2;

  uVar1 = in_EAX & 0xffffff00;
  if (_DAT_00714e7c != -1) {
    iVar2 = _wcscmp(param_1,(wchar_t *)((int)&DAT_00716e7c + 2));
    uVar1 = CONCAT31((int3)((uint)-iVar2 >> 8),'\x01' - (iVar2 != 0));
  }
  return uVar1;
}
#endif
