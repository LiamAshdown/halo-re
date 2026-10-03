/**
 * Per-tick force and torque integration of an object carrying a physics tag, one mass point at a time.
 */

#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "halo/scenario/api.hpp"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "cache.h"
#include "objects.h"
#include <string.h>
#include "game.h"
#include "projectiles.h"
#include "units.h"

#include "halo/physics/object_physics.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

extern "C" { void halo::physics::object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up); }
extern "C" { uint8_t halo::physics::object_physics_check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index); }
extern "C" { void halo::physics::object_physics_compute_mass_point_forces(object_physics_context *context, powered_mass_point_state *powered_states, uint32_t mass_points_address, real_vector3d *out_force, real_vector3d *out_torque); }
extern "C" { uint8_t halo::physics::object_physics_context_build(uint32_t object_index, object_physics_context *out_context); }
extern "C" { void halo::physics::object_physics_handle_nearby_object_impacts(uint32_t object_index); }
extern "C" { void halo::physics::object_physics_integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states, real_vector3d *torque, real_vector3d *force); }
extern "C" { void halo::physics::object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition); }
extern "C" { void halo::physics::object_physics_mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward, real_vector3d *fallback_forward, real_vector3d *fallback_up); }
extern "C" { uint8_t halo::physics::object_physics_resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other); }

extern "C" { extern void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index); }
namespace halo::physics {

/**
 * Adds one sphere (and, for a positive x_offset, the lowered sphere and pill) proxy per Physics
 * mass point of the context's object: the mass point position through the context matrix, height
 * x_offset, radius mass_point.radius * context->scale + y_offset, no surface or material.
 * Returns whether the physics_model now holds any proxy (its three int16 counts).
 * with an invented signature and dropped the transformed position.
 *
 * Original register convention: EBX -> context, stack -> x_offset, y_offset, model_counts.
 *
 * @address 0x507790
 */
uint8_t ObjectPhysics::add_mass_point_shapes(float x_offset, float y_offset, object_physics_context *context, int16_t *model_counts)
{
    Physics *definition = (Physics *)context->definition;
    int16_t i;

    for (i = 0; (int32_t)i < (int32_t)definition->mass_points.count; i++) {
        PhysicsMassPoint *mass_point = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        real_point3d world_position;

        halo::math::matrix4x3_transform_point(world_position, *((real_point3d *)&mass_point->position),
            *((real_matrix4x3 *)&context->scale));
        halo::physics::physics_shape_vertex_to_sphere((physics_model *)model_counts, &world_position, -1, x_offset,
            mass_point->radius * context->scale + y_offset, context->object_index, -1, 0, -1);
    }

    return !(model_counts[0] == 0 && model_counts[1] == 0 && model_counts[2] == 0);
}

}

namespace halo::physics {

/**
 * friction[0..2] holds the force on entry. Type 0 keeps it all parallel (friction[3..5]) with no
 * perpendicular part and returns without blending. Types 1, 2 and 3 split it against an axis --
 * the forward, cross(forward, up), or the up -- into parallel (friction[3..5]) and perpendicular
 * (friction[6..8]) parts; any other type keeps whatever the caller left there. The parts are then
 * scaled and summed back into friction[0..2].
 *
 * @address 0x507c00
 */
void ObjectPhysics::blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up)
{
    real_vector3d cross;
    real_vector3d *axis = 0;

    if (friction_type == 0) {
        friction[3] = friction[0];
        friction[4] = friction[1];
        friction[5] = friction[2];
        friction[6] = 0.0f;
        friction[7] = 0.0f;
        friction[8] = 0.0f;
        return;
    }
    if (friction_type == 1) {
        axis = forward;
    } else if (friction_type == 2) {
        halo::math::vector3d_cross_product(cross, *forward, *up);
        axis = &cross;
    } else if (friction_type == 3) {
        axis = up;
    }
    if (axis != 0) {
        halo::math::vector3d_project_onto_unit_axis((real_vector3d *)&friction[3], *axis, *(real_vector3d *)friction,
            (real_vector3d *)&friction[6]);
    }

    friction[3] = parallel_scale * friction[3];
    friction[4] = parallel_scale * friction[4];
    friction[5] = parallel_scale * friction[5];
    friction[6] = perpendicular_scale * friction[6];
    friction[7] = perpendicular_scale * friction[7];
    friction[8] = perpendicular_scale * friction[8];
    friction[0] = friction[3] + friction[6];
    friction[1] = friction[7] + friction[4];
    friction[2] = friction[8] + friction[5];
}

}

