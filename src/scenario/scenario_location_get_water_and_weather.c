// scenario_location_get_water_and_weather  (Ghidra: FUN_0053ed60, still unnamed there; named for
// this rewrite -- it answers "is this location in water, and what weather applies here",
// building on scenario_location_fog_region and scenario_fog_region_resolve_tag.)
// address 0x53ed60, size 147 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/scenario_types_notes.md's register table ("0x53ed60: stack arguments
//   (bsp_leaf_reference *, int16 *weather_index_out), with EBX = point passed through to
//   0x53ec30. Returns is_water.") and its tag-offset table (ScenarioStructureBSPFogRegion
//   .weather_palette +0x26, ScenarioStructureBSPCluster.weather +0x08, Fog.flags bit 0
//   is_water). Confirmed instruction by instruction against objdump -d -M intel
//   --start-address=0x53ed60 --stop-address=0x53ee00 bin/halo.exe: EBX is live on entry and
//   forwarded to the scenario_location_fog_region call unmodified, and the two stack slots the
//   Ghidra pseudo-code elides (the leading "is_water default" byte, and the "weather default -1"
//   dword) are provably always 0 and -1 respectively -- they are stack-reservation locals, not
//   meaningful state, so the is_water default and weather default below reproduce them as plain
//   initializers instead of the stack-slot indirection.
// register convention: EBX -> point (real_point3d *, forwarded as-is); stack: leaf
//   (bsp_leaf_reference *), then weather_index_out (int16_t *, may be NULL).
// UNSURE: the original leaves the upper 24 bits of EAX as whatever the tag-index shift left
//   behind on the success path (`(tag_index & 0xffff) << 5` with only the low byte replaced by
//   the masked flags byte -- see structure_bsp_load's identical precedent in
//   src/cache/structure_bsp_load.c). The result is therefore typed uint8_t (bool in AL); a
//   caller that tests more than AL would see garbage in the original.
//   // blam-cc: EBX -> point, stack -> leaf, weather_index_out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "scenario.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern tag_instance *tag_instances;                // 0x0087bc14

// blam-cc: EAX -> leaf, EBX -> point
extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf,
    real_point3d *point); // this module, 0x53ec30
// blam-cc: AX -> fog_region
extern uint32_t scenario_fog_region_resolve_tag(int16_t fog_region); // this module, 0x53ed10

// blam-cc: EBX -> point, stack -> leaf, weather_index_out
// Resolves whether a location is in water, and which weather palette entry applies there. The
// weather comes from the location's fog region (if it has one with a weather_palette assigned),
// falling back to the location's own cluster's weather field otherwise.
uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out)
{
    int16_t fog_region;
    int16_t weather;
    uint8_t is_water;

    weather = -1;
    fog_region = scenario_location_fog_region(leaf, point);
    if (fog_region == -1) {
        is_water = 0;
    } else {
        ScenarioStructureBSPFogRegion *region =
            &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)
                [fog_region];
        uint32_t fog_tag = scenario_fog_region_resolve_tag(fog_region);

        if (fog_tag == 0xffffffff) {
            is_water = 0;
        } else {
            Fog *fog_data = (Fog *)tag_instances[fog_tag & 0xffff].data;
            is_water = (uint8_t)(fog_data->flags & k_fog_flag_is_water);
        }

        weather = (int16_t)region->weather_palette;
        if (weather != -1) {
            goto write_output;
        }
    }

    if (leaf->cluster_index != -1) {
        ScenarioStructureBSPCluster *cluster =
            &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)
                [leaf->cluster_index];
        weather = (int16_t)cluster->weather;
    }

write_output:
    if (weather_index_out != 0) {
        *weather_index_out = weather;
    }
    return is_water;
}

#if 0
Original Ghidra decompilation (0x53ed60):

uint FUN_0053ed60(int param_1,short *param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  short sVar6;

  iVar2 = DAT_00746f9c;
  uVar3 = FUN_0053ec30();
  sVar6 = -1;
  if ((short)uVar3 == -1) {
    uVar4 = uVar3 & 0xffffff00;
  }
  else {
    iVar1 = *(int *)(iVar2 + 0x188);
    uVar4 = FUN_0053ed10();
    if (uVar4 == 0xffffffff) {
      uVar4 = 0xffffff00;
    }
    else {
      iVar5 = (uVar4 & 0xffff) * 0x20;
      uVar4 = CONCAT31((int3)((uint)iVar5 >> 8),**(undefined1 **)(iVar5 + 0x14 + DAT_0087bc14)) &
              0xffffff01;
    }
    sVar6 = *(short *)(iVar1 + (short)uVar3 * 0x28 + 0x26);
    if (sVar6 != -1) goto LAB_0053ede4;
  }
  if (*(short *)(param_1 + 4) != -1) {
    sVar6 = *(short *)(*(short *)(param_1 + 4) * 0x68 + 8 + *(int *)(iVar2 + 0x138));
  }
LAB_0053ede4:
  if (param_2 != (short *)0x0) {
    *param_2 = sVar6;
  }
  return uVar4;
}
#endif
