// cluster_flood_fill_with_predicate  (Ghidra: FUN_00554e30, still unnamed there)
// address 0x554e30, size 364 bytes
// name confidence: 0.5 -- matches the phase4 summary ("Iteratively flood-fills through cluster
//   portals using a caller-supplied predicate to decide which neighboring clusters to include").
// rewrite confidence: 0.4 -- an iterative (stack-based, not recursive) sibling of
//   cluster_flood_fill_within_radius; param_1/param_2 are declared but never read anywhere in
//   Ghidra's decompile, matching this batch's recurring "forwarded-but-unread" pattern, though
//   here there is no recursive self-call or resolved callee signature to confirm what they would
//   have fed, so they are kept as opaque unused parameters rather than guessed at.
// evidence: types/tags.h ScenarioStructureBSPCluster.portals / ScenarioStructureBSPClusterPortal;
//   the cluster flood-stamp globals shared with cluster_flood_fill_within_radius (this batch).
// register convention: in_AX -> start_cluster. Stack: param_1, param_2 (both unused here),
//   param_3/param_4/param_5 (forwarded verbatim to vector3d_projection_band_test), param_6 ->
//   max_count, param_7 -> output array.
//   // blam-cc: AX -> start_cluster, stack -> the rest
// UNSURE: vector3d_projection_band_test's exact contract (out of this module, math) -- the value
// read from the portal record and passed to it (offset 0x14, `bounding_radius` per
// ScenarioStructureBSPClusterPortal) is reproduced exactly as disassembled/decompiled, but this
// batch has no independent confirmation that a bare float (rather than a pointer into the record)
// is really what the callee wants.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp; // 0x00746f9c
extern int32_t cluster_flood_stamp;         // 0x006e3f04
extern uint8_t cluster_flood_in_progress;   // 0x006e3f01
extern int32_t cluster_flood_stamps[0x200]; // 0x006e3f08

// math module, out of this batch.
extern uint8_t vector3d_projection_band_test(float value, void *param_3, void *param_4,
                                              void *param_5);

// blam-cc: AX -> start_cluster, stack -> the rest
int16_t cluster_flood_fill_with_predicate(void *param_1, void *param_2, void *predicate_param_3,
                                           void *predicate_param_4, void *predicate_param_5,
                                           int16_t start_cluster, int16_t max_count,
                                           int16_t *output)
{
    // A stack-based (not recursive) flood: `stack` holds clusters queued for expansion, LIFO.
    int16_t stack[0x200];
    int32_t stack_top = 0;

    int16_t written = 0;
    cluster_flood_stamp++;
    cluster_flood_in_progress = 1;
    stack[stack_top++] = start_cluster;
    cluster_flood_stamps[start_cluster] = cluster_flood_stamp;

    while (stack_top > 0 && written < max_count) {
        int16_t cluster_index = stack[--stack_top];
        output[written++] = cluster_index;

        ScenarioStructureBSPCluster *cluster =
            &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[cluster_index];
        ScenarioStructureBSPClusterPortalIndex *portal_refs =
            (ScenarioStructureBSPClusterPortalIndex *)cluster->portals.pointer;
        ScenarioStructureBSPClusterPortal *portals =
            (ScenarioStructureBSPClusterPortal *)structure_bsp->cluster_portals.pointer;

        for (int32_t i = 0; i < (int32_t)cluster->portals.count; i++) {
            ScenarioStructureBSPClusterPortal *portal = &portals[portal_refs[i].portal];
            int16_t neighbor = (portal->front_cluster == (uint16_t)cluster_index)
                                    ? (int16_t)portal->back_cluster
                                    : (int16_t)portal->front_cluster;
            if (cluster_flood_stamps[neighbor] == cluster_flood_stamp) {
                continue;
            }
            if (!vector3d_projection_band_test(portal->bounding_radius, predicate_param_3,
                                                predicate_param_4, predicate_param_5)) {
                continue;
            }
            cluster_flood_stamps[neighbor] = cluster_flood_stamp;
            stack[stack_top++] = neighbor;
        }
    }

    cluster_flood_in_progress = 0;
    return written;
}

#if 0
Original Ghidra decompilation (0x554e30):

undefined4
FUN_00554e30(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,short param_6,int param_7)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  short in_AX;
  short sVar6;
  int iVar7;
  undefined2 uVar9;
  short *psVar8;
  short sVar10;
  short sVar11;
  short sVar12;
  short asStackY_10400 [32742];
  short local_400 [512];

  sVar10 = 0;
  DAT_006e3f04 = DAT_006e3f04 + 1;
  iVar7 = (int)in_AX;
  sVar11 = 1;
  DAT_006e3f01 = 1;
  if (*(int *)(&DAT_006e3f08 + iVar7 * 4) != DAT_006e3f04) {
    *(int *)(&DAT_006e3f08 + iVar7 * 4) = DAT_006e3f04;
  }
  local_400[0] = in_AX;
  do {
    iVar4 = DAT_00746f9c;
    uVar9 = (undefined2)((uint)iVar7 >> 0x10);
    if (param_6 <= sVar10) break;
    sVar11 = sVar11 + -1;
    sVar1 = local_400[sVar11];
    iVar7 = sVar1 * 0x68 + *(int *)(DAT_00746f9c + 0x138);
    *(short *)(param_7 + sVar10 * 2) = sVar1;
    iVar2 = *(int *)(iVar7 + 0x5c);
    sVar10 = sVar10 + 1;
    sVar6 = 0;
    if (0 < iVar2) {
      iVar3 = *(int *)(iVar7 + 0x60);
      iVar4 = *(int *)(iVar4 + 0x158);
      iVar7 = 0;
      do {
        psVar8 = (short *)(*(short *)(iVar3 + iVar7 * 2) * 0x40 + iVar4);
        sVar12 = *psVar8;
        if (sVar12 == sVar1) {
          sVar12 = psVar8[1];
        }
        if ((*(int *)(&DAT_006e3f08 + sVar12 * 4) != DAT_006e3f04) &&
           (cVar5 = vector3d_projection_band_test
                              (*(undefined4 *)(psVar8 + 10),param_3,param_4,param_5), cVar5 != '\0')
           ) {
          *(int *)(&DAT_006e3f08 + sVar12 * 4) = DAT_006e3f04;
          local_400[sVar11] = sVar12;
          sVar11 = sVar11 + 1;
        }
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < iVar2);
    }
    uVar9 = (undefined2)((uint)iVar7 >> 0x10);
  } while (0 < sVar11);
  DAT_006e3f01 = 0;
  return CONCAT22(uVar9,sVar10);
}
#endif
