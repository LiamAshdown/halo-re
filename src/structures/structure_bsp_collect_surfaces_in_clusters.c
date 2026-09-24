// structure_bsp_collect_surfaces_in_clusters  (Ghidra: FUN_00553c40, still unnamed)
// address 0x553c40, size 308 bytes
// name confidence: 0.55 -- matches the phase4 summary ("Collects the visible surface indices from
//   a set of clusters whose bounding boxes pass both an AABB containment test and a frustum-plane
//   classification test, capped at a maximum count").
// rewrite confidence: 0.55 -- Ghidra recovered only 2 of this function's 8 real cdecl parameters
//   (it names them param_1/param_2 and leaves the rest as in_stack_NNNNNNNN using their raw
//   entry-relative stack offsets); objdump disassembly was needed to recover the true parameter
//   list and to confirm the aabb_overlap_classify / frustum_planes_classify_box argument mapping.
// evidence: objdump -M intel disassembly of 0x553c40..0x553d80; the two callees' own register
//   conventions (aabb_overlap_classify.c, frustum_planes_classify_box.c, both this batch);
//   ScenarioStructureBSPCluster.subclusters / ScenarioStructureBSPSubcluster.surface_indices.
// register convention: cdecl, 8 stack parameters (see the extern's real signature below); no
//   register-passed arguments.
// UNSURE: the function's own return value (a count) is discarded by both call sites in
//   0x553d80 (this batch), so its exact packing (CONCAT22 of an always-small high word) is
//   reproduced as a plain int32_t count rather than chasing the original's 16/16 split.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp;                            // 0x00746f9c
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits];  // 0x007d0394

// blam-cc: ECX -> box_a, EDX -> box_b
extern structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a,
    real_rectangle3d *box_b); // 0x5541b0, this module
// blam-cc: EAX -> box, EBX -> planes, DI -> plane_count
extern structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box,
    real_plane3d *planes, int16_t plane_count); // 0x554260, this module

// blam-cc: cdecl, 8 stack params (out_surfaces, max_count, query_box, plane_count, planes,
// visited_bits, cluster_count, cluster_indices)
int32_t structure_bsp_collect_surfaces_in_clusters(int32_t *out_surfaces, int16_t max_count,
                                                    real_rectangle3d *query_box, int16_t plane_count,
                                                    real_plane3d *planes, uint32_t *visited_bits,
                                                    int16_t cluster_count,
                                                    int16_t *cluster_indices)
{
    int16_t written = 0;

    for (int16_t c = 0; c < cluster_count; c++) {
        if (written >= max_count) {
            break;
        }
        ScenarioStructureBSPCluster *cluster =
            &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[cluster_indices[c]];

        for (int32_t s = 0; s < (int32_t)cluster->subclusters.count; s++) {
            if (written >= max_count) {
                break;
            }
            ScenarioStructureBSPSubcluster *subcluster =
                &((ScenarioStructureBSPSubcluster *)cluster->subclusters.pointer)[s];
            // ECX is the subcluster's world_bounds_x/y/z triple, i.e. its leading real_rectangle3d;
            // EDX is the caller's query box (`mov edx,[esp+0x2c]` at 0x553cb7).
            if (aabb_overlap_classify((real_rectangle3d *)subcluster, query_box) ==
                    _structure_bsp_overlap_none) {
                continue;
            }
            if (frustum_planes_classify_box((real_rectangle3d *)subcluster, planes, plane_count) ==
                _structure_bsp_overlap_none) {
                continue;
            }
            int32_t *indices = (int32_t *)subcluster->surface_indices.pointer;
            for (int32_t k = 0; k < (int32_t)subcluster->surface_indices.count; k++) {
                int32_t surface = indices[k];
                int32_t word = surface >> 5;
                uint32_t mask = 1u << (surface & 0x1f);
                if ((surface_visible_bits[word] & mask) != 0 && (visited_bits[word] & mask) == 0) {
                    if (written >= max_count) {
                        break;
                    }
                    visited_bits[word] |= mask;
                    out_surfaces[written] = surface;
                    written++;
                }
            }
        }
    }
    return written;
}

#if 0
Original Ghidra decompilation (0x553c40):

undefined4 FUN_00553c40(int param_1,short param_2)

{
  uint *puVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar6;
  int iVar4;
  int iVar5;
  uint uVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  int in_stack_00000018;
  short in_stack_0000001c;
  int in_stack_00000020;
  int local_8;

  uVar6 = 0;
  sVar8 = 0;
  local_8 = 0;
  if (0 < in_stack_0000001c) {
    do {
      if (param_2 <= sVar8) break;
      iVar9 = *(short *)(in_stack_00000020 + (short)local_8 * 2) * 0x68;
      iVar10 = iVar9 + *(int *)(DAT_00746f9c + 0x138);
      sVar3 = 0;
      if (0 < *(int *)(iVar9 + 0x34 + *(int *)(DAT_00746f9c + 0x138))) {
        do {
          if (param_2 <= sVar8) break;
          iVar9 = *(int *)(iVar10 + 0x38) + sVar3 * 0x24;
          sVar2 = aabb_overlap_classify();
          if ((sVar2 != 0) && (sVar2 = frustum_planes_classify_box(), sVar2 != 0)) {
            piVar11 = *(int **)(iVar9 + 0x1c);
            sVar2 = 0;
            if (0 < *(int *)(iVar9 + 0x18)) {
              do {
                iVar4 = *piVar11 >> 5;
                uVar7 = 1 << ((byte)*piVar11 & 0x1f);
                iVar5 = iVar4 * 4;
                if ((((&DAT_007d0394)[iVar4] & uVar7) != 0) &&
                   ((*(uint *)(iVar5 + in_stack_00000018) & uVar7) == 0)) {
                  if (param_2 <= sVar8) break;
                  puVar1 = (uint *)(iVar5 + in_stack_00000018);
                  *puVar1 = *puVar1 | uVar7;
                  *(int *)(param_1 + sVar8 * 4) = *piVar11;
                  sVar8 = sVar8 + 1;
                }
                piVar11 = piVar11 + 1;
                sVar2 = sVar2 + 1;
              } while ((int)sVar2 < *(int *)(iVar9 + 0x18));
            }
          }
          sVar3 = sVar3 + 1;
        } while ((int)sVar3 < *(int *)(iVar10 + 0x34));
      }
      local_8 = local_8 + 1;
    } while ((short)local_8 < in_stack_0000001c);
    uVar6 = (undefined2)((uint)local_8 >> 0x10);
  }
  return CONCAT22(uVar6,sVar8);
}
#endif
