// recorded_animation_decode_char_difference_event  (Ghidra: FUN_0044a1d0; renamed per types
// notes: "compressed difference event handlers")
// address 0x44a1d0, size 439 bytes
// name confidence: 0.8 (symbols/agent_phase4_cutscene.txt)   rewrite confidence: 0.8
// evidence: out/phase4/cutscene_types_notes.md "Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0,
// 7..14 are 0x44a1d0 (mask = type-7) ... The first set bit gets the delta; the later set bits
// copy the vector just produced (and its compressed angles) instead of decoding their own, so
// one delta pair is consumed per event." types/cutscene.h recorded_animation_vector_mask
// (facing 0x1, aiming 0x2, looking 0x4) and recorded_animation_compressed_event_proc
// (state, control, header, cursor). unit_control_data facing/aiming/looking_vector at
// 0x1c/0x28/0x34 match the raw offsets exactly.
// register convention: cdecl stack parameters (state, control, header, cursor), matching
// Ghidra's own param_1..param_4 recovery -- no register-passed (in_EAX-style) arguments appear
// in the body. blam-cc: stack -> (state, control, header, cursor).
// NOTE (objdump-verified against 0x44a1d0 / 0x44a390, not a doubt): the facing branch and
// the aiming-direct-decode branch inline the cos/sin vector math
// instead of calling recorded_animation_angle_to_vector, while the looking-only branch calls
// both recorded_animation_apply_char_difference and recorded_animation_angle_to_vector for
// real; reproduced exactly (this is an LTCG inlining choice, not a behavioural difference).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

extern void recorded_animation_apply_char_difference(recorded_animation_angles *angles,
    recorded_animation_char_difference *delta); // 0x44a110, this batch
extern void recorded_animation_angle_to_vector(real_vector3d *out,
    recorded_animation_angles *angles); // 0x44a190, this batch

// cos/sin are single x87 FCOS/FSIN instructions in the original code (Ghidra's fcos()/fsin()
// pseudo-calls); declared locally instead of via <math.h> because -I types shadows that header
// name with types/math.h.
extern double cos(double x);
extern double sin(double x);

extern float recorded_animation_angle_scale; // 0x00672dd8, pi / 1000

