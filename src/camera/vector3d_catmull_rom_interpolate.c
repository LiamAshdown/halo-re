// vector3d_catmull_rom_interpolate  (Ghidra: vector3d_catmull_rom_interpolate, already named)
// address 0x447080, size 135 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: three calls to scalar_catmull_rom_interpolate (0x447000), one per component; callers
// (0x447190 camera_evaluate_animation_offset, confirmed by objdump at 0x447278) load four
// consecutive 0x3c-stride array elements at index i, i+1, i+2, i+3 and pass their bases as
// source0 (stack) and source1/source2/source3 (EBX/EDI/ESI), matching the scalar function's
// value0..value3 stencil.
// register convention: source1 in EBX (unaff_EBX), source3 in ESI (unaff_ESI), source2 in EDI
// (unaff_EDI); out, source0, time0, dt and time on the stack (all five recognized by Ghidra as
// param_1..param_5). blam-cc order: EBX, ESI, EDI, then the stack parameters in their original
// order.

#include "tags.h"
#include "memory.h"
#include "camera.h"

// blam-cc: __cdecl, all parameters on the stack (0x447000, this module)
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double scalar_catmull_rom_interpolate(float value0, float value1, float value2,
    float value3, float time0, float dt, float time);

// blam-cc: source1 = EBX, source3 = ESI, source2 = EDI; out, source0, time0, dt, time = stack
// (param_1..param_5)
// Interpolates a 3D vector component-wise, the same way scalar_catmull_rom_interpolate
// interpolates a single float: the cubic through source0/source1/source2/source3 at times
// time0/time0+dt/time0+2*dt/time0+3*dt, evaluated at time and written to *out.
void vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3, Vector3D *source2,
    Vector3D *out, Vector3D *source0, float time0, float dt, float time)
{
    double component;

    component = scalar_catmull_rom_interpolate(source0->i, source1->i, source2->i, source3->i,
        time0, dt, time);
    out->i = (float)component;
    component = scalar_catmull_rom_interpolate(source0->j, source1->j, source2->j, source3->j,
        time0, dt, time);
    out->j = (float)component;
    component = scalar_catmull_rom_interpolate(source0->k, source1->k, source2->k, source3->k,
        time0, dt, time);
    out->k = (float)component;
}

#if 0
Original Ghidra decompilation (0x447080):

void vector3d_catmull_rom_interpolate
               (float *param_1,float *param_2,float param_3,float param_4,float param_5)

{
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  double dVar1;

  dVar1 = scalar_catmull_rom_interpolate
                    (*param_2,*unaff_EBX,*unaff_EDI,*unaff_ESI,param_3,param_4,param_5);
  *param_1 = (float)dVar1;
  dVar1 = scalar_catmull_rom_interpolate
                    (param_2[1],unaff_EBX[1],unaff_EDI[1],unaff_ESI[1],param_3,param_4,param_5);
  param_1[1] = (float)dVar1;
  dVar1 = scalar_catmull_rom_interpolate
                    (param_2[2],unaff_EBX[2],unaff_EDI[2],unaff_ESI[2],param_3,param_4,param_5);
  param_1[2] = (float)dVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
