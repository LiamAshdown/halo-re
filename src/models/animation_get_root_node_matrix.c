// animation_get_root_node_matrix  (Ghidra: FUN_004d49b0, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d49b0, size 72 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: objdump -d -M intel --start-address=0x4d49b0 --stop-address=0x4d49f8 bin/halo.exe:
//   the 0x800-byte stack reservation is a real_orientation[k_maximum_nodes_per_model] array
//   (matches models.h). It is filled by animation_get_frame_orientations, then node 0's
//   rotation alone is fed to matrix4x3_from_quaternion (ECX = &orientations[0].rotation, EDX =
//   the caller's out matrix), and finally node 0's translation overwrites out->position
//   directly -- out->scale is left at whatever matrix4x3_from_quaternion set (1.0);
//   orientations[0].scale is never read here.
// register convention: out matrix in EAX (in_EAX, becomes ESI across the call), frame in ECX,
//   animation in EDI (unaff_EDI, unmodified from entry); model as the recognized stack
//   parameter.
//   // blam-cc: EAX -> out, ECX -> frame, EDI -> animation, stack -> model

#include "tags.h"
#include "math.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0
extern void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model,
                                              int16_t frame, real_orientation *out_orientations); // 0x4d4a80

// Builds the root (node 0) world matrix for one animation frame: samples the frame's node
// orientations, converts node 0's rotation into *out, then replaces *out's position with node
// 0's translation (not run through the rotation/scale the matrix build just produced).
void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame,
                                     ModelAnimationsAnimation *animation, GBXModel *model)
{
    real_orientation orientations[k_maximum_nodes_per_model];

    animation_get_frame_orientations(animation, model, frame, orientations);
    matrix4x3_from_quaternion(&orientations[0].rotation, out);
    out->position = orientations[0].translation;
}

#if 0
Original Ghidra decompilation (0x4d49b0):

void FUN_004d49b0(void)

{
  int in_EAX;
  undefined4 local_7f0;
  undefined4 local_7ec;
  undefined4 local_7e8;

  FUN_004d4a80();
  matrix4x3_from_quaternion();
  *(undefined4 *)(in_EAX + 0x28) = local_7f0;
  *(undefined4 *)(in_EAX + 0x2c) = local_7ec;
  *(undefined4 *)(in_EAX + 0x30) = local_7e8;
  return;
}

objdump -d -M intel --start-address=0x4d49b0 --stop-address=0x4d49f8 bin/halo.exe:

004d49b0:  81 ec 00 08 00 00     sub    esp,0x800
004d49b6:  56                    push   esi
004d49b7:  8b f0                 mov    esi,eax
004d49b9:  8d 44 24 04           lea    eax,[esp+0x4]
004d49bd:  50                    push   eax
004d49be:  8b 84 24 0c 08 00 00  mov    eax,DWORD PTR [esp+0x80c]
004d49c5:  51                    push   ecx
004d49c6:  e8 b5 00 00 00        call   0x4d4a80
004d49cb:  83 c4 08              add    esp,0x8
004d49ce:  8d 4c 24 04           lea    ecx,[esp+0x4]
004d49d2:  8b d6                 mov    edx,esi
004d49d4:  e8 f7 70 ff ff        call   0x4cbad0
004d49d9:  8b 54 24 14           mov    edx,DWORD PTR [esp+0x14]
004d49dd:  8b 44 24 18           mov    eax,DWORD PTR [esp+0x18]
004d49e1:  8b 4c 24 1c           mov    ecx,DWORD PTR [esp+0x1c]
004d49e5:  83 c6 28              add    esi,0x28
004d49e8:  89 16                 mov    DWORD PTR [esi],edx
004d49ea:  89 46 04              mov    DWORD PTR [esi+0x4],eax
004d49ed:  89 4e 08              mov    DWORD PTR [esi+0x8],ecx
004d49f0:  5e                    pop    esi
004d49f1:  81 c4 00 08 00 00     add    esp,0x800
004d49f7:  c3                    ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
