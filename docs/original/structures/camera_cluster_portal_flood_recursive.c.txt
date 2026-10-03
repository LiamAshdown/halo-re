// camera_cluster_portal_flood_recursive  (Ghidra: FUN_005545d0, already named)
// address 0x5545d0, size 632 bytes
// name confidence: 0.7 -- already named; matches the body exactly (recursive portal flood from a
//   starting cluster, clipping the view polygon at each portal crossing).
// rewrite confidence: 0.7 -- the largest and most intricate function in this batch. The control
//   flow and arithmetic are Ghidra's; the polygon hand-off is reconstructed from disassembly
//   (0x5546eb..0x554820) and this is where the earlier rewrite of this file was wrong. There are
//   TWO local polygon2d buffers, not one:
//     - P1 at esp+0x28 (count) / esp+0x2c (points): the EDX output argument of
//       structure_bsp_portal_test_and_project (`lea edx,[esp+0x2c]` at 0x554767).
//     - P2 at esp+0x82c / esp+0x830: the clip destination (`lea edx,[esp+0x834]` at 0x5547be with
//       one push already done, then `mov WORD [esp+0x82c],ax` for the count at 0x5547e8), and it
//       is P2 that gets pushed for the recursive call (`lea eax,[esp+0x82c]` at 0x5547f2).
//   0x28 + 0x804 == 0x82c, so both buffers are exactly one polygon2d.
//   polygon2d_clip_to_planes is called with its full canonical argument list
//   (src/math/polygon2d_clip_to_planes.c): ECX/EDX are the SUBJECT polygon -- P1's count and
//   points -- while the caller's `view_polygon` is the CLIP polygon on the stack, and the output
//   goes to P2's point array. The earlier rewrite treated view_polygon as the subject, reused one
//   buffer for both roles, and passed the struct base rather than the point array as the
//   destination (a 4-byte shift on every output point).
// evidence: types/structures.h structure_bsp_visible_cluster, polygon2d, and the whole globals
//   section this function's body accounts for (cluster_visible_bits, visible_clusters,
//   visible_cluster_count, cluster_visible_index, k_default_screen_bounds, flood_recursion_bits).
// register convention: cdecl, 2 stack parameters (matches Ghidra's own signature exactly).
// UNSURE: none outstanding for this function's own body. Its callees'
//   own files carry whatever is left.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern int32_t render_cluster_index;        // 0x007c3348
extern uint32_t *flood_recursion_bits;      // 0x006e3af8
extern uint32_t cluster_visible_bits[0x10]; // 0x007c3350
extern int16_t visible_cluster_count;       // 0x007d0390
extern int16_t cluster_visible_index[0x200]; // 0x006e3afc
extern real_bounds *k_default_screen_bounds; // 0x00696744
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern uint8_t render_cluster_has_sky;      // 0x007c334d
extern float portal_visibility_tolerance;   // 0x007c3154 (render camera block +0x54)

// math module, canonical form. blam-cc: ECX -> vertex_count, EDX -> vertices
extern int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices,
                                         int16_t clip_point_count, real_point2d *clip_points,
                                         int16_t maximum_count, real_point2d *out,
                                         real epsilon); // 0x4caee0
extern void polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon);
extern uint8_t structure_bsp_portal_test_and_project(char same_side, int16_t portal_index,
                                                      polygon2d *out);
// this batch (0x554a20): "are these points all behind (within tolerance of) a camera-relative
// band plane" -- used here as a cheap band-visibility test before falling back to the sky check.
extern uint8_t structure_bsp_points_within_band(real_point3d *points, int16_t point_count,
                                                 float tolerance);

