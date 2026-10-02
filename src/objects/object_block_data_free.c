// object_block_data_free  (Ghidra: FUN_004f7de0; renamed, Blam-style, not previously named,
// paired with object_block_data_new/object_block_data_grow per types/objects.h's
// object_block_reference comment)
// address 0x4f7de0, size 105 bytes
// name confidence: 0.5 (matches functions.md's summary: "Frees a previously allocated data
//   block and its associated datum")
// rewrite confidence: 0.6
// evidence: types/objects.h object_header (flags 0x02, data 0x08); types/memory.h
//   memory_pool (free_bytes 0x2c, first_block 0x30, last_block 0x34), memory_pool_block
//   (size 0x04, next 0x0c, previous 0x10; the payload starts at block+0x18, so this function's
//   payload-relative offsets -0x14/-8/-0xc/-0x10 land on size/previous/next/previous again --
//   the same header fields, reached from the payload end rather than the block start).
// register convention: array pointer in EAX, datum handle in EDX. Confirmed against objdump
//   -d -M intel bin/halo.exe: 0x4f7de0 (no stack access at all; both inputs are already live
//   registers at entry per Ghidra's own in_EAX/in_EDX).
//   // blam-cc: EAX -> array, EDX -> handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern memory_pool *object_memory_pool; // 0x006b8cb4

extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510

void object_block_data_free(data_array *array, datum_index handle) // blam-cc: EAX -> array, EDX -> handle
{
    object_header *header = (object_header *)array->data + (handle & 0xffff);
    uint8_t *payload = (uint8_t *)header->data;

    if (payload != 0) {
        memory_pool_block *block = (memory_pool_block *)(payload - 0x18);

        object_memory_pool->free_bytes += block->size;

        if (block->previous == 0) {
            object_memory_pool->first_block = block->next;
        } else {
            block->previous->next = block->next;
        }

        if (block->next == 0) {
            object_memory_pool->last_block = block->previous;
        } else {
            block->next->previous = block->previous;
        }
    }

    datum_delete(array, handle);
    header->data = 0;
    header->flags = 0;
}

#if 0
Original Ghidra decompilation (0x4f7de0):

void FUN_004f7de0(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int in_EAX;
  uint in_EDX;

  iVar3 = DAT_006b8cb4;
  iVar1 = *(int *)(in_EAX + 0x34) + (in_EDX & 0xffff) * 0xc;
  iVar2 = *(int *)(iVar1 + 8);
  if (iVar2 != 0) {
    *(int *)(DAT_006b8cb4 + 0x2c) = *(int *)(DAT_006b8cb4 + 0x2c) + *(int *)(iVar2 + -0x14);
    if (*(int *)(iVar2 + -8) == 0) {
      *(undefined4 *)(iVar3 + 0x30) = *(undefined4 *)(iVar2 + -0xc);
    }
    else {
      *(undefined4 *)(*(int *)(iVar2 + -8) + 0xc) = *(undefined4 *)(iVar2 + -0xc);
    }
    if (*(int *)(iVar2 + -0xc) == 0) {
      *(undefined4 *)(iVar3 + 0x34) = *(undefined4 *)(iVar2 + -8);
    }
    else {
      *(undefined4 *)(*(int *)(iVar2 + -0xc) + 0x10) = *(undefined4 *)(iVar2 + -8);
    }
  }
  datum_delete();
  *(undefined4 *)(iVar1 + 8) = 0;
  *(undefined1 *)(iVar1 + 2) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
