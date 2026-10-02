// render_billboard_compute_view_fade  (Ghidra: FUN_005113b0; new name, evidence below)
// address 0x5113b0, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: phase4 one-liner: "Computes a view/normal alignment fade factor (0..1) for a
// billboard segment, inverted for a particular render-type selector."
// register convention (objdump 0x5113b0..0x511408): EAX = a (real_vector3d*), ECX = b
//   (real_vector3d*), stack = render_type.
//   // blam-cc: EAX -> a, ECX -> b, stack -> render_type

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern double fabs(double x); // ABS is a single x87 FABS instruction

// blam-cc: EAX -> a, ECX -> b, stack -> render_type
real render_billboard_compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type)
{
    real fade;
    real dot;
    real length;

    if (render_type == 0) {
        return 1.0f;
    }

    dot = a->i * b->i + a->j * b->j + a->k * b->k;
    length = (real)sqrt((double)(a->i * a->i + a->j * a->j + a->k * a->k));
    fade = (real)fabs((double)(dot / length));

    if (render_type == 2) {
        fade = 1.0f - fade;
    }
    return fade;
}

#if 0
Original Ghidra decompilation (0x5113b0):

float10 FUN_005113b0(short param_1)

{
  float *in_EAX;
  float *in_ECX;
  float10 fVar1;

  fVar1 = (float10)1.0;
  if ((param_1 != 0) &&
     (fVar1 = ABS(((float10)*in_EAX * (float10)*in_ECX +
                  (float10)in_ECX[1] * (float10)in_EAX[1] + (float10)in_ECX[2] * (float10)in_EAX[2])
                  / SQRT((float10)*in_EAX * (float10)*in_EAX +
                         (float10)in_EAX[2] * (float10)in_EAX[2] +
                         (float10)in_EAX[1] * (float10)in_EAX[1])), param_1 == 2)) {
    fVar1 = (float10)1.0 - fVar1;
  }
  return fVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