void camera_cluster_portal_flood_recursive(int16_t cluster_index, polygon2d *view_polygon)
{
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];

    uint32_t bit = 1u << (cluster_index & 0x1f);
    int32_t word = cluster_index >> 5;
    flood_recursion_bits[word] |= bit;

    if ((cluster_visible_bits[word] & bit) == 0) {
        int16_t visible_index = visible_cluster_count;
        cluster_visible_index[cluster_index] = visible_index;
        visible_cluster_count++;
        visible_clusters[visible_index].cluster_index = cluster_index;
        visible_clusters[visible_index].screen_bounds_x = k_default_screen_bounds[0];
        visible_clusters[visible_index].screen_bounds_y = k_default_screen_bounds[1];
    }
    cluster_visible_bits[word] |= bit;

    int16_t visible_index = cluster_visible_index[cluster_index];
    polygon2d_bounds_expand((real_bounds *)&visible_clusters[visible_index].screen_bounds_x,
                             view_polygon);

    ScenarioStructureBSPClusterPortalIndex *portal_refs =
        (ScenarioStructureBSPClusterPortalIndex *)cluster->portals.pointer;
    ScenarioStructureBSPClusterPortal *portals =
        (ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer;

    for (int32_t i = 0; i < (int32_t)cluster->portals.count; i++) {
        ScenarioStructureBSPClusterPortal *portal = &portals[portal_refs[i].portal];
        uint8_t same_side = (portal->front_cluster == (uint16_t)cluster_index);
        int16_t neighbor = same_side ? (int16_t)portal->back_cluster
                                      : (int16_t)portal->front_cluster;
        if (neighbor < 0 || neighbor >= global_structure_bsp->clusters.count) {
            continue;
        }
        uint32_t neighbor_bit = 1u << (neighbor & 0x1f);
        int32_t neighbor_word = neighbor >> 5;
        if ((flood_recursion_bits[neighbor_word] & neighbor_bit) != 0) {
            continue; // already on the recursion stack
        }
        // The neighbor must also be in the camera cluster's own PVS row.
        int32_t row_dwords = (global_structure_bsp->clusters.count + 0x1f) >> 5;
        uint32_t *pvs_row = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                          render_cluster_index * row_dwords * 4);
        if ((pvs_row[neighbor_word] & neighbor_bit) == 0) {
            continue;
        }

        polygon2d portal_polygon;    // P1, esp+0x28
        polygon2d clipped_polygon;   // P2, esp+0x82c
        uint8_t project_result =
            structure_bsp_portal_test_and_project(same_side, portal_refs[i].portal,
                                                   &portal_polygon);
        polygon2d *next_polygon = view_polygon;
        if (project_result != 2) {
            if (project_result != 0 ||
                (render_cluster_has_sky == 0 &&
                 // RESOLVED (was UNSURE): disassembly at 0x55479e/0x5547a1 loads EDX from
                 // [esi+0x38] and SI from [esi+0x34], i.e. the portal's own vertices.pointer and
                 // vertices.count, with the tolerance at 0x007c3154 pushed as the single stack
                 // argument -- matching structure_bsp_points_within_band's EDX/SI convention.
                 structure_bsp_points_within_band(
                     (real_point3d *)portal->vertices.pointer, (int16_t)portal->vertices.count,
                     portal_visibility_tolerance) == 0)) {
                continue;
            }
            // The clip writes into the POINTS array of the local polygon and the count is stored
            // separately 4 bytes in front of it (`lea edx,[esp+0x834]` for the destination,
            // `mov WORD [esp+0x82c],ax` for the count) -- so the destination is
            // &portal_polygon.points[0], not &portal_polygon. An earlier rewrite passed the
            // struct base, which would have shifted every output point back by 4 bytes.
            int16_t clipped_count = polygon2d_clip_to_planes(
                portal_polygon.point_count, &portal_polygon.points[0],
                view_polygon->point_count, &view_polygon->points[0], 0x100,
                &clipped_polygon.points[0], 9.99999975e-05f); // 0x5547b9 pushes the float bits 0x38d1b717
            clipped_polygon.point_count = clipped_count;   // stored before the test, as in the original
            if (clipped_count < 1) {
                if (clipped_count != -1) {
                    continue;
                }
                // -1 means "unchanged": recurse with the caller's polygon as-is
            } else {
                next_polygon = &clipped_polygon;
            }
        }
        camera_cluster_portal_flood_recursive(neighbor, next_polygon);
    }

    flood_recursion_bits[word] &= ~bit;
}

