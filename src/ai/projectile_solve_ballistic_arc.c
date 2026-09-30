// projectile_solve_ballistic_arc  (Ghidra: FUN_004beb30; renamed)
// address 0x4beb30, size 744 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/projectiles_types_notes.md "0x4beb30, 0x4bee20, 0x4beec0 are AI
//   ballistic-aiming helpers, not projectile state... They take a Projectile tag pointer and
//   plain vectors, never an object index... 0x4beb30 is pure math (gravity 0x0069c52c plus the
//   float pool)... UNSURE of the exact name"; src/projectiles/README.md "Misattributed
//   functions": "AI ballistic aiming: the swept/gravity firing-solution solver. Takes a
//   Projectile tag pointer and plain vectors, never an object index". Its only caller is the
//   dispatcher projectile_get_aiming_vector (0x4beec0, this batch), whose two callers are both
//   in src/ai (actor_solve_grenade_lob, actor_get_grenade_launch_velocity). cleanup pass 4
//   orphan pass: neither projectiles nor items claimed this address; picked up here in ai.
// register convention: `objdump -d -M intel --start-address=0x4beb30 --stop-address=0x4bee20
//   bin/halo.exe` cross-checked against the caller (0x4beec0's ballistic branch): EAX = target
//   position (real_point3d *), ECX = origin position (real_point3d *), ESI = out_direction
//   (real_vector3d *), EDI = optional max_speed_override (real *, nullable); nine plain stack
//   arguments follow (speed_limit, gravity_scale, max_time, use_high_arc, out_speed,
//   out_time_of_flight, out_range, and two more out pointers the caller always passes NULL for
//   in this batch's only reachable call path).
//   // blam-cc: EAX -> target, ECX -> origin, ESI -> out_direction, EDI -> max_speed_override,
//   //          stack -> (speed_limit, gravity_scale, max_time, use_high_arc, out_speed,
//   //                    out_time_of_flight, out_range, out_half_gravity_term, out_horizontal_speed)
// UNSURE (major): this is a gravity-arc firing-solution solver (a quadratic in the vertical
//   launch velocity, gated by a speed_limit / max_speed_override ceiling and an optional
//   max_time constraint), but the physical meaning of a few intermediate terms -- in particular
//   why the quadratic is built from `gravity*dz - chosen_max^2` rather than a more familiar
//   range-equation form -- is not confirmed against a reference implementation. The arithmetic
//   below is translated instruction-by-instruction from the disassembly; only the *names* are
//   inferred.
// UNSURE: out_speed_times_time, out_half_gravity_term and out_horizontal_speed are always
//   passed NULL by this batch's one traced call path (projectile_get_aiming_vector always
//   passes 0/0 for the last two, and out_range is written but its caller-side name is not
//   established), so their exact roles are translated but not independently verified.

#include "tags.h"
#include "math.h"

extern float k_physics_gravity; // 0x0069c52c, k_physics_gravity (types/physics.h); named
    // k_physics_gravity here to match src/ai/actor_solve_grenade_lob.c's existing extern
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math module
extern const real_vector3d *global_up3d_pointer; // 0x00696720 == 0x0065c224 (types/math.h)
extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

