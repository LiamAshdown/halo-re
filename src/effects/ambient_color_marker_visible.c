// ambient_color_marker_visible  (Ghidra: FUN_0053f860, still unnamed there; named from its own
//   summary in out/phase4/effects_functions.md: "Determines whether an effect marker's
//   containing cluster matches a requested sky/indoor visibility filter, computing the marker's
//   ambient color as a side effect via FUN_0053f940")
// address 0x53f860, size 219 bytes
// name confidence: 0.4   rewrite confidence: 0.15 (VERY LOW -- see UNSURE)
// evidence: out/phase4/effects_types_notes.md's own description of this address; this module's
//   ambient_color_for_marker.c (the unconditional tail call, forwarding `position` and `flags`
//   straight through).
// register convention: a location pointer (leaf_index +0x00, cluster_index +0x04 -- a
//   bsp_leaf_reference) in EAX (in_EAX); position and filter-flags are Ghidra's own recognized
//   stack parameters (param_1, param_2), forwarded unread to ambient_color_for_marker; a third
//   stack parameter carries the sky/indoor filter bits actually tested here (param_3).
//   // blam-cc: in_EAX -> location, stack -> (position, out, filter_flags)
// UNSURE (heavily): scenario_location_fog_region (cluster -> weather-row-shaped index, outside this batch) and
//   the two nested lookups at `structure_bsp+0x188` (stride 0x28) and `structure_bsp+0x194`
//   (stride 0x88, +0x2c a tag id) are BSP/scenario runtime structures this module has no types
//   for; kept as raw offsets. The final tag byte test (`tag_data[0] & 1`) is assumed to be a Sky
//   tag's indoor/outdoor flag by analogy with the function's own summary, but no header confirms
//   the tag group or field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern uint8_t *structure_bsp_globals; // 0x00746f9c
extern tag_instance *tag_instances;    // 0x0087bc14

extern int16_t scenario_location_fog_region(int16_t cluster_index); // 0x53ec30, UNSURE signature
                                    // (structures/BSP module)
extern void ambient_color_for_marker(int16_t weather_row, real_point3d *position, uint8_t flags,
    real_vector3d *out); // 0x53f940, this module; UNSURE: weather_row argument is not visibly
                                    // threaded through this function's own body, see file header

// Tests whether the cluster a location falls in matches a requested sky (bit 3) / indoor (bit 2)
// visibility filter, then always computes that location's ambient colour via
// ambient_color_for_marker as a side effect.
uint8_t ambient_color_marker_visible(bsp_leaf_reference *location, real_point3d *position,
    real_vector3d *out, uint32_t filter_flags)
{
    uint8_t result = 0;

    if (location->cluster_index != -1) {
        int16_t row = scenario_location_fog_region(location->cluster_index); // UNSURE, see file header

        if (row != -1) {
            uint8_t *cluster_row = *(uint8_t **)(structure_bsp_globals + 0x188) + row * 0x28;
            int16_t a = *(int16_t *)(cluster_row + 0x24);
            int16_t b = *(int16_t *)(cluster_row + 0x26);

            if (a != -1 && b != -1) {
                uint32_t tag_id = *(uint32_t *)(*(uint8_t **)(structure_bsp_globals + 0x194) +
                    (uint32_t)a * 0x88 + 0x2c);

                if (tag_id != 0xffffffff) {
                    uint8_t sky_flag = *(uint8_t *)tag_instances[(uint16_t)tag_id].data;

                    if ((sky_flag & 1) == 0) {
                        if ((filter_flags & 4) == 0) {
                            result = 0;
                        }
                    } else if ((filter_flags & 8) == 0) {
                        result = 1;
                    }
                }
            }
        }
    }

    ambient_color_for_marker(0, position, (uint8_t)filter_flags, out); // UNSURE: weather_row
                                    // argument, see file header
    return result;
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
