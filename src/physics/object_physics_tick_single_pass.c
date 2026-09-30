// object_physics_tick_single_pass  (Ghidra: FUN_00509e80, still unnamed there; name chosen since
//   out/phase4/physics_types_notes.md section 5 describes it only as "the alternate, single-pass
//   implementation of the antenna physics tick... used instead of the general multi-vertex path",
//   picked by object_physics_tick (0x507840, this module) when Physics.radius > 0.0)
// address 0x509e80, size 5129 bytes -- the largest function in this module.
// name confidence: 0.35   rewrite confidence: 0.20 (raised from 0.12: phase-4 integration pass restored the two missing delta terms of the at-rest test and un-swapped its velocity/angular-velocity thresholds) -- the lowest-confidence file in this batch.
//   This function fuses object_physics_compute_mass_point_forces (0x507cc0) and
//   object_physics_integrate_and_test_at_rest (0x5097e0)'s jobs into one pass with a simplified
//   scalar (not full inverse-inertia-tensor) angular integration, and Ghidra lost the majority of
//   its register traffic throughout (see UNSURE paragraphs below). This rewrite preserves the
//   control flow, the field accesses confirmed against types/physics.h's mass_point_state
//   word table, and the arithmetic exactly as decompiled; it does not claim full confidence in
//   the reconstructed hidden arguments.
// evidence: types/physics.h mass_point_state's word-by-word field table (the same table
//   object_physics_compute_mass_point_forces.c uses) resolves nearly every puVar15[N] offset
//   below by name; that file's own already-confirmed field names, flag bits and callee
//   signatures (PhysicsPoweredMassPoint flags 0x01/0x02/0x04/0x08/0x10/0x20/0x40 <->
//   powered_mass_point_state ground_friction/water_friction/air_friction/water_lift/air_lift/
//   thrust/antigrav, in that order) are reused verbatim here since the ground/water/air/buoyancy/
//   antigrav formulas in this function match that file's formulas field for field, apart from two
//   confirmed differences: (1) this function never looks up a GlobalsMaterial row, using
//   Physics.ground_friction/ground_normal_k1/ground_normal_k0 unscaled; (2) the mass-point
//   transform uses a fresh object-forward/up matrix positioned at the object's raw position
//   (no centre-of-mass shift), not object_physics_context's own matrix; types/objects.h
//   bsp_leaf_reference (leaf_index/cluster_index, matching object_set_cluster_and_parent's
//   confirmed signature elsewhere in this codebase); math module vector3d_rotate_about_axis
//   (confirmed 4-argument signature reused from src/items and src/math).
// register convention: none recognized as in_EAX etc; all five are Ghidra's own ordinary
//   parameters, identical in shape to object_physics_tick's own
//   (object_index, powered_states, mass_point_states, extra_force, extra_torque).
// UNSURE (major): the two vector3d_rotate_about_axis calls near the end (rotating the object's
//   forward and up by this tick's angular velocity, then re-orthonormalizing up against forward)
//   show only their sin/cos arguments; their v/axis register pair is reconstructed by direct
//   analogy with object_physics_mass_point_update_orientation's (0x5096f0) own, already-confirmed
//   version of the identical idiom, not read from this function's own decompile.
// UNSURE (major): the aggregate angular-acceleration step normalizes the summed torque into an
//   axis, then sums a per-mass-point scalar "moment of inertia along that axis"
//   (offset-perpendicular-to-axis squared, plus 0.4x the offset-along-axis squared, times mass
//   and Physics.moment_scale) to divide the torque magnitude by, rather than using a proper
//   inverse inertia tensor the way object_physics_integrate_and_test_at_rest does. This is
//   preserved exactly as decompiled, not "corrected" to match that function's approach.
// UNSURE: the ground-friction antigrav-adjusted push block, and the plain antigrav block at the
//   very end, are read by direct field-for-field analogy with
//   object_physics_compute_mass_point_forces.c's own already-confirmed versions of the identical
//   formulas (this function's own decompile shows the same float10-heavy, argument-starved shape
//   for both).
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"
#include "physics.h"
#include "fn_math.h"
#include "fn_objects.h"
#include "fn_physics.h"
#include <string.h>

extern double fabs(double x); // ABS is a single x87 FABS instruction
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);

extern data_array *object_data;                              // 0x008603b0
extern ScenarioStructureBSP *global_structure_bsp;          // 0x00746f9c
extern ModelCollisionGeometryBSP *global_collision_bsp;    // 0x00746f90
extern real_vector3d *global_down3d_pointer;       // 0x0069672c
extern float k_physics_gravity;                               // 0x0069c52c
extern tag_instance *tag_instances;                           // 0x0087bc14

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, this module (lower half); called with zero visible
                           // arguments at every site in this function -- UNSURE, reused verbatim
                           // from object_physics_compute_mass_point_forces.c's own resolution

extern void matrix4x3_from_quaternion(void *quaternion, void *out_matrix); // 0x4cbad0, math
    // module; blam-cc UNSURE, see object_physics_tick.c's own identical UNSURE note
extern void matrix4x3_multiply(void *a, void *b, void *out); // 0x4cc0d0, math module
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd820, v in EAX, axis in ECX

extern float scenario_location_water_surface_distance(bsp_leaf_reference *location, real_point3d *point); // 0x53ee00, EAX location, EDI point
extern float real_inverse_lerp_clamped(float value, float ref_k0, float ref_k1); // 0x507430, misattributed
    // math helper, not rewritten in this batch

extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index,
    collision_result *result); // 0x505880, this module (higher half)
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0