extern "C" { extern Globals *global_globals; }
extern "C" { extern float k_impact_damage_scale_table[]; }
extern "C" { extern uint32_t object_collision_context_test_point(object_collision_context *context, real_point3d *point); }
extern "C" { extern uint8_t object_collision_context_gather_sphere_shapes(void *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model); }
extern "C" { extern uint8_t physics_shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact); }
extern "C" { extern double sqrt(double x); }
namespace halo::physics {

/**
 * Tests whether self_object_index and candidate_object_index are colliding hard enough for
 * impact damage: recovers a contact point via FUN_00505200/physics_shape_test_point when self's own
 * collision-node resolution (object_collision_context_test_point) is incomplete, computes a relative-impact velocity,
 * nudges the candidate via object_set_position_and_relink, and applies damage to whichever side
 * (or both) the two "falling damage"-style thresholds indicate.
 *
 * @address 0x508b70
 */
uint8_t ObjectPhysics::check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index)
{
    uint8_t hit_recorded = 0;
    float sample[5];
    real_point3d *contact_point = (real_point3d *)&sample[2];
    real_vector3d impulse;
    real_point3d recovered_position;
    damage_data dd;
    object *self_obj;
    object *candidate_obj;
    real_point3d *self_center;
    float relative_speed;
    float clamped_speed;

    halo::units::unit_get_crouch_height_offset(contact_point, candidate_object_index, &sample[0], &sample[1]);

    if (!halo::physics::object_collision_context_test_point((object_collision_context *)self_object_index, contact_point)) {
        physics_model model;
        physics_model_contact contact;
        float sphere_radius;
        float thickness;

        impulse.i = contact_point->x;
        impulse.j = contact_point->y;
        impulse.k = contact_point->z + sample[0] * 0.5f;
        model.sphere_count = 0;
        model.pill_count = 0;
        model.shape_count = 0;
        sphere_radius = sample[0] * 0.5f + sample[1];
        thickness = sample[1] - 0.015625f;
        if (thickness <= 0.015625f) {
            thickness = 0.015625f;
        }

        halo::physics::object_collision_context_gather_sphere_shapes((object_collision_context *)self_object_index,
            (real_point3d *)&impulse, sphere_radius, sample[0], thickness, &model);
        if (!halo::physics::physics_shape_test_point(&model, contact_point, &contact)) {
            return hit_recorded;
        }
    }

    self_obj = ((object_header *)halo::objects::globals().object_data->data)[*self_object_index & halo::k_slot_mask].data;
    candidate_obj = ((object_header *)halo::objects::globals().object_data->data)[candidate_object_index & halo::k_slot_mask].data;
    self_center = &self_obj->bounding_center;

    relative_speed = (float)sqrt((double)(self_obj->velocity.k * self_obj->velocity.k +
        self_obj->velocity.j * self_obj->velocity.j + self_obj->velocity.i * self_obj->velocity.i));

    impulse.i = candidate_obj->bounding_center.x - self_center->x;
    impulse.j = candidate_obj->bounding_center.y - self_center->y;
    impulse.k = candidate_obj->bounding_center.z - self_center->z;

    halo::math::vector3d_normalize_with_length(impulse);
    impulse.k += 0.8f;
    halo::math::vector3d_normalize_with_length(impulse);

    clamped_speed = (relative_speed <= 0.1f) ? 0.1f : relative_speed;
    impulse.i = (impulse.i * clamped_speed + self_obj->velocity.i) * 0.5f;
    impulse.j = (impulse.j * clamped_speed + self_obj->velocity.j) * 0.5f;
    impulse.k = (clamped_speed * impulse.k + self_obj->velocity.k) * 0.5f;

    halo::units::unit_apply_impulse(candidate_object_index, &impulse);

    contact_point->x = impulse.i + impulse.i + contact_point->x;
    contact_point->y = impulse.j + impulse.j + contact_point->y;
    contact_point->z = impulse.k + impulse.k + contact_point->z;

    hit_recorded = 0;
    {
        uint8_t recovered = halo::physics::physics_point_find_clear_position(0x20c3a0, contact_point,
            sample[1] + sample[1], sample[0], sample[1], candidate_object_index,
            &recovered_position);
        if (recovered) {
            recovered_position.z = recovered_position.z - sample[1];

            halo::objects::object_set_position_and_relink(&recovered_position, candidate_object_index, 0);

            if (*self_object_index == *(uint32_t *)((uint8_t *)candidate_obj + 0x32c) &&
                halo::game::globals().game_time->game_time <=
                    (int32_t)(*(uint32_t *)((uint8_t *)candidate_obj + 0x330) + 0x5a)) {
                return 1;
            }
            if (relative_speed <= 0.06666667f) {
                float dx = candidate_obj->velocity.i - self_obj->velocity.i;
                float dy = candidate_obj->velocity.j - self_obj->velocity.j;
                float dz = candidate_obj->velocity.k - self_obj->velocity.k;
                if (dx * dx + dy * dy + dz * dz <= 0.0011111111f) {
                    return 1;
                }
            }
        }
    }

    {
        uint8_t *collision_damage_tag = *(uint8_t **)((uint8_t *)global_globals + 0x18c);
        int32_t impact_damage_tag_id = *(int32_t *)(collision_damage_tag + 0x68);
        int32_t breakable_damage_tag_id;

        if (impact_damage_tag_id != -1) {
            uint32_t driver = *(uint32_t *)((uint8_t *)self_obj + 0x324);
            uint32_t responsible = *self_object_index;
            object *responsible_obj = self_obj;

            if (driver != halo::k_dword_none) {
                responsible_obj = ((object_header *)halo::objects::globals().object_data->data)[driver & halo::k_slot_mask].data;
                responsible = driver;
            }

            memset(&dd, 0, sizeof(dd));
            dd.flags |= 1;
            dd.material_type = -1;
            dd.location_cluster_index = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = 1.0f;
            dd.responsible_player = responsible_obj->owner_linkage;
            dd.responsible_object = responsible;
            if (responsible_obj->creator_object != halo::k_dword_none) {
                dd.responsible_object = responsible_obj->creator_object;
            }
            dd.team_index = responsible_obj->owner_team;

            dd.epicentre = candidate_obj->bounding_center;
            dd.origin = *self_center;
            dd.direction = impulse;
            dd.damage_effect_tag = impact_damage_tag_id;

            halo::math::vector3d_normalize_with_length(dd.direction);
            halo::objects::object_apply_damage(&dd, candidate_object_index, -1, -1, -1, 0);
        }

        breakable_damage_tag_id = *(int32_t *)(collision_damage_tag + 0x58);
        if (breakable_damage_tag_id != -1) {
            void *candidate_tag = halo::cache::globals().tag_instances[candidate_obj->definition_tag & halo::k_slot_mask].data;

            memset(&dd, 0, sizeof(dd));
            dd.epicentre = candidate_obj->bounding_center;
            dd.team_index = -1;
            dd.responsible_player = -1;
            dd.responsible_object = -1;
            dd.location_cluster_index = -1;
            dd.material_type = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = k_impact_damage_scale_table[
                *(int16_t *)((uint8_t *)candidate_tag + 0x298)];
            dd.direction.i = impulse.i * -1.0f;
            dd.direction.j = impulse.j * -1.0f;
            dd.direction.k = impulse.k * -1.0f;
            dd.damage_effect_tag = breakable_damage_tag_id;

            halo::objects::object_apply_damage(&dd, *self_object_index, -1, -1, -1, 0);
        }
    }

    return 1;
}

}

