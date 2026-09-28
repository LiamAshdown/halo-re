// ambient_color_marker_visible  (Ghidra name kept; really the water/weather probe at a location)
// address 0x53f860, size 219 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN (objdump 0x53f860..0x53f93a; the draft handed the cluster index to the fog lookup, which takes the
//   location pointer and a point). EAX = the location, stack: position, out (the wind/current), filter flags.
//   With a cluster (+0x04 != -1): the fog region (scenario_location_fog_region, EAX location, EBX = position, or
//   NULL with filter bit 2) picks the weather row -- by default the cluster's own (structure bsp +0x138, 0x68 each,
//   +0x08). A region (bsp +0x188, 0x28 each) with both palette words (+0x24 fog, +0x26 weather) set whose fog
//   (bsp +0x194, 0x88 each, tag +0x2c) exists uses its weather row: for a fog tag with flag bit 0 (water) unless
//   filter bit 3 is set, and then reports "in water" (1); for other fogs unless filter bit 2 is set.
//   ambient_color_for_marker(AX = the row, stack: position, filter, EDI = out) fills the output.
// blam-cc: EAX -> location, stack -> position, out, filter_flags

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern ScenarioStructureBSP *global_structure_bsp;
extern tag_instance *tag_instances;    // 0x0087bc14

extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point); // 0x53ec30, EAX, EBX
extern void ambient_color_for_marker(int16_t weather_row, real_point3d *position, uint8_t flags,
    real_vector3d *out); // 0x53f940, blam-cc: AX, stack, stack, EDI

uint8_t ambient_color_marker_visible(bsp_leaf_reference *location, real_point3d *position,
    real_vector3d *out, uint32_t filter_flags)
{
    uint8_t in_water = 0;
    int16_t weather_row = -1;
    int16_t cluster = *(int16_t *)((uint8_t *)location + 4);

    if (cluster != -1) {
        uint32_t skip_non_water = filter_flags & 4;
        int16_t region = scenario_location_fog_region(location, skip_non_water ? (real_point3d *)0 : position);

        weather_row = *(int16_t *)((uint8_t *)global_structure_bsp->clusters.pointer + cluster * 0x68 + 8);
        if (region != -1) {
            uint8_t *fog_region = (uint8_t *)global_structure_bsp->fog_regions.pointer + region * 0x28;
            int16_t fog = *(int16_t *)(fog_region + 0x24);
            int16_t region_weather = *(int16_t *)(fog_region + 0x26);

            if (fog != -1 && region_weather != -1) {
                datum_index fog_tag = *(datum_index *)((uint8_t *)global_structure_bsp->fog_palette.pointer + fog * 0x88 + 0x2c);

                if (fog_tag != k_datum_index_none) {
                    uint8_t *fog_data = (uint8_t *)tag_instances[fog_tag & 0xffff].data;

                    if (fog_data[0] & 1) {
                        if ((filter_flags & 8) == 0) {
                            in_water = 1;
                            weather_row = region_weather;
                        }
                    } else if (!skip_non_water) {
                        weather_row = region_weather;
                    }
                }
            }
        }
    }
    ambient_color_for_marker(weather_row, position, (uint8_t)filter_flags, out);
    return in_water;
}

#if 0
Original Ghidra decompilation (0x53f860):

undefined1 FUN_0053f860(undefined4 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  undefined1 uVar4;
  undefined1 uVar5;
  short sVar6;
  int in_EAX;
  undefined1 local_1;

  iVar3 = DAT_00746f9c;
  local_1 = 0;
  uVar5 = local_1;
  local_1 = 0;
  uVar4 = local_1;
  local_1 = 0;
  if (*(short *)(in_EAX + 4) != -1) {
    sVar6 = FUN_0053ec30();
    local_1 = uVar4;
    if (sVar6 != -1) {
      iVar1 = *(int *)(iVar3 + 0x188) + sVar6 * 0x28;
      sVar6 = *(short *)(iVar1 + 0x24);
      if (((sVar6 != -1) && (*(short *)(iVar1 + 0x26) != -1)) &&
         (uVar2 = *(uint *)(sVar6 * 0x88 + *(int *)(iVar3 + 0x194) + 0x2c), uVar2 != 0xffffffff)) {
        if ((**(byte **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 1) == 0) {
          if ((param_3 & 4) != 0) goto LAB_0053f91c;
        }
        else {
          if ((param_3 & 8) != 0) goto LAB_0053f91c;
          local_1 = 1;
          uVar5 = local_1;
        }
        local_1 = uVar5;
      }
    }
  }
LAB_0053f91c:
  FUN_0053f940(param_1,param_3);
  return local_1;
}
#endif
