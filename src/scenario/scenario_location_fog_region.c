// scenario_location_fog_region  (Ghidra: FUN_0053ec30, still unnamed there; named for this
// rewrite from its role established in out/phase4/scenario_types_notes.md: it resolves the
// ScenarioStructureBSPCluster.fog word of a location's cluster down to a fog region index,
// following the plane indirection when the cluster's fog field names a fog plane instead of a
// region directly, and gates the result on which side of that plane a supplied point sits.)
// address 0x53ec30, size 209 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/scenario_types_notes.md ("Tag offsets confirmed against types/tags.h":
//   ScenarioStructureBSPCluster.fog +0x02, ScenarioStructureBSPFogPlane.front_region +0x00,
//   Fog.flags bit 0 is_water, Fog.distance_to_water_plane +0x74) and its own register-convention
//   table ("0x53ec30: EAX = bsp_leaf_reference *, EBX = point or NULL. Returns an int16 fog
//   region."); confirmed instruction by instruction against objdump -d -M intel
//   --start-address=0x53ec30 --stop-address=0x53ed60 bin/halo.exe.
// register convention: EAX -> leaf (bsp_leaf_reference *), EBX -> point (real_point3d *, may be
//   NULL); no stack parameters. Return value is only ever meaningful in AX (int16_t): the early
//   failure paths load it via `mov ax,bp` with bp pinned to -1 and leave EAX's high half as
//   whatever the caller's own register held, exactly like structure_bsp_load's documented upper-
//   byte garbage (src/cache/structure_bsp_load.c) -- no caller (see 0x53ed10's sibling
//   0x53ed60, and 0x53ee00) ever reads more than the low 16 bits, so this is typed int16_t
//   rather than reproduced bit for bit.
//   // blam-cc: EAX -> leaf, EBX -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "scenario.h"
#include "fn_scenario.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern tag_instance *tag_instances;                // 0x0087bc14

// blam-cc: AX -> fog region


// blam-cc: EAX -> leaf, EBX -> point
// Resolves the fog region that applies at a location. The cluster's fog word (+0x02) is either
// -1 (no fog), a region index with the top bit clear, or -- with the top bit set -- the index of
// a fog plane whose front_region names the region and whose plane equation, tested against
// `point` (when supplied) biased by the region's own water depth, decides whether the region is
// in effect on this side of the plane at all.
int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point)
{
    ScenarioStructureBSPCluster *cluster;
    int16_t fog;
    ScenarioStructureBSPFogPlane *plane;
    int16_t region;
    uint32_t fog_tag;
    float water_bias;

    if (leaf->cluster_index == -1) {
        return -1;
    }

    cluster = &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                  [leaf->cluster_index];
    fog = (int16_t)cluster->fog;
    if (fog == -1) {
        return -1;
    }
    if (fog >= 0) {
        // top bit clear: the cluster names a fog region directly
        return fog & k_cluster_fog_index_mask;
    }

    // top bit set: the cluster names a fog plane; resolve its front region and test the plane
    plane = &((ScenarioStructureBSPFogPlane *)global_structure_bsp->fog_planes.pointer)
                [fog & k_cluster_fog_index_mask];
    region = (int16_t)plane->front_region;

    water_bias = 0.0f;
    fog_tag = scenario_fog_region_resolve_tag(region);
    if (fog_tag != 0xffffffff) {
        Fog *fog_data = (Fog *)tag_instances[fog_tag & 0xffff].data;
        if (fog_data->flags & k_fog_flag_is_water) {
            water_bias = fog_data->distance_to_water_plane;
        }
    }

    if (point != 0) {
        // summed k, j, i as at 0x53ecb5; the test is `test ah,5 / jnp` after fcomp 0.0, so only
        // a distance strictly below 0 keeps the region (0 and NaN both give -1)
        float distance = plane->plane.vector.k * point->z + plane->plane.vector.j * point->y +
                          plane->plane.vector.i * point->x - plane->plane.w + water_bias;
        if (!(distance < 0.0f)) {
            // point is on the outward side of the plane: the region does not apply here
            return -1;
        }
    }
    return region;
}

#if 0
Original Ghidra decompilation (0x53ec30):

uint FUN_0053ec30(void)

{
  float fVar1;
  short sVar2;
  ushort uVar3;
  byte *pbVar4;
  int in_EAX;
  ushort *puVar5;
  uint uVar6;
  float *unaff_EBX;

  if (*(short *)(in_EAX + 4) == -1) {
    return 0xffff;
  }
  sVar2 = *(short *)(*(short *)(in_EAX + 4) * 0x68 + *(int *)(DAT_00746f9c + 0x138) + 2);
  if (sVar2 == -1) {
    return 0xffff;
  }
  if (-1 < sVar2) {
    return (int)sVar2 & 0x7fff;
  }
  puVar5 = (ushort *)(((int)sVar2 & 0x7fffU) * 0x20 + *(int *)(DAT_00746f9c + 0x17c));
  uVar3 = *puVar5;
  uVar6 = FUN_0053ed10();
  fVar1 = 0.0;
  if ((uVar6 != 0xffffffff) &&
     (pbVar4 = *(byte **)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), (*pbVar4 & 1) != 0)) {
    fVar1 = *(float *)(pbVar4 + 0x74);
  }
  if ((unaff_EBX != (float *)0x0) &&
     (0.0 <= ((*(float *)(puVar5 + 2) * *unaff_EBX +
              *(float *)(puVar5 + 4) * unaff_EBX[1] + *(float *)(puVar5 + 6) * unaff_EBX[2]) -
             *(float *)(puVar5 + 8)) + fVar1)) {
    return 0xffff;
  }
  return (uint)uVar3;
}
#endif
