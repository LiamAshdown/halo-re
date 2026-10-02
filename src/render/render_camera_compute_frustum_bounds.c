// render_camera_compute_frustum_bounds  (Ghidra: render_camera_compute_frustum_bounds, already
// named)
// address 0x50cb70, size 207 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/render_functions.md's phase-2 summary: "Computes the asymmetric near-plane
//   frustum extents (left/right/top/bottom) for a camera/viewport, validating the result."
//   Disassembly (objdump -d -M intel, 0x50cb70..0x50cc1f) confirms EAX=camera, EDX=bounds_in,
//   ECX=bounds_out (the leading "push ecx" only reserves a stack slot; ECX itself is never
//   reassigned before its use as the output pointer) and pins the two "less-than xor/eq"
//   comparisons as a strict less-than-or-equal and a strict greater-than respectively (the second
//   one uses `==`, not `!=`, between the two boolean terms, which negates the idiom seen
//   elsewhere in this module).
// register convention: EAX = camera (render_camera*), ECX = bounds_out (float[4]),
//   EDX = bounds_in (float[4], the asymmetric skew terms from
//   render_camera_compute_projection_skew).
//   // blam-cc: EAX=camera, ECX=bounds_out, EDX=bounds_in

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double ftan(double x); // x87 FPTAN

extern uint8_t render_asymmetric_frustum_disabled; // 0x00710321, UNSURE name/owner

// Scales the camera's asymmetric projection skew terms (bounds_in) by the field of view and
// viewport aspect ratio to produce near-plane frustum extents (bounds_out), validating both the
// input ordering (bounds_in[0] < bounds_in[1] and bounds_in[2] < bounds_in[3]) and the result
// before accepting it. Falls back to the symmetric unit bounds (-1, 1, -1, 1) and returns 0 when
// the asymmetric computation is disabled or its input or output is out of order.
uint32_t render_camera_compute_frustum_bounds(render_camera *camera, float bounds_out[4],
                                               float bounds_in[4]) // blam-cc: EAX=camera, ECX=bounds_out, EDX=bounds_in
{
    if (!render_asymmetric_frustum_disabled && bounds_in[0] < bounds_in[1] && bounds_in[2] < bounds_in[3]) {
        float inverse_tan_half_fov = 1.0f / (float)ftan((double)(camera->vertical_field_of_view * 0.5f));
        float aspect_scale = ((float)(camera->viewport_bounds.bottom - camera->viewport_bounds.top) /
                               (float)(camera->viewport_bounds.right - camera->viewport_bounds.left)) *
                              inverse_tan_half_fov;

        bounds_out[0] = aspect_scale * bounds_in[0];
        bounds_out[1] = aspect_scale * bounds_in[1];
        bounds_out[2] = inverse_tan_half_fov * bounds_in[2];
        bounds_out[3] = inverse_tan_half_fov * bounds_in[3];

        if (bounds_out[0] < bounds_out[1] && bounds_out[2] < bounds_out[3]) {
            return 1;
        }
    }

    bounds_out[3] = 1.0f;
    bounds_out[1] = 1.0f;
    bounds_out[2] = -1.0f;
    bounds_out[0] = -1.0f;
    return 0;
}

#if 0
Original Ghidra decompilation (0x50cb70):

undefined4 render_camera_compute_frustum_bounds(void)

{
  int in_EAX;
  float *in_ECX;
  float *in_EDX;
  float10 fVar1;
  float10 fVar2;

  if (((DAT_00710321 == '\0') && (*in_EDX < in_EDX[1])) && (in_EDX[2] < in_EDX[3])) {
    fVar1 = (float10)fptan((float10)*(float *)(in_EAX + 0x28) * (float10)0.5);
    fVar1 = (float10)1.0 / fVar1;
    fVar2 = ((float10)((int)*(short *)(in_EAX + 0x30) - (int)*(short *)(in_EAX + 0x2c)) /
            (float10)((int)*(short *)(in_EAX + 0x32) - (int)*(short *)(in_EAX + 0x2e))) * fVar1;
    *in_ECX = (float)(fVar2 * (float10)*in_EDX);
    in_ECX[1] = (float)(fVar2 * (float10)in_EDX[1]);
    in_ECX[2] = (float)(fVar1 * (float10)in_EDX[2]);
    fVar1 = fVar1 * (float10)in_EDX[3];
    in_ECX[3] = (float)fVar1;
    if ((*in_ECX < in_ECX[1]) && (fVar1 < (float10)in_ECX[2] == (fVar1 == (float10)in_ECX[2]))) {
      return 1;
    }
  }
  in_ECX[3] = 1.0;
  in_ECX[1] = 1.0;
  in_ECX[2] = -1.0;
  *in_ECX = -1.0;
  return 0;
}

The second validity comparison ((a<b)==(a==b)) is FALSE whenever a<b or a==b and TRUE only when
a>b -- i.e. it is a strict ">", the opposite of the "(a<b)!=(a==b)" ("<=") idiom seen in
render_player_frame.c. Substituting fVar1=bounds_out[3], b=bounds_out[2] gives bounds_out[3] >
bounds_out[2], i.e. bounds_out[2] < bounds_out[3], which is what the rewrite above tests.

Disassembly excerpt (objdump -d -M intel) confirming the register roles:

0050cb70:
  push   ecx                   ; reserves a stack slot; ecx itself keeps holding bounds_out
  push   ebx
  push   esi
  mov    esi,eax               ; esi = camera
  ...
  fld    DWORD PTR [edx]       ; bounds_in[0]      (edx = bounds_in)
  fcomp  DWORD PTR [edx+0x4]   ; vs bounds_in[1]
  ...
  fld    DWORD PTR [esi+0x28]  ; camera->vertical_field_of_view
  ...
  fld    st(0)
  fmul   DWORD PTR [edx]
  fstp   DWORD PTR [ecx]       ; bounds_out[0]
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
