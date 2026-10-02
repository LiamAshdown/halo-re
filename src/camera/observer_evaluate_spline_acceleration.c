// observer_evaluate_spline_acceleration  (Ghidra: FUN_00447e40; renamed for this rewrite)
// address 0x447e40, size 452 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/camera_types_notes.md's proposed name and its "0x448210 evaluates
// t*c1+t^2*c2+t^3*c3+t^4*c4+t^5*c5+c0" note (types/camera.h), whose second derivative is exactly
// this function's 2*c2 + 6*c3*t + 12*c4*t^2 + 20*c5*t^3 formula; the blow-up guard matches the
// header's citation of 0x006572c4 {1500,1500,1e5,1e5,1e5} as a per-channel acceleration limit
// that resets channel_times on overflow. The dead byte-remainder loop Ghidra shows after the
// zero-fill (iVar5 starts at, and stays, 0) is omitted as unreachable.
// register convention: local player index in AX (in_AX); no other parameters.
// VERIFIED against disassembly 0x447e40..0x448003 (2026-09-30): pointer bases (+0x120 acceleration, +0x158..+0x1dc the
// coefficients, +0x5c channel times), the limit test (> limit or < -limit), the reset of coupled channels and the term
// association order agree.

#include "tags.h"
#include "memory.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern observer observers[1];       // 0x006ac65c
extern float observer_dt;           // 0x006ac658
extern int16_t observer_derivative_float_counts[5]; // 0x00686aec, {3,3,1,1,3}
extern float observer_channel_acceleration_limit[5]; // 0x006572c4, {1500,1500,1e5,1e5,1e5}

// blam-cc: AX -> local_player_index
// Evaluates each channel's quintic spline acceleration at the time remaining after this tick's
// dt, zeroing it (and resetting the channel's, and any coupled channel's, remaining time) once
// the channel finishes, or snapping the channel to finished if the acceleration blows past its
// per-channel limit.
void observer_evaluate_spline_acceleration(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    float *coefficient_t5 = (float *)&o->coefficient_t5;
    float *coefficient_t4 = (float *)&o->coefficient_t4;
    float *coefficient_t3 = (float *)&o->coefficient_t3;
    float *coefficient_t2 = (float *)&o->coefficient_t2;
    float *acceleration = (float *)&o->acceleration;
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        float t2 = t * t;
        float t3 = t2 * t;
        int16_t i;

        if (t <= 0.0f) {
            for (i = 0; i < count; i++) {
                acceleration[float_index + i] = 0.0f;
            }
        } else {
            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;
                // same association order as the fmul/faddp sequence at 0x447ee4..0x447f26
                float value = (((t3 * coefficient_t5[idx] * 20.0f + t2 * coefficient_t4[idx] * 12.0f) +
                    t * coefficient_t3[idx] * 6.0f) + (coefficient_t2[idx] + coefficient_t2[idx]));

                acceleration[idx] = value;

                if (observer_channel_acceleration_limit[channel] < value ||
                    value < -observer_channel_acceleration_limit[channel]) {
                    int16_t other_channel;

                    for (other_channel = 0; other_channel < k_observer_parameter_count;
                         other_channel++) {
                        if (other_channel != channel &&
                            o->current_command.channel_times[other_channel] ==
                                o->current_command.channel_times[channel]) {
                            o->current_command.channel_times[other_channel] = 0.0f;
                        }
                    }
                    o->current_command.channel_times[channel] = 0.0f;
                }
            }
        }
        float_index += count;
    }
}

#if 0
Original Ghidra decompilation (0x447e40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00447e40(void)

{
  float fVar1;
  float fVar2;
  short in_AX;
  short sVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  short sVar7;
  uint uVar8;
  float *pfVar9;
  short *psVar10;
  float *pfVar11;
  undefined4 *puVar12;
  undefined4 *puVar13;
  undefined *puVar14;
  float *local_24;
  undefined *local_20;
  undefined *local_1c;
  undefined *local_18;

  iVar4 = in_AX * 0x29c;
  local_20 = &DAT_006ac7e0 + iVar4;
  puVar14 = &DAT_006ac7b4 + iVar4;
  local_1c = &DAT_006ac80c + iVar4;
  local_18 = &DAT_006ac838 + iVar4;
  puVar12 = (undefined4 *)(&DAT_006ac77c + iVar4);
  sVar3 = 0;
  psVar10 = &DAT_00686aec;
  pfVar11 = (float *)&DAT_006572c4;
  local_24 = (float *)(&DAT_006ac6b8 + iVar4);
  do {
    fVar2 = *local_24 - _DAT_006ac658;
    if (fVar2 <= 0.0) {
      puVar13 = puVar12;
      for (uVar8 = (int)*psVar10 & 0x3fffffff; uVar8 != 0; uVar8 = uVar8 - 1) {
        *puVar13 = 0;
        puVar13 = puVar13 + 1;
      }
      for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
        *(undefined1 *)puVar13 = 0;
        puVar13 = (undefined4 *)((int)puVar13 + 1);
      }
    }
    else {
      sVar6 = 0;
      if (0 < *psVar10) {
        do {
          iVar5 = sVar6 * 4;
          fVar1 = *(float *)(local_18 + iVar5) + *(float *)(local_18 + iVar5) +
                  fVar2 * *(float *)(local_1c + iVar5) * 6.0 +
                  fVar2 * fVar2 * *(float *)(local_20 + iVar5) * 12.0 +
                  fVar2 * fVar2 * fVar2 * *(float *)(puVar14 + iVar5) * 20.0;
          puVar12[sVar6] = fVar1;
          if ((*pfVar11 < fVar1) || (fVar1 < -*pfVar11)) {
            sVar7 = 0;
            pfVar9 = (float *)(&DAT_006ac6b8 + iVar4);
            do {
              if ((sVar7 != sVar3) && (*pfVar9 == *local_24)) {
                *pfVar9 = 0.0;
              }
              sVar7 = sVar7 + 1;
              pfVar9 = pfVar9 + 1;
            } while (sVar7 < 5);
            *local_24 = 0.0;
          }
          sVar6 = sVar6 + 1;
        } while (sVar6 < *psVar10);
      }
    }
    iVar5 = *psVar10 * 4;
    local_20 = local_20 + iVar5;
    puVar14 = puVar14 + iVar5;
    local_1c = local_1c + iVar5;
    puVar12 = puVar12 + *psVar10;
    local_18 = local_18 + iVar5;
    local_24 = local_24 + 1;
    sVar3 = sVar3 + 1;
    pfVar11 = pfVar11 + 1;
    psVar10 = psVar10 + 1;
  } while (sVar3 < 5);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
