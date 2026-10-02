// contrail_compute_edge_fade_factor  (Ghidra: contrail_compute_edge_fade_factor, already named)
// address 0x50e000, size 133 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/render_functions.md's phase-2 summary: "Computes a view-angle-dependent
//   fade/alpha factor for a contrail segment, used to soften the geometry seen edge-on or
//   head-on." Disassembly (objdump -d -M intel, 0x50e000..0x50e084) confirms EAX=direction,
//   ECX=point position, stack=flags (a single stack argument, loaded from [esp+8] right after the
//   function's own "push ecx" frame reservation), and SI (fade_mode, 0/1/2) is a genuine hidden
//   register argument the caller (contrail_geometry_build_segment, 0x50e090) leaves alive rather
//   than something this function computes; fade_mode 0 short-circuits to a constant 1.0.
// review note (phase-4 gate): SI is the shader framebuffer_fade_mode (render_contrail 0x50e090
//   loads it from Contrail +0xb0 with mov si,[edx+0x2c]), 1 fade when perpendicular, 2 fade when
//   parallel; flags is the Contrail flags word, bit 6 edge_effect_fades_slowly.
// register convention: EAX = direction (real_vector3d*), ECX = point (real_point3d*),
//   ESI = fade_mode (int16_t, low 16 bits of unaff_SI), stack = flags (uint8_t*).
//   // blam-cc: EAX=direction, ECX=point, ESI=fade_mode, stack=flags
// UNSURE: flags+0x40's owner/meaning is not documented anywhere in types/effects.h's
//   contrail_point_flags (which only defines 0x01/0x02/0x04); it may belong to a per-segment flag
//   byte the caller builds on its own stack rather than to contrail_point.flags itself.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real transition_function_evaluate(transition_function_t type, real phase); // 0x4ccac0,
                                                                                  // math module
extern double sqrt(double x); // x87 FSQRT
extern double fabs(double x); // x87 FABS

extern render_camera render_camera_global; // 0x007c3114, this module (read, not owned)

// Computes a 0..1 fade factor for one contrail segment based on the angle between the camera-to-
// point direction and the segment's own axis (direction): a segment seen edge-on (axis
// perpendicular to the view) fades to 0, one seen face-on fades to 1. fade_mode 0 disables the
// fade entirely (returns 1.0); fade_mode 2 inverts the curve.
real contrail_compute_edge_fade_factor(real_vector3d *direction, real_point3d *point,
                                        int16_t fade_mode,
                                        uint8_t *flags) // blam-cc: EAX=direction, ECX=point, ESI=fade_mode, stack=flags
{
    real_vector3d to_camera;
    float fade;

    if (fade_mode == 0) {
        return 1.0f;
    }

    to_camera.i = render_camera_global.position.x - point->x;
    to_camera.j = render_camera_global.position.y - point->y;
    to_camera.k = render_camera_global.position.z - point->z;

    fade = (float)fabs((double)((to_camera.i * direction->i + to_camera.k * direction->k +
                                  to_camera.j * direction->j) /
                                 sqrt((double)(to_camera.k * to_camera.k + to_camera.j * to_camera.j +
                                               to_camera.i * to_camera.i))));

    if ((*flags & 0x40) != 0) {
        fade = transition_function_evaluate(_transition_function_very_early, fade);
    }
    if (fade_mode == 2) {
        fade = 1.0f - fade;
    }
    return fade;
}

#if 0
Original Ghidra decompilation (0x50e000):

float10 contrail_compute_edge_fade_factor(byte *param_1)

{
  float *in_EAX;
  float *in_ECX;
  short unaff_SI;
  float10 fVar1;
  float10 fVar2;
  float10 fVar3;

  fVar1 = (float10)1.0;
  if (unaff_SI != 0) {
    fVar1 = (float10)DAT_007c3114 - (float10)*in_ECX;
    fVar2 = (float10)DAT_007c3118 - (float10)in_ECX[1];
    fVar3 = (float10)DAT_007c311c - (float10)in_ECX[2];
    fVar1 = ABS((fVar1 * (float10)*in_EAX + fVar3 * (float10)in_EAX[2] + fVar2 * (float10)in_EAX[1])
                / SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1));
    if ((*param_1 & 0x40) != 0) {
      fVar1 = (float10)transition_function_evaluate((float)fVar1);
    }
    if (unaff_SI == 2) {
      fVar1 = (float10)1.0 - fVar1;
    }
  }
  return fVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
