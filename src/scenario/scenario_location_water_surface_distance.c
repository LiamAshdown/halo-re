// scenario_location_water_surface_distance  (Ghidra: FUN_0053ee00, still unnamed there; named
// for this rewrite -- the signed-distance counterpart of scenario_location_fog_region: instead
// of returning the fog region, it returns how far `point` is from the water surface the region's
// fog plane describes.)
// address 0x53ee00, size 163 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/scenario_types_notes.md's register table ("0x53ee00: EAX =
//   bsp_leaf_reference *, EDI = point. Returns a float in ST0.") and its tuning-literal table
//   (0x00672bd4 -FLT_MAX = "result when the location has no fog", 0x00672be0 +FLT_MAX = "result
//   when the fog is water but has no plane"); same ScenarioStructureBSPCluster.fog /
//   ScenarioStructureBSPFogPlane.front_region / Fog.flags,.distance_to_water_plane offsets as
//   scenario_location_fog_region (0x53ec30), confirmed the same way. Confirmed instruction by
//   instruction against objdump -d -M intel --start-address=0x53ee00 --stop-address=0x53eeb0
//   bin/halo.exe: the "no plane" case is exactly the *when the cluster's fog field is already a
//   region index* case in 0x53ec30 (top bit of the fog word clear).
// register convention: EAX -> leaf (bsp_leaf_reference *), EDI -> point (real_point3d *); no
//   stack parameters.
//   // blam-cc: EAX -> leaf, EDI -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "scenario.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern tag_instance *tag_instances;                // 0x0087bc14

// blam-cc: AX -> fog_region
extern uint32_t scenario_fog_region_resolve_tag(int16_t fog_region); // this module, 0x53ed10

// blam-cc: EAX -> leaf, EDI -> point
// Signed distance from `point` to the water surface plane of the location's fog, biased by the
// fog's own distance_to_water_plane. Returns -FLT_MAX when the location has no fog (or the fog
// isn't water), and +FLT_MAX when the fog is water but the cluster's fog field already names a
// region directly (no plane to measure against).
float scenario_location_water_surface_distance(bsp_leaf_reference *leaf, real_point3d *point)
{
    ScenarioStructureBSPCluster *cluster;
    int16_t fog;
    ScenarioStructureBSPFogPlane *plane;
    int16_t region;
    uint32_t fog_tag;
    Fog *fog_data;

    if (leaf->cluster_index == -1) {
        return -3.4028235e+38f;
    }

    cluster = &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                  [leaf->cluster_index];
    fog = (int16_t)cluster->fog;
    if (fog == -1) {
        return -3.4028235e+38f;
    }

    plane = 0;
    if (fog < 0) {
        // top bit set: the cluster names a fog plane
        plane = &((ScenarioStructureBSPFogPlane *)global_structure_bsp->fog_planes.pointer)
                    [fog & k_cluster_fog_index_mask];
        region = (int16_t)plane->front_region;
    } else {
        region = fog & k_cluster_fog_index_mask;
    }

    fog_tag = scenario_fog_region_resolve_tag(region);
    if (fog_tag == 0xffffffff) {
        return -3.4028235e+38f;
    }
    fog_data = (Fog *)tag_instances[fog_tag & 0xffff].data;
    if (!(fog_data->flags & k_fog_flag_is_water)) {
        return -3.4028235e+38f;
    }

    if (plane == 0) {
        return 3.4028235e+38f;
    }

    // summed k, j, i as at 0x53ee7d
    return -((plane->plane.vector.k * point->z + plane->plane.vector.j * point->y +
              plane->plane.vector.i * point->x - plane->plane.w) +
             fog_data->distance_to_water_plane);
}

#if 0
Original Ghidra decompilation (0x53ee00):

float10 FUN_0053ee00(void)

{
  short sVar1;
  byte *pbVar2;
  int in_EAX;
  uint uVar3;
  float *pfVar4;
  float *unaff_EDI;
  float10 fVar5;
  float10 extraout_ST0;

  fVar5 = (float10)-3.4028235e+38;
  if (*(short *)(in_EAX + 4) != -1) {
    sVar1 = *(short *)(*(short *)(in_EAX + 4) * 0x68 + *(int *)(DAT_00746f9c + 0x138) + 2);
    if (sVar1 != -1) {
      if (sVar1 < 0) {
        pfVar4 = (float *)(((int)sVar1 & 0x7fffU) * 0x20 + *(int *)(DAT_00746f9c + 0x17c) + 4);
      }
      else {
        pfVar4 = (float *)0x0;
      }
      uVar3 = FUN_0053ed10();
      fVar5 = extraout_ST0;
      if ((uVar3 != 0xffffffff) &&
         (pbVar2 = *(byte **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), (*pbVar2 & 1) != 0)) {
        if (pfVar4 != (float *)0x0) {
          return -((((float10)*unaff_EDI * (float10)*pfVar4 +
                    (float10)pfVar4[1] * (float10)unaff_EDI[1] +
                    (float10)pfVar4[2] * (float10)unaff_EDI[2]) - (float10)pfVar4[3]) +
                  (float10)*(float *)(pbVar2 + 0x74));
        }
        fVar5 = (float10)3.4028235e+38;
      }
    }
  }
  return fVar5;
}
#endif
