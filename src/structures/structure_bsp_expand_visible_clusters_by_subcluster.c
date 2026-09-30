// structure_bsp_expand_visible_clusters_by_subcluster  (Ghidra: FUN_00553920, still unnamed)
// address 0x553920, size 322 bytes
// name confidence: 0.6 -- phase4 summary: "Expands cluster visibility by testing each visible
//   cluster's child bounding boxes against the camera frustum and marking newly visible leaves"
//   (its "leaves" is imprecise -- the bits it sets are surface bits, not leaf bits; the name here
//   says what the code actually does).
// rewrite confidence: 0.5 -- disassembly (objdump) was needed because the Ghidra decompile
//   collapsed the render_frustum_test_bounding_box call to a bare `(0)` and hid an entire
//   frustum-pointer selection that only shows up in the machine code:
//     debug_render_cluster_pvs != 0, OR render_cluster_index == -1  -> use the fixed global
//       camera/projection block at 0x7c3168 (same constant 0x553560/0x5537c0/0x554850 use);
//     otherwise                                                     -> use this visible cluster's
//       own clipped frustum, &visible_clusters[i].unknown_014 (the block the render module fills
//       in after the portal flood, per structure_bsp_visible_cluster's doc comment).
//   Everything else (the subcluster loop, the surface-bit test/set, the two 0x4000 caps) matches
//   the decompile field for field.
// evidence: objdump -M intel disassembly of 0x553920..0x553a70; types/structures.h
//   structure_bsp_visible_cluster, ScenarioStructureBSPCluster.subclusters,
//   ScenarioStructureBSPSubcluster (world_bounds + surface_indices).
// register convention: cdecl, one stack parameter (the resident structure_bsp tag pointer,
//   forwarded by the caller which already holds it).
// UNSURE: render_frustum_test_bounding_box's own parameter order/registers (out of this module,
//   render, 0x50d5b0) -- the box pointer looked implicit (EDI, never reloaded into another
//   register right before the call) rather than an explicit argument; declared here as an
//   ordinary 3-parameter function with a blam-cc comment recording the observed register loads.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"

extern int32_t render_cluster_index;                     // 0x007c3348
extern uint8_t debug_render_cluster_pvs;                  // 0x00724a45
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern int16_t visible_cluster_count;                     // 0x007d0390
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits]; // 0x007d0394
extern int16_t visible_surface_count;                     // 0x00850394

// render module, out of this batch. blam-cc: ECX -> frustum_or_camera, EDI -> box (implicit,
// never reloaded from another register right before the call site), stack -> flags (always 0
// here).
extern int16_t render_frustum_test_bounding_box(void *frustum_or_camera, void *box, int32_t flags);

void structure_bsp_expand_visible_clusters_by_subcluster(ScenarioStructureBSP *tag)
{
    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)tag->clusters.pointer;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        if (visible_surface_count > 0x3fff) {
            return;
        }
        ScenarioStructureBSPCluster *cluster = &clusters[visible_clusters[i].cluster_index];
        void *frustum_or_camera;
        if (debug_render_cluster_pvs != 0 || render_cluster_index == -1) {
            frustum_or_camera = (void *)0x7c3168;
        } else {
            frustum_or_camera = &visible_clusters[i].unknown_014;
        }

        for (int32_t j = 0; j < (int32_t)cluster->subclusters.count; j++) {
            if (visible_surface_count > 0x3fff) {
                break;
            }
            ScenarioStructureBSPSubcluster *subcluster =
                &((ScenarioStructureBSPSubcluster *)cluster->subclusters.pointer)[j];
            if (render_frustum_test_bounding_box(frustum_or_camera, subcluster, 0) == 0) {
                continue;
            }
            int32_t *indices = (int32_t *)subcluster->surface_indices.pointer;
            for (int32_t k = 0; k < (int32_t)subcluster->surface_indices.count; k++) {
                int32_t surface = indices[k];
                int32_t word = surface >> 5;
                uint32_t mask = 1u << (surface & 0x1f);
                if ((surface_visible_bits[word] & mask) != 0) {
                    continue;
                }
                if (visible_surface_count > 0x3fff) {
                    break;
                }
                surface_visible_bits[word] |= mask;
                visible_surface_count++;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x553920):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00553920(int param_1)

{
  int iVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  short sVar7;
  int *piVar8;

  sVar3 = 0;
  if (0 < DAT_007d0390) {
    do {
      if (0x3fff < (short)DAT_00850394) {
        return;
      }
      iVar6 = (short)(&DAT_007c3390)[sVar3 * 0xd0] * 0x68 + *(int *)(param_1 + 0x138);
      sVar7 = 0;
      if (0 < *(int *)(iVar6 + 0x34)) {
        do {
          if (0x3fff < (short)DAT_00850394) break;
          iVar1 = *(int *)(iVar6 + 0x38) + sVar7 * 0x24;
          sVar2 = render_frustum_test_bounding_box(0);
          if (sVar2 != 0) {
            piVar8 = *(int **)(iVar1 + 0x1c);
            sVar2 = 0;
            if (0 < *(int *)(iVar1 + 0x18)) {
              do {
                iVar4 = *piVar8 >> 5;
                uVar5 = 1 << ((byte)*piVar8 & 0x1f);
                if ((uVar5 & (&DAT_007d0394)[iVar4]) == 0) {
                  if (0x3fff < (short)DAT_00850394) break;
                  (&DAT_007d0394)[iVar4] = (&DAT_007d0394)[iVar4] | uVar5;
                  DAT_00850394._0_2_ = (short)DAT_00850394 + 1;
                }
                piVar8 = piVar8 + 1;
                sVar2 = sVar2 + 1;
              } while ((int)sVar2 < *(int *)(iVar1 + 0x18));
            }
          }
          sVar7 = sVar7 + 1;
        } while ((int)sVar7 < *(int *)(iVar6 + 0x34));
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < DAT_007d0390);
  }
  return;
}
#endif
