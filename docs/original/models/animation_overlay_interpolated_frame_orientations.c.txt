// animation_overlay_interpolated_frame_orientations  (Ghidra: model_vertices_get_interpolated_frame,
// phase 2 name, wrong: it works on node orientations, not vertices. Renamed in the review pass
// to the name out/phase4/models_types_notes.md proposes, which also pairs it with its weighted
// sibling animation_overlay_interpolated_frame_orientations_weighted 0x4d57d0; see
// symbols/agent_phase4_models.txt. src/render/render_sky.c, src/units/unit_throw_grenade_release.c,
// src/units/unit_update_aiming_overlay_angles.c and the two src/objects/object_recalculate_
// bounding_radius*.c files still declare it under the old name.)
// address 0x4d53f0, size 967 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// review note: the original inlines 0x4d4810's pointer arithmetic for both frames rather than
//   calling it; calling animation_get_frame_data gives the same two pointers.
// evidence: this is animation_overlay_frame_orientations.c's blend (rotation multiplied,
//   translation added, scale multiplied onto the existing *out_orientations), but for a
//   fractional frame: it interpolates between the frame's two neighboring integer frames
//   (wrapping to 0 past the last frame) instead of taking a caller-supplied weight directly.
//   Ghidra's decompile is structurally complete except for the leading fmod/floor split (both
//   pure x87-stack calls with no visible arguments) and the uncompressed-rotation blend's three
//   quaternion calls, all resolved from
//   objdump -d -M intel --start-address=0x4d53f0 --stop-address=0x4d57d0 bin/halo.exe:
//     `fld [frame]; fld [0x672af8=1.0]; call 0x628cca (_CIfmod, see
//     src/math/periodic_function_evaluate.c)` gives weight = fmod(frame, 1.0) (signed, using
//     frame as-is, NOT frame's absolute value);
//     `fld [frame]; fabs; call 0x623e40 (floor, see
//     src/math/game_engine_accumulate_simulation_ticks.c)` gives base_frame = floor(|frame|).
//     If base_frame is >= frame_count it is clamped to frame_count-1, weight is forced to 1.0,
//     and `frame` itself is overwritten with (float)base_frame -- this clamped value is what
//     the compressed-codec translation/scale calls below then use.
//     `mov edx,ebp/eax; mov eax,ebp; lea ecx,[&new_rotation]; call 0x4cdbf0` (quaternion_
//     multiply) matches every other overlay function's a=existing/b=new/out=existing pattern.
//     The uncompressed-rotation branch inlines its int16x4 decode (no call to
//     animation_quaternion16_decode here, unlike the other overlay functions) but reads the
//     exact same six bytes at the exact same 1/32767 scale, so this rewrite calls the shared
//     decoder anyway -- same result, cleaner code. Comparing the fully Ghidra-resolved
//     translation blend (`weight * next_corner + (1 - weight) * base_corner`, added onto the
//     existing translation) against the quaternion_lerp call's operand order
//     (`lea ecx,[&decoded-next]; lea edx,[&decoded-base]`, matching quaternion_lerp's
//     out = t*a + (1-t)*b with a = ECX) fixes the same a/b assignment for rotation: lerp(next,
//     base, weight), then normalize, then multiply onto the existing rotation.
// CONFIRMED (review pass, fild [esp+0x34] at 0x4d5571 = the movsx base frame stored at
//   0x4d548e; the translation/scale calls push [esp+0x80], the frame argument): the rotation
//   branch's compressed-codec call (animation_node_get_rotation) is given
//   the base (floor'd) frame as a plain integer, not the fractional `frame` -- unlike the
//   translation and scale compressed-codec calls, which are given the (possibly overflow-
//   clamped) fractional `frame` directly. Confirmed against the disassembly (the rotation call
//   reads a stack slot written only by the floor'd base-frame conversion, the translation/scale
//   calls read the original argument slot), not a transcription slip -- reproduced exactly.
// register convention: animation in EDI (unaff_EDI); frame and the in/out array as the
//   recognized stack parameters (param_1, param_2).
//   // blam-cc: EDI -> animation, stack -> (frame, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t animation_compressed_data_enabled; // 0x006894b4

