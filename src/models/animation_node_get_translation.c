// animation_node_get_translation  (Ghidra: model_node_get_interpolated_translation, wrong
// name; renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions"
// table -- this evaluates the compressed animation codec's per-node translation curve)
// address 0x4d6cf0, size 390 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md animation_compressed_header section. Same
//   three-way bracketing scheme as animation_node_get_rotation (0x4d6b60) -- below the first
//   real keyframe interpolates from the node's default at time 0, at-or-past the last real
//   keyframe interpolates towards the default at a virtual time one past it, otherwise
//   animation_keyframe_time_search finds the pair -- but Ghidra keeps every argument here
//   (translation is plain floats, not an opaque struct return), so this one needed no objdump
//   cross-check beyond confirming FUN_004d6b10's call (dropped args, matching the shared
//   EAX=count/BX=frame/stack=times convention already established for it).
// register convention: animation in ECX (in_ECX); frame, translation index, node and the
//   output real_point3d pointer as the recognized stack parameters, in that order.
//   // blam-cc: ECX -> animation, stack -> frame, translation_index, node, out

#include "tags.h"
#include "math.h"
#include "models.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double floor(double x); // 0x623e40, MSVC CRT, see src/models/animation_overlay_interpolated_frame_orientations.c
extern int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame); // 0x4d6b10, this batch

// Evaluates one node's compressed translation curve at a (possibly fractional) frame: finds
// the two bracketing keyframes (or the node's default, used as an implicit keyframe at time 0
// before the first real one and again, symmetrically, one frame past the last real one) and
// lerps between them, or returns the matching keyframe/default directly when frame lands
// exactly on it.
void animation_node_get_translation(ModelAnimationsAnimation *animation, real frame,
                                     int16_t translation_index, int16_t node, real_point3d *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    real_point3d *defaults_node;

    header_base = (uint8_t *)animation->frame_data.pointer + animation->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + header->translation_keyframe_headers))[translation_index];
    defaults_node = (real_point3d *)(header_base + header->translation_defaults) + node;
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);

    if (count == 0) {
        *out = *defaults_node;
        return;
    }

    {
        int32_t first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift); // movsx ecx,dx at 0x4d6d4f: low 16 bits, signed
        uint16_t *times = (uint16_t *)(header_base + header->translation_keyframe_times) + first_index;
        real_point3d *keyframes = (real_point3d *)(header_base + header->translation_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        real_point3d *source_a;
        real_point3d *source_b;
        int16_t time_a, time_b;

        // both compares are movsx rounded vs movzx time, i.e. the times are unsigned here
        // (unlike the signed compares inside 0x4d6b10)
        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            source_a = defaults_node;
            time_b = (int16_t)times[0];
            source_b = &keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            source_a = &keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            source_b = defaults_node;
        } else {
            int16_t index = animation_keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            source_a = &keyframes[index];
            source_b = &keyframes[index + 1];
        }

        if (frame == (real)time_a) {
            *out = *source_a;
        } else {
            real t = (frame - (real)time_a) / (real)(time_b - time_a);
            real one_minus_t = 1.0f - t;

            out->x = t * source_b->x + one_minus_t * source_a->x;
            out->y = t * source_b->y + one_minus_t * source_a->y;
            out->z = t * source_b->z + one_minus_t * source_a->z;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d6cf0):

void model_node_get_interpolated_translation
               (float param_1,short param_2,short param_3,float *param_4)

{
  uint uVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  int iVar5;
  int in_ECX;
  int iVar6;
  float *pfVar7;
  ushort uVar8;
  int iVar9;
  ushort uVar10;
  ushort *puVar11;
  float *pfVar12;
  float10 fVar13;

  iVar5 = *(int *)(in_ECX + 0xac) + *(int *)(in_ECX + 0x88);
  uVar1 = *(uint *)(*(int *)(iVar5 + 0xc) + param_2 * 4 + iVar5);
  iVar9 = *(int *)(iVar5 + 0x14) + iVar5;
  if ((uVar1 & 0xfff) == 0) {
    pfVar12 = (float *)(iVar9 + param_3 * 0xc);
    *param_4 = *pfVar12;
    param_4[1] = pfVar12[1];
    param_4[2] = pfVar12[2];
    return;
  }
  iVar6 = (int)(short)(uVar1 >> 0xc);
  pfVar12 = (float *)(*(int *)(iVar5 + 0x18) + iVar6 * 0xc + iVar5);
  puVar11 = (ushort *)(*(int *)(iVar5 + 0x10) + iVar6 * 2 + iVar5);
  fVar13 = (float10)FUN_00623e40((double)param_1);
  uVar10 = *puVar11;
  param_2 = (short)(int)ROUND((float)fVar13);
  if ((int)param_2 < (int)(uint)uVar10) {
    uVar8 = 0;
    pfVar7 = (float *)(iVar9 + param_3 * 0xc);
  }
  else {
    iVar5 = (int)(short)((ushort)uVar1 & 0xfff);
    uVar8 = puVar11[iVar5 + -1];
    if ((int)param_2 == (uint)uVar8) {
      pfVar7 = pfVar12 + iVar5 * 3 + -3;
      uVar10 = uVar8 + 1;
      pfVar12 = (float *)(iVar9 + param_3 * 0xc);
    }
    else {
      sVar4 = FUN_004d6b10();
      iVar5 = (int)sVar4;
      uVar8 = puVar11[iVar5];
      uVar10 = puVar11[iVar5 + 1];
      pfVar7 = pfVar12 + iVar5 * 3;
      pfVar12 = pfVar7 + 3;
    }
  }
  fVar2 = (float)(int)(short)uVar8;
  if (param_1 == fVar2) {
    *param_4 = *pfVar7;
    param_4[1] = pfVar7[1];
    param_4[2] = pfVar7[2];
    return;
  }
  fVar2 = (param_1 - fVar2) / (float)((int)(short)uVar10 - (int)(short)uVar8);
  fVar3 = 1.0 - fVar2;
  *param_4 = fVar2 * *pfVar12 + fVar3 * *pfVar7;
  param_4[1] = fVar2 * pfVar12[1] + fVar3 * pfVar7[1];
  param_4[2] = fVar2 * pfVar12[2] + fVar3 * pfVar7[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
