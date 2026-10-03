// contrail_delete  (Ghidra: contrail_delete, already named)
// address 0x44cad0, size 109 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: types/effects.h contrail.first_point[4] / contrail_point.next_point (0x34).
// register convention: __cdecl, contrail_index on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *contrail_data;       // 0x0087abec
extern data_array *contrail_point_data; // 0x0087abe8

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle

// Frees every contrail_point on all four of a contrail's point lists, then deletes the contrail
// itself.
void contrail_delete(datum_index contrail_index)
{
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
    int list;

    for (list = 0; list < 4; list++) {
        datum_index point = self->first_point[list];

        while (point != k_datum_index_none) {
            datum_index next = ((contrail_point *)contrail_point_data->data)[(uint16_t)point].next_point;

            datum_delete(contrail_point_data, point);
            point = next;
        }
    }

    datum_delete(contrail_data, contrail_index);
}

#if 0
Original Ghidra decompilation (0x44cad0):

void __cdecl contrail_delete(uint contrail_index)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  
  puVar4 = (uint *)((contrail_index & 0xffff) * 0x44 + *(int *)(DAT_0087abec + 0x34) + 0x34);
  iVar3 = 4;
  iVar2 = DAT_0087abe8;
  do {
    uVar1 = *puVar4;
    while (uVar1 != 0xffffffff) {
      uVar1 = *(uint *)((uVar1 & 0xffff) * 0x38 + 0x34 + *(int *)(iVar2 + 0x34));
      iVar2 = datum_delete();
    }
    puVar4 = puVar4 + 1;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
