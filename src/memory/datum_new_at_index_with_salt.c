// datum_new_at_index_with_salt  (Ghidra: datum_new_at_index_with_salt, already named)
// address 0x4d03d0, size 92 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md "datum_get, FUN_004d0630 (datum_next),
// FUN_004d06c0 (datum_element_initialize), datum_new_at_index @0x4d0430,
// datum_new_at_index_with_salt @0x4d03d0: all consistent."; field offsets 0x20 maximum_count,
// 0x22 size, 0x2e last_index, 0x30 actual_count, 0x34 data all match types/memory.h data_array.
// register convention: combined datum_index (index in the low 16 bits, salt in the high 16) in
// EAX (in_EAX), data_array* in EDX (in_EDX).

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void datum_element_initialize(data_array *array, void *element); // blam-cc: array in EDX, element in ESI

// blam-cc: requested handle (index | salt<<16) in EAX, array in EDX
// Allocates the data_array slot at requested_handle's index, using requested_handle's own salt
// (identifier) rather than an auto-assigned one. Fails (returns k_datum_index_none) if the index
// is out of range, the caller-supplied salt is k_datum_identifier_none, or the slot is already
// in use. On success the slot is zero-initialized (which also assigns it a fresh auto salt) and
// then its identifier is overwritten with the caller-supplied salt; the returned handle carries
// that same caller-supplied salt.
datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array)
{
    int16_t index;
    int16_t salt;
    int16_t *element;

    index = (int16_t)requested_handle;
    if (-1 < index && index < array->maximum_count) {
        salt = (int16_t)(requested_handle >> 0x10);
        if (salt != 0) {
            element = (int16_t *)((int32_t)array->size * (int32_t)index + (int32_t)array->data);
            if (*element == 0) {
                array->actual_count = array->actual_count + 1;
                if (array->last_index <= index) {
                    array->last_index = index + 1;
                }
                datum_element_initialize(array, element);
                *element = salt;
                return (uint32_t)((int32_t)salt << 0x10) | (uint16_t)index;
            }
            return 0xffffffff;
        }
        return 0xffffffff;
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4d03d0):

uint datum_new_at_index_with_salt(void)

{
  short sVar1;
  undefined4 in_EAX;
  int in_EDX;
  short *psVar2;
  short sVar3;

  sVar1 = (short)in_EAX;
  if (((-1 < sVar1) && (sVar1 < *(short *)(in_EDX + 0x20))) &&
     (sVar3 = (short)((uint)in_EAX >> 0x10), sVar3 != 0)) {
    psVar2 = (short *)((int)*(short *)(in_EDX + 0x22) * (int)sVar1 + *(int *)(in_EDX + 0x34));
    if (*psVar2 == 0) {
      *(short *)(in_EDX + 0x30) = *(short *)(in_EDX + 0x30) + 1;
      if (*(short *)(in_EDX + 0x2e) <= sVar1) {
        *(short *)(in_EDX + 0x2e) = sVar1 + 1;
      }
      FUN_004d06c0();
      *psVar2 = sVar3;
      return (int)sVar3 << 0x10 | (int)sVar1;
    }
    return 0xffffffff;
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
