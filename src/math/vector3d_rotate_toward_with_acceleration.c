// vector3d_rotate_toward_with_acceleration
//   (Ghidra: vector3d_random_point_in_cone; RENAMED -- the old name is wrong, see below)
// address 0x4cf530, size 615 bytes
// name confidence: 0.55   rewrite confidence: 0.85
//
// MISATTRIBUTED. The phase-2 name and summary claim this "generates a randomly deviated
// direction vector within a cone ... for angular spread/inaccuracy sampling". There is no
// randomness anywhere in the function: it touches neither random_seed_global (0x00719cd0) nor
// local_random_seed (0x00719cd4) nor any RNG helper, and its only callees are acos,
// vector3d_normalize_with_length and vector3d_rotate_about_axis. What it actually implements is
// a bounded-acceleration angular servo: it drives `direction` toward `target_direction` while
// carrying an angular-velocity vector between calls.
//
// Caller evidence (FUN_005625b0 @0x5625b0, the unit aiming update, the only caller):
//   vector3d_random_point_in_cone(puVar4 + 0x92, local_1c, local_18)
//   local_1c = <unit tag +0x264> * 0.033333335   (per-second limit / 30 ticks  -> velocity)
//   local_18 = <unit tag +0x268> * 0.0011111111  (per-second-squared / 900     -> acceleration)
// and the sibling early-out in that caller, taken when both are 0.0, does
//   aiming(+0x8f) = desired_aiming(+0x8c);  angular_velocity(+0x92) = global_origin3d
// which is exactly this function's own degenerate path, and is what pins ESI to the unit's
// current aiming vector and EDI to its desired aiming vector.
//
// register convention: current direction in ESI (rotated in place), target direction in EDI
//   (read-only), and three stack parameters: the persistent angular-velocity vector, the
//   maximum angular velocity, and the angular acceleration (both in radians per tick).
//   // blam-cc: ESI -> direction, EDI -> target_direction,
//   //          stack -> (angular_velocity, maximum_velocity, acceleration)
//
// VERIFIED against the disassembly at 0x4cf530 (objdump -d -M intel). Ghidra's decompile of
// this function is wrong in three places that mattered, all of them x87-stack effects:
//   1. 0x4cf59c/0x4cf5b7  the dot product of direction and target_direction is CLAMPED to
//      [-1, 1] (against ds:0x672ba8 == -1.0 and ds:0x672ac4 == 1.0) and only then passed to
//      0x628140 == acos. Ghidra shows the acos call with no argument at all, and an earlier
//      reading concluded the argument was unrecoverable; it is fully recoverable here.
//   2. 0x4cf64a  the cross product goes into a stack temporary at [esp+0x8], and it is THAT
//      temporary that vector3d_normalize_with_length normalizes (lea ecx,[esp+0x8]) -- ESI is
//      not touched. Only after the call is the normalized temporary scaled by the target speed
//      (0x4cf655..0x4cf675). Ghidra hoists the scale before the call and applies it to the raw
//      cross product, which is a different vector whenever the operands are not unit length.
//   3. 0x4cf736  the final normalize also runs on a copy ([esp+0x14], lea ecx,[esp+0x14]), so
//      the angular-velocity vector keeps its magnitude; the copy becomes the rotation axis and
//      the returned length becomes the rotation angle. The rotate call is
//      vector3d_rotate_about_axis(EAX = direction, ECX = axis, sin, cos) -- mov eax,esi at
//      0x4cf76f -- and the trailing normalize at 0x4cf784 is `mov ecx,esi`, i.e. it
//      renormalizes DIRECTION, not the velocity vector.
// Constants read out of bin/halo.exe: 0x672ba8 = -1.0, 0x672ac4 = 1.0, 0x672ac0 = 0.0,
// 0x672f4c = 1.0000001111620804e-06, and 0x696714 is global_origin3d_pointer -> 0x0065c230.
//
// The control law, once the above is untangled, is textbook: the speed the servo wants is
// min(sqrt(2 * acceleration * angle_remaining), maximum_velocity) -- the v^2 = 2*a*d braking
// curve -- carried on the axis direction x target_direction, and the stored angular velocity is
// allowed to move toward that goal by at most `acceleration` per call.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern real_point3d global_origin3d; // 0x0065c230, reached via global_origin3d_pointer @0x00696714

extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, v in EAX, axis in ECX
extern double acos(double x); // 0x628140, MSVC 7.1 CRT; operand arrives on the x87 stack
extern double cos(double x);  // x87 FCOS
extern double sin(double x);  // x87 FSIN
extern double sqrt(double x); // x87 FSQRT

