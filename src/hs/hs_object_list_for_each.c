// hs_object_list_for_each  (Ghidra: FUN_00488740)
// address 0x488740, size 142 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Applies a callback to every object
//   referenced by a reference list"; the callback here is a fixed function, not a parameter)
// rewrite confidence: 0.6
// evidence: types/hs.h object_list_header/object_list_reference, matching the identical
//   traversal in object_list_nth_reference.c and the other reference-list walkers in this module.
// register convention: reference-list header index in EAX (in_EAX); no other arguments.
//   // blam-cc: EAX -> header_index

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern void object_notify_children_recursive(datum_index object_index); // objects module, 0x4f7b00

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Calls object_notify_children_recursive(object_index) for every object referenced by the list headed by
// `header_index`, in list order.
void hs_object_list_for_each(datum_index header_index)
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;

    object_index = -1;
    next = 0xffffffff;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = 0xffffffff;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    while (object_index != -1) {
        object_notify_children_recursive(object_index);
        if (next == 0xffffffff) {
            object_index = -1;
            next = 0xffffffff;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x488740):

void FUN_00488740(void)

{
  uint in_EAX;
  uint in_ECX;
  int iVar1;
  uint uVar2;

  iVar1 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar1 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  while (iVar1 != -1) {
    FUN_004f7b00(iVar1);
    if (in_ECX == 0xffffffff) {
      iVar1 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  return;
}
#endif