extern "C" { extern double fabs(double x); }
extern "C" { extern ModelCollisionGeometryBSP *global_collision_bsp; }
extern "C" { extern real_vector3d *global_down3d_pointer; }
extern "C" { extern float k_physics_gravity; }
extern "C" { extern uint8_t material_table_warning_issued; }
extern "C" { extern int32_t material_table_bad_index; }
extern "C" { extern uint8_t material_table_fallback[0x374]; }
extern "C" { extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); }
extern "C" { extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); }
namespace halo::physics {

/**
 * Computes gravity plus every per-mass-point force (ground/water/air friction, buoyancy, powered
 * thrust/lift/antigrav) for context's object, filling out_mass_points (zeroed first, one
 * mass_point_state per Physics.mass_points entry) and summing the per-mass-point total_force and
 * torque into *out_force / *out_torque (seeded with straight gravity beforehand). powered_states
 * is the runtime powered_mass_point_state array built by object_physics_tick's quaternion loop,
 * or NULL when the object has no live powered-mass-point state.
 *
 * @address 0x507cc0
 */
void ObjectPhysics::compute_mass_point_forces(object_physics_context *context, powered_mass_point_state *powered_states, uint32_t mass_points_address, real_vector3d *out_force, real_vector3d *out_torque)
{
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[context->object_index & halo::k_slot_mask].data;
    Physics *definition = (Physics *)context->definition;
    float gravity_scale = k_physics_gravity * definition->gravity_scale;
    mass_point_state *mass_points = (mass_point_state *)mass_points_address;
    int32_t i;

    out_force->i = 0.0f;
    out_force->j = 0.0f;
    out_force->k = -(gravity_scale * definition->mass);
    out_torque->i = 0.0f;
    out_torque->j = 0.0f;
    out_torque->k = 0.0f;

    memset(mass_points, 0, definition->mass_points.count * sizeof(mass_point_state));

    for (i = 0; i < definition->mass_points.count; i++) {
        PhysicsMassPoint *mp_def = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        mass_point_state *mp = &mass_points[i];
        int16_t powered_index = mp_def->powered_mass_point;
        PhysicsPoweredMassPoint *powered_def = 0;
        powered_mass_point_state *powered_state = 0;
        real_vector3d offset, velocity;

        if (powered_index != -1 && powered_states != 0) {
            powered_def = &((PhysicsPoweredMassPoint *)definition->powered_mass_points.pointer)[powered_index];
            if ((void *)powered_def != 0) {
                powered_state = &powered_states[powered_index];
            } else {
                powered_def = 0;
            }
        }

        mp->flags = 0;
        halo::math::matrix4x3_transform_point(*((real_point3d *)&mp->position_x), *((real_point3d *)&mp_def->position),
            *((real_matrix4x3 *)&context->scale));

        if (powered_state == 0) {
            mp->forward_i = mp_def->forward.k * context->up_i + mp_def->forward.j * context->left_i +
                mp_def->forward.i * context->forward_i;
            mp->forward_j = mp_def->forward.k * context->up_j + mp_def->forward.j * context->left_j +
                mp_def->forward.i * context->forward_j;
            mp->forward_k = mp_def->forward.k * context->up_k + mp_def->forward.j * context->left_k +
                mp_def->forward.i * context->forward_k;
            mp->up_i = mp_def->up.k * context->up_i + mp_def->up.j * context->left_i +
                mp_def->up.i * context->forward_i;
            mp->up_j = mp_def->up.k * context->up_j + mp_def->up.j * context->left_j +
                mp_def->up.i * context->forward_j;
            mp->up_k = mp_def->up.k * context->up_k + mp_def->up.j * context->left_k +
                mp_def->up.i * context->forward_k;
        } else {
            real_matrix4x3 combined;
            halo::math::matrix4x3_multiply(reinterpret_cast<real_matrix4x3 *>(&context->scale), reinterpret_cast<real_matrix4x3 *>(&powered_state->matrix_scale), &combined);
            mp->forward_i = mp_def->forward.k * combined.up.i + mp_def->forward.j * combined.left.i +
                mp_def->forward.i * combined.forward.i;
            mp->forward_j = mp_def->forward.k * combined.up.j + mp_def->forward.j * combined.left.j +
                mp_def->forward.i * combined.forward.j;
            mp->forward_k = mp_def->forward.k * combined.up.k + mp_def->forward.j * combined.left.k +
                mp_def->forward.i * combined.forward.k;
            mp->up_i = mp_def->up.k * combined.up.i + mp_def->up.j * combined.left.i + mp_def->up.i * combined.forward.i;
            mp->up_j = mp_def->up.k * combined.up.j + mp_def->up.j * combined.left.j + mp_def->up.i * combined.forward.j;
            mp->up_k = mp_def->up.k * combined.up.k + mp_def->up.j * combined.left.k + mp_def->up.i * combined.forward.k;
        }

        mp->leaf_index = halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&mp->position_x);
        mp->cluster_index = (mp->leaf_index == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[mp->leaf_index & 0x7fffffff].cluster;

        offset.i = mp->position_x - obj->position.x;
        offset.j = mp->position_y - obj->position.y;
        offset.k = mp->position_z - obj->position.z;
        mp->offset_x = offset.i;
        mp->offset_y = offset.j;
        mp->offset_z = offset.k;

        velocity.i = obj->angular_velocity.j * offset.k - offset.j * obj->angular_velocity.k;
        velocity.j = offset.i * obj->angular_velocity.k - obj->angular_velocity.i * offset.k;
        velocity.k = obj->angular_velocity.i * offset.j - obj->angular_velocity.j * offset.i;
        velocity.i += obj->velocity.i;
        velocity.j += obj->velocity.j;
        velocity.k += obj->velocity.k;
        mp->velocity_i = velocity.i;
        mp->velocity_j = velocity.j;
        mp->velocity_k = velocity.k;

        halo::physics::object_physics_mass_point_resolve_ground_contact(context->object_index, mp, mp_def);
        mp->water_depth = halo::scenario::scenario_location_water_surface_distance((bsp_leaf_reference *)((uint8_t *)mp + 0x34),
            (real_point3d *)&mp->position_x);

        if (0.0f < mp->ground_depth) {
            GlobalsMaterial *material;
            int16_t material_index = mp->material_type;
            float ground_friction, ground_normal_k1, ground_normal_k0, ground_depth_scale,
                ground_damp_fraction_scale;
            float tangential_speed;

            if (material_index < 0 || (int32_t)material_index >= global_globals->materials.count) {
                if (!material_table_warning_issued) {
                    material_table_bad_index = -1;
                    material_table_warning_issued = 1;
                }
                material = (GlobalsMaterial *)material_table_fallback;
            } else {
                material = &((GlobalsMaterial *)global_globals->materials.pointer)[material_index];
            }

            ground_friction = (!(material->ground_friction_scale > 0.0f) ||
                !(definition->mass <= 7500.0f)) ? definition->ground_friction :
                definition->ground_friction * material->ground_friction_scale;
            ground_normal_k1 = !(material->ground_friction_normal_k1_scale > 0.0f) ?
                definition->ground_normal_k1 :
                definition->ground_normal_k1 * material->ground_friction_normal_k1_scale;
            ground_normal_k0 = !(material->ground_friction_normal_k0_scale > 0.0f) ?
                definition->ground_normal_k0 :
                definition->ground_normal_k0 * material->ground_friction_normal_k0_scale;
            ground_depth_scale = definition->ground_depth;
            if (0.0f < material->ground_depth_scale) {
                ground_depth_scale *= material->ground_depth_scale;
            }
            ground_damp_fraction_scale = definition->ground_damp_fraction;
            if (0.0f < material->ground_damp_fraction_scale) {
                ground_damp_fraction_scale *= material->ground_damp_fraction_scale;
            }

            tangential_speed = mp->velocity_k * mp->resting_plane_k + mp->velocity_j * mp->resting_plane_j +
                mp->velocity_i * mp->resting_plane_i;

            mp->ground_normal_magnitude = ((mp->ground_depth / ground_depth_scale) * k_physics_gravity -
                tangential_speed * ground_damp_fraction_scale) * definition->mass;
            mp->ground_normal_force_i = mp->ground_normal_magnitude * mp->resting_plane_i;
            mp->ground_normal_force_j = mp->ground_normal_magnitude * mp->resting_plane_j;
            mp->ground_normal_force_k = mp->ground_normal_magnitude * mp->resting_plane_k;

            {
                float friction_magnitude = -(ground_friction * mp_def->mass);
                mp->tangential_velocity_i = -tangential_speed * mp->resting_plane_i + mp->velocity_i;
                mp->tangential_velocity_j = -tangential_speed * mp->resting_plane_j + mp->velocity_j;
                mp->tangential_velocity_k = -tangential_speed * mp->resting_plane_k + mp->velocity_k;
                mp->ground_friction_force[0] = friction_magnitude * mp->tangential_velocity_i;
                mp->ground_friction_force[1] = friction_magnitude * mp->tangential_velocity_j;
                mp->ground_friction_force[2] = friction_magnitude * mp->tangential_velocity_k;

                if (powered_def != 0 && (powered_def->flags & 0x01) != 0 && powered_state->ground_friction != 0.0f) {
                    float lean = halo::math::real_inverse_lerp_clamped(mp->resting_plane_k, ground_normal_k0, ground_normal_k1);
                    float alignment = mp->up_k * mp->resting_plane_k + mp->up_j * mp->resting_plane_j +
                        mp->resting_plane_i * mp->up_i;
                    float scale;
                    float push_i, push_j, push_k;
                    float d;

                    if (alignment < 0.0f) alignment = 0.0f;
                    else if (alignment > 1.0f) alignment = 1.0f;
                    scale = lean * (alignment * alignment * lean) * friction_magnitude;

                    d = -((-(powered_state->ground_friction) * mp->forward_k) * mp->resting_plane_k +
                          (-(powered_state->ground_friction) * mp->forward_j) * mp->resting_plane_j +
                          (-(powered_state->ground_friction) * mp->forward_i) * mp->resting_plane_i);
                    push_i = d * mp->resting_plane_i + -(powered_state->ground_friction) * mp->forward_i;
                    push_j = d * mp->resting_plane_j + -(powered_state->ground_friction) * mp->forward_j;
                    push_k = d * mp->resting_plane_k + -(powered_state->ground_friction) * mp->forward_k;

                    mp->tangential_velocity_i += push_i;
                    mp->tangential_velocity_j += push_j;
                    mp->tangential_velocity_k += push_k;
                    mp->ground_friction_force[0] += push_i * scale;
                    mp->ground_friction_force[1] += push_j * scale;
                    mp->ground_friction_force[2] += push_k * scale;
                }
            }

            halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                mp_def->friction_perpendicular_scale, mp->ground_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
        }

        if (mp->water_depth <= 0.0f) {
            goto powered_air_friction;
        } else {
            float water_fade;

            water_fade = (definition->water_depth <= mp->water_depth) ? 1.0f :
                mp->water_depth / definition->water_depth;

            if (0.0f < mp_def->density && 0.0f < definition->water_depth) {
                float buoyancy = (mp_def->mass / mp_def->density) * definition->water_density *
                    water_fade * gravity_scale;
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

            if (powered_def != 0) {
                if ((powered_def->flags & 0x08) != 0 && powered_state->water_lift != 0.0f) {
                    float lift = (float)fabs((double)(mp->forward_k * mp->velocity_k +
                        mp->forward_j * mp->velocity_j + mp->velocity_i * mp->forward_i)) *
                        powered_state->water_lift * definition->mass * water_fade;
                    mp->powered_force_i += lift * mp->up_i;
                    mp->powered_force_j += lift * mp->up_j;
                    mp->powered_force_k += lift * mp->up_k;
                }
                goto object_water_air_friction_common;
            }
        }
        goto plain_air_friction;

    powered_air_friction:
        if (powered_def != 0 && (powered_def->flags & 0x04) != 0 && powered_state->air_friction != 0.0f) {
            float neg = -powered_state->air_friction;
            float t1 = neg * mp->forward_j + mp->velocity_j;
            float t2 = neg * mp->forward_k + mp->velocity_k;
            float d = -(mp_def->mass * definition->air_friction);
            mp->air_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
            mp->air_friction_force[1] = t1 * d;
            mp->air_friction_force[2] = t2 * d;
            goto after_air_friction;
        }
        goto plain_air_friction;

    object_water_air_friction_common:
        if (powered_def != 0 && (powered_def->flags & 0x04) != 0 && powered_state->air_friction != 0.0f) {
            float neg = -powered_state->air_friction;
            float t1 = neg * mp->forward_j + mp->velocity_j;
            float t2 = neg * mp->forward_k + mp->velocity_k;
            float d = -(mp_def->mass * definition->air_friction);
            mp->air_friction_force[0] = d * (neg * mp->forward_i + mp->velocity_i);
            mp->air_friction_force[1] = t1 * d;
            mp->air_friction_force[2] = t2 * d;
            goto after_air_friction;
        }

    plain_air_friction:
        {
            float d = -(mp_def->mass * definition->air_friction);
            mp->air_friction_force[0] = d * mp->velocity_i;
            mp->air_friction_force[1] = d * mp->velocity_j;
            mp->air_friction_force[2] = d * mp->velocity_k;
        }

    after_air_friction:
        halo::physics::object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
            mp_def->friction_perpendicular_scale, mp->air_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);

        if (powered_def != 0 && (powered_def->flags & 0x10) != 0 && powered_state->air_lift != 0.0f) {
            float lift = (float)fabs((double)(mp->forward_k * mp->velocity_k + mp->forward_j * mp->velocity_j +
                mp->forward_i * mp->velocity_i)) * definition->mass * powered_state->air_lift;
            mp->powered_force_i += lift * mp->up_i;
            mp->powered_force_j += lift * mp->up_j;
            mp->powered_force_k += lift * mp->up_k;
        }

        if (mp->velocity_i * mp->velocity_i + mp->velocity_j * mp->velocity_j +
            mp->velocity_k * mp->velocity_k < 0.0011111111f) {
            mp->flags |= _mass_point_at_rest_bit;
        } else {
            mp->flags &= ~(uint32_t)_mass_point_at_rest_bit;
        }
        mp->flags = (mp->ground_depth <= 0.0f) ? (mp->flags & ~(uint32_t)_mass_point_ground_contact_bit) :
            (mp->flags | _mass_point_ground_contact_bit);
        mp->flags = (mp->water_depth <= 0.0f) ? (mp->flags & ~(uint32_t)_mass_point_water_contact_bit) :
            (mp->flags | _mass_point_water_contact_bit);

        if (powered_def != 0) {
            if ((powered_def->flags & 0x20) != 0) {
                float thrust = powered_state->thrust * definition->mass;
                mp->powered_force_i += thrust * mp->forward_i;
                mp->powered_force_j += thrust * mp->forward_j;
                mp->powered_force_k += thrust * mp->forward_k;
            }
            if ((powered_def->flags & 0x40) != 0) {
                real_vector3d delta;
                collision_result result;
                float probe_length = powered_def->antigrav_height + mp_def->radius;

                delta.i = probe_length * global_down3d_pointer->i;
                delta.j = probe_length * global_down3d_pointer->j;
                delta.k = probe_length * global_down3d_pointer->k;

                if (halo::physics::collision_test_movement_segment(0xc0a0, (real_point3d *)&mp->position_x, &delta,
                        context->object_index, &result)) {
                    float clearance = probe_length * result.t - mp_def->radius;
                    float lean = halo::math::real_inverse_lerp_clamped(mp->up_k, powered_def->antigrav_normal_k0,
                        powered_def->antigrav_normal_k1);
                    float fade = (clearance <= 0.0f) ? 1.0f : 1.0f - clearance / powered_def->antigrav_height;
                    float dot_nv = result.plane.normal.j * mp->velocity_j + result.plane.normal.k * mp->velocity_k +
                        result.plane.normal.i * mp->velocity_i;
                    float push = (fade * fade * k_physics_gravity - dot_nv * powered_def->antigrav_damp_fraction) *
                        powered_state->antigrav * powered_def->antigrav_strength * definition->mass * lean;

                    mp->powered_force_i += result.plane.normal.i * push;
                    mp->powered_force_j += result.plane.normal.j * push;
                    mp->powered_force_k += result.plane.normal.k * push;
                    mp->flags |= _mass_point_antigrav_bit;
                }
            }
        }

        mp->total_force_i = mp->ground_normal_force_i + mp->ground_friction_force[0] +
            mp->buoyancy_force_i + mp->water_friction_force[0] + mp->air_friction_force[0] +
            mp->powered_force_i;
        mp->total_force_j = mp->ground_normal_force_j + mp->ground_friction_force[1] +
            mp->buoyancy_force_j + mp->water_friction_force[1] + mp->air_friction_force[1] +
            mp->powered_force_j;
        mp->total_force_k = mp->ground_normal_force_k + mp->ground_friction_force[2] +
            mp->buoyancy_force_k + mp->water_friction_force[2] + mp->air_friction_force[2] +
            mp->powered_force_k;

        mp->torque_i = mp->total_force_k * mp->offset_y - mp->offset_z * mp->total_force_j;
        mp->torque_j = mp->total_force_i * mp->offset_z - mp->total_force_k * mp->offset_x;
        mp->torque_k = mp->total_force_j * mp->offset_x - mp->total_force_i * mp->offset_y;

        out_force->i += mp->total_force_i;
        out_force->j += mp->total_force_j;
        out_force->k += mp->total_force_k;
        out_torque->i += mp->torque_i;
        out_torque->j += mp->torque_j;
        out_torque->k += mp->torque_k;
    }
}

}

