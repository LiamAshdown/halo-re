// structure_bsp_cluster_visibility_update  (Ghidra: FUN_005537c0, still unnamed there)
// address 0x5537c0, size 345 bytes
// name confidence: 0.6 -- matches the phase4 summary ("top-level per-frame entry point that
//   resets visibility state, floods the cluster/portal graph from the camera, and drives either
//   the plane-based or bounding-box-based cluster visibility test") with no other candidate name.
// rewrite confidence: 0.55 -- the two reset loops, the debug PVS override and the final dispatch
//   are all straightforward once named; the one thing verified by disassembly rather than trusted
//   from the decompile is the debug-branch call to FUN_0050ddc0 (see below).
// evidence: types/structures.h's global list (cluster_visible_bits, visible_clusters,
//   visible_cluster_count, surface_visible_bits, visible_surface_count, debug_render_cluster_pvs,
//   cluster_visible_index, no_subcluster_path_taken) and the ScenarioStructureBSPCluster.
//   subclusters field that picks 0x553920 vs 0x553a70. Disassembly of the debug branch shows
//   `mov ecx,0x7c3168; call 0x50ddc0` with EAX = &visible_clusters[i].screen_bounds_x -- the same
//   "compute camera screen clip rect" helper used by 0x553560 and 0x5544f0, here fed the fixed
//   global projection context instead of a per-call camera pointer.
// register convention: no incoming parameters; every input is a global. No return value.
// UNSURE: none outside the general FUN_0050ddc0 contract already noted in the mirror-query file.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp;   // 0x00746f9c
extern int32_t render_cluster_index;          // 0x007c3348
extern uint32_t cluster_visible_bits[0x10];   // 0x007c3350
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern int16_t visible_cluster_count;         // 0x007d0390
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits]; // 0x007d0394
extern int16_t visible_surface_count;         // 0x00850394
extern uint8_t debug_render_cluster_pvs;      // 0x00724a45
extern int16_t cluster_visible_index[0x200];  // 0x006e3afc
extern uint8_t no_subcluster_path_taken;      // 0x006e3af0

// render module, below this batch's assigned range; see structure_bsp_mirror_query.c.
// blam-cc: EAX -> out (float[4]), ECX -> camera
extern void FUN_0050ddc0(float *out, void *camera);

extern void structure_bsp_camera_visibility_pass(void); // 0x5544f0, this module: portal flood
    // from the camera cluster, then a clipped frustum per visible cluster

// this batch: the two cluster-visibility expansion passes (0x553920 subcluster-aabb path,
// 0x553a70 no-subcluster/plane path), chosen below by whether cluster 0 has any subclusters.
extern void structure_bsp_expand_visible_clusters_by_subcluster(ScenarioStructureBSP *tag);
extern void structure_bsp_expand_visible_clusters_by_plane(ScenarioStructureBSP *tag);

