// object_list_reference_add  (Ghidra: object_list_reference_add, already named)
// address 0x48b2a0, size 76 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/hs.h object_list_header (count 0x06, first_reference 0x08) and
//   object_list_reference (object_index 0x04, next 0x08), whose raw-byte evidence is this
//   function's own body.
// register convention: list header index in EAX (in_EAX); object index as the recognized stack
//   parameter (param_1).
//   // blam-cc: EAX -> header_index, stack -> object_index

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern datum_index datum_new(data_array *array); // blam-cc: EDX; memory module, 0x4d0480

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Pushes `object_index` onto the front of the reference list headed by `header_index`,
// allocating a new node from object_list_reference_data. header_index's count is bumped either
// way, even if the allocation failed.
void object_list_reference_add(datum_index header_index, datum_index object_index)
{
    object_list_header *header;
    datum_index node_index;
    object_list_reference *node;

    header = (object_list_header *)((uint8_t *)object_list_header_data->data +
        (header_index & 0xffff) * 0x0c);
    node_index = datum_new(object_list_reference_data);
    if (node_index != k_datum_index_none) {
        node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
            (node_index & 0xffff) * 0x0c);
        node->object_index = object_index;
        node->next = header->first_reference;
        header->first_reference = node_index;
    }
    header->count = header->count + 1;
}

#if 0
Original Ghidra decompilation (0x48b2a0):

void object_list_reference_add(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  undefined8 uVar4;

  iVar1 = *(int *)(DAT_0087a464 + 0x34) + (in_EAX & 0xffff) * 0xc;
  uVar4 = datum_new();
  uVar3 = (uint)uVar4;
  if (uVar3 != 0xffffffff) {
    iVar2 = *(int *)((int)((ulonglong)uVar4 >> 0x20) + 0x34) + (uVar3 & 0xffff) * 0xc;
    *(undefined4 *)(iVar2 + 4) = param_1;
    *(undefined4 *)(iVar2 + 8) = *(undefined4 *)(iVar1 + 8);
    *(uint *)(iVar1 + 8) = uVar3;
  }
  *(short *)(iVar1 + 6) = *(short *)(iVar1 + 6) + 1;
  return;
}
#endif
