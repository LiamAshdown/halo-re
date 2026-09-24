// vector3d_cubic_interpolate  (orphan pass 4: FUN_004fcb00, no Ghidra name)
// address 0x4fcb00, size 168 bytes
// name confidence: 0.55 (calls cubic_interpolate_divided_difference once per component to
//   interpolate a 3-float vector through 4 control vectors at 4 sample positions; no Bungie
//   string or symbol names it)
// rewrite confidence: 0.7 (register convention confirmed against objdump: three of the four
//   input vector pointers arrive in registers EBX/EDI/ESI, which is unusual but consistent
//   with LTCG's per-function custom register allocation; the fourth input pointer and all four
//   sample positions plus the query position are ordinary cdecl stack arguments)
// evidence: out/phase4/objects_types_notes.md "the interpolators belong with types/math.h";
//   types/math.h real_vector3d (i 0x00, j 0x04, k 0x08). Sole caller is FUN_004fde40
//   (glow_particle_reposition per src/objects/README.md's misattribution table), which passes
//   4 glow-marker positions and their sample times to build a smooth path for a glow particle.
// register convention (confirmed via objdump): output vector in the 1st stack argument, the
//   first control-point vector (p0) in the 2nd stack argument, p1 in EBX, p2 in EDI, p3 in
//   ESI, the 4 sample positions (t0..t3) and the query position (t) as the remaining 5 stack
//   arguments -- confirmed by objdump's `mov edx,[esi]` / `mov eax,[edi]` / `mov ecx,[ebx]`
//   dereferences feeding each cubic_interpolate_divided_difference call, repeated at +0x4 and
//   +0x8 for the j/k components.
// blam-cc: EBX -> p1, EDI -> p2, ESI -> p3, stack -> out, p0, t0, t1, t2, t3, t
// FIXED (register inputs, objdump): EBX/EDI/ESI already had C parameters (p1/p2/p3) but no
//   machine-checked "blam-cc" line existed in the parser's "REG -> name" format (the old note
//   was a prose function signature); reworded so the checker recognizes them.

#include "tags.h"
#include "math.h"

extern float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3,
    float x0, float x1, float x2, float x3, float x); // 0x4fca60, same file

// blam-cc: EBX -> p1, EDI -> p2, ESI -> p3, stack -> out, p0, t0, t1, t2, t3, t
void vector3d_cubic_interpolate(real_vector3d *out, real_vector3d *p0, real_vector3d *p1,
                                 real_vector3d *p2, real_vector3d *p3,
                                 float t0, float t1, float t2, float t3, float t)
{
    out->i = cubic_interpolate_divided_difference(p0->i, p1->i, p2->i, p3->i, t0, t1, t2, t3, t);
    out->j = cubic_interpolate_divided_difference(p0->j, p1->j, p2->j, p3->j, t0, t1, t2, t3, t);
    out->k = cubic_interpolate_divided_difference(p0->k, p1->k, p2->k, p3->k, t0, t1, t2, t3, t);
}

#if 0
Original Ghidra decompilation (0x4fcb00):

void FUN_004fcb00(float *param_1,undefined4 *param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  undefined4 *unaff_EBX;
  undefined4 *unaff_ESI;
  undefined4 *unaff_EDI;
  float10 fVar1;

  fVar1 = (float10)FUN_004fca60(*param_2,*unaff_EBX,*unaff_EDI,*unaff_ESI,param_3,param_4,param_5,
                                param_6,param_7);
  *param_1 = (float)fVar1;
  fVar1 = (float10)FUN_004fca60(param_2[1],unaff_EBX[1],unaff_EDI[1],unaff_ESI[1],param_3,param_4,
                                param_5,param_6,param_7);
  param_1[1] = (float)fVar1;
  fVar1 = (float10)FUN_004fca60(param_2[2],unaff_EBX[2],unaff_EDI[2],unaff_ESI[2],param_3,param_4,
                                param_5,param_6,param_7);
  param_1[2] = (float)fVar1;
  return;
}
#endif
