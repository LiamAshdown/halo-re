// animation_overlay_interpolated_frame_orientations_weighted  (Ghidra: FUN_004d57d0, unnamed;
// renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d57d0, size 1052 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: animation_overlay_interpolated_frame_orientations.c's frame-corner interpolation combined with
//   animation_overlay_frame_orientations_weighted.c's caller weight: for rotation, the two
//   corner orientations are first lerped by the frame's own fractional part, then that result
//   is lerped a second time toward identity by `weight`, normalized, and multiplied onto the
//   existing rotation; translation adds the corner-interpolated value scaled by `weight`;
//   scale multiplies by (corner-interpolated value * weight + (1 - weight)). Ghidra's decompile
//   is structurally complete except for the frame-corner decode/lerp/normalize and the second
//   (identity) lerp/normalize/multiply, all pure register calls, resolved from
//   objdump -d -M intel --start-address=0x4d57d0 --stop-address=0x4d5c00 bin/halo.exe:
//     `mov edx,[weight_param]; push edx; lea edx,[&decoded-base]; ...; call 0x4cdcc0` for the
//     frame-corner lerp (a = &decoded-next, matching animation_overlay_interpolated_frame_orientations.c's
//     next/base assignment), then `call 0x4cdb20` normalizes it in place;
//     `mov eax,[weight_param2]; mov edx,[0x696738]; lea esi,[same buffer]; push eax;
//     mov ecx,esi; call 0x4cdcc0` is quaternion_lerp(a=&buffer, b=&global_identity_quaternion
//     (0x0065c274, indirected through the pointer table math.h documents), out=&buffer,
//     t=weight) -- same identity blend as
//     animation_overlay_frame_orientations_weighted.c -- then `call 0x4cdb20` normalizes again
//     and `mov eax,[&out_node]; mov edx,eax; call 0x4cdbf0` multiplies the twice-blended
//     rotation onto the existing out[node].rotation (a=edx=out=existing, matching every other
//     overlay function's pattern).
// CONFIRMED (review pass, 0x4d57f4..0x4d57fe: fld / fstp qword / call floor, no fabs):
//   unlike animation_overlay_interpolated_frame_orientations.c, the base-frame floor here is NOT
//   applied to |frame| -- `fld [frame]; sub esp,8; fstp [esp]; call 0x623e40` with no
//   intervening `fabs`, confirmed in the disassembly -- so a negative frame floors toward
//   negative infinity here instead of being treated as positive. As with the sibling function,
//   the rotation branch's compressed-codec call is given the base (floor'd) frame as a plain
//   integer while the translation/scale compressed-codec calls are given the fractional/
//   overflow-clamped `frame` directly; reproduced exactly, not "fixed".
// register convention: animation in EDI (unaff_EDI); frame, weight and the in/out array as the
//   recognized stack parameters (param_1, param_2, param_3).
//   // blam-cc: EDI -> animation, stack -> (frame, weight, out_orientations)

#include "tags.h"
#include "math.h"
#include "models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4
extern real_quaternion *global_identity_quaternion_pointer; // 0x00696738: indirect pointer to
                                                             // math.h's global_identity_quaternion
                                                             // (0x0065c274), per the pattern in
                                                             // src/units/biped_update.c

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

// Same fractional-frame interpolation as animation_overlay_interpolated_frame_orientations, additionally
// scaled by `weight`: the interpolated rotation is lerped a second time toward identity by
// weight before being multiplied onto the existing rotation, the interpolated translation is
// scaled by weight before being added, and the interpolated scale is scaled toward 1.0 by
// weight before being multiplied in. Does nothing if the animation is not type 1.
void animation_overlay_interpolated_frame_orientations_weighted(ModelAnimationsAnimation *animation,
                                                                  float frame, float weight,
                                                                  real_orientation *out_orientations)
{
    float one_minus_weight; // 1.0 - weight (the outer/caller weight)
    float frame_weight;     // fmod(frame, 1.0), the frame's own fractional part
    float base_frame_f;
    int16_t frame_count;
    int16_t base_frame;
    int16_t next_frame;
    int use_compressed_codec;
    uint8_t *base_cursor;
    uint8_t *next_cursor;
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

    one_minus_weight = 1.0f - weight;

    frame_weight = (float)fmod((double)frame, 1.0);
    base_frame_f = (float)floor((double)frame);
    base_frame = (int16_t)base_frame_f;
    frame_count = animation->frame_count;
    if (frame_count <= base_frame) {
        base_frame = frame_count - 1;
        base_frame_f = (float)base_frame;
        frame_weight = 1.0f;
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
                quaternion_lerp(&corner_next, &corner_base, &new_rotation, frame_weight);
                quaternion_normalize(&new_rotation);
            }
            quaternion_lerp(&new_rotation, global_identity_quaternion_pointer, &new_rotation, weight);
            quaternion_normalize(&new_rotation);
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
                new_translation.x = frame_weight * corner_next->x + (1.0f - frame_weight) * corner_base->x;
                new_translation.y = frame_weight * corner_next->y + (1.0f - frame_weight) * corner_base->y;
                new_translation.z = frame_weight * corner_next->z + (1.0f - frame_weight) * corner_base->z;
                base_cursor += sizeof(real_point3d);
                next_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x * weight + out_node->translation.x;
            out_node->translation.y = new_translation.y * weight + out_node->translation.y;
            out_node->translation.z = new_translation.z * weight + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_node_get_scale(animation, scale_index, frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                float base_scale = *(float *)base_cursor;
                float next_scale = *(float *)next_cursor;
                new_scale = frame_weight * next_scale + (1.0f - frame_weight) * base_scale;
                base_cursor += sizeof(float);
                next_cursor += sizeof(float);
            }
            out_node->scale = (new_scale * weight + one_minus_weight) * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d57d0):

