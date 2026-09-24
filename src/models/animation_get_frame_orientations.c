// animation_get_frame_orientations  (Ghidra: FUN_004d4a80, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d4a80, size 847 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: Ghidra's own decompilation is structurally correct for every branch except the two
//   calls in the "rotation default, compressed" sub-branch, whose arguments it dropped
//   entirely (they are pure register calls). Filled in from
//   objdump -d -M intel --start-address=0x4d4a80 --stop-address=0x4d4dd0 bin/halo.exe: the
//   source is frame_cursor + header->rotation_defaults + node*sizeof(animation_quaternion48)
//   (ECX), the destination is &out[node].rotation (ESI), matching
//   animation_quaternion48_decode's register convention and animation_compressed_header's
//   "defaults indexed by node" note. The same disassembly also recovered a fourth loop-carried
//   index (the running "scale index", DX in animation_node_get_scale's register convention)
//   that Ghidra's pseudocode never surfaced as a variable at all -- it lives entirely in a
//   register/stack slot Ghidra folded away, initialized to 0 alongside the rotation/translation
//   indices and incremented after every animated-scale call, exactly like those two.
// QUIRK (confirmed in the review pass, 0x4d4abd..0x4d4b15: [esp+0xb] vs al): use_compressed_frame_base (frame_cursor's identity: compressed header base vs.
//   uncompressed frame address) and use_compressed_codec (which per-component code path runs)
//   are computed independently and are NOT always equal: when
//   animation_compressed_data_enabled == 0 and offset_to_compressed_data == 0,
//   use_compressed_codec is true (nothing blocks it) while use_compressed_frame_base is false
//   (compressed_data_enabled is required for it), so frame_cursor would hold the uncompressed
//   frame address while the compressed-default code reads it as a header. Reproduced exactly
//   as the original computes it, not "fixed".
// register convention: model in EAX (in_EAX), animation in EDI (unaff_EDI); frame and the
//   output array as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> model, EDI -> animation, stack -> (frame, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4

extern void quaternion_normalize(real_quaternion *q); // 0x4cdb20, verified in src/math
extern void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out); // 0x4d6380
extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, float frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, float frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0
extern void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index,
                                      float frame, float *out); // 0x4d6e80
extern void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out); // 0x4d7610

