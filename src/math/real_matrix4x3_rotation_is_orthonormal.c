// real_matrix4x3_rotation_is_orthonormal  (Ghidra: FUN_005579e0; renamed for this rewrite)
// address 0x5579e0, size 164 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/units_types_notes.md "0x564ae0, 0x5579e0, 0x558860, 0x55eed0, 0x5658f0,
//   0x572a90 -- vector/basis helpers ... orthonormal rebuild". The only caller (0x558b90, in
//   `units`) indexes an array with stride 0x34 (sizeof real_matrix4x3, types/math.h) and passes
//   `&m[i].forward` (+0x04), `&m[i].left` (+0x10) and `&m[i].up` (+0x1c) as the three vectors,
//   confirming this checks a real_matrix4x3's own rotation rows, not three arbitrary vectors.
// register convention: forward in ESI, left in EDI, up in EBX; no stack arguments. Confirmed
//   against objdump 0x5579e0..0x557a83 (three `vector3d_is_unit_length` calls each move one of
//   ESI/EDI/EBX into EAX first) and the caller (`lea ebx,[eax+0x1c]; lea edi,[eax+0x10];
//   lea esi,[eax+0x4]` immediately before `call 0x5579e0`).
//   // blam-cc: ESI -> forward, EDI -> left, EBX -> up
// UNSURE: vector3d_is_unit_length (0x4476e0) and real_approximately_equal (0x447680) are not
//   in this task's address list; declared here with the signature this call site implies
//   (EAX -> v for the former; two float stack args for the latter, confirmed by the `fstp
//   [esp]` / `push 0x0` pair at each real_approximately_equal call site).

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t vector3d_is_unit_length(real_vector3d *v); // 0x4476e0, v in EAX
extern uint8_t real_approximately_equal(real a, real b); // 0x447680

// Checks that (forward, left, up) -- typically a real_matrix4x3's own rotation rows -- form a
// valid orthonormal basis: each is unit length, and every pair is mutually perpendicular.
uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    if (!vector3d_is_unit_length(forward)) return 0;
    if (!vector3d_is_unit_length(left)) return 0;
    if (!vector3d_is_unit_length(up)) return 0;

    if (!real_approximately_equal(forward->i * left->i + forward->j * left->j + forward->k * left->k, 0.0f))
        return 0;
    if (!real_approximately_equal(forward->i * up->i + forward->j * up->j + forward->k * up->k, 0.0f))
        return 0;
    if (!real_approximately_equal(left->i * up->i + left->j * up->j + left->k * up->k, 0.0f))
        return 0;

    return 1;
}

#if 0
Original Ghidra decompilation (0x5579e0):

undefined4 FUN_005579e0(void)

{
  char cVar1;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;

  cVar1 = vector3d_is_unit_length();
  if (cVar1 != '\0') {
    cVar1 = vector3d_is_unit_length();
    if (cVar1 != '\0') {
      cVar1 = vector3d_is_unit_length();
      if (cVar1 != '\0') {
        cVar1 = real_approximately_equal
                          (unaff_EDI[1] * unaff_ESI[1] +
                           unaff_ESI[2] * unaff_EDI[2] + *unaff_ESI * *unaff_EDI,0);
        if (cVar1 != '\0') {
          cVar1 = real_approximately_equal
                            (unaff_EDI[1] * unaff_EBX[1] +
                             unaff_EBX[2] * unaff_EDI[2] + *unaff_EDI * *unaff_EBX,0);
          if (cVar1 != '\0') {
            cVar1 = real_approximately_equal
                              (unaff_ESI[1] * unaff_EBX[1] +
                               unaff_ESI[2] * unaff_EBX[2] + *unaff_ESI * *unaff_EBX,0);
            if (cVar1 != '\0') {
              return 1;
            }
          }
        }
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