namespace halo::physics {

/**
 * Builds an object_physics_context for object_index. Fails (returns 0) when the Object tag has
 * no physics reference (tag offset 0x8c == -1); otherwise fills object_index, definition (the
 * Physics tag data), scale (forced to 1.0), the (forward, left, up) orientation basis (left via
 * cross product), and a translation built from the object's position and the Physics
 * definition's centre of mass (see UNSURE above), returning 1.
 *
 * @address 0x5074b0
 */
uint8_t ObjectPhysics::context_build(uint32_t object_index, object_physics_context *out_context)
{
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    void *object_tag_data = halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;
    int32_t physics_tag_id = *(int32_t *)((uint8_t *)object_tag_data + 0x8c);
    void *physics_definition;

    if (physics_tag_id == -1) {
        return 0;
    }

    out_context->object_index = object_index;
    physics_definition = halo::cache::globals().tag_instances[(uint16_t)physics_tag_id].data;
    out_context->definition = physics_definition;
    out_context->scale = 1.0f;

    halo::objects::object_get_position((real_point3d *)&out_context->position_x, object_index);
    halo::objects::object_get_orientation((real_vector3d *)&out_context->forward_i, object_index, (real_vector3d *)&out_context->up_i);
    halo::math::vector3d_cross_product(*((real_vector3d *)&out_context->left_i), *((const real_vector3d *)&out_context->forward_i),
                           *((const real_vector3d *)&out_context->up_i));
    {
        real_point3d point;
        point.x = -*(float *)((uint8_t *)physics_definition + 0x0c);
        point.y = -*(float *)((uint8_t *)physics_definition + 0x10);
        point.z = -*(float *)((uint8_t *)physics_definition + 0x14);
        halo::math::matrix4x3_transform_point(point, point, *((real_matrix4x3 *)&out_context->scale));
        out_context->position_x = point.x;
        out_context->position_y = point.y;
        out_context->position_z = point.z;
    }

    return 1;
}

}