// The single-pass alternative to object_physics_tick's general path: for each of Physics'
// mass points (transformed by a fresh object-forward/up matrix at the object's raw position, not
// object_physics_context's centre-of-mass-shifted one), computes the same ground/water/air
// friction, buoyancy and powered forces as object_physics_compute_mass_point_forces (without a
// GlobalsMaterial lookup), immediately resolves its own per-mass-point collision, then folds the
// summed force into a linear acceleration and the summed torque into a single scalar-moment-of-
// inertia angular acceleration, integrates the object's position/velocity/orientation by one tick,
// updates its cluster, and tallies the same ground/water/at-rest object flags as
// object_physics_integrate_and_test_at_rest.
void object_physics_tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states,
    mass_point_state *mass_point_states, real_vector3d *extra_force, real_vector3d *extra_torque)
{
    object *self = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *object_tag_data;
    Physics *definition;
    float gravity_scale;
    real_matrix4x3 step_matrix;
    real_vector3d total_force = {0.0f, 0.0f, 0.0f};
    real_vector3d total_torque = {0.0f, 0.0f, 0.0f};
    // local_50/4c/48 and local_38/34/30: this tick's velocity and angular-velocity increments.
    // Declared at function scope because the at-rest test at the very end reads them back.
    real_vector3d linear_accel = {0.0f, 0.0f, 0.0f};
    real_vector3d angular_accel = {0.0f, 0.0f, 0.0f};
    int32_t ground_contact_count = 0, on_ground_surface_count = 0, at_rest_count = 0, water_contact_count = 0;
    int32_t i;

    object_tag_data = tag_instances[self->definition_tag & 0xffff].data;
    definition = (Physics *)tag_instances[(uint16_t)(*(int32_t *)((uint8_t *)object_tag_data + 0x8c)) & 0xffff].data;
    gravity_scale = k_physics_gravity * definition->gravity_scale;

    matrix4x3_from_forward_up(&self->up, &self->forward, &step_matrix);
    step_matrix.position = self->position;
    total_force.k = -(gravity_scale * definition->mass);

    if (powered_states != 0 && definition->powered_mass_points.count > 0) {
        int16_t p;
        for (p = 0; p < definition->powered_mass_points.count; p++) {
            powered_mass_point_state *powered = &powered_states[p];
            float t;
            // FIXED (0x509f98..0x509f9e): ECX = the entry's quaternion (+0x1c), EDX = its matrix (+0x2c); the draft
            //   passed NULL, which crashes for any powered vehicle
            matrix4x3_from_quaternion((uint8_t *)powered + 0x1c, &powered->matrix_scale);
            t = powered->matrix[0][1]; powered->matrix[0][1] = powered->matrix[1][0]; powered->matrix[1][0] = t;
            t = powered->matrix[0][2]; powered->matrix[0][2] = powered->matrix[2][0]; powered->matrix[2][0] = t;
            t = powered->matrix[1][2]; powered->matrix[1][2] = powered->matrix[2][1]; powered->matrix[2][1] = t;
        }
    }

    memset(mass_point_states, 0, definition->mass_points.count * sizeof(mass_point_state));

    if (extra_force != (real_vector3d *)0) {
        total_force.i += extra_force->i;
        total_force.j += extra_force->j;
        total_force.k += extra_force->k;
    }
    if (extra_torque != (real_vector3d *)0) {
        total_torque.i += extra_torque->i;
        total_torque.j += extra_torque->j;
        total_torque.k += extra_torque->k;
    }

    for (i = 0; i < definition->mass_points.count; i++) {
        PhysicsMassPoint *mp_def = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        mass_point_state *mp = &mass_point_states[i];
        int16_t powered_index = mp_def->powered_mass_point;
        PhysicsPoweredMassPoint *powered_def = 0;
        powered_mass_point_state *powered_state = 0;
        Point3D local_position = mp_def->position;
        real_vector3d offset, velocity;

        if (powered_index != -1 && powered_states != 0) {
            powered_def = &((PhysicsPoweredMassPoint *)definition->powered_mass_points.pointer)[powered_index];
            powered_state = &powered_states[powered_index];
        }

        mp->flags = 0;
        if (powered_state == 0) {
            mp->position_x = local_position.x * step_matrix.forward.i + local_position.y * step_matrix.left.i +
                local_position.z * step_matrix.up.i + step_matrix.position.x;
            mp->position_y = local_position.x * step_matrix.forward.j + local_position.y * step_matrix.left.j +
                local_position.z * step_matrix.up.j + step_matrix.position.y;
            mp->position_z = local_position.x * step_matrix.forward.k + local_position.y * step_matrix.left.k +
                local_position.z * step_matrix.up.k + step_matrix.position.z;

            mp->forward_i = mp_def->forward.i * step_matrix.forward.i + mp_def->forward.j * step_matrix.left.i +
                mp_def->forward.k * step_matrix.up.i;
            mp->forward_j = mp_def->forward.i * step_matrix.forward.j + mp_def->forward.j * step_matrix.left.j +
                mp_def->forward.k * step_matrix.up.j;
            mp->forward_k = mp_def->forward.i * step_matrix.forward.k + mp_def->forward.j * step_matrix.left.k +
                mp_def->forward.k * step_matrix.up.k;
            mp->up_i = mp_def->up.i * step_matrix.forward.i + mp_def->up.j * step_matrix.left.i +
                mp_def->up.k * step_matrix.up.i;
            mp->up_j = mp_def->up.i * step_matrix.forward.j + mp_def->up.j * step_matrix.left.j +
                mp_def->up.k * step_matrix.up.j;
            mp->up_k = mp_def->up.i * step_matrix.forward.k + mp_def->up.j * step_matrix.left.k +
                mp_def->up.k * step_matrix.up.k;
        } else {
            real_matrix3x3 combined; // built from step_matrix * powered_state->matrix_scale
            matrix4x3_multiply(&step_matrix, &powered_state->matrix_scale, &combined);

            mp->position_x = local_position.x * step_matrix.forward.i + local_position.y * step_matrix.left.i +
                local_position.z * step_matrix.up.i + step_matrix.position.x;
            mp->position_y = local_position.x * step_matrix.forward.j + local_position.y * step_matrix.left.j +
                local_position.z * step_matrix.up.j + step_matrix.position.y;
            mp->position_z = local_position.x * step_matrix.forward.k + local_position.y * step_matrix.left.k +
                local_position.z * step_matrix.up.k + step_matrix.position.z;

            mp->forward_i = mp_def->forward.i * combined.forward.i + mp_def->forward.j * combined.left.i +
                mp_def->forward.k * combined.up.i;
            mp->forward_j = mp_def->forward.i * combined.forward.j + mp_def->forward.j * combined.left.j +
                mp_def->forward.k * combined.up.j;
            mp->forward_k = mp_def->forward.i * combined.forward.k + mp_def->forward.j * combined.left.k +
                mp_def->forward.k * combined.up.k;
            mp->up_i = mp_def->up.i * combined.forward.i + mp_def->up.j * combined.left.i +
                mp_def->up.k * combined.up.i;
            mp->up_j = mp_def->up.i * combined.forward.j + mp_def->up.j * combined.left.j +
                mp_def->up.k * combined.up.j;
            mp->up_k = mp_def->up.i * combined.forward.k + mp_def->up.j * combined.left.k +
                mp_def->up.k * combined.up.k;
        }

        // FIXED (0x50a300..0x50a348): ECX = the collision BSP [0x746f90] (the draft passed NULL and crashed in
        //   bsp3d_node_find_leaf once a hover vehicle's mass points were ticked), and the leaf is masked.
        mp->leaf_index = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&mp->position_x);
        mp->cluster_index = (mp->leaf_index == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[mp->leaf_index & 0x7fffffff].cluster;

        offset.i = mp->position_x - self->position.x;
        offset.j = mp->position_y - self->position.y;
        offset.k = mp->position_z - self->position.z;
        mp->offset_x = offset.i;
        mp->offset_y = offset.j;
        mp->offset_z = offset.k;

        velocity.i = self->angular_velocity.j * offset.k - offset.j * self->angular_velocity.k;
        velocity.j = offset.i * self->angular_velocity.k - self->angular_velocity.i * offset.k;
        velocity.k = self->angular_velocity.i * offset.j - self->angular_velocity.j * offset.i;
        velocity.i += self->velocity.i;
        velocity.j += self->velocity.j;
        velocity.k += self->velocity.k;
        mp->velocity_i = velocity.i;
        mp->velocity_j = velocity.j;
        mp->velocity_k = velocity.k;

        object_physics_mass_point_resolve_ground_contact(object_index, mp, mp_def);
        mp->water_depth = scenario_location_water_surface_distance((bsp_leaf_reference *)((uint8_t *)mp + 0x34),
            (real_point3d *)&mp->position_x); // EAX mass point +0x34, EDI +0x04

        if (0.0f < mp->ground_depth) {
            float tangential_speed = mp->resting_plane_i * mp->velocity_i + mp->resting_plane_j * mp->velocity_j +
                mp->resting_plane_k * mp->velocity_k;
            float friction_magnitude = -(mp_def->mass * definition->ground_friction);

            mp->ground_normal_magnitude = ((mp->ground_depth / definition->ground_depth) * k_physics_gravity -
                tangential_speed * definition->ground_damp_fraction) * mp_def->mass;
            mp->ground_normal_force_i = mp->ground_normal_magnitude * mp->resting_plane_i;
            mp->ground_normal_force_j = mp->ground_normal_magnitude * mp->resting_plane_j;
            mp->ground_normal_force_k = mp->ground_normal_magnitude * mp->resting_plane_k;

            mp->tangential_velocity_i = -tangential_speed * mp->resting_plane_i + mp->velocity_i;
            mp->tangential_velocity_j = -tangential_speed * mp->resting_plane_j + mp->velocity_j;
            mp->tangential_velocity_k = -tangential_speed * mp->resting_plane_k + mp->velocity_k;
            mp->ground_friction_force[0] = friction_magnitude * mp->tangential_velocity_i;
            mp->ground_friction_force[1] = friction_magnitude * mp->tangential_velocity_j;
            mp->ground_friction_force[2] = friction_magnitude * mp->tangential_velocity_k;

            if (powered_def != 0 && (powered_def->flags & 0x01) != 0 && powered_state->ground_friction != 0.0f) {
                float lean = real_inverse_lerp_clamped(mp->resting_plane_k, definition->ground_normal_k0, definition->ground_normal_k1);
                float alignment = mp->resting_plane_i * mp->up_i + mp->up_j * mp->resting_plane_j +
                    mp->up_k * mp->resting_plane_k;
                float scale, d, push_i, push_j, push_k;
                float neg_gf = -powered_state->ground_friction;

                if (alignment < 0.0f) alignment = 0.0f;
                else if (alignment > 1.0f) alignment = 1.0f;
                scale = alignment * alignment * lean * lean * friction_magnitude;

                d = -(neg_gf * mp->forward_i * mp->resting_plane_i + (neg_gf * mp->forward_j) * mp->resting_plane_j +
                    (neg_gf * mp->forward_k) * mp->resting_plane_k);
                push_i = d * mp->resting_plane_i + neg_gf * mp->forward_i;
                push_j = d * mp->resting_plane_j + neg_gf * mp->forward_j;
                push_k = d * mp->resting_plane_k + neg_gf * mp->forward_k;

                mp->tangential_velocity_i += push_i;
                mp->tangential_velocity_j += push_j;
                mp->tangential_velocity_k += push_k;
                mp->ground_friction_force[0] += push_i * scale;
                mp->ground_friction_force[1] += push_j * scale;
                mp->ground_friction_force[2] += push_k * scale;
            }

            if (mp->material_type == 0x1f) { // 0x50a605: material 0x1f blends at an eighth of both scales
                object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale * 0.125f,
                    mp_def->friction_perpendicular_scale * 0.125f, mp->ground_friction_force,
                    (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
            } else {
                object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                    mp_def->friction_perpendicular_scale, mp->ground_friction_force,
                    (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
            }
        }

        mp->flags = (mp->velocity_i * mp->velocity_i + mp->velocity_j * mp->velocity_j +
            mp->velocity_k * mp->velocity_k < 0.0011111111f) ? (mp->flags | _mass_point_at_rest_bit) : mp->flags;
        mp->flags = (mp->ground_depth <= 0.0f) ? mp->flags : (mp->flags | _mass_point_ground_contact_bit);
        mp->flags = (mp->water_depth <= 0.0f) ? mp->flags : (mp->flags | _mass_point_water_contact_bit);

        if (mp->water_depth <= 0.0f) {
            float d = -(mp_def->mass * definition->air_friction);
            if (powered_def != 0 && (powered_def->flags & 0x04) != 0 && powered_state->air_friction != 0.0f) {
                float neg = -powered_state->air_friction;
                float t1 = neg * mp->forward_j + mp->velocity_j;
                float t2 = neg * mp->forward_k + mp->velocity_k;
                d = -(mp_def->mass * definition->air_friction);
                mp->air_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
                mp->air_friction_force[1] = t1 * d;
                mp->air_friction_force[2] = t2 * d;
            } else {
                mp->air_friction_force[0] = d * mp->velocity_i;
                mp->air_friction_force[1] = d * mp->velocity_j;
                mp->air_friction_force[2] = d * mp->velocity_k;
            }
        } else {
            float water_fade = (definition->water_depth <= mp->water_depth) ? 1.0f :
                mp->water_depth / definition->water_depth;

            if (0.0f < mp_def->density && 0.0f < definition->water_depth) {
                float buoyancy = (mp_def->mass / mp_def->density) * definition->water_density * water_fade * gravity_scale;
                mp->buoyancy_magnitude = buoyancy;
                mp->buoyancy_force_i = 0.0f;
                mp->buoyancy_force_j = 0.0f;
                mp->buoyancy_force_k = buoyancy;
            }

            if (powered_def == 0 || (powered_def->flags & 0x02) == 0 || powered_state->water_friction == 0.0f) {
                float d = -(mp_def->mass * definition->water_friction);
                mp->water_friction_force[0] = d * mp->velocity_i;
                mp->water_friction_force[1] = d * mp->velocity_j;
                mp->water_friction_force[2] = d * mp->velocity_k;
            } else {
                float neg = -powered_state->water_friction;
                float t1 = neg * mp->forward_j + mp->velocity_j;
                float t2 = neg * mp->forward_k + mp->velocity_k;
                float d = -(mp_def->mass * definition->water_friction);
                mp->water_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
                mp->water_friction_force[1] = t1 * d;
                mp->water_friction_force[2] = t2 * d;
            }

            object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                mp_def->friction_perpendicular_scale, mp->water_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);

            if (powered_def != 0 && (powered_def->flags & 0x08) != 0 && powered_state->water_lift != 0.0f) {
                float lift = (float)fabs((double)(mp->velocity_i * mp->forward_i + mp->forward_j * mp->velocity_j +
                    mp->forward_k * mp->velocity_k)) * powered_state->water_lift * definition->mass * water_fade;
                mp->powered_force_i += lift * mp->up_i;
                mp->powered_force_j += lift * mp->up_j;
                mp->powered_force_k += lift * mp->up_k;
            }

            if (powered_def != 0 && (powered_def->flags & 0x04) != 0 && powered_state->air_friction != 0.0f) {
                float neg = -powered_state->air_friction;
                float t1 = neg * mp->forward_j + mp->velocity_j;
                float t2 = neg * mp->forward_k + mp->velocity_k;
                float d = -(mp_def->mass * definition->air_friction);
                mp->air_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
                mp->air_friction_force[1] = t1 * d;
                mp->air_friction_force[2] = t2 * d;
            } else if (powered_def == 0) {
                float d = -(mp_def->mass * definition->air_friction);
                mp->air_friction_force[0] = d * mp->velocity_i;
                mp->air_friction_force[1] = d * mp->velocity_j;
                mp->air_friction_force[2] = d * mp->velocity_k;
            }
        }
        object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
            mp_def->friction_perpendicular_scale, mp->air_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);

        if (powered_def != 0 && (powered_def->flags & 0x10) != 0 && powered_state->air_lift != 0.0f) {
            float lift = (float)fabs((double)(mp->velocity_i * mp->forward_i + mp->forward_j * mp->velocity_j +
                mp->forward_k * mp->velocity_k)) * definition->mass * powered_state->air_lift;
            mp->powered_force_i += lift * mp->up_i;
            mp->powered_force_j += lift * mp->up_j;
            mp->powered_force_k += lift * mp->up_k;
        }

        if (powered_def != 0) {
            if ((powered_def->flags & 0x20) != 0) {
                float thrust = powered_state->thrust * definition->mass;
                mp->powered_force_i += thrust * mp->forward_i;
                mp->powered_force_j += thrust * mp->forward_j;
                mp->powered_force_k += thrust * mp->forward_k;
            }
            if ((powered_def->flags & 0x40) != 0) {
                real_vector3d delta;
                collision_result probe_result;
                float probe_length = powered_def->antigrav_height + mp_def->radius;

                delta.i = probe_length * global_down3d_pointer->i;
                delta.j = probe_length * global_down3d_pointer->j;
                delta.k = probe_length * global_down3d_pointer->k;

                if (collision_test_movement_segment(0xc0a0, (real_point3d *)&mp->position_x, &delta,
                        object_index, &probe_result)) {
                    float clearance = probe_length * probe_result.t - mp_def->radius;
                    float lean = real_inverse_lerp_clamped(mp->up_k, powered_def->antigrav_normal_k0, powered_def->antigrav_normal_k1);
                    float fade = (clearance <= 0.0f) ? 1.0f : 1.0f - clearance / powered_def->antigrav_height;
                    float dot_nv = probe_result.plane.normal.i * mp->velocity_i + probe_result.plane.normal.k * mp->velocity_k +
                        probe_result.plane.normal.j * mp->velocity_j;
                    float push = (fade * fade * k_physics_gravity - dot_nv * powered_def->antigrav_damp_fraction) *
                        powered_state->antigrav * powered_def->antigrav_strength * definition->mass * lean;

                    mp->powered_force_i += probe_result.plane.normal.i * push;
                    mp->powered_force_j += probe_result.plane.normal.j * push;
                    mp->powered_force_k += probe_result.plane.normal.k * push;
                }
            }
        }

        mp->total_force_i = mp->ground_normal_force_i + mp->ground_friction_force[0] + mp->buoyancy_force_i +
            mp->water_friction_force[0] + mp->air_friction_force[0] + mp->powered_force_i;
        mp->total_force_j = mp->ground_normal_force_j + mp->ground_friction_force[1] + mp->buoyancy_force_j +
            mp->water_friction_force[1] + mp->air_friction_force[1] + mp->powered_force_j;
        mp->total_force_k = mp->ground_normal_force_k + mp->ground_friction_force[2] + mp->buoyancy_force_k +
            mp->water_friction_force[2] + mp->air_friction_force[2] + mp->powered_force_k;

        mp->torque_i = mp->total_force_k * mp->offset_y - mp->offset_z * mp->total_force_j;
        mp->torque_j = mp->total_force_i * mp->offset_z - mp->offset_x * mp->total_force_k; // 0x50ace7 (FIXED 2026-09-28: the draft had the sign flipped)
        mp->torque_k = mp->total_force_j * mp->offset_x - mp->total_force_i * mp->offset_y;

        total_force.i += mp->total_force_i;
        total_force.j += mp->total_force_j;
        total_force.k += mp->total_force_k;
        total_torque.i += mp->torque_i;
        total_torque.j += mp->torque_j;
        total_torque.k += mp->torque_k;

        at_rest_count += (mp->flags & _mass_point_at_rest_bit) != 0 ? 1 : 0;
        ground_contact_count += (mp->flags & _mass_point_ground_contact_bit) != 0 ? 1 : 0;
        on_ground_surface_count += (mp->flags & _mass_point_on_ground_surface_bit) != 0 ? 1 : 0;
        water_contact_count += (mp->flags & _mass_point_water_contact_bit) != 0 ? 1 : 0;
    }

    {
        if (definition->mass != 0.0f) {
            float inverse_mass = 1.0f / definition->mass;
            linear_accel.i = total_force.i * inverse_mass;
            linear_accel.j = total_force.j * inverse_mass;
            linear_accel.k = total_force.k * inverse_mass;
        }

        {
            real_vector3d torque_axis = total_torque;
            float torque_length = (float)sqrt((double)(torque_axis.i * torque_axis.i +
                torque_axis.j * torque_axis.j + torque_axis.k * torque_axis.k));

            if (0.0001f <= (float)fabs((double)torque_length) && torque_length != 0.0f) {
                float inverse_length = 1.0f / torque_length;
                float moment_sum = 0.0f;

                torque_axis.i *= inverse_length;
                torque_axis.j *= inverse_length;
                torque_axis.k *= inverse_length;

                for (i = 0; i < definition->mass_points.count; i++) {
                    PhysicsMassPoint *mp_def = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
                    mass_point_state *mp = &mass_point_states[i];
                    // Parallel-axis theorem: perpendicular-offset term (point mass at its offset,
                    // projected off the torque axis) plus the intrinsic solid-sphere moment of a
                    // mass point of this radius (2/5 * m * r^2, i.e. 0.4 * radius^2).
                    float along = -(torque_axis.i * mp->offset_x + torque_axis.j * mp->offset_y +
                        torque_axis.k * mp->offset_z);
                    float perp_x = torque_axis.i * along + mp->offset_x;
                    float perp_y = torque_axis.j * along + mp->offset_y;
                    float perp_z = torque_axis.k * along + mp->offset_z;

                    moment_sum += (perp_x * perp_x + mp_def->radius * mp_def->radius * 0.4f +
                        perp_y * perp_y + perp_z * perp_z) * mp_def->mass * definition->moment_scale;
                }

                if (moment_sum != 0.0f) {
                    float inverse_moment = 1.0f / moment_sum;
                    angular_accel.i = torque_axis.i * inverse_moment * torque_length;
                    angular_accel.j = torque_axis.j * inverse_moment * torque_length;
                    angular_accel.k = torque_axis.k * inverse_moment * torque_length;
                }
            }
        }

        self->velocity.i += linear_accel.i;
        self->velocity.j += linear_accel.j;
        self->velocity.k += linear_accel.k;
        self->angular_velocity.i += angular_accel.i;
        self->angular_velocity.j += angular_accel.j;
        self->angular_velocity.k += angular_accel.k;

        {
            real_point3d new_position;
            bsp_leaf_reference location;

            new_position.x = self->position.x + self->velocity.i;
            new_position.y = self->position.y + self->velocity.j;
            new_position.z = self->position.z + self->velocity.k;

            // FIXED (0x50af09..0x50af5d): ECX = the collision BSP [0x746f90]; the leaf is masked
            location.leaf_index = bsp3d_node_find_leaf(0, global_collision_bsp, &new_position);
            location.cluster_index = (location.leaf_index == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[location.leaf_index & 0x7fffffff].cluster;

            object_unlink_cluster_or_notify_parent(object_index);
            self->position = new_position;
            object_set_cluster_and_parent(object_index, &location);
        }

        {
            real_vector3d axis = self->angular_velocity;
            float axis_length = (float)sqrt((double)(axis.i * axis.i + axis.j * axis.j + axis.k * axis.k));

            if (0.0001f <= (float)fabs((double)axis_length)) {
                float inverse_length = 1.0f / axis_length;
                axis.i *= inverse_length;
                axis.j *= inverse_length;
                axis.k *= inverse_length;

                if (axis_length != 0.0f) {
                    float sin_angle = (real)sin((double)axis_length);
                    float cos_angle = (real)cos((double)axis_length);
                    real_vector3d forward_length_check;

                    // UNSURE: v/axis register pair reconstructed by analogy with
                    // object_physics_mass_point_update_orientation's (0x5096f0) own confirmed
                    // rotate-forward-then-up-then-reorthonormalize idiom; see file header.
                    vector3d_rotate_about_axis(&self->forward, &axis, sin_angle, cos_angle);
                    vector3d_rotate_about_axis(&self->up, &axis, sin_angle, cos_angle);

                    forward_length_check = self->forward;
                    {
                        float len = (float)sqrt((double)(forward_length_check.k * forward_length_check.k +
                            forward_length_check.j * forward_length_check.j +
                            forward_length_check.i * forward_length_check.i));
                        if (0.0001f <= (float)fabs((double)len)) {
                            float inv = 1.0f / len;
                            self->forward.i *= inv;
                            self->forward.j *= inv;
                            self->forward.k *= inv;
                        }
                    }

                    {
                        float neg_dot = -(self->up.i * self->forward.i + self->forward.j * self->up.j +
                            self->forward.k * self->up.k);
                        self->up.i += neg_dot * self->forward.i;
                        self->up.j += neg_dot * self->forward.j;
                        self->up.k += neg_dot * self->forward.k;

                        {
                            float len = (float)sqrt((double)(self->up.k * self->up.k + self->up.j * self->up.j +
                                self->up.i * self->up.i));
                            if (0.0001f <= (float)fabs((double)len)) {
                                float inv = 1.0f / len;
                                self->up.i *= inv;
                                self->up.j *= inv;
                                self->up.k *= inv;
                            }
                        }
                    }
                }
            }
        }
    }

    // The same four-term at-rest test object_physics_integrate_and_test_at_rest runs, against the
    // same four constants and in the same order: velocity (object +0x68, puVar15[0x1a..0x1c]),
    // angular velocity (+0x8c, pfVar1/puVar15[0x24]/[0x25]), then THIS tick's two increments
    // (local_50/4c/48 and local_38/34/30). An earlier rewrite of this file had the first two
    // swapped and dropped the last two clauses entirely; restored by the phase-4 pass. Each
    // comparison is `x < K != (x == K)` in Ghidra, i.e. x <= K.
    if (at_rest_count == definition->mass_points.count && ground_contact_count > 2 && on_ground_surface_count == 0 &&
        (self->velocity.i * self->velocity.i + self->velocity.j * self->velocity.j +
         self->velocity.k * self->velocity.k) <= 0.0011111111f &&
        (self->angular_velocity.i * self->angular_velocity.i + self->angular_velocity.j * self->angular_velocity.j +
         self->angular_velocity.k * self->angular_velocity.k) <= 0.0027415568f &&
        (linear_accel.i * linear_accel.i + linear_accel.j * linear_accel.j +
         linear_accel.k * linear_accel.k) <= 3.0864197e-07f &&
        (angular_accel.i * angular_accel.i + angular_accel.j * angular_accel.j +
         angular_accel.k * angular_accel.k) <= 3.0461742e-06f) {
        self->flags |= _object_at_rest_bit;
    } else {
        self->flags &= ~_object_at_rest_bit;
    }

    self->flags = (ground_contact_count >= 1) ? (self->flags | 0x02u) : (self->flags & ~0x02u); // UNSURE bit name
    self->flags = (water_contact_count >= 1) ? (self->flags | 0x04u) : (self->flags & ~0x04u);   // UNSURE bit name
    self->flags = (water_contact_count >= 1) ? (self->flags | 0x08u) : (self->flags & ~0x08u);   // UNSURE bit name

    if (water_contact_count != definition->mass_points.count) {
        self->flags &= ~0x10u; // UNSURE bit name
        return;
    }
    self->flags |= 0x10u; // UNSURE bit name
}

#if 0
Original Ghidra decompilation (0x509e80):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00509e80(uint param_1,int param_2,undefined4 *param_3,float *param_4,float *param_5)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  float fVar6;
  float fVar7;
  char cVar8;
  undefined2 uVar9;
  short sVar10;
  uint uVar11;
  int iVar12;
  int iVar13;
  int extraout_EDX;
  float *extraout_EDX_00;
  int iVar14;
  uint *puVar15;
  undefined4 *puVar16;
  float10 fVar17;
  float10 fVar18;
  float10 fVar19;
  float fVar20;
  float fVar21;
  undefined1 local_1a0 [20];
  float local_18c;
  float local_17c;
  float local_178;
  float local_174;
  float local_144;
  float local_13c;
  float local_138;
  uint local_134;
  uint local_130;
  uint local_12c;
  float local_124;
  float local_120;
  float local_11c;
  float local_118;
  float local_114;
  undefined1 local_110 [4];
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f8;
  float local_f4;
  float local_f0;
  float local_ec;
  int local_d4;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
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
  int local_70;
  int local_6c;
  uint *local_68;
  float local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float *local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  int local_10;
  float local_c;
  
  local_d4 = (param_1 & 0xffff) * 0xc;
  puVar15 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_d4);
  iVar14 = *(int *)((*(uint *)(*(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c) &
                    0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_ac = _DAT_0069c52c * *(float *)(iVar14 + 0x1c);
  local_58 = 0;
  local_54 = 0;
  local_60 = 0;
  local_5c = 0;
  local_68 = puVar15;
  local_10 = iVar14;
  matrix4x3_from_forward_up(&local_a8);
  local_80 = (float)puVar15[0x17];
  local_7c = (float)puVar15[0x18];
  local_78 = (float)puVar15[0x19];
  local_14 = -(local_ac * *(float *)(iVar14 + 8));
  local_1c = 0.0;
  local_18 = 0.0;
  local_28 = 0.0;
  local_24 = 0.0;
  local_20 = 0.0;
  local_50 = 0.0;
  local_4c = 0.0;
  local_48 = 0.0;
  local_38 = 0.0;
  local_34 = 0.0;
  local_30 = 0.0;
  if ((param_2 != 0) && (sVar10 = 0, 0 < *(int *)(iVar14 + 0x68))) {
    do {
      matrix4x3_from_quaternion();
      uVar4 = *(undefined4 *)(extraout_EDX + 8);
      *(undefined4 *)(extraout_EDX + 8) = *(undefined4 *)(extraout_EDX + 0x10);
      uVar5 = *(undefined4 *)(extraout_EDX + 0xc);
      *(undefined4 *)(extraout_EDX + 0x10) = uVar4;
      *(undefined4 *)(extraout_EDX + 0xc) = *(undefined4 *)(extraout_EDX + 0x1c);
      uVar4 = *(undefined4 *)(extraout_EDX + 0x18);
      sVar10 = sVar10 + 1;
      *(undefined4 *)(extraout_EDX + 0x18) = *(undefined4 *)(extraout_EDX + 0x20);
      *(undefined4 *)(extraout_EDX + 0x20) = uVar4;
      *(undefined4 *)(extraout_EDX + 0x1c) = uVar5;
    } while ((int)sVar10 < *(int *)(iVar14 + 0x68));
  }
  puVar16 = param_3;
  for (uVar11 = (uint)(*(int *)(iVar14 + 0x74) * 0x130) >> 2; uVar11 != 0; uVar11 = uVar11 - 1) {
    *puVar16 = 0;
    puVar16 = puVar16 + 1;
  }
  for (iVar12 = 0; iVar12 != 0; iVar12 = iVar12 + -1) {
    *(undefined1 *)puVar16 = 0;
    puVar16 = (undefined4 *)((int)puVar16 + 1);
  }
  if (param_4 != (float *)0x0) {
    local_1c = *param_4;
    local_14 = local_14 + param_4[2];
    local_18 = param_4[1];
  }
  if (param_5 != (float *)0x0) {
    local_28 = *param_5;
    local_24 = param_5[1];
    local_20 = param_5[2];
  }
  local_70 = 0;
  if (0 < *(int *)(iVar14 + 0x74)) {
    iVar14 = 0;
    do {
      iVar12 = iVar14 * 0x80 + *(int *)(local_10 + 0x78);
      sVar10 = *(short *)(iVar12 + 0x20);
      puVar15 = param_3 + iVar14 * 0x4c;
      if ((sVar10 == -1) || (param_2 == 0)) {
        param_4 = (float *)0x0;
LAB_0050a082:
        local_2c = (float *)0x0;
      }
      else {
        param_4 = (float *)(sVar10 * 0x80 + *(int *)(local_10 + 0x6c));
        if (param_4 == (float *)0x0) goto LAB_0050a082;
        local_2c = (float *)(sVar10 * 0x60 + param_2);
      }
      *puVar15 = 0;
      local_44 = *(float *)(iVar12 + 0x38) - *(float *)(local_10 + 0xc);
      local_40 = *(float *)(iVar12 + 0x3c) - *(float *)(local_10 + 0x10);
      local_3c = *(float *)(iVar12 + 0x40) - *(float *)(local_10 + 0x14);
      fVar21 = local_44;
      fVar20 = local_3c;
      fVar3 = local_40;
      if (local_a8 != 1.0) {
        fVar21 = local_a8 * local_44;
        fVar3 = local_40 * local_a8;
        fVar20 = local_3c * local_a8;
      }
      puVar15[1] = (uint)(local_a4 * fVar21 + local_98 * fVar3 + local_8c * fVar20 + local_80);
      puVar15[2] = (uint)(local_a0 * fVar21 + local_94 * fVar3 + local_88 * fVar20 + local_7c);
      puVar15[3] = (uint)(local_9c * fVar21 + local_90 * fVar3 + local_84 * fVar20 + local_78);
      if (local_2c == (float *)0x0) {
        fVar21 = *(float *)(iVar12 + 0x44);
        fVar20 = *(float *)(iVar12 + 0x48);
        fVar3 = *(float *)(iVar12 + 0x4c);
        puVar15[4] = (uint)(local_a4 * fVar21 + local_98 * fVar20 + local_8c * fVar3);
        puVar15[5] = (uint)(local_a0 * fVar21 + local_94 * fVar20 + local_88 * fVar3);
        puVar15[6] = (uint)(local_9c * fVar21 + local_90 * fVar20 + local_84 * fVar3);
        fVar21 = *(float *)(iVar12 + 0x50);
        fVar20 = *(float *)(iVar12 + 0x54);
        fVar3 = *(float *)(iVar12 + 0x58);
        puVar15[10] = (uint)(local_a4 * fVar21 + local_98 * fVar20 + local_8c * fVar3);
        puVar15[0xb] = (uint)(local_a0 * fVar21 + local_94 * fVar20 + local_88 * fVar3);
        fVar20 = local_90 * fVar20 + local_84 * fVar3;
        fVar3 = local_9c;
      }
      else {
        (*(code *)PTR_matrix4x3_multiply_00696664)(&local_a8,local_2c + 0xb,local_110);
        fVar21 = *(float *)(iVar12 + 0x44);
        fVar20 = *(float *)(iVar12 + 0x48);
        fVar3 = *(float *)(iVar12 + 0x4c);
        puVar15[4] = (uint)(local_10c * fVar21 + local_100 * fVar20 + local_f4 * fVar3);
        puVar15[5] = (uint)(local_108 * fVar21 + local_fc * fVar20 + local_f0 * fVar3);
        puVar15[6] = (uint)(local_104 * fVar21 + local_f8 * fVar20 + local_ec * fVar3);
        fVar21 = *(float *)(iVar12 + 0x50);
        fVar20 = *(float *)(iVar12 + 0x54);
        fVar3 = *(float *)(iVar12 + 0x58);
        puVar15[10] = (uint)(local_10c * fVar21 + local_100 * fVar20 + local_f4 * fVar3);
        puVar15[0xb] = (uint)(local_108 * fVar21 + local_fc * fVar20 + local_f0 * fVar3);
        fVar20 = local_f8 * fVar20 + local_ec * fVar3;
        fVar3 = local_104;
      }
      puVar15[0xc] = (uint)(fVar3 * fVar21 + fVar20);
      uVar11 = FUN_005013a0();
      puVar15[0xd] = uVar11;
      if (uVar11 == 0xffffffff) {
        uVar9 = 0xffff;
      }
      else {
        uVar9 = *(undefined2 *)(uVar11 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      *(undefined2 *)(puVar15 + 0xe) = uVar9;
      pfVar1 = (float *)(puVar15 + 0x12);
      puVar15[0xf] = (uint)((float)puVar15[1] - (float)local_68[0x17]);
      puVar15[0x10] = (uint)((float)puVar15[2] - (float)local_68[0x18]);
      puVar15[0x11] = (uint)((float)puVar15[3] - (float)local_68[0x19]);
      local_b8 = (float)puVar15[0x11] * (float)local_68[0x24] -
                 (float)local_68[0x25] * (float)puVar15[0x10];
      local_b4 = (float)local_68[0x25] * (float)puVar15[0xf] -
                 (float)local_68[0x23] * (float)puVar15[0x11];
      fVar21 = (float)local_68[0x23];
      fVar20 = (float)local_68[0x24];
      *pfVar1 = local_b8;
      puVar15[0x13] = (uint)local_b4;
      local_b0 = fVar21 * (float)puVar15[0x10] - (float)puVar15[0xf] * fVar20;
      puVar15[0x14] = (uint)local_b0;
      *pfVar1 = (float)local_68[0x1a] + *pfVar1;
      puVar15[0x13] = (uint)((float)puVar15[0x13] + (float)local_68[0x1b]);
      puVar15[0x14] = (uint)((float)puVar15[0x14] + (float)local_68[0x1c]);
      FUN_00507ac0(param_1,puVar15,iVar12);
      fVar17 = (float10)FUN_0053ee00();
      puVar15[0x1f] = (uint)(float)fVar17;
      if ((0.0 < (float)puVar15[0x1d]) && (0.0 < *(float *)(local_10 + 0x24))) {
        pfVar2 = (float *)(puVar15 + 0x24);
        fVar21 = *pfVar1 * (float)puVar15[0x18] +
                 (float)puVar15[0x13] * (float)puVar15[0x19] +
                 (float)puVar15[0x14] * (float)puVar15[0x1a];
        fVar20 = (((float)puVar15[0x1d] / *(float *)(local_10 + 0x24)) * _DAT_0069c52c -
                 fVar21 * *(float *)(local_10 + 0x28)) * *(float *)(local_10 + 8);
        puVar15[0x20] = (uint)fVar20;
        puVar15[0x21] = (uint)(fVar20 * (float)puVar15[0x18]);
        puVar15[0x22] = (uint)(fVar20 * (float)puVar15[0x19]);
        puVar15[0x23] = (uint)(fVar20 * (float)puVar15[0x1a]);
        local_c = -(*(float *)(iVar12 + 0x2c) * *(float *)(local_10 + 0x20));
        fVar21 = -fVar21;
        puVar15[0x15] = (uint)(fVar21 * (float)puVar15[0x18] + *pfVar1);
        puVar15[0x16] = (uint)(fVar21 * (float)puVar15[0x19] + (float)puVar15[0x13]);
        puVar15[0x17] = (uint)(fVar21 * (float)puVar15[0x1a] + (float)puVar15[0x14]);
        *pfVar2 = local_c * (float)puVar15[0x15];
        puVar15[0x25] = (uint)(local_c * (float)puVar15[0x16]);
        puVar15[0x26] = (uint)(local_c * (float)puVar15[0x17]);
        if ((param_4 != (float *)0x0) &&
           (((*(byte *)((int)param_4 + 0x20) & 1) != 0 && (*local_2c != 0.0)))) {
          fVar17 = (float10)FUN_00507430(puVar15[0x1a],*(undefined4 *)(local_10 + 0x30),
                                         *(undefined4 *)(local_10 + 0x2c));
          fVar19 = (float10)(float)puVar15[10] * (float10)(float)puVar15[0x18] +
                   (float10)(float)puVar15[0xb] * (float10)(float)puVar15[0x19] +
                   (float10)(float)puVar15[0xc] * (float10)(float)puVar15[0x1a];
          if ((float10)0.0 <= fVar19) {
            if ((float10)1.0 < fVar19) {
              fVar19 = (float10)1.0;
            }
          }
          else {
            fVar19 = (float10)0.0;
          }
          fVar17 = fVar19 * fVar19 * fVar17 * fVar17 * (float10)local_c;
          fVar19 = -(float10)*local_2c;
          local_cc = (float)(fVar19 * (float10)(float)puVar15[5]);
          local_c8 = (float)(fVar19 * (float10)(float)puVar15[6]);
          fVar18 = -(fVar19 * (float10)(float)puVar15[4] * (float10)(float)puVar15[0x18] +
                    (float10)local_cc * (float10)(float)puVar15[0x19] +
                    fVar19 * (float10)(float)puVar15[6] * (float10)(float)puVar15[0x1a]);
          local_c = (float)fVar18;
          fVar18 = fVar18 * (float10)(float)puVar15[0x18] + fVar19 * (float10)(float)puVar15[4];
          fVar19 = (float10)local_c * (float10)(float)puVar15[0x19] + (float10)local_cc;
          local_144 = local_c * (float)puVar15[0x1a] + local_c8;
          puVar15[0x15] = (uint)(float)(fVar18 + (float10)(float)puVar15[0x15]);
          puVar15[0x16] = (uint)(float)(fVar19 + (float10)(float)puVar15[0x16]);
          puVar15[0x17] = (uint)(local_144 + (float)puVar15[0x17]);
          *pfVar2 = (float)(fVar18 * fVar17 + (float10)*pfVar2);
          puVar15[0x25] = (uint)(float)(fVar19 * fVar17 + (float10)(float)puVar15[0x25]);
          puVar15[0x26] = (uint)(float)((float10)local_144 * fVar17 + (float10)(float)puVar15[0x26])
          ;
        }
        if ((short)puVar15[0x1c] == 0x1f) {
          fVar21 = *(float *)(iVar12 + 100) * 0.125;
          uVar11 = (uint)*(ushort *)(iVar12 + 0x5c);
          fVar20 = *(float *)(iVar12 + 0x60) * 0.125;
        }
        else {
          fVar21 = *(float *)(iVar12 + 100);
          fVar20 = *(float *)(iVar12 + 0x60);
          uVar11 = (uint)*(short *)(iVar12 + 0x5c);
        }
        FUN_00507c00(uVar11,fVar20,fVar21);
      }
      if ((float)puVar15[0x1f] <= 0.0) {
LAB_0050a808:
        if (((param_4 == (float *)0x0) || ((*(byte *)((int)param_4 + 0x20) & 4) == 0)) ||
           (local_2c[2] == 0.0)) goto LAB_0050a886;
        fVar21 = -local_2c[2];
        local_124 = fVar21 * (float)puVar15[5] + (float)puVar15[0x13];
        local_120 = fVar21 * (float)puVar15[6] + (float)puVar15[0x14];
        local_c = -(*(float *)(iVar12 + 0x2c) * *(float *)(local_10 + 0x48));
        puVar15[0x3a] = (uint)(local_c * (fVar21 * (float)puVar15[4] + *pfVar1));
        puVar15[0x3b] = (uint)(local_124 * local_c);
        fVar21 = local_120 * local_c;
      }
      else {
        if (*(float *)(local_10 + 0x3c) <= (float)puVar15[0x1f]) {
          local_64 = 1.0;
        }
        else {
          local_64 = (float)puVar15[0x1f] / *(float *)(local_10 + 0x3c);
        }
        if ((0.0 < *(float *)(iVar12 + 0x34)) && (0.0 < *(float *)(local_10 + 0x3c))) {
          fVar21 = (*(float *)(local_10 + 0x40) / *(float *)(iVar12 + 0x34)) *
                   *(float *)(iVar12 + 0x2c) * local_64 * local_ac;
          puVar15[0x2d] = (uint)fVar21;
          puVar15[0x2e] = 0;
          puVar15[0x30] = (uint)fVar21;
          puVar15[0x2f] = 0;
        }
        if (((param_4 == (float *)0x0) || ((*(byte *)((int)param_4 + 0x20) & 2) == 0)) ||
           (local_2c[1] == 0.0)) {
          fVar21 = -(*(float *)(iVar12 + 0x2c) * *(float *)(local_10 + 0x38));
          puVar15[0x31] = (uint)(fVar21 * *pfVar1);
          puVar15[0x32] = (uint)(fVar21 * (float)puVar15[0x13]);
          fVar21 = fVar21 * (float)puVar15[0x14];
        }
        else {
          fVar21 = -local_2c[1];
          local_13c = fVar21 * (float)puVar15[5] + (float)puVar15[0x13];
          local_138 = fVar21 * (float)puVar15[6] + (float)puVar15[0x14];
          local_c = -(*(float *)(iVar12 + 0x2c) * *(float *)(local_10 + 0x38));
          puVar15[0x31] = (uint)(local_c * (fVar21 * (float)puVar15[4] + *pfVar1));
          puVar15[0x32] = (uint)(local_13c * local_c);
          fVar21 = local_138 * local_c;
        }
        puVar15[0x33] = (uint)fVar21;
        FUN_00507c00(*(undefined2 *)(iVar12 + 0x5c),*(undefined4 *)(iVar12 + 0x60),
                     *(undefined4 *)(iVar12 + 100));
        if (param_4 != (float *)0x0) {
          if (((*(byte *)((int)param_4 + 0x20) & 8) != 0) && (local_2c[3] != 0.0)) {
            fVar21 = ABS((float)puVar15[4] * *pfVar1 +
                         (float)puVar15[5] * (float)puVar15[0x13] +
                         (float)puVar15[6] * (float)puVar15[0x14]) * local_2c[3] *
                     *(float *)(local_10 + 8) * local_64;
            puVar15[0x43] = (uint)(fVar21 * (float)puVar15[10] + (float)puVar15[0x43]);
            puVar15[0x44] = (uint)(fVar21 * (float)puVar15[0xb] + (float)puVar15[0x44]);
            puVar15[0x45] = (uint)(fVar21 * (float)puVar15[0xc] + (float)puVar15[0x45]);
          }
          goto LAB_0050a808;
        }
LAB_0050a886:
        fVar21 = -(*(float *)(iVar12 + 0x2c) * *(float *)(local_10 + 0x48));
        puVar15[0x3a] = (uint)(fVar21 * *pfVar1);
        puVar15[0x3b] = (uint)(fVar21 * (float)puVar15[0x13]);
        fVar21 = fVar21 * (float)puVar15[0x14];
      }
      puVar15[0x3c] = (uint)fVar21;
      FUN_00507c00((int)*(short *)(iVar12 + 0x5c),*(undefined4 *)(iVar12 + 0x60),
                   *(undefined4 *)(iVar12 + 100));
      if (((param_4 != (float *)0x0) && ((*(byte *)((int)param_4 + 0x20) & 0x10) != 0)) &&
         (local_2c[4] != 0.0)) {
        fVar21 = ABS((float)puVar15[4] * *pfVar1 +
                     (float)puVar15[5] * (float)puVar15[0x13] +
                     (float)puVar15[6] * (float)puVar15[0x14]) * *(float *)(local_10 + 8) *
                 local_2c[4];
        puVar15[0x43] = (uint)(fVar21 * (float)puVar15[10] + (float)puVar15[0x43]);
        puVar15[0x44] = (uint)(fVar21 * (float)puVar15[0xb] + (float)puVar15[0x44]);
        puVar15[0x45] = (uint)(fVar21 * (float)puVar15[0xc] + (float)puVar15[0x45]);
      }
      if (0.0011111111 <=
          *pfVar1 * *pfVar1 +
          (float)puVar15[0x13] * (float)puVar15[0x13] + (float)puVar15[0x14] * (float)puVar15[0x14])
      {
        uVar11 = *puVar15 & 0xfffffffe;
      }
      else {
        uVar11 = *puVar15 | 1;
      }
      *puVar15 = uVar11;
      if ((float)puVar15[0x1d] <= 0.0) {
        uVar11 = *puVar15 & 0xfffffffd;
      }
      else {
        uVar11 = *puVar15 | 2;
      }
      *puVar15 = uVar11;
      if ((float)puVar15[0x1f] <= 0.0) {
        uVar11 = *puVar15 & 0xfffffff7;
      }
      else {
        uVar11 = *puVar15 | 8;
      }
      *puVar15 = uVar11;
      local_58 = local_58 + ((byte)*puVar15 & 1);
      uVar11 = *puVar15;
      local_54 = local_54 + (uVar11 >> 1 & 1);
      local_60 = local_60 + (uVar11 >> 2 & 1);
      local_5c = local_5c + (uVar11 >> 3 & 1);
      if (param_4 != (float *)0x0) {
        if ((*(byte *)((int)param_4 + 0x20) & 0x20) != 0) {
          fVar21 = local_2c[5] * *(float *)(local_10 + 8);
          puVar15[0x43] = (uint)(fVar21 * (float)puVar15[4] + (float)puVar15[0x43]);
          puVar15[0x44] = (uint)(fVar21 * (float)puVar15[5] + (float)puVar15[0x44]);
          puVar15[0x45] = (uint)(fVar21 * (float)puVar15[6] + (float)puVar15[0x45]);
        }
        if ((*(byte *)((int)param_4 + 0x20) & 0x40) != 0) {
          local_134 = puVar15[1];
          local_130 = puVar15[2];
          local_12c = puVar15[3];
          local_114 = *(float *)((int)param_4 + 0x2c) + *(float *)(iVar12 + 0x68);
          local_11c = local_114 * *(float *)PTR_DAT_0069672c;
          local_118 = local_114 * *(float *)(PTR_DAT_0069672c + 4);
          local_114 = local_114 * *(float *)(PTR_DAT_0069672c + 8);
          cVar8 = FUN_00505880(0xc0a0,&local_134,&local_11c,param_1,local_1a0);
          if (cVar8 != '\0') {
            fVar21 = (*(float *)((int)param_4 + 0x2c) + *(float *)(iVar12 + 0x68)) * local_18c -
                     *(float *)(iVar12 + 0x68);
            fVar17 = (float10)FUN_00507430(puVar15[0xc],*(undefined4 *)((int)param_4 + 0x38),
                                           *(undefined4 *)((int)param_4 + 0x34));
            if (fVar21 <= 0.0) {
              fVar19 = (float10)1.0;
            }
            else {
              fVar19 = (float10)1.0 - (float10)fVar21 / (float10)*(float *)((int)param_4 + 0x2c);
            }
            fVar17 = (fVar19 * fVar19 * (float10)_DAT_0069c52c -
                     ((float10)local_17c * (float10)*pfVar1 +
                     (float10)local_174 * (float10)(float)puVar15[0x14] +
                     (float10)local_178 * (float10)(float)puVar15[0x13]) *
                     (float10)*(float *)((int)param_4 + 0x30)) * (float10)local_2c[6] *
                     (float10)*(float *)((int)param_4 + 0x24) * (float10)*(float *)(local_10 + 8) *
                     fVar17;
            puVar15[0x43] =
                 (uint)(float)((float10)local_17c * fVar17 + (float10)(float)puVar15[0x43]);
            puVar15[0x44] =
                 (uint)(float)((float10)local_178 * fVar17 + (float10)(float)puVar15[0x44]);
            puVar15[0x45] =
                 (uint)(float)((float10)local_174 * fVar17 + (float10)(float)puVar15[0x45]);
          }
        }
      }
      puVar15[0x46] = (uint)((float)puVar15[0x21] + (float)puVar15[0x46]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x22]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x23]);
      puVar15[0x46] = (uint)((float)puVar15[0x24] + (float)puVar15[0x46]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x25]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x26]);
      puVar15[0x46] = (uint)((float)puVar15[0x46] + (float)puVar15[0x2e]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x2f]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x30]);
      puVar15[0x46] = (uint)((float)puVar15[0x46] + (float)puVar15[0x31]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x32]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x33]);
      puVar15[0x46] = (uint)((float)puVar15[0x46] + (float)puVar15[0x3a]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x3b]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x3c]);
      puVar15[0x46] = (uint)((float)puVar15[0x46] + (float)puVar15[0x43]);
      puVar15[0x47] = (uint)((float)puVar15[0x47] + (float)puVar15[0x44]);
      puVar15[0x48] = (uint)((float)puVar15[0x48] + (float)puVar15[0x45]);
      local_c4 = (float)puVar15[0x10] * (float)puVar15[0x48] -
                 (float)puVar15[0x47] * (float)puVar15[0x11];
      local_c0 = (float)puVar15[0x46] * (float)puVar15[0x11] -
                 (float)puVar15[0xf] * (float)puVar15[0x48];
      puVar15[0x49] = (uint)local_c4;
      puVar15[0x4a] = (uint)local_c0;
      local_bc = (float)puVar15[0xf] * (float)puVar15[0x47] -
                 (float)puVar15[0x10] * (float)puVar15[0x46];
      puVar15[0x4b] = (uint)local_bc;
      local_1c = local_1c + (float)puVar15[0x46];
      local_18 = local_18 + (float)puVar15[0x47];
      local_14 = local_14 + (float)puVar15[0x48];
      local_28 = local_28 + (float)puVar15[0x49];
      local_24 = local_24 + (float)puVar15[0x4a];
      local_20 = local_20 + (float)puVar15[0x4b];
      local_70 = local_70 + 1;
      iVar14 = (int)(short)local_70;
    } while (iVar14 < *(int *)(local_10 + 0x74));
  }
  iVar14 = local_10;
  puVar15 = local_68;
  if (*(float *)(local_10 + 8) != 0.0) {
    local_48 = 1.0 / *(float *)(local_10 + 8);
    local_50 = local_1c * local_48;
    local_4c = local_18 * local_48;
    local_48 = local_14 * local_48;
  }
  local_14 = local_20;
  local_18 = local_24;
  local_1c = local_28;
  fVar21 = SQRT(local_28 * local_28 + local_24 * local_24 + local_20 * local_20);
  if (0.0001 <= ABS(fVar21)) {
    local_14 = 1.0 / fVar21;
    local_1c = local_28 * local_14;
    local_18 = local_24 * local_14;
    local_14 = local_20 * local_14;
    if (fVar21 != 0.0) {
      fVar21 = 0.0;
      sVar10 = 0;
      if (0 < *(int *)(local_10 + 0x74)) {
        iVar12 = 0;
        do {
          iVar13 = iVar12 * 0x80 + *(int *)(local_10 + 0x78);
          sVar10 = sVar10 + 1;
          fVar20 = -(local_1c * (float)param_3[iVar12 * 0x4c + 0xf] +
                    local_18 * (float)param_3[iVar12 * 0x4c + 0x10] +
                    local_14 * (float)param_3[iVar12 * 0x4c + 0x11]);
          fVar7 = local_1c * fVar20 + (float)param_3[iVar12 * 0x4c + 0xf];
          fVar6 = local_18 * fVar20 + (float)param_3[iVar12 * 0x4c + 0x10];
          fVar3 = local_14 * fVar20 + (float)param_3[iVar12 * 0x4c + 0x11];
          iVar12 = (int)sVar10;
          fVar20 = *(float *)(iVar13 + 0x68);
          fVar21 = (fVar7 * fVar7 + fVar20 * fVar20 * 0.4 + fVar6 * fVar6 + fVar3 * fVar3) *
                   *(float *)(iVar13 + 0x2c) * *(float *)(local_10 + 4) + fVar21;
        } while (iVar12 < *(int *)(local_10 + 0x74));
        if (fVar21 != 0.0) {
          fVar21 = 1.0 / fVar21;
          local_38 = local_28 * fVar21;
          local_34 = local_24 * fVar21;
          local_30 = local_20 * fVar21;
        }
      }
    }
  }
  pfVar1 = (float *)(local_68 + 0x23);
  local_68[0x1a] = (uint)(local_50 + (float)local_68[0x1a]);
  local_68[0x1b] = (uint)(local_4c + (float)local_68[0x1b]);
  local_68[0x1c] = (uint)(local_48 + (float)local_68[0x1c]);
  *pfVar1 = local_38 + *pfVar1;
  local_68[0x24] = (uint)(local_34 + (float)local_68[0x24]);
  local_68[0x25] = (uint)(local_30 + (float)local_68[0x25]);
  local_44 = (float)local_68[0x17] + (float)local_68[0x1a];
  local_40 = (float)local_68[0x18] + (float)local_68[0x1b];
  local_3c = (float)local_68[0x19] + (float)local_68[0x1c];
  local_6c = FUN_005013a0();
  if (local_6c == -1) {
    uVar9 = 0xffff;
  }
  else {
    uVar9 = *(undefined2 *)(local_6c * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  local_68 = (uint *)CONCAT22(local_68._2_2_,uVar9);
  iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_d4);
  object_unlink_cluster_or_notify_parent();
  *(float *)(iVar12 + 0x5c) = local_44;
  *(float *)(iVar12 + 0x60) = local_40;
  *(float *)(iVar12 + 100) = local_3c;
  object_set_cluster_and_parent(param_1,&local_6c);
  local_28 = *pfVar1;
  local_24 = (float)puVar15[0x24];
  local_20 = (float)puVar15[0x25];
  fVar21 = SQRT(local_28 * local_28 + local_24 * local_24 + local_20 * local_20);
  if (0.0001 <= ABS(fVar21)) {
    fVar20 = 1.0 / fVar21;
    local_28 = local_28 * fVar20;
    local_24 = local_24 * fVar20;
    local_20 = local_20 * fVar20;
    if (fVar21 != 0.0) {
      fVar17 = (float10)fsin((float10)fVar21);
      fVar19 = (float10)fcos((float10)fVar21);
      vector3d_rotate_about_axis((float)fVar17,(float)fVar19);
      pfVar2 = (float *)(puVar15 + 0x20);
      vector3d_rotate_about_axis((float)fVar17,(float)fVar19);
      fVar21 = SQRT(extraout_EDX_00[2] * extraout_EDX_00[2] +
                    extraout_EDX_00[1] * extraout_EDX_00[1] + *extraout_EDX_00 * *extraout_EDX_00);
      if (0.0001 <= ABS(fVar21)) {
        fVar21 = 1.0 / fVar21;
        *extraout_EDX_00 = fVar21 * *extraout_EDX_00;
        extraout_EDX_00[1] = fVar21 * extraout_EDX_00[1];
        extraout_EDX_00[2] = fVar21 * extraout_EDX_00[2];
      }
      fVar21 = -(*pfVar2 * *extraout_EDX_00 +
                extraout_EDX_00[1] * (float)puVar15[0x21] +
                extraout_EDX_00[2] * (float)puVar15[0x22]);
      *pfVar2 = fVar21 * *extraout_EDX_00 + *pfVar2;
      puVar15[0x21] = (uint)(fVar21 * extraout_EDX_00[1] + (float)puVar15[0x21]);
      puVar15[0x22] = (uint)(fVar21 * extraout_EDX_00[2] + (float)puVar15[0x22]);
      fVar21 = SQRT((float)puVar15[0x22] * (float)puVar15[0x22] +
                    (float)puVar15[0x21] * (float)puVar15[0x21] + *pfVar2 * *pfVar2);
      iVar14 = local_10;
      if (0.0001 <= ABS(fVar21)) {
        fVar21 = 1.0 / fVar21;
        *pfVar2 = fVar21 * *pfVar2;
        puVar15[0x21] = (uint)(fVar21 * (float)puVar15[0x21]);
        puVar15[0x22] = (uint)(fVar21 * (float)puVar15[0x22]);
      }
    }
  }
  if (((((int)(short)local_58 == *(int *)(iVar14 + 0x74)) && (2 < (short)local_54)) &&
      ((((short)local_60 == 0 &&
        ((fVar21 = (float)puVar15[0x1c] * (float)puVar15[0x1c] +
                   (float)puVar15[0x1b] * (float)puVar15[0x1b] +
                   (float)puVar15[0x1a] * (float)puVar15[0x1a],
         fVar21 < 0.0011111111 != (fVar21 == 0.0011111111) &&
         (fVar21 = (float)puVar15[0x25] * (float)puVar15[0x25] +
                   (float)puVar15[0x24] * (float)puVar15[0x24] + *pfVar1 * *pfVar1,
         fVar21 < 0.0027415568 != (fVar21 == 0.0027415568))))) &&
       (fVar21 = local_50 * local_50 + local_4c * local_4c + local_48 * local_48,
       fVar21 < 3.0864197e-07 != (fVar21 == 3.0864197e-07))))) &&
     (fVar21 = local_38 * local_38 + local_34 * local_34 + local_30 * local_30,
     fVar21 < 3.0461742e-06 != (fVar21 == 3.0461742e-06))) {
    uVar11 = puVar15[4] | 0x20;
  }
  else {
    uVar11 = puVar15[4] & 0xffffffdf;
  }
  puVar15[4] = uVar11;
  if ((short)local_54 < 1) {
    uVar11 = uVar11 & 0xfffffffd;
  }
  else {
    uVar11 = uVar11 | 2;
  }
  puVar15[4] = uVar11;
  sVar10 = (short)local_5c;
  if (sVar10 < 1) {
    uVar11 = puVar15[4] & 0xfffffffb;
  }
  else {
    uVar11 = puVar15[4] | 4;
  }
  puVar15[4] = uVar11;
  if (sVar10 < 1) {
    uVar11 = uVar11 & 0xfffffff7;
  }
  else {
    uVar11 = uVar11 | 8;
  }
  puVar15[4] = uVar11;
  if ((int)sVar10 != *(int *)(iVar14 + 0x74)) {
    puVar15[4] = uVar11 & 0xffffffef;
    return;
  }
  puVar15[4] = uVar11 | 0x10;
  return;
}
#endif
