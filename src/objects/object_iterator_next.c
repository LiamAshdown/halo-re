// object_iterator_next  (Ghidra: object_iterator_next, already named)
// address 0x4f6f20, size 129 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Advances an object iterator to the next object matching the
//   requested type and flag mask, returning its data pointer")
// rewrite confidence: 0.75
// evidence: types/objects.h object_iterator (type_mask 0x00, flags_mask 0x04, index 0x06,
//   handle 0x08), object_header (identifier 0x00, flags 0x02, type 0x03); types/memory.h
//   data_array (last_index 0x2e); global 0x008603b0 object_data.
// register convention: the iterator pointer is the sole, genuinely-stack, parameter (Ghidra's
//   own "object_iterator_next(undefined4 *param_1)"). Confirmed against objdump -d -M intel
//   bin/halo.exe: 0x4f6f22 mov ebp,[esp+0xc] reads it from the stack after two register pushes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

object *object_iterator_next(object_iterator *iterator)
{
    int16_t index = iterator->index;

    while (index < object_data->last_index) {
        object_header *header = (object_header *)object_data->data + index;
        int16_t this_index = index;
        index++;

        if ((header->identifier != 0) &&
            ((header->flags & iterator->flags_mask) == iterator->flags_mask) &&
            ((iterator->type_mask & (1u << (header->type & 0x1f))) != 0)) {
            iterator->handle = ((uint32_t)(uint16_t)header->identifier << 16) | (uint16_t)this_index;
            iterator->index = index;
            return header->data;
        }
    }

    iterator->index = index;
    return (object *)0;
}

#if 0
Original Ghidra decompilation (0x4f6f20):

undefined4 object_iterator_next(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  short sVar3;
  short *psVar4;

  sVar3 = *(short *)((int)param_1 + 6);
  uVar1 = 0;
  psVar4 = (short *)(*(int *)(DAT_008603b0 + 0x34) + sVar3 * 0xc);
  if (sVar3 < *(short *)(DAT_008603b0 + 0x2e)) {
    while( true ) {
      uVar2 = (uint)sVar3;
      sVar3 = sVar3 + 1;
      if (((*psVar4 != 0) && ((*(byte *)(psVar4 + 1) & (byte)param_1[1]) == (byte)param_1[1])) &&
         ((*param_1 & 1 << (*(byte *)((int)psVar4 + 3) & 0x1f)) != 0)) break;
      psVar4 = psVar4 + 6;
      if (*(short *)(DAT_008603b0 + 0x2e) <= sVar3) {
        *(short *)((int)param_1 + 6) = sVar3;
        return 0;
      }
    }
    param_1[2] = (int)*psVar4 << 0x10 | uVar2;
    uVar1 = *(undefined4 *)(psVar4 + 4);
  }
  *(short *)((int)param_1 + 6) = sVar3;
  return uVar1;
}
#endif
