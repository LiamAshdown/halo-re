// animation_replace_frame_orientations  (Ghidra: FUN_004d4dd0, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d4dd0, size 440 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: types notes "It requires type 2 (replacement)." Ghidra's decompile is structurally
//   complete for every branch except the two calls whose arguments it dropped
//   (animation_get_frame_data and animation_quaternion16_decode, both pure register calls).
//   Filled in from objdump -d -M intel --start-address=0x4d4dd0 --stop-address=0x4d4f90
//   bin/halo.exe: animation_get_frame_data(ECX = animation, stack = frame) returns the frame
//   cursor into EAX/ebp; animation_quaternion16_decode(ECX = frame cursor, EAX = &out[node].
//   rotation) is called with no stack args and the cursor is then advanced 8 bytes. The same
//   disassembly also recovered a hidden running "scale index" (DX of animation_node_get_scale)
//   that Ghidra's pseudocode never named as a variable, exactly as in
//   animation_get_frame_orientations.c. Unlike that sibling function, a mask bit that is clear
//   here leaves the matching field of *out_orientations completely untouched -- there is no
//   defaults stream in replacement mode, only animated (mask-set) components are ever written.
// register convention: animation in ESI (unaff_ESI); frame and the in/out array as the
//   recognized stack parameters (param_1, param_2).
//   // blam-cc: ESI -> animation, stack -> (frame, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4

extern void *animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame); // 0x4d4810, see animation_get_frame_data.c
extern void animation_quaternion16_decode(int16_t *source, real_quaternion *out); // 0x4d6330
extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, float frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, float frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0
extern void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index,
                                      float frame, float *out); // 0x4d6e80

// Overwrites only the rotation/translation/scale fields that `animation` (a type-2 replacement
// animation) actually animates for the given frame, leaving every other field of
// *out_orientations exactly as the caller had it. Does nothing if the animation is not type 2
// or frame is out of range.
void animation_replace_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame,
                                           real_orientation *out_orientations)
{
    uint8_t *frame_cursor;
    int use_compressed_codec;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;

    if (animation->type != 2) {
        return;
    }
    if (frame < 0 || (int16_t)animation->frame_count <= frame) { // signed word compare against +0x22
        return;
    }

    use_compressed_codec = ((animation->flags & 1) != 0) &&
                           !((animation_compressed_data_enabled == 0) && (animation->offset_to_compressed_data != 0));

    frame_cursor = (uint8_t *)animation_get_frame_data(animation, frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = animation->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = animation->node_transform_flag_data[mask_word];
            rotation_mask = animation->node_rotation_flag_data[mask_word];
            scale_mask = animation->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_rotation(animation, (float)frame, rotation_index, node, &out_node->rotation);
                rotation_index = rotation_index + 1;
            } else {
                animation_quaternion16_decode((int16_t *)frame_cursor, &out_node->rotation);
                frame_cursor += 8;
            }
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_translation(animation, (float)frame, translation_index, node, &out_node->translation);
                translation_index = translation_index + 1;
            } else {
                out_node->translation = *(real_point3d *)frame_cursor;
                frame_cursor += sizeof(real_point3d);
            }
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_scale(animation, scale_index, (float)frame, &out_node->scale);
                scale_index = scale_index + 1;
            } else {
                out_node->scale = *(float *)frame_cursor;
                frame_cursor += sizeof(float);
            }
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d4dd0):

void FUN_004d4dd0(undefined4 param_1,int param_2)