// VERIFIED (logic) against disassembly 0x4beb30..0x4bee17 (2026-09-30): every branch, constant, stack argument slot and
//   output store was traced through the x87 stack and matches. STILL-UNSURE: the original keeps intermediates in 80-bit
//   registers (gravity, shallow_speed, discriminant are never rounded between operations), the C rounds each to float,
//   so tiny gravity scales can differ (underflow of gravity^2 -> NaN) even though the algorithm is identical.
// Solves a gravity-arc firing solution from *origin to *target: gravity is
// k_physics_gravity * gravity_scale (clamped to >= 0), the launch speed is capped by
// speed_limit (or by *max_speed_override when non-NULL, which also skips the max_time-derived
// tightening of that cap), and use_high_arc selects the lofted root of the two solutions to the
// vertical-velocity quadratic when a real root exists. The resulting unit direction is written
// to *out_direction (falling back to the straight-line direction to the target, and then to
// the global up vector, if the solved direction is degenerate). Any of the out pointers may be
// NULL. Returns 1 when a real arc solution was found and used, 0 when the shallow/no-solution
// fallback time and speed were used instead.
uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin,
    real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc,
    real_vector3d *out_direction, real *max_speed_override, real *out_speed,
    real *out_time_of_flight, real *out_range, real *out_half_gravity_term,
    real *out_horizontal_speed)
{
    // x87 dataflow (0x4beb30..0x4bee17): every `fst/fstp dword` below rounds to float (real), everything else stays in an
    // 80-bit register, modelled with double. This matters: the root2 step subtracts two nearly equal numbers
    // (sqrt(a^2 - disc) vs a), which loses most of its digits if the intermediates are rounded to float.
    real dx, dy, dz;
    real dxy2, qg, twoqg, distance_sq, disc, shallow_time, dzg, chosen_max, a, disc2, root2, t;
    double g, d2, disc_ext, neg_root, shallow_speed_ext, dzg_ext, s_ext;
    double inv_t, vertical_velocity_ext;
    real_vector3d dir;
    real length;
    real horizontal_speed, vertical_velocity;
    uint8_t used_root = 1;

    dx = target->x - origin->x;
    dy = target->y - origin->y;
    dz = target->z - origin->z;
    dxy2 = (real)((double)dy * dy + (double)dx * dx);                   // 0x4beb57..0x4beb69 -> [esp+0x1c]

    g = (double)k_physics_gravity * (double)gravity_scale;             // 0x4beb6d: stays in st(0)
    if (g < 0.0) {
        g = 0.0;
    }
    qg = (real)(g * g * 0.25);                                         // 0x4beb8e..0x4beb98 -> [esp+8]
    d2 = (double)dz * dz + (double)dxy2;                               // 0x4beb9c..0x4bebba
    distance_sq = (real)d2;                                            // [esp+0x10]
    disc_ext = d2 * (double)qg * 4.0;
    disc = (real)disc_ext;                                             // [esp+0x14]
    neg_root = -sqrt(disc_ext);                                        // fsqrt; fchs
    twoqg = qg + qg;                                                   // [esp+0x18]
    shallow_time = (real)sqrt((-1.0 / (double)twoqg) * neg_root);      // 0x4bebc8..0x4bebd6 -> [esp+0x1c]
    dzg_ext = g * (double)dz;
    dzg = (real)dzg_ext;                                               // [esp+0xc]
    s_ext = dzg_ext - neg_root;
    shallow_speed_ext = (s_ext < 0.0) ? 0.0 : sqrt(s_ext);             // `test ah,5; jp`: NaN takes the sqrt

    if (max_speed_override != (real *)0) {
        chosen_max = *max_speed_override;
    } else {
        chosen_max = speed_limit;
        if ((max_time != (real *)0) && (0.0f < *max_time)) {
            double scaled = (double)shallow_time * (double)*max_time;
            double scaled_sq = scaled * scaled;
            double sum = scaled_sq * (double)qg + (double)distance_sq / scaled_sq;
            double candidate = sqrt((double)dzg + sum);

            if ((double)speed_limit > candidate) {
                chosen_max = (real)candidate;                          // 0x4bec55
            }
        }
    }

    // 0x4bec5d: proceed only when chosen_max >= shallow_speed
    if (!((double)chosen_max < shallow_speed_ext)) {
        double a_ext = (double)dzg - (double)chosen_max * (double)chosen_max;
        double disc2_ext;

        a = (real)a_ext;                                               // 0x4bec7a -> [esp+0x3c]
        disc2_ext = a_ext * (double)a - (double)disc;                  // fmul (unrounded a_ext) * a; fsub disc
        disc2 = (real)disc2_ext;                                       // 0x4bec86 -> [esp+0x14]
        if ((a < 0.0f) && (0.0f <= disc2)) {
            double root2_ext = (sqrt((double)disc2) * (double)(int)((use_high_arc != 0) * 2 - 1) - (double)a) / (double)twoqg;

            root2 = (real)root2_ext;                                   // 0x4becd1
            if (root2_ext > 0.0) {
                t = (real)sqrt((double)root2);
                goto have_root;
            }
        }
    }
    used_root = 0;
    t = shallow_time;
    chosen_max = (real)shallow_speed_ext;                              // 0x4becf4

have_root:
    inv_t = 1.0 / (double)t;
    dir.i = (real)((double)dx * inv_t);
    dir.j = (real)((double)dy * inv_t);
    vertical_velocity_ext = inv_t * (double)dz + (double)t * g * 0.5;
    dir.k = (real)vertical_velocity_ext;
    vertical_velocity = (real)vertical_velocity_ext;
    horizontal_speed = (real)sqrt((double)dir.j * dir.j + (double)dir.i * dir.i); // 0x4bed38..0x4bed4a, before the normalise

    length = vector3d_normalize_with_length(&dir);
    if (length == 0.0f) {
        used_root = 0;
        dir.i = dx;
        dir.j = dy;
        dir.k = dz;
        length = vector3d_normalize_with_length(&dir);
        if (length == 0.0f) {
            dir = *global_up3d_pointer;
            used_root = 0;
        }
    }

    out_direction->i = dir.i;
    out_direction->j = dir.j;
    out_direction->k = dir.k;

    if (out_range != (real *)0) {
        *out_range = (real)((double)t * (double)chosen_max);
    }
    if (out_speed != (real *)0) {
        *out_speed = chosen_max;
    }
    if (out_half_gravity_term != (real *)0) {
        *out_half_gravity_term = vertical_velocity;
    }
    if (out_horizontal_speed != (real *)0) {
        *out_horizontal_speed = horizontal_speed;
    }
    if (out_time_of_flight != (real *)0) {
        *out_time_of_flight = t;
    }
    return used_root;
}