// Rotates `direction` toward `target_direction`, carrying `angular_velocity` between calls and
// respecting a maximum angular velocity and a maximum angular acceleration (both per tick).
void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, real_vector3d *target_direction,
                                              real_vector3d *angular_velocity, real maximum_velocity,
                                              real acceleration)
{
    real dot;
    real braking_speed;   // 2 * acceleration * angle_remaining, i.e. the square of the goal speed
    real target_speed;
    real_vector3d goal;   // the angular velocity the servo currently wants
    real dx, dy, dz;      // goal - angular_velocity
    real distance_squared;
    real_vector3d axis;
    real speed;

    if (acceleration <= 0.0f && maximum_velocity <= 0.0f) {
        angular_velocity->i = global_origin3d.x;
        angular_velocity->j = global_origin3d.y;
        angular_velocity->k = global_origin3d.z;
        *direction = *target_direction;
        return;
    }

    dot = target_direction->k * direction->k + direction->i * target_direction->i +
          target_direction->j * direction->j;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }

    braking_speed = (real)acos((double)dot) * acceleration;
    braking_speed = braking_speed + braking_speed;
    if (braking_speed < maximum_velocity * maximum_velocity) {
        target_speed = (real)sqrt((double)braking_speed);
    } else {
        target_speed = maximum_velocity;
    }

    // goal = normalize(direction x target_direction) * target_speed
    goal.i = direction->j * target_direction->k - direction->k * target_direction->j;
    goal.j = direction->k * target_direction->i - direction->i * target_direction->k;
    goal.k = direction->i * target_direction->j - direction->j * target_direction->i;
    vector3d_normalize_with_length(&goal); // the returned length is discarded (fstp st(0))
    goal.i = goal.i * target_speed;
    goal.j = goal.j * target_speed;
    goal.k = goal.k * target_speed;

    dx = goal.i - angular_velocity->i;
    dy = goal.j - angular_velocity->j;
    dz = goal.k - angular_velocity->k;
    distance_squared = dx * dx + dy * dy + dz * dz;

    if (acceleration * acceleration <= distance_squared) {
        // Too far to reach this tick: step toward the goal by exactly `acceleration`.
        real step = acceleration / (real)sqrt((double)distance_squared);
        angular_velocity->i = dx * step + angular_velocity->i;
        angular_velocity->j = dy * step + angular_velocity->j;
        angular_velocity->k = dz * step + angular_velocity->k;
    } else if (target_speed < 1.0000001e-06f) {
        // Already there: snap, zero the velocity and stop.
        angular_velocity->i = global_origin3d.x;
        angular_velocity->j = global_origin3d.y;
        angular_velocity->k = global_origin3d.z;
        *direction = *target_direction;
        return;
    } else {
        *angular_velocity = goal;
    }

    // Apply the (possibly just-updated) angular velocity as a rotation of `direction`.
    axis = *angular_velocity;
    speed = vector3d_normalize_with_length(&axis);
    if (speed == 0.0f) {
        return;
    }
    vector3d_rotate_about_axis(direction, &axis, (real)sin((double)speed), (real)cos((double)speed));
    vector3d_normalize_with_length(direction);
}

#if 0
Original Ghidra decompilation (0x4cf530):

void vector3d_random_point_in_cone(float *param_1,float param_2,float param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  undefined *puVar13;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar14;
  float10 fVar15;
  float local_1c;

  puVar13 = PTR_DAT_00696714;
  if ((param_3 < 0.0 != (param_3 == 0.0)) && (param_2 < 0.0 != (param_2 == 0.0))) {
    *param_1 = *(float *)PTR_DAT_00696714;
    param_1[1] = *(float *)(puVar13 + 4);
    param_1[2] = *(float *)(puVar13 + 8);
    *unaff_ESI = *unaff_EDI;
    unaff_ESI[1] = unaff_EDI[1];
    unaff_ESI[2] = unaff_EDI[2];
    return;
  }
  fVar14 = (float10)FUN_00628140();
  fVar14 = fVar14 * (float10)param_3 + fVar14 * (float10)param_3;
  if (fVar14 < (float10)param_2 * (float10)param_2) {
    local_1c = (float)SQRT(fVar14);
  }
  else {
    local_1c = param_2;
  }
  fVar1 = unaff_EDI[2];
  fVar2 = unaff_ESI[1];
  fVar3 = unaff_ESI[2];
  fVar4 = unaff_EDI[1];
  fVar5 = unaff_ESI[2];
  fVar6 = *unaff_EDI;
  fVar7 = unaff_EDI[2];
  fVar8 = *unaff_ESI;
  fVar9 = *unaff_ESI;
  fVar10 = unaff_EDI[1];
  fVar11 = *unaff_EDI;
  fVar12 = unaff_ESI[1];
  vector3d_normalize_with_length();
  puVar13 = PTR_DAT_00696714;
  fVar1 = (fVar1 * fVar2 - fVar3 * fVar4) * local_1c;
  fVar2 = (fVar5 * fVar6 - fVar7 * fVar8) * local_1c;
  fVar3 = (fVar9 * fVar10 - fVar11 * fVar12) * local_1c;
  fVar4 = fVar1 - *param_1;
  fVar7 = fVar2 - param_1[1];
  fVar6 = fVar3 - param_1[2];
  fVar5 = fVar4 * fVar4 + fVar7 * fVar7 + fVar6 * fVar6;
  if (param_3 * param_3 <= fVar5) {
    param_3 = param_3 / SQRT(fVar5);
    *param_1 = fVar4 * param_3 + *param_1;
    param_1[1] = fVar7 * param_3 + param_1[1];
    param_1[2] = fVar6 * param_3 + param_1[2];
  }
  else {
    if (local_1c < 1.0000001e-06) {
      *param_1 = *(float *)PTR_DAT_00696714;
      param_1[1] = *(float *)(puVar13 + 4);
      param_1[2] = *(float *)(puVar13 + 8);
      *unaff_ESI = *unaff_EDI;
      unaff_ESI[1] = unaff_EDI[1];
      unaff_ESI[2] = unaff_EDI[2];
      return;
    }
    *param_1 = fVar1;
    param_1[1] = fVar2;
    param_1[2] = fVar3;
  }
  fVar14 = (float10)vector3d_normalize_with_length();
  if (fVar14 == (float10)0.0) {
    return;
  }
  fVar15 = (float10)fcos(fVar14);
  fVar14 = (float10)fsin(fVar14);
  vector3d_rotate_about_axis((float)fVar14,(float)fVar15);
  vector3d_normalize_with_length();
  return;
}
#endif
