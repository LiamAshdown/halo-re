// observer_evaluate_spline_velocity  (Ghidra: FUN_00448010; renamed for this rewrite)
// address 0x448010, size 496 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/camera_types_notes.md's proposed name and its "value = t*c1+...+c0"
// formula (types/camera.h), whose first derivative is exactly this function's
// c1 + 2*c2*t + 3*c3*t^2 + 4*c4*t^3 + 5*c5*t^4. The finishing-channel branch matches
// observer_interpolation_flags's own comment ("with snap, stop dead"): valid + (exact or snap)
// zeroes velocity, valid + neither computes a closing velocity from -remaining_offset/dt, and an
// invalid command leaves velocity untouched. The dead byte-remainder loop Ghidra shows after the
// zero-fill is omitted as unreachable (see observer_evaluate_spline_acceleration.c).
// register convention: local player index in AX (in_AX); no other parameters.
// VERIFIED against disassembly 0x448010..0x448200 (2026-09-30): 1/dt stays in extended precision, the valid/exact/snap branch,
// the closing velocity and the term association order agree.

#include "tags.h"
#include "memory.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern observer observers[1];       // 0x006ac65c
extern float observer_dt;           // 0x006ac658
extern int16_t observer_derivative_float_counts[5]; // 0x00686aec, {3,3,1,1,3}

// blam-cc: AX -> local_player_index
void observer_evaluate_spline_velocity(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    float *remaining_offset = (float *)&o->remaining_offset;
    float *coefficient_t5 = (float *)&o->coefficient_t5;
    float *coefficient_t4 = (float *)&o->coefficient_t4;
    float *coefficient_t3 = (float *)&o->coefficient_t3;
    float *coefficient_t2 = (float *)&o->coefficient_t2;
    float *coefficient_t1 = (float *)&o->coefficient_t1;
    float *velocity = (float *)&o->velocity;
    double inv_dt = 1.0 / (double)observer_dt; // fdivr qword [0x672af8] (1.0): stays in x87 extended precision
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        int16_t i;

        if (t <= 0.0f) {
            uint8_t valid = (o->current_command.flags & _observer_command_valid_bit) != 0;

            if (!valid ||
                (o->current_command.interpolation_flags[channel] &
                 _observer_interpolation_exact_bit) == 0 &&
                (o->current_command.flags & _observer_command_snap_bit) == 0) {
                if (valid) {
                    for (i = 0; i < count; i++) {
                        velocity[float_index + i] = (float)-(inv_dt * remaining_offset[float_index + i]);
                    }
                }
            } else {
                for (i = 0; i < count; i++) {
                    velocity[float_index + i] = 0.0f;
                }
            }
        } else {
            float t2 = t * t;
            float t3 = t2 * t;
            float t4 = t3 * t;

            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;

                // same association order as the fmul/faddp sequence at 0x4480e0..0x44812d
                velocity[idx] = ((((t4 * coefficient_t5[idx] * 5.0f + t3 * coefficient_t4[idx] * 4.0f) +
                    t2 * coefficient_t3[idx] * 3.0f) + (t * coefficient_t2[idx] * 2.0f)) +
                    coefficient_t1[idx]);
            }
        }
        float_index += count;
    }
}

#if 0
Original Ghidra decompilation (0x448010):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00448010(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short in_AX;
  int iVar4;
  int iVar5;
  short sVar6;
  uint uVar7;
  short *psVar8;
  undefined4 *puVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined *puVar12;
  undefined *local_2c;
  undefined *local_28;
  undefined *local_24;
  undefined *local_20;
  undefined *local_1c;
  float *local_18;
  byte *local_14;
  int local_8;

  fVar2 = 1.0 / _DAT_006ac658;
  local_8 = 5;
  iVar10 = in_AX * 0x29c;
  local_2c = &DAT_006ac8bc + iVar10;
  local_24 = &DAT_006ac80c + iVar10;
  local_28 = &DAT_006ac7e0 + iVar10;
  local_20 = &DAT_006ac838 + iVar10;
  local_18 = (float *)(&DAT_006ac6b8 + iVar10);
  puVar12 = &DAT_006ac7b4 + iVar10;
  local_1c = &DAT_006ac864 + iVar10;
  local_14 = (byte *)(iVar10 + 0x6ac6b0);
  puVar9 = (undefined4 *)(&DAT_006ac744 + iVar10);
  psVar8 = &DAT_00686aec;
  do {
    fVar3 = *local_18 - _DAT_006ac658;
    if (fVar3 <= 0.0) {
      uVar7 = *(uint *)(&DAT_006ac664 + iVar10) & 1;
      if ((uVar7 == 0) || (((*local_14 & 2) == 0 && ((*(uint *)(&DAT_006ac664 + iVar10) & 8) == 0)))
         ) {
        if ((uVar7 != 0) && (sVar6 = 0, 0 < *psVar8)) {
          do {
            iVar4 = (int)sVar6;
            sVar6 = sVar6 + 1;
            puVar9[iVar4] = -(fVar2 * *(float *)(local_2c + iVar4 * 4));
          } while (sVar6 < *psVar8);
        }
      }
      else {
        puVar11 = puVar9;
        for (uVar7 = (int)*psVar8 & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
          *puVar11 = 0;
          puVar11 = puVar11 + 1;
        }
        for (iVar4 = 0; iVar4 != 0; iVar4 = iVar4 + -1) {
          *(undefined1 *)puVar11 = 0;
          puVar11 = (undefined4 *)((int)puVar11 + 1);
        }
      }
    }
    else {
      sVar6 = 0;
      fVar1 = fVar3 * fVar3 * fVar3;
      if (0 < *psVar8) {
        do {
          iVar4 = (int)sVar6;
          iVar5 = iVar4 * 4;
          sVar6 = sVar6 + 1;
          puVar9[iVar4] =
               fVar3 * *(float *)(local_20 + iVar5) + fVar3 * *(float *)(local_20 + iVar5) +
               fVar3 * fVar3 * *(float *)(local_24 + iVar5) * 3.0 +
               fVar1 * *(float *)(local_28 + iVar5) * 4.0 +
               fVar1 * fVar3 * *(float *)(puVar12 + iVar5) * 5.0 + *(float *)(local_1c + iVar5);
        } while (sVar6 < *psVar8);
      }
    }
    iVar4 = *psVar8 * 4;
    local_2c = local_2c + iVar4;
    local_28 = local_28 + iVar4;
    local_24 = local_24 + iVar4;
    local_20 = local_20 + iVar4;
    puVar12 = puVar12 + iVar4;
    puVar9 = puVar9 + *psVar8;
    local_1c = local_1c + iVar4;
    local_18 = local_18 + 1;
    local_14 = local_14 + 1;
    psVar8 = psVar8 + 1;
    local_8 = local_8 + -1;
  } while (local_8 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
