// object_physics_resolve_mass_point_overlap  (Ghidra: FUN_005090c0, still unnamed there;
//   renamed per out/phase4/physics_types_notes.md section 5's general guidance for this whole
//   call family: "object_physics_* / mass_point_*")
// address 0x5090c0, size 1573 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/physics_functions.md summary ("Detects overlapping collision spheres
//   between the vertices of two physics/antenna objects and applies a repulsion impulse to
//   separate them"); its one call site, object_physics_handle_nearby_object_impacts.c (0x508a10,
//   this module), calls it as `FUN_005090c0(&candidate_context)` right after building both
//   contexts, which is what fixes the EDX/param_1 role split below; types/physics.h
//   object_physics_context field layout (forward/left/up/position at the exact word indices
//   this function reads through both contexts) and its own note "0x005090c0 skips the multiply
//   while [scale] still is [1.0]"; types/tags.h Physics.radius (+0x00, "0x005097e0 picks
//   [FUN_00509e80] when Physics.radius > 0.0" -- the same field gates whether this function
//   accumulates into the *other* object here); types/units.h vehicle_data.accumulated_force
//   (+0x508) and accumulated_torque (+0x514); types/objects.h object_flags._object_at_rest_bit
//   (0x20, cleared here on both objects) and object.network_role (+0x04).
// register convention: in_EDX -> self (object_physics_context *, the object object_physics_tick
//   is currently processing). param_1 is Ghidra's own recognized stack parameter (other,
//   object_physics_context * for the nearby candidate object).
//   // blam-cc: EDX -> self, stack -> other
// UNSURE: unit_any_flagged_seat_occupied (0x56cc80) is outside this module's slice (called from vehicle_update and
//   elsewhere); declared opaque. Read from its two call sites here as a network/local-authority
//   gate: force is applied to an object only when its network_role isn't the "puppet" value (1),
//   or this gate passes anyway.
// UNSURE: the field at object_physics_context.definition + 0x00 (Physics.radius) gates whether
//   *other* receives the accumulated force/torque at all; *self* has no such gate. The asymmetry
//   is preserved exactly as Ghidra shows it, not "fixed" to be symmetric.
// reconciled: R24 vehicle_data unknown_508..unknown_51c -> real_vector3d accumulated_force (+0x508) / accumulated_torque (+0x514); the float casts over the uint32 placeholders are gone

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

extern data_array *object_data; // 0x008603b0
extern float k_physics_gravity;           // 0x0069c52c
extern float k_physics_collision_damping; // 0x0069c538, UNSURE, see types/physics.h
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index); // 0x56cc80, EAX

extern double sqrt(double x);
extern double fabs(double x); // ABS is a single x87 FABS instruction

