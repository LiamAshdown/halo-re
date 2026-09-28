// object_physics_compute_mass_point_forces  (Ghidra: antenna_object_compute_vertex_forces;
//   renamed per out/phase4/physics_types_notes.md section 5, which gives this exact new name and
//   explains the phase2 antenna_* name was wrong: every field this function touches belongs to
//   the object Physics tag's mass_points block, not the antenna widget)
// address 0x507cc0, size 3403 bytes -- the largest function in this batch.
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 against objdump 0x507cc0..0x508a0a (full decode). Fixed: torque.j had the wrong sign
//   (every vehicle's per-mass-point torque about world y was inverted); the ground normal force scales by
//   Physics.mass, not the mass point's mass; the material friction scale applies when Physics.mass <= 7500
//   (the draft inverted it); the leaf lookup uses the structure collision BSP 0x746f90 (the draft passed
//   0x746f98). Sums follow the original k, j, i orders.
// evidence: types/physics.h mass_point_state's full word-by-word field table (source: this
//   function, out/phase4/physics_types_notes.md section 2) is used directly below in place of
//   every puVar12[N] offset; types/tags.h Physics/PhysicsMassPoint/PhysicsPoweredMassPoint field
//   names (ground_friction, ground_depth, ground_damp_fraction, ground_normal_k1/k0,
//   water_friction, water_depth, water_density, air_friction, mass_points, powered_mass_points;
//   relative_mass, mass, relative_density, density, position, forward, up, friction_type,
//   friction_parallel_scale, friction_perpendicular_scale, radius; antigrav_strength/offset/
//   height/damp_fraction/normal_k1/normal_k0) resolve nearly every raw offset here by name;
//   types/objects.h object.position/velocity/angular_velocity (0x5c/0x68/0x8c); types/tags.h
//   GlobalsMaterial (ground_friction_scale +0x94 and its four siblings) and Globals.materials.
// register convention: none recognized as in_EAX etc; all five are Ghidra's own ordinary
//   parameters (`antenna_object_compute_vertex_forces(uint *param_1, int param_2, undefined4
//   *param_3, float *param_4, float *param_5)`), confirmed as (context, powered_states,
//   out_mass_points, out_force, out_torque) by object_physics_tick's own call site.
// UNSURE (major): the float10 (x87 80-bit extended) arithmetic throughout the antigrav-adjusted
//   friction and antigrav-lift blocks is narrowed to plain float here; this changes rounding
//   only, not the formula. UNSURE: pfVar11 (Ghidra's "high 32 bits of FUN_005013a0's 64-bit
//   return") is really just &mass_point->position, a leftover register value unrelated to that
//   call's actual (32-bit) result; this rewrite drops the bogus 64-bit split. UNSURE:
//   matrix4x3_multiply is invoked indirectly through the function-pointer global
//   PTR_matrix4x3_multiply_00696664 rather than called directly; this rewrite calls it directly,
//   which is observationally identical unless something else in the game retargets that pointer.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"
#include "physics.h"
#include <string.h>

extern double fabs(double x); // ABS is a single x87 FABS instruction

extern data_array *object_data;                     // 0x008603b0
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, the structure collision BSP (0x507f32 ECX)
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern Globals *game_globals;                        // 0x00746fa0
extern real_vector3d *global_reference_vector_0069672c; // 0x0069672c
extern float k_physics_gravity;                      // 0x0069c52c
extern uint8_t material_table_warning_issued;         // 0x00721e4c
extern int32_t material_table_bad_index;              // 0x006e3578
extern uint8_t material_table_fallback[0x374];        // 0x006e3208, zeroed fallback
                                                       // GlobalsMaterial-shaped record, UNSURE name

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, this module (lower half)
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
    real_matrix4x3 *m); // 0x4cbde0, math module (src/math/matrix4x3_transform_point.c)
    // blam-cc: only m is on the stack; out and point arrive in registers Ghidra loses at every
    //          call site in this module
    // module; UNSURE args -- called with only the matrix visible here, as elsewhere in this batch
extern void matrix4x3_multiply(void *a, void *b, real_matrix4x3 *out); // 0x4cc0d0, math module
extern void object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index,
    mass_point_state *mass_point, PhysicsMassPoint *definition); // 0x507ac0, this module
