/**
 * Scratch physics model of sphere, pill and polygon proxies built from a query, and the tests run against it.
 */

#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

#include "halo/physics/physics_model.hpp"
#include "halo/math/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/core/libm.hpp"

extern "C" { uint8_t halo::physics::physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model); }
extern "C" { int16_t halo::physics::physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts); }
extern "C" { void halo::physics::physics_point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position, uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index); }
extern "C" { void halo::physics::physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp, real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index, physics_model *model); }
extern "C" { void halo::physics::physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame, int32_t surface_index, float margin, float thickness, int32_t object_index, physics_model *model); }
extern "C" { void halo::physics::physics_shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index, uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius, physics_model *model); }
extern "C" { void halo::physics::physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model); }
extern "C" { void halo::physics::physics_shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir, float height_offset, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type); }
extern "C" { uint8_t halo::physics::physics_shape_pill_test_point(real_point3d *point, physics_model_pill *pill, real_plane3d *out_normal, float *out_depth); }
extern "C" { uint8_t halo::physics::physics_shape_pill_test_ray(real_vector3d *delta, real_point3d *origin, real_plane3d *out_plane, physics_model_pill *pill, float *out_t); }
extern "C" { uint8_t halo::physics::physics_shape_polygon_test_point(physics_model_shape *shape, real_point3d *point, float *out_depth, real_plane3d *out_normal); }
extern "C" { uint8_t halo::physics::physics_shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape, real_vector3d *delta, float *out_t, real_plane3d *out_plane); }
extern "C" { uint8_t halo::physics::physics_shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin, real_vector3d *delta, float *out_t, float radius); }
extern "C" { uint8_t halo::physics::physics_shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere, real_plane3d *out_normal, float *out_depth); }
extern "C" { uint8_t halo::physics::physics_shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta, physics_model_sphere *sphere, real_plane3d *out_plane, float *out_t); }
extern "C" { void halo::physics::physics_shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices, real_plane3d *plane, float margin, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type, physics_model *model); }
extern "C" { uint32_t halo::physics::physics_shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact); }
extern "C" { uint32_t halo::physics::physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta, physics_model_contact *out_contact); }
extern "C" { void halo::physics::physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index); }

static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);
static auto &breakable_surface_state = halo::link::ref<breakable_surface_globals *>(halo::physics::vars().breakable_surface_state);
static auto &object_cluster_stamp = halo::link::ref<int32_t>(halo::physics::vars().object_cluster_stamp);
static auto &collideable_object_references = halo::link::ref<data_array *>(halo::physics::vars().collideable_object_references);
extern "C" { extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius); }
extern "C" { extern void collision_gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model); }
namespace halo::physics {

/**
 * Runs a sphere query (center, radius + 0.0625 margin) against the structure BSP. When flags bit
 * 0x20 is set and the query found geometry, converts it into physics_model proxies via
 * FUN_00503d90. When flags bit 0x80 is set and any leaf was touched, walks every object in every
 * touched cluster (deduping clusters and objects exactly like collision_test_movement_segment)
 * and folds each into the same physics_model via collision_gather_nearby_object_shapes. Returns
 * whether *model ended up with any sphere, pill or shape proxies.
 *
 * @address 0x506440
 */
uint8_t PhysicsModelOps::model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model)
{
    collision_bsp_sphere_result sphere_result;
    uint8_t found_surface;

    model->sphere_count = 0;
    model->pill_count = 0;
    model->shape_count = 0;

    if ((flags & 0x20) != 0 || (flags & 0xc0) != 0) {
        found_surface = (uint8_t)halo::physics::collision_bsp_query_sphere_init(global_structure_collision_bsp,
            k_maximum_breakable_surfaces_per_bsp, &sphere_result,
            breakable_surface_state->active[halo::scenario::globals().structure_bsp_index], center,
            radius + 0.0625f);

        if (found_surface && (flags & 0x20) != 0) {
            halo::physics::physics_shape_build_proxies_from_query(&sphere_result, (real_matrix4x3 *)0,
                global_structure_collision_bsp, x_offset, y_offset, -1, model);
        }

        if ((flags & 0x80) != 0 && sphere_result.leaf_count > 0) {
            int32_t stamp;
            int32_t i;

            if ((flags & 0xfff00) == 0) {
                flags |= 0xfff00;
            }
            halo::structures::globals().cluster_flood_stamp++;
            halo::objects::globals().object_globals->collecting_in_clusters = 1;
            stamp = object_cluster_stamp + 1;
            halo::structures::globals().cluster_flood_in_progress = 1;
            object_cluster_stamp = stamp;

            for (i = 0; i < sphere_result.leaf_count; i++) {
                int16_t cluster_index = ((ScenarioStructureBSPLeaf *)
                    halo::scenario::globals().structure_bsp->leaves.pointer)[sphere_result.leaves[i] & 0x7fffffff].cluster;

                if (halo::structures::globals().cluster_visit_stamp[cluster_index] != halo::structures::globals().cluster_flood_stamp) {
                    datum_index ref;

                    halo::structures::globals().cluster_visit_stamp[cluster_index] = halo::structures::globals().cluster_flood_stamp;
                    ref = halo::physics::globals().collideable_cluster_first[cluster_index];
                    while (ref != k_datum_index_none) {
                        object_cluster_reference *node = (object_cluster_reference *)
                            collideable_object_references->data + (ref & halo::k_slot_mask);
                        datum_index object_index = node->object_index;
                        object *obj;

                        if (object_index == k_datum_index_none) {
                            break;
                        }
                        obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;

                        if (obj->cluster_stamp != stamp) {
                            obj->cluster_stamp = stamp;
                            halo::physics::collision_gather_nearby_object_shapes(flags, object_index, center,
                                radius + 0.0625f, x_offset, y_offset, exclude_object_index, model);
                        }
                        ref = node->next_reference;
                    }
                }
            }

            halo::objects::globals().object_globals->collecting_in_clusters = 0;
            halo::structures::globals().cluster_flood_in_progress = 0;
        }
    }

    return (model->sphere_count != 0 || model->pill_count != 0 || model->shape_count != 0);
}

}

extern "C" { extern void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v); }
#define CONTACT_PLANE(c) ((real_plane3d *)&(c)->plane_i)
static float dot3(const real_vector3d *a, const real_vector3d *b)
{
    return a->i * b->i + a->j * b->j + a->k * b->k;
}

