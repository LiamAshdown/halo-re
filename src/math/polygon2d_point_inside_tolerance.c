// polygon2d_point_inside_tolerance  (Ghidra: polygon2d_point_inside_tolerance, already named)
// address 0x4cad80, size 210 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/math_types_notes.md ("stride their vertex array by 8 ... which fixes the
//   2D size"); out/phase4/math_functions.md ("Tests whether a 2D point lies inside a convex
//   polygon within a given distance tolerance"). For each edge, the point is rejected once its
//   perpendicular distance outside that edge (compared via squared terms to avoid a sqrt)
//   exceeds tolerance.
// register convention: vertex array in ECX (in_ECX); count, point and tolerance as the three
//   recognized stack parameters, in that order.
//   // blam-cc: ECX -> vertices, stack -> (count, point, tolerance)

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Tests whether a 2D point lies inside a convex polygon within a given distance tolerance.
uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count, real_point2d *point, real tolerance)
{
    int16_t i;
    int16_t next;
    real edge_dx, edge_dy, dist_sq, cross;
    real tolerance_sq = tolerance * tolerance; // 0x4cad81..0x4cad96, rounded to a float local

    if (0 < count) {
        for (i = 0; i < count; i++) {
            next = (int16_t)(i + 1);
            if (count <= next) {
                next = 0;
            }
            edge_dx = vertices[next].x - vertices[i].x;
            edge_dy = vertices[next].y - vertices[i].y;
            dist_sq = edge_dx * edge_dx + edge_dy * edge_dy;
            if (dist_sq != 0.0f) {
                cross = (point->x - vertices[i].x) * edge_dy - edge_dx * (point->y - vertices[i].y);
                if (0.0f < cross) {
                    if (dist_sq * tolerance_sq < cross * cross) { // 0x4cae1b: dist_sq * the float tol^2 from entry
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cad80):

undefined4 polygon2d_point_inside_tolerance(short param_1,float *param_2,float param_3)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined4 uVar5;
  int iVar6;
  int in_ECX;
  uint uVar7;
  short sVar8;
  
  sVar8 = 0;
  uVar5 = 1;
  if (0 < param_1) {
    while( true ) {
      iVar6 = (int)sVar8;
      uVar7 = ((int)param_1 <= (int)(iVar6 + 1U)) - 1 & iVar6 + 1U;
      pfVar1 = (float *)(in_ECX + 4 + iVar6 * 8);
      fVar2 = *(float *)(in_ECX + uVar7 * 8) - *(float *)(in_ECX + iVar6 * 8);
      fVar4 = *(float *)(in_ECX + 4 + uVar7 * 8) - *pfVar1;
      fVar3 = fVar4 * fVar4 + fVar2 * fVar2;
      if (((fVar3 != 0.0) &&
          (fVar2 = (*param_2 - *(float *)(in_ECX + iVar6 * 8)) * fVar4 -
                   fVar2 * (param_2[1] - *pfVar1), 0.0 < fVar2)) &&
         (uVar5 = 0, fVar3 * param_3 * param_3 < fVar2 * fVar2)) break;
      sVar8 = sVar8 + 1;
      if (param_1 <= sVar8) {
        return 1;
      }
    }
  }
  return uVar5;
}
#endif
