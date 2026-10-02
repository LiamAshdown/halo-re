// lens_flare_update_visibility  (Ghidra: decal_shadow_value_update, misnamed; renamed per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513780, size 277 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED against objdump 0x513780..0x513894: per instance the visibility byte (BSP marker table 0x6be810 / object table 0x6bc512) is 0 without samples, else round(result*255/samples) capped at 255, 0 clears, rising blends (3*old+new)/4, falling (old+new)/2; the count is reset)
// evidence: walks lens_flare_instances[0..lens_flare_instance_count) (types/rasterizer.h),
//   re-deriving the same visibility-byte pointer as lens_flare_get_visibility_byte.c inline
//   (the array strides in the original -- ushort index*0x14, byte index*0x28, int32 index*10 --
//   all resolve to lens_flare_instance's 0x28 byte stride), blends each toward a freshly
//   sampled occlusion percentage, and resets the instance count for the next frame.
// register convention: none -- all state is global.
// UNSURE: rasterizer_lens_flare_occlusion_query_get_result is called with no visible argument;
//   the only value that makes semantic sense per query slot is the loop index, so it is declared
//   here taking one -- Ghidra lost the register that carries it (most likely EAX, still holding
//   the loop index from the pointer arithmetic just above the call).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t unknown_006893ff;    // 0x006893ff UNSURE: console/debug toggle, owner module unclear
extern int16_t unknown_00719aac;    // 0x00719aac UNSURE: gating value, owner module unclear
extern int16_t screenshot_scale;    // 0x00696568 UNSURE: gating value, owner module unclear
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count; // 0x0071d134

extern uint8_t *lens_flare_get_visibility_byte(lens_flare_instance *flare); // 0x5134f0
extern int32_t rasterizer_lens_flare_occlusion_query_get_result(int32_t slot_index); // 0x537b40, ESI slot = the loop index (0x5137c0)

// Per-frame smoothing pass: while occlusion queries are enabled (unknown_006893ff) and the
// window/mode gate allows it, blends each active lens flare's visibility byte toward its
// freshly sampled occlusion percentage (0 when it has no samples), then clears the active count.
void lens_flare_update_visibility(void)
{
    int32_t i;
    uint8_t *slot;
    int32_t percent;
    uint8_t old_value;
    uint8_t new_value;

    if (unknown_006893ff == 0 || unknown_00719aac >= 2 ||
        (unknown_00719aac == 1 && screenshot_scale >= 2)) {
        return;
    }

    for (i = 0; i < lens_flare_instance_count; i++) {
        slot = lens_flare_get_visibility_byte(&lens_flare_instances[i]);

        if (lens_flare_instances[i].sample_count < 1) {
            *slot = 0;
            continue;
        }

        percent = (rasterizer_lens_flare_occlusion_query_get_result(i) * 0xff +
                   (lens_flare_instances[i].sample_count >> 1)) /
                  lens_flare_instances[i].sample_count;
        if (percent < 0xff) {
            new_value = (uint8_t)percent;
            if (new_value == 0) {
                *slot = 0;
                continue;
            }
        } else {
            new_value = 0xff;
        }

        old_value = *slot;
        if (old_value < new_value) {
            new_value = (uint8_t)(((uint32_t)old_value * 3 + (uint32_t)new_value) >> 2);
        } else if (old_value > new_value) {
            new_value = (uint8_t)(((uint32_t)old_value + (uint32_t)new_value) / 2);
        } else {
            continue;
        }
        *slot = new_value;
    }

    lens_flare_instance_count = 0;
}

#if 0
Original Ghidra decompilation (0x513780):

void decal_shadow_value_update(void)

{
  byte bVar1;
  ushort uVar2;
  int iVar3;
  byte bVar4;
  byte *pbVar5;
  short sVar6;
  int iVar7;

  if (((DAT_006893ff != '\0') && (DAT_00719aac < 2)) &&
     ((DAT_00719aac != 1 || ((short)DAT_00696568 < 2)))) {
    sVar6 = 0;
    if (0 < DAT_0071d134) {
      iVar7 = 0;
      do {
        uVar2 = (&DAT_006ce836)[iVar7 * 0x14];
        if ((short)uVar2 < 0) {
          pbVar5 = (byte *)((int)&DAT_006be810 +
                           ((byte)(&DAT_006ce83a)[iVar7 * 0x28] & 0xffffff7f) +
                           ((uVar2 & 0x7fff) << 0x10 | (int)(short)(&DAT_006ce838)[iVar7 * 0x14]));
        }
        else {
          pbVar5 = (byte *)(((byte)(&DAT_006ce83a)[iVar7 * 0x28] & 0xffffff7f) + (short)uVar2 * 10 +
                            0x6bc512 + (int)(short)(&DAT_006ce838)[iVar7 * 0x14]);
        }
        if ((int)(&DAT_006ce83c)[iVar7 * 10] < 1) {
LAB_00513840:
          *pbVar5 = 0;
        }
        else {
          iVar3 = rasterizer_lens_flare_occlusion_query_get_result();
          iVar7 = (iVar3 * 0xff + ((int)(&DAT_006ce83c)[iVar7 * 10] >> 1)) /
                  (int)(&DAT_006ce83c)[iVar7 * 10];
          if (iVar7 < 0xff) {
            bVar4 = (byte)iVar7;
            if (bVar4 == 0) goto LAB_00513840;
          }
          else {
            bVar4 = 0xff;
          }
          bVar1 = *pbVar5;
          if (bVar1 < bVar4) {
            bVar4 = (byte)((int)((uint)bVar1 * 3 + (uint)bVar4) >> 2);
          }
          else {
            if (bVar1 <= bVar4) goto LAB_00513875;
            bVar4 = (byte)(((uint)bVar1 + (uint)bVar4) / 2);
          }
          *pbVar5 = bVar4;
        }
LAB_00513875:
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < DAT_0071d134);
    }
    DAT_0071d134 = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
