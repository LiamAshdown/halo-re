// polygon2d_point_inside_margin  (Ghidra: polygon2d_point_inside_margin, already named)
// address 0x4cae60, size 128 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/math_functions.md ("Cheaper variant of the convex-polygon containment
//   test that compares raw cross-product margin instead of true distance"); same edge-vertex
//   striding (8 bytes) as polygon2d_point_inside_tolerance @0x4cad80.
// register convention: vertex array in ECX (in_ECX); count, point and margin as the three
//   recognized stack parameters, in that order.
//   // blam-cc: ECX -> vertices, stack -> (count, point, margin)

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Cheaper variant of the convex-polygon containment test that compares raw cross-product
// margin instead of true distance.
uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, real_point2d *point, real margin)
{
    int16_t i;
    int16_t next;
    real edge_dx, edge_dy, cross;

    if (0 < count) {
        for (i = 0; i < count; i++) {
            next = (int16_t)(i + 1);
            if (count <= next) {
                next = 0;
            }
            edge_dx = vertices[next].x - vertices[i].x;
            edge_dy = vertices[next].y - vertices[i].y;
            cross = edge_dx * (point->y - vertices[i].y) - (point->x - vertices[i].x) * edge_dy;
            if (cross < -margin) {
                return 0;
            }
        }
        return 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cae60):

undefined4 polygon2d_point_inside_margin(short param_1,float *param_2,float param_3)

{
  float *pfVar1;
  undefined4 uVar2;
  int iVar3;
  int in_ECX;
  uint uVar4;
  short sVar5;
  
  sVar5 = 0;
  uVar2 = 1;
  if (0 < param_1) {
    while( true ) {
      iVar3 = (int)sVar5;
      uVar4 = ((int)param_1 <= (int)(iVar3 + 1U)) - 1 & iVar3 + 1U;
      pfVar1 = (float *)(in_ECX + 4 + iVar3 * 8);
      if ((*(float *)(in_ECX + uVar4 * 8) - *(float *)(in_ECX + iVar3 * 8)) * (param_2[1] - *pfVar1)
          - (*param_2 - *(float *)(in_ECX + iVar3 * 8)) *
            (*(float *)(in_ECX + 4 + uVar4 * 8) - *pfVar1) < -param_3) break;
      sVar5 = sVar5 + 1;
      if (param_1 <= sVar5) {
        return 1;
      }
    }
    uVar2 = 0;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
