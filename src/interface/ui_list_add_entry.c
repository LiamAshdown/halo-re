// ui_list_add_entry  (Ghidra: FUN_004a7ba0; named by types/interface.h's ui_list_item note,
// "ui_list_add_entry @0x4a7ba0")
// address 0x4a7ba0, size 172 bytes
// name confidence: 0.45 (from types/interface.h)   rewrite confidence: 0.55
// evidence: types/interface.h ui_list_item (name, data, id, is_default all match) and ui_lists;
// growable_array_add_element already established in src/memory/.
// UNSURE: wcslen (0x625b7a, out of this module's range) is called twice with the same
// argument purely to recompute a length already known from the first call; both calls are
// preserved as decompiled rather than collapsed into one. Declared here as a wcslen-alike based
// on how its result feeds the GlobalAlloc size (len*2+2 bytes).
// register convention: name/id/data/size as the four recognized parameters; group index in EAX
// (in_EAX) and is_default in CL (in_CL, the low byte of ECX), both unresolved register reads.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern uint8_t ui_list_has_default;  // 0x007192f8
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)
extern uint32_t growable_array_add_element(growable_array *array); // 0x4cf810, established in src/memory/

// Appends one entry to a UI selection list group: a GlobalAlloc'd copy of name, the given id,
// and an optional GlobalAlloc'd copy of a data blob. Setting is_default also raises the shared
// ui_list_has_default flag.
// blam-cc: EAX -> group_index, CL -> is_default, stack -> name, id, data_blob, data_size
void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob,
                        uint32_t data_size, uint8_t is_default)
{
    int32_t index = growable_array_add_element(&ui_lists[group_index]);
    ui_list_item *entry;
    int32_t length;

    if (index == -1) {
        return;
    }

    length = wcslen((const wchar_t *)name);
    entry = (ui_list_item *)ui_lists[group_index].data + index;
    entry->data = 0;
    entry->name = (uint16_t *)(GlobalAlloc(0, length * 2 + 2));
    entry->id = id;
    entry->is_default = is_default;
    if (is_default != 0) {
        ui_list_has_default = 1;
    }
    wcslen((const wchar_t *)name);
    wcscpy((wchar_t *)entry->name, (const wchar_t *)name);

    if (data_blob != 0 && data_size != 0) {
        uint8_t *dst = (uint8_t *)GlobalAlloc(0, data_size);
        const uint8_t *src = (const uint8_t *)data_blob;
        uint32_t words = data_size >> 2;
        uint32_t bytes_left = data_size & 3;

        entry->data = dst;
        while (words != 0) {
            *(uint32_t *)dst = *(const uint32_t *)src;
            src = src + 4;
            dst = dst + 4;
            words = words - 1;
        }
        while (bytes_left != 0) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
            bytes_left = bytes_left - 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a7ba0):

void FUN_004a7ba0(wchar_t *param_1,undefined4 param_2,undefined4 *param_3,uint param_4)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  wchar_t *_Dest;
  undefined4 *puVar3;
  char in_CL;
  uint uVar4;
  undefined4 *puVar5;

  iVar1 = growable_array_add_element();
  if (iVar1 != -1) {
    iVar2 = FUN_00625b7a(param_1);
    puVar5 = (undefined4 *)(iVar1 * 0x10 + (&DAT_006b3838)[in_EAX * 3]);
    puVar5[1] = 0;
    _Dest = GlobalAlloc(0,iVar2 * 2 + 2);
    *puVar5 = _Dest;
    puVar5[2] = param_2;
    *(char *)(puVar5 + 3) = in_CL;
    if (in_CL != '\0') {
      DAT_007192f8 = 1;
    }
    FUN_00625b7a(param_1);
    _wcscpy(_Dest,param_1);
    if ((param_3 != (undefined4 *)0x0) && (param_4 != 0)) {
      puVar3 = GlobalAlloc(0,param_4);
      puVar5[1] = puVar3;
      for (uVar4 = param_4 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar3 = *param_3;
        param_3 = param_3 + 1;
        puVar3 = puVar3 + 1;
      }
      for (param_4 = param_4 & 3; param_4 != 0; param_4 = param_4 - 1) {
        *(undefined1 *)puVar3 = *(undefined1 *)param_3;
        param_3 = (undefined4 *)((int)param_3 + 1);
        puVar3 = (undefined4 *)((int)puVar3 + 1);
      }
    }
  }
  return;
}
#endif
