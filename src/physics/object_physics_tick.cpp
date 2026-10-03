/**
 * Per-tick force and torque integration of an object carrying a physics tag, one mass point at a time.
 */

#include "tags.h"
#include "halo/scenario/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"
#include "physics.h"
#include <string.h>

#include "halo/physics/object_physics.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/objects/api.hpp"

extern "C" { extern double fabs(double x); }
extern "C" { extern double sqrt(double x); }
extern "C" { extern double sin(double x); }
extern "C" { extern double cos(double x); }
extern "C" { extern data_array *object_data; }
extern "C" { extern ModelCollisionGeometryBSP *global_collision_bsp; }
extern "C" { extern real_vector3d *global_down3d_pointer; }
extern "C" { extern float k_physics_gravity; }
extern "C" { extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); }
extern "C" { extern void object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition); }
extern "C" { extern void object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up); }
extern "C" { extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); }
namespace halo::physics {

/**
 * The single-pass alternative to object_physics_tick's general path: for each of Physics'
 * mass points (transformed by a fresh object-forward/up matrix at the object's raw position, not
 * object_physics_context's centre-of-mass-shifted one), computes the same ground/water/air
 * friction, buoyancy and powered forces as object_physics_compute_mass_point_forces (without a
 * GlobalsMaterial lookup), immediately resolves its own per-mass-point collision, then folds the
 * summed force into a linear acceleration and the summed torque into a single scalar-moment-of-
 * inertia angular acceleration, integrates the object's position/velocity/orientation by one tick,
 * updates its cluster, and tallies the same ground/water/at-rest object flags as
 *
 * @address 0x509e80
 */
void ObjectPhysics::tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, mass_point_state *mass_point_states, real_vector3d *extra_force, real_vector3d *extra_torque)
{
    object *self = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *object_tag_data;
    Physics *definition;
    float gravity_scale;
    real_matrix4x3 step_matrix;
    real_vector3d total_force = {0.0f, 0.0f, 0.0f};
    real_vector3d total_torque = {0.0f, 0.0f, 0.0f};

    real_vector3d linear_accel = {0.0f, 0.0f, 0.0f};
    real_vector3d angular_accel = {0.0f, 0.0f, 0.0f};
    int32_t ground_contact_count = 0, on_ground_surface_count = 0, at_rest_count = 0, water_contact_count = 0;
    int32_t i;

    object_tag_data = halo::cache::globals().tag_instances[self->definition_tag & 0xffff].data;
    definition = (Physics *)halo::cache::globals().tag_instances[(uint16_t)(*(int32_t *)((uint8_t *)object_tag_data + 0x8c)) & 0xffff].data;
    gravity_scale = k_physics_gravity * definition->gravity_scale;

    halo::math::matrix4x3_from_forward_up(self->up, self->forward, step_matrix);
    step_matrix.position = self->position;
    total_force.k = -(gravity_scale * definition->mass);

    if (powered_states != 0 && definition->powered_mass_points.count > 0) {
        int16_t p;
        for (p = 0; p < definition->powered_mass_points.count; p++) {
            powered_mass_point_state *powered = &powered_states[p];
            float t;

            halo::math::matrix4x3_from_quaternion(*reinterpret_cast<real_quaternion *>((uint8_t *)powered + 0x1c), *reinterpret_cast<real_matrix4x3 *>(&powered->matrix_scale));
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
        {
            float local_x = mp_def->position.x - definition->center_of_mass.x;
            float local_y = mp_def->position.y - definition->center_of_mass.y;
            float local_z = mp_def->position.z - definition->center_of_mass.z;
            real_matrix4x3 combined;
            const real_matrix4x3 *basis = &step_matrix;

            if (step_matrix.scale != 1.0f) {
                local_x = local_x * step_matrix.scale;
                local_y = local_y * step_matrix.scale;
                local_z = local_z * step_matrix.scale;
            }
            mp->position_x = ((local_z * step_matrix.up.i + local_y * step_matrix.left.i) +
                local_x * step_matrix.forward.i) + step_matrix.position.x;
            mp->position_y = ((local_z * step_matrix.up.j + local_y * step_matrix.left.j) +
                local_x * step_matrix.forward.j) + step_matrix.position.y;
            mp->position_z = ((local_z * step_matrix.up.k + local_y * step_matrix.left.k) +
                local_x * step_matrix.forward.k) + step_matrix.position.z;

            if (powered_state != 0) {
                halo::math::matrix4x3_multiply(&step_matrix, reinterpret_cast<real_matrix4x3 *>(&powered_state->matrix_scale), &combined);
                basis = &combined;
            }
            mp->forward_i = (mp_def->forward.k * basis->up.i + mp_def->forward.j * basis->left.i) +
                mp_def->forward.i * basis->forward.i;
            mp->forward_j = (mp_def->forward.k * basis->up.j + mp_def->forward.j * basis->left.j) +
                mp_def->forward.i * basis->forward.j;
            mp->forward_k = (mp_def->forward.k * basis->up.k + mp_def->forward.j * basis->left.k) +
                mp_def->forward.i * basis->forward.k;
            mp->up_i = (mp_def->up.k * basis->up.i + mp_def->up.j * basis->left.i) + mp_def->up.i * basis->forward.i;
            mp->up_j = (mp_def->up.k * basis->up.j + mp_def->up.j * basis->left.j) + mp_def->up.i * basis->forward.j;
            mp->up_k = (mp_def->up.k * basis->up.k + mp_def->up.j * basis->left.k) + mp_def->up.i * basis->forward.k;
        }

        mp->leaf_index = halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&mp->position_x);
        mp->cluster_index = (mp->leaf_index == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[mp->leaf_index & 0x7fffffff].cluster;

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

        halo::physics::object_physics_mass_point_resolve_ground_contact(object_index, mp, mp_def);
        mp->water_depth = halo::scenario::scenario_location_water_surface_distance((bsp_leaf_reference *)((uint8_t *)mp + 0x34),
            (real_point3d *)&mp->position_x);

        if (0.0f < mp->ground_depth && 0.0f < definition->ground_depth) {
            float tangential_speed = (mp->velocity_k * mp->resting_plane_k + mp->velocity_j * mp->resting_plane_j) +
                mp->velocity_i * mp->resting_plane_i;
            float friction_magnitude = -(mp_def->mass * definition->ground_friction);

            mp->ground_normal_magnitude = ((mp->ground_depth / definition->ground_depth) * k_physics_gravity -
                tangential_speed * definition->ground_damp_fraction) * definition->mass;
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
                float lean = halo::math::real_inverse_lerp_clamped(mp->resting_plane_k, definition->ground_normal_k0, definition->ground_normal_k1);
                float alignment = (mp->up_k * mp->resting_plane_k + mp->up_j * mp->resting_plane_j) +
                    mp->up_i * mp->resting_plane_i;
                float scale, d, push_i, push_j, push_k;
                float neg_gf = -powered_state->ground_friction;

                if (alignment < 0.0f) alignment = 0.0f;
                else if (alignment > 1.0f) alignment = 1.0f;
                scale = lean * (alignment * alignment * lean) * friction_magnitude;

                d = -(((neg_gf * mp->forward_k) * mp->resting_plane_k + (neg_gf * mp->forward_j) * mp->resting_plane_j) +
                    (neg_gf * mp->forward_i) * mp->resting_plane_i);
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

            if (mp->material_type == 0x1f) {
                halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale * 0.125f,
                    mp_def->friction_perpendicular_scale * 0.125f, mp->ground_friction_force,
                    (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
            } else {
                halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                    mp_def->friction_perpendicular_scale, mp->ground_friction_force,
                    (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
            }
        }

        if (0.0f < mp->water_depth) {
            float water_fade = (definition->water_depth <= mp->water_depth) ? 1.0f :
                mp->water_depth / definition->water_depth;

            if (0.0f < mp_def->density && 0.0f < definition->water_depth) {
                float buoyancy = (((definition->water_density / mp_def->density) * mp_def->mass) * water_fade) * gravity_scale;
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

            halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                mp_def->friction_perpendicular_scale, mp->water_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);

            if (powered_def != 0 && (powered_def->flags & 0x08) != 0 && powered_state->water_lift != 0.0f) {
                float lift = (float)fabs((double)((mp->forward_k * mp->velocity_k + mp->forward_j * mp->velocity_j) +
                    mp->forward_i * mp->velocity_i)) * powered_state->water_lift * definition->mass * water_fade;
                mp->powered_force_i += lift * mp->up_i;
                mp->powered_force_j += lift * mp->up_j;
                mp->powered_force_k += lift * mp->up_k;
            }
        }

        if (powered_def != 0 && (powered_def->flags & 0x04) != 0 && powered_state->air_friction != 0.0f) {
            float neg = -powered_state->air_friction;
            float t1 = neg * mp->forward_j + mp->velocity_j;
            float t2 = neg * mp->forward_k + mp->velocity_k;
            float d = -(mp_def->mass * definition->air_friction);
            mp->air_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
            mp->air_friction_force[1] = t1 * d;
            mp->air_friction_force[2] = t2 * d;
        } else {
            float d = -(mp_def->mass * definition->air_friction);
            mp->air_friction_force[0] = d * mp->velocity_i;
            mp->air_friction_force[1] = d * mp->velocity_j;
            mp->air_friction_force[2] = d * mp->velocity_k;
        }
        halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
            mp_def->friction_perpendicular_scale, mp->air_friction_force,
            (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);

        if (powered_def != 0 && (powered_def->flags & 0x10) != 0 && powered_state->air_lift != 0.0f) {
            float lift = (float)fabs((double)((mp->forward_k * mp->velocity_k + mp->forward_j * mp->velocity_j) +
                mp->forward_i * mp->velocity_i)) * definition->mass * powered_state->air_lift;
            mp->powered_force_i += lift * mp->up_i;
            mp->powered_force_j += lift * mp->up_j;
            mp->powered_force_k += lift * mp->up_k;
        }

        mp->flags = ((mp->velocity_k * mp->velocity_k + mp->velocity_j * mp->velocity_j) + mp->velocity_i * mp->velocity_i <
            0.0011111111f) ? (mp->flags | _mass_point_at_rest_bit) : (mp->flags & ~(uint32_t)_mass_point_at_rest_bit);
        mp->flags = (mp->ground_depth <= 0.0f) ? (mp->flags & ~(uint32_t)_mass_point_ground_contact_bit) :
            (mp->flags | _mass_point_ground_contact_bit);
        mp->flags = (mp->water_depth <= 0.0f) ? (mp->flags & ~(uint32_t)_mass_point_water_contact_bit) :
            (mp->flags | _mass_point_water_contact_bit);
        at_rest_count += (mp->flags & _mass_point_at_rest_bit) != 0 ? 1 : 0;
        ground_contact_count += (mp->flags & _mass_point_ground_contact_bit) != 0 ? 1 : 0;
        on_ground_surface_count += (mp->flags & _mass_point_on_ground_surface_bit) != 0 ? 1 : 0;
        water_contact_count += (mp->flags & _mass_point_water_contact_bit) != 0 ? 1 : 0;

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

                if (halo::physics::collision_test_movement_segment(0xc0a0, (real_point3d *)&mp->position_x, &delta,
                        object_index, &probe_result)) {
                    float clearance = probe_length * probe_result.t - mp_def->radius;
                    float lean = halo::math::real_inverse_lerp_clamped(mp->up_k, powered_def->antigrav_normal_k0, powered_def->antigrav_normal_k1);
                    float fade = (clearance <= 0.0f) ? 1.0f : 1.0f - clearance / powered_def->antigrav_height;
                    float dot_nv = (probe_result.plane.normal.j * mp->velocity_j + probe_result.plane.normal.k * mp->velocity_k) +
                        probe_result.plane.normal.i * mp->velocity_i;
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
        mp->torque_j = mp->total_force_i * mp->offset_z - mp->offset_x * mp->total_force_k;
        mp->torque_k = mp->total_force_j * mp->offset_x - mp->total_force_i * mp->offset_y;

        total_force.i += mp->total_force_i;
        total_force.j += mp->total_force_j;
        total_force.k += mp->total_force_k;
        total_torque.i += mp->torque_i;
        total_torque.j += mp->torque_j;
        total_torque.k += mp->torque_k;
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

            float torque_length = (float)sqrt((double)((torque_axis.k * torque_axis.k +
                torque_axis.j * torque_axis.j) + torque_axis.i * torque_axis.i));

            if (0.0001f <= (float)fabs((double)torque_length) && torque_length != 0.0f) {
                float inverse_length = 1.0f / torque_length;
                float moment_sum = 0.0f;

                torque_axis.i *= inverse_length;
                torque_axis.j *= inverse_length;
                torque_axis.k *= inverse_length;

                for (i = 0; i < definition->mass_points.count; i++) {
                    PhysicsMassPoint *mp_def = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
                    mass_point_state *mp = &mass_point_states[i];

                    float along = -((torque_axis.k * mp->offset_z + torque_axis.j * mp->offset_y) +
                        torque_axis.i * mp->offset_x);
                    float perp_x = torque_axis.i * along + mp->offset_x;
                    float perp_y = torque_axis.j * along + mp->offset_y;
                    float perp_z = torque_axis.k * along + mp->offset_z;

                    moment_sum += ((((perp_z * perp_z + perp_y * perp_y) + mp_def->radius * mp_def->radius * 0.4f) +
                        perp_x * perp_x) * mp_def->mass) * definition->moment_scale;
                }

                if (moment_sum != 0.0f) {
                    float inverse_moment = 1.0f / moment_sum;

                    angular_accel.i = total_torque.i * inverse_moment;
                    angular_accel.j = total_torque.j * inverse_moment;
                    angular_accel.k = total_torque.k * inverse_moment;
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

            location.leaf_index = halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, &new_position);
            location.cluster_index = (location.leaf_index == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[location.leaf_index & 0x7fffffff].cluster;

            halo::objects::object_unlink_cluster_or_notify_parent(object_index);
            self->position = new_position;
            halo::objects::object_set_cluster_and_parent(object_index, &location);
        }

        {
            real_vector3d axis = self->angular_velocity;
            float axis_length = (float)sqrt((double)((axis.k * axis.k + axis.j * axis.j) + axis.i * axis.i));

            if (0.0001f <= (float)fabs((double)axis_length)) {
                float inverse_length = 1.0f / axis_length;
                axis.i *= inverse_length;
                axis.j *= inverse_length;
                axis.k *= inverse_length;

                if (axis_length != 0.0f) {
                    float sin_angle = (real)sin((double)axis_length);
                    float cos_angle = (real)cos((double)axis_length);
                    real_vector3d forward_length_check;

                    halo::math::vector3d_rotate_about_axis(self->forward, axis, sin_angle, cos_angle);
                    halo::math::vector3d_rotate_about_axis(self->up, axis, sin_angle, cos_angle);

                    forward_length_check = self->forward;
                    {
                        float len = (float)sqrt((double)((forward_length_check.i * forward_length_check.i +
                            forward_length_check.j * forward_length_check.j) +
                            forward_length_check.k * forward_length_check.k));
                        if (0.0001f <= (float)fabs((double)len)) {
                            float inv = 1.0f / len;
                            self->forward.i *= inv;
                            self->forward.j *= inv;
                            self->forward.k *= inv;
                        }
                    }

                    {
                        float neg_dot = -((self->forward.k * self->up.k + self->forward.j * self->up.j) +
                            self->up.i * self->forward.i);
                        self->up.i += neg_dot * self->forward.i;
                        self->up.j += neg_dot * self->forward.j;
                        self->up.k += neg_dot * self->forward.k;

                        {
                            float len = (float)sqrt((double)((self->up.i * self->up.i + self->up.j * self->up.j) +
                                self->up.k * self->up.k));
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

    if (at_rest_count == definition->mass_points.count && ground_contact_count > 2 && on_ground_surface_count == 0 &&
        (self->velocity.i * self->velocity.i + self->velocity.j * self->velocity.j +
         self->velocity.k * self->velocity.k) <= 0.0011111111f &&
        (self->angular_velocity.i * self->angular_velocity.i + self->angular_velocity.j * self->angular_velocity.j +
         self->angular_velocity.k * self->angular_velocity.k) <= 0.0027415568f &&
        ((linear_accel.k * linear_accel.k + linear_accel.j * linear_accel.j) +
         linear_accel.i * linear_accel.i) <= 3.0864197e-07f &&
        ((angular_accel.k * angular_accel.k + angular_accel.j * angular_accel.j) +
         angular_accel.i * angular_accel.i) <= 3.0461742e-06f) {
        self->flags |= _object_at_rest_bit;
    } else {
        self->flags &= ~_object_at_rest_bit;
    }

    self->flags = (ground_contact_count >= 1) ? (self->flags | 0x02u) : (self->flags & ~0x02u);
    self->flags = (water_contact_count >= 1) ? (self->flags | 0x04u) : (self->flags & ~0x04u);
    self->flags = (water_contact_count >= 1) ? (self->flags | 0x08u) : (self->flags & ~0x08u);

    if (water_contact_count != definition->mass_points.count) {
        self->flags &= ~0x10u;
        return;
    }
    self->flags |= 0x10u;
}

}
