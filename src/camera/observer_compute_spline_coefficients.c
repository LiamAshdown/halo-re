// observer_compute_spline_coefficients  (Ghidra: FUN_00447be0; renamed for this rewrite)
// address 0x447be0, size 604 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/camera_types_notes.md's proposed name; field offsets (+0x008
// current_command, +0x054 flags therein via the +0x5c channel_times walk, +0x0e8 velocity, +0x120
// acceleration, +0x158.. coefficient_t5..t0, +0x260 remaining_offset, and the current_command's
// own +0x3c velocity) all match types/camera.h's observer / observer_command layout, and the
// per-channel float counts match 0x00686aec {3,3,1,1,3} (types/camera.h's
// observer_derivative_float_counts). Confirmed the formula against objdump: fits a quintic in
// "time remaining" (t = channel_times[channel]) from the current acceleration, velocity and
// remaining offset (all in remaining-offset space, see types/camera.h), plus -- for the position
// channel only -- an extra correction from the incoming command's own velocity scaled by 30 (the
// per-tick-to-per-second conversion types/camera.h's observer_command.velocity comment notes).
// register convention: local player index in DX (in_DX); no other parameters.

#include "tags.h"
#include "memory.h"
#include "camera.h"
#include "fn_camera.h"

extern observer observers[1];       // 0x006ac65c
extern float observer_dt;           // 0x006ac658
extern int16_t observer_derivative_float_counts[5]; // 0x00686aec, {3,3,1,1,3}

// blam-cc: DX -> local_player_index
void observer_compute_spline_coefficients(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    float *remaining_offset = (float *)&o->remaining_offset;
    float *derivative_velocity = (float *)&o->velocity;
    float *acceleration = (float *)&o->acceleration;
    float *coefficient_t5 = (float *)&o->coefficient_t5;
    float *coefficient_t4 = (float *)&o->coefficient_t4;
    float *coefficient_t3 = (float *)&o->coefficient_t3;
    float *coefficient_t2 = (float *)&o->coefficient_t2;
    float *coefficient_t1 = (float *)&o->coefficient_t1;
    float *coefficient_t0 = (float *)&o->coefficient_t0;
    float *command_velocity = (float *)&o->current_command.velocity;
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];

        if ((o->current_command.flags & _observer_command_valid_bit) != 0 &&
            observer_dt < o->current_command.channel_times[channel]) {
            float inv_t = 1.0f / o->current_command.channel_times[channel];
            float inv_t2 = inv_t * inv_t;
            float inv_t3 = inv_t2 * inv_t;
            float inv_t4 = inv_t3 * inv_t;
            int16_t i;

            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;

                coefficient_t5[idx] = inv_t3 * acceleration[idx] * 0.5f -
                    (inv_t4 * inv_t * remaining_offset[idx] * 6.0f +
                     inv_t4 * derivative_velocity[idx] * 3.0f);
                coefficient_t4[idx] = (inv_t4 * remaining_offset[idx] * 15.0f +
                     inv_t3 * derivative_velocity[idx] * 7.0f) - inv_t2 * acceleration[idx];
                coefficient_t3[idx] = inv_t * acceleration[idx] * 0.5f -
                    (inv_t3 * remaining_offset[idx] * 10.0f + inv_t2 * derivative_velocity[idx] *
                     4.0f);
                coefficient_t2[idx] = 0.0f;
                coefficient_t1[idx] = 0.0f;
                coefficient_t0[idx] = remaining_offset[idx];

                if (channel == _observer_parameter_position) {
                    float scaled_velocity = command_velocity[i] * 30.0f;

                    coefficient_t5[idx] -= inv_t4 * scaled_velocity * 3.0f;
                    coefficient_t4[idx] += inv_t3 * scaled_velocity * 8.0f;
                    coefficient_t3[idx] -= inv_t2 * scaled_velocity * 6.0f;
                    coefficient_t1[idx] += scaled_velocity;
                }
            }
        }
        float_index += count;
    }
}

