// animation_aiming_screen_blend  (Ghidra: model_vertices_bilinear_interpolate_2d_frame, wrong
// name; renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions"
// table -- this works on node orientations, not vertices)
// address 0x4d5c00, size 1660 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// note: the uncompressed stream is walked with the rotation and translation masks only; a
//   node with a scale bit set would desynchronize p00..p11, exactly as in the original.
// evidence: out/phase4/models_types_notes.md animation_aiming_screen section and register
//   table. VERIFIED against objdump -d -M intel bin/halo.exe (scratchpad/halo_disasm.txt,
//   0x4d5c00..0x4d627b):
//     0x4d5c84  fld [yaw] / fcomp 0.0 / ... -> ebx+0x04 (left_yaw_per_frame) when yaw > 0,
//               else ebx+0x00 (right_yaw_per_frame)
//     0x4d5c9f  fucompp against 0.0 / jp -> quotient = yaw / divisor, else quotient = 0.0
//               (same "s = (len==0) ? 0 : k/len" idiom as matrix4x3_from_quaternion)
//     0x4d5cca  call 0x6391b4 (__ftol, truncate) then call 0x628cca (_CIfmod, x=quotient
//               ST(1), y=1.0 ST(0)); if the fmod remainder is negative, decrement the
//               truncated value and add 1.0 to the remainder -- the standard truncate+fmod
//               spelling of floor()/frac(), also used by periodic_function_evaluate
//               (src/math/periodic_function_evaluate.c).
//     0x4d5d1a..0x4d5d3f  clamp the column to [-right_frame_count, left_frame_count-1], then
//               add right_frame_count to make it a 0-based column index.
//     0x4d5d38..0x4d5dea  the same sequence for pitch, with up/down swapped for left/right.
//     0x4d5dec..0x4d5e12  re-check 0 <= row < total_rows and 0 <= column < total_columns
//               (kept even though the clamp above already guarantees it, matching the
//               original -- a genuine no-op safety net, not an invented one).
//     0x4d5e12..0x4d5e63  four corner frame indices from (row, row+-or-same) x
//               (column, column+-or-same), each corner fetched through
//               animation_get_frame_data.
//     0x4d5ea2..0x4d6067  the node loop: refresh the rotation/translation bit masks every 32
//               nodes, decode/interpolate the 4 corner values per node, blend and write.
//   The rotation blend's three quaternion_lerp calls each drop their two quaternion pointer
//   arguments in Ghidra's decompilation (they are loaded through LEA immediately before the
//   call, which Ghidra's x87/struct-arg tracking does not surface), so only the interpolation
//   factor (fStack_e4 / fStack_dc, both spelled out by Ghidra) and the call order are used
//   here; the corner/weight assignment below is inferred by direct structural analogy with
//   the fully inline (and therefore fully verified) translation blend that follows it in the
//   same loop iteration. Review pass: the pointer arguments were read back from the
//   disassembly (see the note at the lerp calls) and agree with that layout.
// register convention: animation in EDI (unaff_EDI); animation_aiming_screen*, yaw, pitch and
//   the output real_orientation array as the recognized stack parameters, in that order.
//   // blam-cc: EDI -> animation, stack -> screen, yaw, pitch, orientation_out

#include "tags.h"
#include "math.h"
#include "models.h"
#include "fn_math.h"
#include "fn_models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4

extern int __ftol(double value); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod: x in ST(1), y in ST(0)


extern void animation_node_get_rotation(ModelAnimationsAnimation *animation, real frame,
                                         int16_t rotation_index, int16_t node, real_quaternion *out); // 0x4d6b60, this batch
extern void animation_node_get_translation(ModelAnimationsAnimation *animation, real frame,
                                            int16_t translation_index, int16_t node, real_point3d *out); // 0x4d6cf0, this batch


// Splits value/divisor into a floor()'d integer part and a [0,1) fractional part, using the
// truncate-then-fmod idiom the compiler generated inline at both call sites (yaw and pitch).
// divisor == 0.0f short-circuits to a quotient of 0.0f, matching the fucompp guard in the asm.
static void aiming_screen_frame_split(real value, real divisor, int32_t *out_frame, real *out_frac)
{
    real quotient;
    int32_t frame;
    real frac;

    quotient = (divisor == 0.0f) ? 0.0f : value / divisor;
    frame = __ftol((double)quotient);
    frac = (real)fmod((double)quotient, 1.0);
    if (frac < 0.0f) {
        frame = frame - 1;
        frac = frac + 1.0f;
    }
    *out_frame = frame;
    *out_frac = frac;
}