void FUN_004d57d0(float param_1,float param_2,int param_3)

{
  short *psVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  bool bVar5;
  bool bVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  int extraout_EDX;
  ushort uVar10;
  uint uVar11;
  float *pfVar12;
  float *pfVar13;
  float *pfVar14;
  float *pfVar15;
  int unaff_EDI;
  float10 fVar16;
  float fStack_70;
  float *pfStack_68;
  int iStack_64;
  uint uStack_60;
  int iStack_5c;
  uint uStack_58;
  uint uStack_50;
  float fStack_4c;
  int iStack_48;
  int iStack_44;
  float local_40;
  float fStack_3c;
  float fStack_38;
  float fStack_34;
  undefined1 auStack_30 [16];
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  float fStack_4;

  local_40 = 1.0 - param_2;
  fVar16 = (float10)FUN_00628cca();
  fStack_70 = (float)fVar16;
  fVar16 = (float10)FUN_00623e40((double)param_1);
  fStack_4c = (float)fVar16;
  uStack_50 = (uint)ROUND(fStack_4c);
  sVar4 = *(short *)(unaff_EDI + 0x22);
  sVar7 = (short)uStack_50;
  if (sVar4 <= sVar7) {
    sVar7 = sVar4 + -1;
    fStack_4c = (float)(int)sVar7;
    fStack_70 = 1.0;
    param_1 = (float)(int)fStack_4c;
  }
  if (*(short *)(unaff_EDI + 0x20) == 1) {
    uVar10 = *(ushort *)(unaff_EDI + 0x3a) & 1;
    if ((uVar10 == 0) || ((DAT_006894b4 == '\0' && (*(int *)(unaff_EDI + 0x88) != 0)))) {
      bVar6 = false;
    }
    else {
      bVar6 = true;
    }
    iStack_48 = (int)sVar7;
    if (iStack_48 == sVar4 + -1) {
      sVar7 = 0;
    }
    else {
      sVar7 = sVar7 + 1;
    }
    if ((uVar10 == 0) || (DAT_006894b4 == '\0')) {
      bVar5 = false;
    }
    else {
      bVar5 = true;
    }
    if (bVar5) {
      iVar9 = *(int *)(unaff_EDI + 0x88);
    }
    else {
      iVar9 = *(short *)(unaff_EDI + 0x24) * iStack_48;
    }
    pfVar14 = (float *)(*(int *)(unaff_EDI + 0xac) + iVar9);
    if (((*(ushort *)(unaff_EDI + 0x3a) & 1) == 0) || (DAT_006894b4 == '\0')) {
      bVar5 = false;
    }
    else {
      bVar5 = true;
    }
    if (bVar5) {
      iVar9 = *(int *)(unaff_EDI + 0x88);
    }
    else {
      iVar9 = (int)*(short *)(unaff_EDI + 0x24) * (int)sVar7;
    }
    pfVar12 = (float *)(*(int *)(unaff_EDI + 0xac) + iVar9);
    uVar11 = 0;
    iStack_64 = 0;
    iStack_5c = 0;
    pfStack_68 = pfVar14;
    if (0 < *(short *)(unaff_EDI + 0x2c)) {
      do {
        iVar9 = (short)uVar11 * 0x20 + param_3;
        if ((uVar11 & 0x1f) == 0) {
          iVar8 = (int)(short)((short)uVar11 >> 5);
          uStack_58 = *(uint *)(unaff_EDI + 0x5c + iVar8 * 4);
          uStack_60 = *(uint *)(unaff_EDI + 0x6c + iVar8 * 4);
          uStack_50 = *(uint *)(unaff_EDI + 0x7c + iVar8 * 4);
        }
        iVar8 = iVar9;
        pfVar13 = pfVar12;
        pfVar15 = pfVar14;
        if ((uStack_60 & 1) != 0) {
          if (bVar6) {
            model_node_get_interpolated_rotation((float)iStack_48,iStack_64,uVar11,auStack_30);
            iStack_64 = iStack_64 + 1;
          }
          else {
            sVar7 = *(short *)pfVar12;
            fStack_10 = (float)(int)*(short *)pfVar14 * 3.051851e-05;
            iStack_44 = (int)*(short *)((int)pfVar12 + 6);
            fStack_c = (float)(int)*(short *)((int)pfVar14 + 2) * 3.051851e-05;
            psVar1 = (short *)((int)pfVar12 + 2);
            pfStack_68 = pfVar14 + 2;
            pfVar13 = pfVar12 + 1;
            fStack_8 = (float)(int)*(short *)(pfVar14 + 1) * 3.051851e-05;
            pfVar12 = pfVar12 + 2;
            fStack_4 = (float)(int)*(short *)((int)pfVar14 + 6) * 3.051851e-05;
            fStack_20 = (float)(int)sVar7 * 3.051851e-05;
            fStack_1c = (float)(int)*psVar1 * 3.051851e-05;
            fStack_18 = (float)(int)*(short *)pfVar13 * 3.051851e-05;
            fStack_14 = (float)iStack_44 * 3.051851e-05;
            quaternion_lerp(fStack_70);
            quaternion_normalize();
          }
          quaternion_lerp(param_2);
          quaternion_normalize();
          quaternion_multiply();
          iVar8 = extraout_EDX;
          pfVar13 = pfVar12;
          pfVar15 = pfStack_68;
        }
        uStack_60 = uStack_60 >> 1;
        pfVar12 = pfVar13;
        pfVar14 = pfVar15;
        if ((uStack_58 & 1) != 0) {
          if (bVar6) {
            model_node_get_interpolated_translation(param_1,iStack_5c,uVar11,&fStack_3c);
            iStack_5c = iStack_5c + 1;
            iVar8 = iVar9;
          }
          else {
            fVar2 = 1.0 - fStack_70;
            pfVar14 = pfVar15 + 3;
            pfVar12 = pfVar13 + 3;
            fStack_3c = fStack_70 * *pfVar13 + fVar2 * *pfVar15;
            fStack_38 = fStack_70 * pfVar13[1] + fVar2 * pfVar15[1];
            fStack_34 = fStack_70 * pfVar13[2] + fVar2 * pfVar15[2];
            pfStack_68 = pfVar14;
          }
          *(float *)(iVar8 + 0x10) = fStack_3c * param_2 + *(float *)(iVar8 + 0x10);
          *(float *)(iVar8 + 0x14) = fStack_38 * param_2 + *(float *)(iVar8 + 0x14);
          *(float *)(iVar8 + 0x18) = fStack_34 * param_2 + *(float *)(iVar8 + 0x18);
        }
        uStack_58 = uStack_58 >> 1;
        if ((uStack_50 & 1) != 0) {
          if (bVar6) {
            model_node_get_interpolated_scale(param_1,&fStack_4c);
            iVar8 = iVar9;
          }
          else {
            fVar2 = *pfVar14;
            pfVar14 = pfVar14 + 1;
            fVar3 = *pfVar12;
            pfVar12 = pfVar12 + 1;
            fStack_4c = fVar3 * fStack_70 + (1.0 - fStack_70) * fVar2;
            pfStack_68 = pfVar14;
          }
          *(float *)(iVar8 + 0x1c) = (fStack_4c * param_2 + local_40) * *(float *)(iVar8 + 0x1c);
        }
        uStack_50 = uStack_50 >> 1;
        uVar11 = uVar11 + 1;
      } while ((short)uVar11 < *(short *)(unaff_EDI + 0x2c));
    }
  }
  return;
}