namespace halo::physics {

/**
 * Slides a point along the world's collision proxies and reports every surface it touched.
 *
 * Original register convention: EAX -> start_position, stack -> delta, model, out_position, out_velocity, max_contacts, contacts.
 *
 * @address 0x5067b0
 */
int16_t PhysicsModelOps::model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts)
{
    const double epsilon = (double)0.0001f;
    real_vector3d remaining = *delta;
    real_point3d position = *start_position;
    real_vector3d step = *delta;
    int16_t contact_count = 0;
    int16_t last_contact = -1;
    int16_t planes[3];
    int16_t plane_count = 0;
    real_plane3d plane;
    real_vector3d line_direction;
    real_point3d line_point;
    real_point3d corner;

    for (;;) {
        physics_model_contact *contact;
        real_point3d hit_point;
        int16_t new_planes[3];
        int16_t new_count;
        float scale;
        float along;
        int16_t i;

        if (halo::libm::fabs(step.i) < epsilon && halo::libm::fabs(step.j) < epsilon && halo::libm::fabs(step.k) < epsilon) {
            break;
        }
        contact = &contacts[contact_count];
        if (!halo::physics::physics_shape_test_ray(model, &position, &step, contact)) {
            position.x = contact->point_x;
            position.y = contact->point_y;
            position.z = contact->point_z;
            break;
        }
        contact_count++;
        scale = 1.0f - contact->t;
        remaining.i *= scale;
        remaining.j *= scale;
        remaining.k *= scale;
        hit_point.x = contact->point_x;
        hit_point.y = contact->point_y;
        hit_point.z = contact->point_z;

        last_contact++;
        plane = *CONTACT_PLANE(&contacts[last_contact]);
        new_planes[0] = last_contact;
        new_count = 1;

        along = -dot3(&plane.normal, &remaining);
        step.i = plane.normal.i * along + remaining.i;
        step.j = plane.normal.j * along + remaining.j;
        step.k = plane.normal.k * along + remaining.k;
        along = -((plane.normal.i * hit_point.x + plane.normal.j * hit_point.y + plane.normal.k * hit_point.z) -
                  plane.d);
        position.x = plane.normal.i * along + hit_point.x;
        position.y = plane.normal.j * along + hit_point.y;
        position.z = plane.normal.k * along + hit_point.z;

        if (plane_count > 0) {
            real_plane3d *plane0 = CONTACT_PLANE(&contacts[planes[0]]);
            real_plane3d *new_plane = CONTACT_PLANE(&contacts[last_contact]);
            uint8_t creased = 0;

            if (dot3(&step, &plane0->normal) < -0.0001f &&
                halo::math::plane3d_intersect_pair_to_line(line_direction, *plane0, *new_plane, line_point)) {
                creased = 1;
                new_planes[1] = planes[0];
                new_count = 2;
                along = dot3(&line_direction, &remaining) / dot3(&line_direction, &line_direction);
                step.i = line_direction.i * along;
                step.j = line_direction.j * along;
                step.k = line_direction.k * along;
                halo::math::point3d_project_onto_line(hit_point, line_direction, line_point, position);
                if (plane_count > 1) {
                    real_plane3d *plane1 = CONTACT_PLANE(&contacts[planes[1]]);
                    if (dot3(&step, &plane1->normal) < -0.0001f &&
                        halo::math::plane3d_intersect_three(*new_plane, *plane0, *plane1, corner)) {
                        new_planes[2] = planes[1];
                        new_count = 3;
                        step.i = 0.0f;
                        step.j = 0.0f;
                        step.k = 0.0f;
                        position = corner;
                    }
                }
            }
            if (!creased && plane_count > 1) {
                real_plane3d *plane1 = CONTACT_PLANE(&contacts[planes[1]]);
                if (dot3(&step, &plane1->normal) < -0.0001f &&
                    halo::math::plane3d_intersect_pair_to_line(line_direction, *plane1, *new_plane, line_point)) {
                    new_planes[1] = planes[1];
                    new_count = 2;
                    along = dot3(&line_direction, &remaining) / dot3(&line_direction, &line_direction);
                    step.i = line_direction.i * along;
                    step.j = line_direction.j * along;
                    step.k = line_direction.k * along;
                    halo::math::point3d_project_onto_line(hit_point, line_direction, line_point, position);
                }
            }
        }

        for (i = 0; i < new_count; i++) {
            planes[i] = new_planes[i];
        }
        plane_count = new_count;
        if (contact_count >= max_contacts) {
            break;
        }
    }

    *out_position = position;
    switch (plane_count) {
    case 0:
        *out_velocity = *delta;
        break;
    case 1: {
        float along = -(plane.normal.i * delta->i + plane.normal.k * delta->k + plane.normal.j * delta->j);
        out_velocity->i = plane.normal.i * along + delta->i;
        out_velocity->j = plane.normal.j * along + delta->j;
        out_velocity->k = plane.normal.k * along + delta->k;
        break;
    }
    case 2:
        halo::physics::vector3d_project_onto_direction(out_velocity, &line_direction, delta);
        break;
    default:
        out_velocity->i = 0.0f;
        out_velocity->j = 0.0f;
        out_velocity->k = 0.0f;
        break;
    }

    if (plane_count > 1 && contact_count < max_contacts) {
        physics_model_contact *source = &contacts[planes[plane_count - 1]];
        physics_model_contact *floor = &contacts[contact_count];
        real_vector3d *normal = (real_vector3d *)&floor->plane_i;
        int16_t lowest = -1;
        float lowest_k = 0.0f;
        int16_t i;

        floor->t = source->t;
        floor->point_x = source->point_x;
        floor->point_y = source->point_y;
        floor->point_z = source->point_z;
        contact_count++;
        floor->object_index = halo::k_dword_none;
        floor->surface_index = -1;
        floor->surface_flags = 0;
        floor->breakable_surface_index = 0;
        floor->material_type = -1;

        for (i = 0; i < plane_count; i++) {
            float k = contacts[planes[i]].plane_k;
            if (lowest_k > k) {
                lowest = i;
                lowest_k = k;
            }
        }

        if (plane_count == 2) {
            if (lowest == -1) {
                float along = -(line_direction.k / (line_direction.j * line_direction.j +
                    line_direction.i * line_direction.i + line_direction.k * line_direction.k));
                normal->i = line_direction.i * along + halo::math::globals().global_up3d_pointer->i;
                normal->j = line_direction.j * along + halo::math::globals().global_up3d_pointer->j;
                normal->k = line_direction.k * along + halo::math::globals().global_up3d_pointer->k;
            } else if (lowest == 0) {
                halo::math::vector3d_cross_product(*normal, CONTACT_PLANE(&contacts[planes[0]])->normal, line_direction);
            } else {
                halo::math::vector3d_cross_product(*normal, line_direction, CONTACT_PLANE(&contacts[planes[lowest]])->normal);
            }
            if (halo::math::vector3d_normalize_with_length(*normal) == 0.0f) {
                return (int16_t)(contact_count - 1);
            }
            floor->plane_d = line_point.y * normal->j + line_point.z * normal->k + line_point.x * normal->i;
        } else {
            if (lowest == -1) {
                *normal = *halo::math::globals().global_up3d_pointer;
            } else {
                real_plane3d *low = CONTACT_PLANE(&contacts[planes[lowest]]);
                float along = -low->normal.k;
                normal->i = along * low->normal.i + halo::math::globals().global_up3d_pointer->i;
                normal->j = along * low->normal.j + halo::math::globals().global_up3d_pointer->j;
                normal->k = along * low->normal.k + halo::math::globals().global_up3d_pointer->k;
                if (halo::math::vector3d_normalize_with_length(*normal) == 0.0f) {
                    return (int16_t)(contact_count - 1);
                }
            }
            floor->plane_d = corner.y * normal->j + corner.z * normal->k + corner.x * normal->i;
        }
    }
    return contact_count;
}

}

#undef CONTACT_PLANE

static auto &global_down3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &k_physics_displacement_directions = halo::link::ref<float [k_physics_displacement_direction_count][3]>(halo::physics::vars().k_physics_displacement_directions);
namespace halo::physics {

/**
 * Builds a physics_model around current_position (sphere radius = x_margin/2 + sample_radius +
 * y_margin) and checks whether current_position itself is already clear (no model overlap, and
 * object_collision_test_cluster_group reports no nearby-object collision either). If so, returns it unchanged. Otherwise
 * samples the 17-direction displacement ring (k_physics_displacement_directions) at
 * sample_radius, looking for a candidate that both misses the model and passes object_collision_test_cluster_group;
 * among those, prefers the first one whose separating-plane Z component exceeds cos(40 degrees)
 * (a roughly floor-like recovery direction) and walks toward it via physics_point_walk_toward_target.
 * If none qualify but at least one clear candidate was found, walks toward the FIRST clear
 *
 * @address 0x507170
 */
uint8_t PhysicsModelOps::point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position)
{
    physics_model model;
    physics_model_contact contact;
    real_point3d sweep_center;
    uint8_t have_fallback = 0;
    real_point3d fallback_candidate;
    uint16_t i;

    sweep_center.x = current_position->x;
    sweep_center.y = current_position->y;
    sweep_center.z = x_margin * 0.5f + current_position->z;

    halo::physics::physics_model_build_from_sphere_query(flags, &sweep_center,
        x_margin * 0.5f + sample_radius + y_margin, x_margin, y_margin, exclude_object_index,
        &model);

    if (!halo::physics::physics_shape_test_point(&model, current_position, &contact)) {
        if (!halo::physics::object_collision_test_cluster_group(flags, current_position, exclude_object_index)) {
            *out_position = *current_position;
            return 1;
        }
    }

    for (i = 0; i < k_physics_displacement_direction_count; i++) {
        real_point3d candidate;

        candidate.x = sample_radius * k_physics_displacement_directions[i][0] + current_position->x;
        candidate.y = sample_radius * k_physics_displacement_directions[i][1] + current_position->y;
        candidate.z = sample_radius * k_physics_displacement_directions[i][2] + current_position->z;

        if (!halo::physics::physics_shape_test_point(&model, &candidate, &contact) &&
            !halo::physics::object_collision_test_cluster_group(flags, &candidate, exclude_object_index)) {
            real_vector3d probe;
            probe.i = sample_radius * global_down3d_pointer->i;
            probe.j = sample_radius * global_down3d_pointer->j;
            probe.k = sample_radius * global_down3d_pointer->k;

            if (halo::physics::physics_shape_test_ray(&model, &candidate, &probe, &contact) && 0.76604444f < contact.plane_k) {
                halo::physics::physics_point_walk_toward_target((physics_point_walk_state *)&contact, &candidate, flags, &probe,
                    exclude_object_index);
                out_position->x = contact.point_x;
                out_position->y = contact.point_y;
                out_position->z = contact.point_z;
                return 1;
            }

            if (!have_fallback) {
                fallback_candidate = candidate;
                have_fallback = 1;
            }
        }
    }

    if (have_fallback) {
        real_vector3d to_fallback;
        to_fallback.i = current_position->x - fallback_candidate.x;
        to_fallback.j = current_position->y - fallback_candidate.y;
        to_fallback.k = current_position->z - fallback_candidate.z;
        halo::physics::physics_shape_test_ray(&model, &fallback_candidate, &to_fallback, &contact);
        halo::physics::physics_point_walk_toward_target((physics_point_walk_state *)&contact, &fallback_candidate, flags, &to_fallback,
            exclude_object_index);
        out_position->x = contact.point_x;
        out_position->y = contact.point_y;
        out_position->z = contact.point_z;
        return 1;
    }

    return 0;
}

}

