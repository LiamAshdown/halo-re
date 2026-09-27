// object_physics_integrate_and_test_at_rest  (Ghidra: antenna_object_integrate_and_test_rest;
//   renamed per out/phase4/physics_types_notes.md section 5, which gives this exact new name:
//   "antenna_object_integrate_and_test_rest (0x5097e0) is
//   object_physics_integrate_and_test_at_rest")
// address 0x5097e0, size 1692 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (raised from 0.15: phase-4 integration pass corrected the angular-velocity scale factor (remaining_t, not the friction term) and the four at-rest boundary tests) -- among the lowest-confidence files in this
//   batch; Ghidra lost the majority of the register traffic in the collision sub-step loop (see
//   the UNSURE paragraphs below), and this rewrite preserves the visible control flow and the
//   field accesses it could confirm without inventing the lost ones.
// evidence: object_physics_tick.c (0x507840, this module) calls this as
//   `object_physics_integrate_and_test_at_rest(&context, param_3, &accum)`, which fixes
//   param_1 = object_physics_context *, param_2 = mass_point_state array base (stride 0x130,
//   matching types/physics.h), and in_ECX = &accum (the {torque; force;} pair that function
//   builds); types/physics.h k_physics_integration_substeps (4, "0x005097e0 runs its collision
//   loop 4x"); types/objects.h object.velocity/angular_velocity/position/forward/up; types/tags.h
//   Physics.mass/center_of_mass/inertial_matrix_and_inverse; types/projectiles.h collision_result
//   (t at +0x14, normal at +0x24 -- the fields this function's post-loop friction term reads);
//   collision_test_movement_segment.c's (0x505880, this module) own confirmed signature; math
//   module matrix3x3_from_forward_up / matrix3x3_multiply / matrix3x3_transpose /
//   matrix3x3_inverse_transform_vector / matrix4x3_from_forward_up / matrix4x3_transform_point
//   (all already rewritten in src/math, their real signatures reused verbatim here);
//   object_physics_mass_point_update_orientation (0x5096f0, this module) confirmed call site
//   `FUN_005096f0(iVar5 + 0x74, iVar5 + 0x80)` = (fallback_forward = &object->forward,
//   fallback_up = &object->up).
// register convention: in_ECX -> torque_and_force (the caller's combined {torque[3]; force[3]}
//   accumulator; only the first 3 floats -- torque -- are read directly here, see below).
//   param_1/param_2 are Ghidra's own recognized stack parameters (context, mass_point_states).
//   // blam-cc: ECX -> torque_and_force, stack -> context, mass_point_states
// UNSURE (major): the linear-velocity update (`velocity += torque_and_force[0..2] / mass`) reads
//   the TORQUE half of the caller's accumulator (confirmed: object_physics_tick.c's own struct
//   places torque before force at that pointer, and object_physics_compute_mass_point_forces.c
//   seeds the force half with gravity, which only makes physical sense as the *second* triple).
//   Dividing torque by mass and adding it directly to linear velocity is not standard rigid-body
//   integration; this rewrite preserves it exactly as Ghidra shows rather than "fixing" it to
//   read the force half instead, since it cannot rule out that this engine's antenna/mass-point
//   integrator really does use a single combined response scalar (1/mass) for both linear and
//   angular response here. The same pointer is assumed (not confirmed) to also be the hidden
//   vector input to matrix3x3_inverse_transform_vector below, since it is the only vector Ghidra
//   shows in scope at that point.
// UNSURE (major): the matrix3x3_from_forward_up -> matrix3x3_multiply -> matrix3x3_transpose ->
//   matrix3x3_multiply -> matrix3x3_inverse_transform_vector chain (building a world-space
//   inverse inertia tensor and transforming torque through it into an angular-velocity delta) has
//   every intermediate out-buffer and the forward-up source vectors themselves completely
//   invisible in Ghidra's decompile -- every call after the first shows at most one recovered
//   argument. This rewrite reconstructs the standard R * I_inv * R^T formula (R = object
//   orientation from forward/up) as the most plausible reading, using object->forward/up as the
//   orientation source and Physics.inertial_matrix_and_inverse's inverse half (+0x24) as I_inv,
//   but the exact register wiring is a guess, not a confirmed fact.
// UNSURE (major): the per-substep collision loop's own transform (matrix4x3_from_forward_up
//   into a fresh matrix, its position field overwritten by the just-integrated position and then
//   by that position transformed with the negated centre of mass via matrix4x3_transform_point)
//   mirrors object_physics_context_build's own construction, but again every source vector is
//   hidden; read as (up = object->up, forward = object->forward) for the from_forward_up call.
// UNSURE: object_set_position_and_orientation's forward/up arguments (local_c0/local_cc in
//   Ghidra) are read with NO visible prior store anywhere in this function's own decompile --
//   not even as an unaff_/extraout_ register. This rewrite passes object->forward/object->up
//   unchanged rather than inventing a plausible source, and flags the call UNSURE; the position
//   argument is a fourth, entirely hidden parameter per object_set_position_and_orientation's
//   confirmed 4-argument signature (src/units/unit_update_recoil_decay.c), passed here as the
//   sub-step matrix's own position field, itself only a best guess.
// UNSURE: object_flags bits 0x02/0x04/0x08/0x10 are not named in types/objects.h; kept as raw
//   hex with local comments describing what this function's own tallies (ground/water contact
//   counts) drive them from. TYPES-GAP: consider folding these into object_flags once another
//   module corroborates them.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"
#include "physics.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t physics_disable_integration; // 0x0071cfbc
extern double fabs(double x); // ABS is a single x87 FABS instruction

