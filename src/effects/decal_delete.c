// decal_delete  (Ghidra: decal_delete, already named)
// address 0x44e3c0, size 147 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x44e3c0..0x44e452 (EDX decal; tail-jumps datum_delete).)
// evidence: types/effects.h decal (previous_decal 0x30, next_decal 0x34, cluster_index 0x04,
// layer 0x06) and decal_grid (cluster_first/first_object_decal).
// register convention: decal handle in EDX (in_EDX).
//   // blam-cc: EDX -> decal_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *decal_data;       // 0x0087abe4
extern decal_grid *decal_grid_block; // 0x006b0ad8

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle

// Unlinks a decal from its list (a cluster row or the object-attached list) and deletes it.
void decal_delete(datum_index decal_index)
{
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    datum_index next = self->next_decal;
    datum_index previous = self->previous_decal;

    if (next != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)next].previous_decal = previous;
    }

    if (previous != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)previous].next_decal = next;
    } else if (self->cluster_index == -1) {
        decal_grid_block->first_object_decal = next;
    } else {
        decal_grid_block->cluster_first[self->layer][self->cluster_index] = next;
    }

    datum_delete(decal_data, decal_index);
}

#if 0
Original Ghidra decompilation (0x44e3c0):

void decal_delete(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint in_EDX;

  iVar3 = DAT_0087abe4;
  iVar4 = (in_EDX & 0xffff) * 0x38;
  iVar1 = *(int *)(DAT_0087abe4 + 0x34);
  uVar2 = *(uint *)(iVar4 + 0x34 + iVar1);
  iVar4 = iVar4 + iVar1;
  if (uVar2 != 0xffffffff) {
    *(undefined4 *)((uVar2 & 0xffff) * 0x38 + 0x30 + iVar1) = *(undefined4 *)(iVar4 + 0x30);
  }
  if (*(uint *)(iVar4 + 0x30) != 0xffffffff) {
    *(undefined4 *)((*(uint *)(iVar4 + 0x30) & 0xffff) * 0x38 + 0x34 + *(int *)(iVar3 + 0x34)) =
         *(undefined4 *)(iVar4 + 0x34);
    datum_delete();
    return;
  }
  if (*(short *)(iVar4 + 4) == -1) {
    *(undefined4 *)(DAT_006b0ad8 + 0x2800) = *(undefined4 *)(iVar4 + 0x34);
    datum_delete();
    return;
  }
  *(undefined4 *)(DAT_006b0ad8 + (*(short *)(iVar4 + 6) * 0x200 + (int)*(short *)(iVar4 + 4)) * 4) =
       *(undefined4 *)(iVar4 + 0x34);
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