namespace halo::physics {

/**
 * Re-resolves point's containing leaf in the current structure BSP and, if it still lands
 * inside the tree at all, runs a zero-radius-ish sphere query (radius) at the same spot against
 * the current BSP's still-intact breakable surfaces. Returns whether the point is still
 * considered active (either outside the tree entirely, or a fresh sphere touch was found).
 * bsp3d_node_find_leaf, 0x50556c pushes it; every caller does mov/lea edx), not ESI.
 *
 * Original register convention: EDX -> point, stack -> radius.
 *
 * @address 0x505540
 */
uint8_t PhysicsModelOps::point_refresh_leaf(real_point3d *point, float radius)
{
    if (halo::physics::bsp3d_node_find_leaf(0, global_structure_collision_bsp, point) != halo::k_dword_none) {
        collision_bsp_sphere_result result;
        uint32_t *breakable_surfaces =
            (uint32_t *)((uint8_t *)breakable_surface_state + 1 +
                         halo::scenario::globals().structure_bsp_index * 0x20);

        if (!halo::physics::collision_bsp_query_sphere_init(global_structure_collision_bsp,
                                              k_maximum_breakable_surfaces_per_bsp, &result,
                                              breakable_surfaces, point, radius)) {
            return 0;
        }
    }
    return 1;
}

}

namespace halo::physics {

/**
 * Tests state->position (initially the desired target) against nearby objects via
 * object_collision_test_cluster_group; while it is blocked and state->t > 0, backs it off by a fixed 0.03125 step along
 * -step_direction (recomputed from start_position each time) and retests. Stops as soon as a
 * clear position is found. If the whole step distance is consumed without ever clearing, snaps
 * state->position back to exactly *start_position.
 *
 * @address 0x5070d0
 */
void PhysicsModelOps::point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position, uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index)
{
    if (0.0f < state->t) {
        do {
            if (!halo::physics::object_collision_test_cluster_group(flags, &state->position, exclude_object_index)) {
                break;
            }
            state->t -= 0.03125f;
            state->position.x = state->t * step_direction->i + start_position->x;
            state->position.y = state->t * step_direction->j + start_position->y;
            state->position.z = state->t * step_direction->k + start_position->z;
        } while (0.0f < state->t);
    }

    if (state->t <= 0.0f) {
        state->position = *start_position;
    }
}

}

namespace halo::physics {

/**
 * REWRITTEN from objdump 0x503ae0..0x503c3c: the draft took the start vertex and edge direction as parameters,
 * but the original computes both from the BSP (vertices at bsp+0x58, edge = {start, end, ..., left_surface +0x10,
 * right_surface +0x14}), takes five stack arguments, and passes the scalar triple product (left normal on the
 * stack, right normal in EAX, direction in EDX); returns when it is <= -0.0001 (same plane sides) or >= 0.0001
 * (opposite sides). The transformed direction is written in place; the transformed start vertex goes to a local.
 *
 * Original register convention: EAX -> edge_index, ECX -> bsp, stack -> matrix, height_offset, thickness, object_index, model.
 *
 * @address 0x503ae0
 */
void PhysicsModelOps::shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp, real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index, physics_model *model)
{
    ModelCollisionGeometryBSPEdge *edge =
        &((ModelCollisionGeometryBSPEdge *)bsp->edges.pointer)[edge_index];
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPSurface *left_surface = &surfaces[edge->left_surface];
    ModelCollisionGeometryBSPSurface *right_surface = &surfaces[edge->right_surface];
    uint32_t left_plane = left_surface->plane;
    uint32_t right_plane = right_surface->plane;
    real_point3d *start;
    real_point3d *end;
    real_vector3d direction;
    real_point3d transformed_start;
    real_point3d *near_vertex;
    int32_t surface_index;

    if (left_plane == right_plane) {
        return;
    }

    start = (real_point3d *)((uint8_t *)bsp->vertices.pointer + edge->start_vertex * 0x10);
    end = (real_point3d *)((uint8_t *)bsp->vertices.pointer + edge->end_vertex * 0x10);
    direction.i = end->x - start->x;
    direction.j = end->y - start->y;
    direction.k = end->z - start->z;

    if ((left_plane & 0x7fffffffu) != (right_plane & 0x7fffffffu)) {
        uint8_t *planes = (uint8_t *)bsp->planes.pointer;
        real_vector3d *left_normal = (real_vector3d *)(planes + (left_plane & 0x7fffffffu) * 0x10);
        real_vector3d *right_normal = (real_vector3d *)(planes + (right_plane & 0x7fffffffu) * 0x10);
        real triple = halo::math::vector3d_scalar_triple_product(*left_normal, *right_normal, direction);
        if (((left_plane & 0x80000000u) != 0) == ((right_plane & 0x80000000u) != 0)) {
            if (triple <= -0.0001f) {
                return;
            }
        } else if (!(triple < 0.0001f)) {
            return;
        }
    }

    surface_index = -1;
    if (object_index == -1) {
        surface_index = (int32_t)edge->left_surface;
    }

    near_vertex = start;
    if (matrix != 0) {
        halo::math::matrix4x3_transform_vector(direction, direction, *matrix);
        halo::math::matrix4x3_transform_point(transformed_start, *start, *matrix);
        near_vertex = &transformed_start;
    }

    halo::physics::physics_shape_edge_to_pill_and_quad(model, near_vertex, &direction, height_offset, thickness,
                                         (uint32_t)object_index, surface_index,
                                         left_surface->flags,
                                         left_surface->breakable_surface,
                                         (int16_t)left_surface->material);
}

}

extern "C" { extern int16_t collision_bsp_surface_get_vertices(ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point3d *out_vertices); }
namespace halo::physics {

/**
 * stack -> surface_index, margin, thickness, object_index, model
 *
 * Original register convention: EAX -> bsp, ESI -> moving_frame,.
 *
 * @address 0x503c50
 */
void PhysicsModelOps::shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame, int32_t surface_index, float margin, float thickness, int32_t object_index, physics_model *model)
{
    ModelCollisionGeometryBSPSurface *surface =
        &((ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[surface_index];

    real_point3d vertices[8];
    real_plane3d plane;
    int16_t vertex_count = halo::physics::collision_bsp_surface_get_vertices(bsp, surface_index, vertices);

    halo::structures::structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surface->plane);

    if (moving_frame != 0) {
        int16_t i;
        for (i = 0; i < vertex_count; i++) {
            halo::math::matrix4x3_transform_point(vertices[i], vertices[i], *(real_matrix4x3 *)moving_frame);
        }
        {
            float new_i = plane.normal.j * moving_frame[4] + plane.normal.k * moving_frame[7] +
                          plane.normal.i * moving_frame[1];
            float new_j = plane.normal.j * moving_frame[5] + plane.normal.k * moving_frame[8] +
                          plane.normal.i * moving_frame[2];
            float new_k = plane.normal.k * moving_frame[9] + plane.normal.i * moving_frame[3] +
                          plane.normal.j * moving_frame[6];
            plane.d = new_i * moving_frame[10] + plane.d * moving_frame[0] +
                      new_j * moving_frame[11] + new_k * moving_frame[12];
            plane.normal.i = new_i;
            plane.normal.j = new_j;
            plane.normal.k = new_k;
        }
    }

    {
        int32_t out_surface_index = surface_index;
        if (object_index != -1) {
            out_surface_index = -1;
        }
        halo::physics::physics_shape_surface_to_polygon(vertex_count, vertices, &plane, margin, thickness,
                                          (uint32_t)object_index, out_surface_index,
                                          surface->flags, surface->breakable_surface,
                                          (int16_t)surface->material, model);
    }
}

}