extern void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out); // 0x4cc560
extern void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b); // 0x4cc5f0
extern void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in); // 0x4cc500
extern void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix3x3 *m); // 0x4cc710
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, up in EAX, forward in ECX
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void object_physics_mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward,
    real_vector3d *fallback_forward, real_vector3d *fallback_up); // 0x5096f0, this module
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880, this module
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
    real_vector3d *up, real_point3d *position); // 0x4f51c0

// Integrates one tick of linear and angular momentum for context's object from torque_and_force
// (see UNSURE above), then resolves the resulting movement in up to k_physics_integration_substeps
// sub-steps: each sub-step re-tests every mass point's movement segment against the world
// (collision_test_movement_segment), and on the closest hit applies a friction/bounce correction
// to velocity before repeating, or commits the position/orientation directly once a sub-step finds
// no hit at all. Finally tallies each mass point's ground/water contact flags (written by
// object_physics_compute_mass_point_forces into mass_point_states) and updates the object's own
// at-rest, ground-contact and water-contact flags from the tallies and from how much the tick
// actually changed velocity and angular velocity.
// FIXED (objdump 0x509812..0x5098bc): ECX is the FORCE (linear: velocity += force / mass at 0x50981d) and the third
//   stack argument the TORQUE (angular, 0x5098b6); the draft used one pointer for both.
// blam-cc: stack -> context, mass_point_states, torque; ECX -> force
void object_physics_integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states,
    real_vector3d *torque, real_vector3d *force)
{
    object *self = ((object_header *)object_data->data)[context->object_index & 0xffff].data;
    Physics *definition = (Physics *)context->definition;
    real inverse_mass = 1.0f / definition->mass;

    real_vector3d delta_velocity;
    real_vector3d new_velocity;
    real_vector3d new_position;
    real_vector3d delta_angular_velocity;
    real_vector3d new_angular_velocity;

    delta_velocity.i = inverse_mass * force->i;
    delta_velocity.j = inverse_mass * force->j;
    delta_velocity.k = inverse_mass * force->k;
    new_velocity.i = delta_velocity.i + self->velocity.i;
    new_velocity.j = delta_velocity.j + self->velocity.j;
    new_velocity.k = delta_velocity.k + self->velocity.k;
    new_position.i = new_velocity.i + self->position.x;
    new_position.j = new_velocity.j + self->position.y;
    new_position.k = new_velocity.k + self->position.z;

    {
        real_matrix3x3 orientation;
        real_matrix3x3 step1;
        real_matrix3x3 step1_transposed;
        real_matrix3x3 world_inverse_inertia;
        real_matrix3x3 *inverse_inertia_local =
            (real_matrix3x3 *)((uint8_t *)definition->inertial_matrix_and_inverse.pointer + 0x24);

        matrix3x3_from_forward_up(&self->up, &self->forward, &orientation);
        matrix3x3_multiply(&step1, &orientation, inverse_inertia_local);
        matrix3x3_transpose(&step1_transposed, &step1);
        matrix3x3_multiply(&world_inverse_inertia, &step1_transposed, &orientation);
        matrix3x3_inverse_transform_vector(&delta_angular_velocity, torque, &world_inverse_inertia);
    }
    new_angular_velocity.i = delta_angular_velocity.i + self->angular_velocity.i;
    new_angular_velocity.j = delta_angular_velocity.j + self->angular_velocity.j;
    new_angular_velocity.k = delta_angular_velocity.k + self->angular_velocity.k;

    // 0x5098c1..0x5098fc (REWRITTEN 2026-09-27 static loop): the rotated orientation goes into LOCALS (EDI = new forward
    // ebp-0xbc, ESI = new up ebp-0xc8), rotated from the object's CURRENT forward / up, which this function never writes
    // itself; the object only changes through object_set_position_and_orientation with (new forward, new up, EDI =
    // new position = velocity + position, ebp-0x44). The draft rotated self->forward / up in place (so every collision
    // sub-step compounded the rotation and a fully blocked tick still turned the object), committed the old position
    // when integration was disabled and the centre-of-mass point after a free sub-step.
    {
        real_vector3d new_forward;
        real_vector3d new_up;
        real_point3d commit_position;

        object_physics_mass_point_update_orientation(&new_angular_velocity, &new_up, &new_forward,
            &self->forward, &self->up);

        self->velocity = new_velocity;
        self->angular_velocity = new_angular_velocity;
        commit_position.x = new_position.i;
        commit_position.y = new_position.j;
        commit_position.z = new_position.k;

        if (physics_disable_integration != 0) {
            object_set_position_and_orientation(context->object_index, &new_forward, &new_up, &commit_position);
        } else {
            int32_t substep;
            uint32_t hit_mask = 0;

            for (substep = k_physics_integration_substeps - 1; substep >= 0; substep--) {
                real_matrix4x3 step_matrix;
                real_point3d center_of_mass_local;
                real_point3d center_of_mass_world;
                uint8_t any_hit = 0;
                int32_t i;
                collision_result best_result;
                real_vector3d best_delta = {0.0f, 0.0f, 0.0f};

                hit_mask = 0;

                matrix4x3_from_forward_up(&new_up, &new_forward, &step_matrix); // 0x509964..0x509983: EAX up, ECX forward
                step_matrix.position = commit_position;

                center_of_mass_local.x = -definition->center_of_mass.x;
                center_of_mass_local.y = -definition->center_of_mass.y;
                center_of_mass_local.z = -definition->center_of_mass.z;
                matrix4x3_transform_point(&center_of_mass_world, &center_of_mass_local, &step_matrix);
                step_matrix.position = center_of_mass_world;

                for (i = 0; i < definition->mass_points.count; i++) {
                    PhysicsMassPoint *point_definition = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
                    mass_point_state *point_state = &mass_point_states[i];
                    Point3D local_position = point_definition->position;
                    real_point3d world_position;
                    real_vector3d delta;
                    collision_result candidate;

                    if (step_matrix.scale != 1.0f) {
                        local_position.x *= step_matrix.scale;
                        local_position.y *= step_matrix.scale;
                        local_position.z *= step_matrix.scale;
                    }

                    world_position.x = local_position.x * step_matrix.forward.i + local_position.y * step_matrix.left.i +
                        local_position.z * step_matrix.up.i + step_matrix.position.x;
                    world_position.y = local_position.x * step_matrix.forward.j + local_position.y * step_matrix.left.j +
                        local_position.z * step_matrix.up.j + step_matrix.position.y;
                    world_position.z = local_position.x * step_matrix.forward.k + local_position.y * step_matrix.left.k +
                        local_position.z * step_matrix.up.k + step_matrix.position.z;

                    delta.i = world_position.x - point_state->position_x;
                    delta.j = world_position.y - point_state->position_y;
                    delta.k = world_position.z - point_state->position_z;

                    if (collision_test_movement_segment(0xc0a1, (real_point3d *)&point_state->position_x, &delta,
                            context->object_index, &candidate) != 0) {
                        hit_mask |= 1u << (i & 0x1f);
                        if (any_hit == 0 || candidate.t < best_result.t) {
                            best_result = candidate;
                            best_delta = delta;
                            any_hit = 1;
                        }
                    }
                }

                if (any_hit == 0) {
                    // 0x509cc6: also reached with no mass points at all
                    object_set_position_and_orientation(context->object_index, &new_forward, &new_up, &commit_position);
                    break;
                }

                {
                    real dot_delta = best_result.plane.normal.i * best_delta.i + best_result.plane.normal.j * best_delta.j +
                        best_result.plane.normal.k * best_delta.k;
                    real friction_t = (dot_delta == 0.0f) ? 0.03125f : (0.0078125f / (real)fabs((double)dot_delta));
                    real remaining_t = best_result.t - friction_t;
                    real dot_velocity;

                    if (remaining_t <= 0.0f) {
                        remaining_t = 0.0f;
                    }

                    dot_velocity = best_result.plane.normal.i * new_velocity.i + best_result.plane.normal.j * new_velocity.j +
                        best_result.plane.normal.k * new_velocity.k;
                    if (dot_velocity < 0.0f) {
                        real bounce = (remaining_t - 1.0f) * dot_velocity;
                        new_velocity.i += best_result.plane.normal.i * bounce;
                        new_velocity.j += best_result.plane.normal.j * bounce;
                        new_velocity.k += best_result.plane.normal.k * bounce;
                        self->velocity = new_velocity;
                        commit_position.x = new_velocity.i + self->position.x;
                        commit_position.y = new_velocity.j + self->position.y;
                        commit_position.z = new_velocity.k + self->position.z;
                    }

                    new_angular_velocity.i *= remaining_t;
                    new_angular_velocity.j *= remaining_t;
                    new_angular_velocity.k *= remaining_t;
                    self->angular_velocity = new_angular_velocity;

                    // 0x509c66..0x509cb1: re-rotated from the object's (unchanged) orientation
                    object_physics_mass_point_update_orientation(&new_angular_velocity, &new_up, &new_forward,
                        &self->forward, &self->up);
                }
            }
            // four blocked sub-steps: nothing is committed (0x509cc4 jumps straight to the mask store)

            ((vehicle_data *)((uint8_t *)self + k_unit_object_size))->active_marker_mask = hit_mask;
        }
    }

    {
        int32_t count = definition->mass_points.count;
        int32_t at_rest_count = 0, ground_contact_count = 0, on_ground_surface_count = 0, water_contact_count = 0;
        int32_t i;

        for (i = 0; i < count; i++) {
            uint32_t flags = mass_point_states[i].flags;
            at_rest_count += (flags & _mass_point_at_rest_bit) != 0 ? 1 : 0;
            ground_contact_count += (flags & _mass_point_ground_contact_bit) != 0 ? 1 : 0;
            on_ground_surface_count += (flags & _mass_point_on_ground_surface_bit) != 0 ? 1 : 0;
            water_contact_count += (flags & _mass_point_water_contact_bit) != 0 ? 1 : 0;
        }

        // Ghidra renders each threshold test as `x < K != (x == K)`, which is true exactly when
        // x <= K -- so these are <=, not <. (Boundary-only, but preserved.)
        if (at_rest_count == count && ground_contact_count > 2 && on_ground_surface_count == 0 &&
            (new_velocity.i * new_velocity.i + new_velocity.j * new_velocity.j + new_velocity.k * new_velocity.k) <= 0.0011111111f &&
            (new_angular_velocity.i * new_angular_velocity.i + new_angular_velocity.j * new_angular_velocity.j +
             new_angular_velocity.k * new_angular_velocity.k) <= 0.0027415568f &&
            (delta_velocity.i * delta_velocity.i + delta_velocity.j * delta_velocity.j + delta_velocity.k * delta_velocity.k) <= 3.0864197e-07f &&
            (delta_angular_velocity.i * delta_angular_velocity.i + delta_angular_velocity.j * delta_angular_velocity.j +
             delta_angular_velocity.k * delta_angular_velocity.k) <= 3.0461742e-06f) {
            self->flags |= _object_at_rest_bit;
        } else {
            self->flags &= ~_object_at_rest_bit;
        }

        self->flags = (ground_contact_count >= 1) ? (self->flags | 0x02u) : (self->flags & ~0x02u); // UNSURE bit name
        self->flags = (water_contact_count >= 1) ? (self->flags | 0x04u) : (self->flags & ~0x04u);   // UNSURE bit name
        self->flags = (water_contact_count >= 1) ? (self->flags | 0x08u) : (self->flags & ~0x08u);   // UNSURE bit name

        if (water_contact_count != count) {
            self->flags &= ~0x10u; // UNSURE bit name
            return;
        }
        self->flags |= 0x10u; // UNSURE bit name: every mass point reported water contact
    }
}

