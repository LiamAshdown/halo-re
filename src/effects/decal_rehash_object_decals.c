// decal_rehash_object_decals  (Ghidra: FUN_0044e000; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "decal_rehash_object_decals 0x44e000 sets +0x04 from
// bsp3d_node_find_leaf -> ScenarioStructureBSPLeaf.cluster")
// address 0x44e000, size 311 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/effects.h decal_grid.first_object_decal (0x2800), decal (previous_decal 0x30,
// next_decal 0x34, cluster_index 0x04, layer 0x06); the relink half of this function is
// byte-for-byte decal_link 0x44dd30's own body, so it is expressed here as a call to it.
// register convention: __cdecl, no arguments.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *decal_data;         // 0x0087abe4
extern decal_grid *decal_grid_block;   // 0x006b0ad8
extern ModelCollisionGeometryBSP *global_collision_bsp;           // 0x00746f90, passed to FUN_005013a0 in ECX
extern uint8_t *global_structure_bsp; // 0x00746f9c; +0xe4 is the per-leaf lookup table

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index
extern void decal_link(int16_t cluster_index, datum_index decal_index, int16_t layer); // 0x44dd30,
    // this module; blam-cc: EBX -> cluster_index, ESI -> decal_index, EDI -> layer

// Re-probes every object-attached decal against the structure BSP, and once it resolves to a
// valid cluster, unlinks it from the object-attached list and relinks it into that cluster's row
// of decal_grid.
void decal_rehash_object_decals(void)
{
    if (decal_data->valid) {
        datum_index decal_index = decal_grid_block->first_object_decal;

        while (decal_index != k_datum_index_none) {
            decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
            datum_index next = self->next_decal;
            int32_t leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, &self->position);

            if (leaf != -1) {
                int16_t cluster = *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) +
                    (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

                if (cluster != -1) {
                    if (next != k_datum_index_none) {
                        ((decal *)decal_data->data)[(uint16_t)next].previous_decal = self->previous_decal;
                    }
                    if (self->previous_decal == k_datum_index_none) {
                        decal_grid_block->first_object_decal = next;
                    } else {
                        ((decal *)decal_data->data)[(uint16_t)self->previous_decal].next_decal = next;
                    }

                    decal_link(cluster, decal_index, self->layer);
                }
            }

            decal_index = next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x44e000):

void FUN_0044e000(void)

{
  uint *puVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  int iVar10;

  iVar8 = DAT_0087abe4;
  if (*(char *)(DAT_0087abe4 + 0x24) != '\0') {
    uVar5 = *(uint *)(DAT_006b0ad8 + 0x2800);
    while (uVar7 = uVar5, uVar7 != 0xffffffff) {
      iVar4 = *(int *)(iVar8 + 0x34);
      iVar10 = (uVar7 & 0xffff) * 0x38;
      uVar5 = *(uint *)(iVar4 + 0x34 + iVar10);
      iVar9 = FUN_005013a0();
      if ((iVar9 != -1) &&
         (sVar2 = *(short *)(iVar9 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)), sVar2 != -1)) {
        if (uVar5 != 0xffffffff) {
          *(undefined4 *)((uVar5 & 0xffff) * 0x38 + 0x30 + iVar4) =
               *(undefined4 *)(iVar4 + 0x30 + iVar10);
        }
        uVar6 = *(uint *)(iVar4 + 0x30 + iVar10);
        if (uVar6 == 0xffffffff) {
          *(undefined4 *)(DAT_006b0ad8 + 0x2800) = *(undefined4 *)(iVar4 + 0x34 + iVar10);
        }
        else {
          *(undefined4 *)((uVar6 & 0xffff) * 0x38 + 0x34 + *(int *)(iVar8 + 0x34)) =
               *(undefined4 *)(iVar4 + 0x34 + iVar10);
        }
        sVar3 = *(short *)(iVar4 + 6 + iVar10);
        puVar1 = (uint *)(DAT_006b0ad8 + (sVar3 * 0x200 + (int)sVar2) * 4);
        uVar6 = *puVar1;
        iVar10 = *(int *)(iVar8 + 0x34) + iVar10;
        *(undefined4 *)(iVar10 + 0x30) = 0xffffffff;
        *(uint *)(iVar10 + 0x34) = uVar6;
        *(short *)(iVar10 + 4) = sVar2;
        *(short *)(iVar10 + 6) = sVar3;
        if (uVar6 != 0xffffffff) {
          *(uint *)((uVar6 & 0xffff) * 0x38 + 0x30 + *(int *)(iVar8 + 0x34)) = uVar7;
        }
        *puVar1 = uVar7;
      }
    }
  }
  return;
}
#endif
