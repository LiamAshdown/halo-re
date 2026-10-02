// bounded_ramp_profile_evaluate  (Ghidra: FUN_00564990; renamed for this rewrite)
// address 0x564990, size 334 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/units_types_notes.md "0x564580, 0x564840, 0x564990 -- a bounded
//   acceleration/velocity ramp solver and its evaluator." This is the evaluator: advances a
//   profile built by bounded_ramp_profile_build (0x564580) forward by `time`, integrating
//   position/velocity through whichever of the three phases (accel / cruise / decel) `time`
//   reaches, and reports whether the whole profile was consumed (the ramp finished, i.e. the
//   caller has reached its target) via the return value.
// register convention: none recognized by Ghidra -- profile in ECX (in_ECX), the rest ordinary
//   stack parameters.
//   // blam-cc: ECX -> profile, stack -> (time, start_position, out_position, start_velocity, out_velocity)
// UNSURE: the return value's upper 3 bytes (CONCAT31 in the decompile) are never read by any
//   known caller; preserved as an unspecified byte return rather than truncated to bool.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// See src/math/bounded_ramp_profile_build.c for the profile buffer layout (TYPES-GAP, 0x20 bytes).

// Advances `profile` by `time`, writing the resulting position to *out_position and velocity to
// *out_velocity. Returns nonzero once `time` has run past every phase in the profile (i.e. the
// ramp has finished and the caller has reached its target); while the profile is already in the
// dead zone (within_dead_zone != 0) it returns that same flag unchanged.
uint8_t bounded_ramp_profile_evaluate(bounded_ramp_profile *profile, real time, real start_position,
                                       real *out_position, real start_velocity, real *out_velocity)
{
    uint8_t coasting;
    real position;
    real velocity;
    real phase_time;

    coasting = profile->within_dead_zone;
    position = start_position;
    velocity = start_velocity;

    if (coasting == 0 && 0.0f < time) {
        if (0.0f < profile->phase1_duration) {
            phase_time = time;
            if (profile->phase1_duration < time) {
                phase_time = profile->phase1_duration;
            }
            position = (phase_time * profile->phase1_acceleration * 0.5f + start_velocity) * phase_time + position;
            velocity = phase_time * profile->phase1_acceleration + start_velocity;
            time = time - phase_time;
        }
        if (0.0f < time) {
            if (0.0f < profile->phase2_duration) {
                phase_time = time;
                if (profile->phase2_duration < time) {
                    phase_time = profile->phase2_duration;
                }
                position = velocity * phase_time + position;
                time = time - phase_time;
            }
            if (0.0f < time) {
                if (0.0f < profile->phase3_duration) {
                    phase_time = time;
                    if (profile->phase3_duration < time) {
                        phase_time = profile->phase3_duration;
                    }
                    position = (phase_time * profile->phase3_acceleration * 0.5f + velocity) * phase_time + position;
                    velocity = phase_time * profile->phase3_acceleration + velocity;
                    time = time - phase_time;
                }
                if (0.0f < time) {
                    *out_position = position;
                    *out_velocity = velocity;
                    return 1;
                }
            }
        }
    }

    *out_position = position;
    *out_velocity = velocity;
    return coasting;
}

#if 0
Original Ghidra decompilation (0x564990):

undefined4 FUN_00564990(float param_1,float param_2,float *param_3,float param_4,float *param_5)

{
  char cVar1;
  float fVar2;
  char *in_ECX;
  float local_8;

  cVar1 = *in_ECX;
  local_8 = param_4;
  if ((cVar1 == '\0') && (0.0 < param_1)) {
    if (0.0 < *(float *)(in_ECX + 0x10)) {
      fVar2 = param_1;
      if (*(float *)(in_ECX + 0x10) < param_1) {
        fVar2 = *(float *)(in_ECX + 0x10);
      }
      param_2 = (fVar2 * *(float *)(in_ECX + 0xc) * 0.5 + param_4) * fVar2 + param_2;
      local_8 = fVar2 * *(float *)(in_ECX + 0xc) + param_4;
      param_1 = param_1 - fVar2;
    }
    if (0.0 < param_1) {
      if (0.0 < *(float *)(in_ECX + 0x14)) {
        fVar2 = param_1;
        if (*(float *)(in_ECX + 0x14) < param_1) {
          fVar2 = *(float *)(in_ECX + 0x14);
        }
        param_2 = local_8 * fVar2 + param_2;
        param_1 = param_1 - fVar2;
      }
      if (0.0 < param_1) {
        if (0.0 < *(float *)(in_ECX + 0x1c)) {
          fVar2 = param_1;
          if (*(float *)(in_ECX + 0x1c) < param_1) {
            fVar2 = *(float *)(in_ECX + 0x1c);
          }
          param_2 = (fVar2 * *(float *)(in_ECX + 0x18) * 0.5 + local_8) * fVar2 + param_2;
          local_8 = fVar2 * *(float *)(in_ECX + 0x18) + local_8;
          param_1 = param_1 - fVar2;
        }
        if (0.0 < param_1) {
          *param_3 = param_2;
          *param_5 = local_8;
          return CONCAT31((int3)((uint)local_8 >> 8),1);
        }
      }
    }
  }
  *param_3 = param_2;
  *param_5 = local_8;
  return CONCAT31((int3)((uint)local_8 >> 8),cVar1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
