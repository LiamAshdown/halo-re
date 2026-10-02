// objects_get_statistics  (Ghidra: FUN_004f7950; renamed, Blam-style, not previously named)
// address 0x4f7950, size 120 bytes
// name confidence: 0.35 (matches functions.md's summary: "Computes debug/statistics counters
//   for how many objects exist and how full a related data pool is")
// rewrite confidence: 0.55
// evidence: types/objects.h object_header (identifier 0x00, flags 0x02); types/memory.h
//   data_array (last_index 0x2e), memory_pool (base 0x24, size 0x28, last_block 0x34),
//   memory_pool_block (size 0x04); global 0x008603b0 object_data, global 0x006b8cb4
//   object_memory_pool.
// register convention: out pointer in EDX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f7953 mov ecx,edx at entry.
//   // blam-cc: EDX -> out
// UNSURE: the 4.7683716e-07 scale factor (1/0x200000) implies the pool's total size is assumed
//   fixed at 0x200000 (2 MiB) rather than read from memory_pool.size; preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern memory_pool *object_memory_pool; // 0x006b8cb4

void objects_get_statistics(object_statistics *out) // blam-cc: EDX -> out
{
    int32_t used_end;
    int16_t i;

    out->count = 0;
    out->active_count = 0;

    for (i = 0; i < object_data->last_index; i++) {
        object_header *header = (object_header *)object_data->data + i;
        if (header->identifier != 0) {
            out->count = out->count + 1;
            if ((header->flags & _object_header_active_bit) != 0) {
                out->active_count = out->active_count + 1;
            }
        }
    }

    if (object_memory_pool->last_block == 0) {
        used_end = 0;
    } else {
        used_end = (int32_t)object_memory_pool->last_block + object_memory_pool->last_block->size -
                   (int32_t)object_memory_pool->base;
    }

    out->pool_fullness_fraction = 1.0f - (float)(object_memory_pool->size - used_end) * 4.7683716e-07f;
}

#if 0
Original Ghidra decompilation (0x4f7950):

void FUN_004f7950(void)

{
  short *psVar1;
  int iVar2;
  short sVar3;
  short *in_EDX;

  in_EDX[0] = 0;
  in_EDX[1] = 0;
  in_EDX[2] = 0;
  iVar2 = DAT_008603b0;
  in_EDX[3] = 0;
  psVar1 = *(short **)(DAT_008603b0 + 0x34);
  sVar3 = 0;
  if (0 < *(short *)(DAT_008603b0 + 0x2e)) {
    do {
      if ((*psVar1 != 0) && (*in_EDX = *in_EDX + 1, (*(byte *)(psVar1 + 1) & 1) != 0)) {
        in_EDX[1] = in_EDX[1] + 1;
      }
      sVar3 = sVar3 + 1;
      psVar1 = psVar1 + 6;
    } while (sVar3 < *(short *)(iVar2 + 0x2e));
  }
  if (*(int *)(DAT_006b8cb4 + 0x34) == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = (*(int *)(*(int *)(DAT_006b8cb4 + 0x34) + 4) - *(int *)(DAT_006b8cb4 + 0x24)) +
            *(int *)(DAT_006b8cb4 + 0x34);
  }
  *(float *)(in_EDX + 2) = 1.0 - (float)(*(int *)(DAT_006b8cb4 + 0x28) - iVar2) * 4.7683716e-07;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