// Samples one animation frame into an array of k_maximum_nodes_per_model real_orientation
// entries: falls back to the model's bind pose (model_nodes_get_default_transforms) unless the
// animation is a base animation (type 0) whose node list matches the model, then for every node
// reads a rotation/translation/scale each from either the compressed codec, the uncompressed
// per-frame stream, or the matching defaults stream, according to that node's three bit masks.
void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model,
                                       int16_t frame, real_orientation *out_orientations)
{
    uint8_t *frame_cursor;  // ebp: compressed header base, or the uncompressed frame address
    uint8_t *default_cursor; // local_20: uncompressed default-data cursor
    int use_compressed_frame_base; // bVar3
    int use_compressed_codec;      // bVar4
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;

    if (animation->type != 0 ||
        (model != 0 &&
         (((animation->node_list_checksum != 0 && animation->node_list_checksum != model->node_list_checksum) &&
           model->node_list_checksum != 0) ||
          model->nodes.count != (int32_t)(int16_t)animation->node_count))) { // movsx +0x2c
        model_nodes_get_default_transforms(model, out_orientations);
        return;
    }

    use_compressed_codec = ((animation->flags & 1) != 0) &&
                           !((animation_compressed_data_enabled == 0) && (animation->offset_to_compressed_data != 0));
    use_compressed_frame_base = ((animation->flags & 1) != 0) && (animation_compressed_data_enabled != 0);

    if (use_compressed_frame_base) {
        frame_cursor = (uint8_t *)animation->frame_data.pointer + animation->offset_to_compressed_data;
    } else {
        frame_cursor = (uint8_t *)animation->frame_data.pointer + (int32_t)(int16_t)animation->frame_size * (int32_t)frame; // movsx both
    }

    default_cursor = (uint8_t *)animation->default_data.pointer;
    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = animation->node_count;
    if (node_count < 1) {
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

        // rotation
        if ((rotation_mask & 1) == 0) {
            if (!use_compressed_codec) {
                int16_t *src = (int16_t *)default_cursor;
                out_node->rotation.i = (float)src[0] * 3.051851e-05f;
                out_node->rotation.j = (float)src[1] * 3.051851e-05f;
                out_node->rotation.k = (float)src[2] * 3.051851e-05f;
                out_node->rotation.w = (float)src[3] * 3.051851e-05f;
                default_cursor += 8;
            } else {
                animation_compressed_header *header = (animation_compressed_header *)frame_cursor;
                animation_quaternion48 *def = (animation_quaternion48 *)(frame_cursor + header->rotation_defaults +
                                                                          (int)node * (int)sizeof(animation_quaternion48));
                animation_quaternion48_decode(def, &out_node->rotation);
                quaternion_normalize(&out_node->rotation);
            }
        } else if (use_compressed_codec) {
            animation_node_get_rotation(animation, (float)frame, rotation_index, node, &out_node->rotation);
            rotation_index = rotation_index + 1;
        } else {
            int16_t *src = (int16_t *)frame_cursor;
            out_node->rotation.i = (float)src[0] * 3.051851e-05f;
            out_node->rotation.j = (float)src[1] * 3.051851e-05f;
            out_node->rotation.k = (float)src[2] * 3.051851e-05f;
            out_node->rotation.w = (float)src[3] * 3.051851e-05f;
            frame_cursor += 8;
        }
        rotation_mask = rotation_mask >> 1;

        // translation
        if ((translation_mask & 1) == 0) {
            if (use_compressed_codec) {
                animation_compressed_header *header = (animation_compressed_header *)frame_cursor;
                real_point3d *def = (real_point3d *)(frame_cursor + header->translation_defaults +
                                                       (int)node * (int)sizeof(real_point3d));
                out_node->translation = *def;
            } else {
                real_point3d *src = (real_point3d *)default_cursor;
                out_node->translation = *src;
                default_cursor += sizeof(real_point3d);
            }
        } else if (use_compressed_codec) {
            animation_node_get_translation(animation, (float)frame, translation_index, node, &out_node->translation);
            translation_index = translation_index + 1;
        } else {
            real_point3d *src = (real_point3d *)frame_cursor;
            out_node->translation = *src;
            frame_cursor += sizeof(real_point3d);
        }
        translation_mask = translation_mask >> 1;

        // scale
        if ((scale_mask & 1) == 0) {
            if (use_compressed_codec) {
                out_node->scale = 1.0f;
            } else {
                out_node->scale = *(float *)default_cursor;
                default_cursor += sizeof(float);
            }
        } else if (use_compressed_codec) {
            animation_node_get_scale(animation, scale_index, (float)frame, &out_node->scale);
            scale_index = scale_index + 1;
        } else {
            out_node->scale = *(float *)frame_cursor;
            frame_cursor += sizeof(float);
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d4a80):

void FUN_004d4a80(short param_1,int param_2)

{
  short sVar1;
  float fVar2;
  bool bVar3;
  bool bVar4;
  ushort uVar5;
  int in_EAX;
  int iVar6;
  float *pfVar7;
  short sVar8;
  uint uVar9;
  float *pfVar10;
  float *pfVar11;
  int unaff_EDI;
  float *local_20;
  int local_1c;
  uint local_18;
  int local_14;
  uint local_10;
  uint local_c;
  int local_4;

  uVar9 = 0;
  if ((*(short *)(unaff_EDI + 0x20) != 0) ||
     ((in_EAX != 0 &&
      ((((*(int *)(unaff_EDI + 0x28) != 0 && (*(int *)(unaff_EDI + 0x28) != *(int *)(in_EAX + 4)))
        && (*(int *)(in_EAX + 4) != 0)) ||
       (*(int *)(in_EAX + 0xb8) != (int)*(short *)(unaff_EDI + 0x2c))))))) {
    model_nodes_get_default_transforms(param_2);
    return;
  }
  uVar5 = *(ushort *)(unaff_EDI + 0x3a) & 1;
  if ((uVar5 == 0) || ((DAT_006894b4 == '\0' && (*(int *)(unaff_EDI + 0x88) != 0)))) {
    bVar4 = false;
  }
  else {
    bVar4 = true;
  }
  if ((uVar5 == 0) || (DAT_006894b4 == '\0')) {
    bVar3 = false;
  }
  else {
    bVar3 = true;
  }
  if (bVar3) {
    iVar6 = *(int *)(unaff_EDI + 0x88);
  }
  else {
    iVar6 = (int)*(short *)(unaff_EDI + 0x24) * (int)param_1;
  }
  pfVar10 = (float *)(*(int *)(unaff_EDI + 0xac) + iVar6);
  local_20 = *(float **)(unaff_EDI + 0x98);
  local_1c = 0;
  local_14 = 0;
  if (*(short *)(unaff_EDI + 0x2c) < 1) {
    return;
  }
  do {
    sVar8 = (short)uVar9;
    pfVar11 = (float *)(sVar8 * 0x20 + param_2);
    if ((uVar9 & 0x1f) == 0) {
      iVar6 = (int)(sVar8 >> 5);
      local_10 = *(uint *)(unaff_EDI + 0x5c + iVar6 * 4);
      local_18 = *(uint *)(unaff_EDI + 0x6c + iVar6 * 4);
      local_c = *(uint *)(unaff_EDI + 0x7c + iVar6 * 4);
    }
    if ((local_18 & 1) == 0) {
      if (!bVar4) {
        *pfVar11 = (float)(int)*(short *)local_20 * 3.051851e-05;
        pfVar11[1] = (float)(int)*(short *)((int)local_20 + 2) * 3.051851e-05;
        pfVar11[2] = (float)(int)*(short *)(local_20 + 1) * 3.051851e-05;
        sVar1 = *(short *)((int)local_20 + 6);
        local_20 = local_20 + 2;
        goto LAB_004d4c84;
      }
      model_vertex_unpack_compressed_normal();
      quaternion_normalize();
    }
    else if (bVar4) {
      model_node_get_interpolated_rotation((float)(int)param_1,local_1c,uVar9,pfVar11);
      local_1c = local_1c + 1;
    }
    else {
      *pfVar11 = (float)(int)*(short *)pfVar10 * 3.051851e-05;
      pfVar11[1] = (float)(int)*(short *)((int)pfVar10 + 2) * 3.051851e-05;
      pfVar11[2] = (float)(int)*(short *)(pfVar10 + 1) * 3.051851e-05;
      sVar1 = *(short *)((int)pfVar10 + 6);
      pfVar10 = pfVar10 + 2;
LAB_004d4c84:
      local_4 = (int)sVar1;
      pfVar11[3] = (float)local_4 * 3.051851e-05;
    }
    local_18 = local_18 >> 1;
    if ((local_10 & 1) == 0) {
      if (bVar4) {
        pfVar7 = (float *)((int)pfVar10[5] + sVar8 * 0xc + (int)pfVar10);
        pfVar11[4] = *pfVar7;
        pfVar11[5] = pfVar7[1];
        pfVar11[6] = pfVar7[2];
      }
      else {
        pfVar11[4] = *local_20;
        pfVar11[5] = local_20[1];
        pfVar11[6] = local_20[2];
        local_20 = local_20 + 3;
      }
    }
    else if (bVar4) {
      model_node_get_interpolated_translation((float)(int)param_1,local_14,uVar9,pfVar11 + 4);
      local_14 = local_14 + 1;
    }
    else {
      pfVar11[4] = *pfVar10;
      pfVar11[5] = pfVar10[1];
      pfVar11[6] = pfVar10[2];
      pfVar10 = pfVar10 + 3;
    }
    local_10 = local_10 >> 1;
    if ((local_c & 1) == 0) {
      if (bVar4) {
        pfVar11[7] = 1.0;
      }
      else {
        fVar2 = *local_20;
        local_20 = local_20 + 1;
        pfVar11[7] = fVar2;
      }
    }
    else if (bVar4) {
      model_node_get_interpolated_scale((float)(int)param_1,pfVar11 + 7);
    }
    else {
      pfVar11[7] = *pfVar10;
      pfVar10 = pfVar10 + 1;
    }
    local_c = local_c >> 1;
    uVar9 = uVar9 + 1;
    if (*(short *)(unaff_EDI + 0x2c) <= (short)uVar9) {
      return;
    }
  } while( true );
}

objdump -d -M intel --start-address=0x4d4a80 --stop-address=0x4d4dd0 bin/halo.exe (key excerpt,
the compressed rotation-default call site the decompiler above dropped):

004d4c17:  8d 04 49              lea    eax,[ecx+ecx*2]        ; eax = node*3 (ecx still holds
                                                                ; the node index from 004d4b44)
004d4c1a:  8b 4d 04              mov    ecx,DWORD PTR [ebp+0x4]  ; ecx = header->rotation_defaults
004d4c1d:  8d 0c 41              lea    ecx,[ecx+eax*2]         ; ecx += node*6
004d4c20:  03 cd                 add    ecx,ebp                  ; ecx = header base + offset + node*6
004d4c22:  e8 59 17 00 00        call   0x4d6380                 ; animation_quaternion48_decode(ecx, esi)
004d4c27:  8b ce                 mov    ecx,esi
004d4c29:  e8 f2 8e ff ff        call   0x4cdb20                 ; quaternion_normalize(ecx)

(the running scale index, invisible in the decompiled C above, at the animation_node_get_scale
call site):

004d4d51:  89 4c 24 2c           mov    DWORD PTR [esp+0x2c],ecx  ; relocate frame (int) across the push below
004d4d55:  83 c6 1c              add    esi,0x1c                   ; esi = &out[node].scale
004d4d58:  56                    push   esi
004d4d59:  db 44 24 30           fild   DWORD PTR [esp+0x30]        ; frame (int) -> float
004d4d5d:  8b 74 24 2c           mov    esi,DWORD PTR [esp+0x2c]     ; esi = scale_index (the push
                                                                     ; above shifted this slot from
                                                                     ; the [esp+0x28]=0 initializer)
004d4d61:  51                    push   ecx
004d4d62:  8b d6                 mov    edx,esi                      ; edx (DX) = scale_index
004d4d64:  8b cf                 mov    ecx,edi                       ; ecx = animation
004d4d66:  d9 1c 24              fstp   DWORD PTR [esp]                ; frame as float, arg 0
004d4d69:  e8 12 21 00 00        call   0x4d6e80
004d4d6e:  83 c4 08              add    esp,0x8
004d4d71:  46                    inc    esi                             ; scale_index++
004d4d72:  89 74 24 28           mov    DWORD PTR [esp+0x28],esi
#endif