#if 0
Original Ghidra decompilation (0x5097e0):

void antenna_object_integrate_and_test_rest(uint *param_1,int param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  short sVar6;
  char cVar7;
  undefined4 uVar8;
  int iVar9;
  uint *puVar10;
  uint uVar11;
  short sVar12;
  float *in_ECX;
  short sVar13;
  int iVar14;
  undefined4 *puVar15;
  short sVar16;
  undefined4 *puVar17;
  undefined4 local_1a0 [5];
  float local_18c;
  undefined1 local_14c [36];
  undefined4 local_128 [5];
  float local_114;
  float local_104;
  float local_100;
  float local_fc;
  float local_d8;
  float local_d4;
  undefined1 local_cc [12];
  undefined1 local_c0 [12];
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  int local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  int local_30;
  float local_2c;
  float local_28;
  float local_24;
  int local_20;
  uint local_1c;
  float local_18;
  float local_14;
  float local_10;
  char local_9;

  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*param_1 & 0xffff) * 0xc);
  local_34 = 1.0 / *(float *)(param_1[1] + 8);
  local_3c = local_34 * *in_ECX;
  local_38 = local_34 * in_ECX[1];
  local_34 = local_34 * in_ECX[2];
  local_18 = local_3c + *(float *)(iVar5 + 0x68);
  local_14 = local_38 + *(float *)(iVar5 + 0x6c);
  local_10 = local_34 + *(float *)(iVar5 + 0x70);
  local_48 = local_18 + *(float *)(iVar5 + 0x5c);
  local_44 = local_14 + *(float *)(iVar5 + 0x60);
  local_40 = local_10 + *(float *)(iVar5 + 100);
  matrix3x3_from_forward_up(local_14c);
  matrix3x3_multiply(*(int *)(param_1[1] + 0x60) + 0x24);
  uVar8 = matrix3x3_transpose();
  uVar8 = matrix3x3_multiply(uVar8);
  matrix3x3_inverse_transform_vector(uVar8);
  local_2c = local_54 + *(float *)(iVar5 + 0x8c);
  local_28 = local_50 + *(float *)(iVar5 + 0x90);
  local_24 = local_4c + *(float *)(iVar5 + 0x94);
  FUN_005096f0(iVar5 + 0x74,iVar5 + 0x80);
  *(float *)(iVar5 + 0x68) = local_18;
  *(float *)(iVar5 + 0x6c) = local_14;
  *(float *)(iVar5 + 0x70) = local_10;
  *(float *)(iVar5 + 0x8c) = local_2c;
  *(float *)(iVar5 + 0x90) = local_28;
  *(float *)(iVar5 + 0x94) = local_24;
  if (DAT_0071cfbc == '\0') {
    local_20 = 4;
    do {
      local_20 = local_20 + -1;
      iVar14 = 0;
      local_9 = '\0';
      local_1c = 0;
      matrix4x3_from_forward_up(&local_a8);
      local_78 = local_40;
      uVar11 = param_1[1];
      local_80 = local_48;
      local_64 = -*(float *)(uVar11 + 0xc);
      local_7c = local_44;
      local_60 = -*(float *)(uVar11 + 0x10);
      local_5c = -*(float *)(uVar11 + 0x14);
      matrix4x3_transform_point(&local_a8);
      local_7c = local_60;
      uVar11 = param_1[1];
      local_78 = local_5c;
      local_80 = local_64;
      local_30 = 0;
      if (*(int *)(uVar11 + 0x74) < 1) {
LAB_00509cc6:
        object_set_position_and_orientation(*param_1,local_c0,local_cc);
        break;
      }
      do {
        iVar9 = iVar14 * 0x130 + param_2;
        pfVar1 = (float *)(iVar14 * 0x80 + 0x38 + *(int *)(uVar11 + 0x78));
        fVar2 = *pfVar1;
        fVar3 = pfVar1[1];
        fVar4 = pfVar1[2];
        if (local_a8 != 1.0) {
          fVar2 = local_a8 * fVar2;
          fVar3 = local_a8 * fVar3;
          fVar4 = fVar4 * local_a8;
        }
        local_d8 = local_a4 * fVar2 + local_98 * fVar3 + local_8c * fVar4 + local_80;
        local_d4 = local_a0 * fVar2 + local_94 * fVar3 + local_88 * fVar4 + local_7c;
        local_70 = local_d8 - *(float *)(iVar9 + 4);
        local_6c = local_d4 - *(float *)(iVar9 + 8);
        local_68 = (local_9c * fVar2 + local_90 * fVar3 + local_84 * fVar4 + local_78) -
                   *(float *)(iVar9 + 0xc);
        cVar7 = FUN_00505880(0xc0a1,(float *)(iVar9 + 4),&local_70,*param_1,local_1a0);
        fVar3 = local_68;
        fVar2 = local_70;
        if ((cVar7 != '\0') &&
           ((local_1c = local_1c | 1 << ((byte)iVar14 & 0x1f), local_9 == '\0' ||
            (local_18c < local_114)))) {
          local_b0 = local_6c;
          puVar15 = local_1a0;
          puVar17 = local_128;
          for (iVar14 = 0x14; iVar14 != 0; iVar14 = iVar14 + -1) {
            *puVar17 = *puVar15;
            puVar15 = puVar15 + 1;
            puVar17 = puVar17 + 1;
          }
          local_9 = '\x01';
          local_b4 = fVar2;
          local_ac = fVar3;
        }
        uVar11 = param_1[1];
        local_30 = local_30 + 1;
        iVar14 = (int)(short)local_30;
      } while (iVar14 < *(int *)(uVar11 + 0x74));
      if (local_9 == '\0') goto LAB_00509cc6;
      fVar2 = local_104 * local_b4 + local_100 * local_b0 + local_fc * local_ac;
      if (fVar2 == 0.0) {
        fVar2 = 0.03125;
      }
      else {
        fVar2 = 0.0078125 / ABS(fVar2);
      }
      fVar2 = local_114 - fVar2;
      if (fVar2 <= 0.0) {
        fVar2 = 0.0;
      }
      fVar3 = local_104 * local_18 + local_100 * local_14 + local_fc * local_10;
      if (fVar3 < 0.0) {
        fVar3 = (fVar2 - 1.0) * fVar3;
        local_18 = local_104 * fVar3 + local_18;
        *(float *)(iVar5 + 0x68) = local_18;
        local_14 = local_100 * fVar3 + local_14;
        *(float *)(iVar5 + 0x6c) = local_14;
        local_10 = local_fc * fVar3 + local_10;
        *(float *)(iVar5 + 0x70) = local_10;
        local_48 = local_18 + *(float *)(iVar5 + 0x5c);
        local_44 = local_14 + *(float *)(iVar5 + 0x60);
        local_40 = local_10 + *(float *)(iVar5 + 100);
      }
      local_2c = local_2c * fVar2;
      *(float *)(iVar5 + 0x8c) = local_2c;
      local_28 = local_28 * fVar2;
      *(float *)(iVar5 + 0x90) = local_28;
      local_24 = local_24 * fVar2;
      *(float *)(iVar5 + 0x94) = local_24;
      FUN_005096f0(iVar5 + 0x74,iVar5 + 0x80);
    } while (0 < (short)local_20);
    *(uint *)(iVar5 + 0x520) = local_1c;
  }
  else {
    object_set_position_and_orientation(*param_1,local_c0,local_cc);
  }
  iVar14 = *(int *)(param_1[1] + 0x74);
  iVar9 = 0;
  sVar13 = 0;
  sVar12 = 0;
  sVar16 = 0;
  sVar6 = 0;
  local_20._0_2_ = 0;
  local_1c = 0;
  local_30 = 0;
  local_30._0_2_ = 0;
  local_58 = 0;
  if (0 < iVar14) {
    do {
      puVar10 = (uint *)(iVar9 * 0x130 + param_2);
      uVar11 = *puVar10;
      local_20._0_2_ = sVar6 + ((byte)*puVar10 & 1);
      local_1c = local_1c + (uVar11 >> 1 & 1);
      sVar13 = (short)local_1c;
      local_30 = local_30 + (uVar11 >> 2 & 1);
      local_58 = local_58 + (uVar11 >> 3 & 1);
      sVar12 = (short)local_58;
      sVar16 = sVar16 + 1;
      iVar9 = (int)sVar16;
      sVar6 = (short)local_20;
    } while (iVar9 < iVar14);
  }
  if (((((short)local_20 == iVar14) && (2 < sVar13)) && ((short)local_30 == 0)) &&
     (((fVar2 = local_18 * local_18 + local_14 * local_14 + local_10 * local_10,
       fVar2 < 0.0011111111 != (fVar2 == 0.0011111111) &&
       (fVar2 = local_2c * local_2c + local_28 * local_28 + local_24 * local_24,
       fVar2 < 0.0027415568 != (fVar2 == 0.0027415568))) &&
      ((fVar2 = local_3c * local_3c + local_38 * local_38 + local_34 * local_34,
       fVar2 < 3.0864197e-07 != (fVar2 == 3.0864197e-07) &&
       (fVar2 = local_54 * local_54 + local_50 * local_50 + local_4c * local_4c,
       fVar2 < 3.0461742e-06 != (fVar2 == 3.0461742e-06))))))) {
    uVar11 = *(uint *)(iVar5 + 0x10) | 0x20;
  }
  else {
    uVar11 = *(uint *)(iVar5 + 0x10) & 0xffffffdf;
  }
  *(uint *)(iVar5 + 0x10) = uVar11;
  if (sVar13 < 1) {
    uVar11 = uVar11 & 0xfffffffd;
  }
  else {
    uVar11 = uVar11 | 2;
  }
  *(uint *)(iVar5 + 0x10) = uVar11;
  if (sVar12 < 1) {
    uVar11 = uVar11 & 0xfffffffb;
  }
  else {
    uVar11 = uVar11 | 4;
  }
  *(uint *)(iVar5 + 0x10) = uVar11;
  if (sVar12 < 1) {
    uVar11 = uVar11 & 0xfffffff7;
  }
  else {
    uVar11 = uVar11 | 8;
  }
  *(uint *)(iVar5 + 0x10) = uVar11;
  if ((int)sVar12 != *(int *)(param_1[1] + 0x74)) {
    *(uint *)(iVar5 + 0x10) = *(uint *)(iVar5 + 0x10) & 0xffffffef;
    return;
  }
  *(uint *)(iVar5 + 0x10) = *(uint *)(iVar5 + 0x10) | 0x10;
  return;
}
#endif