namespace halo::physics {

/**
 * Implements physics shape add vertex proxy.
 *
 * @address 0x503a60
 */
void PhysicsModelOps::shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index, uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius, physics_model *model)
{
    ModelCollisionGeometryBSPVertex *vertex_rec =
        &((ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer)[vertex_index];
    ModelCollisionGeometryBSPEdge *edge =
        &((ModelCollisionGeometryBSPEdge *)bsp->edges.pointer)[vertex_rec->first_edge];
    ModelCollisionGeometryBSPSurface *surface =
        &((ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[edge->left_surface];
    int32_t surface_index = (object_index == halo::k_dword_none) ? (int32_t)edge->left_surface : -1;
    real_point3d transformed;
    real_point3d *vertex_point;

    if (matrix != 0) {
        halo::math::matrix4x3_transform_point(transformed, *((real_point3d *)&vertex_rec->point), *matrix);
        vertex_point = &transformed;
    } else {
        vertex_point = (real_point3d *)&vertex_rec->point;
    }

    halo::physics::physics_shape_vertex_to_sphere(model, vertex_point, (int16_t)surface->material, height_offset,
                                    radius, object_index, surface_index, surface->flags,
                                    surface->breakable_surface);
}

}

namespace halo::physics {

/**
 * Adds a sphere/pill proxy for every vertex, pill/quad proxies for every edge and a polygon
 * proxy for every surface the sphere query found, transformed by `matrix` when there is one.
 *
 * Original register convention: EDI -> result, EAX -> matrix, stack -> bsp, margin, thickness, object_index, model.
 *
 * @address 0x503d90
 */
void PhysicsModelOps::shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model)
{
    int32_t i;

    for (i = 0; i < result->vertex_count; i++) {
        halo::physics::physics_shape_add_vertex_proxy(bsp, (uint32_t)result->vertices[i], (uint32_t)object_index, matrix,
                                        margin, thickness, model);
    }
    for (i = 0; i < result->edge_count; i++) {
        halo::physics::physics_shape_add_edge_proxy(result->edges[i], bsp, matrix, margin, thickness, object_index, model);
    }
    for (i = 0; i < result->surface_count; i++) {
        halo::physics::physics_shape_add_surface_proxy(bsp, (float *)matrix, result->surfaces[i], margin, thickness,
                                         object_index, model);
    }
}

}

static void append_quad_vertices(physics_model_shape *shape, float quad[4][3],
                                  projection_axis_pair proj)
{
    int i;
    for (i = 0; i < 4; i++) {
        shape->vertices[i][0] = quad[i][proj.i];
        shape->vertices[i][1] = quad[i][proj.j];
    }
}

namespace halo::physics {

/**
 * stack -> height_offset, thickness, object_index, surface_index, surface_flags,
 * breakable_surface_index, material_type
 *
 * Original register convention: EAX -> model, ECX -> near_vertex, EDX -> edge_dir,.
 *
 * @address 0x503490
 */
void PhysicsModelOps::shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir, float height_offset, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type)
{
    if (model->pill_count < 0x100) {
        physics_model_pill *pill = &model->pills[model->pill_count];
        model->pill_count += 1;
        pill->object_index = object_index;
        pill->surface_index = surface_index;
        pill->surface_flags = surface_flags;
        pill->breakable_surface_index = breakable_surface_index;
        pill->material_type = material_type;
        pill->origin_x = near_vertex->x;
        pill->origin_y = near_vertex->y;
        pill->origin_z = near_vertex->z;
        pill->extent_i = edge_dir->i;
        pill->extent_j = edge_dir->j;
        pill->extent_k = edge_dir->k;
        pill->radius = thickness;
    }

    if (0.0f < height_offset) {
        if (model->pill_count < 0x100) {
            physics_model_pill *pill = &model->pills[model->pill_count];
            model->pill_count += 1;
            pill->object_index = object_index;
            pill->surface_index = surface_index;
            pill->surface_flags = surface_flags;
            pill->breakable_surface_index = breakable_surface_index;
            pill->material_type = material_type;
            pill->origin_x = near_vertex->x;
            pill->origin_y = near_vertex->y;
            pill->origin_z = near_vertex->z - height_offset;
            pill->extent_i = edge_dir->i;
            pill->extent_j = edge_dir->j;
            pill->extent_k = edge_dir->k;
            pill->radius = thickness;
        }

        {
            float perp_i = -edge_dir->j;
            float perp_j = edge_dir->i;
            float perp_len = (float)halo::libm::sqrt((double)(edge_dir->i * edge_dir->i + perp_i * perp_i));

            if (0.0001 <= (float)halo::libm::fabs((double)perp_len)) {
                perp_i = (1.0f / perp_len) * perp_i;
                perp_j = (1.0f / perp_len) * perp_j;
                if (perp_len != 0.0f) {
                    float quad[4][3];
                    float plane_d;

                    quad[0][0] = near_vertex->x;
                    quad[0][1] = near_vertex->y;
                    quad[0][2] = near_vertex->z;
                    plane_d = perp_i * near_vertex->x + perp_j * near_vertex->y;
                    quad[1][0] = near_vertex->x + edge_dir->i;
                    quad[1][1] = near_vertex->y + edge_dir->j;
                    quad[1][2] = near_vertex->z + edge_dir->k;
                    quad[3][0] = near_vertex->x;
                    quad[3][1] = near_vertex->y;
                    quad[3][2] = near_vertex->z - height_offset;
                    quad[2][0] = quad[1][0];
                    quad[2][1] = quad[1][1];
                    quad[2][2] = quad[1][2] - height_offset;

                    if (model->shape_count < 0x100) {
                        physics_model_shape *shape = &model->shapes[model->shape_count];
                        model->shape_count += 1;
                        shape->object_index = object_index;
                        shape->surface_index = surface_index;
                        shape->surface_flags = surface_flags;
                        shape->breakable_surface_index = breakable_surface_index;
                        shape->material_type = material_type;
                        shape->plane_i = perp_i;
                        shape->plane_j = perp_j;
                        shape->plane_k = 0.0f;
                        shape->plane_d = plane_d;
                        shape->thickness = thickness;
                        shape->projection_axis = halo::math::vector3d_major_axis_index(
                            *((real_vector3d *)&shape->plane_i));
                        shape->projection_sign =
                            0.0f < ((float *)&shape->plane_i)[shape->projection_axis];
                        shape->vertex_count = 4;
                        {
                            projection_axis_pair proj = halo::math::globals().k_projection_axes[shape->projection_axis * 2 +
                                                                           shape->projection_sign];
                            append_quad_vertices(shape, quad, proj);
                        }
                    }

                    {
                        float new_quad[4][3];
                        new_quad[0][0] = quad[0][0];
                        new_quad[0][1] = quad[0][1];
                        new_quad[0][2] = quad[0][2];
                        new_quad[1][0] = quad[3][0];
                        new_quad[1][1] = quad[3][1];
                        new_quad[1][2] = quad[3][2];
                        new_quad[2][0] = quad[2][0];
                        new_quad[2][1] = quad[2][1];
                        new_quad[2][2] = quad[2][2];
                        new_quad[3][0] = quad[1][0];
                        new_quad[3][1] = quad[1][1];
                        new_quad[3][2] = quad[1][2];

                        if (model->shape_count < 0x100) {
                            physics_model_shape *shape = &model->shapes[model->shape_count];
                            model->shape_count += 1;
                            shape->object_index = object_index;
                            shape->surface_index = surface_index;
                            shape->surface_flags = surface_flags;
                            shape->breakable_surface_index = breakable_surface_index;
                            shape->material_type = material_type;
                            shape->plane_i = -perp_i;
                            shape->plane_j = -perp_j;
                            shape->plane_k = 0.0f;
                            shape->plane_d = -plane_d;
                            shape->thickness = thickness;
                            shape->projection_axis = halo::math::vector3d_major_axis_index(
                                *((real_vector3d *)&shape->plane_i));
                            shape->projection_sign =
                                0.0f < ((float *)&shape->plane_i)[shape->projection_axis];
                            shape->vertex_count = 4;
                            {
                                projection_axis_pair proj =
                                    halo::math::globals().k_projection_axes[shape->projection_axis * 2 +
                                                       shape->projection_sign];
                                append_quad_vertices(shape, new_quad, proj);
                            }
                        }
                    }
                }
            }
        }
    }
}

}

