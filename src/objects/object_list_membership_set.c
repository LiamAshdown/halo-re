// object_list_membership_set  (named by out/phase4/objects_types_notes.md: "0x110 |
// object_list_membership_set 0x4f7450, the list head is object_globals +0x08")
// address 0x4f7450, size 150 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/objects.h object (next_tracked_object 0x110, flags 0x10 with
//   _object_in_tracked_list_bit, _object_unknown_20000_bit), object_globals
//   (first_tracked_object 0x08); global 0x008603b0 object_data, global 0x006b8cbc
//   object_globals_pointer.
// register convention: object index in ECX, add/remove flag as the sole stack parameter.
//   Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7458 mov eax,ecx / and eax,0xffff
//   at entry.
//   // blam-cc: ECX -> object_index, stack -> add

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern object_globals *object_globals_pointer; // 0x006b8cbc

void object_list_membership_set(uint32_t object_index, char add) // blam-cc: ECX -> object_index, stack -> add
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (add == 0) {
        if ((obj->flags & _object_in_tracked_list_bit) != 0) {
            datum_index *slot = &object_globals_pointer->first_tracked_object;
            while (*slot != object_index) {
                object *node = ((object_header *)object_data->data)[*slot & 0xffff].data;
                slot = &node->next_tracked_object;
            }
            *slot = obj->next_tracked_object;
            obj->next_tracked_object = k_datum_index_none;
            obj->flags &= ~(uint32_t)_object_in_tracked_list_bit;
        }
    } else if ((obj->flags & (_object_in_tracked_list_bit | _object_unknown_20000_bit)) == 0) {
        obj->next_tracked_object = object_globals_pointer->first_tracked_object;
        object_globals_pointer->first_tracked_object = object_index;
        obj->flags |= _object_in_tracked_list_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4f7450):

void FUN_004f7450(char param_1)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint *puVar4;
  uint in_ECX;

  iVar3 = DAT_006b8cbc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (param_1 == '\0') {
    if ((*(uint *)(iVar1 + 0x10) & 0x10000) != 0) {
      puVar4 = (uint *)(DAT_006b8cbc + 8);
      uVar2 = *puVar4;
      while (uVar2 != in_ECX) {
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*puVar4 & 0xffff) * 0xc);
        puVar4 = (uint *)(iVar3 + 0x110);
        uVar2 = *(uint *)(iVar3 + 0x110);
      }
      *puVar4 = *(uint *)(iVar1 + 0x110);
      *(undefined4 *)(iVar1 + 0x110) = 0xffffffff;
      *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) & 0xfffeffff;
    }
  }
  else if ((*(uint *)(iVar1 + 0x10) & 0x30000) == 0) {
    *(undefined4 *)(iVar1 + 0x110) = *(undefined4 *)(DAT_006b8cbc + 8);
    *(uint *)(iVar3 + 8) = in_ECX;
    *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x10000;
    return;
  }
  return;
}
#endif
