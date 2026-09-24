// ai_search_choose_shorter_corner  (Ghidra: ai_search_choose_shorter_corner, renamed)
// address 0x43d240, size 614 bytes
// name confidence: 0.35  rewrite confidence: 0.05
// evidence: phase-4 summary "chooses whichever of two candidate polygon corners produces the
// shorter combined turn when a path bends around an obstacle." Calls
// vector2d_angle_between (0x4cd480, established elsewhere as `real vector2d_angle_between
// (real_vector2d *a, real_vector2d *b)`).
//
// This is the single least confident rewrite in this whole batch. Ghidra shows all four
// calls to vector2d_angle_between with zero visible arguments each -- a total, unrecoverable
// loss of every operand feeding this function's actual comparison. What follows is a
// structural guess (each candidate corner's turn measured against the same two reference
// directions this function has in scope) offered only so the file compiles and the control
// flow / return-value shape is preserved; the specific vectors compared are almost certainly
// wrong and this function needs a disassembly-based rewrite before it should be trusted.
//
// register convention: EBX -> corner_b (the other `unaff_` register Ghidra's decompile
//   shows); stack -> corner_a, reference_direction, out_point.
//   // blam-cc: EBX -> corner_b, stack -> corner_a, reference_direction, out_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern float vector2d_angle_between(real_vector2d *a, real_vector2d *b); // 0x4cd480

// blam-cc: EBX -> corner_b, stack -> corner_a, reference_direction, out_point
// UNSURE: see file header -- this is a structural placeholder, not a confirmed rewrite.
uint8_t ai_search_choose_shorter_corner(real_point2d *corner_a, real_vector2d *reference_direction,
                                        real_point2d *out_point, real_point2d *corner_b)
{
    float turn_a = vector2d_angle_between(reference_direction, (real_vector2d *)corner_a) +
                   vector2d_angle_between((real_vector2d *)corner_a, reference_direction);
    float turn_b = vector2d_angle_between(reference_direction, (real_vector2d *)corner_b) +
                   vector2d_angle_between((real_vector2d *)corner_b, reference_direction);

    if (-turn_b < turn_a) {
        *out_point = *corner_b;
        return 1;
    }
    *out_point = *corner_a;
    return 0;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d240 @ 0x43d240) ----
undefined4 FUN_0043d240(undefined4 *param_1,undefined4 param_2,undefined4 *param_3)

{
  undefined4 *unaff_EBX;
  float10 fVar1;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;

  fVar1 = (float10)vector2d_angle_between();
  fVar2 = (float10)vector2d_angle_between();
  fVar3 = (float10)vector2d_angle_between();
  fVar4 = (float10)vector2d_angle_between();
  if (-(fVar4 + (float10)(float)fVar3) < (float10)(float)(fVar2 + (float10)(float)fVar1)) {
    *param_3 = *unaff_EBX;
    param_3[1] = unaff_EBX[1];
    return 1;
  }
  *param_3 = *param_1;
  param_3[1] = param_1[1];
  return 0;
}
#endif
