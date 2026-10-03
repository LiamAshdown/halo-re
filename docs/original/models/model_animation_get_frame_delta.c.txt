// model_animation_get_frame_delta  (Ghidra: model_animation_get_frame_delta, already named;
// out/phase4/models_types_notes.md calls this name "close to correct" and suggests
// animation_get_root_node_translation_delta -- kept as-is per the phase 2 name)
// address 0x4d4a00, size 118 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: objdump -d -M intel --start-address=0x4d4a00 --stop-address=0x4d4a80 bin/halo.exe.
//   Two back-to-back real_orientation[k_maximum_nodes_per_model] stack buffers (0x1000 bytes
//   total, alloca via __chkstk) are filled by animation_get_frame_orientations for the given
//   frame (clamped to at least 1) and for frame-1; only node 0's translation is read from
//   each, and the difference is written to *out.
// register convention: frame in ECX (mov esi,ecx / test si,si), animation in EDX (mov edi,edx),
//   out vector in EBX (unaff_EBX, used directly, never reloaded); model as the recognized stack
//   parameter.
//   // blam-cc: ECX -> frame, EDX -> animation, EBX -> out, stack -> model

#include "tags.h"
#include "math.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model,
                                              int16_t frame, real_orientation *out_orientations); // 0x4d4a80

// Computes the root (node 0) translation delta between animation frame `frame` (treated as 1
// when 0) and the frame before it.
void model_animation_get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation,
                                      real_vector3d *out, GBXModel *model)
{
    real_orientation current_frame[k_maximum_nodes_per_model];
    real_orientation previous_frame[k_maximum_nodes_per_model];
    int16_t use_frame;

    use_frame = (frame == 0) ? 1 : frame;
    animation_get_frame_orientations(animation, model, use_frame, current_frame);
    animation_get_frame_orientations(animation, model, (int16_t)(use_frame - 1), previous_frame);
    out->i = current_frame[0].translation.x - previous_frame[0].translation.x;
    out->j = current_frame[0].translation.y - previous_frame[0].translation.y;
    out->k = current_frame[0].translation.z - previous_frame[0].translation.z;
}

#if 0
Original Ghidra decompilation (0x4d4a00):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void model_animation_get_frame_delta(void)

{
  int in_ECX;
  float *unaff_EBX;
  undefined1 local_1000 [16];
  float local_ff0;
  float local_fec;
  float local_fe8;
  undefined1 local_800 [16];
  float local_7f0;
  float local_7ec;
  float local_7e8;
  undefined4 uStack_4;

  uStack_4 = 0x4d4a0a;
  if ((short)in_ECX == 0) {
    in_ECX = 1;
  }
  FUN_004d4a80(in_ECX,local_1000);
  FUN_004d4a80(in_ECX + -1,local_800);
  *unaff_EBX = local_ff0 - local_7f0;
  unaff_EBX[1] = local_fec - local_7ec;
  unaff_EBX[2] = local_fe8 - local_7e8;
  return;
}

objdump -d -M intel --start-address=0x4d4a00 --stop-address=0x4d4a80 bin/halo.exe:

004d4a00:  b8 00 10 00 00        mov    eax,0x1000
004d4a05:  e8 36 38 15 00        call   0x628240
004d4a0a:  55                    push   ebp
004d4a0b:  8b ac 24 08 10 00 00  mov    ebp,DWORD PTR [esp+0x1008]
004d4a12:  56                    push   esi
004d4a13:  8b f1                 mov    esi,ecx
004d4a15:  66 85 f6              test   si,si
004d4a18:  57                    push   edi
004d4a19:  8b fa                 mov    edi,edx
004d4a1b:  75 05                 jne    0x4d4a22
004d4a1d:  be 01 00 00 00        mov    esi,0x1
004d4a22:  8d 44 24 0c           lea    eax,[esp+0xc]
004d4a26:  50                    push   eax
004d4a27:  56                    push   esi
004d4a28:  8b c5                 mov    eax,ebp
004d4a2a:  e8 51 00 00 00        call   0x4d4a80
004d4a2f:  8d 8c 24 14 08 00 00  lea    ecx,[esp+0x814]
004d4a36:  51                    push   ecx
004d4a37:  4e                    dec    esi
004d4a38:  56                    push   esi
004d4a39:  8b c5                 mov    eax,ebp
004d4a3b:  e8 40 00 00 00        call   0x4d4a80
004d4a40:  d9 44 24 2c           fld    DWORD PTR [esp+0x2c]
004d4a44:  d8 a4 24 2c 08 00 00  fsub   DWORD PTR [esp+0x82c]
004d4a4b:  83 c4 10              add    esp,0x10
004d4a4e:  5f                    pop    edi
004d4a4f:  5e                    pop    esi
004d4a50:  d9 1b                 fstp   DWORD PTR [ebx]
004d4a52:  5d                    pop    ebp
004d4a53:  d9 44 24 14           fld    DWORD PTR [esp+0x14]
004d4a57:  d8 a4 24 14 08 00 00  fsub   DWORD PTR [esp+0x814]
004d4a5e:  d9 5b 04              fstp   DWORD PTR [ebx+0x4]
004d4a61:  d9 44 24 18           fld    DWORD PTR [esp+0x18]
004d4a65:  d8 a4 24 18 08 00 00  fsub   DWORD PTR [esp+0x818]
004d4a6c:  d9 5b 08              fstp   DWORD PTR [ebx+0x8]
004d4a6f:  81 c4 00 10 00 00     add    esp,0x1000
004d4a75:  c3                    ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
