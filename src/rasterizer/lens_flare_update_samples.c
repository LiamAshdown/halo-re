// lens_flare_update_samples  (Ghidra: FUN_00513ba0, unnamed; named per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513ba0, size 334 bytes
// name confidence: 0.45  rewrite confidence: 0.85
// REWRITTEN from objdump 0x513ba0..0x513ced. For every active lens flare instance of the current render window
//   (window_flags & 0x7f == window +0x02) the occlusion sample point is built in a STACK LOCAL (the draft moved the
//   instance itself every frame):
//     occlusion_offset_direction 0 (0x513c9e): point = position - occlusion_radius * camera forward (0x007c1234)
//     1 (0x513c87): point = position + occlusion_radius * sqrt2 (0x672ef4) * the instance's unpacked normal
//     2 (0x513c6e): point = position
//     other: the local is left as it was (uninitialised in the original).
//   The instance's sample count (+0x24, which lens_flare_render_all requires > 0) is the return value of
//   0x512190(ECX = &point, stack = radius) with EDI = the instance index still live from the loop (0x513cd6), which
//   forwards to rasterizer_lens_flare_occlusion_test_issue(EDI slot, point, radius). The draft passed only the
//   radius (so the query read a float as the point) and the wrapper returned nothing: every flare got a garbage
//   sample count and was either skipped or tested at a garbage point.
//   Gate as in the binary: 0x6893ff set, 0x719aac <= 1 (== 1 needs 0x696568 <= 1), window type 1, count > 0;
//   batching mode 6 (ECX = 1) before, 0x512150 after.
// blam-cc: (none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include "fn_render.h"

extern uint8_t unknown_006893ff;    // 0x006893ff UNSURE: console/debug toggle, owner module unclear
extern int16_t unknown_00719aac;    // 0x00719aac UNSURE: gating value, owner module unclear
extern int16_t screenshot_scale;    // 0x00696568 UNSURE: gating value, owner module unclear
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count;   // 0x0071d134


extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale); // 0x401930, EAX out, ECX direction, stack base, scale
// blam-cc: AX -> mode, ECX -> flags


void lens_flare_update_samples(void)
{
    int16_t i;
    real_vector3d unpack_scratch;
    real_vector3d normal;
    real_point3d sample_point = { 0.0f, 0.0f, 0.0f };
    LensFlare *definition;
    float radius;

    if (unknown_006893ff == 0 || unknown_00719aac > 1 ||
        (unknown_00719aac == 1 && screenshot_scale > 1) ||
        rasterizer_window.type != 1 || lens_flare_instance_count <= 0) {
        return;
    }

    rasterizer_lens_flare_batching_select_mode(6, 1); // 0x513bee: AX 6, ECX still 1

    for (i = 0; i < lens_flare_instance_count; i++) {
        lens_flare_instance *instance = &lens_flare_instances[i];

        definition = (LensFlare *)instance->definition;
        normal = *vector3d_unpack_normal_11_11_10(&unpack_scratch, instance->packed_direction);
        if ((int16_t)(instance->window_flags & 0x7f) != rasterizer_window.window_index) {
            continue;
        }
        radius = definition->occlusion_radius;
        switch (definition->occlusion_offset_direction) {
        case 0:
            point3d_add_scaled(&sample_point, (real_vector3d *)&rasterizer_window.camera.forward, &instance->position,
                               -radius);
            break;
        case 1:
            point3d_add_scaled(&sample_point, &normal, &instance->position, radius * 1.4142135f);
            break;
        case 2:
            sample_point = instance->position;
            break;
        }
        instance->sample_count = render_rasterizer_dispatch_537800(i, &sample_point, radius);
    }

    rasterizer_effect_slot_release_active();
}

#if 0
Original Ghidra decompilation (0x513ba0):

void FUN_00513ba0(void)

{
  int iVar1;
  undefined4 uVar2;
  short sVar3;
  int iVar4;
  float fVar5;

  if ((((DAT_006893ff != '\0') && (DAT_00719aac < 2)) &&
      ((DAT_00719aac != 1 || ((short)DAT_00696568 < 2)))) &&
     (((short)DAT_007c1220 == 1 && (0 < DAT_0071d134)))) {
    FUN_00537130();
    sVar3 = 0;
    if (0 < DAT_0071d134) {
      iVar4 = 0;
      do {
        iVar1 = (&DAT_006ce818)[iVar4 * 10];
        vector3d_unpack_normal_11_11_10();
        if (((byte)(&DAT_006ce83a)[iVar4 * 0x28] & 0xff7f) == DAT_007c1220._2_2_) {
          uVar2 = *(undefined4 *)(iVar1 + 0x10);
          if (*(short *)(iVar1 + 0x14) == 0) {
            fVar5 = -*(float *)(iVar1 + 0x10);
LAB_00513cb0:
            point3d_add_scaled(&DAT_006ce81c + iVar4 * 10,fVar5);
          }
          else if (*(short *)(iVar1 + 0x14) == 1) {
            fVar5 = *(float *)(iVar1 + 0x10) * 1.4142135;
            goto LAB_00513cb0;
          }
          uVar2 = FUN_00512190(uVar2);
          (&DAT_006ce83c)[iVar4 * 10] = uVar2;
        }
        sVar3 = sVar3 + 1;
        iVar4 = (int)sVar3;
      } while (iVar4 < DAT_0071d134);
    }
    FUN_00512150();
  }
  return;
}
#endif
