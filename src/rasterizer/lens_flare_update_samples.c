// lens_flare_update_samples  (Ghidra: FUN_00513ba0, unnamed; named per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513ba0, size 334 bytes
// name confidence: 0.45  rewrite confidence: 0.4
// evidence: for every active lens_flare_instance whose window_flags (masked to the low 7 bits,
//   the window index -- types/rasterizer.h's lens_flare_instance_window_flags) matches the
//   current render window, offsets its position along its unpacked direction by
//   +-LensFlare.occlusion_radius (0 = behind, 1 = diagonally by radius*sqrt2, matching
//   LensFlareOcclusionOffsetDirection_t) via point3d_add_scaled, and derives a sample count from
//   the radius into lens_flare_instance.sample_count -- matches the module note "offsets its
//   position along its normal and computes a per-decal shadow sample count/value".
// register convention: none recognized -- purely global driven, apart from the register
//   forwarding noted below. // blam-cc: (none)
// UNSURE: point3d_add_scaled's EAX (out) and ECX (direction) are not shown at this call site,
//   only the stack args (base, scale). Reconstructed here as out == base (in place update) and
//   direction == the vector just unpacked by vector3d_unpack_normal_11_11_10 on the line above,
//   which is well supported by the surrounding code (there is no other direction vector in
//   scope) but the exact registers were not independently confirmed against a disassembly.
//   FUN_00512150/FUN_00512190/FUN_00537130 are outside this session's address range; their
//   names and FUN_00512190's argument/return types are guessed from this call site only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t unknown_006893ff;    // 0x006893ff UNSURE: console/debug toggle, owner module unclear
extern int16_t unknown_00719aac;    // 0x00719aac UNSURE: gating value, owner module unclear
extern int16_t unknown_00696568;    // 0x00696568 UNSURE: gating value, owner module unclear
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count;   // 0x0071d134

extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale); // 0x401930
// blam-cc: AX -> mode, ECX -> flags
extern void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags); // 0x537130
extern int32_t render_rasterizer_dispatch_537800(float radius); // 0x512190, UNSURE name/signature
extern void rasterizer_effect_slot_release_active(void); // 0x512150, UNSURE name/signature

// Recomputes, for every active lens flare belonging to the current render window, the occluder
// sample point (offset from its true position along its unpacked direction, per
// LensFlare.occlusion_offset_direction) and the number of occlusion samples its radius is worth.
void lens_flare_update_samples(void)
{
    int32_t i;
    real_vector3d unpack_scratch;
    real_vector3d *direction;
    LensFlare *definition;
    float radius;
    float offset;
    int16_t window_index;

    if (unknown_006893ff == 0 || unknown_00719aac >= 2 ||
        (unknown_00719aac == 1 && unknown_00696568 >= 2) ||
        rasterizer_window.type != 1 || lens_flare_instance_count <= 0) {
        return;
    }

    rasterizer_lens_flare_batching_select_mode(6, 1); // 0x513bee: AX 6, ECX still 1

    window_index = rasterizer_window.window_index;

    for (i = 0; i < lens_flare_instance_count; i++) {
        definition = (LensFlare *)lens_flare_instances[i].definition;
        direction = vector3d_unpack_normal_11_11_10(&unpack_scratch, lens_flare_instances[i].packed_direction);

        if ((lens_flare_instances[i].window_flags & 0x7f) == window_index) {
            radius = definition->occlusion_radius;

            if (definition->occlusion_offset_direction == 0) {
                offset = -radius;
                point3d_add_scaled(&lens_flare_instances[i].position, direction,
                                    &lens_flare_instances[i].position, offset);
            } else if (definition->occlusion_offset_direction == 1) {
                offset = radius * 1.4142135f;
                point3d_add_scaled(&lens_flare_instances[i].position, direction,
                                    &lens_flare_instances[i].position, offset);
            }

            lens_flare_instances[i].sample_count = render_rasterizer_dispatch_537800(radius);
        }
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
