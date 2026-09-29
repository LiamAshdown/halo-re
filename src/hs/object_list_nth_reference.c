// object_list_nth_reference  (Ghidra: FUN_00488570)
// address 0x488570, size 136 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Advances N nodes down a reference list
//   starting at a given object's list head, used to seek to the Nth element")
// rewrite confidence: 0.6
// evidence: types/hs.h object_list_header (first_reference at 0x08) and object_list_reference
//   (object_index at 0x04, next at 0x08).
// register convention: list header index in EAX (in_EAX), 0-based element index in ECX (in_ECX).
//   // blam-cc: EAX -> header_index, ECX -> n

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Walks `n` nodes into the reference list headed by `header_index` and returns that node's
// object_index, or k_datum_index_none if the list is shorter than `n + 1` entries (or
// `header_index` itself is k_datum_index_none).
int32_t object_list_nth_reference(datum_index header_index, int16_t n)
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    int16_t remaining;

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

    remaining = n;
    while (0 < remaining && object_index != -1) {
        if (next == 0xffffffff) {
            object_index = -1;
            next = 0xffffffff;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
        remaining = remaining - 1;
    }
    return object_index;
}

#if 0
Original Ghidra decompilation (0x488570):

void FUN_00488570(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  uint in_ECX;
  uint uVar3;
  int iVar4;
  short sVar5;

  iVar4 = -1;
  uVar3 = in_ECX;
  if (in_EAX != 0xffffffff) {
    uVar3 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar3 == 0xffffffff) {
      iVar4 = -1;
      uVar3 = 0xffffffff;
    }
    else {
      iVar1 = *(int *)(DAT_0087a468 + 0x34) + (uVar3 & 0xffff) * 0xc;
      iVar4 = *(int *)(iVar1 + 4);
      uVar3 = *(uint *)(iVar1 + 8);
    }
  }
  sVar5 = (short)in_ECX;
  while ((0 < sVar5 && (iVar4 != -1))) {
    if (uVar3 == 0xffffffff) {
      iVar4 = -1;
      uVar3 = 0xffffffff;
    }
    else {
      uVar2 = uVar3 & 0xffff;
      uVar3 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar4 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
    in_ECX = in_ECX - 1;
    sVar5 = (short)in_ECX;
  }
  return;
}
#endif
