// matrix3x3_transpose  (Ghidra: matrix3x3_transpose, already named)
// address 0x4cc500, size 94 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: types/math.h real_matrix3x3 section ("matrix3x3_transpose... copy exactly 9
//   floats"); handles in==out aliasing with a 3-swap in-place path, otherwise a full 9-float
//   copy with rows/columns swapped.
// register convention: output matrix in EAX (in_EAX), input matrix in ECX (in_ECX).
//   // blam-cc: EAX -> out, ECX -> in

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Transposes a 3x3 matrix, handling in-place (aliased) transposition.
void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in)
{
    real t;

    if (in == out) {
        t = in->forward.j;
        out->forward.j = in->left.i;
        out->left.i = t;
        t = in->forward.k;
        out->forward.k = in->up.i;
        out->up.i = t;
        t = in->left.k;
        out->left.k = in->up.j;
        out->up.j = t;
        return;
    }
    out->forward.i = in->forward.i;
    out->forward.j = in->left.i;
    out->forward.k = in->up.i;
    out->left.i = in->forward.j;
    out->left.j = in->left.j;
    out->left.k = in->up.j;
    out->up.i = in->forward.k;
    out->up.j = in->left.k;
    out->up.k = in->up.k;
}

#if 0
Original Ghidra decompilation (0x4cc500):

void matrix3x3_transpose(void)

{
  undefined4 uVar1;
  undefined4 *in_EAX;
  undefined4 *in_ECX;

  if (in_ECX == in_EAX) {
    uVar1 = in_ECX[3];
    in_EAX[3] = in_ECX[1];
    in_EAX[1] = uVar1;
    uVar1 = in_ECX[6];
    in_EAX[6] = in_ECX[2];
    in_EAX[2] = uVar1;
    uVar1 = in_ECX[7];
    in_EAX[7] = in_ECX[5];
    in_EAX[5] = uVar1;
    return;
  }
  *in_EAX = *in_ECX;
  in_EAX[1] = in_ECX[3];
  in_EAX[2] = in_ECX[6];
  in_EAX[3] = in_ECX[1];
  in_EAX[4] = in_ECX[4];
  in_EAX[5] = in_ECX[7];
  in_EAX[6] = in_ECX[2];
  in_EAX[7] = in_ECX[5];
  in_EAX[8] = in_ECX[8];
  return;
}
#endif
