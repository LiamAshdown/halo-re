// vector3d_rotate_toward_bounded  (Ghidra: FUN_00564ae0 / FUN_00565040; renamed for this rewrite)
// address 0x564ae0, size 1643 bytes (0x564ae0..0x56514a)
// name confidence: 0.45   rewrite confidence: 0.9 (VERIFIED end to end against objdump 0x564ae0..0x56514a)
//
// NOT TWO FUNCTIONS. modules.json / the phase-2 pass list this as two separate addresses,
// 0x564ae0 (1376 bytes) and 0x565040 (267 bytes, inherited the misleading name
// "unit_scripting_set_current_vitality" from an unrelated phase-2 guess). They are one function:
// objdump shows no `ret` anywhere between them (0x565040 begins mid-instruction-stream, with no
// prologue -- the code at 0x564fd0..0x565044 is one unbroken `fcos`/`fsin`/`fmul` chain that
// flows straight across the 0x565040 boundary), and Ghidra's own decompile of FUN_00564ae0
// already contains the entire body through the real final `ret`s at 0x56512c/0x56514a -- i.e.
// Ghidra internally treated this as a single function and only the address-list metadata split
// it in two. This file is that whole function, address 0x564ae0, size 1643 bytes. 0x565040 is
// recorded as a non-function fragment of this address in out/phase4/orphans_notes.md.
//
// evidence: src/units/README.md calls this "the bounded angular servo"; out/phase4/units_types_notes.md
//   groups it with the other pure-math vector/basis helpers skipped from `units`. Its callees
//   (vector3d_normalize_with_length, matrix4x3_transform_normal, vector3d_rotate_about_axis,
//   acos via FUN_00628140) and its v^2=2ad control law are the SAME ones
//   src/math/vector3d_rotate_toward_with_acceleration.c (0x4cf530) already verified and documents
//   in detail -- that function is the plain "rotate current toward target, carrying an angular
//   velocity" servo; this one is the bounded sibling: it first clamps the target direction's
//   azimuth/elevation into a `bounds` cone, optionally works in a local space given by a
//   `transform` matrix instead of world space, and drives azimuth and elevation as two
//   independently bounded-acceleration axes (src/math/bounded_ramp_profile_build.c /
//   _synchronize.c / _evaluate.c, also written by this pass) instead of one combined
//   axis-angle step.
// register convention (objdump 0x564ae0..0x56514a; three real callers found via
//   `objdump -d bin/halo.exe | grep "call.*564ae0"`: 0x55baee in biped_update_facing,
//   0x562f13 and 0x5630c2 in unit_update -- confirmed against all three): `current` and the
//   4 remaining floats/pointers are ordinary stack parameters in the order below; `target` is a
//   genuine 6th argument passed in ECX and `transform` a genuine 7th passed in ESI (NULL at the
//   biped_update_facing call site, non-NULL -- a real per-call local matrix -- at both
//   unit_update call sites, so the transform path is live code, not dead).
//   // blam-cc: stack -> (current, velocity, bounds, max_velocity, max_acceleration),
//   //          ECX -> target, ESI -> transform (nullable)
//
// REVIEW (orphan pass 4, objdump 0x564c43..0x56514a): three behaviour fixes. (1) In the full-circle
//   case the target azimuth is reduced by 2*pi when it is ABOVE bounds[1] (`fcomp [edx+4]; test
//   ah,0x41; jne skip`); the draft subtracted when it was below. (2) With a transform, the final
//   step writes current = normalize(transform * dir) (0x56510d: EAX = current, EDX = &dir) and
//   leaves the velocity in the working space; the draft transformed and normalized the velocity
//   instead and never wrote current. (3) Every angle clamp is now `< lo` / `> hi`, which is how the
//   x87 tests treat a NaN (it passes unchanged); the draft's `lo <= x` forms clamped a NaN.
// UNSURE (extensive; this is one of the hardest functions in the module):
// - src/units/biped_update_facing.c and src/units/unit_update.c already declare
//   `extern void FUN_00564ae0(real_vector3d *current, real_vector3d *velocity, float *bounds,
//   float max_velocity, float max_acceleration);` and call it with only those 5 arguments,
//   leaving `target`/`transform` as unmodelled register state (their own header comments call
//   this "math module (skipped)"). This file's own signature is the true one, independently
//   confirmed against objdump; the two consumer files are not touched by this pass.
// - FUN_00628140 (0x628140) is `acos`, per vector3d_rotate_toward_with_acceleration.c's own
//   verified identification of the same address; reused here without re-deriving it.
// - the exact shape of the two-phase clamp-then-rebuild-direction block (`target_in_range`,
//   `full_circle`) is transcribed from Ghidra's decompile, which matches every spot check this
//   rewrite made against objdump, but was not re-derived instruction-by-instruction end to end;
//   treat the control flow as reliable and the *names* (`target_in_range`, `full_circle`,
//   `predicted_azimuth_delta`) as this rewrite's own inference.
// - the caller-side `transform` buffer's producer is not traced; this file only documents what
//   IT reads (real_matrix4x3, per types/math.h -- the dot-product pattern at the top exactly
//   matches matrix4x3_inverse_transform_normal's formula, and the forward transform near the end
//   is a real call to matrix4x3_transform_normal).

