// vector3d_quantize  (Ghidra: FUN_004eb4a0; named per this rewrite)
// address 0x4eb4a0, size 123 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Quantizes each axis of a 3D vector into an
// integer index using the shared float-quantization helper."); message_delta_quantize_float_to_int.c
// (this batch, the callee).
// register convention: EBX -> out_indices (3 ints), EDI -> range (min at [0], max at [1]).
//   // blam-cc: EBX -> out_indices, EDI -> range, stack -> point
// UNSURE: message_delta_vector3d_mode's two branches are byte-for-byte identical in the decompile
// (both call message_delta_quantize_float_to_int the same way); the max_level argument that
// function actually needs (its own ESI register) is not visible at this call site in either
// branch, so it is modeled as one explicit parameter shared by both axes rather than two silently
// duplicated branches.

#include "tags.h"
#include "memory.h"
#include "math.h"

extern uint8_t message_delta_vector3d_mode; // 0x0069b350
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum,
    real maximum); // this module, 0x4ea480

// Quantizes point's three axes into out_indices, each within [0, max_level], against the shared
// [range[0], range[1]] bounds.
void vector3d_quantize(int32_t *out_indices, real *range, uint32_t max_level, real *point)
    // blam-cc: EBX -> out_indices, EDI -> range, stack -> point
{
    out_indices[0] = message_delta_quantize_float_to_int(max_level, point[0], range[0], range[1]);
    out_indices[1] = message_delta_quantize_float_to_int(max_level, point[1], range[0], range[1]);
    out_indices[2] = message_delta_quantize_float_to_int(max_level, point[2], range[0], range[1]);
}

#if 0
Original Ghidra decompilation (0x4eb4a0), from tools/pack.py 0x4eb4a0:

void FUN_004eb4a0(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 *unaff_EBX;
  undefined4 *unaff_EDI;

  if (DAT_0069b350 == 0) {
    uVar1 = message_delta_quantize_float_to_int(*param_1,*unaff_EDI,unaff_EDI[1]);
    *unaff_EBX = uVar1;
    uVar1 = message_delta_quantize_float_to_int(param_1[1],*unaff_EDI,unaff_EDI[1]);
    unaff_EBX[1] = uVar1;
  }
  else {
    uVar1 = message_delta_quantize_float_to_int(*param_1,*unaff_EDI,unaff_EDI[1]);
    *unaff_EBX = uVar1;
    uVar1 = message_delta_quantize_float_to_int(param_1[1],*unaff_EDI,unaff_EDI[1]);
    unaff_EBX[1] = uVar1;
  }
  uVar1 = message_delta_quantize_float_to_int(param_1[2],*unaff_EDI,unaff_EDI[1]);
  unaff_EBX[2] = uVar1;
  return;
}
#endif