{
  bool bVar1;
  short sVar2;
  undefined4 *puVar3;
  int iVar4;
  uint uVar5;
  int unaff_ESI;
  int iVar6;
  int local_1c;
  uint local_18;
  int local_14;
  uint local_10;
  uint local_8;

  if (*(short *)(unaff_ESI + 0x20) == 2) {
    uVar5 = 0;
    sVar2 = (short)param_1;
    if ((-1 < sVar2) && (sVar2 < *(short *)(unaff_ESI + 0x22))) {
      if (((*(byte *)(unaff_ESI + 0x3a) & 1) == 0) ||
         ((DAT_006894b4 == '\0' && (*(int *)(unaff_ESI + 0x88) != 0)))) {
        bVar1 = false;
      }
      else {
        bVar1 = true;
      }
      puVar3 = (undefined4 *)FUN_004d4810(param_1);
      local_1c = 0;
      local_14 = 0;
      if (0 < *(short *)(unaff_ESI + 0x2c)) {
        do {
          iVar6 = (short)uVar5 * 0x20 + param_2;
          if ((uVar5 & 0x1f) == 0) {
            iVar4 = (int)(short)((short)uVar5 >> 5);
            local_10 = *(uint *)(unaff_ESI + 0x5c + iVar4 * 4);
            local_18 = *(uint *)(unaff_ESI + 0x6c + iVar4 * 4);
            local_8 = *(uint *)(unaff_ESI + 0x7c + iVar4 * 4);
          }
          if ((local_18 & 1) != 0) {
            if (bVar1) {
              model_node_get_interpolated_rotation((float)(int)sVar2,local_1c,uVar5,iVar6);
              local_1c = local_1c + 1;
            }
            else {
              FUN_004d6330();
              puVar3 = puVar3 + 2;
            }
          }
          local_18 = local_18 >> 1;
          if ((local_10 & 1) != 0) {
            if (bVar1) {
              model_node_get_interpolated_translation
                        ((float)(int)sVar2,local_14,uVar5,(undefined4 *)(iVar6 + 0x10));
              local_14 = local_14 + 1;
            }
            else {
              *(undefined4 *)(iVar6 + 0x10) = *puVar3;
              *(undefined4 *)(iVar6 + 0x14) = puVar3[1];
              *(undefined4 *)(iVar6 + 0x18) = puVar3[2];
              puVar3 = puVar3 + 3;
            }
          }
          local_10 = local_10 >> 1;
          if ((local_8 & 1) != 0) {
            if (bVar1) {
              model_node_get_interpolated_scale((float)(int)sVar2,iVar6 + 0x1c);
            }
            else {
              *(undefined4 *)(iVar6 + 0x1c) = *puVar3;
              puVar3 = puVar3 + 1;
            }
          }
          local_8 = local_8 >> 1;
          uVar5 = uVar5 + 1;
        } while ((short)uVar5 < *(short *)(unaff_ESI + 0x2c));
      }
    }
  }
  return;
}

objdump -d -M intel excerpt (the two calls Ghidra dropped, plus the hidden scale index):

004d4e1c:  55                    push   ebp                   ; save caller's ebp (used as the
                                                                ; frame cursor register here)
004d4e1d:  50                    push   eax                     ; frame
004d4e1e:  8b ce                 mov    ecx,esi                  ; ecx = animation
004d4e20:  e8 eb f9 ff ff        call   0x4d4810                  ; animation_get_frame_data
004d4e25:  83 c4 04              add    esp,0x4
004d4e2c:  8b e8                 mov    ebp,eax                    ; ebp = frame cursor

004d4eb0:  8b c7                 mov    eax,edi                     ; eax = &out[node].rotation
004d4eb2:  8b cd                 mov    ecx,ebp                      ; ecx = frame cursor
004d4eb4:  e8 77 14 00 00        call   0x4d6330                      ; animation_quaternion16_decode
004d4eb9:  83 c5 08              add    ebp,0x8                        ; cursor += 8

004d4f3e:  8b 54 24 20           mov    edx,DWORD PTR [esp+0x20]        ; edx (DX) = scale_index
004d4f49:  56                    push   edi                              ; out
004d4f4b:  8b ce                 mov    ecx,esi                           ; ecx = animation
004d4f50:  e8 2b 1f 00 00        call   0x4d6e80                           ; animation_node_get_scale
004d4f5c:  40                    inc    eax
004d4f5d:  89 44 24 20           mov    DWORD PTR [esp+0x20],eax           ; scale_index++
#endif
