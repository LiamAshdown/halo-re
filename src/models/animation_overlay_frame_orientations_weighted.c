// animation_overlay_frame_orientations_weighted  (Ghidra: FUN_004d51a0, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d51a0, size 583 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: same shape as animation_overlay_frame_orientations.c, weighted by a caller-supplied
//   factor. The two hidden-argument calls (quaternion_lerp, quaternion_multiply) were resolved
//   from objdump -d -M intel --start-address=0x4d51a0 --stop-address=0x4d53f0 bin/halo.exe:
//   `mov ecx,[esp+0x5c]; mov edx,[0x696738]; lea esi,[esp+0x44]; push ecx; mov ecx,esi;
//   call 0x4cdcc0` is quaternion_lerp(&new_rotation, *0x696738, &new_rotation, weight) -- the
//   .data pointer at 0x696738 holds 0x0065c274, math.h's global_identity_quaternion, read
//   indirectly through the shared constant-pointer table math.h documents. This blends the
//   frame's rotation toward identity by (1 - weight) before
//   `mov edx,ebp; mov eax,ebp; call 0x4cdbf0` multiplies the blended rotation onto the existing
//   out[node].rotation, same a/b/out pattern as the unweighted overlay. Translation instead
//   adds new*weight (x87 fmul weight, fadd existing), and scale multiplies by
//   (new*weight + (1-weight)) -- Ghidra's own decompile already had the translation/scale
//   arithmetic right, only the two quaternion calls needed filling in.
// register convention: animation in EDI (unaff_EDI); frame, weight and the in/out array as the
//   recognized stack parameters (param_1, param_2, param_3).
//   // blam-cc: EDI -> animation, stack -> (frame, weight, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"
#include "fn_math.h"
#include "fn_models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4
extern real_quaternion *global_identity_quaternion_pointer; // 0x00696738: indirect pointer to
                                                             // math.h's global_identity_quaternion
                                                             // (0x0065c274), per the pattern in
                                                             // src/units/biped_update.c


extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, float frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, float frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0
extern void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index,
                                      float frame, float *out); // 0x4d6e80


// Same blend as animation_overlay_frame_orientations, scaled by `weight`: the new rotation is
// first lerped toward identity by (1 - weight) before being multiplied onto the existing
// rotation, the new translation is added after scaling by weight, and the new scale is
// multiplied in after scaling toward 1.0 by weight. Does nothing if the animation is not type 1
// or frame is out of range.
void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int16_t frame,
                                                     float weight, real_orientation *out_orientations)
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
    float one_minus_weight;

    one_minus_weight = 1.0f - weight;

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
            quaternion_lerp(&new_rotation, global_identity_quaternion_pointer, &new_rotation, weight);
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
            out_node->translation.x = new_translation.x * weight + out_node->translation.x;
            out_node->translation.y = new_translation.y * weight + out_node->translation.y;
            out_node->translation.z = new_translation.z * weight + out_node->translation.z;
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
            out_node->scale = (new_scale * weight + one_minus_weight) * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d51a0):

void FUN_004d51a0(undefined4 param_1,float param_2,int param_3)

{
  bool bVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  short sVar5;
  int unaff_EDI;
  float *local_44;
  int local_40;
  uint local_3c;
  int local_38;
  uint local_34;
  uint local_2c;
  float local_28;
  int local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined1 local_10 [16];

  local_20 = 1.0 - param_2;
  if (*(short *)(unaff_EDI + 0x20) == 1) {
    uVar3 = 0;
    sVar5 = (short)param_1;
    if ((-1 < sVar5) && (sVar5 < *(short *)(unaff_EDI + 0x22))) {
      if (((*(byte *)(unaff_EDI + 0x3a) & 1) == 0) ||
         ((DAT_006894b4 == '\0' && (*(int *)(unaff_EDI + 0x88) != 0)))) {
        bVar1 = false;
      }
      else {
        bVar1 = true;
      }
      local_44 = (float *)FUN_004d4810(param_1);
      local_40 = 0;
      local_38 = 0;
      if (0 < *(short *)(unaff_EDI + 0x2c)) {
        do {
          iVar4 = (short)uVar3 * 0x20 + param_3;
          if ((uVar3 & 0x1f) == 0) {
            iVar2 = (int)(short)((short)uVar3 >> 5);
            local_34 = *(uint *)(unaff_EDI + 0x5c + iVar2 * 4);
            local_3c = *(uint *)(unaff_EDI + 0x6c + iVar2 * 4);
            local_2c = *(uint *)(unaff_EDI + 0x7c + iVar2 * 4);
          }
          if ((local_3c & 1) != 0) {
            if (bVar1) {
              local_24 = (int)sVar5;
              model_node_get_interpolated_rotation((float)local_24,local_40,uVar3,local_10);
              local_40 = local_40 + 1;
            }
            else {
              FUN_004d6330();
              local_44 = local_44 + 2;
            }
            quaternion_lerp(param_2);
            quaternion_multiply();
          }
          local_3c = local_3c >> 1;
          if ((local_34 & 1) != 0) {
            if (bVar1) {
              local_24 = (int)sVar5;
              model_node_get_interpolated_translation((float)local_24,local_38,uVar3,&local_1c);
              local_38 = local_38 + 1;
            }
            else {
              local_1c = *local_44;
              local_18 = local_44[1];
              local_14 = local_44[2];
              local_44 = local_44 + 3;
            }
            *(float *)(iVar4 + 0x10) = local_1c * param_2 + *(float *)(iVar4 + 0x10);
            *(float *)(iVar4 + 0x14) = local_18 * param_2 + *(float *)(iVar4 + 0x14);
            *(float *)(iVar4 + 0x18) = local_14 * param_2 + *(float *)(iVar4 + 0x18);
          }
          local_34 = local_34 >> 1;
          if ((local_2c & 1) != 0) {
            if (bVar1) {
              local_24 = (int)sVar5;
              model_node_get_interpolated_scale((float)local_24,&local_28);
            }
            else {
              local_28 = *local_44;
              local_44 = local_44 + 1;
            }
            *(float *)(iVar4 + 0x1c) = (local_28 * param_2 + local_20) * *(float *)(iVar4 + 0x1c);
          }
          local_2c = local_2c >> 1;
          uVar3 = uVar3 + 1;
        } while ((short)uVar3 < *(short *)(unaff_EDI + 0x2c));
      }
    }
  }
  return;
}

objdump -d -M intel excerpt (the quaternion_lerp / quaternion_multiply calls Ghidra dropped):

004d52a3:  8b 4c 24 5c           mov    ecx,DWORD PTR [esp+0x5c]  ; weight
004d52a7:  8b 15 38 67 69 00     mov    edx,DWORD PTR ds:0x696738   ; &global_identity_quaternion
004d52ad:  8d 74 24 44           lea    esi,[esp+0x44]               ; &new_rotation
004d52b1:  51                    push   ecx                            ; t = weight
004d52b2:  8b ce                 mov    ecx,esi                         ; a = &new_rotation
004d52b4:  e8 07 8a ff ff        call   0x4cdcc0                         ; quaternion_lerp(a,b,out=esi,t)
004d52bc:  8b d5                 mov    edx,ebp                          ; out = &out[node].rotation
004d52be:  8b c5                 mov    eax,ebp                           ; a = &out[node].rotation
004d52c0:  e8 2b 89 ff ff        call   0x4cdbf0                          ; quaternion_multiply(a, ecx=&new_rotation, out)
#endif
