// ui_list_get_data  (Ghidra: FUN_004a7c50; named by types/interface.h's ui_list_item note,
// "ui_list_get_data @0x4a7c50")
// address 0x4a7c50, size 40 bytes
// name confidence: 0.45 (from types/interface.h)   rewrite confidence: 0.75
// evidence: types/interface.h ui_list_item (data at +0x04) and ui_lists / ui_list_current.
// register convention: index in EDX (in_EDX, unresolved register read).
//   // blam-cc: index -> EDX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t ui_list_current;      // 0x00692c04
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)

// blam-cc: index -> EDX
// Returns the data pointer of the current UI selection list's entry at index, or NULL if index
// is out of range.
void *ui_list_get_data(int32_t index)
{
    if (index > -1 && index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + index;
        return entry->data;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a7c50):

undefined4 FUN_004a7c50(void)

{
  undefined4 uVar1;
  int in_EDX;

  uVar1 = 0;
  if ((-1 < in_EDX) && (in_EDX < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    uVar1 = *(undefined4 *)((&DAT_006b3838)[DAT_00692c04 * 3] + 4 + in_EDX * 0x10);
  }
  return uVar1;
}
#endif
