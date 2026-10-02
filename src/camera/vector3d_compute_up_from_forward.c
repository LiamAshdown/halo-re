// vector3d_compute_up_from_forward  (Ghidra: FUN_004479c0; renamed for this rewrite)
// address 0x4479c0, size 149 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x4479c0..0x447a54; matches (the zero third component is folded).
// evidence: types/camera.h calls this "the up vector from forward" (its five call sites all pass
// a command's forward as input and its up as output, e.g. camera_third_person_compute_pov
// 0x447370: ESI=&command->forward, EDI=&command->up). Confirmed against objdump: builds the
// horizontal perpendicular (forward.j, -forward.i, 0), normalizes it via
// vector3d_normalize_with_length (falling back to (1, 0) when forward is vertical and the
// horizontal component is zero-length), then combines it with the original forward to produce up.
// register convention: ESI -> forward (unaff_ESI), EDI -> up (unaff_EDI); no stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX

// blam-cc: ESI -> forward, EDI -> up
void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *up)
{
    real_vector3d horizontal_perp;
    real length;
    float forward_i, forward_j, forward_k;

    horizontal_perp.i = forward->j;
    horizontal_perp.j = -forward->i;
    horizontal_perp.k = 0.0f;

    length = vector3d_normalize_with_length(&horizontal_perp);
    if (length == 0.0f) {
        // Forward has no horizontal component (looking straight up or down): fall back to an
        // arbitrary horizontal perpendicular instead of a zero vector.
        horizontal_perp.i = 1.0f;
        horizontal_perp.j = 0.0f;
    }

    forward_i = forward->i;
    forward_j = forward->j;
    forward_k = forward->k;

    up->i = horizontal_perp.j * forward_k - forward_j * 0.0f;
    up->j = forward_i * 0.0f - horizontal_perp.i * forward_k;
    up->k = horizontal_perp.i * forward_j - horizontal_perp.j * forward_i;
}

#if 0
Original Ghidra decompilation (0x4479c0):

void FUN_004479c0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar6;
  float local_18;

  local_18 = unaff_ESI[1];
  fVar5 = -*unaff_ESI;
  fVar6 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar6) {
    local_18 = 1.0;
    fVar5 = 0.0;
  }
  fVar1 = *unaff_ESI;
  fVar2 = unaff_ESI[2];
  fVar3 = unaff_ESI[1];
  fVar4 = *unaff_ESI;
  *unaff_EDI = fVar5 * unaff_ESI[2] - unaff_ESI[1] * 0.0;
  unaff_EDI[1] = fVar1 * 0.0 - local_18 * fVar2;
  unaff_EDI[2] = local_18 * fVar3 - fVar5 * fVar4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