#if 0
Original Ghidra decompilation (0x4beb30):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1
FUN_004beb30(float param_1,float param_2,float *param_3,char param_4,float *param_5,float *param_6,
            float *param_7,float *param_8,float *param_9)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_EAX;
  float fVar9;
  float *in_ECX;
  float fVar10;
  float fVar11;
  undefined1 uVar12;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar13;

  fVar1 = *in_EAX - *in_ECX;
  uVar12 = 1;
  fVar2 = in_EAX[1] - in_ECX[1];
  fVar3 = in_EAX[2] - in_ECX[2];
  fVar6 = _DAT_0069c52c * param_2;
  if (fVar6 < 0.0) {
    fVar6 = 0.0;
  }
  fVar8 = fVar6 * fVar6 * 0.25;
  fVar11 = fVar3 * fVar3 + fVar1 * fVar1 + fVar2 * fVar2;
  fVar5 = fVar11 * fVar8 * 4.0;
  fVar7 = -SQRT(fVar5);
  fVar10 = SQRT((-1.0 / (fVar8 + fVar8)) * fVar7);
  fVar9 = fVar6 * fVar3;
  fVar7 = fVar9 - fVar7;
  if (0.0 <= fVar7) {
    fVar7 = SQRT(fVar7);
  }
  else {
    fVar7 = 0.0;
  }
  if (unaff_EDI == (float *)0x0) {
    param_2 = param_1;
    if (((param_3 != (float *)0x0) && (0.0 < *param_3)) &&
       (fVar4 = fVar10 * *param_3, fVar4 = fVar4 * fVar4,
       fVar11 = SQRT(fVar9 - -(fVar4 * fVar8 + fVar11 / fVar4)), fVar11 < param_1)) {
      param_2 = fVar11;
    }
  }
  else {
    param_2 = *unaff_EDI;
  }
  if (fVar7 <= param_2) {
    fVar9 = fVar9 - param_2 * param_2;
    fVar5 = fVar9 * fVar9 - fVar5;
    if (((fVar9 < 0.0) && (0.0 <= fVar5)) &&
       (fVar9 = (SQRT(fVar5) * (float)(int)((uint)(param_4 != '\0') * 2 + -1) - fVar9) /
                (fVar8 + fVar8), 0.0 < fVar9)) {
      param_1 = SQRT(fVar9);
      goto LAB_004becfe;
    }
  }
  uVar12 = 0;
  param_1 = fVar10;
  param_2 = fVar7;
LAB_004becfe:
  fVar9 = 1.0 / param_1;
  fVar5 = fVar1 * fVar9;
  fVar7 = fVar2 * fVar9;
  fVar6 = param_1 * fVar6 * 0.5 + fVar9 * fVar3;
  fVar13 = (float10)vector3d_normalize_with_length();
  fVar9 = fVar7;
  fVar10 = fVar5;
  fVar11 = fVar6;
  if ((float10)0.0 == fVar13) {
    uVar12 = 0;
    fVar13 = (float10)vector3d_normalize_with_length();
    fVar9 = fVar2;
    fVar10 = fVar1;
    fVar11 = fVar3;
    if ((float10)0.0 == fVar13) {
      fVar9 = *(float *)(PTR_DAT_00696720 + 4);
      fVar10 = *(float *)PTR_DAT_00696720;
      fVar11 = *(float *)(PTR_DAT_00696720 + 8);
      uVar12 = 0;
    }
  }
  *unaff_ESI = fVar10;
  unaff_ESI[1] = fVar9;
  unaff_ESI[2] = fVar11;
  if (param_7 != (float *)0x0) {
    *param_7 = param_1 * param_2;
  }
  if (param_5 != (float *)0x0) {
    *param_5 = param_2;
  }
  if (param_8 != (float *)0x0) {
    *param_8 = fVar6;
  }
  if (param_9 != (float *)0x0) {
    *param_9 = SQRT(fVar5 * fVar5 + fVar7 * fVar7);
  }
  if (param_6 != (float *)0x0) {
    *param_6 = param_1;
  }
  return uVar12;
}
#endif
