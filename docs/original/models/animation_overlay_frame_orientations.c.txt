// animation_overlay_frame_orientations  (Ghidra: FUN_004d4f90, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d4f90, size 520 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (review pass: checked against objdump)
// evidence: types notes "It requires type 1 (overlay)." Same structure as
//   animation_replace_frame_orientations.c (animation_get_frame_data,
//   animation_quaternion16_decode, the hidden scale-index register), plus quaternion_multiply,
//   whose no-argument call site was resolved from
//   objdump -d -M intel --start-address=0x4d4f90 --stop-address=0x4d51a0 bin/halo.exe:
//   `mov edx,edi; mov eax,edi; lea ecx,[esp+0x40]; call 0x4cdbf0` -- EAX/EDX both point at
//   out[node].rotation (the existing orientation) and ECX points at the freshly decoded/
//   interpolated quaternion, so out.rotation = quaternion_multiply(existing, new, existing):
//   the new rotation is applied on top of what was already there. Translation instead adds the
//   new value component-wise (x87 fadd against out[node].translation) and scale multiplies
//   (fmul against out[node].scale) -- Ghidra's own decompile already got the add/multiply parts
//   right, only the two register-only calls needed filling in.
// register convention: animation in ESI (unaff_ESI); frame and the in/out array as the
//   recognized stack parameters (param_1, param_2).
//   // blam-cc: ESI -> animation, stack -> (frame, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t animation_compressed_data_enabled; // 0x006894b4

extern void *animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame); // 0x4d4810, see animation_get_frame_data.c
extern void animation_quaternion16_decode(int16_t *source, real_quaternion *out); // 0x4d6330
extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, float frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, float frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0
extern void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index,
                                      float frame, float *out); // 0x4d6e80
extern void quaternion_multiply(real_quaternion *a, real_quaternion *b, real_quaternion *out); // 0x4cdbf0, verified in src/math

// Blends `animation` (a type-1 overlay animation) onto *out_orientations for the given frame:
// for every node it animates, the new rotation is multiplied onto the existing rotation, the
// new translation is added to the existing translation, and the new scale is multiplied onto
// the existing scale. Nodes/components the animation does not touch are left unchanged. Does
// nothing if the animation is not type 1 or frame is out of range.
void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame,
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
    real_quaternion new_rotation;
    real_point3d new_translation;
    float new_scale;

    if (animation->type != 1) {
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
                animation_node_get_rotation(animation, (float)frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                animation_quaternion16_decode((int16_t *)frame_cursor, &new_rotation);
                frame_cursor += 8;
            }
            quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_translation(animation, (float)frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                new_translation = *(real_point3d *)frame_cursor;
                frame_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x + out_node->translation.x;
            out_node->translation.y = new_translation.y + out_node->translation.y;
            out_node->translation.z = new_translation.z + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_scale(animation, scale_index, (float)frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                new_scale = *(float *)frame_cursor;
                frame_cursor += sizeof(float);
            }
            out_node->scale = new_scale * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d4f90):

void FUN_004d4f90(undefined4 param_1,int param_2)

{
  bool bVar1;
  int iVar2;
  uint uVar3;
  short sVar4;
  int unaff_ESI;
  int iVar5;
  float *local_40;
  int local_3c;
  uint local_38;
  int local_34;
  uint local_30;
  uint local_28;
  float local_24;
  int local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined1 local_10 [16];

  if (*(short *)(unaff_ESI + 0x20) == 1) {
    uVar3 = 0;
    sVar4 = (short)param_1;
    if ((-1 < sVar4) && (sVar4 < *(short *)(unaff_ESI + 0x22))) {
      if (((*(byte *)(unaff_ESI + 0x3a) & 1) == 0) ||
         ((DAT_006894b4 == '\0' && (*(int *)(unaff_ESI + 0x88) != 0)))) {
        bVar1 = false;
      }
      else {
        bVar1 = true;
      }
      local_40 = (float *)FUN_004d4810(param_1);
      local_3c = 0;
      local_34 = 0;
      if (0 < *(short *)(unaff_ESI + 0x2c)) {
        do {
          iVar5 = (short)uVar3 * 0x20 + param_2;
          if ((uVar3 & 0x1f) == 0) {
            iVar2 = (int)(short)((short)uVar3 >> 5);
            local_30 = *(uint *)(unaff_ESI + 0x5c + iVar2 * 4);
            local_38 = *(uint *)(unaff_ESI + 0x6c + iVar2 * 4);
            local_28 = *(uint *)(unaff_ESI + 0x7c + iVar2 * 4);
          }
          if ((local_38 & 1) != 0) {
            if (bVar1) {
              local_20 = (int)sVar4;
              model_node_get_interpolated_rotation((float)local_20,local_3c,uVar3,local_10);
              local_3c = local_3c + 1;
            }
            else {
              FUN_004d6330();
              local_40 = local_40 + 2;
            }
            quaternion_multiply();
          }
          local_38 = local_38 >> 1;
          if ((local_30 & 1) != 0) {
            if (bVar1) {
              local_20 = (int)sVar4;
              model_node_get_interpolated_translation((float)local_20,local_34,uVar3,&local_1c);
              local_34 = local_34 + 1;
            }
            else {
              local_1c = *local_40;
              local_18 = local_40[1];
              local_14 = local_40[2];
              local_40 = local_40 + 3;
            }
            *(float *)(iVar5 + 0x10) = local_1c + *(float *)(iVar5 + 0x10);
            *(float *)(iVar5 + 0x14) = local_18 + *(float *)(iVar5 + 0x14);
            *(float *)(iVar5 + 0x18) = local_14 + *(float *)(iVar5 + 0x18);
          }
          local_30 = local_30 >> 1;
          if ((local_28 & 1) != 0) {
            if (bVar1) {
              local_20 = (int)sVar4;
              model_node_get_interpolated_scale((float)local_20,&local_24);
            }
            else {
              local_24 = *local_40;
              local_40 = local_40 + 1;
            }
            *(float *)(iVar5 + 0x1c) = local_24 * *(float *)(iVar5 + 0x1c);
          }
          local_28 = local_28 >> 1;
          uVar3 = uVar3 + 1;
        } while ((short)uVar3 < *(short *)(unaff_ESI + 0x2c));
      }
    }
  }
  return;
}

objdump -d -M intel excerpt (the quaternion_multiply call Ghidra dropped):

004d5085:  8b d7                 mov    edx,edi          ; edx = &out[node].rotation
004d5087:  8b c7                 mov    eax,edi           ; eax = &out[node].rotation
004d5089:  8d 4c 24 40           lea    ecx,[esp+0x40]      ; ecx = &new_rotation
004d508d:  e8 5e 8b ff ff        call   0x4cdbf0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
