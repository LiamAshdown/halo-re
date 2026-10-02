// vector3d_barycentric_interpolate  (orphan pass 4: FUN_004f06d0, no Ghidra name)
// address 0x4f06d0, size 87 bytes
// name confidence: 0.6 (behaviour is exactly barycentric interpolation of a 3-component
//   vector over a triangle: out = v0 + w1*(v1-v0) + w2*(v2-v0). Its only callers are the two
//   normal blends in object_lighting_sample_point (0x4f2830, 0x4f294b); the lightmap samplers
//   0x4f0730/0x4f0900 inline the same formula for their UVs)
// rewrite confidence: 0.85 (pure floating point, no memory writes beyond the 3 output floats,
//   register convention confirmed against objdump)
// evidence: out/phase4/objects_types_notes.md, "Not objects-module code": "0x4f06d0 / 0x4fca60
//   / 0x4fcb00 (barycentric and cubic interpolation) are generic helpers with no object in
//   sight. The interpolators belong with types/math.h if that header is ever extended; they
//   are not declared in objects.h." This pass moves it to src/math, following the precedent in
//   src/math/path_find_closest_point_on_segment.c of relocating plain math helpers that live in
//   a foreign module's address range. types/math.h real_vector3d (i 0x00, j 0x04, k 0x08).
// register convention (objdump 0x4f06d0..0x4f0726): output vector in EAX, v1 in ECX (weighted by
//   the SECOND stack float, [esp+8]), v2 in EDX (weighted by the FIRST stack float, [esp+4]), v0
//   (base) in ESI; each component is ((v2-v0)*[esp+4] + (v1-v0)*[esp+8]) + v0. The orphan pass 4
//   review corrected the weights, which the first draft had swapped (it multiplied v1 by the
//   first stack float). No struct pointer is returned; EAX is a genuine output-pointer parameter.
// blam-cc: EAX -> out, ECX -> v1, EDX -> v2, ESI -> v0, stack -> w2, w1
// FIXED (register inputs, objdump): notes were written as a full call signature instead of a
// parseable "REG -> name" mapping, so none of EAX/ECX/EDX/ESI (read at 0x4f06d0..0x4f06e6)
// looked claimed. Body already used all four correctly; reworded only.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void vector3d_barycentric_interpolate(real_vector3d *out, real_vector3d *v1, real_vector3d *v2,
                                       real_vector3d *v0, float w2, float w1)
{
    out->i = (v2->i - v0->i) * w2 + (v1->i - v0->i) * w1 + v0->i;
    out->j = (v2->j - v0->j) * w2 + (v1->j - v0->j) * w1 + v0->j;
    out->k = (v2->k - v0->k) * w2 + (v1->k - v0->k) * w1 + v0->k;
}

#if 0
Original Ghidra decompilation (0x4f06d0):

void vector3d_barycentric_interpolate(float param_1,float param_2)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;

  fVar1 = *unaff_ESI;
  *in_EAX = (*in_ECX - fVar1) * param_2 + (*in_EDX - fVar1) * param_1 + fVar1;
  fVar1 = unaff_ESI[1];
  in_EAX[1] = (in_ECX[1] - fVar1) * param_2 + (in_EDX[1] - fVar1) * param_1 + fVar1;
  fVar1 = unaff_ESI[2];
  in_EAX[2] = (in_ECX[2] - fVar1) * param_2 + (in_EDX[2] - fVar1) * param_1 + fVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