namespace halo::physics {

/**
 * object_index's collision-model bounding sphere and, for each: if it is a non-frozen biped,
 * runs object_physics_check_impact_damage (impact damage); if it is a vehicle other than object_index itself and both
 * objects build valid object_physics_contexts, runs object_physics_resolve_mass_point_overlap (vertex-pair repulsion) -- but
 * only once per pair, when candidate_index < object_index, or when the candidate is at rest, or
 * when local_2074's UNRESOLVED condition holds (see file header).
 * REWRITTEN from objdump. Every biped (type 0) or, when the object has a collision context, vehicle (type 1) near the
 * object is handled. A biped without the +0x106 bit 2 gets object_physics_check_impact_damage(&self collision
 * context, biped). Another vehicle whose physics context builds gets
 *
 * @address 0x508a10
 */
void ObjectPhysics::handle_nearby_object_impacts(uint32_t object_index)
{
    object_collision_context self_collision_context;
    object_physics_context self_physics_context;
    uint8_t has_collision_context;
    datum_index candidates[0x800];
    int16_t count;
    int16_t i;
    uint32_t self_slot = object_index & halo::k_slot_mask;

    has_collision_context = halo::physics::object_collision_context_build(object_index, &self_collision_context);
    if (!halo::physics::object_physics_context_build(object_index, &self_physics_context)) {
        return;
    }

    {
        object *self = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;

        count = (int16_t)halo::objects::object_find_in_sphere(1, (uint32_t)(has_collision_context != 0) + 2, &self->location_leaf_index,
            &self->bounding_center, self->bounding_radius, candidates, 0x800);
    }

    for (i = 0; i < count; i++) {
        uint32_t candidate_index = candidates[i];
        object_header *header = &((object_header *)halo::objects::globals().object_data->data)[candidate_index & halo::k_slot_mask];

        if (header->type == 0) {
            if ((*(uint16_t *)((uint8_t *)header->data + 0x106) & 4) == 0) {
                halo::physics::object_physics_check_impact_damage((uint32_t *)&self_collision_context, candidate_index);
            }
        } else if (header->type == 1 && candidate_index != object_index) {
            object_physics_context candidate_context;

            if (halo::physics::object_physics_context_build(candidate_index, &candidate_context)) {
                if ((candidate_index & halo::k_slot_mask) < self_slot ||
                    (header->data->flags & _object_at_rest_bit) != 0 ||
                    *(float *)candidate_context.definition > 0.0f) {
                    halo::physics::object_physics_resolve_mass_point_overlap(&self_physics_context, &candidate_context);
                }
            }
        }
    }
}

}