objdump -d -M intel excerpt (the leading floor-without-fabs, and the rotation calls Ghidra
dropped, from --start-address=0x4d57d0 --stop-address=0x4d5c00):

004d57f4:  d9 44 24 78           fld    DWORD PTR [esp+0x78]   ; frame (no fabs before floor,
004d57fb:  dd 1c 24              fstp   QWORD PTR [esp]           ; unlike the sibling function)
004d57fe:  e8 3d e6 14 00        call   0x623e40                   ; floor(frame)

004d5a09:  8b 54 24 10           mov    edx,DWORD PTR [esp+0x10]  ; edx = frame_weight
004d5a0d:  52                    push   edx                         ; t
004d5a14:  8d 54 24 74           lea    edx,[esp+0x74]                ; b = &decoded-base
004d5a52:  e8 69 82 ff ff        call   0x4cdcc0                        ; quaternion_lerp
004d5a5c:  e8 bf 80 ff ff        call   0x4cdb20                          ; quaternion_normalize
004d5a61:  8b 84 24 88 00 00 00  mov    eax,DWORD PTR [esp+0x88]            ; eax = weight (outer)
004d5a68:  8b 15 38 67 69 00     mov    edx,DWORD PTR ds:0x696738              ; &global_identity_quaternion
004d5a6e:  8d 74 24 50           lea    esi,[esp+0x50]                          ; out (same buffer)
004d5a72:  50                    push   eax                                      ; t = weight
004d5a73:  8b ce                 mov    ecx,esi                                    ; a = same buffer
004d5a75:  e8 46 82 ff ff        call   0x4cdcc0                                    ; quaternion_lerp
004d5a7d:  e8 9e 80 ff ff        call   0x4cdb20                                      ; quaternion_normalize
004d5a82:  8b 44 24 14           mov    eax,DWORD PTR [esp+0x14]                        ; &out[node].rotation
004d5a86:  8b d0                 mov    edx,eax
004d5a88:  e8 63 81 ff ff        call   0x4cdbf0                                          ; quaternion_multiply
#endif
