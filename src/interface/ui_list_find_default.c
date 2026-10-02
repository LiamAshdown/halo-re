// ui_list_find_default  (Ghidra: FUN_004a7cb0; named by types/interface.h's ui_list_item note,
// "ui_list_find_default @0x4a7cb0")
// address 0x4a7cb0, size 67 bytes
// name confidence: 0.45 (from types/interface.h)   rewrite confidence: 0.75
// evidence: types/interface.h ui_list_item (0x006b3830 growable_array group of three, element
// size 0x10, is_default at +0x0c) and the ui_list_has_default global (0x007192f8).
// register convention: group index in ECX (in_ECX, unresolved register read).
//   // blam-cc: group index -> ECX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t ui_list_has_default;  // 0x007192f8
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)

// blam-cc: group index -> ECX
// Scans one of the three UI selection lists for the first entry flagged is_default, returning
// its index, or -1 if ui_list_has_default is clear, the list is empty, or no entry is flagged.
int32_t ui_list_find_default(int32_t group_index)
{
    int32_t count;
    uint8_t *entry;
    int32_t index;

    if (!ui_list_has_default) {
        return -1;
    }
    count = ui_lists[group_index].count;
    if (count <= 0) {
        return -1;
    }
    entry = (uint8_t *)ui_lists[group_index].data + 0x0c; // ui_list_item::is_default
    index = 0;
    while (*entry == 0) {
        index = index + 1;
        entry = entry + 0x10; // sizeof(ui_list_item)
        if (count <= index) {
            return -1;
        }
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x4a7cb0):

int FUN_004a7cb0(void)

{
  int iVar1;
  int in_ECX;
  char *pcVar2;
  int iVar3;

  iVar1 = -1;
  iVar3 = iVar1;
  if (DAT_007192f8 != '\0') {
    if (0 < (int)(&DAT_006b3834)[in_ECX * 3]) {
      pcVar2 = (char *)((&DAT_006b3838)[in_ECX * 3] + 0xc);
      iVar3 = 0;
      while (*pcVar2 == '\0') {
        iVar3 = iVar3 + 1;
        pcVar2 = pcVar2 + 0x10;
        if ((int)(&DAT_006b3834)[in_ECX * 3] <= iVar3) {
          return iVar1;
        }
      }
    }
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