// For every pair of mass-point spheres between self and other (self's mass points transformed
// by self's own matrix, other's by other's), tests whether the spheres overlap and, if so,
// applies an equal-and-opposite spring repulsion impulse (scaled by penetration depth and the
// geometric mean of the two masses) plus the matching torque about each object's centre, summed
// across every overlapping pair before being added once to each object's accumulated
// force/torque. Returns whether any pair overlapped.
uint8_t object_physics_resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other)
{
    Physics *self_definition = (Physics *)self->definition;
    Physics *other_definition = (Physics *)other->definition;
    float other_mass = other_definition->mass;
    float self_mass = self_definition->mass;

    int32_t self_mass_point_count = self_definition->mass_points.count;
    uint8_t overlapped = 0;

    real_vector3d self_force = {0.0f, 0.0f, 0.0f};
    real_vector3d self_torque = {0.0f, 0.0f, 0.0f};
    real_vector3d other_force = {0.0f, 0.0f, 0.0f};
    real_vector3d other_torque = {0.0f, 0.0f, 0.0f};

    if (self_mass_point_count <= 0) {
        return 0;
    }

    {
        PhysicsMassPoint *self_mass_points = (PhysicsMassPoint *)self_definition->mass_points.pointer;
        PhysicsMassPoint *other_mass_points = (PhysicsMassPoint *)other_definition->mass_points.pointer;
        int32_t other_mass_point_count = other_definition->mass_points.count;
        int32_t i;

        for (i = 0; i < self_mass_point_count; i++) {
            real_point3d self_world_position;
            int32_t j;

            matrix4x3_transform_point(&self_world_position, (real_point3d *)&self_mass_points[i].position,
                (real_matrix4x3 *)&self->scale);

            for (j = 0; j < other_mass_point_count; j++) {
                float combined_radius = other_mass_points[j].radius + self_mass_points[i].radius;
                float local_x = other_mass_points[j].position.x;
                float local_y = other_mass_points[j].position.y;
                float local_z = other_mass_points[j].position.z;
                float delta_x, delta_y, delta_z, distance;

                if (other->scale != 1.0f) {
                    local_x *= other->scale;
                    local_y *= other->scale;
                    local_z *= other->scale;
                }

                // 0x509224..0x509279: each component is ((z * up + y * left) + x * forward) + position, in that
                // order (float rounding differs from the x-first order)
                delta_x = (((local_z * other->up_i + local_y * other->left_i) + local_x * other->forward_i) + other->position_x)
                    - self_world_position.x;
                delta_y = (((local_z * other->up_j + local_y * other->left_j) + local_x * other->forward_j) + other->position_y)
                    - self_world_position.y;
                delta_z = (((local_z * other->up_k + local_y * other->left_k) + local_x * other->forward_k) + other->position_z)
                    - self_world_position.z;

                // 0x50929d..0x5092b9: (dz^2 + dy^2) + dx^2
                distance = (float)sqrt((double)((delta_z * delta_z + delta_y * delta_y) + delta_x * delta_x));
                if ((float)fabs((double)distance) < 0.0001f) {
                    distance = 0.0f;
                } else {
                    float inv_distance = 1.0f / distance;
                    delta_x *= inv_distance;
                    delta_y *= inv_distance;
                    delta_z *= inv_distance;
                }

                if (distance < combined_radius && 0.0f < distance) {
                    float penetration_half = (combined_radius - distance) * 0.5f;
                    float impulse = (k_physics_gravity / k_physics_collision_damping) * penetration_half *
                        (float)sqrt((double)(other_mass * self_mass));
                    float force_self_x, force_self_y, force_self_z;
                    float force_other_x, force_other_y, force_other_z;
                    float contact_distance, contact_x, contact_y, contact_z;
                    float self_offset_x, self_offset_y, self_offset_z;
                    float other_offset_x, other_offset_y, other_offset_z;
                    impulse += impulse; // doubled

                    force_self_x = delta_x * -impulse;
                    force_self_y = delta_y * -impulse;
                    force_self_z = delta_z * -impulse;
                    force_other_x = delta_x * impulse;
                    force_other_y = delta_y * impulse;
                    force_other_z = delta_z * impulse;

                    contact_distance = self_mass_points[i].radius - penetration_half;
                    contact_x = delta_x * contact_distance + self_world_position.x;
                    contact_y = delta_y * contact_distance + self_world_position.y;
                    contact_z = delta_z * contact_distance + self_world_position.z;

                    {
                        object *self_object = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
                        object *other_object = ((object_header *)object_data->data)[other->object_index & 0xffff].data;

                        self_offset_x = contact_x - self_object->position.x;
                        self_offset_y = contact_y - self_object->position.y;
                        self_offset_z = contact_z - self_object->position.z;
                        other_offset_x = contact_x - other_object->position.x;
                        other_offset_y = contact_y - other_object->position.y;
                        other_offset_z = contact_z - other_object->position.z;
                    }

                    overlapped = 1;
                    self_force.i += force_self_x;
                    self_force.j += force_self_y;
                    self_force.k += force_self_z;
                    other_force.i += force_other_x;
                    other_force.j += force_other_y;
                    other_force.k += force_other_z;

                    self_torque.i += self_offset_y * force_self_z - self_offset_z * force_self_y;
                    self_torque.j += self_offset_z * force_self_x - force_self_z * self_offset_x;
                    self_torque.k += self_offset_x * force_self_y - self_offset_y * force_self_x;

                    other_torque.i += other_offset_y * force_other_z - other_offset_z * force_other_y;
                    other_torque.j += other_offset_z * force_other_x - force_other_z * other_offset_x;
                    other_torque.k += other_offset_x * force_other_y - other_offset_y * force_other_x;
                }
            }
        }
    }

    if (overlapped != 0) {
        object *self_object = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        object *other_object = ((object_header *)object_data->data)[other->object_index & 0xffff].data;

        // FIXED (0x5095b1 / 0x50964d): EAX = self->object_index / other->object_index
        if (self_object->network_role != 1 || unit_any_flagged_seat_occupied(self->object_index) == 1) {
            vehicle_data *self_vehicle = (vehicle_data *)((uint8_t *)self_object + k_unit_object_size);
            self_vehicle->accumulated_force.i += self_force.i;
            self_vehicle->accumulated_force.j += self_force.j;
            self_vehicle->accumulated_force.k += self_force.k;
            self_vehicle->accumulated_torque.i += self_torque.i;
            self_vehicle->accumulated_torque.j += self_torque.j;
            self_vehicle->accumulated_torque.k += self_torque.k;
            self_object->flags &= ~_object_at_rest_bit;
            self_vehicle->collision_update_pending = 1; // UNSURE: raw offset, see file header
        }

        if (other_definition->radius <= 0.0f &&
            (other_object->network_role != 1 || unit_any_flagged_seat_occupied(other->object_index) == 1)) {
            vehicle_data *other_vehicle = (vehicle_data *)((uint8_t *)other_object + k_unit_object_size);
            other_vehicle->accumulated_force.i += other_force.i;
            other_vehicle->accumulated_force.j += other_force.j;
            other_vehicle->accumulated_force.k += other_force.k;
            other_vehicle->accumulated_torque.i += other_torque.i;
            other_vehicle->accumulated_torque.j += other_torque.j;
            other_vehicle->accumulated_torque.k += other_torque.k;
            other_object->flags &= ~_object_at_rest_bit;
            other_vehicle->collision_update_pending = 1;
        }
    }

    return overlapped;
}

