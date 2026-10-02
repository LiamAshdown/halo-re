// cluster_flood_fill_within_radius  (Ghidra: FUN_00554d10, already named)
// address 0x554d10, size 281 bytes
// name confidence: 0.6 -- already named; matches its body (recursive portal flood, gated by a
//   sphere-vs-portal test, collecting cluster indices).
// rewrite confidence: 0.85 (VERIFIED against objdump 0x554d10..0x554e28: writes the cluster when the budget allows, stamps it, then for each cluster portal (bsp+0x158, 0x40 each; front/back words) recurses into the unstamped far cluster when structure_bsp_portal_sphere_test passes; returns 1 + the children's counts) -- Ghidra recovered a clean, fully-stack, 5-parameter signature with no
//   in_stack/extraout artifacts; the only thing not visible in the decompile (calls shown with no
//   arguments) is which registers feed FUN_00554b00, resolved by disassembly.
// evidence: objdump -M intel disassembly of 0x554d10..0x554e30, cross-checked against the sibling
//   caller FUN_00554cb0 (this batch), whose own disassembly resolves the *same* ambiguity from the
//   other side (its "unused" param_1 is this function's point pointer, forwarded opaquely -- never
//   dereferenced by either function directly, only handed to structure_bsp_portal_sphere_test).
// register convention: cdecl, all 5 parameters on the stack.
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern int32_t cluster_flood_stamp;         // 0x006e3f04
extern int32_t cluster_visit_stamp[0x200]; // 0x006e3f08

// this batch (0x554b00): tests whether a sphere (point, tolerance) intersects a specific portal.
// blam-cc: EAX -> global_structure_bsp, ECX -> point, DX -> portal_index, stack -> tolerance
extern uint8_t structure_bsp_portal_sphere_test(ScenarioStructureBSP *global_structure_bsp,
    real_point3d *point, int16_t portal_index, float tolerance); // 0x554b00, this module

int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point,
                                          float tolerance, int32_t remaining_budget,
                                          int16_t *output)
{
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];

    if (remaining_budget > 0) {
        *output++ = cluster_index;
    }
    if (cluster_visit_stamp[cluster_index] != cluster_flood_stamp) {
        cluster_visit_stamp[cluster_index] = cluster_flood_stamp;
    }

    int32_t written = 1;
    int32_t next_budget = remaining_budget - 1;
    ScenarioStructureBSPClusterPortalIndex *portal_refs =
        (ScenarioStructureBSPClusterPortalIndex *)cluster->portals.pointer;
    ScenarioStructureBSPClusterPortal *portals =
        (ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer;

    for (int32_t i = 0; i < (int32_t)cluster->portals.count; i++) {
        ScenarioStructureBSPClusterPortal *portal = &portals[portal_refs[i].portal];
        int16_t neighbor = (portal->front_cluster == (uint16_t)cluster_index)
                                ? (int16_t)portal->back_cluster
                                : (int16_t)portal->front_cluster;
        if (cluster_visit_stamp[neighbor] == cluster_flood_stamp) {
            continue;
        }
        if (!structure_bsp_portal_sphere_test(global_structure_bsp, point, portal_refs[i].portal,
                                             tolerance)) {
            continue;
        }
        int32_t child_count =
            cluster_flood_fill_within_radius(neighbor, point, tolerance, next_budget, output);
        written += child_count;
        next_budget -= child_count;
        output += child_count;
    }
    return written;
}

#if 0
Original Ghidra decompilation (0x554d10):

undefined4
cluster_flood_fill_within_radius
          (short param_1,undefined4 param_2,undefined4 param_3,int param_4,short *param_5)

{
  short sVar1;
  int iVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  undefined4 uVar6;
  short *psVar7;
  undefined4 unaff_EBX;
  undefined2 uVar8;
  int iVar9;
  int iVar10;

  iVar2 = DAT_00746f9c;
  uVar8 = (undefined2)((uint)unaff_EBX >> 0x10);
  iVar5 = (int)param_1;
  iVar10 = iVar5 * 0x68 + *(int *)(DAT_00746f9c + 0x138);
  iVar9 = param_4 + -1;
  if (0 < (short)param_4) {
    *param_5 = param_1;
    param_5 = param_5 + 1;
  }
  if (*(int *)(&DAT_006e3f08 + iVar5 * 4) != DAT_006e3f04) {
    *(int *)(&DAT_006e3f08 + iVar5 * 4) = DAT_006e3f04;
  }
  uVar6 = 1;
  param_4 = 1;
  sVar4 = 0;
  if (0 < *(int *)(iVar10 + 0x5c)) {
    iVar5 = 0;
    while( true ) {
      psVar7 = (short *)(*(short *)(*(int *)(iVar10 + 0x60) + iVar5 * 2) * 0x40 +
                        *(int *)(iVar2 + 0x158));
      sVar1 = *psVar7;
      uVar6 = CONCAT22((short)((uint)iVar2 >> 0x10),sVar1);
      if (sVar1 == param_1) {
        uVar6 = CONCAT22(uVar8,psVar7[1]);
      }
      if ((*(int *)(&DAT_006e3f08 + (short)uVar6 * 4) != DAT_006e3f04) &&
         (cVar3 = FUN_00554b00(param_3), cVar3 != '\0')) {
        iVar5 = cluster_flood_fill_within_radius(uVar6,param_2,param_3,iVar9,param_5);
        param_4 = param_4 + iVar5;
        iVar9 = iVar9 - iVar5;
        param_5 = param_5 + (short)iVar5;
      }
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
      if (*(int *)(iVar10 + 0x5c) <= iVar5) break;
      uVar8 = (undefined2)((uint)uVar6 >> 0x10);
    }
    uVar6 = CONCAT22(sVar4 >> 0xf,(undefined2)param_4);
  }
  return uVar6;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
