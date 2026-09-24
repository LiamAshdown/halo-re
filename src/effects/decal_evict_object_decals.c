// decal_evict_object_decals  (Ghidra: FUN_0044e310; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "decal_clear_flags 0x44e220 and decal_evict_object_decals
// 0x44e310 walk it")
// address 0x44e310, size 168 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/effects.h decal_grid.cluster_first/first_object_decal, decal_flags
// (_decal_object_attached_bit); src/memory/cache_evict_entry.c establishes cache_evict_entry's
// (handle EBX, cache* EDI) convention.
// register convention: cluster_index is Ghidra's own recognised stack parameter (param_1),
// -1 meaning "every cell".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *decal_data;         // 0x0087abe4
extern decal_grid *decal_grid_block;   // 0x006b0ad8
extern cache *decal_geometry_cache;    // 0x0071d1c0

extern void cache_evict_entry(datum_index handle, cache *self); // 0x4d1c20,
    // blam-cc: EBX -> handle, EDI -> self

// Clears the object-attached flag (and its cached render geometry) of every object-attached
// decal in `cluster_index`, or of every object-attached decal in the whole table when
// `cluster_index` is -1.
void decal_evict_object_decals(int16_t cluster_index)
{
    if (decal_data->valid) {
        int layer;

        for (layer = 0; layer < 5; layer++) {
            datum_index current;

            if (cluster_index == -1) {
                if (layer != 0) {
                    continue;
                }
                current = decal_grid_block->first_object_decal;
            } else {
                current = decal_grid_block->cluster_first[layer][cluster_index];
            }

            while (current != k_datum_index_none) {
                decal *self = &((decal *)decal_data->data)[(uint16_t)current];
                datum_index next = self->next_decal;

                if ((self->flags & _decal_object_attached_bit) != 0) {
                    self->flags = self->flags & ~_decal_object_attached_bit;
                    decal_grid_block->object_count = decal_grid_block->object_count - 1;
                    cache_evict_entry(current, decal_geometry_cache);
                }

                current = next;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x44e310):

void FUN_0044e310(short param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  short sVar4;

  if (*(char *)(DAT_0087abe4 + 0x24) != '\0') {
    sVar4 = 0;
    iVar3 = DAT_006b0ad8;
    do {
      if (param_1 == -1) {
        if (sVar4 == 0) {
          uVar1 = *(uint *)(iVar3 + 0x2800);
          goto joined_r0x0044e359;
        }
      }
      else {
        uVar1 = *(uint *)(iVar3 + (sVar4 * 0x200 + (int)param_1) * 4);
joined_r0x0044e359:
        while (uVar1 != 0xffffffff) {
          iVar2 = (uVar1 & 0xffff) * 0x38 + *(int *)(DAT_0087abe4 + 0x34);
          uVar1 = *(uint *)(iVar2 + 0x34);
          if ((*(ushort *)(iVar2 + 2) & 2) != 0) {
            *(ushort *)(iVar2 + 2) = *(ushort *)(iVar2 + 2) & 0xfffd;
            *(int *)(iVar3 + 0x2808) = *(int *)(iVar3 + 0x2808) + -1;
            cache_evict_entry();
            iVar3 = DAT_006b0ad8;
          }
        }
      }
      sVar4 = sVar4 + 1;
    } while (sVar4 < 5);
  }
  return;
}
#endif
