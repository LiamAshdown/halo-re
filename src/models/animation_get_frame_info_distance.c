// animation_get_frame_info_distance  (Ghidra: FUN_004d4850, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d4850, size 123 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: types notes: "It sums frame_info dx, not vertex weights." animation->frame_info
//   (types/tags.h TagDataOffset.pointer) holds frame_count entries, each 2/3/4 floats wide
//   depending on frame_info_type (DxDy / DxDyDyaw / DxDyDzDyaw -- only the leading dx float is
//   read here). The running sum is snapshotted at key_frame_index and returned separately from
//   the grand total.
// register convention: animation in ECX (in_ECX); dx_to_key_frame, dx_total as the recognized
//   stack parameters (param_1, param_2).
//   // blam-cc: ECX -> animation, stack -> (dx_to_key_frame, dx_total)

#include "tags.h"
#include "math.h"
#include "models.h"

// Sums the leading dx component of every frame_info entry (stride depends on frame_info_type)
// and writes the running total through *dx_total; *dx_to_key_frame receives the same running
// total as it stood right after the entry at key_frame_index was folded in. Either output
// pointer may be NULL. The float literal 0.0 below is the shared .rdata constant at 0x00672ac0
// (see src/input/input_clamp_unit_float.c).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total)
{
    int16_t frame_info_type;
    int16_t frame;
    float total;
    float value_at_key_frame;
    float *frame_info;

    total = 0.0f;
    value_at_key_frame = 0.0f;
    frame_info = (float *)animation->frame_info.pointer;
    frame = 0;
    // the count is tested as a signed word (test di,di / jle, cmp si,di / jl at 0x4d4866, 0x4d48a8)
    if (0 < (int16_t)animation->frame_count) {
        frame_info_type = animation->frame_info_type;
        do {
            if (frame_info_type == 1) {
                total = total + frame_info[0];
                frame_info += 2;
            } else if (frame_info_type == 2) {
                total = total + frame_info[0];
                frame_info += 3;
            } else if (frame_info_type == 3) {
                total = total + frame_info[0];
                frame_info += 4;
            }
            if (frame == (int16_t)animation->key_frame_index) {
                value_at_key_frame = total;
            }
            frame = frame + 1;
        } while (frame < (int16_t)animation->frame_count);
    }
    if (dx_total != 0) {
        *dx_total = total;
    }
    if (dx_to_key_frame != 0) {
        *dx_to_key_frame = value_at_key_frame;
    }
}

#if 0
Original Ghidra decompilation (0x4d4850):

void FUN_004d4850(float *param_1,float *param_2)

{
  short sVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  int in_ECX;
  short sVar5;

  pfVar3 = param_2;
  fVar2 = 0.0;
  pfVar4 = *(float **)(in_ECX + 0x54);
  sVar5 = 0;
  param_2 = (float *)0x0;
  if (0 < *(short *)(in_ECX + 0x22)) {
    sVar1 = *(short *)(in_ECX + 0x26);
    do {
      if (sVar1 == 1) {
        fVar2 = fVar2 + *pfVar4;
        pfVar4 = pfVar4 + 2;
      }
      else if (sVar1 == 2) {
        fVar2 = fVar2 + *pfVar4;
        pfVar4 = pfVar4 + 3;
      }
      else if (sVar1 == 3) {
        fVar2 = fVar2 + *pfVar4;
        pfVar4 = pfVar4 + 4;
      }
      if (sVar5 == *(short *)(in_ECX + 0x34)) {
        param_2 = (float *)fVar2;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < *(short *)(in_ECX + 0x22));
  }
  if (pfVar3 != (float *)0x0) {
    *pfVar3 = fVar2;
  }
  if (param_1 != (float *)0x0) {
    *param_1 = (float)param_2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