#if 0
Original Ghidra decompilation (0x5545d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void camera_cluster_portal_flood_recursive(short param_1,short *param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  undefined *puVar4;
  int iVar5;
  char cVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  uint uVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  uint uVar15;
  short *psVar16;
  int iVar17;
  bool bVar18;
  short local_804 [2];
  undefined1 local_800 [2044];
  undefined4 uStack_4;

  iVar5 = DAT_00746f9c;
  uStack_4 = 0x5545da;
  iVar9 = (int)param_1;
  iVar12 = iVar9 * 0x68 + *(int *)(DAT_00746f9c + 0x138);
  iVar2 = *(int *)(DAT_00746f9c + 0x134);
  iVar17 = (int)DAT_007c3348;
  iVar3 = *(int *)(DAT_00746f9c + 0x14c);
  uVar15 = 1 << ((byte)param_1 & 0x1f);
  iVar13 = iVar9 >> 5;
  iVar14 = iVar13 * 4;
  *(uint *)(DAT_006e3af8 + iVar14) = *(uint *)(DAT_006e3af8 + iVar14) | uVar15;
  if (((&DAT_007c3350)[iVar13] & uVar15) == 0) {
    (&DAT_006e3afc)[iVar9] = DAT_007d0390;
    puVar4 = PTR_DAT_00696744;
    iVar10 = (short)(&DAT_006e3afc)[iVar9] * 0x1a0;
    DAT_007d0390 = DAT_007d0390 + 1;
    (&DAT_007c3390)[(short)(&DAT_006e3afc)[iVar9] * 0xd0] = param_1;
    *(undefined4 *)(&DAT_007c3394 + iVar10) = *(undefined4 *)puVar4;
    *(undefined4 *)(&DAT_007c3398 + iVar10) = *(undefined4 *)(puVar4 + 4);
    *(undefined4 *)(&DAT_007c339c + iVar10) = *(undefined4 *)(puVar4 + 8);
    *(undefined4 *)(&DAT_007c33a0 + iVar10) = *(undefined4 *)(puVar4 + 0xc);
  }
  (&DAT_007c3350)[iVar13] = (&DAT_007c3350)[iVar13] | uVar15;
  polygon2d_bounds_expand();
  iVar9 = 0;
  sVar8 = 0;
  if (0 < *(int *)(iVar12 + 0x5c)) {
    do {
      iVar13 = *(int *)(iVar5 + 0x158);
      psVar16 = (short *)(*(short *)(*(int *)(iVar12 + 0x60) + iVar9 * 2) * 0x40 + iVar13);
      bVar18 = *psVar16 == param_1;
      sVar1 = psVar16[bVar18];
      if ((-1 < sVar1) && ((int)sVar1 < *(int *)(iVar5 + 0x134))) {
        uVar11 = 1 << ((byte)sVar1 & 0x1f);
        iVar9 = ((int)sVar1 >> 5) * 4;
        if (((*(uint *)(iVar9 + DAT_006e3af8) & uVar11) == 0) &&
           ((*(uint *)(iVar9 + iVar3 + (iVar2 + 0x1f >> 5) * iVar17 * 4) & uVar11) != 0)) {
          sVar7 = FUN_005549c0(bVar18);
          psVar16 = param_2;
          if (sVar7 != 2) {
            if ((sVar7 != 0) ||
               ((DAT_007c334d == '\0' && (cVar6 = FUN_00554a20(DAT_007c3154), cVar6 == '\0'))))
            goto LAB_00554813;
            local_804[0] = polygon2d_clip_to_planes(*param_2,param_2 + 2,0x100,local_800,0x38d1b717)
            ;
            if (local_804[0] < 1) {
              if (local_804[0] != -1) goto LAB_00554813;
            }
            else {
              psVar16 = local_804;
            }
          }
          camera_cluster_portal_flood_recursive
                    (CONCAT22((short)((uint)iVar13 >> 0x10),sVar1),psVar16);
        }
      }
LAB_00554813:
      sVar8 = sVar8 + 1;
      iVar9 = (int)sVar8;
    } while (iVar9 < *(int *)(iVar12 + 0x5c));
  }
  *(uint *)(DAT_006e3af8 + iVar14) = *(uint *)(DAT_006e3af8 + iVar14) & ~uVar15;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