#if 0
Original Ghidra decompilation (0x447be0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00447be0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  short sVar6;
  int iVar7;
  undefined *puVar8;
  short in_DX;
  int iVar9;
  short sVar10;
  float *pfVar11;
  undefined *puVar12;
  undefined *puVar13;
  undefined *local_28;
  undefined *local_24;
  undefined *local_20;
  undefined *local_1c;
  undefined *local_18;
  undefined *local_14;

  iVar9 = in_DX * 0x29c;
  local_28 = &DAT_006ac7b4 + iVar9;
  local_24 = &DAT_006ac7e0 + iVar9;
  local_20 = &DAT_006ac80c + iVar9;
  local_18 = &DAT_006ac838 + iVar9;
  local_1c = &DAT_006ac864 + iVar9;
  local_14 = &DAT_006ac890 + iVar9;
  pfVar11 = (float *)(&DAT_006ac6b8 + iVar9);
  sVar10 = 0;
  puVar12 = &DAT_006ac77c + iVar9;
  puVar8 = &DAT_006ac8bc + iVar9;
  puVar13 = &DAT_006ac744 + iVar9;
  do {
    if ((((&DAT_006ac664)[iVar9] & 1) != 0) && (_DAT_006ac658 < *pfVar11)) {
      sVar6 = 0;
      fVar1 = 1.0 / *pfVar11;
      fVar2 = fVar1 * fVar1;
      fVar3 = fVar2 * fVar1;
      fVar4 = fVar3 * fVar1;
      if (0 < (short)(&DAT_00686aec)[sVar10]) {
        do {
          iVar7 = (int)sVar6;
          *(float *)(local_28 + iVar7 * 4) =
               fVar3 * *(float *)(puVar12 + iVar7 * 4) * 0.5 -
               (fVar4 * fVar1 * *(float *)(puVar8 + iVar7 * 4) * 6.0 +
               fVar4 * *(float *)(puVar13 + iVar7 * 4) * 3.0);
          *(float *)(local_24 + iVar7 * 4) =
               (fVar4 * *(float *)(puVar8 + iVar7 * 4) * 15.0 +
               fVar3 * *(float *)(puVar13 + iVar7 * 4) * 7.0) -
               fVar2 * *(float *)(puVar12 + iVar7 * 4);
          *(float *)(local_20 + iVar7 * 4) =
               fVar1 * *(float *)(puVar12 + iVar7 * 4) * 0.5 -
               (fVar3 * *(float *)(puVar8 + iVar7 * 4) * 10.0 +
               fVar2 * *(float *)(puVar13 + iVar7 * 4) * 4.0);
          *(undefined4 *)(local_18 + iVar7 * 4) = 0;
          *(undefined4 *)(local_1c + iVar7 * 4) = 0;
          *(undefined4 *)(local_14 + iVar7 * 4) = *(undefined4 *)(puVar8 + iVar7 * 4);
          if (sVar10 == 0) {
            fVar5 = *(float *)(&DAT_006ac6a0 + iVar7 * 4 + iVar9) * 30.0;
            *(float *)(local_28 + iVar7 * 4) =
                 *(float *)(local_28 + iVar7 * 4) - fVar4 * fVar5 * 3.0;
            *(float *)(local_24 + iVar7 * 4) =
                 fVar3 * fVar5 * 8.0 + *(float *)(local_24 + iVar7 * 4);
            *(float *)(local_20 + iVar7 * 4) =
                 *(float *)(local_20 + iVar7 * 4) - fVar2 * fVar5 * 6.0;
            *(float *)(local_1c + iVar7 * 4) = fVar5 + *(float *)(local_1c + iVar7 * 4);
          }
          sVar6 = sVar6 + 1;
        } while (sVar6 < (short)(&DAT_00686aec)[sVar10]);
      }
    }
    iVar7 = (short)(&DAT_00686aec)[sVar10] * 4;
    local_28 = local_28 + iVar7;
    local_24 = local_24 + iVar7;
    local_20 = local_20 + iVar7;
    local_18 = local_18 + iVar7;
    local_1c = local_1c + iVar7;
    local_14 = local_14 + iVar7;
    pfVar11 = pfVar11 + 1;
    puVar12 = puVar12 + iVar7;
    puVar8 = puVar8 + iVar7;
    puVar13 = puVar13 + iVar7;
    sVar10 = sVar10 + 1;
  } while (sVar10 < 5);
  return;
}
#endif
