// real_matrix4x3_rotation_from_forward  (Ghidra: FUN_0055eed0; renamed for this rewrite)
// address 0x55eed0, size 242 bytes
// name confidence: 0.45   rewrite confidence: 0.9 (checked against objdump -d 0x55eed0..0x55efc1 in step 1)
// evidence: out/phase4/units_types_notes.md "0x564ae0, 0x5579e0, 0x558860, 0x55eed0, 0x5658f0,
//   0x572a90 -- vector/basis helpers ... orthonormal rebuild". Sibling of
//   real_matrix4x3_rotation_rebuild_orthonormal (0x558860), simplified for the case where the
//   caller has no approximate up vector at all: it always starts from global_up3d, derives
//   left = up x forward, and falls back to global_forward3d as the up candidate (retrying the
//   cross product) only when forward is itself parallel to world-up (the first cross product
//   normalizes to zero length). The one difference from the sibling's retry path: the fallback
//   here goes through an explicit vector3d_cross_product call instead of being inlined, and
//   there is no third safety re-check on the final `up` (the sibling checks and defaults up
//   twice; this one only computes it once, at the very end, with no degenerate check at all).
// register convention: forward in ESI (read-only, confirmed by both call sites: 0x418c39 sets
//   ESI = ECX, an incoming pointer parameter; 0x55f02d, see below), left (out) in EBX, up (out)
//   in EDI -- both local buffers at the call sites. No stack arguments.
//   // blam-cc: ESI -> forward, EBX -> left (out), EDI -> up (out)

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX->out, ECX->a, stack->b (out = b x a)
extern const real_vector3d *global_forward3d_pointer; // 0x00696718 -> 0x0065c20c ( 1, 0, 0)
extern const real_vector3d *global_up3d_pointer;      // 0x00696720 -> 0x0065c224 ( 0, 0, 1)

static void cross(real_vector3d *out, real_vector3d *a, real_vector3d *b)
{
    out->i = a->j * b->k - a->k * b->j;
    out->j = a->k * b->i - a->i * b->k;
    out->k = a->i * b->j - a->j * b->i;
}

// Builds (left, up) orthonormal to `forward` starting from world-up, falling back to
// world-forward as the up candidate if `forward` is itself parallel to world-up.
void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    *up = *global_up3d_pointer;

    cross(left, up, forward); // left = up x forward
    if (vector3d_normalize_with_length(left) == 0.0f) {
        *up = *global_forward3d_pointer;
        // FIXED (objdump 0x55ef5b..0x55ef60): EAX = left, ECX = forward, push up; the helper computes
        // b x a, so this is up x forward like the main path (the draft passed (up, forward): forward x up)
        vector3d_cross_product(left, forward, up);
        vector3d_normalize_with_length(left);
    }

    cross(up, forward, left); // up = forward x left
    vector3d_normalize_with_length(up);
}

#if 0
Original Ghidra decompilation (0x55eed0):

void FUN_0055eed0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  undefined *puVar9;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar10;

  puVar9 = PTR_DAT_00696720;
  *unaff_EDI = *(float *)PTR_DAT_00696720;
  unaff_EDI[1] = *(float *)(puVar9 + 4);
  unaff_EDI[2] = *(float *)(puVar9 + 8);
  fVar1 = unaff_EDI[2];
  fVar2 = *unaff_ESI;
  fVar3 = unaff_ESI[2];
  fVar4 = *unaff_EDI;
  fVar5 = unaff_ESI[1];
  fVar6 = *unaff_EDI;
  fVar7 = unaff_EDI[1];
  fVar8 = *unaff_ESI;
  *unaff_EBX = unaff_EDI[1] * unaff_ESI[2] - unaff_EDI[2] * unaff_ESI[1];
  unaff_EBX[1] = fVar1 * fVar2 - fVar3 * fVar4;
  unaff_EBX[2] = fVar5 * fVar6 - fVar7 * fVar8;
  fVar10 = (float10)vector3d_normalize_with_length();
  puVar9 = PTR_DAT_00696718;
  if ((float10)0.0 == fVar10) {
    *unaff_EDI = *(float *)PTR_DAT_00696718;
    unaff_EDI[1] = *(float *)(puVar9 + 4);
    unaff_EDI[2] = *(float *)(puVar9 + 8);
    vector3d_cross_product();
    vector3d_normalize_with_length();
  }
  fVar1 = unaff_ESI[2];
  fVar2 = *unaff_EBX;
  fVar3 = unaff_EBX[2];
  fVar4 = *unaff_ESI;
  fVar5 = *unaff_ESI;
  fVar6 = unaff_EBX[1];
  fVar7 = unaff_ESI[1];
  fVar8 = *unaff_EBX;
  *unaff_EDI = unaff_EBX[2] * unaff_ESI[1] - unaff_ESI[2] * unaff_EBX[1];
  unaff_EDI[1] = fVar1 * fVar2 - fVar3 * fVar4;
  unaff_EDI[2] = fVar5 * fVar6 - fVar7 * fVar8;
  vector3d_normalize_with_length();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
