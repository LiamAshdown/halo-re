// lens_flare_add_instance  (Ghidra: decal_add_to_active_list, misnamed; renamed per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x5138a0, size 342 bytes
// name confidence: 0.55  rewrite confidence: 0.5
// evidence: the candidate record read through unaff_EBX has exactly lens_flare_instance's
//   layout (definition/far_fade_distance culled at +0x00/+0x1c via LensFlare tag data, position
//   at +0x04, color/alpha at +0x18, object_index at +0x1c); it is bulk-copied (10 dwords) into
//   lens_flare_instances[lens_flare_instance_count] and this function then resolves the new
//   instance's visibility_high/visibility_low (BSP marker case) or the matching
//   lens_flare_object_visibility slot (object case), matching the type header's field notes for
//   both.
// register convention: candidate record in unaff_EBX (unresolved register read).
//   // blam-cc: unaff_EBX -> candidate

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern uint8_t unknown_006893ff;    // 0x006893ff UNSURE: console/debug toggle, owner module unclear
extern int16_t unknown_00719aac;    // 0x00719aac UNSURE: gating value, owner module unclear
extern int16_t screenshot_scale;    // 0x00696568 UNSURE: gating value, owner module unclear
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count;   // 0x0071d134
extern uint8_t lens_flare_instance_overflow; // 0x0071d138
extern lens_flare_object_visibility lens_flare_object_visibility_table[k_lens_flare_object_visibility_slots]; // 0x006bc510

// blam-cc: unaff_EBX -> candidate
// Culls a candidate lens flare (by view-space depth against its LensFlare.far_fade_distance,
// and by a zero alpha) and, if visible and the active list has room, appends it to
// lens_flare_instances and resolves its per window visibility byte: for a BSP marker flare
// (candidate->object_index == -1) it rewrites visibility_high/visibility_low from the source
// marker offset; for an object flare it resets the matching lens_flare_object_visibility slot
// whenever that slot no longer belongs to this object.
void lens_flare_add_instance(lens_flare_instance *candidate)
{
    int32_t new_index;
    float depth;
    LensFlare *definition;

    new_index = lens_flare_instance_count;

    if (unknown_006893ff == 0 || unknown_00719aac >= 2 ||
        (unknown_00719aac == 1 && screenshot_scale >= 2) || rasterizer_window.type != 1) {
        return;
    }

    if (lens_flare_instance_count >= 0x400) {
        if (lens_flare_instance_overflow == 0) {
            lens_flare_instance_overflow = 1;
        }
        return;
    }

    definition = (LensFlare *)candidate->definition;
    depth = rasterizer_window.camera.forward.i * (candidate->position.x - rasterizer_window.camera.position.x) +
            rasterizer_window.camera.forward.j * (candidate->position.y - rasterizer_window.camera.position.y) +
            rasterizer_window.camera.forward.k * (candidate->position.z - rasterizer_window.camera.position.z);

    if ((definition->far_fade_distance != 0.0f && depth >= definition->far_fade_distance) ||
        (candidate->color & 0xff000000) == 0) {
        return;
    }

    lens_flare_instances[new_index] = *candidate;
    lens_flare_instance_count = new_index + 1;

    if (candidate->object_index == -1) {
        uint16_t marker_visibility_high = (uint16_t)candidate->visibility_high;

        if (marker_visibility_high != 0xffff) {
            int16_t marker_offset = (int16_t)candidate->visibility_low; // unaff_EBX[8] low half
            lens_flare_instances[new_index].visibility_low = marker_offset + 8;
            lens_flare_instances[new_index].visibility_high =
                (int16_t)(marker_visibility_high | (uint16_t)(marker_offset >> 0xf) | 0x8000);
            return;
        }
        lens_flare_instances[new_index].visibility_high = (int16_t)0x8000;
    } else {
        if (candidate->object_index != lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].object_index) {
            int16_t object_index = lens_flare_instances[new_index].object_index;
            int i;

            for (i = 0; i < 8; i++) {
                lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].visibility[i] = 0;
            }
            lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].object_index = object_index;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5138a0):

void decal_add_to_active_list(void)

{
  int iVar1;
  ushort uVar2;
  undefined2 uVar3;
  int iVar4;
  int iVar5;
  int *unaff_EBX;
  int *piVar6;
  int *piVar7;

  iVar4 = DAT_0071d134;
  if ((((DAT_006893ff != '\0') && (DAT_00719aac < 2)) &&
      ((DAT_00719aac != 1 || ((short)DAT_00696568 < 2)))) && ((short)DAT_007c1220 == 1)) {
    if (DAT_0071d134 < 0x400) {
      if (((*(float *)(*unaff_EBX + 0x1c) == 0.0) ||
          (DAT_007c1234 * ((float)unaff_EBX[1] - DAT_007c1228) +
           DAT_007c1238 * ((float)unaff_EBX[2] - DAT_007c122c) +
           DAT_007c123c * ((float)unaff_EBX[3] - DAT_007c1230) < *(float *)(*unaff_EBX + 0x1c))) &&
         ((unaff_EBX[6] & 0xff000000U) != 0)) {
        iVar1 = DAT_0071d134 * 0x28;
        piVar6 = unaff_EBX;
        piVar7 = &DAT_006ce818 + DAT_0071d134 * 10;
        DAT_0071d134 = DAT_0071d134 + 1;
        for (iVar5 = 10; iVar5 != 0; iVar5 = iVar5 + -1) {
          *piVar7 = *piVar6;
          piVar6 = piVar6 + 1;
          piVar7 = piVar7 + 1;
        }
        if ((short)unaff_EBX[7] == -1) {
          uVar2 = *(ushort *)((int)unaff_EBX + 0x1e);
          if (uVar2 != 0xffff) {
            iVar1 = unaff_EBX[8];
            (&DAT_006ce838)[iVar4 * 0x14] = (short)iVar1 + 8;
            (&DAT_006ce836)[iVar4 * 0x14] = uVar2 | (short)iVar1 >> 0xf | 0x8000;
            return;
          }
          (&DAT_006ce836)[iVar4 * 0x14] = 0x8000;
        }
        else {
          iVar5 = (short)(&DAT_006ce836)[iVar4 * 0x14] * 10;
          if ((short)unaff_EBX[7] !=
              *(short *)((int)&DAT_006bc510 + (short)(&DAT_006ce836)[iVar4 * 0x14] * 10)) {
            uVar3 = *(undefined2 *)(&DAT_006ce834 + iVar1);
            *(undefined4 *)((int)&DAT_006bc510 + iVar5 + 2) = 0;
            *(undefined4 *)((int)&DAT_006bc514 + iVar5 + 2) = 0;
            *(undefined2 *)((int)&DAT_006bc510 + iVar5) = uVar3;
            return;
          }
        }
      }
    }
    else if (DAT_0071d138 == '\0') {
      DAT_0071d138 = 1;
      return;
    }
  }
  return;
}
#endif
