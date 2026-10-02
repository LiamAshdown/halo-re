// datum_get  (Ghidra: datum_get, already named)
// address 0x4d0680, size 59 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/memory_types_notes.md "datum_get @0x4d0680 ... all consistent."; matches
// data_array field layout exactly (0x20 maximum_count, 0x22 size, 0x34 data).
// register convention: handle in EDX (in_EDX), data_array* in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"

// blam-cc: handle in EDX, array in ESI
// Resolves `handle` (index in the low 16 bits, salt in the high 16, zero salt acting as a
// wildcard) to its element pointer if the index and salt are valid and the slot is in use,
// otherwise returns NULL.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void *datum_get(datum_index handle, data_array *array)
{
    void *result;
    int16_t index;
    int16_t *element;
    int16_t identifier;
    int16_t salt;

    result = 0;
    if (handle != 0xffffffff) {
        index = (int16_t)handle;
        if (-1 < index && index < array->maximum_count) {
            element = (int16_t *)((int32_t)array->size * (int32_t)index + (int32_t)array->data);
            identifier = *element;
            if (identifier != 0) {
                salt = (int16_t)(handle >> 0x10);
                if (salt == 0 || identifier == salt) {
                    result = element;
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d0680):

short * datum_get(void)

{
  short *psVar1;
  short *psVar2;
  short sVar3;
  int in_EDX;
  int unaff_ESI;
  short sVar4;

  psVar1 = (short *)0x0;
  if (((in_EDX != -1) && (sVar3 = (short)in_EDX, -1 < sVar3)) &&
     (sVar3 < *(short *)(unaff_ESI + 0x20))) {
    psVar2 = (short *)((int)*(short *)(unaff_ESI + 0x22) * (int)sVar3 + *(int *)(unaff_ESI + 0x34));
    sVar3 = *psVar2;
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_EDX >> 0x10), sVar4 == 0 || (sVar3 == sVar4)))) {
      psVar1 = psVar2;
    }
  }
  return psVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