// Bilinearly blends the four corner keyframes of a 2D (yaw x pitch) aiming-overlay animation
// grid into orientation_out: rotation is composed onto the existing per-node rotation
// (quaternion multiply, like the other overlay samplers), translation is added onto the
// existing per-node translation. The animation must be an overlay (type 1) with enough frames
// for the full grid; out of range yaw/pitch, or too few frames, leaves orientation_out
// untouched.
void animation_aiming_screen_blend(ModelAnimationsAnimation *animation, animation_aiming_screen *screen,
                                    real yaw, real pitch, real_orientation *orientation_out)
{
    int16_t total_columns;
    int16_t total_rows;

    total_columns = (int16_t)(screen->left_frame_count + screen->right_frame_count + 1);
    total_rows = (int16_t)(screen->down_pitch_frame_count + screen->up_pitch_frame_count + 1);

    if (animation->type != 1) {
        return;
    }
    // movsx frame_count, then a 32 bit signed compare against the product (0x4d5c46..0x4d5c59)
    if ((int32_t)(int16_t)animation->frame_count < (int32_t)total_columns * (int32_t)total_rows) {
        return;
    }

    {
        int use_interpolated;
        int32_t yaw_frame, pitch_frame;
        real yaw_frac, pitch_frac;

        // Same "use the compressed/interpolated curve evaluators" test as the other samplers
        // (e.g. src/models/animation_get_frame_data.c): needs the compressed flag, and either
        // the module toggle is on or there is no separate compressed data to fall back to.
        use_interpolated = ((animation->flags & 1) != 0) &&
                            !((animation_compressed_data_enabled == 0) &&
                              (animation->offset_to_compressed_data != 0));

        aiming_screen_frame_split(yaw, (0.0f < yaw) ? screen->left_yaw_per_frame : screen->right_yaw_per_frame,
                                   &yaw_frame, &yaw_frac);
        if ((int16_t)screen->left_frame_count <= (int16_t)yaw_frame) {
            yaw_frame = (int16_t)screen->left_frame_count - 1;
            yaw_frac = 1.0f;
        }
        if ((int16_t)yaw_frame < -(int16_t)screen->right_frame_count) {
            yaw_frame = -(int16_t)screen->right_frame_count;
            yaw_frac = 0.0f;
        }
        yaw_frame = yaw_frame + screen->right_frame_count;

        aiming_screen_frame_split(pitch, (0.0f < pitch) ? screen->up_pitch_per_frame : screen->down_pitch_per_frame,
                                   &pitch_frame, &pitch_frac);
        if ((int16_t)screen->up_pitch_frame_count <= (int16_t)pitch_frame) {
            pitch_frame = (int16_t)screen->up_pitch_frame_count - 1;
            pitch_frac = 1.0f;
        }
        if ((int16_t)pitch_frame < -(int16_t)screen->down_pitch_frame_count) {
            pitch_frame = -(int16_t)screen->down_pitch_frame_count;
            pitch_frac = 0.0f;
        }
        pitch_frame = pitch_frame + screen->down_pitch_frame_count;

        // Re-checked in the original even though the clamps above already guarantee it -- see
        // the file header note.
        if (0 <= pitch_frame && pitch_frame < total_rows && 0 <= yaw_frame && yaw_frame < total_columns) {
            int16_t column0, column1, row0_index, row1_index;
            int16_t f00, f01, f10, f11;
            uint8_t *p00, *p01, *p10, *p11;
            int16_t rotation_index, translation_index;
            int16_t node;
            uint32_t rotation_mask, translation_mask;

            column0 = (int16_t)yaw_frame;
            column1 = (int16_t)(column0 + 1);
            if (column1 == total_columns) {
                column1 = column0;
            }
            row0_index = (int16_t)pitch_frame;
            row1_index = (int16_t)(row0_index + 1);
            if (row1_index == total_rows) {
                row1_index = row0_index;
            }

            f00 = (int16_t)(row0_index * total_columns + column0);
            f01 = (int16_t)(row0_index * total_columns + column1);
            f10 = (int16_t)(row1_index * total_columns + column0);
            f11 = (int16_t)(row1_index * total_columns + column1);

            p00 = (uint8_t *)animation_get_frame_data(animation, f00);
            p01 = (uint8_t *)animation_get_frame_data(animation, f01);
            p10 = (uint8_t *)animation_get_frame_data(animation, f10);
            p11 = (uint8_t *)animation_get_frame_data(animation, f11);

            rotation_index = 0;
            translation_index = 0;
            rotation_mask = 0;
            translation_mask = 0;

            for (node = 0; node < (int16_t)animation->node_count; node++) {
                real_orientation *out_node = &orientation_out[node];

                if ((node & 0x1f) == 0) {
                    int mask_word = node >> 5;
                    translation_mask = animation->node_transform_flag_data[mask_word];
                    rotation_mask = animation->node_rotation_flag_data[mask_word];
                }

                if ((rotation_mask & 1) != 0) {
                    real_quaternion q00, q01, q10, q11;
                    real_quaternion row0, row1, blended;

                    if (use_interpolated) {
                        animation_node_get_rotation(animation, (real)f00, rotation_index, node, &q00);
                        animation_node_get_rotation(animation, (real)f01, rotation_index, node, &q01);
                        animation_node_get_rotation(animation, (real)f10, rotation_index, node, &q10);
                        animation_node_get_rotation(animation, (real)f11, rotation_index, node, &q11);
                        rotation_index = rotation_index + 1;
                    } else {
                        animation_quaternion16_decode((int16_t *)p00, &q00); p00 += 8;
                        animation_quaternion16_decode((int16_t *)p01, &q01); p01 += 8;
                        animation_quaternion16_decode((int16_t *)p10, &q10); p10 += 8;
                        animation_quaternion16_decode((int16_t *)p11, &q11); p11 += 8;
                    }

                    // CONFIRMED (review pass, 0x4d5fe3..0x4d605e): ECX (the t weighted
                    // side) is q01 / q11 / row1 and EDX is q00 / q10 / row0, the results land
                    // in esp+0xc4 / 0xa4 / 0xe4, and quaternion_multiply 0x4cdbf0 runs with
                    // EAX = EDX = &out->rotation and ECX = the blended quaternion.
                    quaternion_lerp(&q01, &q00, &row0, yaw_frac);
                    quaternion_normalize(&row0);
                    quaternion_lerp(&q11, &q10, &row1, yaw_frac);
                    quaternion_normalize(&row1);
                    quaternion_lerp(&row1, &row0, &blended, pitch_frac);
                    quaternion_normalize(&blended);
                    quaternion_multiply(&out_node->rotation, &blended, &out_node->rotation);
                }
                rotation_mask = rotation_mask >> 1;

                if ((translation_mask & 1) != 0) {
                    real_point3d t00, t01, t10, t11;
                    real one_minus_yaw, one_minus_pitch;

                    if (use_interpolated) {
                        animation_node_get_translation(animation, (real)f00, translation_index, node, &t00);
                        animation_node_get_translation(animation, (real)f01, translation_index, node, &t01);
                        animation_node_get_translation(animation, (real)f10, translation_index, node, &t10);
                        animation_node_get_translation(animation, (real)f11, translation_index, node, &t11);
                        translation_index = translation_index + 1;
                    } else {
                        t00 = *(real_point3d *)p00; p00 += 12;
                        t01 = *(real_point3d *)p01; p01 += 12;
                        t10 = *(real_point3d *)p10; p10 += 12;
                        t11 = *(real_point3d *)p11; p11 += 12;
                    }

                    one_minus_yaw = 1.0f - yaw_frac;
                    one_minus_pitch = 1.0f - pitch_frac;
                    out_node->translation.x = (t01.x * yaw_frac + t00.x * one_minus_yaw) * one_minus_pitch +
                                               (t11.x * yaw_frac + t10.x * one_minus_yaw) * pitch_frac +
                                               out_node->translation.x;
                    out_node->translation.y = (t01.y * yaw_frac + t00.y * one_minus_yaw) * one_minus_pitch +
                                               (t11.y * yaw_frac + t10.y * one_minus_yaw) * pitch_frac +
                                               out_node->translation.y;
                    out_node->translation.z = (t11.z * yaw_frac + t10.z * one_minus_yaw) * pitch_frac +
                                               (t01.z * yaw_frac + t00.z * one_minus_yaw) * one_minus_pitch +
                                               out_node->translation.z;
                }
                translation_mask = translation_mask >> 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d5c00):

void model_vertices_bilinear_interpolate_2d_frame
               (int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  float fVar5;
  float fVar6;
  bool bVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  short sVar14;
  uint uVar15;
  short sVar16;
  int iVar17;
  short sVar18;
  int iVar19;
  int unaff_EDI;
  float10 fVar20;
  float fStack_e4;
  float *local_e0;
  float fStack_dc;
  int iStack_d8;
  uint uStack_cc;
  short local_c8;
  short local_c4;
  float *pfStack_c0;
  short local_bc;
  float *pfStack_b8;
  float *pfStack_b4;
  short local_b0;
  uint uStack_a4;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_94;
  float fStack_90;
  float fStack_8c;
  float fStack_88;
  float fStack_84;
  float fStack_80;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  undefined1 auStack_70 [16];
  undefined1 auStack_60 [32];
  undefined1 auStack_40 [32];
  undefined1 auStack_20 [32];

  uVar2 = *(ushort *)(param_1 + 0x14);
  iVar19 = *(ushort *)(param_1 + 10) + 1 + (uint)*(ushort *)(param_1 + 8);
  uVar3 = *(ushort *)(param_1 + 0x16);
  sVar8 = uVar2 + 1 + uVar3;
  if (*(short *)(unaff_EDI + 0x20) == 1) {
    sVar16 = (short)iVar19;
    if ((int)sVar16 * (int)sVar8 <= (int)*(short *)(unaff_EDI + 0x22)) {
      if (((*(byte *)(unaff_EDI + 0x3a) & 1) == 0) ||
         ((DAT_006894b4 == '\0' && (*(int *)(unaff_EDI + 0x88) != 0)))) {
        bVar7 = false;
      }
      else {
        bVar7 = true;
      }
      iVar9 = __ftol();
      fVar20 = (float10)FUN_00628cca();
      fStack_e4 = (float)fVar20;
      if (fVar20 < (float10)0.0) {
        iVar9 = iVar9 + -1;
        fStack_e4 = fStack_e4 + 1.0;
      }
      if ((short)*(ushort *)(param_1 + 10) <= (short)iVar9) {
        iVar9 = *(ushort *)(param_1 + 10) - 1;
        fStack_e4 = 1.0;
      }
      uVar4 = *(ushort *)(param_1 + 8);
      if ((int)(short)iVar9 < -(int)(short)uVar4) {
        iVar9 = -(uint)uVar4;
        fStack_e4 = 0.0;
      }
      iVar9 = iVar9 + (uint)uVar4;
      iVar10 = __ftol();
      fVar20 = (float10)FUN_00628cca();
      fStack_dc = (float)fVar20;
      if (fVar20 < (float10)0.0) {
        iVar10 = iVar10 + -1;
        fStack_dc = fStack_dc + 1.0;
      }
      if ((short)uVar3 <= (short)iVar10) {
        iVar10 = uVar3 - 1;
        fStack_dc = 1.0;
      }
      if ((int)(short)iVar10 < -(int)(short)uVar2) {
        iVar10 = -(uint)uVar2;
        fStack_dc = 0.0;
      }
      iVar10 = iVar10 + (uint)uVar2;
      sVar14 = (short)iVar10;
      if ((((-1 < sVar14) && (sVar14 < sVar8)) && (sVar18 = (short)iVar9, -1 < sVar18)) &&
         (sVar18 < sVar16)) {
        iVar13 = sVar18 + 1;
        if (iVar13 == sVar16) {
          iVar13 = (int)sVar18;
        }
        uStack_cc = (int)sVar14 + 1;
        if (uStack_cc == (int)sVar8) {
          uStack_cc = (uint)sVar14;
        }
        iVar10 = iVar10 * iVar19;
        iVar1 = iVar10 + iVar9;
        iVar10 = iVar10 + iVar13;
        iVar9 = iVar9 + uStack_cc * iVar19;
        iVar13 = uStack_cc * iVar19 + iVar13;
        pfStack_b8 = (float *)FUN_004d4810(iVar1);
        pfStack_b4 = (float *)FUN_004d4810(iVar10);
        pfStack_c0 = (float *)FUN_004d4810(iVar9);
        local_e0 = (float *)FUN_004d4810(iVar13);
        iVar19 = 0;
        uVar15 = 0;
        iVar17 = 0;
        iStack_d8 = 0;
        if (0 < *(short *)(unaff_EDI + 0x2c)) {
          do {
            iVar11 = (short)uVar15 * 0x20 + param_4;
            if ((uVar15 & 0x1f) == 0) {
              iVar12 = (int)(short)((short)uVar15 >> 5);
              uStack_cc = *(uint *)(unaff_EDI + 0x5c + iVar12 * 4);
              uStack_a4 = *(uint *)(unaff_EDI + 0x6c + iVar12 * 4);
            }
            local_c4 = (short)iVar1;
            local_c8 = (short)iVar10;
            local_b0 = (short)iVar9;
            local_bc = (short)iVar13;
            if ((uStack_a4 & 1) != 0) {
              if (bVar7) {
                model_node_get_interpolated_rotation((float)(int)local_c4,iVar19,uVar15,auStack_70);
                model_node_get_interpolated_rotation((float)(int)local_c8,iVar19,uVar15,auStack_20);
                model_node_get_interpolated_rotation((float)(int)local_b0,iVar19,uVar15,auStack_60);
                model_node_get_interpolated_rotation((float)(int)local_bc,iVar19,uVar15,auStack_40);
                iStack_d8 = iVar19 + 1;
              }
              else {
                FUN_004d6330();
                pfStack_b8 = pfStack_b8 + 2;
                FUN_004d6330();
                pfStack_b4 = pfStack_b4 + 2;
                FUN_004d6330();
                pfStack_c0 = pfStack_c0 + 2;
                FUN_004d6330();
                local_e0 = local_e0 + 2;
              }
              quaternion_lerp(fStack_e4);
              quaternion_normalize();
              quaternion_lerp(fStack_e4);
              quaternion_normalize();
              quaternion_lerp(fStack_dc);
              quaternion_normalize();
              quaternion_multiply();
              iVar19 = iStack_d8;
            }
            uStack_a4 = uStack_a4 >> 1;
            if ((uStack_cc & 1) != 0) {
              fVar5 = 1.0 - fStack_e4;
              fVar6 = 1.0 - fStack_dc;
              if (bVar7) {
                model_node_get_interpolated_translation
                          ((float)(int)local_c4,iVar17,uVar15,&fStack_a0);
                model_node_get_interpolated_translation
                          ((float)(int)local_c8,iVar17,uVar15,&fStack_88);
                model_node_get_interpolated_translation
                          ((float)(int)local_b0,iVar17,uVar15,&fStack_94);
                model_node_get_interpolated_translation
                          ((float)(int)local_bc,iVar17,uVar15,&fStack_7c);
                iVar17 = iVar17 + 1;
              }
              else {
                fStack_a0 = *pfStack_b8;
                fStack_9c = pfStack_b8[1];
                fStack_98 = pfStack_b8[2];
                pfStack_b8 = pfStack_b8 + 3;
                fStack_88 = *pfStack_b4;
                fStack_84 = pfStack_b4[1];
                fStack_80 = pfStack_b4[2];
                pfStack_b4 = pfStack_b4 + 3;
                fStack_94 = *pfStack_c0;
                fStack_90 = pfStack_c0[1];
                fStack_8c = pfStack_c0[2];
                pfStack_c0 = pfStack_c0 + 3;
                fStack_7c = *local_e0;
                fStack_78 = local_e0[1];
                fStack_74 = local_e0[2];
                local_e0 = local_e0 + 3;
              }
              *(float *)(iVar11 + 0x10) =
                   (fStack_88 * fStack_e4 + fStack_a0 * fVar5) * fVar6 +
                   (fStack_7c * fStack_e4 + fStack_94 * fVar5) * fStack_dc +
                   *(float *)(iVar11 + 0x10);
              *(float *)(iVar11 + 0x14) =
                   (fStack_84 * fStack_e4 + fStack_9c * fVar5) * fVar6 +
                   (fStack_78 * fStack_e4 + fStack_90 * fVar5) * fStack_dc +
                   *(float *)(iVar11 + 0x14);
              *(float *)(iVar11 + 0x18) =
                   (fStack_74 * fStack_e4 + fStack_8c * fVar5) * fStack_dc +
                   (fStack_80 * fStack_e4 + fStack_98 * fVar5) * fVar6 + *(float *)(iVar11 + 0x18);
            }
            uStack_cc = uStack_cc >> 1;
            uVar15 = uVar15 + 1;
          } while ((short)uVar15 < *(short *)(unaff_EDI + 0x2c));
        }
      }
    }
  }
  return;
}
#endif