namespace halo::physics {

/**
 * VERIFIED (logic) against disassembly 0x503050..0x503288 (2026-09-30): the discriminant, both roots, the t clamp, the edge
 * parameter branches (near vertex / far vertex sphere test, out_edge_fraction 0 / 1 / param/len) and the return values match;
 * only the floating point summation order was aligned with the x87 code. STILL-UNSURE: the original keeps every
 * intermediate in 80-bit registers, so a rare 6/200 difference at the comparisons may remain.
 * stack -> radius, out_t, out_edge_fraction
 *
 * Original register convention: ECX -> near_vertex, EDX -> delta, EBX -> origin, EDI -> edge_dir,.
 *
 * @address 0x503050
 */
uint8_t PhysicsModelOps::shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta, real_point3d *origin, real_vector3d *edge_dir, float radius, float *out_t, float *out_edge_fraction)
{
    float rel_x = origin->x - near_vertex->x;
    float rel_y = origin->y - near_vertex->y;
    float rel_z = origin->z - near_vertex->z;

    float edge_len_sq = (edge_dir->k * edge_dir->k + edge_dir->i * edge_dir->i) + edge_dir->j * edge_dir->j;
    float edge_dot_delta = (edge_dir->k * delta->k + edge_dir->j * delta->j) + edge_dir->i * delta->i;
    float delta_len_sq = (delta->i * delta->i + delta->j * delta->j) + delta->k * delta->k;
    float disc_scale = delta_len_sq * edge_len_sq - edge_dot_delta * edge_dot_delta;

    if (disc_scale != 0.0f) {
        float rel_dot_edge = (rel_x * edge_dir->i + rel_y * edge_dir->j) + rel_z * edge_dir->k;
        float rel_dot_delta = (rel_x * delta->i + rel_y * delta->j) + rel_z * delta->k;
        float b = rel_dot_edge * edge_dot_delta - rel_dot_delta * edge_len_sq;
        float rel_len_sq = (rel_x * rel_x + rel_y * rel_y) + rel_z * rel_z;
        float disc = b * b - ((rel_len_sq - radius * radius) * edge_len_sq - rel_dot_edge * rel_dot_edge) *
                              disc_scale;

        if (0.0f <= disc) {
            float sqrt_disc = (float)halo::libm::sqrt((double)disc);
            float t = -((sqrt_disc + b) * (1.0f / disc_scale));

            if ((t <= 1.0f) && (0.0f <= -((b - sqrt_disc) * (1.0f / disc_scale)))) {
                float edge_param;

                if (t < 0.0f) {
                    t = 0.0f;
                }
                edge_param = edge_dot_delta * t + rel_dot_edge;
                if (0.0f <= edge_param) {
                    if (edge_param <= edge_len_sq) {
                        *out_t = t;
                        *out_edge_fraction = edge_param / edge_len_sq;
                        return 1;
                    }
                    {
                        real_point3d far_vertex;
                        far_vertex.x = near_vertex->x + edge_dir->i;
                        far_vertex.y = near_vertex->y + edge_dir->j;
                        far_vertex.z = near_vertex->z + edge_dir->k;
                        if (halo::physics::physics_shape_sphere_sweep_test_ray(&far_vertex, origin, delta, out_t,
                                                                 radius)) {
                            *out_edge_fraction = 1.0f;
                            return 1;
                        }
                    }
                } else {
                    if (halo::physics::physics_shape_sphere_sweep_test_ray(near_vertex, origin, delta, out_t,
                                                             radius)) {
                        *out_edge_fraction = 0.0f;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Implements physics shape pill test point.
 *
 * Original register convention: EAX -> point, ESI -> pill, EDI -> out_normal, stack -> out_depth.
 *
 * @address 0x503f90
 */
uint8_t PhysicsModelOps::shape_pill_test_point(real_point3d *point, physics_model_pill *pill, real_plane3d *out_normal, float *out_depth)
{
    float rel_x = point->x - pill->origin_x;
    float rel_y = point->y - pill->origin_y;
    float rel_z = point->z - pill->origin_z;
    float proj = rel_x * pill->extent_i + rel_y * pill->extent_j + rel_z * pill->extent_k;

    if (proj >= 0.0f) {
        float extent_len_sq =
            pill->extent_k * pill->extent_k + pill->extent_j * pill->extent_j +
            pill->extent_i * pill->extent_i;

        if (proj < extent_len_sq || proj == extent_len_sq) {
            float perp_dist_sq =
                (rel_z * rel_z + rel_y * rel_y + rel_x * rel_x) * extent_len_sq - proj * proj;
            float radius_sq_scaled = pill->radius * pill->radius * extent_len_sq;

            if (radius_sq_scaled >= perp_dist_sq && radius_sq_scaled != perp_dist_sq) {
                real vector_len;

                if (extent_len_sq <= 0.0f) {
                    out_normal->normal.i = rel_x;
                    out_normal->normal.j = rel_y;
                    out_normal->normal.k = rel_z;
                } else {
                    float t = proj / extent_len_sq;
                    out_normal->normal.i = rel_x - t * pill->extent_i;
                    out_normal->normal.j = rel_y - t * pill->extent_j;
                    out_normal->normal.k = rel_z - t * pill->extent_k;
                }
                vector_len = halo::math::vector3d_normalize_with_length(*((real_vector3d *)&out_normal->normal));
                if (vector_len == 0.0f) {
                    out_normal->normal.i = 0.0f;
                    out_normal->normal.j = 0.0f;
                    out_normal->normal.k = 1.0f;
                }
                out_normal->d = pill->origin_x * out_normal->normal.i +
                                 pill->origin_y * out_normal->normal.j +
                                 pill->origin_z * out_normal->normal.k + pill->radius;
                *out_depth = pill->radius - vector_len;
                return 1;
            }
        }
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Tests a world-space ray (origin, delta) against pill, the ray counterpart of
 * physics_shape_pill_test_point. Solves the swept-cylinder quadratic for the along-axis
 * parameter range, clips it against the pill's flat end caps, and reports the near root when
 * the clipped range is non-empty. On a hit, out_plane is the outward-facing separating plane at
 * the hit point (the component of the hit point perpendicular to the pill's axis).
 *
 * Original register convention: ECX -> delta, EDX -> origin, EBX -> out_plane, EDI -> pill, stack -> out_t.
 *
 * @address 0x5045c0
 */
uint8_t PhysicsModelOps::shape_pill_test_ray(real_vector3d *delta, real_point3d *origin, real_plane3d *out_plane, physics_model_pill *pill, float *out_t)
{
    float extent_len_sq = pill->extent_k * pill->extent_k + pill->extent_j * pill->extent_j +
                           pill->extent_i * pill->extent_i;
    float proj_delta_extent =
        pill->extent_i * delta->i + pill->extent_j * delta->j + pill->extent_k * delta->k;
    float denom = (delta->k * delta->k + delta->j * delta->j + delta->i * delta->i) * extent_len_sq -
                  proj_delta_extent * proj_delta_extent;

    if (denom != 0.0f) {
        real_point3d rel;
        float proj_rel_extent;
        float b, disc;

        rel.x = origin->x - pill->origin_x;
        rel.y = origin->y - pill->origin_y;
        rel.z = origin->z - pill->origin_z;
        proj_rel_extent = rel.x * pill->extent_i + rel.y * pill->extent_j + rel.z * pill->extent_k;
        b = proj_rel_extent * proj_delta_extent -
            (rel.z * delta->k + rel.x * delta->i + rel.y * delta->j) * extent_len_sq;
        disc = b * b - (((rel.x * rel.x + rel.y * rel.y + rel.z * rel.z) -
                          pill->radius * pill->radius) * extent_len_sq -
                         proj_rel_extent * proj_rel_extent) * denom;

        if (disc >= 0.0f) {
            float t0 = (b - (float)halo::libm::sqrt((double)disc)) * (1.0f / denom);
            float t1 = ((float)halo::libm::sqrt((double)disc) + b) * (1.0f / denom);

            if (t0 <= 1.0f && t1 >= 0.0f) {
                if (t0 < 0.0f) {
                    t0 = 0.0f;
                }
                if (t1 > 1.0f) {
                    t1 = 1.0f;
                }
                if (proj_delta_extent == 0.0f) {
                    if (proj_rel_extent < 0.0f) {
                        return 0;
                    }
                    if (proj_rel_extent > extent_len_sq) {
                        return 0;
                    }
                } else {
                    float cap_a = -(proj_rel_extent * (1.0f / proj_delta_extent));
                    float cap_b = (extent_len_sq - proj_rel_extent) * (1.0f / proj_delta_extent);

                    if (proj_delta_extent <= 0.0f) {
                        if (t0 < cap_b) {
                            t0 = cap_b;
                        }
                        if (cap_a < t1) {
                            t1 = cap_a;
                        }
                    } else {
                        if (t0 < cap_a) {
                            t0 = cap_a;
                        }
                        if (cap_b < t1) {
                            t1 = cap_b;
                        }
                    }
                    if (t0 > t1) {
                        return 0;
                    }
                }

                {
                    real_point3d hit_relative;
                    float proj_hit_extent;
                    real length;

                    *out_t = t0;
                    halo::math::point3d_add_scaled(hit_relative, *delta, rel, t0);

                    proj_hit_extent = hit_relative.x * pill->extent_i +
                                      hit_relative.y * pill->extent_j +
                                      hit_relative.z * pill->extent_k;
                    out_plane->normal.i = hit_relative.x - (proj_hit_extent / extent_len_sq) * pill->extent_i;
                    out_plane->normal.j = hit_relative.y - (proj_hit_extent / extent_len_sq) * pill->extent_j;
                    out_plane->normal.k = hit_relative.z - (proj_hit_extent / extent_len_sq) * pill->extent_k;

                    length = halo::math::vector3d_normalize_with_length(out_plane->normal);
                    if (length == 0.0f) {
                        out_plane->normal.i = 1.0f;
                        out_plane->normal.j = 0.0f;
                        out_plane->normal.k = 0.0f;
                    }
                    out_plane->d = pill->origin_x * out_plane->normal.i +
                                    pill->origin_y * out_plane->normal.j +
                                    pill->origin_z * out_plane->normal.k + pill->radius;
                    return 1;
                }
            }
        }
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Implements physics shape polygon test point.
 *
 * Original register convention: ECX -> shape, EDX -> point, stack -> out_depth, out_normal.
 *
 * @address 0x504120
 */
uint8_t PhysicsModelOps::shape_polygon_test_point(physics_model_shape *shape, real_point3d *point, float *out_depth, real_plane3d *out_normal)
{
    float dist = shape->plane_i * point->x + shape->plane_k * point->z +
                 shape->plane_j * point->y - shape->plane_d;

    if (0.0f <= dist && dist < shape->thickness) {
        float t = -dist;
        real_point3d proj;
        projection_axis_pair proj_axes;
        float proj2d[2];
        int32_t i;

        proj.x = t * shape->plane_i + point->x;
        proj.y = t * shape->plane_j + point->y;
        proj.z = t * shape->plane_k + point->z;
        proj_axes = halo::math::globals().k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
        proj2d[0] = ((float *)&proj)[proj_axes.i];
        proj2d[1] = ((float *)&proj)[proj_axes.j];

        for (i = 0; i < shape->vertex_count; i++) {
            int32_t next = (i + 1 < shape->vertex_count) ? (i + 1) : 0;
            float cross = (shape->vertices[next][1] - proj2d[1]) *
                              (shape->vertices[i][0] - proj2d[0]) -
                          (shape->vertices[i][1] - proj2d[1]) *
                              (shape->vertices[next][0] - proj2d[0]);
            if (cross < 0.0f) {
                return 0;
            }
        }

        out_normal->normal.i = shape->plane_i;
        out_normal->normal.j = shape->plane_j;
        out_normal->normal.k = shape->plane_k;
        out_normal->d = shape->plane_d + shape->thickness;
        *out_depth = shape->thickness - dist;
        return 1;
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Tests a world-space ray (origin, delta) against shape, the ray counterpart of
 * physics_shape_polygon_test_point. Clips the ray's plane-crossing interval [t_min, t_max]
 * against the polygon's thickness slab, then narrows it further against each boundary edge of
 * the polygon's 2D projection (Cyrus-Beck line clipping); reports the entry fraction t_min when
 * the clipped interval is still non-empty after every edge.
 *
 * Original register convention: EAX -> origin, ECX -> shape, EDX -> delta, stack -> out_t, out_plane.
 *
 * @address 0x5048d0
 */
uint8_t PhysicsModelOps::shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape, real_vector3d *delta, float *out_t, real_plane3d *out_plane)
{
    float dist = origin->x * shape->plane_i + shape->plane_k * origin->z +
                 shape->plane_j * origin->y - shape->plane_d;
    float dot_delta_normal =
        delta->i * shape->plane_i + shape->plane_j * delta->j + shape->plane_k * delta->k;
    float t_min = 0.0f;
    float t_max = 1.0f;
    real_point3d point_on_plane;
    real_vector3d delta_on_plane;
    projection_axis_pair proj_axes;
    float point_proj[2];
    float delta_proj[2];
    int32_t vertex_count;
    int32_t i;

    if (dot_delta_normal == 0.0f) {
        if (dist < 0.0f || dist >= shape->thickness) {
            return 0;
        }
    } else {
        float t_a = -(dist * (1.0f / dot_delta_normal));
        float t_b = -((dist - shape->thickness) * (1.0f / dot_delta_normal));

        if (dot_delta_normal > 0.0f) {
            if (0.0f < t_a) {
                t_min = t_a;
            }
            if (t_b < 1.0f) {
                t_max = t_b;
            }
        } else {
            if (0.0f < t_b) {
                t_min = t_b;
            }
            if (t_a < 1.0f) {
                t_max = t_a;
            }
        }
        if (t_min > t_max) {
            return 0;
        }
    }

    dist = -dist;
    point_on_plane.x = dist * shape->plane_i + origin->x;
    point_on_plane.y = dist * shape->plane_j + origin->y;
    point_on_plane.z = dist * shape->plane_k + origin->z;

    dot_delta_normal = -dot_delta_normal;
    delta_on_plane.i = dot_delta_normal * shape->plane_i + delta->i;
    delta_on_plane.j = dot_delta_normal * shape->plane_j + delta->j;
    delta_on_plane.k = dot_delta_normal * shape->plane_k + delta->k;

    proj_axes = halo::math::globals().k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
    point_proj[0] = ((float *)&point_on_plane)[proj_axes.i];
    point_proj[1] = ((float *)&point_on_plane)[proj_axes.j];
    delta_proj[0] = ((float *)&delta_on_plane)[proj_axes.i];
    delta_proj[1] = ((float *)&delta_on_plane)[proj_axes.j];

    vertex_count = shape->vertex_count;
    for (i = 0; i < vertex_count; i++) {
        int32_t next = (i + 1 < vertex_count) ? (i + 1) : 0;
        float edge_x = shape->vertices[next][0] - shape->vertices[i][0];
        float edge_y = shape->vertices[next][1] - shape->vertices[i][1];
        float denom = edge_y * delta_proj[0] - delta_proj[1] * edge_x;
        float numer = (point_proj[1] - shape->vertices[i][1]) * edge_x -
                      edge_y * (point_proj[0] - shape->vertices[i][0]);

        if (denom == 0.0f) {
            if (numer < 0.0f) {
                return 0;
            }
        } else {
            float t = numer / denom;

            if (denom < 0.0f) {
                if (t > t_min) {
                    t_min = t;
                }
            } else {
                if (t < t_max) {
                    t_max = t;
                }
            }
            if (t_min > t_max) {
                return 0;
            }
        }
    }

    *out_t = t_min;
    out_plane->normal.i = shape->plane_i;
    out_plane->normal.j = shape->plane_j;
    out_plane->normal.k = shape->plane_k;
    out_plane->d = shape->plane_d + shape->thickness;
    return 1;
}

}

namespace halo::physics {

/**
 * Implements physics shape sphere sweep test ray.
 *
 * Original register convention: EAX -> point, ECX -> origin, EDX -> delta, ESI -> out_t, stack -> radius.
 *
 * @address 0x503290
 */
uint8_t PhysicsModelOps::shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin, real_vector3d *delta, float *out_t, float radius)
{
    float dx = origin->x - point->x;
    float dy = origin->y - point->y;
    float dz = origin->z - point->z;
    float c = (dz * dz + dy * dy + dx * dx) - radius * radius;

    if (c == c && c <= 0.0f) {
        *out_t = 0.0f;
        return 1;
    }

    {
        float b = dz * delta->k + dy * delta->j + dx * delta->i;

        if (b >= 0.0f && b != 0.0f) {
            float a = delta->i * delta->i + delta->k * delta->k + delta->j * delta->j;
            float disc = b * b - a * c;
            if (0.0f <= disc) {
                float t = (b - (float)halo::libm::sqrt((double)disc)) / a;

                if (t <= 1.0f) {
                    *out_t = t;
                    return 1;
                }
            }
        }
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Implements physics shape sphere test point.
 *
 * Original register convention: EAX -> point, ECX -> sphere, EDX -> out_normal, stack -> out_depth.
 *
 * @address 0x503ec0
 */
uint8_t PhysicsModelOps::shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere, real_plane3d *out_normal, float *out_depth)
{
    float dx = point->x - sphere->center_x;
    float dy = point->y - sphere->center_y;
    float dz = point->z - sphere->center_z;
    float dist_sq = dx * dx + dy * dy + dz * dz;

    if (dist_sq < sphere->radius * sphere->radius) {
        float dist = (float)halo::libm::sqrt((double)dist_sq);

        if (dist <= 0.0f) {
            out_normal->normal.i = 0.0f;
            out_normal->normal.j = 0.0f;
            out_normal->normal.k = 1.0f;
        } else {
            float inv = 1.0f / dist;
            out_normal->normal.i = dx * inv;
            out_normal->normal.j = dy * inv;
            out_normal->normal.k = dz * inv;
        }
        out_normal->d = sphere->center_x * out_normal->normal.i +
                         sphere->center_y * out_normal->normal.j +
                         sphere->center_z * out_normal->normal.k + sphere->radius;
        *out_depth = sphere->radius - dist;
        return 1;
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Tests a world-space ray (origin, delta) against sphere, the ray counterpart of
 * physics_shape_sphere_test_point. When origin already starts inside (or exactly on) the
 * sphere, reports an immediate hit at t = 0; otherwise solves the ray/sphere quadratic and
 * reports the near root when it falls within [0, 1] of delta. On a hit, out_plane is the
 * outward-facing separating plane at the hit point.
 *
 * Original register convention: EAX -> origin, ECX -> delta, ESI -> sphere, EDI -> out_plane, stack -> out_t.
 *
 * @address 0x504430
 */
uint8_t PhysicsModelOps::shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta, physics_model_sphere *sphere, real_plane3d *out_plane, float *out_t)
{
    float dx = sphere->center_x - origin->x;
    float dy = sphere->center_y - origin->y;
    float dz = sphere->center_z - origin->z;
    float c = (dx * dx + dz * dz + dy * dy) - sphere->radius * sphere->radius;
    float t;
    real length;

    if (c <= 0.0f) {
        t = 0.0f;
    } else {
        float b = dz * delta->k + dy * delta->j + dx * delta->i;

        if (b <= 0.0f) {
            return 0;
        }

        {
            float delta_len_sq = delta->i * delta->i + delta->k * delta->k + delta->j * delta->j;
            float disc = b * b - delta_len_sq * c;

            if (disc < 0.0f) {
                return 0;
            }
            t = b - (float)halo::libm::sqrt((double)disc);
            if (t > delta_len_sq) {
                return 0;
            }
            t = t / delta_len_sq;
        }
    }

    *out_t = t;
    out_plane->normal.i = t * delta->i - dx;
    out_plane->normal.j = t * delta->j - dy;
    out_plane->normal.k = t * delta->k - dz;
    length = halo::math::vector3d_normalize_with_length(out_plane->normal);
    if (length == 0.0f) {
        out_plane->normal.i = 0.0f;
        out_plane->normal.j = 0.0f;
        out_plane->normal.k = 1.0f;
    }
    out_plane->d = sphere->center_x * out_plane->normal.i + sphere->center_y * out_plane->normal.j +
                    sphere->center_z * out_plane->normal.k + sphere->radius;
    return 1;
}

}

namespace halo::physics {

/**
 * surface_index, surface_flags, breakable_surface_index, material_type, model
 *
 * Original register convention: stack -> vertex_count, vertices, plane, margin, thickness, object_index,.
 *
 * @address 0x5038a0
 */
void PhysicsModelOps::shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices, real_plane3d *plane, float margin, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type, physics_model *model)
{
    if (model->shape_count < 0x100) {
        physics_model_shape *shape = &model->shapes[model->shape_count];
        model->shape_count += 1;

        shape->object_index = object_index;
        shape->surface_index = surface_index;
        shape->surface_flags = surface_flags;
        shape->breakable_surface_index = breakable_surface_index;
        shape->material_type = material_type;
        shape->plane_i = plane->normal.i;
        shape->plane_j = plane->normal.j;
        shape->plane_k = plane->normal.k;
        shape->plane_d = plane->d;
        shape->thickness = thickness;

        if (((float)halo::libm::fabs((double)shape->plane_k) < (float)halo::libm::fabs((double)shape->plane_j)) ||
            ((float)halo::libm::fabs((double)shape->plane_k) < (float)halo::libm::fabs((double)shape->plane_i))) {
            shape->projection_axis =
                ((float)halo::libm::fabs((double)shape->plane_j) < (float)halo::libm::fabs((double)shape->plane_i)) ? 0
                                                                                              : 1;
        } else {
            shape->projection_axis = 2;
        }
        shape->projection_sign = 0.0f < ((float *)&shape->plane_i)[shape->projection_axis];

        shape->vertex_count = vertex_count;
        if (0 < vertex_count) {
            projection_axis_pair proj =
                halo::math::globals().k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
            int16_t i;
            for (i = 0; i < shape->vertex_count; i++) {
                shape->vertices[i][0] = ((float *)&vertices[i])[proj.i];
                shape->vertices[i][1] = ((float *)&vertices[i])[proj.j];
            }
        }

        if ((0.0f < margin) && (plane->normal.k < 0.0f)) {
            shape->plane_d = shape->plane_d - margin * shape->plane_k;
            if (shape->projection_axis != 2) {
                projection_axis_pair proj =
                    halo::math::globals().k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
                int16_t component = (proj.j == 2) ? 1 : 0;
                int16_t i;
                for (i = 0; i < shape->vertex_count; i++) {
                    shape->vertices[i][component] -= margin;
                }
            }
        }
    }
}

}

namespace halo::physics {

/**
 * Implements physics shape test point.
 *
 * Original register convention: stack -> model, point, out_contact.
 *
 * @address 0x504260
 */
uint32_t PhysicsModelOps::shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact)
{
    int32_t best_type = -1;
    int32_t best_index = -1;
    float best_depth = -3.4028235e+38f;
    real_plane3d best_normal;
    int32_t type;

    for (type = 0; type < 3; type++) {
        int16_t count = ((int16_t *)model)[type];
        int32_t i;
        for (i = 0; i < count; i++) {
            float depth;
            real_plane3d normal;
            uint32_t hit;

            if (type == 0) {
                hit = halo::physics::physics_shape_sphere_test_point(point, &model->spheres[i], &normal, &depth);
            } else if (type == 1) {
                hit = halo::physics::physics_shape_pill_test_point(point, &model->pills[i], &normal, &depth);
            } else {
                hit = halo::physics::physics_shape_polygon_test_point(&model->shapes[i], point, &depth, &normal);
            }

            if (hit && (best_depth < depth)) {
                best_depth = depth;
                best_index = i;
                best_type = type;
                best_normal = normal;
            }
        }
    }

    if (best_type == -1) {
        return 0;
    }

    out_contact->t = best_depth;
    out_contact->plane_i = best_normal.normal.i;
    out_contact->plane_j = best_normal.normal.j;
    out_contact->plane_k = best_normal.normal.k;
    out_contact->plane_d = best_normal.d;

    if (best_type == 0) {
        physics_model_sphere *sphere = &model->spheres[best_index];
        out_contact->object_index = sphere->object_index;
        out_contact->surface_index = sphere->surface_index;
        out_contact->surface_flags = sphere->surface_flags;
        out_contact->breakable_surface_index = sphere->breakable_surface_index;
        out_contact->material_type = sphere->material_type;
    } else if (best_type == 1) {
        physics_model_pill *pill = &model->pills[best_index];
        out_contact->object_index = pill->object_index;
        out_contact->surface_index = pill->surface_index;
        out_contact->surface_flags = pill->surface_flags;
        out_contact->breakable_surface_index = pill->breakable_surface_index;
        out_contact->material_type = pill->material_type;
    } else {
        physics_model_shape *shape = &model->shapes[best_index];
        out_contact->object_index = shape->object_index;
        out_contact->surface_index = shape->surface_index;
        out_contact->surface_flags = shape->surface_flags;
        out_contact->breakable_surface_index = shape->breakable_surface_index;
        out_contact->material_type = shape->material_type;
    }
    return 1;
}

}

namespace halo::physics {

/**
 * Tests a world-space ray (origin, delta) against every sphere, pill and polygon shape in
 * model, keeping the closest hit whose surface faces back against the ray (normal . delta <
 * -0.0001, rejecting glancing/backfacing hits). Always fills out_contact's point; on a miss it
 * is origin + delta with t = 1 and the function returns 0, matching
 * physics_shape_test_point's sibling behaviour for the no-hit case.
 *
 * Original register convention: stack -> model, origin, delta, out_contact.
 *
 * @address 0x504bb0
 */
uint32_t PhysicsModelOps::shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta, physics_model_contact *out_contact)
{
    int32_t best_type = -1;
    int32_t best_index = -1;
    float best_t = 3.4028235e+38f;
    real_plane3d best_normal;
    int32_t type;

    for (type = 0; type < 3; type++) {
        int16_t count = ((int16_t *)model)[type];
        int32_t i;

        for (i = 0; i < count; i++) {
            float t;
            real_plane3d normal;
            uint32_t hit;

            if (type == 0) {
                hit = halo::physics::physics_shape_sphere_test_ray(origin, delta, &model->spheres[i], &normal, &t);
            } else if (type == 1) {
                hit = halo::physics::physics_shape_pill_test_ray(delta, origin, &normal, &model->pills[i], &t);
            } else {
                hit = halo::physics::physics_shape_polygon_test_ray(origin, &model->shapes[i], delta, &t, &normal);
            }

            if (hit && t < best_t &&
                (normal.normal.i * delta->i + normal.normal.k * delta->k +
                 normal.normal.j * delta->j) < -0.0001f) {
                best_t = t;
                best_type = type;
                best_index = i;
                best_normal = normal;
            }
        }
    }

    if (best_type == -1) {
        out_contact->t = 1.0f;
        out_contact->point_x = origin->x + delta->i;
        out_contact->point_y = origin->y + delta->j;
        out_contact->point_z = origin->z + delta->k;
        return 0;
    }

    out_contact->t = best_t;
    out_contact->point_x = best_t * delta->i + origin->x;
    out_contact->point_y = best_t * delta->j + origin->y;
    out_contact->point_z = best_t * delta->k + origin->z;
    out_contact->plane_i = best_normal.normal.i;
    out_contact->plane_j = best_normal.normal.j;
    out_contact->plane_k = best_normal.normal.k;
    out_contact->plane_d = best_normal.d;

    if (best_type == 0) {
        physics_model_sphere *sphere = &model->spheres[best_index];
        out_contact->object_index = sphere->object_index;
        out_contact->surface_index = sphere->surface_index;
        out_contact->surface_flags = sphere->surface_flags;
        out_contact->breakable_surface_index = sphere->breakable_surface_index;
        out_contact->material_type = sphere->material_type;
    } else if (best_type == 1) {
        physics_model_pill *pill = &model->pills[best_index];
        out_contact->object_index = pill->object_index;
        out_contact->surface_index = pill->surface_index;
        out_contact->surface_flags = pill->surface_flags;
        out_contact->breakable_surface_index = pill->breakable_surface_index;
        out_contact->material_type = pill->material_type;
    } else {
        physics_model_shape *shape = &model->shapes[best_index];
        out_contact->object_index = shape->object_index;
        out_contact->surface_index = shape->surface_index;
        out_contact->surface_flags = shape->surface_flags;
        out_contact->breakable_surface_index = shape->breakable_surface_index;
        out_contact->material_type = shape->material_type;
    }
    return 1;
}

}

namespace halo::physics {

/**
 * stack -> height_offset, radius, object_index, surface_index, surface_flags,
 * breakable_surface_index
 *
 * Original register convention: ECX -> model, ESI -> vertex, DI -> material_type,.
 *
 * @address 0x503360
 */
void PhysicsModelOps::shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index)
{
    float lowered_z;

    if (model->sphere_count < 0x100) {
        physics_model_sphere *sphere = &model->spheres[model->sphere_count];
        model->sphere_count += 1;
        sphere->object_index = object_index;
        sphere->surface_flags = surface_flags;
        sphere->breakable_surface_index = breakable_surface_index;
        sphere->surface_index = surface_index;
        sphere->material_type = material_type;
        sphere->center_x = vertex->x;
        sphere->center_y = vertex->y;
        sphere->center_z = vertex->z;
        sphere->radius = radius;
    }

    if (!(0.0f < height_offset)) {
        return;
    }
    lowered_z = vertex->z - height_offset;

    if (model->sphere_count < 0x100) {
        physics_model_sphere *sphere = &model->spheres[model->sphere_count];
        float x, y;
        model->sphere_count += 1;
        sphere->object_index = object_index;
        sphere->surface_flags = surface_flags;
        sphere->surface_index = surface_index;
        sphere->breakable_surface_index = breakable_surface_index;
        sphere->material_type = material_type;
        x = vertex->x;
        y = vertex->y;
        sphere->center_x = x;
        sphere->center_y = y;
        sphere->center_z = lowered_z;
        sphere->radius = radius;
    }
    if (model->pill_count < 0x100) {
        physics_model_pill *pill = &model->pills[model->pill_count];
        float x, y;
        model->pill_count += 1;
        pill->object_index = object_index;
        pill->surface_index = surface_index;
        pill->surface_flags = surface_flags;
        pill->breakable_surface_index = breakable_surface_index;
        pill->material_type = material_type;
        x = vertex->x;
        y = vertex->y;
        pill->origin_x = x;
        pill->origin_y = y;
        pill->origin_z = lowered_z;
        pill->extent_i = 0.0f;
        pill->extent_j = 0.0f;
        pill->extent_k = height_offset;
        pill->radius = radius;
    }
}

}

namespace halo::physics {

/**
 * stack -> flags, pill_height, pill_radius, out_position, max_contacts, contacts
 * Moves a pill of the given height and radius from origin by delta through the world,
 * sliding along what it touches; returns how many contacts were recorded.
 *
 * Original register convention: EDI -> origin, ESI -> delta, EBX -> out_velocity, ECX -> exclude_object_index,.
 *
 * @address 0x506fb0
 */
int16_t PhysicsModelOps::sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity, uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius, real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts)
{
    physics_model model;
    real_point3d center;
    float radius;

    center.x = delta->i * 0.5f + origin->x;
    center.y = delta->j * 0.5f + origin->y;
    center.z = delta->k * 0.5f + origin->z + pill_height * 0.5f;
    radius = (float)halo::libm::sqrt((double)(delta->i * delta->i + delta->j * delta->j + delta->k * delta->k)) * 0.5f +
             pill_height * 0.5f + pill_radius;

    if (halo::physics::physics_model_build_from_sphere_query(flags, &center, radius, pill_height, pill_radius,
                                              exclude_object_index, &model)) {
        return halo::physics::physics_model_slide_along_contacts(origin, delta, &model, out_position, out_velocity,
                                                  max_contacts, contacts);
    }
    out_position->x = origin->x + delta->i;
    out_position->y = origin->y + delta->j;
    out_position->z = origin->z + delta->k;
    *out_velocity = *delta;
    return 0;
}

}
