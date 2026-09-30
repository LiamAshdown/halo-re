// actor_movement_choose_strafe_axis  (Ghidra: actor_movement_choose_strafe_axis, renamed)
// address 0x418a40, size 477 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: its only caller is the steering routine 0x4180c0, which feeds the index it
// returns straight into the switch that turns 0/1/2/3 into +forward / -forward / -left /
// +left. It builds those same four candidate axes on the stack out of the requested
// direction (normalized, flattened to 2D unless the 3D flag is set) and scores each one by
// two dot products -- against the reference direction in EDI and against the actor facing in
// ESI -- keeping the candidate that wins on the first score unless the second score would
// drop below an already-good (>= 0.5) value.
// register convention: the requested direction is in EAX, the 2D/3D flag in BL, the actor
// facing in ESI and the reference direction in EDI; the two out-parameters are genuine stack
// parameters.
// blam-cc: EAX -> direction, BL -> use_3d, ESI -> facing, EDI -> reference, stack -> out_axis, out_index
// UNSURE: in the 3D branch candidates 2 and 3 are the shared zero vector at 0x00696714 and
// its negation, so they can never win either dot-product test. That is what the original
// does; it is transcribed, not corrected.
// UNSURE: the four candidates live at consecutive 12-byte stack slots (-0x30, -0x24, -0x18,
// -0xc) which Ghidra prints as eleven separate float locals; the array reading below is the
// same memory.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"
#include "fn_math.h"


extern const real_vector3d *global_origin3d_pointer;            // 0x00696714

// blam-cc: EAX -> direction, BL -> use_3d, ESI -> facing, EDI -> reference, stack -> out_axis, out_index
void actor_movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d,
                                       const real_vector3d *facing, const real_vector3d *reference,
                                       real_vector3d *out_axis, int16_t *out_index)
{
    real_vector3d candidates[4];
    float best_reference_dot;
    float best_facing_dot;
    float reference_dot;
    float facing_dot;
    int16_t best_index;
    int16_t chosen;
    int16_t i;

    candidates[0] = *direction;
    if (use_3d == 0) {
        candidates[0].k = 0.0f;
        if (vector3d_normalize_with_length(&candidates[0]) == 0.0f) {
            candidates[0] = *facing;
        }
        candidates[2].i = -candidates[0].j;
        candidates[2].j = candidates[0].i;
        candidates[2].k = 0.0f;
    } else {
        if (vector3d_normalize_with_length(&candidates[0]) == 0.0f) {
            candidates[0] = *facing;
        }
        candidates[2] = *global_origin3d_pointer;
    }

    candidates[1].i = -candidates[0].i;
    candidates[1].j = -candidates[0].j;
    candidates[1].k = -candidates[0].k;
    candidates[3].i = -candidates[2].i;
    candidates[3].j = -candidates[2].j;
    candidates[3].k = -candidates[2].k;

    best_index = -1;
    best_reference_dot = 0.0f;
    best_facing_dot = 0.0f;

    for (i = 0; i < 4; i++) {
        if (use_3d == 0) {
            reference_dot = reference->j * candidates[i].j + candidates[i].i * reference->i;
            facing_dot = candidates[i].i * facing->i;
        } else {
            reference_dot = reference->j * candidates[i].j + candidates[i].i * reference->i +
                            reference->k * candidates[i].k;
            facing_dot = candidates[i].i * facing->i + facing->k * candidates[i].k;
        }
        facing_dot = candidates[i].j * facing->j + facing_dot;

        chosen = i;
        if (best_index != -1) {
            if (reference_dot <= best_reference_dot) {
                if (facing_dot <= best_facing_dot || reference_dot <= 0.5f) {
                    chosen = best_index;
                    facing_dot = best_facing_dot;
                    reference_dot = best_reference_dot;
                }
            } else if (facing_dot <= best_facing_dot && best_facing_dot >= 0.5f) {
                chosen = best_index;
                facing_dot = best_facing_dot;
                reference_dot = best_reference_dot;
            }
        }
        best_reference_dot = reference_dot;
        best_facing_dot = facing_dot;
        best_index = chosen;
    }

    *out_index = best_index;
    *out_axis = candidates[best_index];
}

#if 0
Original Ghidra decompilation (0x418a40):

void FUN_00418a40(float *param_1,short *param_2)

{
  float fVar1;
  float fVar2;
  short sVar3;
  float *in_EAX;
  int iVar4;
  float *pfVar5;
  short sVar6;
  char unaff_BL;
  short sVar7;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar8;
  float local_3c;
  float local_34;
  float local_30 [4];
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_30[0] = *in_EAX;
  local_30[1] = in_EAX[1];
  local_30[2] = in_EAX[2];
  if (unaff_BL == '\0') {
    local_30[2] = 0.0;
    fVar8 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar8) {
      local_30[0] = *unaff_ESI;
      local_30[1] = unaff_ESI[1];
      local_30[2] = unaff_ESI[2];
    }
    local_14 = local_30[0];
    local_18 = -local_30[1];
    local_10 = 0.0;
  }
  else {
    fVar8 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar8) {
      local_30[0] = *unaff_ESI;
      local_30[1] = unaff_ESI[1];
      local_30[2] = unaff_ESI[2];
    }
    local_18 = *(float *)PTR_DAT_00696714;
    local_14 = *(float *)(PTR_DAT_00696714 + 4);
    local_10 = *(float *)(PTR_DAT_00696714 + 8);
  }
  sVar7 = -1;
  local_30[3] = -local_30[0];
  sVar6 = 0;
  pfVar5 = local_30 + 1;
  local_20 = -local_30[1];
  local_1c = -local_30[2];
  local_c = -local_18;
  local_8 = -local_14;
  local_4 = -local_10;
  do {
    if (unaff_BL == '\0') {
      fVar1 = unaff_EDI[1] * *pfVar5 + pfVar5[-1] * *unaff_EDI;
      fVar2 = pfVar5[-1] * *unaff_ESI;
    }
    else {
      fVar1 = unaff_EDI[1] * *pfVar5 + pfVar5[-1] * *unaff_EDI + unaff_EDI[2] * pfVar5[1];
      fVar2 = pfVar5[-1] * *unaff_ESI + unaff_ESI[2] * pfVar5[1];
    }
    fVar2 = *pfVar5 * unaff_ESI[1] + fVar2;
    sVar3 = sVar6;
    if (sVar7 != -1) {
      if (fVar1 <= local_34) {
        if ((fVar2 <= local_3c) || (fVar1 <= 0.5)) goto LAB_00418bad;
      }
      else if ((fVar2 <= local_3c) && (0.5 <= local_3c)) {
LAB_00418bad:
        sVar3 = sVar7;
        fVar2 = local_3c;
        fVar1 = local_34;
      }
    }
    local_34 = fVar1;
    local_3c = fVar2;
    sVar7 = sVar3;
    sVar6 = sVar6 + 1;
    pfVar5 = pfVar5 + 3;
    if (3 < sVar6) {
      iVar4 = (int)sVar7;
      *param_2 = sVar7;
      *param_1 = local_30[iVar4 * 3];
      fVar1 = local_30[iVar4 * 3 + 2];
      param_1[1] = local_30[iVar4 * 3 + 1];
      param_1[2] = fVar1;
      return;
    }
  } while( true );
}
#endif