extern "C" { extern uint8_t physics_disable_integration; }
namespace halo::physics {

/**
 * Integrates one tick of linear and angular momentum for context's object from torque_and_force
 * (see UNSURE above), then resolves the resulting movement in up to k_physics_integration_substeps
 * sub-steps: each sub-step re-tests every mass point's movement segment against the world
 * (collision_test_movement_segment), and on the closest hit applies a friction/bounce correction
 * to velocity before repeating, or commits the position/orientation directly once a sub-step finds
 * no hit at all. Finally tallies each mass point's ground/water contact flags (written by
 * object_physics_compute_mass_point_forces into mass_point_states) and updates the object's own
 * at-rest, ground-contact and water-contact flags from the tallies and from how much the tick
 *
 * Original register convention: stack -> context, mass_point_states, torque; ECX -> force.
 *
 * @address 0x5097e0
 */
void ObjectPhysics::integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states, real_vector3d *torque, real_vector3d *force)
{
    object *self = ((object_header *)halo::objects::globals().object_data->data)[context->object_index & halo::k_slot_mask].data;
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

        halo::math::matrix3x3_from_forward_up(self->up, self->forward, orientation);
        halo::math::matrix3x3_multiply(&step1, &orientation, inverse_inertia_local);
        halo::math::matrix3x3_transpose(&step1_transposed, &step1);
        halo::math::matrix3x3_multiply(&world_inverse_inertia, &step1_transposed, &orientation);
        halo::math::matrix3x3_inverse_transform_vector(&delta_angular_velocity, torque, world_inverse_inertia);
    }
    new_angular_velocity.i = delta_angular_velocity.i + self->angular_velocity.i;
    new_angular_velocity.j = delta_angular_velocity.j + self->angular_velocity.j;
    new_angular_velocity.k = delta_angular_velocity.k + self->angular_velocity.k;

    {
        real_vector3d new_forward;
        real_vector3d new_up;
        real_point3d commit_position;

        halo::physics::object_physics_mass_point_update_orientation(&new_angular_velocity, &new_up, &new_forward,
            &self->forward, &self->up);

        self->velocity = new_velocity;
        self->angular_velocity = new_angular_velocity;
        commit_position.x = new_position.i;
        commit_position.y = new_position.j;
        commit_position.z = new_position.k;

        if (physics_disable_integration != 0) {
            halo::objects::object_set_position_and_orientation(context->object_index, &new_forward, &new_up, &commit_position);
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

                halo::math::matrix4x3_from_forward_up(new_up, new_forward, step_matrix);
                step_matrix.position = commit_position;

                center_of_mass_local.x = -definition->center_of_mass.x;
                center_of_mass_local.y = -definition->center_of_mass.y;
                center_of_mass_local.z = -definition->center_of_mass.z;
                halo::math::matrix4x3_transform_point(center_of_mass_world, center_of_mass_local, step_matrix);
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

                    if (halo::physics::collision_test_movement_segment(0xc0a1, (real_point3d *)&point_state->position_x, &delta,
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
                    halo::objects::object_set_position_and_orientation(context->object_index, &new_forward, &new_up, &commit_position);
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

                    halo::physics::object_physics_mass_point_update_orientation(&new_angular_velocity, &new_up, &new_forward,
                        &self->forward, &self->up);
                }
            }

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

        self->flags = (ground_contact_count >= 1) ? (self->flags | 0x02u) : (self->flags & ~0x02u);
        self->flags = (water_contact_count >= 1) ? (self->flags | 0x04u) : (self->flags & ~0x04u);
        self->flags = (water_contact_count >= 1) ? (self->flags | 0x08u) : (self->flags & ~0x08u);

        if (water_contact_count != count) {
            self->flags &= ~0x10u;
            return;
        }
        self->flags |= 0x10u;
    }
}

}

extern "C" { extern float k_default_resting_plane[4]; }
namespace halo::physics {

/**
 * Seeds mass_point's resting plane to k_default_resting_plane and computes its ground_depth
 * against that default plane, then runs a sphere query (physics_model_build_from_sphere_query,
 * flags 0xc0a0: structure BSP + nearby objects) around mass_point->position at definition->radius.
 * If that finds anything and a point test against the resulting model (physics_shape_test_point) also hits,
 * overwrites resting_plane/ground_depth/material_type from the contact, updates
 * _mass_point_on_ground_surface_bit (see UNSURE above), and depletes the hit object's shield
 * when the contact was against an object rather than the world.
 *
 * @address 0x507ac0
 */
void ObjectPhysics::mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition)
{
    physics_model model;
    physics_model_contact contact;

    mass_point->resting_plane_i = k_default_resting_plane[0];
    mass_point->resting_plane_j = k_default_resting_plane[1];
    mass_point->resting_plane_k = k_default_resting_plane[2];
    mass_point->resting_plane_d = k_default_resting_plane[3];
    mass_point->material_type = -1;

    mass_point->ground_depth = definition->radius -
        ((mass_point->resting_plane_i * mass_point->position_x +
          mass_point->resting_plane_j * mass_point->position_y +
          mass_point->resting_plane_k * mass_point->position_z) - mass_point->resting_plane_d);

    if (halo::physics::physics_model_build_from_sphere_query(0xc0a0, (real_point3d *)&mass_point->position_x,
            definition->radius, 0.0f, definition->radius, exclude_object_index, &model)) {
        if (halo::physics::physics_shape_test_point(&model, (real_point3d *)&mass_point->position_x, &contact)) {
            uint8_t is_scenery;

            mass_point->resting_plane_i = contact.plane_i;
            mass_point->resting_plane_j = contact.plane_j;
            mass_point->resting_plane_k = contact.plane_k;
            mass_point->ground_depth = contact.t;
            mass_point->resting_plane_d = contact.plane_d;
            mass_point->material_type =
                halo::physics::physics_resolve_material_type(contact.object_index, contact.material_type);

            is_scenery = contact.object_index != halo::k_dword_none &&
                (1u << (((object_header *)halo::objects::globals().object_data->data)[contact.object_index & halo::k_slot_mask].type &
                        0x1f) & 0x40) != 0;

            if ((contact.surface_flags & 8) == 0 &&
                (contact.object_index == halo::k_dword_none || is_scenery)) {
                mass_point->flags &= ~(uint32_t)_mass_point_on_ground_surface_bit;
            } else {
                mass_point->flags |= _mass_point_on_ground_surface_bit;
            }

            if (contact.object_index != halo::k_dword_none) {
                halo::objects::object_set_shield_depleted_flag(contact.object_index);
            }
        }
    }
}

}