// blam-cc: stack -> (state, control, header, cursor)
// Decodes one compressed char-difference event (types 7..14): applies the 8 bit delta at
// *cursor to whichever of facing/aiming/looking is the first set bit of (header type - 7),
// recomputes that vector, and has every later set bit copy the vector (and angle pair) just
// produced instead of decoding its own. Advances *cursor by 2.
void recorded_animation_decode_char_difference_event(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    recorded_animation_char_difference *delta;
    uint8_t mask;
    uint8_t facing_bit;
    int16_t yaw;
    int16_t pitch;
    double cos_pitch;
    double cos_yaw;

    delta = (recorded_animation_char_difference *)*cursor;
    mask = (uint8_t)((*header >> 2) - 7);
    facing_bit = mask & 1;

    if (facing_bit != 0) {
        state->facing.yaw = state->facing.yaw + (int16_t)delta->yaw;
        yaw = state->facing.yaw;
        if (1000 < yaw) {
            yaw = yaw - 1000;
            state->facing.yaw = yaw;
        } else if (yaw < -1000) {
            yaw = yaw + 1000;
            state->facing.yaw = yaw;
        }
        state->facing.pitch = state->facing.pitch + (int16_t)delta->pitch;
        yaw = state->facing.yaw;
        pitch = state->facing.pitch;
        cos_pitch = cos((double)pitch * (double)recorded_animation_angle_scale);
        cos_yaw = cos((double)yaw * (double)recorded_animation_angle_scale);
        control->facing_vector.i = (float)(cos_yaw * cos_pitch);
        control->facing_vector.j = (float)(sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
        control->facing_vector.k = (float)sin((double)pitch * (double)recorded_animation_angle_scale);
    }

    if ((mask & 2) != 0) {
        if (facing_bit == 0) {
            recorded_animation_apply_char_difference(&state->aiming, delta);
            yaw = state->aiming.yaw;
            pitch = state->aiming.pitch;
            cos_pitch = cos((double)pitch * (double)recorded_animation_angle_scale);
            cos_yaw = cos((double)yaw * (double)recorded_animation_angle_scale);
            control->aiming_vector.i = (float)(cos_yaw * cos_pitch);
            control->aiming_vector.j = (float)(sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
            control->aiming_vector.k = (float)sin((double)pitch * (double)recorded_animation_angle_scale);
        } else {
            state->aiming = state->facing;
            control->aiming_vector = control->facing_vector;
        }
    }

    if ((mask & 4) != 0) {
        if (facing_bit != 0) {
            state->looking = state->facing;
            control->looking_vector = control->facing_vector;
            *cursor += 2;
            return;
        }
        if ((mask & 2) != 0) {
            state->looking = state->aiming;
            control->looking_vector = control->aiming_vector;
            *cursor += 2;
            return;
        }
        recorded_animation_apply_char_difference(&state->looking, delta);
        recorded_animation_angle_to_vector(&control->looking_vector, &state->looking);
    }
    *cursor += 2;
}

#if 0
Original Ghidra decompilation (0x44a1d0):

void FUN_0044a1d0(short *param_1,int param_2,byte *param_3,int *param_4)

{
  short sVar1;
  char *pcVar2;
  byte bVar3;
  short sVar4;
  short *psVar5;
  byte bVar6;
  float10 fVar7;
  float10 fVar8;

  pcVar2 = (char *)*param_4;
  bVar6 = (*param_3 >> 2) - 7;
  bVar3 = bVar6 & 1;
  if (bVar3 == 0) goto LAB_0044a278;
  *param_1 = *param_1 + (short)*pcVar2;
  sVar4 = *param_1;
  if (sVar4 < 0x3e9) {
    if (sVar4 < -1000) {
      sVar4 = sVar4 + 1000;
      goto LAB_0044a229;
    }
  }
  else {
    sVar4 = sVar4 + -1000;
LAB_0044a229:
    *param_1 = sVar4;
  }
  param_1[1] = param_1[1] + (short)pcVar2[1];
  sVar4 = *param_1;
  sVar1 = param_1[1];
  fVar7 = (float10)fcos((float10)(int)sVar1 * (float10)0.0031415927);
  fVar8 = (float10)fcos((float10)(int)sVar4 * (float10)0.0031415927);
  *(float *)(param_2 + 0x1c) = (float)(fVar8 * fVar7);
  fVar8 = (float10)fsin((float10)(int)sVar4 * (float10)0.0031415927);
  *(float *)(param_2 + 0x20) = (float)(fVar8 * fVar7);
  fVar7 = (float10)fsin((float10)(int)sVar1 * (float10)0.0031415927);
  *(float *)(param_2 + 0x24) = (float)fVar7;
LAB_0044a278:
  if ((bVar6 & 2) != 0) {
    if (bVar3 == 0) {
      psVar5 = (short *)FUN_0044a110();
      sVar4 = *psVar5;
      sVar1 = psVar5[1];
      fVar7 = (float10)fcos((float10)(int)sVar1 * (float10)0.0031415927);
      fVar8 = (float10)fcos((float10)(int)sVar4 * (float10)0.0031415927);
      *(float *)(param_2 + 0x28) = (float)(fVar8 * fVar7);
      fVar8 = (float10)fsin((float10)(int)sVar4 * (float10)0.0031415927);
      *(float *)(param_2 + 0x2c) = (float)(fVar8 * fVar7);
      fVar7 = (float10)fsin((float10)(int)sVar1 * (float10)0.0031415927);
      *(float *)(param_2 + 0x30) = (float)fVar7;
    }
    else {
      *(undefined4 *)(param_1 + 2) = *(undefined4 *)param_1;
      *(undefined4 *)(param_2 + 0x28) = *(undefined4 *)(param_2 + 0x1c);
      *(undefined4 *)(param_2 + 0x2c) = *(undefined4 *)(param_2 + 0x20);
      *(undefined4 *)(param_2 + 0x30) = *(undefined4 *)(param_2 + 0x24);
    }
  }
  if ((bVar6 & 4) != 0) {
    if (bVar3 != 0) {
      *(undefined4 *)(param_1 + 4) = *(undefined4 *)param_1;
      *(undefined4 *)(param_2 + 0x34) = *(undefined4 *)(param_2 + 0x1c);
      *(undefined4 *)(param_2 + 0x38) = *(undefined4 *)(param_2 + 0x20);
      *(undefined4 *)(param_2 + 0x3c) = *(undefined4 *)(param_2 + 0x24);
      *param_4 = *param_4 + 2;
      return;
    }
    if ((bVar6 & 2) != 0) {
      *(undefined4 *)(param_1 + 4) = *(undefined4 *)(param_1 + 2);
      *(undefined4 *)(param_2 + 0x34) = *(undefined4 *)(param_2 + 0x28);
      *(undefined4 *)(param_2 + 0x38) = *(undefined4 *)(param_2 + 0x2c);
      *(undefined4 *)(param_2 + 0x3c) = *(undefined4 *)(param_2 + 0x30);
      *param_4 = *param_4 + 2;
      return;
    }
    FUN_0044a110();
    recorded_animation_angle_to_vector();
  }
  *param_4 = *param_4 + 2;
  return;
}
#endif
