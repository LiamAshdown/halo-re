// object_recompute_basis_from_marker_delta
// address 0x4f62f0, size 332 bytes
// name confidence: 0.4 (still FUN_004f62f0 in Ghidra; no functions.md entry describes this
//   address directly -- it is a private helper of FUN_004f6180/object_reorient_relative_to_
//   marker, called only from there in this batch)
// rewrite confidence: 0.75
// evidence: types/objects.h object (position 0x05c, forward 0x074, up 0x080), object_marker
//   (node_transform at 0x38); types/math.h real_matrix4x3. The stack frame was resolved against
//   the disassembly (objdump -d -M intel bin/halo.exe, 0x4f62f0..0x4f6433): Ghidra numbers its
//   locals four bytes below the EBP displacement, so Ghidra's local_54 is [ebp-0x50] and the
//   whole 0x34-byte span [ebp-0x50]..[ebp-0x1d] is ONE real_matrix4x3.
// register convention: object* in EAX (in_EAX), object_marker* on the stack ([ebp+0x8]),
//   output matrix pointer on the stack ([ebp+0xc]).
//   // blam-cc: EAX -> obj; stack -> marker, output_matrix
// resolved from disassembly (previously UNSURE):
//   - 0x4f62fe lea eax,[ebp-0x50] / lea edi,[esi+0x80] / lea ebx,[esi+0x74] / push eax /
//     mov eax,edi / mov ecx,ebx / call 0x4cb970 -- FUN_004cb970 is math's
//     matrix4x3_from_forward_up(up in EAX, forward in ECX, out on the stack), so this builds
//     the object's basis matrix from obj->up and obj->forward.
//   - Ghidra's local_2c/local_28/local_24 are [ebp-0x28]/[ebp-0x24]/[ebp-0x20], which is
//     exactly basis.position (0x28 into the matrix at [ebp-0x50]). They are NOT a separate
//     saved-position triple: the code fills the matrix's translation row from obj->position
//     and later copies it straight back, so the pair reads as
//     "basis.position = obj->position" followed by a redundant write-back.
//   - 0x4f6330 / 0x4f6357: matrix4x3_inverse takes its destination in EAX and its source in
//     ECX (verified against the body at 0x4cb7a0), so the first call is
//     inverse(relative, basis) and the second inverts relative in place.
//   - 0x4f6423 mov ecx,ebx / call 0x401990 and 0x4f642c mov ecx,edi / call 0x401990 --
//     vector3d_normalize_with_length takes its vector in ECX, so the two trailing calls
//     normalize obj->forward and then obj->up. Both results are discarded (fstp st(0)).
// UNSURE: the indirect call through the global function pointer at 0x00696664 is kept as an
//   indirect call rather than folded into the directly-callable matrix4x3_multiply used
//   elsewhere in the codebase, since this function demonstrably uses the indirection.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_math.h"

extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664
    // PHASE-4 REVIEW: this file used to call the same global `matrix4x3_multiply_dispatch`
    // and document its parameters as (dest, a, b). src/math/math_initialize.c, which assigns
    // the global, and the five other files that read it all use the name and the (a, b, out)
    // order below; the two call sites here already pass their operands in that order.


// Produces the transform that carries the object from its current placement to the marker's
// node placement, and reorthonormalizes the object's own forward/up basis on the way through.
void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker,
                                               real_matrix4x3 *output_matrix)
    // blam-cc: EAX -> obj; stack -> marker, output_matrix
{
    real_matrix4x3 relative;  // [ebp-0x88], Ghidra local_8c
    real_matrix4x3 basis;     // [ebp-0x50], Ghidra local_54..local_24
    real_vector3d cross;      // [ebp-0xc]/[ebp-0x8]/[ebp-0x4], Ghidra local_10/local_c/local_8

    matrix4x3_from_forward_up(&obj->up, &obj->forward, &basis);
    basis.position = obj->position;

    matrix4x3_inverse(&relative, &basis);
    matrix4x3_multiply_procedure(&relative, &marker->node_transform, &relative);
    matrix4x3_inverse(&relative, &relative);
    matrix4x3_multiply_procedure(output_matrix, &relative, &basis);

    obj->position = basis.position;
    obj->forward = basis.forward;

    // up' = (basis.up x basis.forward) x basis.forward -- orthogonalize the up hint against
    // the forward axis, then renormalize both below.
    cross.i = basis.up.k * basis.forward.j - basis.up.j * basis.forward.k;
    cross.j = basis.forward.k * basis.up.i - basis.up.k * basis.forward.i;
    cross.k = basis.up.j * basis.forward.i - basis.up.i * basis.forward.j;

    obj->up.i = cross.j * basis.forward.k - cross.k * basis.forward.j;
    obj->up.j = cross.k * basis.forward.i - basis.forward.k * cross.i;
    obj->up.k = cross.i * basis.forward.j - cross.j * basis.forward.i;

    vector3d_normalize_with_length(&obj->forward);
    vector3d_normalize_with_length(&obj->up);
}

#if 0
Original Ghidra decompilation (0x4f62f0):

void FUN_004f62f0(int param_1,undefined4 param_2)

{
  int in_EAX;
  undefined1 local_8c [56];
  undefined1 local_54 [4];
  float local_50;
  float local_4c;
  float local_48;
  float local_38;
  float local_34;
  float local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  FUN_004cb970(local_54);
  local_2c = *(undefined4 *)(in_EAX + 0x5c);
  local_28 = *(undefined4 *)(in_EAX + 0x60);
  local_24 = *(undefined4 *)(in_EAX + 100);
  matrix4x3_inverse();
  (*(code *)PTR_matrix4x3_multiply_00696664)(local_8c,param_1 + 0x38,local_8c);
  matrix4x3_inverse();
  (*(code *)PTR_matrix4x3_multiply_00696664)(param_2,local_8c,local_54);
  *(undefined4 *)(in_EAX + 0x5c) = local_2c;
  *(undefined4 *)(in_EAX + 0x60) = local_28;
  local_10 = local_30 * local_4c - local_34 * local_48;
  *(undefined4 *)(in_EAX + 100) = local_24;
  *(float *)(in_EAX + 0x74) = local_50;
  *(float *)(in_EAX + 0x78) = local_4c;
  *(float *)(in_EAX + 0x7c) = local_48;
  local_c = local_48 * local_38 - local_30 * local_50;
  local_8 = local_34 * local_50 - local_38 * local_4c;
  local_1c = local_c * local_48 - local_8 * local_4c;
  *(float *)(in_EAX + 0x80) = local_1c;
  local_18 = local_8 * local_50 - local_48 * local_10;
  *(float *)(in_EAX + 0x84) = local_18;
  local_14 = local_10 * local_4c - local_c * local_50;
  *(float *)(in_EAX + 0x88) = local_14;
  vector3d_normalize_with_length();
  vector3d_normalize_with_length();
  return;
}
#endif
