// contrail_refresh_lightmap  (Ghidra: FUN_0044cda0; named per out/phase4/effects_types_notes.md,
// which refers to this address by this name repeatedly and pins down what it actually does:
// "location at 0x14 is a bsp_leaf_reference because contrail_refresh_lightmap 0x44cda0 rewrites
// it with the bsp3d_node_find_leaf + ScenarioStructureBSPLeaf.cluster pair objects.h already
// documents" -- the name is kept even though it refreshes a point's BSP leaf/cluster location,
// not a lightmap colour, because that is the identifier the rest of the notes already use for
// this address.)
// address 0x44cda0, size 287 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/objects.h bsp_leaf_reference, global_structure_bsp (0x00746f9c, leaf array at
// +0xe4, stride 0x10, cluster at +0x08); src/objects/antenna_apply_marker_delta.c establishes
// FUN_005013a0's (globals, point, index) signature and the leaf-to-cluster lookup idiom used
// here verbatim.
// register convention: __cdecl, no arguments.
// UNSURE: the "callers=0" in this batch's metadata and this file's own scan-every-contrail shape
// suggest it runs once per tick outside this address range (a lightmap-rebuild driver), but no
// caller is available to confirm the calling convention beyond it taking no arguments.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *contrail_data;       // 0x0087abec
extern data_array *contrail_point_data; // 0x0087abe8
extern ModelCollisionGeometryBSP *global_collision_bsp;            // 0x00746f90, passed to FUN_005013a0 in ECX
extern ScenarioStructureBSP *global_structure_bsp;

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index

// For every point of every live contrail that already has a valid cluster, re-probes its
// position against the structure BSP and refreshes its cached leaf/cluster location.
void contrail_refresh_lightmap(void)
{
    datum_index contrail_index = datum_next(-1, contrail_data);

    while (contrail_index != k_datum_index_none) {
        contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
        int list;

        for (list = 0; list < 4; list++) {
            datum_index point_index = self->first_point[list];

            while (point_index != k_datum_index_none) {
                contrail_point *point = &((contrail_point *)contrail_point_data->data)[(uint16_t)point_index];

                if (point->location.cluster_index != -1) {
                    int32_t leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, &point->position);

                    point->location.leaf_index = leaf;
                    if (leaf == -1) {
                        point->location.cluster_index = -1;
                    } else {
                        point->location.cluster_index = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                            (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
                    }
                }

                point_index = point->next_point;
            }
        }

        contrail_index = datum_next((int16_t)contrail_index, contrail_data);
    }
}

#if 0
Original Ghidra decompilation (0x44cda0):

void FUN_0044cda0(void)

{
  uint uVar1;
  int iVar2;
  undefined2 uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  short *psVar7;
  short sVar8;
  uint *puVar9;
  int local_4;

  uVar4 = datum_next();
  iVar2 = DAT_0087abe8;
  do {
    do {
      if (uVar4 == 0xffffffff) {
        return;
      }
      puVar9 = (uint *)((uVar4 & 0xffff) * 0x44 + *(int *)(DAT_0087abec + 0x34) + 0x34);
      local_4 = 4;
      do {
        uVar1 = *puVar9;
        while (uVar1 != 0xffffffff) {
          iVar5 = (uVar1 & 0xffff) * 0x38 + *(int *)(iVar2 + 0x34);
          if (*(short *)(iVar5 + 0x18) != -1) {
            iVar6 = FUN_005013a0();
            *(int *)(iVar5 + 0x14) = iVar6;
            if (iVar6 == -1) {
              uVar3 = 0xffff;
            }
            else {
              uVar3 = *(undefined2 *)(iVar6 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
            }
            *(undefined2 *)(iVar5 + 0x18) = uVar3;
          }
          uVar1 = *(uint *)(iVar5 + 0x34);
        }
        puVar9 = puVar9 + 1;
        local_4 = local_4 + -1;
      } while (local_4 != 0);
      iVar5 = uVar4 + 1;
      sVar8 = (short)iVar5;
      uVar4 = 0xffffffff;
    } while ((sVar8 < 0) || (*(short *)(DAT_0087abec + 0x2e) <= sVar8));
    psVar7 = (short *)((int)sVar8 * (int)*(short *)(DAT_0087abec + 0x22) +
                      *(int *)(DAT_0087abec + 0x34));
    do {
      iVar2 = DAT_0087abe8;
      if (*psVar7 != 0) {
        uVar4 = (int)*psVar7 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar7 = (short *)((int)psVar7 + (int)*(short *)(DAT_0087abec + 0x22));
    } while ((short)iVar5 < *(short *)(DAT_0087abec + 0x2e));
  } while( true );
}
#endif
