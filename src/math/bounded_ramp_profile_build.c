// bounded_ramp_profile_build  (Ghidra: FUN_00564580; renamed for this rewrite)
// address 0x564580, size 689 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/units_types_notes.md "0x564580, 0x564840, 0x564990 -- a bounded
//   acceleration/velocity ramp solver and its evaluator, working entirely on a caller-supplied
//   profile buffer. Pure math." This file is the solver half: given a position error and a
//   starting velocity, it builds a bang-bang (accelerate / coast / decelerate) motion profile
//   bounded by a maximum velocity and a maximum acceleration, exactly the v^2 = 2*a*d braking
//   curve that src/math/vector3d_rotate_toward_with_acceleration.c documents for the simpler
//   unbounded servo (0x4cf530) -- this is the same control law with an explicit output profile
//   instead of one implicit step. Confirmed against pack.py's Ghidra decompile; not independently
//   re-verified against objdump instruction-by-instruction (this file's own control flow already
//   matches the decompile 1:1, so the extra pass would not change anything). The profile buffer
//   layout (TYPES-GAP: no established struct) is inferred from every read site in
//   bounded_ramp_profile_evaluate (0x564990) and bounded_ramp_profile_synchronize (0x564840),
//   which this rewrite treats as ground truth for the field offsets.
// register convention: none recognized by Ghidra -- all five are ordinary stack parameters.
//   // blam-cc: stack -> (position_error, initial_velocity, max_velocity, max_acceleration, profile)
// REVIEW (orphan pass 4, objdump 0x564580..0x564830): fixed the braking quadratic's root choice
//   (smallest non-negative root) and made every `initial_velocity <= 0` branch test the single
//   CL flag the original computes once (initial_velocity > 0), which differs only for a NaN.
// UNSURE: parameter names are inferred from the arithmetic (a v^2=2ad braking-distance solve),
//   not from a string or caller comment; out_profile's field names are likewise inferred, not
//   established elsewhere.

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // x87 FSQRT, declared locally as elsewhere in this module
extern double fabs(double x); // x87 FABS

// bounded_ramp_profile (TYPES-GAP, 0x20 bytes, no established type -- every field name is this
// rewrite's own inference from bounded_ramp_profile_evaluate/_synchronize):
//   0x00 uint8_t  within_dead_zone     -- true if |position_error| and |initial_velocity| are
//                                         both already below 0.001 (no ramp needed)
//   0x04 float    start_position       -- position_error, always written
//   0x08 float    start_velocity       -- initial_velocity, always written
//   0x0c float    phase1_acceleration
//   0x10 float    phase1_duration
//   0x14 float    phase2_duration      -- constant-velocity coast phase
//   0x18 float    phase3_acceleration
//   0x1c float    phase3_duration

