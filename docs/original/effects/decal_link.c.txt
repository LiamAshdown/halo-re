// decal_link  (Ghidra: FUN_0044dd30; named per out/phase4/effects_types_notes.md, which refers
// to this exact insertion as "decal_link 0x44dd30 own flags, cluster_index, layer and the list
// links")
// address 0x44dd30, size 86 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x44dd30..0x44dd85 (EBX cluster, ESI decal, EDI layer).)
// evidence: types/effects.h decal (cluster_index 0x04, layer 0x06, previous_decal 0x30,
// next_decal 0x34) and decal_grid.cluster_first[5][0x200].
// register convention: cluster index in BX (unaff_BX), decal handle in ESI (unaff_ESI), layer in
// DI (unaff_DI).
//   // blam-cc: EBX -> cluster_index, ESI -> decal_index, EDI -> layer

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

// Inserts `decal_index` at the head of decal_grid's (layer, cluster_index) list.
void decal_link(int16_t cluster_index, datum_index decal_index, int16_t layer)
{
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    datum_index *head = &decal_grid_block->cluster_first[layer][cluster_index];
    datum_index old_head = *head;

    self->previous_decal = k_datum_index_none;
    self->next_decal = old_head;
    self->cluster_index = cluster_index;
    self->layer = layer;

    if (old_head != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)old_head].previous_decal = decal_index;
    }

    *head = decal_index;
}

#if 0
Original Ghidra decompilation (0x44dd30):

void FUN_0044dd30(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  short unaff_BX;
  uint unaff_ESI;
  short unaff_DI;

  iVar3 = DAT_0087abe4;
  iVar4 = unaff_DI * 0x200 + (int)unaff_BX;
  uVar2 = *(uint *)(DAT_006b0ad8 + iVar4 * 4);
  puVar1 = (uint *)(DAT_006b0ad8 + iVar4 * 4);
  iVar4 = (unaff_ESI & 0xffff) * 0x38 + *(int *)(DAT_0087abe4 + 0x34);
  *(undefined4 *)(iVar4 + 0x30) = 0xffffffff;
  *(uint *)(iVar4 + 0x34) = uVar2;
  *(short *)(iVar4 + 4) = unaff_BX;
  *(short *)(iVar4 + 6) = unaff_DI;
  if (uVar2 != 0xffffffff) {
    *(uint *)((uVar2 & 0xffff) * 0x38 + 0x30 + *(int *)(iVar3 + 0x34)) = unaff_ESI;
  }
  *puVar1 = unaff_ESI;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