#if 0
Original Ghidra decompilation (0x5090c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char FUN_005090c0(uint *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  char cVar21;
  short sVar22;
  int iVar23;
  int iVar24;
  int iVar25;
  short sVar26;
  uint *in_EDX;
  char local_d5;
  float local_d4;
  float local_d0;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_68;
  float local_64;
  float local_60;

  fVar1 = *(float *)(param_1[1] + 8);
  iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*in_EDX & 0xffff) * 0xc);
  iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*param_1 & 0xffff) * 0xc);
  uVar8 = in_EDX[1];
  fVar2 = *(float *)(uVar8 + 8);
  iVar9 = *(int *)(uVar8 + 0x74);
  local_d5 = '\0';
  local_c8 = 0.0;
  local_c4 = 0.0;
  local_c0 = 0.0;
  local_bc = 0.0;
  local_b8 = 0.0;
  local_b4 = 0.0;
  local_98 = 0.0;
  local_94 = 0.0;
  local_90 = 0.0;
  local_a4 = 0.0;
  local_a0 = 0.0;
  local_9c = 0.0;
  sVar22 = 0;
  if (0 < iVar9) {
    iVar10 = *(int *)(uVar8 + 0x78);
    iVar23 = 0;
    iVar11 = *(int *)(param_1[1] + 0x74);
    do {
      iVar23 = iVar23 * 0x80 + iVar10;
      matrix4x3_transform_point(in_EDX + 2);
      sVar26 = 0;
      if (0 < iVar11) {
        iVar24 = 0;
        do {
          iVar25 = iVar24 * 0x80 + *(int *)(param_1[1] + 0x78);
          fVar12 = *(float *)(iVar24 * 0x80 + 0x68 + *(int *)(param_1[1] + 0x78)) +
                   *(float *)(iVar23 + 0x68);
          fVar3 = *(float *)(iVar25 + 0x38);
          fVar4 = *(float *)(iVar25 + 0x3c);
          fVar5 = *(float *)(iVar25 + 0x40);
          if (param_1[2] != 0x3f800000) {
            fVar3 = fVar3 * (float)param_1[2];
            fVar4 = fVar4 * (float)param_1[2];
            fVar5 = fVar5 * (float)param_1[2];
          }
          local_d4 = (fVar3 * (float)param_1[3] +
                      fVar4 * (float)param_1[6] + fVar5 * (float)param_1[9] + (float)param_1[0xc]) -
                     local_68;
          local_d0 = (fVar3 * (float)param_1[4] +
                      fVar4 * (float)param_1[7] + fVar5 * (float)param_1[10] + (float)param_1[0xd])
                     - local_64;
          local_cc = (fVar3 * (float)param_1[5] +
                      fVar4 * (float)param_1[8] + fVar5 * (float)param_1[0xb] + (float)param_1[0xe])
                     - local_60;
          fVar3 = SQRT(local_d4 * local_d4 + local_d0 * local_d0 + local_cc * local_cc);
          if (ABS(fVar3) < 0.0001) {
            fVar3 = 0.0;
          }
          else {
            fVar4 = 1.0 / fVar3;
            local_d4 = local_d4 * fVar4;
            local_d0 = local_d0 * fVar4;
            local_cc = local_cc * fVar4;
          }
          if ((fVar3 < fVar12) && (0.0 < fVar3)) {
            fVar3 = (fVar12 - fVar3) * 0.5;
            fVar15 = (_DAT_0069c52c / _DAT_0069c538) * fVar3 * SQRT(fVar1 * fVar2);
            fVar15 = fVar15 + fVar15;
            fVar16 = -fVar15;
            fVar19 = local_d4 * fVar16;
            fVar20 = local_d0 * fVar16;
            fVar16 = local_cc * fVar16;
            fVar17 = local_d4 * fVar15;
            fVar18 = local_d0 * fVar15;
            fVar15 = fVar15 * local_cc;
            fVar3 = *(float *)(iVar23 + 0x68) - fVar3;
            fVar13 = local_d4 * fVar3 + local_68;
            fVar12 = local_d0 * fVar3 + local_64;
            fVar5 = local_cc * fVar3 + local_60;
            fVar14 = fVar13 - *(float *)(iVar6 + 0x5c);
            fVar3 = fVar12 - *(float *)(iVar6 + 0x60);
            fVar4 = fVar5 - *(float *)(iVar6 + 100);
            fVar13 = fVar13 - *(float *)(iVar7 + 0x5c);
            fVar12 = fVar12 - *(float *)(iVar7 + 0x60);
            fVar5 = fVar5 - *(float *)(iVar7 + 100);
            local_d5 = '\x01';
            local_c8 = fVar19 + local_c8;
            local_c4 = fVar20 + local_c4;
            local_c0 = fVar16 + local_c0;
            local_bc = fVar17 + local_bc;
            local_b8 = fVar18 + local_b8;
            local_b4 = fVar15 + local_b4;
            local_98 = (fVar3 * fVar16 - fVar4 * fVar20) + local_98;
            local_94 = (fVar4 * fVar19 - fVar16 * fVar14) + local_94;
            local_90 = (fVar20 * fVar14 - fVar3 * fVar19) + local_90;
            local_a4 = (fVar12 * fVar15 - fVar5 * fVar18) + local_a4;
            local_a0 = (fVar5 * fVar17 - fVar15 * fVar13) + local_a0;
            local_9c = (fVar18 * fVar13 - fVar12 * fVar17) + local_9c;
          }
          sVar26 = sVar26 + 1;
          iVar24 = (int)sVar26;
        } while (iVar24 < iVar11);
      }
      sVar22 = sVar22 + 1;
      iVar23 = (int)sVar22;
    } while (iVar23 < iVar9);
    if (local_d5 != '\0') {
      if ((*(int *)(iVar6 + 4) != 1) || (cVar21 = FUN_0056cc80(), cVar21 == '\x01')) {
        *(float *)(iVar6 + 0x508) = local_c8 + *(float *)(iVar6 + 0x508);
        *(float *)(iVar6 + 0x50c) = local_c4 + *(float *)(iVar6 + 0x50c);
        *(float *)(iVar6 + 0x510) = local_c0 + *(float *)(iVar6 + 0x510);
        *(float *)(iVar6 + 0x514) = local_98 + *(float *)(iVar6 + 0x514);
        *(float *)(iVar6 + 0x518) = local_94 + *(float *)(iVar6 + 0x518);
        *(float *)(iVar6 + 0x51c) = local_90 + *(float *)(iVar6 + 0x51c);
        *(uint *)(iVar6 + 0x10) = *(uint *)(iVar6 + 0x10) & 0xffffffdf;
        *(undefined1 *)(iVar6 + 0x524) = 1;
      }
      if ((*(float *)param_1[1] <= 0.0) &&
         ((*(int *)(iVar7 + 4) != 1 || (cVar21 = FUN_0056cc80(), cVar21 == '\x01')))) {
        *(float *)(iVar7 + 0x508) = local_bc + *(float *)(iVar7 + 0x508);
        *(float *)(iVar7 + 0x50c) = local_b8 + *(float *)(iVar7 + 0x50c);
        *(float *)(iVar7 + 0x510) = local_b4 + *(float *)(iVar7 + 0x510);
        *(float *)(iVar7 + 0x514) = local_a4 + *(float *)(iVar7 + 0x514);
        *(float *)(iVar7 + 0x518) = local_a0 + *(float *)(iVar7 + 0x518);
        *(float *)(iVar7 + 0x51c) = local_9c + *(float *)(iVar7 + 0x51c);
        *(uint *)(iVar7 + 0x10) = *(uint *)(iVar7 + 0x10) & 0xffffffdf;
        *(undefined1 *)(iVar7 + 0x524) = 1;
      }
    }
    return local_d5;
  }
  return '\0';
}
#endif
