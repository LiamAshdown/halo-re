// path_find_heights_are_close  (Ghidra: path_find_heights_are_close, renamed)
// address 0x43d910, size 160 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: phase-4 summary "checks whether two navmesh cluster locations sit at nearly the
// same height, used to reject path steps needing a large vertical jump." Calls FUN_0044d860
// (outside this rewrite's range) twice to resolve each vertex's position; the CONCAT/NAN
// wrapping on the return value is this module's familiar "clean 0/1 in the low byte, garbage
// above" pattern, reproduced here as a plain uint8_t return.
// register convention: ECX -> vertex_a, stack -> vertex_b.
//   // blam-cc: ECX -> vertex_a, stack -> vertex_b
//
// UNSURE: FUN_0044d860 is called here with only its output pointer visible; the vertex id it
// resolves a position for is presumably forwarded via a register this decompilation does not
// show (vertex_a for the first call, vertex_b for the second, by position in the source).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double fabs(double x); // ABS
extern void FUN_0044d860(real_point3d *out_position); // 0x44d860, outside this rewrite's range; see header UNSURE

// blam-cc: ECX -> vertex_a, stack -> vertex_b
uint8_t path_find_heights_are_close(int32_t vertex_a, int32_t vertex_b)
{
    if ((vertex_a != -1) && (vertex_b != -1)) {
        real_point3d position_a, position_b;
        FUN_0044d860(&position_a);
        FUN_0044d860(&position_b);
        return fabs(position_a.z - position_b.z) < 0.05;
    }
    return 0;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d910 @ 0x43d910) ----
uint FUN_0043d910(int param_1)

{
  float fVar1;
  uint in_EAX;
  uint uVar2;
  undefined2 extraout_var;
  uint3 uVar3;
  int in_ECX;
  undefined1 local_18 [8];
  float local_10;
  undefined1 local_c [8];
  float local_4;

  uVar2 = in_EAX & 0xffffff00;
  if ((in_ECX != -1) && (param_1 != -1)) {
    FUN_0044d860(local_18);
    FUN_0044d860(local_c);
    fVar1 = ABS(local_10 - local_4);
    uVar3 = (uint3)(CONCAT22(extraout_var,
                             (ushort)(fVar1 < 0.05) << 8 | (ushort)NAN(fVar1) << 10 |
                             (ushort)(fVar1 == 0.05) << 0xe) >> 8);
    if (fVar1 < 0.05) {
      return CONCAT31(uVar3,1);
    }
    uVar2 = (uint)uVar3 << 8;
  }
  return uVar2;
}
#endif