// Builds a bounded (max_velocity / max_acceleration limited) three-phase motion profile that
// carries a point at initial_velocity, offset by -position_error, back to 0 and brings it to
// rest. Recurses once on a sign flip so the forward-moving case only needs to be solved once.
void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity,
                                 real max_acceleration, uint8_t *profile)
{
    real half_v_over_a;
    real reach; // position_error plus the distance covered while coasting to a stop at initial_velocity
    real accel_time;
    real b, disc;
    real end_velocity;
    real root_1, root_2;
    uint8_t moving_forward; // CL in the original: initial_velocity > 0 (a NaN counts as not forward)

    *(real *)(profile + 0x04) = position_error;
    *(real *)(profile + 0x08) = initial_velocity;

    if ((real)fabs((double)position_error) < 0.001f && (real)fabs((double)initial_velocity) < 0.001f) {
        profile[0] = 1;
        *(uint32_t *)(profile + 0x0c) = 0;
        *(uint32_t *)(profile + 0x10) = 0;
        *(uint32_t *)(profile + 0x14) = 0;
        *(uint32_t *)(profile + 0x18) = 0;
        *(uint32_t *)(profile + 0x1c) = 0;
        return;
    }
    profile[0] = 0;

    half_v_over_a = (real)fabs((double)initial_velocity) / max_acceleration;
    moving_forward = (uint8_t)(initial_velocity > 0.0f);
    if (half_v_over_a * 0.5f * initial_velocity * 0.5f + position_error < 0.0f) {
        // Moving the wrong way: solve the mirrored problem and negate the result.
        bounded_ramp_profile_build(-position_error, -initial_velocity, max_velocity, max_acceleration, profile);
        *(real *)(profile + 0x04) = -*(real *)(profile + 0x04);
        *(real *)(profile + 0x08) = -*(real *)(profile + 0x08);
        *(real *)(profile + 0x0c) = -*(real *)(profile + 0x0c);
        *(real *)(profile + 0x18) = -*(real *)(profile + 0x18);
        return;
    }

    reach = initial_velocity * 0.5f * half_v_over_a + position_error;
    if (reach < 0.0f) {
        // Already carrying enough velocity to overshoot while just coasting to a stop: the whole
        // profile is a single deceleration with no accel or cruise phase.
        *(uint32_t *)(profile + 0x0c) = 0;
        *(uint32_t *)(profile + 0x10) = 0;
        *(uint32_t *)(profile + 0x14) = 0;
        accel_time = (initial_velocity * initial_velocity) / (position_error + position_error);
        *(real *)(profile + 0x18) = accel_time;
        *(real *)(profile + 0x1c) = -(initial_velocity / accel_time);
        return;
    }

    if (!moving_forward) {
        // Solve the quadratic for the time-to-reach-cruise-speed braking curve and take its
        // smallest non-negative root (0x5646e1..0x56476f; the orphan pass 4 review corrected the
        // root selection, which the draft reduced to "root_1 unless both are negative").
        b = -max_acceleration;
        disc = (real)sqrt((double)((initial_velocity + initial_velocity) * (initial_velocity + initial_velocity) -
                      b * reach * 4.0f));
        root_1 = (-(initial_velocity + initial_velocity) - disc) / (b + b);
        root_2 = (disc - (initial_velocity + initial_velocity)) / (b + b);
        if (!(root_1 < 0.0f) && (root_2 < 0.0f || root_1 < root_2)) {
            end_velocity = root_1;
        } else if (0.0f > root_2) {
            end_velocity = 0.0f;
        } else {
            end_velocity = root_2;
        }
    } else {
        end_velocity = (real)sqrt((double)(reach / max_acceleration));
    }

    if (0.0f < max_velocity) {
        if (!moving_forward) {
            max_velocity = initial_velocity + max_velocity;
        }
        max_velocity = max_velocity / max_acceleration;
        if (max_velocity < 0.0f) {
            max_velocity = 0.0f;
        }
        if (max_velocity < end_velocity) {
            goto have_accel_time;
        }
    }
    max_velocity = end_velocity;

have_accel_time:
    *(real *)(profile + 0x0c) = -max_acceleration;
    *(real *)(profile + 0x18) = max_acceleration;
    if (!moving_forward) {
        *(real *)(profile + 0x10) = max_velocity;
        half_v_over_a = max_velocity + half_v_over_a;
    } else {
        *(real *)(profile + 0x10) = max_velocity + half_v_over_a;
        half_v_over_a = max_velocity;
    }
    *(real *)(profile + 0x1c) = half_v_over_a;

    if (max_velocity < end_velocity) {
        real coast_velocity = -max_acceleration * *(real *)(profile + 0x10) + initial_velocity;
        real coast_time = end_velocity - max_velocity;
        real term = coast_time * coast_velocity;
        *(real *)(profile + 0x14) =
            ((term + term) - coast_time * coast_time * max_acceleration) / coast_velocity;
        return;
    }
    *(uint32_t *)(profile + 0x14) = 0;
}

#if 0
Original Ghidra decompilation (0x564580):