#include "tags.h"
#include "math.h"

extern double acos(double x);  // 0x628140, MSVC 7.1 CRT; see vector3d_rotate_toward_with_acceleration.c
extern double cos(double x);   // x87 FCOS
extern double sin(double x);   // x87 FSIN
extern double sqrt(double x);  // x87 FSQRT
extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction (see quaternion_to_axis_angle.c)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0
extern real_point3d global_origin3d; // 0x0065c230, reached via global_origin3d_pointer @0x00696714

extern void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity,
    real max_acceleration, uint8_t *profile); // 0x564580, this module
extern void bounded_ramp_profile_synchronize(uint8_t *profile_a, uint8_t *profile_b, real max_acceleration); // 0x564840, this module
extern uint8_t bounded_ramp_profile_evaluate(uint8_t *profile, real time, real start_position,
    real *out_position, real start_velocity, real *out_velocity); // 0x564990, this module

#define K_TWO_PI 6.2831855f
#define K_PI 3.1415927f

// Rotates `current` toward `target`, carrying `velocity` between calls, while independently
// bounding the azimuth and elevation of the direction the servo is allowed to reach to the
// [az_min,az_max]/[el_min,el_max] cone in `bounds`. When `transform` is non-NULL, `current` and
// `target` are worked on in that matrix's local space (and the final result transformed back to
// world space) instead of world space directly.
void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds,
    real max_velocity, real max_acceleration, real_vector3d *target, real_matrix4x3 *transform)
{
    real current_x, current_y, current_z;
    real target_x, target_y, target_z;
    real current_azimuth, current_elevation;
    real target_azimuth, target_elevation;
    uint8_t full_circle;      // bounds[1]-bounds[0] already spans (approximately) a full circle
    uint8_t target_in_range = 1;
    real clamped_x, clamped_y, clamped_z; // the clamped target direction
    real_vector3d axis;
    real axis_length;
    real predicted_azimuth_delta, predicted_elevation_delta;
    real azimuth_error, elevation_error;
    uint8_t azimuth_profile[0x20];
    uint8_t elevation_profile[0x20];
    uint8_t azimuth_done, elevation_done;

    if (transform != 0) {
        // Into the transform's local space (matrix4x3_inverse_transform_normal's own formula).
        current_x = current->i * transform->forward.i + current->j * transform->forward.j + current->k * transform->forward.k;
        current_y = current->i * transform->left.i + current->j * transform->left.j + current->k * transform->left.k;
        current_z = current->i * transform->up.i + current->j * transform->up.j + current->k * transform->up.k;
        target_x = target->i * transform->forward.i + target->j * transform->forward.j + target->k * transform->forward.k;
        target_y = target->i * transform->left.i + target->j * transform->left.j + target->k * transform->left.k;
        target_z = target->i * transform->up.i + target->j * transform->up.j + target->k * transform->up.k;
    } else {
        current_x = current->i;
        current_y = current->j;
        current_z = current->k;
        target_x = target->i;
        target_y = target->j;
        target_z = target->k;
    }

    full_circle = (uint8_t)(-0.0001f < (bounds[1] - bounds[0]) - K_TWO_PI);

    current_azimuth = (real)atan2((double)current_y, (double)current_x);
    current_elevation = (real)atan2((double)current_z, (double)sqrt((double)(current_x * current_x + current_y * current_y)));
    target_azimuth = (real)atan2((double)target_y, (double)target_x);
    target_elevation = (real)atan2((double)target_z, (double)sqrt((double)(target_x * target_x + target_y * target_y)));

    // 0x564c43..0x564c9a. Every clamp in this function is `fcomp lo; test ah,5; jp` then
    // `fcomp hi; test ah,0x41; jne`, so a NaN angle passes both tests unchanged; the tests are
    // written `< lo` / `> hi` to keep that.
    if (full_circle) {
        if (target_azimuth < bounds[0]) {
            target_azimuth = target_azimuth + K_TWO_PI;
        } else if (target_azimuth > bounds[1]) {
            target_azimuth = target_azimuth - K_TWO_PI;
        }
    } else if (target_azimuth < bounds[0]) {
        target_azimuth = bounds[0];
        target_in_range = 0;
    } else if (target_azimuth > bounds[1]) {
        target_azimuth = bounds[1];
        target_in_range = 0;
    }

    // 0x564c9c..0x564dde
    if (target_elevation < bounds[2]) {
        target_elevation = bounds[2];
    } else if (target_elevation > bounds[3]) {
        target_elevation = bounds[3];
    } else if (target_in_range) {
        // Fully in range: the clamped direction is just the target, unchanged.
        clamped_x = target->i;
        clamped_y = target->j;
        clamped_z = target->k;
        goto velocity_prediction;
    }

    clamped_x = (real)cos((double)target_elevation) * (real)cos((double)target_azimuth);
    clamped_y = (real)cos((double)target_elevation) * (real)sin((double)target_azimuth);
    clamped_z = (real)sin((double)target_elevation);
    if (transform != 0) {
        real_vector3d clamped;
        clamped.i = clamped_x; clamped.j = clamped_y; clamped.k = clamped_z;
        matrix4x3_transform_normal(&clamped, &clamped, transform);
        vector3d_normalize_with_length(&clamped);
        clamped_x = clamped.i; clamped_y = clamped.j; clamped_z = clamped.k;
    }

velocity_prediction:
    axis.i = velocity->i;
    axis.j = velocity->j;
    axis.k = velocity->k;
    axis_length = vector3d_normalize_with_length(&axis);
    if (axis_length == 0.0f) {
        predicted_azimuth_delta = 0.0f;
        predicted_elevation_delta = 0.0f;
    } else {
        real_vector3d rotated;
        real new_azimuth, new_elevation;
        rotated.i = current_x; rotated.j = current_y; rotated.k = current_z;
        vector3d_rotate_about_axis(&rotated, &axis, (real)sin((double)axis_length), (real)cos((double)axis_length));
        new_azimuth = (real)atan2((double)rotated.j, (double)rotated.i);
        new_elevation = (real)atan2((double)rotated.k, (double)sqrt((double)(rotated.i * rotated.i + rotated.j * rotated.j)));
        predicted_azimuth_delta = new_azimuth - current_azimuth;
        predicted_elevation_delta = new_elevation - current_elevation;
    }

    azimuth_error = current_azimuth - target_azimuth;
    elevation_error = current_elevation - target_elevation;
    if (full_circle) {                      // 0x564e15..0x564e49
        if (azimuth_error > K_PI) {
            azimuth_error = azimuth_error - K_TWO_PI;
        } else if (azimuth_error < -K_PI) {
            azimuth_error = azimuth_error + K_TWO_PI;
        }
    }

    bounded_ramp_profile_build(azimuth_error, predicted_azimuth_delta, max_velocity, max_acceleration, azimuth_profile);
    bounded_ramp_profile_build(elevation_error, predicted_elevation_delta, max_velocity, max_acceleration, elevation_profile);
    bounded_ramp_profile_synchronize(azimuth_profile, elevation_profile, max_acceleration);
    azimuth_done = bounded_ramp_profile_evaluate(azimuth_profile, 1.0f, azimuth_error, &azimuth_error,
                                                  predicted_azimuth_delta, &predicted_azimuth_delta);
    elevation_done = bounded_ramp_profile_evaluate(elevation_profile, 1.0f, elevation_error, &elevation_error,
                                                    predicted_elevation_delta, &predicted_elevation_delta);

    if (azimuth_done && elevation_done) {
        // Reached the clamped target this tick: snap to it and zero the angular velocity.
        current->i = clamped_x;
        current->j = clamped_y;
        current->k = clamped_z;
        velocity->i = global_origin3d.x;
        velocity->j = global_origin3d.y;
        velocity->k = global_origin3d.z;
        return;
    }

    // Not there yet: step azimuth/elevation by this tick's ramp-profile output and rebuild a
    // direction vector from the new (unclamped-by-bounds, but error-relative) angles.
    {
        real new_azimuth = azimuth_error + target_azimuth;
        real new_elevation = elevation_error + target_elevation;
        real dir_x, dir_y, dir_z;
        real ref_x, ref_y, ref_z;
        real dot, cross_x, cross_y, cross_z;

        if (full_circle) {                  // 0x564f5b..0x564f78
            if (new_azimuth < bounds[0]) {
                new_azimuth = new_azimuth + K_TWO_PI;
            } else if (new_azimuth > bounds[1]) {
                new_azimuth = new_azimuth - K_TWO_PI;
            }
        } else if (new_azimuth < bounds[0]) { // 0x564f7a..0x564f98
            new_azimuth = bounds[0];
        } else if (new_azimuth > bounds[1]) {
            new_azimuth = bounds[1];
        }

        if (new_elevation < bounds[2]) {    // 0x564fa2..0x564fca
            new_elevation = bounds[2];
        } else if (new_elevation > bounds[3]) {
            new_elevation = bounds[3];
        }

        dir_x = (real)cos((double)new_elevation) * (real)cos((double)new_azimuth);
        dir_y = (real)cos((double)new_elevation) * (real)sin((double)new_azimuth);
        dir_z = (real)sin((double)new_elevation);
        ref_x = (real)cos((double)(new_elevation + predicted_elevation_delta)) * (real)cos((double)(new_azimuth + predicted_azimuth_delta));
        ref_y = (real)cos((double)(new_elevation + predicted_elevation_delta)) * (real)sin((double)(new_azimuth + predicted_azimuth_delta));
        ref_z = (real)sin((double)(new_elevation + predicted_elevation_delta));

        dot = dir_z * ref_z + ref_y * dir_y + ref_x * dir_x; // x87 order, 0x56501c..0x565034
        if (dot < -1.0f) {
            dot = -1.0f;
        } else if (1.0f < dot) {
            dot = 1.0f;
        }

        cross_x = dir_y * ref_z - ref_y * dir_z;
        cross_y = dir_z * ref_x - ref_z * dir_x;
        cross_z = ref_y * dir_x - dir_y * ref_x;

        // velocity <- normalize(cross(dir, ref)) * min(acos(dot), max_velocity), in the working
        // space; current <- dir, taken back to world space through `transform` when there is
        // one (0x56510d..0x565121: EAX = current, EDX = &dir, stack transform, then normalize).
        velocity->i = cross_x; velocity->j = cross_y; velocity->k = cross_z;
        axis_length = vector3d_normalize_with_length(velocity);
        {
            real speed = (real)acos((double)dot);
            if (max_velocity < speed) {
                speed = max_velocity;
            }
            velocity->i = velocity->i * speed;
            velocity->j = velocity->j * speed;
            velocity->k = velocity->k * speed;
        }
        if (transform != 0) {
            real_vector3d direction;
            direction.i = dir_x; direction.j = dir_y; direction.k = dir_z;
            matrix4x3_transform_normal(current, &direction, transform);
            vector3d_normalize_with_length(current);
            return;
        }
        current->i = dir_x;
        current->j = dir_y;
        current->k = dir_z;
    }
}

