// data_delete_all  (Ghidra: data_delete_all, already named)
// address 0x4d0580, size 75 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: out/phase4/memory_types_notes.md "data_delete_all @0x4d0580 reseeds +0x32 with
// strncpy(&next_identifier, name, 2) then ORs in 0x8000, and zeroes the identifier of every one
// of maximum_count elements."
// register convention: data_array* in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"

extern char *strncpy(char *dst, const char *src, uint32_t count); // 00623a90 _strncpy

// blam-cc: array in ESI
// Resets an entire data_array to empty: clears last_index/actual_count, reseeds next_identifier
// from the first two bytes of the array's own name (then forces the top bit so it is never
// k_datum_identifier_none), and zero-fills every element's identifier.
void data_delete_all(data_array *array)
{
    int16_t index;

    array->last_index = 0;
    array->actual_count = 0;
    array->next_index = 0;
    strncpy((char *)&array->next_identifier, array->name, 2);
    array->next_identifier = array->next_identifier | (int16_t)k_datum_identifier_wrap;
    index = 0;
    if (0 < array->maximum_count) {
        do {
            *(int16_t *)((uint8_t *)array->data + (int32_t)array->size * (int32_t)index) = 0;
            index = index + 1;
        } while (index < array->maximum_count);
    }
}

#if 0
Original Ghidra decompilation (0x4d0580):

void data_delete_all(void)

{
  ushort *_Dest;
  short sVar1;
  int iVar2;
  char *unaff_ESI;

  _Dest = (ushort *)(unaff_ESI + 0x32);
  unaff_ESI[0x2e] = '\0';
  unaff_ESI[0x2f] = '\0';
  unaff_ESI[0x30] = '\0';
  unaff_ESI[0x31] = '\0';
  unaff_ESI[0x2c] = '\0';
  unaff_ESI[0x2d] = '\0';
  _strncpy((char *)_Dest,unaff_ESI,2);
  *_Dest = *_Dest | 0x8000;
  sVar1 = 0;
  if (0 < *(short *)(unaff_ESI + 0x20)) {
    do {
      iVar2 = (int)sVar1;
      sVar1 = sVar1 + 1;
      *(undefined2 *)(*(short *)(unaff_ESI + 0x22) * iVar2 + *(int *)(unaff_ESI + 0x34)) = 0;
    } while (sVar1 < *(short *)(unaff_ESI + 0x20));
  }
  return;
}
#endif