extern "C" { extern double sin(double x); }
extern "C" { extern double cos(double x); }
namespace halo::physics {

/**
 * VERIFIED (logic) against disassembly 0x5096f0..0x5097d2 (2026-09-30): register/stack roles (EAX axis, ESI up, EDI forward,
 * stack fallback_forward/fallback_up), the fcos/fsin call order into matrix4x3_from_axis_angle, both transforms, the
 * forward renormalise, the Gram-Schmidt step and the zero-length fallback copy match. STILL-UNSURE: the original feeds
 * fsin/fcos the UNROUNDED extended-precision length returned in st(0); the C rounds it to float first, which only
 * matters for large random axes (angle > ~1e3 rad), as the difftest uses.
 * Rotates forward and up in place by the small rotation axis gives this tick (treating the
 * axis's length as the rotation angle in radians), then re-orthonormalizes up against forward
 * (Gram-Schmidt: subtract up's projection onto forward, renormalize) to correct drift.
 *
 * @address 0x5096f0
 */
void ObjectPhysics::mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward, real_vector3d *fallback_forward, real_vector3d *fallback_up)
{
    real_vector3d local_axis = *axis;
    real length = halo::math::vector3d_normalize_with_length(local_axis);

    if (length != 0.0f) {
        real_matrix4x3 rotation;
        halo::math::matrix4x3_from_axis_angle(rotation, local_axis, (real)sin((double)length), (real)cos((double)length));
        halo::math::matrix4x3_transform_vector(*forward, *fallback_forward, rotation);
        halo::math::matrix4x3_transform_vector(*up, *fallback_up, rotation);
        halo::math::vector3d_normalize_with_length(*forward);

        real neg_dot = -(forward->k * up->k + up->j * forward->j + forward->i * up->i);
        up->i = neg_dot * forward->i + up->i;
        up->j = neg_dot * forward->j + up->j;
        up->k = neg_dot * forward->k + up->k;
        halo::math::vector3d_normalize_with_length(*up);
        return;
    }

    *forward = *fallback_forward;
    *up = *fallback_up;
}

}

extern "C" { extern float k_physics_collision_damping; }
namespace halo::physics {

/**
 * For every pair of mass-point spheres between self and other (self's mass points transformed
 * by self's own matrix, other's by other's), tests whether the spheres overlap and, if so,
 * applies an equal-and-opposite spring repulsion impulse (scaled by penetration depth and the
 * geometric mean of the two masses) plus the matching torque about each object's centre, summed
 * across every overlapping pair before being added once to each object's accumulated
 * force/torque. Returns whether any pair overlapped.
 *
 * @address 0x5090c0
 */
uint8_t ObjectPhysics::resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other)
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

            halo::math::matrix4x3_transform_point(self_world_position, *((real_point3d *)&self_mass_points[i].position),
                *((real_matrix4x3 *)&self->scale));

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

                delta_x = (((local_z * other->up_i + local_y * other->left_i) + local_x * other->forward_i) + other->position_x)
                    - self_world_position.x;
                delta_y = (((local_z * other->up_j + local_y * other->left_j) + local_x * other->forward_j) + other->position_y)
                    - self_world_position.y;
                delta_z = (((local_z * other->up_k + local_y * other->left_k) + local_x * other->forward_k) + other->position_z)
                    - self_world_position.z;

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
                    impulse += impulse;

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
                        object *self_object = ((object_header *)halo::objects::globals().object_data->data)[self->object_index & halo::k_slot_mask].data;
                        object *other_object = ((object_header *)halo::objects::globals().object_data->data)[other->object_index & halo::k_slot_mask].data;

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
        object *self_object = ((object_header *)halo::objects::globals().object_data->data)[self->object_index & halo::k_slot_mask].data;
        object *other_object = ((object_header *)halo::objects::globals().object_data->data)[other->object_index & halo::k_slot_mask].data;

        if (self_object->network_role != 1 || halo::units::unit_any_flagged_seat_occupied(self->object_index) == 1) {
            vehicle_data *self_vehicle = (vehicle_data *)((uint8_t *)self_object + k_unit_object_size);
            self_vehicle->accumulated_force.i += self_force.i;
            self_vehicle->accumulated_force.j += self_force.j;
            self_vehicle->accumulated_force.k += self_force.k;
            self_vehicle->accumulated_torque.i += self_torque.i;
            self_vehicle->accumulated_torque.j += self_torque.j;
            self_vehicle->accumulated_torque.k += self_torque.k;
            self_object->flags &= ~_object_at_rest_bit;
            self_vehicle->collision_update_pending = 1;
        }

        if (other_definition->radius <= 0.0f &&
            (other_object->network_role != 1 || halo::units::unit_any_flagged_seat_occupied(other->object_index) == 1)) {
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

}

namespace halo::physics {

/**
 * Transforms world_point into context's object-local space and tests it against every mass
 * point's collision sphere (PhysicsMassPoint.position/.radius). Returns 1 and (best-effort)
 * *out_index the first mass point whose sphere contains the local point; 0 if none do.
 *
 * @address 0x507590
 */
uint8_t ObjectPhysics::test_point_against_mass_points(object_physics_context *context, real_point3d *world_point, int16_t *out_index)
{
    Physics *definition = (Physics *)context->definition;
    real_point3d local_point;
    int32_t count = definition->mass_points.count;
    int16_t i;

    halo::math::matrix4x3_inverse_transform_point(*((real_matrix4x3 *)&context->scale), local_point, *world_point);

    for (i = 0; i < count; i++) {
        PhysicsMassPoint *mass_point =
            &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        float dx = mass_point->position.x - local_point.x;
        float dy = mass_point->position.y - local_point.y;
        float dz = mass_point->position.z - local_point.z;
        float radius = mass_point->radius;

        if (dx * dx + dy * dy + dz * dz <= radius * radius) {
            if (out_index) {
                *out_index = i;
            }
            return 1;
        }
    }

    return 0;
}

}

