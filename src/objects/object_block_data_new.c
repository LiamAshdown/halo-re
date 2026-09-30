// object_block_data_new  (named by types/objects.h's own object_block_reference comment:
// "object_block_data_new 0x4f7d50 and object_block_data_grow 0x4f7e50 (block size at +0x06)")
// address 0x4f7d50, size 130 bytes
// name confidence: 0.85 (fixed by types/objects.h's own citation of this address by this name)
// rewrite confidence: 0.6
// evidence: types/objects.h object_header (block_size 0x06, data 0x08); types/memory.h
//   data_array (data 0x34), datum_new / datum_new_at_index_with_salt / datum_delete /
//   block_list_allocate (all established elsewhere in this codebase).
// register convention: specific datum index (or -1 for "any") in EAX, the target data_array
//   and requested size are stack parameters. Confirmed against objdump -d -M intel
//   bin/halo.exe: 0x4f7d50 cmp eax,0xffffffff before any stack access.
//   // blam-cc: EAX -> specific_index, stack -> array, size

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_memory.h"

extern memory_pool *object_memory_pool; // 0x006b8cb4


extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0
extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510

    // memory module, 0x4d1d30, same declaration as src/memory/block_list_reallocate.c; this
    // call site only tests it for zero/non-zero

datum_index object_block_data_new(int32_t specific_index, data_array *array, int16_t size)
    // blam-cc: EAX -> specific_index, stack -> array, size
{
    datum_index handle = (specific_index == -1) ? datum_new(array) : datum_new_at_index_with_salt(specific_index, array);

    if (handle != k_datum_index_none) {
        object_header *header = (object_header *)array->data + (handle & 0xffff);

        if (block_list_allocate(object_memory_pool, size, (void **)&header->data) != 0) {
            uint8_t *data = (uint8_t *)header->data;
            int16_t i;
            header->block_size = size;
            for (i = 0; i < size; i++) {
                data[i] = 0;
            }
            return handle;
        }

        datum_delete(array, handle);
        handle = k_datum_index_none;
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4f7d50):

uint FUN_004f7d50(int param_1,short param_2)

{
  int iVar1;
  char cVar2;
  int in_EAX;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;

  if (in_EAX == -1) {
    uVar3 = datum_new();
  }
  else {
    uVar3 = datum_new_at_index_with_salt();
  }
  if (uVar3 != 0xffffffff) {
    iVar1 = *(int *)(param_1 + 0x34) + (uVar3 & 0xffff) * 0xc;
    cVar2 = block_list_allocate();
    if (cVar2 != '\0') {
      *(short *)(iVar1 + 6) = param_2;
      puVar5 = *(undefined4 **)(iVar1 + 8);
      for (uVar4 = (uint)(int)param_2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar5 = 0;
        puVar5 = puVar5 + 1;
      }
      for (uVar4 = (int)param_2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar5 = 0;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
      }
      return uVar3;
    }
    datum_delete();
    uVar3 = 0xffffffff;
  }
  return uVar3;
}
#endif