void FUN_00564580(float param_1,float param_2,float param_3,float param_4,char *param_5)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  char cVar6;

  *(float *)(param_5 + 4) = param_1;
  *(float *)(param_5 + 8) = param_2;
  if ((0.001 <= ABS(param_1)) || (0.001 <= ABS(param_2))) {
    cVar6 = '\0';
  }
  else {
    cVar6 = '\x01';
  }
  *param_5 = cVar6;
  if (cVar6 != '\0') {
    param_5[0xc] = '\0';
    param_5[0xd] = '\0';
    param_5[0xe] = '\0';
    param_5[0xf] = '\0';
    param_5[0x10] = '\0';
    param_5[0x11] = '\0';
    param_5[0x12] = '\0';
    param_5[0x13] = '\0';
    param_5[0x18] = '\0';
    param_5[0x19] = '\0';
    param_5[0x1a] = '\0';
    param_5[0x1b] = '\0';
    param_5[0x1c] = '\0';
    param_5[0x1d] = '\0';
    param_5[0x1e] = '\0';
    param_5[0x1f] = '\0';
    param_5[0x14] = '\0';
    param_5[0x15] = '\0';
    param_5[0x16] = '\0';
    param_5[0x17] = '\0';
    return;
  }
  fVar1 = ABS(param_2) / param_4;
  if (fVar1 * 0.5 * param_2 * 0.5 + param_1 < 0.0) {
    FUN_00564580(-param_1,-param_2,param_3,param_4,param_5);
    *(float *)(param_5 + 4) = *(float *)(param_5 + 4) * -1.0;
    *(float *)(param_5 + 8) = *(float *)(param_5 + 8) * -1.0;
    *(float *)(param_5 + 0xc) = *(float *)(param_5 + 0xc) * -1.0;
    *(float *)(param_5 + 0x18) = *(float *)(param_5 + 0x18) * -1.0;
    return;
  }
  fVar2 = param_2 * 0.5 * fVar1 + param_1;
  if (fVar2 < 0.0) {
    param_5[0xc] = '\0';
    param_5[0xd] = '\0';
    param_5[0xe] = '\0';
    param_5[0xf] = '\0';
    param_5[0x10] = '\0';
    param_5[0x11] = '\0';
    param_5[0x12] = '\0';
    param_5[0x13] = '\0';
    param_5[0x14] = '\0';
    param_5[0x15] = '\0';
    param_5[0x16] = '\0';
    param_5[0x17] = '\0';
    fVar1 = (param_2 * param_2) / (param_1 + param_1);
    *(float *)(param_5 + 0x18) = fVar1;
    *(float *)(param_5 + 0x1c) = -(param_2 / fVar1);
    return;
  }
  if (param_2 <= 0.0) {
    fVar3 = -param_4;
    fVar5 = param_2 + param_2;
    fVar4 = SQRT(fVar5 * fVar5 - fVar3 * fVar2 * 4.0);
    fVar2 = (-fVar5 - fVar4) / (fVar3 + fVar3);
    fVar3 = (fVar4 - fVar5) / (fVar3 + fVar3);
    if (((fVar2 < 0.0) || ((0.0 <= fVar3 && (fVar3 <= fVar2)))) && (fVar2 = fVar3, fVar3 < 0.0)) {
      fVar2 = 0.0;
    }
  }
  else {
    fVar2 = SQRT(fVar2 / param_4);
  }
  if (0.0 < param_3) {
    if (param_2 <= 0.0) {
      param_3 = param_2 + param_3;
    }
    param_3 = param_3 / param_4;
    if (param_3 < 0.0) {
      param_3 = 0.0;
    }
    if (param_3 < fVar2) goto LAB_005647c0;
  }
  param_3 = fVar2;
LAB_005647c0:
  *(float *)(param_5 + 0xc) = -param_4;
  *(float *)(param_5 + 0x18) = param_4;
  if (param_2 <= 0.0) {
    *(float *)(param_5 + 0x10) = param_3;
    fVar1 = param_3 + fVar1;
  }
  else {
    *(float *)(param_5 + 0x10) = param_3 + fVar1;
    fVar1 = param_3;
  }
  *(float *)(param_5 + 0x1c) = fVar1;
  if (param_3 < fVar2) {
    param_2 = -param_4 * *(float *)(param_5 + 0x10) + param_2;
    fVar2 = fVar2 - param_3;
    fVar1 = fVar2 * param_2;
    *(float *)(param_5 + 0x14) = ((fVar1 + fVar1) - fVar2 * fVar2 * param_4) / param_2;
    return;
  }
  param_5[0x14] = '\0';
  param_5[0x15] = '\0';
  param_5[0x16] = '\0';
  param_5[0x17] = '\0';
  return;
}
#endif