extern float scenario_location_water_surface_distance(bsp_leaf_reference *location, real_point3d *point); // 0x53ee00, EAX location, EDI point
                                  // point; UNSURE args
extern float real_inverse_lerp_clamped(float value, float ref_k0, float ref_k1); // 0x507430, misattributed
                                  // math helper, not rewritten in this batch
extern void object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale,
    float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up); // 0x507c00, stack, EDI friction, ECX forward, EDX up
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index,
    collision_result *result); // 0x505880, this module (higher half)

// Computes gravity plus every per-mass-point force (ground/water/air friction, buoyancy, powered
// thrust/lift/antigrav) for context's object, filling out_mass_points (zeroed first, one
// mass_point_state per Physics.mass_points entry) and summing the per-mass-point total_force and
// torque into *out_force / *out_torque (seeded with straight gravity beforehand). powered_states
// is the runtime powered_mass_point_state array built by object_physics_tick's quaternion loop,
// or NULL when the object has no live powered-mass-point state.
void object_physics_compute_mass_point_forces(object_physics_context *context,
    powered_mass_point_state *powered_states, uint32_t param_3,
    real_vector3d *out_force, real_vector3d *out_torque)
{
    object *obj = ((object_header *)object_data->data)[context->object_index & 0xffff].data;
    Physics *definition = (Physics *)context->definition;
    float gravity_scale = k_physics_gravity * definition->gravity_scale;
    mass_point_state *mass_points = (mass_point_state *)param_3;
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
        matrix4x3_transform_point((real_point3d *)&mp->position_x, (real_point3d *)&mp_def->position,
            (real_matrix4x3 *)&context->scale); // UNSURE: out and point are register arguments

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
            // 0x507dcc..0x507dd8: combined (a full real_matrix4x3 at ebp-0x94) = context matrix * the powered state's
            // matrix, through matrix4x3_multiply_procedure. FIXED 2026-09-28: the draft used float[9] (36 bytes) for the
            // 52-byte output, overflowing the stack (the vehicle-physics crash), and read it one float early
            // (combined[0] is the scale).
            real_matrix4x3 combined;
            matrix4x3_multiply(&context->scale, &powered_state->matrix_scale, &combined);
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

        mp->leaf_index = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&mp->position_x);
        mp->cluster_index = (mp->leaf_index == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[mp->leaf_index].cluster;

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

        object_physics_mass_point_resolve_ground_contact(context->object_index, mp, mp_def);
        mp->water_depth = scenario_location_water_surface_distance((bsp_leaf_reference *)((uint8_t *)mp + 0x34),
            (real_point3d *)&mp->position_x); // EAX mass point +0x34, EDI +0x04

        if (0.0f < mp->ground_depth) {
            GlobalsMaterial *material;
            int16_t material_index = mp->material_type;
            float ground_friction, ground_normal_k1, ground_normal_k0, ground_depth_scale,
                ground_damp_fraction_scale;
            float tangential_speed;

            if (material_index < 0 || (int32_t)material_index >= game_globals->materials.count) {
                if (!material_table_warning_issued) {
                    material_table_bad_index = -1;
                    material_table_warning_issued = 1;
                }
                material = (GlobalsMaterial *)material_table_fallback;
            } else {
                material = &((GlobalsMaterial *)game_globals->materials.pointer)[material_index];
            }

            // 0x50808f / 0x50809f: the material scale applies only to objects of at most 7500 mass (the draft
            // had the mass test inverted)
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
            // 0x50816f: the OBJECT's mass (Physics +0x08), not the mass point's
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
                    float lean = real_inverse_lerp_clamped(mp->resting_plane_k, ground_normal_k0, ground_normal_k1);
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

            object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
                mp_def->friction_perpendicular_scale, mp->ground_friction_force,
                (real_vector3d *)&mp->forward_i, (real_vector3d *)&mp->up_i);
        }

        if (mp->water_depth <= 0.0f) {
            goto powered_air_friction; // no buoyancy/water friction; see below
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

            object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
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
        object_physics_blend_friction_axes(mp_def->friction_type, mp_def->friction_parallel_scale,
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

                delta.i = probe_length * global_reference_vector_0069672c->i;
                delta.j = probe_length * global_reference_vector_0069672c->j;
                delta.k = probe_length * global_reference_vector_0069672c->k;

                if (collision_test_movement_segment(0xc0a0, (real_point3d *)&mp->position_x, &delta,
                        context->object_index, &result)) {
                    float clearance = probe_length * result.t - mp_def->radius;
                    float lean = real_inverse_lerp_clamped(mp->up_k, powered_def->antigrav_normal_k0,
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
        mp->torque_j = mp->total_force_i * mp->offset_z - mp->total_force_k * mp->offset_x; // 0x508989 (draft had the sign flipped)
        mp->torque_k = mp->total_force_j * mp->offset_x - mp->total_force_i * mp->offset_y;

        out_force->i += mp->total_force_i;
        out_force->j += mp->total_force_j;
        out_force->k += mp->total_force_k;
        out_torque->i += mp->torque_i;
        out_torque->j += mp->torque_j;
        out_torque->k += mp->torque_k;
    }
}

#if 0
Original Ghidra decompilation (0x507cc0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void antenna_object_compute_vertex_forces
               (uint *param_1,int param_2,undefined4 *param_3,float *param_4,float *param_5)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  char cVar6;
  undefined2 uVar7;
  undefined *puVar8;
  uint uVar9;
  int iVar10;
  float *pfVar11;
  uint *puVar12;
  undefined4 *puVar13;
  int iVar14;
  float10 fVar15;
  float10 fVar16;
  float10 fVar17;
  undefined8 uVar18;
  undefined1 local_128 [20];
  float local_114;
  float local_104;
  float local_100;
  float local_fc;
  float local_cc;
  float local_c4;
  float local_c0;
  float local_b8;
  float local_b4;
  uint local_b0;
  uint local_ac;
  uint local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  undefined1 local_98 [4];
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  int local_38;
  float local_34;
  int local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_1c;
  int local_18;
  float *local_14;
  uint local_10;
  float local_c;

  local_30 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*param_1 & 0xffff) * 0xc);
  local_10 = param_1[1];
  local_28 = _DAT_0069c52c * *(float *)(local_10 + 0x1c);
  fVar2 = *(float *)(local_10 + 8);
  *param_4 = 0.0;
  param_4[1] = 0.0;
  param_4[2] = -(local_28 * fVar2);
  *param_5 = 0.0;
  param_5[1] = 0.0;
  param_5[2] = 0.0;
  puVar13 = param_3;
  for (uVar9 = (uint)(*(int *)(local_10 + 0x74) * 0x130) >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
    *puVar13 = 0;
    puVar13 = puVar13 + 1;
  }
  for (iVar10 = 0; iVar10 != 0; iVar10 = iVar10 + -1) {
    *(undefined1 *)puVar13 = 0;
    puVar13 = (undefined4 *)((int)puVar13 + 1);
  }
  iVar10 = 0;
  local_38 = 0;
  if (0 < *(int *)(local_10 + 0x74)) {
    do {
      iVar14 = iVar10 * 0x80 + *(int *)(local_10 + 0x78);
      puVar12 = param_3 + iVar10 * 0x4c;
      sVar5 = *(short *)(iVar14 + 0x20);
      if ((sVar5 == -1) || (param_2 == 0)) {
        local_18 = 0;
LAB_00507da2:
        local_14 = (float *)0x0;
      }
      else {
        local_18 = sVar5 * 0x80 + *(int *)(local_10 + 0x6c);
        if (local_18 == 0) goto LAB_00507da2;
        local_14 = (float *)(sVar5 * 0x60 + param_2);
      }
      *puVar12 = 0;
      local_1c = iVar14;
      matrix4x3_transform_point(param_1 + 2);
      if (local_14 == (float *)0x0) {
        fVar2 = *(float *)(iVar14 + 0x44);
        fVar3 = *(float *)(iVar14 + 0x48);
        fVar4 = *(float *)(iVar14 + 0x4c);
        puVar12[4] = (uint)(fVar2 * (float)param_1[3] +
                           fVar3 * (float)param_1[6] + fVar4 * (float)param_1[9]);
        puVar12[5] = (uint)(fVar2 * (float)param_1[4] +
                           fVar3 * (float)param_1[7] + fVar4 * (float)param_1[10]);
        puVar12[6] = (uint)(fVar2 * (float)param_1[5] +
                           fVar3 * (float)param_1[8] + fVar4 * (float)param_1[0xb]);
        fVar2 = *(float *)(iVar14 + 0x50);
        fVar3 = *(float *)(iVar14 + 0x54);
        fVar4 = *(float *)(iVar14 + 0x58);
        puVar12[10] = (uint)(fVar2 * (float)param_1[3] +
                            fVar3 * (float)param_1[6] + fVar4 * (float)param_1[9]);
        puVar12[0xb] = (uint)(fVar2 * (float)param_1[4] +
                             fVar3 * (float)param_1[7] + fVar4 * (float)param_1[10]);
        puVar12[0xc] = (uint)(fVar2 * (float)param_1[5] +
                             fVar3 * (float)param_1[8] + fVar4 * (float)param_1[0xb]);
      }
      else {
        (*(code *)PTR_matrix4x3_multiply_00696664)(param_1 + 2,local_14 + 0xb,local_98);
        fVar2 = *(float *)(iVar14 + 0x44);
        fVar3 = *(float *)(iVar14 + 0x48);
        fVar4 = *(float *)(iVar14 + 0x4c);
        puVar12[4] = (uint)(local_94 * fVar2 + local_88 * fVar3 + local_7c * fVar4);
        puVar12[5] = (uint)(local_90 * fVar2 + local_84 * fVar3 + local_78 * fVar4);
        puVar12[6] = (uint)(local_8c * fVar2 + local_80 * fVar3 + local_74 * fVar4);
        fVar2 = *(float *)(iVar14 + 0x50);
        fVar3 = *(float *)(iVar14 + 0x54);
        fVar4 = *(float *)(iVar14 + 0x58);
        puVar12[10] = (uint)(local_94 * fVar2 + local_88 * fVar3 + local_7c * fVar4);
        puVar12[0xb] = (uint)(local_90 * fVar2 + local_84 * fVar3 + local_78 * fVar4);
        puVar12[0xc] = (uint)(local_8c * fVar2 + local_80 * fVar3 + local_74 * fVar4);
      }
      uVar18 = FUN_005013a0();
      pfVar11 = (float *)((ulonglong)uVar18 >> 0x20);
      uVar9 = (uint)uVar18;
      puVar12[0xd] = uVar9;
      if (uVar9 == 0xffffffff) {
        uVar7 = 0xffff;
      }
      else {
        uVar7 = *(undefined2 *)(uVar9 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      *(undefined2 *)(puVar12 + 0xe) = uVar7;
      pfVar1 = (float *)(puVar12 + 0x12);
      puVar12[0xf] = (uint)(*pfVar11 - *(float *)(local_30 + 0x5c));
      puVar12[0x10] = (uint)(pfVar11[1] - *(float *)(local_30 + 0x60));
      puVar12[0x11] = (uint)(pfVar11[2] - *(float *)(local_30 + 100));
      local_50 = *(float *)(local_30 + 0x90) * (float)puVar12[0x11] -
                 (float)puVar12[0x10] * *(float *)(local_30 + 0x94);
      local_4c = (float)puVar12[0xf] * *(float *)(local_30 + 0x94) -
                 *(float *)(local_30 + 0x8c) * (float)puVar12[0x11];
      fVar2 = *(float *)(local_30 + 0x8c);
      fVar3 = *(float *)(local_30 + 0x90);
      *pfVar1 = local_50;
      puVar12[0x13] = (uint)local_4c;
      local_48 = fVar2 * (float)puVar12[0x10] - fVar3 * (float)puVar12[0xf];
      puVar12[0x14] = (uint)local_48;
      *pfVar1 = *(float *)(local_30 + 0x68) + *pfVar1;
      puVar12[0x13] = (uint)((float)puVar12[0x13] + *(float *)(local_30 + 0x6c));
      puVar12[0x14] = (uint)((float)puVar12[0x14] + *(float *)(local_30 + 0x70));
      FUN_00507ac0(*param_1,puVar12,iVar14);
      fVar15 = (float10)FUN_0053ee00();
      puVar12[0x1f] = (uint)(float)fVar15;
      if (0.0 < (float)puVar12[0x1d]) {
        sVar5 = (short)puVar12[0x1c];
        if ((sVar5 < 0) || (*(int *)(DAT_00746fa0 + 0x194) <= (int)sVar5)) {
          if (DAT_00721e4c == '\0') {
            DAT_006e3578 = 0xffffffff;
            DAT_00721e4c = '\x01';
          }
          puVar8 = &DAT_006e3208;
        }
        else {
          puVar8 = (undefined *)(sVar5 * 0x374 + *(int *)(DAT_00746fa0 + 0x198));
        }
        if ((*(float *)(puVar8 + 0x94) <= 0.0) ||
           (*(float *)(local_10 + 8) < 7500.0 == (*(float *)(local_10 + 8) == 7500.0))) {
          fVar2 = *(float *)(local_10 + 0x20);
        }
        else {
          fVar2 = *(float *)(local_10 + 0x20) * *(float *)(puVar8 + 0x94);
        }
        if (*(float *)(puVar8 + 0x98) <= 0.0) {
          local_34 = *(float *)(local_10 + 0x2c);
        }
        else {
          local_34 = *(float *)(local_10 + 0x2c) * *(float *)(puVar8 + 0x98);
        }
        if (*(float *)(puVar8 + 0x9c) <= 0.0) {
          local_2c = *(float *)(local_10 + 0x30);
        }
        else {
          local_2c = *(float *)(local_10 + 0x30) * *(float *)(puVar8 + 0x9c);
        }
        fVar3 = *(float *)(local_10 + 0x24);
        if (0.0 < *(float *)(puVar8 + 0xa0)) {
          fVar3 = fVar3 * *(float *)(puVar8 + 0xa0);
        }
        fVar4 = *(float *)(local_10 + 0x28);
        if (0.0 < *(float *)(puVar8 + 0xa4)) {
          fVar4 = fVar4 * *(float *)(puVar8 + 0xa4);
        }
        pfVar11 = (float *)(puVar12 + 0x24);
        local_20 = *pfVar1 * (float)puVar12[0x18] +
                   (float)puVar12[0x13] * (float)puVar12[0x19] +
                   (float)puVar12[0x14] * (float)puVar12[0x1a];
        fVar3 = (((float)puVar12[0x1d] / fVar3) * _DAT_0069c52c - local_20 * fVar4) *
                *(float *)(local_10 + 8);
        puVar12[0x20] = (uint)fVar3;
        puVar12[0x21] = (uint)(fVar3 * (float)puVar12[0x18]);
        puVar12[0x22] = (uint)(fVar3 * (float)puVar12[0x19]);
        puVar12[0x23] = (uint)(fVar3 * (float)puVar12[0x1a]);
        local_c = -(fVar2 * *(float *)(local_1c + 0x2c));
        fVar2 = -local_20;
        puVar12[0x15] = (uint)(fVar2 * (float)puVar12[0x18] + *pfVar1);
        puVar12[0x16] = (uint)(fVar2 * (float)puVar12[0x19] + (float)puVar12[0x13]);
        puVar12[0x17] = (uint)(fVar2 * (float)puVar12[0x1a] + (float)puVar12[0x14]);
        *pfVar11 = local_c * (float)puVar12[0x15];
        puVar12[0x25] = (uint)(local_c * (float)puVar12[0x16]);
        puVar12[0x26] = (uint)(local_c * (float)puVar12[0x17]);
        if (((local_18 != 0) && ((*(byte *)(local_18 + 0x20) & 1) != 0)) && (*local_14 != 0.0)) {
          fVar15 = (float10)FUN_00507430(puVar12[0x1a],local_2c,local_34);
          fVar17 = (float10)(float)puVar12[0x18] * (float10)(float)puVar12[10] +
                   (float10)(float)puVar12[0xb] * (float10)(float)puVar12[0x19] +
                   (float10)(float)puVar12[0xc] * (float10)(float)puVar12[0x1a];
          if ((float10)0.0 <= fVar17) {
            if ((float10)1.0 < fVar17) {
              fVar17 = (float10)1.0;
            }
          }
          else {
            fVar17 = (float10)0.0;
          }
          fVar15 = fVar17 * fVar17 * fVar15 * fVar15 * (float10)local_c;
          fVar17 = -(float10)*local_14;
          local_58 = (float)(fVar17 * (float10)(float)puVar12[5]);
          local_54 = (float)(fVar17 * (float10)(float)puVar12[6]);
          fVar16 = -(fVar17 * (float10)(float)puVar12[4] * (float10)(float)puVar12[0x18] +
                    (float10)local_58 * (float10)(float)puVar12[0x19] +
                    fVar17 * (float10)(float)puVar12[6] * (float10)(float)puVar12[0x1a]);
          local_c = (float)fVar16;
          fVar16 = fVar16 * (float10)(float)puVar12[0x18] + fVar17 * (float10)(float)puVar12[4];
          fVar17 = (float10)local_c * (float10)(float)puVar12[0x19] + (float10)local_58;
          local_cc = local_c * (float)puVar12[0x1a] + local_54;
          puVar12[0x15] = (uint)(float)(fVar16 + (float10)(float)puVar12[0x15]);
          puVar12[0x16] = (uint)(float)(fVar17 + (float10)(float)puVar12[0x16]);
          puVar12[0x17] = (uint)(local_cc + (float)puVar12[0x17]);
          *pfVar11 = (float)(fVar16 * fVar15 + (float10)*pfVar11);
          puVar12[0x25] = (uint)(float)(fVar17 * fVar15 + (float10)(float)puVar12[0x25]);
          puVar12[0x26] = (uint)(float)((float10)local_cc * fVar15 + (float10)(float)puVar12[0x26]);
        }
        FUN_00507c00((int)*(short *)(local_1c + 0x5c),*(undefined4 *)(local_1c + 0x60),
                     *(undefined4 *)(local_1c + 100));
      }
      if ((float)puVar12[0x1f] <= 0.0) {
LAB_005084d1:
        if (((local_18 == 0) || ((*(byte *)(local_18 + 0x20) & 4) == 0)) || (local_14[2] == 0.0))
        goto LAB_00508549;
        fVar2 = -local_14[2];
        local_b8 = fVar2 * (float)puVar12[5] + (float)puVar12[0x13];
        local_b4 = fVar2 * (float)puVar12[6] + (float)puVar12[0x14];
        local_c = -(*(float *)(local_1c + 0x2c) * *(float *)(local_10 + 0x48));
        puVar12[0x3a] = (uint)(local_c * (fVar2 * (float)puVar12[4] + *pfVar1));
        puVar12[0x3b] = (uint)(local_b8 * local_c);
        fVar2 = local_b4 * local_c;
      }
      else {
        if (*(float *)(local_10 + 0x3c) <= (float)puVar12[0x1f]) {
          local_24 = 1.0;
        }
        else {
          local_24 = (float)puVar12[0x1f] / *(float *)(local_10 + 0x3c);
        }
        if ((0.0 < *(float *)(local_1c + 0x34)) && (0.0 < *(float *)(local_10 + 0x3c))) {
          fVar2 = (*(float *)(local_1c + 0x2c) / *(float *)(local_1c + 0x34)) *
                  *(float *)(local_10 + 0x40) * local_24 * local_28;
          puVar12[0x2d] = (uint)fVar2;
          puVar12[0x2e] = 0;
          puVar12[0x30] = (uint)fVar2;
          puVar12[0x2f] = 0;
        }
        if (((local_18 == 0) || ((*(byte *)(local_18 + 0x20) & 2) == 0)) || (local_14[1] == 0.0)) {
          fVar2 = -(*(float *)(local_1c + 0x2c) * *(float *)(local_10 + 0x38));
          puVar12[0x31] = (uint)(fVar2 * *pfVar1);
          puVar12[0x32] = (uint)(fVar2 * (float)puVar12[0x13]);
          fVar2 = fVar2 * (float)puVar12[0x14];
        }
        else {
          fVar2 = -local_14[1];
          local_c4 = fVar2 * (float)puVar12[5] + (float)puVar12[0x13];
          local_c0 = fVar2 * (float)puVar12[6] + (float)puVar12[0x14];
          local_c = -(*(float *)(local_1c + 0x2c) * *(float *)(local_10 + 0x38));
          puVar12[0x31] = (uint)(local_c * (fVar2 * (float)puVar12[4] + *pfVar1));
          puVar12[0x32] = (uint)(local_c4 * local_c);
          fVar2 = local_c0 * local_c;
        }
        puVar12[0x33] = (uint)fVar2;
        FUN_00507c00((int)*(short *)(local_1c + 0x5c),*(undefined4 *)(local_1c + 0x60),
                     *(undefined4 *)(local_1c + 100));
        if (local_18 != 0) {
          if (((*(byte *)(local_18 + 0x20) & 8) != 0) && (local_14[3] != 0.0)) {
            fVar2 = ABS(*pfVar1 * (float)puVar12[4] +
                        (float)puVar12[5] * (float)puVar12[0x13] +
                        (float)puVar12[6] * (float)puVar12[0x14]) * local_14[3] *
                    *(float *)(local_10 + 8) * local_24;
            puVar12[0x43] = (uint)(fVar2 * (float)puVar12[10] + (float)puVar12[0x43]);
            puVar12[0x44] = (uint)(fVar2 * (float)puVar12[0xb] + (float)puVar12[0x44]);
            puVar12[0x45] = (uint)(fVar2 * (float)puVar12[0xc] + (float)puVar12[0x45]);
          }
          goto LAB_005084d1;
        }
LAB_00508549:
        fVar2 = -(*(float *)(local_1c + 0x2c) * *(float *)(local_10 + 0x48));
        puVar12[0x3a] = (uint)(fVar2 * *pfVar1);
        puVar12[0x3b] = (uint)(fVar2 * (float)puVar12[0x13]);
        fVar2 = fVar2 * (float)puVar12[0x14];
      }
      puVar12[0x3c] = (uint)fVar2;
      FUN_00507c00((int)*(short *)(local_1c + 0x5c),*(undefined4 *)(local_1c + 0x60),
                   *(undefined4 *)(local_1c + 100));
      if (((local_18 != 0) && ((*(byte *)(local_18 + 0x20) & 0x10) != 0)) && (local_14[4] != 0.0)) {
        fVar2 = ABS((float)puVar12[4] * *pfVar1 +
                    (float)puVar12[5] * (float)puVar12[0x13] +
                    (float)puVar12[6] * (float)puVar12[0x14]) * *(float *)(local_10 + 8) *
                local_14[4];
        puVar12[0x43] = (uint)(fVar2 * (float)puVar12[10] + (float)puVar12[0x43]);
        puVar12[0x44] = (uint)(fVar2 * (float)puVar12[0xb] + (float)puVar12[0x44]);
        puVar12[0x45] = (uint)(fVar2 * (float)puVar12[0xc] + (float)puVar12[0x45]);
      }
      if (0.0011111111 <=
          (float)puVar12[0x14] * (float)puVar12[0x14] +
          (float)puVar12[0x13] * (float)puVar12[0x13] + *pfVar1 * *pfVar1) {
        uVar9 = *puVar12 & 0xfffffffe;
      }
      else {
        uVar9 = *puVar12 | 1;
      }
      *puVar12 = uVar9;
      if ((float)puVar12[0x1d] <= 0.0) {
        uVar9 = *puVar12 & 0xfffffffd;
      }
      else {
        uVar9 = *puVar12 | 2;
      }
      *puVar12 = uVar9;
      if ((float)puVar12[0x1f] <= 0.0) {
        uVar9 = *puVar12 & 0xfffffff7;
      }
      else {
        uVar9 = *puVar12 | 8;
      }
      *puVar12 = uVar9;
      if (local_18 != 0) {
        if ((*(byte *)(local_18 + 0x20) & 0x20) != 0) {
          fVar2 = local_14[5] * *(float *)(local_10 + 8);
          puVar12[0x43] = (uint)(fVar2 * (float)puVar12[4] + (float)puVar12[0x43]);
          puVar12[0x44] = (uint)(fVar2 * (float)puVar12[5] + (float)puVar12[0x44]);
          puVar12[0x45] = (uint)(fVar2 * (float)puVar12[6] + (float)puVar12[0x45]);
        }
        if ((*(byte *)(local_18 + 0x20) & 0x40) != 0) {
          local_ac = puVar12[2];
          local_b0 = puVar12[1];
          local_9c = *(float *)(local_18 + 0x2c) + *(float *)(local_1c + 0x68);
          local_a8 = puVar12[3];
          local_a4 = local_9c * *(float *)PTR_DAT_0069672c;
          local_a0 = local_9c * *(float *)(PTR_DAT_0069672c + 4);
          local_9c = local_9c * *(float *)(PTR_DAT_0069672c + 8);
          cVar6 = FUN_00505880(0xc0a0,&local_b0,&local_a4,*param_1,local_128);
          if (cVar6 != '\0') {
            local_20 = (*(float *)(local_18 + 0x2c) + *(float *)(local_1c + 0x68)) * local_114 -
                       *(float *)(local_1c + 0x68);
            fVar15 = (float10)FUN_00507430(puVar12[0xc],*(undefined4 *)(local_18 + 0x38),
                                           *(undefined4 *)(local_18 + 0x34));
            if (local_20 <= 0.0) {
              fVar17 = (float10)1.0;
            }
            else {
              fVar17 = (float10)1.0 - (float10)local_20 / (float10)*(float *)(local_18 + 0x2c);
            }
            fVar15 = (fVar17 * fVar17 * (float10)_DAT_0069c52c -
                     ((float10)local_104 * (float10)*pfVar1 +
                     (float10)local_fc * (float10)(float)puVar12[0x14] +
                     (float10)local_100 * (float10)(float)puVar12[0x13]) *
                     (float10)*(float *)(local_18 + 0x30)) * (float10)local_14[6] *
                     (float10)*(float *)(local_18 + 0x24) * (float10)*(float *)(local_10 + 8) *
                     fVar15;
            puVar12[0x43] =
                 (uint)(float)((float10)local_104 * fVar15 + (float10)(float)puVar12[0x43]);
            puVar12[0x44] =
                 (uint)(float)((float10)local_100 * fVar15 + (float10)(float)puVar12[0x44]);
            puVar12[0x45] =
                 (uint)(float)((float10)local_fc * fVar15 + (float10)(float)puVar12[0x45]);
            *puVar12 = *puVar12 | 0x10;
          }
        }
      }
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x21]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x22]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x23]);
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x24]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x25]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x26]);
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x2e]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x2f]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x30]);
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x31]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x32]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x33]);
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x3a]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x3b]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x3c]);
      puVar12[0x46] = (uint)((float)puVar12[0x46] + (float)puVar12[0x43]);
      puVar12[0x47] = (uint)((float)puVar12[0x47] + (float)puVar12[0x44]);
      puVar12[0x48] = (uint)((float)puVar12[0x48] + (float)puVar12[0x45]);
      local_44 = (float)puVar12[0x48] * (float)puVar12[0x10] -
                 (float)puVar12[0x11] * (float)puVar12[0x47];
      local_40 = (float)puVar12[0x46] * (float)puVar12[0x11] -
                 (float)puVar12[0x48] * (float)puVar12[0xf];
      puVar12[0x49] = (uint)local_44;
      puVar12[0x4a] = (uint)local_40;
      local_3c = (float)puVar12[0x47] * (float)puVar12[0xf] -
                 (float)puVar12[0x46] * (float)puVar12[0x10];
      puVar12[0x4b] = (uint)local_3c;
      *param_4 = *param_4 + (float)puVar12[0x46];
      param_4[1] = (float)puVar12[0x47] + param_4[1];
      param_4[2] = (float)puVar12[0x48] + param_4[2];
      *param_5 = (float)puVar12[0x49] + *param_5;
      param_5[1] = (float)puVar12[0x4a] + param_5[1];
      param_5[2] = (float)puVar12[0x4b] + param_5[2];
      local_38 = local_38 + 1;
      iVar10 = (int)(short)local_38;
    } while (iVar10 < *(int *)(local_10 + 0x74));
  }
  return;
}
#endif
