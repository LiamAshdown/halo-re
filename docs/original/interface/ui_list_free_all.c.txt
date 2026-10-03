// ui_list_free_all  (Ghidra: FUN_004a7b20; named by types/interface.h's ui_list_item note,
// "ui_list_free_all @0x4a7b20")
// address 0x4a7b20, size 116 bytes
// name confidence: 0.45 (from types/interface.h)   rewrite confidence: 0.6
// evidence: types/interface.h ui_list_item (name at +0x00, data at +0x04) and ui_lists
// (0x006b3830, growable_array group of three).
// UNSURE: each list's element_size AND count are both reset to -1 (not 0/NULL), matching
// Ghidra exactly; not "fixed" to a more sensible reset value.
// register convention: no parameters; operates on the shared ui_lists global directly.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)

// Frees every GlobalAlloc'd name and data blob across all three UI selection lists, then frees
// each list's own backing storage and resets its element_size/count to -1.
void ui_list_free_all(void)
{
    int32_t group;

    for (group = 0; group < 3; group = group + 1) {
        int32_t count = ui_lists[group].count;
        int32_t i;

        for (i = 0; i < count; i = i + 1) {
            ui_list_item *entry = (ui_list_item *)ui_lists[group].data + i;
            if (entry->name != 0) {
                GlobalFree(entry->name);
            }
            if (entry->data != 0) {
                GlobalFree(entry->data);
            }
        }

        ui_lists[group].element_size = -1;
        ui_lists[group].count = -1;
        if (ui_lists[group].data != 0) {
            GlobalFree(ui_lists[group].data);
            ui_lists[group].data = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a7b20):

void FUN_004a7b20(void)

{
  int iVar1;
  HGLOBAL pvVar2;
  int iVar3;
  int iVar4;
  int *piVar5;

  piVar5 = &DAT_006b3838;
  do {
    iVar4 = 0;
    if (0 < piVar5[-1]) {
      iVar3 = 0;
      do {
        iVar1 = *piVar5;
        pvVar2 = *(HGLOBAL *)(iVar1 + iVar3);
        if (pvVar2 != (HGLOBAL)0x0) {
          GlobalFree(pvVar2);
        }
        pvVar2 = *(HGLOBAL *)(iVar1 + iVar3 + 4);
        if (pvVar2 != (HGLOBAL)0x0) {
          GlobalFree(pvVar2);
        }
        iVar4 = iVar4 + 1;
        iVar3 = iVar3 + 0x10;
      } while (iVar4 < piVar5[-1]);
    }
    piVar5[-2] = -1;
    piVar5[-1] = -1;
    if ((HGLOBAL)*piVar5 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*piVar5);
      *piVar5 = 0;
    }
    piVar5 = piVar5 + 3;
  } while ((int)piVar5 < 0x6b385c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
