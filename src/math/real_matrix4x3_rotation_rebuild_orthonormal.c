// real_matrix4x3_rotation_rebuild_orthonormal  (Ghidra: FUN_00558860; renamed for this rewrite)
// address 0x558860, size 446 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/units_types_notes.md "0x564ae0, 0x5579e0, 0x558860, 0x55eed0, 0x5658f0,
//   0x572a90 -- vector/basis helpers ... orthonormal rebuild". Pairs with
//   real_matrix4x3_rotation_is_orthonormal (0x5579e0): given a forward vector and an
//   approximate up vector, re-derives left = up x forward, then up = forward x left (so the
//   result is guaranteed orthonormal even if the input up wasn't perpendicular to forward), each
//   step re-normalized and defaulted to the matching global_*3d constant
//   (types/math.h) if the input degenerates to zero length. The only caller (0x558c00, `units`)
//   passes `&m[i].forward` (+0x04), `&m[i].left` (+0x10) and `&m[i].up` (+0x1c) of a
//   real_matrix4x3-shaped array (same call site pattern, same stride 0x34, as
//   real_matrix4x3_rotation_is_orthonormal's caller).
// register convention: forward in ESI (normalized in place), left in EBX (computed, out), up in
//   EDI (normalized in place, then recomputed). Confirmed against the caller's
//   `lea edi,[eax+0x1c]; lea ebx,[eax+0x10]; lea esi,[eax+0x4]` immediately before the call.
//   // blam-cc: ESI -> forward, EBX -> left, EDI -> up
// UNSURE: the final re-normalize of `left` (step 5 below) defaults to whatever
//   global_forward3d_pointer (0x696718) still holds in the local that step 1 loaded it into,
//   not global_left3d_pointer (0x69671c) as step 3's own default does. This looks like a genuine
//   original-game bug (the degenerate-left fallback silently becomes forward instead of left on
//   the last pass) rather than an intentional choice; preserved exactly, not "fixed".

#include "tags.h"
#include "math.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern const real_vector3d *global_forward3d_pointer; // 0x00696718 -> 0x0065c20c ( 1, 0, 0)
extern const real_vector3d *global_left3d_pointer;    // 0x0069671c -> 0x0065c218 ( 0, 1, 0)
extern const real_vector3d *global_up3d_pointer;      // 0x00696720 -> 0x0065c224 ( 0, 0, 1)

static void cross(real_vector3d *out, real_vector3d *a, real_vector3d *b)
{
    out->i = a->j * b->k - a->k * b->j;
    out->j = a->k * b->i - a->i * b->k;
    out->k = a->i * b->j - a->j * b->i;
}

// Rebuilds (left, up) into an orthonormal basis with `forward`, defaulting any input that
// normalizes to zero length to the matching world axis.
void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    if (vector3d_normalize_with_length(forward) == 0.0f) {
        *forward = *global_forward3d_pointer;
    }
    if (vector3d_normalize_with_length(up) == 0.0f) {
        *up = *global_up3d_pointer;
    }

    cross(left, up, forward);
    if (vector3d_normalize_with_length(left) == 0.0f) {
        *left = *global_left3d_pointer;
    }

    cross(up, forward, left);
    if (vector3d_normalize_with_length(up) == 0.0f) {
        *up = *global_up3d_pointer;
    }

    cross(left, up, forward);
    if (vector3d_normalize_with_length(left) == 0.0f) {
        // UNSURE: preserved bug -- the original defaults to global_forward3d here, not
        // global_left3d as the first `left` normalize above does. See file header.
        *left = *global_forward3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x558860):

void FUN_00558860(void)

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
  undefined *puVar10;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar11;

  fVar11 = (float10)vector3d_normalize_with_length();
  puVar9 = PTR_DAT_00696718;
  if ((float10)0.0 == fVar11) {
    *unaff_ESI = *(float *)PTR_DAT_00696718;
    unaff_ESI[1] = *(float *)(puVar9 + 4);
    unaff_ESI[2] = *(float *)(puVar9 + 8);
  }
  fVar11 = (float10)vector3d_normalize_with_length();
  puVar9 = PTR_DAT_00696720;
  if ((float10)0.0 == fVar11) {
    *unaff_EDI = *(float *)PTR_DAT_00696720;
    unaff_EDI[1] = *(float *)(puVar9 + 4);
    unaff_EDI[2] = *(float *)(puVar9 + 8);
  }
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
  fVar11 = (float10)vector3d_normalize_with_length();
  puVar9 = PTR_DAT_0069671c;
  if ((float10)0.0 == fVar11) {
    *unaff_EBX = *(float *)PTR_DAT_0069671c;
    unaff_EBX[1] = *(float *)(puVar9 + 4);
    unaff_EBX[2] = *(float *)(puVar9 + 8);
  }
  fVar1 = unaff_ESI[2];
  fVar2 = *unaff_EBX;
  fVar3 = unaff_EBX[2];
  fVar4 = *unaff_ESI;
  fVar5 = unaff_EBX[1];
  fVar6 = *unaff_ESI;
  fVar7 = unaff_ESI[1];
  fVar8 = *unaff_EBX;
  *unaff_EDI = unaff_EBX[2] * unaff_ESI[1] - unaff_ESI[2] * unaff_EBX[1];
  unaff_EDI[1] = fVar1 * fVar2 - fVar3 * fVar4;
  unaff_EDI[2] = fVar5 * fVar6 - fVar7 * fVar8;
  fVar11 = (float10)vector3d_normalize_with_length();
  puVar10 = PTR_DAT_00696720;
  if ((float10)0.0 == fVar11) {
    *unaff_EDI = *(float *)PTR_DAT_00696720;
    unaff_EDI[1] = *(float *)(puVar10 + 4);
    unaff_EDI[2] = *(float *)(puVar10 + 8);
  }
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
  fVar11 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar11) {
    *unaff_EBX = *(float *)puVar9;
    unaff_EBX[1] = *(float *)(puVar9 + 4);
    unaff_EBX[2] = *(float *)(puVar9 + 8);
  }
  return;
}
#endif
