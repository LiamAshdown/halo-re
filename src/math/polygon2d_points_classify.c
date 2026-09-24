// polygon2d_points_classify  (Ghidra: FUN_004caa40; renamed, Blam-style, not previously named)
// address 0x4caa40, size 153 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md ("Classifies a point set as degenerate
//   (coincident/collinear) or in general position, feeding the convex-hull builder at
//   0x4caae0"); confirmed as polygon2d_convex_hull_build @0x4caae0's first call, forwarding its
//   own (points, count) and returning early unless the classification is 2.
// register convention: points array in EBX (real_point2d, stride 8); count as the single
//   16-bit stack parameter; the result comes back in AX only (mov ax,si at 0x4caad0).
//   // blam-cc: EBX -> points, stack -> count
//
// VERIFIED against the disassembly at 0x4caa40 (objdump -d -M intel). An earlier reading called
// this "one of the worst Ghidra decompiles in the module" because local_c / local_8 / local_4
// are used without ever being assigned. They are not dropped statements: they are the
// three-float OUTPUT of the call at 0x4caaa4, whose pointer arguments Ghidra lost because all
// three travel in registers:
//   0x4caa96  movsx ecx,di / lea edx,[ebx+ecx*8]   EDX = &points[i]
//   0x4caa9c  lea eax,[esp+0xc]                    EAX = &first_point (saved at state -1)
//   0x4caaa0  lea ecx,[esp+0x14]                   ECX = &edge          <- local_c/_8/_4
//   0x4caaa4  call 0x44d950
// and 0x44d950 (out of module, still FUN_0044d950 in symbols/functions.txt) is
//   plane2d_from_points(ECX = out, EAX = a, EDX = b):
//     out.normal.i = a.y - b.y;  out.normal.j = b.x - a.x;
//     len = sqrt(i*i + j*j);  if (|len| < 0.0001) { out.d = 0; return NULL; }
//     out.normal /= len;  out.d = dot(out.normal, b);  return out;
// so the state machine is fully determined:
//   state -1  nothing seen yet          -> remember points[i], go to 0
//   state  0  one point seen            -> try to build the edge plane through it and
//                                          points[i]; a non-NULL result (the two points are
//                                          further apart than 0.0001) advances to 1
//   state  1  a line is established     -> any point further than 0.0001 off that line is
//                                          proof of general position, go to 2 and stop
// Returns -1 for an empty array, 0 if every point is the first point, 1 if they are all
// collinear, 2 for general position.
//
// The epsilon at 0x672bd8 is the double 9.999999747378752e-05 (fcomp QWORD, 0x4caa82), i.e. the
// float 0.0001f widened to double, so the comparison happens in double and is written that way
// below.

#include "tags.h"
#include "math.h"

// 0x44d950, out of this module and still unnamed in symbols/functions.txt; ECX = out plane,
// EAX = first point, EDX = second point. Returns out_plane, or NULL if the points coincide.
extern real_plane2d *plane2d_from_points(real_plane2d *out_plane, const real_point2d *a, const real_point2d *b);

// fabs is a single x87 FABS instruction in the original code (Ghidra's ABS() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double fabs(double x);

// Classifies a point set as degenerate (coincident/collinear) or in general position, feeding
// the convex-hull builder at 0x4caae0.
int16_t polygon2d_points_classify(real_point2d *points, int16_t count)
{
    int16_t state;
    int16_t i;
    real_point2d first_point; // [esp+0xc]
    real_plane2d edge;        // [esp+0x14]
    real_point2d *p;

    state = -1;
    i = 0;
    while (i < count) {
        if (state == -1) {
            first_point = points[i];
            state = 0;
        } else if (state == 0) {
            if (plane2d_from_points(&edge, &first_point, &points[i]) != 0) {
                state = 1;
            }
        } else if (state == 1) {
            p = &points[i];
            if (0.0001 <= fabs((double)((edge.normal.i * p->x + edge.normal.j * p->y) - edge.d))) {
                state = 2;
            }
        }
        i = i + 1;
        if (2 <= state) {
            break;
        }
    }
    return state;
}

#if 0
Original Ghidra decompilation (0x4caa40):

undefined4 FUN_004caa40(short param_1)

{
  float *pfVar1;
  uint in_EAX;
  undefined2 uVar2;
  int unaff_EBX;
  short sVar3;
  short sVar4;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  sVar3 = -1;
  sVar4 = 0;
  do {
    uVar2 = (undefined2)(in_EAX >> 0x10);
    if (param_1 <= sVar4) break;
    in_EAX = (uint)sVar3;
    if (in_EAX == 0xffffffff) {
      in_EAX = *(uint *)(unaff_EBX + sVar4 * 8);
      sVar3 = 0;
    }
    else if (in_EAX == 0) {
      in_EAX = FUN_0044d950();
      if (in_EAX != 0) {
        sVar3 = 1;
      }
    }
    else if ((in_EAX == 1) &&
            (pfVar1 = (float *)(unaff_EBX + sVar4 * 8), in_EAX = (uint)pfVar1 & 0xffff0000,
            0.0001 <= ABS((local_c * *pfVar1 + local_8 * *(float *)(unaff_EBX + 4 + sVar4 * 8)) -
                          local_4))) {
      sVar3 = 2;
    }
    uVar2 = (undefined2)(in_EAX >> 0x10);
    sVar4 = sVar4 + 1;
  } while (sVar3 < 2);
  return CONCAT22(uVar2,sVar3);
}
#endif