extern double fabs(double x);
extern double floor(double x); // 0x623e40, MSVC CRT, see src/game/game_engine_accumulate_simulation_ticks.c
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod: x in ST(1), y in ST(0),
                                        // see src/math/periodic_function_evaluate.c

extern void *animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame); // 0x4d4810, see animation_get_frame_data.c
extern void animation_quaternion16_decode(int16_t *source, real_quaternion *out); // 0x4d6330
extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, float frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, float frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0
extern void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index,
                                      float frame, float *out); // 0x4d6e80
extern void quaternion_multiply(real_quaternion *a, real_quaternion *b, real_quaternion *out); // 0x4cdbf0, verified in src/math
extern void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t); // 0x4cdcc0, verified in src/math
extern void quaternion_normalize(real_quaternion *q); // 0x4cdb20, verified in src/math

// Same overlay blend as animation_overlay_frame_orientations, but for a fractional `frame`:
// interpolates between the two integer frames on either side of it (the second wrapping to 0
// past the last frame) instead of taking a caller-supplied weight. Does nothing if the
// animation is not type 1.
void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
                                            real_orientation *out_orientations)
{
    float weight;         // fmod(frame, 1.0)
    float base_frame_f;
    int16_t frame_count;
    int16_t base_frame;
    int16_t next_frame;
    int use_compressed_codec;
    uint8_t *base_cursor;  // uncompressed frame data for base_frame
    uint8_t *next_cursor;  // uncompressed frame data for next_frame
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

    weight = (float)fmod((double)frame, 1.0);
    base_frame_f = (float)floor(fabs((double)frame));
    base_frame = (int16_t)base_frame_f;
    frame_count = animation->frame_count;
    if (frame_count <= base_frame) {
        base_frame = frame_count - 1;
        base_frame_f = (float)base_frame;
        weight = 1.0f;
        frame = base_frame_f;
    }

    if (animation->type != 1) {
        return;
    }

    use_compressed_codec = ((animation->flags & 1) != 0) &&
                           !((animation_compressed_data_enabled == 0) && (animation->offset_to_compressed_data != 0));

    next_frame = (base_frame == frame_count - 1) ? 0 : (int16_t)(base_frame + 1);

    base_cursor = (uint8_t *)animation_get_frame_data(animation, base_frame);
    next_cursor = (uint8_t *)animation_get_frame_data(animation, next_frame);

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
                animation_node_get_rotation(animation, (float)base_frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                real_quaternion corner_base;
                real_quaternion corner_next;
                animation_quaternion16_decode((int16_t *)base_cursor, &corner_base);
                base_cursor += 8;
                animation_quaternion16_decode((int16_t *)next_cursor, &corner_next);
                next_cursor += 8;
                quaternion_lerp(&corner_next, &corner_base, &new_rotation, weight);
                quaternion_normalize(&new_rotation);
            }
            quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_translation(animation, frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                real_point3d *corner_base = (real_point3d *)base_cursor;
                real_point3d *corner_next = (real_point3d *)next_cursor;
                new_translation.x = weight * corner_next->x + (1.0f - weight) * corner_base->x;
                new_translation.y = weight * corner_next->y + (1.0f - weight) * corner_base->y;
                new_translation.z = weight * corner_next->z + (1.0f - weight) * corner_base->z;
                base_cursor += sizeof(real_point3d);
                next_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x + out_node->translation.x;
            out_node->translation.y = new_translation.y + out_node->translation.y;
            out_node->translation.z = new_translation.z + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_scale(animation, scale_index, frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                float base_scale = *(float *)base_cursor;
                float next_scale = *(float *)next_cursor;
                new_scale = weight * next_scale + (1.0f - weight) * base_scale;
                base_cursor += sizeof(float);
                next_cursor += sizeof(float);
            }
            out_node->scale = new_scale * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d53f0):

void model_vertices_get_interpolated_frame(float param_1,int param_2)

{
  float fVar1;
  float fVar2;
  short sVar3;
  bool bVar4;
  bool bVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  int extraout_EDX;
  ushort uVar9;
  uint uVar10;
  float *pfVar11;
  float *pfVar12;
  float *pfVar13;
  float *pfVar14;
  int unaff_EDI;
  float10 fVar15;
  float local_6c;
  int local_64;
  uint local_60;
  int local_5c;
  uint local_58;
  uint local_50;
  float local_4c;
  int local_48;
  int local_44;
  float *local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined1 local_10 [16];

  fVar15 = (float10)FUN_00628cca();
  local_6c = (float)fVar15;
  fVar15 = (float10)FUN_00623e40((double)ABS(param_1));
  local_4c = (float)fVar15;
  local_50 = (uint)ROUND(local_4c);
  sVar3 = *(short *)(unaff_EDI + 0x22);
  sVar6 = (short)local_50;
  if (sVar3 <= sVar6) {
    sVar6 = sVar3 + -1;
    local_4c = (float)(int)sVar6;
    local_6c = 1.0;
    param_1 = (float)(int)local_4c;
  }
  if (*(short *)(unaff_EDI + 0x20) == 1) {
    uVar9 = *(ushort *)(unaff_EDI + 0x3a) & 1;
    if ((uVar9 == 0) || ((DAT_006894b4 == '\0' && (*(int *)(unaff_EDI + 0x88) != 0)))) {
      bVar5 = false;
    }
    else {
      bVar5 = true;
    }
    local_48 = (int)sVar6;
    if (local_48 == sVar3 + -1) {
      sVar6 = 0;
    }
    else {
      sVar6 = sVar6 + 1;
    }
    if ((uVar9 == 0) || (DAT_006894b4 == '\0')) {
      bVar4 = false;
    }
    else {
      bVar4 = true;
    }
    if (bVar4) {
      iVar8 = *(int *)(unaff_EDI + 0x88);
    }
    else {
      iVar8 = *(short *)(unaff_EDI + 0x24) * local_48;
    }
    pfVar13 = (float *)(*(int *)(unaff_EDI + 0xac) + iVar8);
    if (((*(ushort *)(unaff_EDI + 0x3a) & 1) == 0) || (DAT_006894b4 == '\0')) {
      bVar4 = false;
    }
    else {
      bVar4 = true;
    }
    if (bVar4) {
      iVar8 = *(int *)(unaff_EDI + 0x88);
    }
    else {
      iVar8 = (int)*(short *)(unaff_EDI + 0x24) * (int)sVar6;
    }
    pfVar11 = (float *)(*(int *)(unaff_EDI + 0xac) + iVar8);
    uVar10 = 0;
    local_64 = 0;
    local_5c = 0;
    if (0 < *(short *)(unaff_EDI + 0x2c)) {
      do {
        iVar8 = (short)uVar10 * 0x20 + param_2;
        if ((uVar10 & 0x1f) == 0) {
          iVar7 = (int)(short)((short)uVar10 >> 5);
          local_58 = *(uint *)(unaff_EDI + 0x5c + iVar7 * 4);
          local_60 = *(uint *)(unaff_EDI + 0x6c + iVar7 * 4);
          local_50 = *(uint *)(unaff_EDI + 0x7c + iVar7 * 4);
        }
        iVar7 = iVar8;
        pfVar12 = pfVar11;
        pfVar14 = pfVar13;
        if ((local_60 & 1) != 0) {
          if (bVar5) {
            model_node_get_interpolated_rotation((float)local_48,local_64,uVar10,local_10);
            local_64 = local_64 + 1;
          }
          else {
            local_20 = (float)(int)*(short *)pfVar13 * 3.051851e-05;
            local_40 = pfVar13 + 2;
            local_1c = (float)(int)*(short *)((int)pfVar13 + 2) * 3.051851e-05;
            pfVar12 = pfVar11 + 2;
            local_18 = (float)(int)*(short *)(pfVar13 + 1) * 3.051851e-05;
            local_44 = (int)*(short *)((int)pfVar11 + 6);
            local_14 = (float)(int)*(short *)((int)pfVar13 + 6) * 3.051851e-05;
            local_30 = (float)(int)*(short *)pfVar11 * 3.051851e-05;
            local_2c = (float)(int)*(short *)((int)pfVar11 + 2) * 3.051851e-05;
            local_28 = (float)(int)*(short *)(pfVar11 + 1) * 3.051851e-05;
            local_24 = (float)local_44 * 3.051851e-05;
            quaternion_lerp(local_6c);
            quaternion_normalize();
            pfVar14 = local_40;
          }
          quaternion_multiply();
          iVar7 = extraout_EDX;
        }
        local_60 = local_60 >> 1;
        pfVar11 = pfVar12;
        pfVar13 = pfVar14;
        if ((local_58 & 1) != 0) {
          if (bVar5) {
            model_node_get_interpolated_translation(param_1,local_5c,uVar10,&local_3c);
            local_5c = local_5c + 1;
            iVar7 = iVar8;
          }
          else {
            fVar1 = 1.0 - local_6c;
            pfVar13 = pfVar14 + 3;
            pfVar11 = pfVar12 + 3;
            local_3c = local_6c * *pfVar12 + fVar1 * *pfVar14;
            local_38 = local_6c * pfVar12[1] + fVar1 * pfVar14[1];
            local_34 = local_6c * pfVar12[2] + fVar1 * pfVar14[2];
          }
          *(float *)(iVar7 + 0x10) = local_3c + *(float *)(iVar7 + 0x10);
          *(float *)(iVar7 + 0x14) = local_38 + *(float *)(iVar7 + 0x14);
          *(float *)(iVar7 + 0x18) = local_34 + *(float *)(iVar7 + 0x18);
        }
        local_58 = local_58 >> 1;
        if ((local_50 & 1) != 0) {
          if (bVar5) {
            model_node_get_interpolated_scale(param_1,&local_4c);
            iVar7 = iVar8;
          }
          else {
            fVar1 = *pfVar13;
            pfVar13 = pfVar13 + 1;
            fVar2 = *pfVar11;
            pfVar11 = pfVar11 + 1;
            local_4c = fVar2 * local_6c + (1.0 - local_6c) * fVar1;
          }
          *(float *)(iVar7 + 0x1c) = local_4c * *(float *)(iVar7 + 0x1c);
        }
        local_50 = local_50 >> 1;
        uVar10 = uVar10 + 1;
      } while ((short)uVar10 < *(short *)(unaff_EDI + 0x2c));
    }
  }
  return;
}

objdump -d -M intel excerpt (the leading fmod/floor split and the quaternion calls Ghidra
dropped, from --start-address=0x4d53f0 --stop-address=0x4d57d0):

004d53f3:  d9 44 24 74           fld    DWORD PTR [esp+0x74]        ; frame
004d53f7:  dd 05 f8 2a 67 00     fld    QWORD PTR ds:0x672af8         ; 1.0
004d53fd:  e8 c8 38 15 00        call   0x628cca                       ; fmod(frame, 1.0)
004d5402:  d9 5c 24 04           fstp   DWORD PTR [esp+0x4]             ; weight
004d5406:  d9 44 24 74           fld    DWORD PTR [esp+0x74]              ; frame
004d540d:  d9 e1                 fabs
004d5412:  e8 29 ea 14 00        call   0x623e40                            ; floor(|frame|)

004d5646:  8d 4c 24 50           lea    ecx,[esp+0x50]      ; ecx = &decoded corner (next)
004d5619:  8b 54 24 10           mov    edx,DWORD PTR [esp+0x10]  ; edx = weight
004d561d:  52                    push   edx                          ; t
004d5624:  8d 54 24 60           lea    edx,[esp+0x60]        ; edx = &decoded corner (base)
004d5662:  e8 59 86 ff ff        call   0x4cdcc0                      ; quaternion_lerp(a=next,b=base,out,t)
004d566a:  8b ce                 mov    ecx,esi                          ; esi = out (from lerp)
004d566c:  e8 af 84 ff ff        call   0x4cdb20                          ; quaternion_normalize(out)
004d5679:  8b d0                 mov    edx,eax                              ; edx = &out[node].rotation
004d567b:  8d 4c 24 6c           lea    ecx,[esp+0x6c]                          ; ecx = lerped/normalized rotation
004d567f:  e8 6c 85 ff ff        call   0x4cdbf0                                  ; quaternion_multiply(a=edx,b=ecx,out=edx)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
