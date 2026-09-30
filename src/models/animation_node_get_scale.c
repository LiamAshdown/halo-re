// animation_node_get_scale  (Ghidra: model_node_get_interpolated_scale, wrong name; renamed
// per out/phase4/models_types_notes.md "Misnamed or misattributed functions" table -- this
// evaluates the compressed animation codec's per-node scale curve)
// address 0x4d6e80, size 320 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md animation_compressed_header section, in
//   particular the recorded quirk: "0x4d6e80 indexes both the scale header array and the
//   scale default array with the same DX value (the running scale index from its callers)."
//   Confirmed directly in the Ghidra output: unlike rotation/translation (which index their
//   defaults by node), there is no node parameter here at all -- both `*(int*)(header +
//   scale_keyframe_headers) + scale_index*4` and `*(float*)(header + scale_defaults) +
//   scale_index*4` use the same scale_index. Reproduced as-is, not "fixed": see
//   types/models.h's own note that whether this is a bug or means the scale defaults are
//   stored per animated node is unresolved.
// register convention: animation in ECX (in_ECX), scale index in DX (in_DX); frame and the
//   output float pointer as the recognized stack parameters.
//   // blam-cc: ECX -> animation, DX -> scale_index, stack -> frame, out

#include "tags.h"
#include "math.h"
#include "models.h"
#include "fn_models.h"

extern double floor(double x); // 0x623e40, MSVC CRT, see src/models/animation_overlay_interpolated_frame_orientations.c


// Evaluates one animated node's compressed scale curve at a (possibly fractional) frame. See
// animation_node_get_rotation/_translation for the shared three-way bracketing scheme; the one
// difference here is that scale_index, not a node index, selects the default value on both
// sides of the "before the first keyframe" / "exactly at the last keyframe" cases.
void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index, real frame, real *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    real *default_value; // read only where the original reads it (0x4d6ebd, 0x4d6f10, 0x4d6f3d): never on the
                         // keyframe-search path, where it may point at unmapped memory

    header_base = (uint8_t *)animation->frame_data.pointer + animation->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + header->scale_keyframe_headers))[scale_index];
    default_value = &((real *)(header_base + header->scale_defaults))[scale_index];
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);

    if (count == 0) {
        *out = *default_value;
        return;
    }

    {
        int32_t first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift); // movsx ecx,dx at 0x4d6ed1: low 16 bits, signed
        uint16_t *times = (uint16_t *)(header_base + header->scale_keyframe_times) + first_index;
        real *keyframes = (real *)(header_base + header->scale_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        real value_a, value_b;
        int16_t time_a, time_b;

        // both compares are movsx rounded vs movzx time, i.e. the times are unsigned here
        // (unlike the signed compares inside 0x4d6b10)
        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            value_a = *default_value;
            time_b = (int16_t)times[0];
            value_b = keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            value_a = keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            value_b = *default_value;
        } else {
            int16_t index = animation_keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            value_a = keyframes[index];
            value_b = keyframes[index + 1];
        }

        if (frame == (real)time_a) {
            *out = value_a;
        } else {
            real t = (frame - (real)time_a) / (real)(time_b - time_a);
            *out = t * value_b + (1.0f - t) * value_a;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d6e80):

void model_node_get_interpolated_scale(float param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  uint uVar3;
  float fVar4;
  short sVar5;
  int iVar6;
  ushort uVar7;
  int in_ECX;
  int iVar8;
  short in_DX;
  int iVar9;
  int iVar10;
  ushort uVar11;
  ushort *puVar12;
  float *pfVar13;
  float10 fVar14;
  short local_c;

  iVar10 = *(int *)(in_ECX + 0xac);
  iVar8 = *(int *)(in_ECX + 0x88);
  iVar6 = iVar10 + iVar8;
  iVar9 = in_DX * 4;
  uVar3 = *(uint *)(*(int *)(iVar10 + 0x1c + iVar8) + iVar9 + iVar6);
  iVar10 = *(int *)(iVar10 + 0x24 + iVar8) + iVar6;
  if ((uVar3 & 0xfff) == 0) {
    *param_2 = *(float *)(iVar9 + iVar10);
    return;
  }
  iVar8 = (int)(short)(uVar3 >> 0xc);
  pfVar13 = (float *)(*(int *)(iVar6 + 0x28) + iVar8 * 4 + iVar6);
  puVar12 = (ushort *)(*(int *)(iVar6 + 0x20) + iVar8 * 2 + iVar6);
  fVar14 = (float10)FUN_00623e40((double)param_1);
  local_c = (short)(int)ROUND((float)fVar14);
  uVar11 = *puVar12;
  if ((int)local_c < (int)(uint)uVar11) {
    fVar1 = *(float *)(iVar9 + iVar10);
    uVar7 = 0;
    fVar2 = *pfVar13;
  }
  else {
    iVar8 = (int)(short)((ushort)uVar3 & 0xfff);
    uVar7 = puVar12[iVar8 + -1];
    if ((int)local_c == (uint)uVar7) {
      fVar1 = pfVar13[iVar8 + -1];
      uVar11 = uVar7 + 1;
      fVar2 = *(float *)(iVar9 + iVar10);
    }
    else {
      sVar5 = FUN_004d6b10();
      iVar10 = (int)sVar5;
      uVar7 = puVar12[iVar10];
      uVar11 = puVar12[iVar10 + 1];
      fVar1 = pfVar13[iVar10];
      fVar2 = pfVar13[iVar10 + 1];
    }
  }
  fVar4 = (float)(int)(short)uVar7;
  if (param_1 == fVar4) {
    *param_2 = fVar1;
    return;
  }
  fVar4 = (param_1 - fVar4) / (float)((int)(short)uVar11 - (int)(short)uVar7);
  *param_2 = fVar4 * fVar2 + (1.0 - fVar4) * fVar1;
  return;
}
#endif