#if 0
Original Ghidra decompilation (0x564ae0), covering both this address and 0x565040 (Ghidra
decompiled the whole function as one body under the FUN_00564ae0 entry point, matching the
"one function, two address-list entries" finding above):

void FUN_00564ae0(float *param_1,float *param_2,float *param_3,float param_4,undefined4 param_5)

{
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  undefined *puVar5;
  char cVar6;
  char cVar7;
  float *in_ECX;
  int unaff_ESI;
  bool bVar8;
  float10 fVar9;
  float10 fVar10;
  float10 fVar11;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  undefined1 local_40 [32];
  undefined1 local_20 [32];

  if (unaff_ESI == 0) {
    local_70 = *param_1;
    local_6c = param_1[1];
    local_68 = param_1[2];
    local_84 = *in_ECX;
    local_80 = in_ECX[1];
    local_7c = in_ECX[2];
    fVar9 = (float10)local_7c;
  }
  else {
    fVar1 = *param_1;
    fVar2 = param_1[1];
    fVar3 = param_1[2];
    local_70 = fVar2 * *(float *)(unaff_ESI + 8) +
               fVar3 * *(float *)(unaff_ESI + 0xc) + fVar1 * *(float *)(unaff_ESI + 4);
    local_6c = fVar3 * *(float *)(unaff_ESI + 0x18) +
               fVar1 * *(float *)(unaff_ESI + 0x10) + fVar2 * *(float *)(unaff_ESI + 0x14);
    local_68 = fVar2 * *(float *)(unaff_ESI + 0x20) +
               fVar3 * *(float *)(unaff_ESI + 0x24) + fVar1 * *(float *)(unaff_ESI + 0x1c);
    fVar9 = (float10)*in_ECX;
    fVar10 = (float10)in_ECX[1];
    fVar11 = (float10)in_ECX[2];
    local_84 = (float)(fVar10 * (float10)*(float *)(unaff_ESI + 8) +
                      fVar11 * (float10)*(float *)(unaff_ESI + 0xc) +
                      fVar9 * (float10)*(float *)(unaff_ESI + 4));
    local_80 = (float)(fVar10 * (float10)*(float *)(unaff_ESI + 0x14) +
                      fVar11 * (float10)*(float *)(unaff_ESI + 0x18) +
                      fVar9 * (float10)*(float *)(unaff_ESI + 0x10));
    fVar9 = fVar10 * (float10)*(float *)(unaff_ESI + 0x20) +
            fVar11 * (float10)*(float *)(unaff_ESI + 0x24) +
            fVar9 * (float10)*(float *)(unaff_ESI + 0x1c);
  }
  bVar4 = true;
  bVar8 = -0.0001 < (param_3[1] - *param_3) - 6.2831855;
  fVar10 = (float10)fpatan((float10)local_6c,(float10)local_70);
  local_64 = (float)fVar10;
  fVar10 = (float10)fpatan((float10)local_68,
                           SQRT((float10)local_70 * (float10)local_70 +
                                (float10)local_6c * (float10)local_6c));
  local_60 = (float)fVar10;
  fVar10 = (float10)fpatan((float10)local_80,(float10)local_84);
  local_78 = (float)fVar10;
  fVar9 = (float10)fpatan(fVar9,SQRT((float10)local_84 * (float10)local_84 +
                                     (float10)local_80 * (float10)local_80));
  local_74 = (float)fVar9;
  if (bVar8) {
    if (*param_3 <= local_78) {
      if (param_3[1] < local_78) {
        local_78 = local_78 - 6.2831855;
      }
    }
    else {
      local_78 = local_78 + 6.2831855;
    }
  }
  else {
    if (*param_3 <= local_78) {
      if (local_78 <= param_3[1]) goto LAB_00564c9c;
      local_78 = param_3[1];
    }
    else {
      local_78 = *param_3;
    }
    bVar4 = false;
  }
LAB_00564c9c:
  if (param_3[2] <= local_74) {
    if (param_3[3] < local_74) {
      local_74 = param_3[3];
    }
    else if (bVar4) {
      local_58 = *in_ECX;
      local_54 = in_ECX[1];
      local_50 = in_ECX[2];
      goto LAB_00564cfb;
    }
  }
  else {
    local_74 = param_3[2];
  }
  fVar9 = (float10)fcos((float10)local_74);
  fVar10 = (float10)fcos((float10)local_78);
  local_58 = (float)(fVar10 * fVar9);
  fVar10 = (float10)fsin((float10)local_78);
  local_54 = (float)(fVar10 * fVar9);
  fVar9 = (float10)fsin((float10)local_74);
  local_50 = (float)fVar9;
  if (unaff_ESI != 0) {
    matrix4x3_transform_normal();
    vector3d_normalize_with_length();
  }
LAB_00564cfb:
  local_4c = *param_2;
  local_48 = param_2[1];
  local_44 = param_2[2];
  fVar9 = (float10)vector3d_normalize_with_length();
  if (fVar9 == (float10)0.0) {
    local_70 = 0.0;
    local_6c = 0.0;
  }
  else {
    fVar10 = (float10)fcos(fVar9);
    local_80 = local_6c;
    local_7c = local_68;
    local_84 = local_70;
    fVar9 = (float10)fsin(fVar9);
    vector3d_rotate_about_axis((float)fVar9,(float)fVar10);
    fVar9 = (float10)fpatan((float10)local_80,(float10)local_84);
    fVar10 = (float10)fpatan((float10)local_7c,
                             SQRT((float10)local_84 * (float10)local_84 +
                                  (float10)local_80 * (float10)local_80));
    local_70 = (float)(fVar9 - (float10)local_64);
    local_6c = (float)(fVar10 - (float10)local_60);
  }
  fVar1 = local_70;
  local_84 = local_64 - local_78;
  local_80 = local_60 - local_74;
  if (bVar8) {
    if (local_84 <= 3.1415927) {
      if (local_84 < -3.1415927) {
        local_84 = local_84 + 6.2831855;
      }
    }
    else {
      local_84 = local_84 - 6.2831855;
    }
  }
  FUN_00564580(local_84,local_70,param_4,param_5,local_40);
  fVar2 = local_6c;
  FUN_00564580(local_80,local_6c,param_4,param_5,local_20);
  FUN_00564840(param_5);
  cVar6 = FUN_00564990(0x3f800000,local_84,&local_84,fVar1,&local_70);
  cVar7 = FUN_00564990(0x3f800000,local_80,&local_80,fVar2,&local_6c);
  if ((cVar6 != '\0') && (cVar7 != '\0')) {
    *param_1 = local_58;
    param_1[1] = local_54;
    puVar5 = PTR_DAT_00696714;
    param_1[2] = local_50;
    *param_2 = *(float *)puVar5;
    param_2[1] = *(float *)(puVar5 + 4);
    param_2[2] = *(float *)(puVar5 + 8);
    return;
  }
  fVar9 = (float10)local_84 + (float10)local_78;
  local_80 = local_80 + local_74;
  if (bVar8) {
    if ((float10)*param_3 <= fVar9) {
      if ((float10)param_3[1] < fVar9) {
        fVar9 = fVar9 - (float10)6.2831855;
      }
    }
    else {
      fVar9 = fVar9 + (float10)6.2831855;
    }
  }
  else if ((float10)*param_3 <= fVar9) {
    if ((float10)param_3[1] < fVar9) {
      fVar9 = (float10)param_3[1];
    }
  }
  else {
    fVar9 = (float10)*param_3;
  }
  if (param_3[2] <= local_80) {
    if (param_3[3] < local_80) {
      local_80 = param_3[3];
    }
  }
  else {
    local_80 = param_3[2];
  }
  fVar10 = (float10)fcos((float10)local_80);
  fVar11 = (float10)fcos(fVar9);
  local_64 = (float)(fVar11 * fVar10);
  fVar11 = (float10)fsin(fVar9);
  local_60 = (float)(fVar11 * fVar10);
  fVar10 = (float10)fsin((float10)local_80);
  local_5c = (float)fVar10;
  fVar10 = (float10)fcos((float10)local_6c + (float10)local_80);
  fVar11 = (float10)fcos(fVar9 + (float10)local_70);
  local_58 = (float)(fVar11 * fVar10);
  fVar9 = (float10)fsin(fVar9 + (float10)local_70);
  local_54 = (float)(fVar9 * fVar10);
  fVar9 = (float10)fsin((float10)local_6c + (float10)local_80);
  fVar10 = (float10)local_58 * (float10)local_64 +
           (float10)local_54 * (float10)local_60 + (float10)local_5c * fVar9;
  local_78 = (float)fVar10;
  if ((float10)-1.0 <= fVar10) {
    if (1.0 < local_78) {
      local_78 = 1.0;
    }
  }
  else {
    local_78 = -1.0;
  }
  local_70 = (float)((float10)local_60 * fVar9 - (float10)local_54 * (float10)local_5c);
  *param_2 = local_70;
  local_6c = (float)((float10)local_5c * (float10)local_58 - fVar9 * (float10)local_64);
  param_2[1] = local_6c;
  local_68 = local_54 * local_64 - local_60 * local_58;
  param_2[2] = local_68;
  vector3d_normalize_with_length();
  fVar9 = (float10)FUN_00628140();
  if ((float10)param_4 < fVar9) {
    fVar9 = (float10)param_4;
  }
  *param_2 = (float)(fVar9 * (float10)*param_2);
  param_2[1] = (float)(fVar9 * (float10)param_2[1]);
  param_2[2] = (float)(fVar9 * (float10)param_2[2]);
  if (unaff_ESI != 0) {
    matrix4x3_transform_normal();
    vector3d_normalize_with_length();
    return;
  }
  *param_1 = local_64;
  param_1[1] = local_60;
  param_1[2] = local_5c;
  return;
}
#endif