void structure_bsp_cluster_visibility_update(void)
{
    ScenarioStructureBSP *tag = global_structure_bsp;
    // When the camera is outside the BSP entirely, mark every cluster visible (-1 fills every
    // bit) so nothing gets culled; otherwise start with nothing visible and let the flood below
    // fill it in.
    uint32_t fill = (render_cluster_index != -1) ? 0 : 0xffffffff;
    int32_t cluster_dwords = (tag->clusters.count + 0x1f) >> 5;
    for (int32_t i = 0; i < cluster_dwords; i++) {
        cluster_visible_bits[i] = fill;
    }

    visible_surface_count = 0;
    int32_t surface_dwords = (tag->surfaces.count + 0x1f) >> 5;
    for (int32_t i = 0; i < surface_dwords; i++) {
        surface_visible_bits[i] = 0;
    }

    visible_cluster_count = 0;
    structure_bsp_camera_visibility_pass();

    if (debug_render_cluster_pvs != 0) {
        // Debug override: replace the portal-flood result with the raw PVS row of the camera's
        // own cluster, rebuilding the visible-cluster list (and its screen bounds) directly.
        visible_cluster_count = 0;
        int32_t row_dwords = (tag->clusters.count + 0x1f) >> 5;
        uint32_t *pvs_row = (uint32_t *)((uint8_t *)tag->cluster_data.pointer +
                                          render_cluster_index * row_dwords * 4);
        for (int32_t i = 0; i < row_dwords; i++) {
            cluster_visible_bits[i] = pvs_row[i];
        }
        if (tag->clusters.count > 0) {
            for (int16_t cluster_index = 0; cluster_index < tag->clusters.count; cluster_index++) {
                if ((cluster_visible_bits[cluster_index >> 5] & (1u << (cluster_index & 0x1f))) == 0) {
                    continue;
                }
                int16_t visible_index = visible_cluster_count++;
                cluster_visible_index[cluster_index] = visible_index;
                visible_clusters[visible_index].cluster_index = cluster_index;
                FUN_0050ddc0((float *)&visible_clusters[visible_index].screen_bounds_x,
                             (void *)0x7c3168);
            }
        }
    }

    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)tag->clusters.pointer;
    if (clusters[0].subclusters.count == 0) {
        if (no_subcluster_path_taken == 0) {
            no_subcluster_path_taken = 1;
        }
        structure_bsp_expand_visible_clusters_by_plane(tag);
        return;
    }
    structure_bsp_expand_visible_clusters_by_subcluster(tag);
}

#if 0
Original Ghidra decompilation (0x5537c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005537c0(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int extraout_EDX;
  undefined4 *puVar6;
  undefined4 *puVar7;

  iVar1 = DAT_00746f9c;
  uVar2 = 0xffffffff;
  if (_DAT_007c3348 != -1) {
    uVar2 = 0;
  }
  puVar6 = &DAT_007c3350;
  for (uVar4 = *(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5 & 0x3fffffff; uVar4 != 0;
      uVar4 = uVar4 - 1) {
    *puVar6 = uVar2;
    puVar6 = puVar6 + 1;
  }
  for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
    *(char *)puVar6 = (char)uVar2;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  DAT_00850394._0_2_ = 0;
  puVar6 = &DAT_007d0394;
  for (uVar4 = *(int *)(iVar1 + 0xf8) + 0x1f >> 5 & 0x3fffffff; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  DAT_007d0390 = 0;
  FUN_005544f0();
  if (DAT_00724a45 != '\0') {
    DAT_007d0390 = 0;
    iVar5 = *(int *)(iVar1 + 0x134) + 0x1f >> 5;
    puVar6 = (undefined4 *)(*(int *)(iVar1 + 0x14c) + DAT_007c3348 * iVar5 * 4);
    puVar7 = &DAT_007c3350;
    for (; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    iVar5 = 0;
    if (0 < *(int *)(iVar1 + 0x134)) {
      iVar3 = 0;
      do {
        if (((&DAT_007c3350)[iVar3 >> 5] & 1 << ((byte)iVar3 & 0x1f)) != 0) {
          (&DAT_006e3afc)[iVar3] = DAT_007d0390;
          DAT_007d0390 = DAT_007d0390 + 1;
          (&DAT_007c3390)[(short)(&DAT_006e3afc)[iVar3] * 0xd0] = (short)iVar5;
          FUN_0050ddc0();
          iVar5 = extraout_EDX;
        }
        iVar5 = iVar5 + 1;
        iVar3 = (int)(short)iVar5;
      } while (iVar3 < *(int *)(iVar1 + 0x134));
    }
  }
  if (*(int *)(*(int *)(iVar1 + 0x138) + 0x34) == 0) {
    if (DAT_006e3af0 == '\0') {
      DAT_006e3af0 = '\x01';
    }
    FUN_00553a70(iVar1);
    return;
  }
  FUN_00553920(iVar1);
  return;
}
#endif