namespace halo::physics {

/**
 * Transforms the world-space ray (world_origin, world_direction) into context's object-local
 * space and tests it against every mass point's collision sphere, keeping the closest hit.
 * Writes the winning fraction, world-space contact normal and plane d into *out_result (t is
 * seeded to FLT_MAX so a caller can tell "no hit" from result->t remaining unchanged) and
 * returns whether anything was hit.
 * matrix4x3_inverse_transform_vector and ray_intersects_sphere now follow those functions' definitions (the
 * draft passed them in a different order).
 *
 * Original register convention: EAX -> world_origin, EBX -> out_result, stack -> context, world_direction.
 *
 * @address 0x507610
 */
uint8_t ObjectPhysics::test_ray_against_mass_points(real_point3d *world_origin, real_vector3d *world_direction, object_physics_ray_result *out_result, object_physics_context *context)
{
    Physics *definition = (Physics *)context->definition;
    real_point3d local_origin;
    real_vector3d local_direction;
    int32_t count = definition->mass_points.count;
    int16_t i;
    uint8_t hit = 0;

    out_result->t = 3.4028235e+38f;

    halo::math::matrix4x3_inverse_transform_point(*((real_matrix4x3 *)&context->scale), local_origin, *world_origin);
    halo::math::matrix4x3_inverse_transform_vector(local_direction, *world_direction, *((real_matrix4x3 *)&context->scale));

    for (i = 0; i < count; i++) {
        PhysicsMassPoint *mass_point =
            &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        real_vector3d normal;
        float t;

        if (halo::math::ray_intersects_sphere(local_origin, &normal, local_direction, t,
                *((real_point3d *)&mass_point->position), mass_point->radius) && t < out_result->t) {
            out_result->t = t;
            out_result->plane_i = normal.i;
            out_result->plane_j = normal.j;
            out_result->plane_k = normal.k;
            hit = 1;
            out_result->plane_d =
                (local_direction.i * t + local_origin.x) * out_result->plane_i +
                (local_direction.j * t + local_origin.y) * out_result->plane_j +
                (local_direction.k * t + local_origin.z) * out_result->plane_k;
        }
    }

    if (hit) {
        float ni = out_result->plane_i;
        float nj = out_result->plane_j;
        float nk = out_result->plane_k;

        out_result->plane_i = ni * context->forward_i + nj * context->left_i + nk * context->up_i;
        out_result->plane_j = ni * context->forward_j + nj * context->left_j + nk * context->up_j;
        out_result->plane_k = ni * context->forward_k + nj * context->left_k + nk * context->up_k;
        out_result->plane_d = context->position_x * out_result->plane_i +
            out_result->plane_d * context->scale + context->position_y * out_result->plane_j +
            out_result->plane_k * context->position_z;
    }

    return hit;
}

}

extern "C" { extern void object_physics_tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t mass_points, real_vector3d *extra_force, real_vector3d *extra_torque); }
namespace halo::physics {

/**
 * REWRITTEN from objdump 0x507840..0x507a36. Stack: (object, powered states, mass point states, extra force, extra
 * torque). A Physics tag with a radius takes the single-pass path (0x509e80). Otherwise: build the context; when
 * powered states are given, each one's matrix (+0x2c) is rebuilt from its quaternion (+0x1c) and transposed;
 * mass-point forces (0x507cc0) plus the object's accumulated force / torque (+0x508 / +0x514, then cleared)
 * plus the extras are integrated (0x5097e0: force in ECX, torque on the stack) and nearby impacts handled.
 * The draft rebuilt the matrices from a NULL quaternion into the Physics tag's own block.
 *
 * @address 0x507840
 */
void ObjectPhysics::tick(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t mass_points, real_vector3d *extra_force, real_vector3d *extra_torque)
{
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    Object *object_tag = (Object *)halo::cache::globals().tag_instances[((object *)obj)->definition_tag & halo::k_slot_mask].data;
    Physics *physics = (Physics *)halo::cache::globals().tag_instances[*(datum_index *)&object_tag->physics.tag_id & halo::k_slot_mask].data;
    object_physics_context context;
    real_vector3d torque;
    real_vector3d force;

    if (physics->radius > 0.0f) {
        halo::physics::object_physics_tick_single_pass(object_index, powered_states, (mass_point_state *)mass_points, extra_force, extra_torque);
        return;
    }
    halo::physics::object_physics_context_build(object_index, &context);
    if (powered_states != 0 && (int32_t)physics->powered_mass_points.count > 0) {
        int16_t i;

        for (i = 0; (int32_t)i < (int32_t)physics->powered_mass_points.count; i++) {
            uint8_t *state = (uint8_t *)powered_states + i * 0x60;
            float *m = (float *)(state + 0x2c);
            float t;

            halo::math::matrix4x3_from_quaternion(*(real_quaternion *)(state + 0x1c), *(real_matrix4x3 *)m);
            t = m[2]; m[2] = m[4]; m[4] = t;
            t = m[3]; m[3] = m[7]; m[7] = t;
            t = m[6]; m[6] = m[8]; m[8] = t;
        }
    }
    halo::physics::object_physics_compute_mass_point_forces(&context, powered_states, mass_points, &force, &torque);
    obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    force.i += *(float *)(obj + 0x508);
    force.j += *(float *)(obj + 0x50c);
    force.k += *(float *)(obj + 0x510);
    torque.i += *(float *)(obj + 0x514);
    torque.j += *(float *)(obj + 0x518);
    torque.k += *(float *)(obj + 0x51c);
    memset(obj + 0x508, 0, 0x18);
    if (extra_force != 0) {
        force.i += extra_force->i;
        force.j += extra_force->j;
        force.k += extra_force->k;
    }
    if (extra_torque != 0) {
        torque.i += extra_torque->i;
        torque.j += extra_torque->j;
        torque.k += extra_torque->k;
    }
    halo::physics::object_physics_integrate_and_test_at_rest(&context, (mass_point_state *)mass_points, &torque, &force);
    halo::physics::object_physics_handle_nearby_object_impacts(object_index);
}

}
