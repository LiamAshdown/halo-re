// euler_angles_to_basis_vectors  (Ghidra: FUN_004cdde0; renamed, no established name)
// address 0x4cdde0, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: math_functions.md: "Wraps a helper that derives two output vectors from an input
//   direction vector, likely constructing tangent/bitangent-style basis vectors."
//   out/phase4/math_types_notes.md real_euler_angles3d section: calls
//   matrix4x3_from_euler_angles @0x4cba10 as (*in_EAX, in_EAX[1], in_EAX[2]), i.e. from one
//   3-float euler_angles3d object; real_matrix4x3 section: builds a matrix4x3 on the stack and
//   copies local_38..local_30 (matrix base+0x04, the forward row) and local_20..local_18
//   (matrix base+0x1c, the up row) out to its two output pointers.
// register convention: euler-angles pointer in EAX (in_EAX); output pointers in EDX
//   (extraout_EDX, up) and ESI (unaff_ESI, forward), no stack arguments.
//   // blam-cc: EAX -> angles, EDX -> up_out, ESI -> forward_out
//
//
// VERIFIED against the disassembly at 0x4cdde0 (objdump -d -M intel):
//   0x4cdde6  push [eax+8] / push [eax+4] / push [eax]   -> (yaw, pitch, roll) on the stack
//   0x4cddf1  lea eax,[ebp-0x38] / call 0x4cba10         -> the matrix's address goes in EAX
//   0x4cddf9  [esi+0..8] = [ebp-0x34], [ebp-0x30], [ebp-0x2c]   = matrix+0x04 -> forward row
//   0x4cde07  [edx+0..8] = [ebp-0x1c], [ebp-0x18], [ebp-0x14]   = matrix+0x1c -> up row
// Two things this settles:
//   * Ghidra's `extraout_EDX` is a false alarm. matrix4x3_from_euler_angles @0x4cba10 contains
//     no write to EDX/DX/DL anywhere in its 192 bytes, so EDX cannot be a value that call
//     returns; it is this function's own register parameter, live across the call.
//   * Ghidra reads the forward row from local_38/_34/_30, i.e. matrix+0x00..+0x08, which would
//     start at the scale field. The disassembly reads matrix+0x04..+0x0c, so it really is the
//     forward row, exactly as real_matrix4x3 in types/math.h lays it out.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10

void euler_angles_to_basis_vectors(real_euler_angles3d *angles, real_vector3d *up_out, real_vector3d *forward_out)
{
    real_matrix4x3 matrix;

    matrix4x3_from_euler_angles(&matrix, angles->yaw, angles->pitch, angles->roll);

    *forward_out = matrix.forward;
    *up_out = matrix.up;
}

#if 0
Original Ghidra decompilation (0x4cdde0):

void FUN_004cdde0(void)

{
  undefined4 *in_EAX;
  undefined4 *extraout_EDX;
  undefined4 *unaff_ESI;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;

  matrix4x3_from_euler_angles(*in_EAX,in_EAX[1],in_EAX[2]);
  *unaff_ESI = local_38;
  unaff_ESI[1] = local_34;
  unaff_ESI[2] = local_30;
  *extraout_EDX = local_20;
  extraout_EDX[1] = local_1c;
  extraout_EDX[2] = local_18;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
