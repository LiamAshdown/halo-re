/**
 * World-level movement tests: structure BSP, nearby objects and water surfaces.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "physics.h"
#include "projectiles.h"
#include "cache.h"

#include "halo/physics/collision_world.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"

extern "C" { void halo::physics::collision_gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model); }
extern "C" { uint8_t halo::physics::collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); }
extern "C" { uint8_t halo::physics::object_collision_context_build(uint32_t object_index, object_collision_context *out_context); }
extern "C" { uint8_t halo::physics::object_collision_context_gather_sphere_shapes(object_collision_context *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model); }
extern "C" { uint32_t halo::physics::object_collision_context_test_point(object_collision_context *context, real_point3d *point); }
extern "C" { uint8_t halo::physics::object_collision_context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result); }
extern "C" { uint8_t halo::physics::object_collision_test_nearby_chain(uint32_t start_object_index, uint32_t type_mask, real_point3d *position, uint32_t exclude_object_index); }
extern "C" { uint8_t halo::physics::object_collision_test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask, uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *out_result); }

extern "C" { extern data_array *object_data; }
extern "C" { extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out); }
namespace halo::physics {

/**
 * Walks start_object_index and its next_object siblings (recursing into each one's children)
 * and adds physics_model proxies for every object whose bounding sphere reaches the query
 * sphere (origin, radius), whose type bit (1 << (type + 8)) is set in flags, and which is not
 * exclude_object_index, not no-collision (flags bit 0), not flagged 0x1000000 and not a dead
 * (health-frozen) biped. The type table 0x506434 maps bipeds to one sphere/pill proxy, vehicles,
 * scenery and both device kinds to their collision-node shapes (or, for vehicles with flags
 * bit 0x400000, their mass points), and weapons, equipment, garbage and projectiles to nothing.
 *
 * @address 0x5061c0
 */
void CollisionWorld::gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model)
{
    uint32_t object_index = start_object_index;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        float reach;
        float dx, dy, dz;

        if (object_index == exclude_object_index ||
            (obj->flags & _object_no_collision_bit) != 0 ||
            (obj->flags & 0x01000000) != 0 ||
            ((obj->vitality_flags & _object_health_frozen_bit) != 0 && obj->type == _object_type_biped)) {
            object_index = obj->next_object;
            continue;
        }
        reach = radius + obj->bounding_radius;
        dx = obj->bounding_center.x - origin->x;
        dy = obj->bounding_center.y - origin->y;
        dz = obj->bounding_center.z - origin->z;
        if (!(reach * reach >= dx * dx + dy * dy + dz * dz)) {
            object_index = obj->next_object;
            continue;
        }

        if ((flags & (1u << ((obj->type + 8) & 0x1f))) != 0 && (uint32_t)obj->type <= 8) {
            switch (obj->type) {
            case _object_type_biped: {
                biped_data *biped = (biped_data *)((uint8_t *)obj + 0x4cc);
                unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

                if (((flags & 0x200000) == 0 || (biped->flags & 0x10) == 0) &&
                    (obj->parent_object == k_datum_index_none || unit->vehicle_seat_index == -1)) {
                    real_point3d position;
                    float pill_height;
                    float pill_radius;

                    unit_get_crouch_height_offset(&position, object_index, &pill_height, &pill_radius);
                    position.z += pill_height;
                    halo::physics::physics_shape_vertex_to_sphere(model, &position, -1, pill_height + x_offset,
                        pill_radius + y_offset, object_index, -1, 0, -1);
                }
                break;
            }
            case _object_type_vehicle:
            case _object_type_scenery:
            case _object_type_device_machine:
            case _object_type_device_control:
                if (obj->type == _object_type_vehicle && (flags & 0x400000) != 0) {
                    object_physics_context physics_ctx;
                    if (halo::physics::object_physics_context_build(object_index, &physics_ctx)) {
                        halo::physics::object_physics_add_mass_point_shapes(x_offset, y_offset, &physics_ctx, (int16_t *)model);
                    }
                } else {
                    object_collision_context node_ctx;
                    if (halo::physics::object_collision_context_build(object_index, &node_ctx)) {
                        halo::physics::object_collision_context_gather_sphere_shapes(&node_ctx, origin, radius, x_offset,
                                                                      y_offset, model);
                    }
                }
                break;
            default:
                break;
            }
        }

        if (obj->first_child_object != k_datum_index_none) {
            halo::physics::collision_gather_nearby_object_shapes(flags, obj->first_child_object, origin,
                radius, x_offset, y_offset, exclude_object_index, model);
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);
}

}

extern "C" { extern ModelCollisionGeometryBSP *global_structure_collision_bsp; }
extern "C" { extern ScenarioStructureBSP *global_structure_bsp; }
extern "C" { extern ModelCollisionGeometryBSP *global_collision_bsp; }
static int16_t pill_leaf_cluster(int32_t leaf)
{
    if (leaf == -1) {
        return -1;
    }
    return (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
}

namespace halo::physics {

/**
 * REWRITTEN from objdump 0x506040..0x5061b2. Stack: (flags, origin, radius); EDI: delta; ESI: result. Sweeps a pill
 * of the radius from origin along delta through the structure BSP (0x502730, max fraction FLT_MAX). With flags
 * 0x20 a contact becomes a BSP hit (type 2): plane +0x24, material +0x34 / +0x4e, surface +0x44, plane index -1.
 * The first / last touched leaves (and clusters) go to +0x4 / +0xc; t is 1 without a hit; the end point (+0x18)
 * and its leaf (+0xc, cluster +0x10) follow. The draft had no radius, called the query with 2 of 6 operands and
 * stored the last leaf over the first.
 *
 * Original register convention: stack -> flags, origin, radius; EDI -> delta; ESI -> result.
 *
 * @address 0x506040
 */
uint8_t CollisionWorld::test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta, collision_result *result)
{
    collision_result *r = result;
    collision_bsp_pill_result pill;
    uint8_t hit = 0;
    int32_t leaf;
    uint32_t flt_max_bits = 0x7f7fffff;

    r->type = -1;
    *(uint32_t *)&r->t = 0x7f7fffff;
    if (halo::physics::collision_bsp_query_pill_init(global_structure_collision_bsp, &pill, origin, delta, radius,
            *(float *)&flt_max_bits)) {
        r->t = pill.t;
        if (flags & 0x20) {
            r->plane.normal.i = pill.plane_i;
            r->plane.normal.j = pill.plane_j;
            r->plane.normal.k = pill.plane_k;
            r->plane.d = pill.plane_d;
            r->surface_flags = 0;
            r->breakable_surface_index = 0;
            r->type = 2;
            r->material_type = pill.material_index;
            r->surface_index = pill.surface_index;
            r->plane_index = -1;
            r->collision_material_index = pill.material_index;
            hit = 1;
        }
    }
    if (pill.leaf_count > 0) {
        r->first_leaf = pill.leaves[0];
        r->first_cluster = pill_leaf_cluster(pill.leaves[0]);
        leaf = pill.leaves[pill.leaf_count - 1];
        r->leaf.leaf_index = leaf;
        r->leaf.cluster_index = pill_leaf_cluster(leaf);
    }
    if (!hit) {
        r->t = 1.0f;
    }
    {
        float t = r->t;
        real_point3d *point = &r->point;

        point->x = t * delta->i + origin->x;
        point->y = t * delta->j + origin->y;
        point->z = t * delta->k + origin->z;
        leaf = (int32_t)halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, point);
    }
    r->leaf.leaf_index = leaf;
    r->leaf.cluster_index = pill_leaf_cluster(leaf);
    return hit;
}

}

extern "C" { extern object_globals *object_globals_pointer; }
extern "C" { extern int32_t object_cluster_stamp; }
extern "C" { extern datum_index *collideable_cluster_first; }
extern "C" { extern data_array *collideable_object_references; }
extern "C" { extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result, ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin, real_vector3d *delta, float max_fraction); }
extern "C" { extern void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point); }
extern "C" { extern breakable_surface_globals *breakable_surface_state; }
extern "C" { extern int16_t global_structure_bsp_index; }
extern "C" { extern double fabs(double x); }
namespace halo::physics {

/**
 * Casts a movement segment (origin, origin+delta) through the world and writes the collision it
 * found (if any) into *result. With none of _collision_test_flag_structure_bsp/_water_surface/
 * _nearby_objects set, this only resolves the endpoint's leaf and returns false. Otherwise it
 * always runs the structure-BSP segment query (to learn every leaf the segment passed through),
 * applies that hit to *result when _collision_test_flag_structure_bsp is set, tests the
 * destination cluster's fog plane for a water-surface crossing when _collision_test_flag_water_surface
 * is set, and walks every object in every touched cluster (excluding exclude_object_index) when
 * _collision_test_flag_nearby_objects is set. Finally, when _collision_test_flag_unstick is set and
 *
 * @address 0x505880
 */
uint8_t CollisionWorld::test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result)
{
    uint8_t hit;
    real_point3d *point;
    bsp_leaf_reference *first_leaf_ref;
    bsp_leaf_reference *last_leaf_ref;
    int32_t leaf_index;

    hit = 0;

    first_leaf_ref = (bsp_leaf_reference *)&result->first_leaf;
    last_leaf_ref = (bsp_leaf_reference *)&result->leaf;
    result->type = -1;
    first_leaf_ref->leaf_index = -1;
    first_leaf_ref->cluster_index = -1;
    last_leaf_ref->leaf_index = -1;
    last_leaf_ref->cluster_index = -1;
    result->t = 1.0f;

    if ((flags & (_collision_test_flag_structure_bsp | _collision_test_flag_water_surface |
                  _collision_test_flag_nearby_objects)) == 0) {
        result->t = 1.0f;
        result->point.x = origin->x + delta->i;
        result->point.y = origin->y + delta->j;
        result->point.z = origin->z + delta->k;
        leaf_index = halo::physics::bsp3d_node_find_leaf(0, global_structure_collision_bsp, &result->point);
        last_leaf_ref->leaf_index = leaf_index;
        if (leaf_index == -1) {
            last_leaf_ref->cluster_index = -1;
            return 0;
        }

        last_leaf_ref->cluster_index =
            ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index & 0x7fffffff].cluster;
        return 0;
    }

    {
        uint32_t object_type_mask = flags >> 7;
        uint32_t sanitized_flags = flags;
        uint32_t segment_flags;
        collision_bsp_segment_result seg_result;
        uint32_t found_surface;

        if ((sanitized_flags & 3) == 0) {
            sanitized_flags |= 3;
        }
        segment_flags = sanitized_flags & 0x1f;

        found_surface = halo::physics::collision_bsp_query_segment_init(segment_flags, &seg_result,
            global_structure_collision_bsp, k_maximum_breakable_surfaces_per_bsp,
            breakable_surface_state->active[global_structure_bsp_index],
            origin, delta, 3.4028235e+38f );

        if (found_surface && (flags & _collision_test_flag_structure_bsp) != 0) {
            result->t = seg_result.t;
            result->type = 2;
            result->plane.normal.i = ((real_plane3d *)seg_result.plane)->normal.i;
            result->plane.normal.j = ((real_plane3d *)seg_result.plane)->normal.j;
            result->plane.normal.k = ((real_plane3d *)seg_result.plane)->normal.k;
            result->plane.d = ((real_plane3d *)seg_result.plane)->d;
            if (seg_result.plane_index < 0) {
                result->plane.normal.i = -result->plane.normal.i;
                result->plane.normal.j = -result->plane.normal.j;
                result->plane.normal.k = -result->plane.normal.k;
                result->plane.d = -result->plane.d;
            }
            if (seg_result.material_index == -1) {
                result->material_type = -1;
            } else {
                result->material_type = ((ScenarioStructureBSPCollisionMaterial *)
                    global_structure_bsp->collision_materials.pointer)[seg_result.material_index].material;
            }
            result->surface_index = seg_result.surface_index;
            result->plane_index = (uint32_t)seg_result.plane_index;

            result->surface_flags = seg_result.surface_flags;
            result->breakable_surface_index = seg_result.breakable_surface_index;
            result->collision_material_index = seg_result.material_index;
            hit = 1;
        }

        if (seg_result.leaf_count > 0) {
            int32_t first_leaf = seg_result.leaves[0];
            int32_t last_leaf = seg_result.leaves[seg_result.leaf_count - 1];

            first_leaf_ref->leaf_index = first_leaf;
            first_leaf_ref->cluster_index = (first_leaf == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[first_leaf & 0x7fffffff].cluster;

            last_leaf_ref->leaf_index = last_leaf;
            last_leaf_ref->cluster_index = (last_leaf == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[last_leaf & 0x7fffffff].cluster;
        }

        if ((flags & _collision_test_flag_water_surface) != 0 && last_leaf_ref->cluster_index != -1) {
            ScenarioStructureBSPCluster *cluster = &((ScenarioStructureBSPCluster *)
                global_structure_bsp->clusters.pointer)[last_leaf_ref->cluster_index];
            int16_t fog = (int16_t)cluster->fog;
            if (fog != -1 && fog < 0) {
                ScenarioStructureBSPFogPlane *fog_plane = &((ScenarioStructureBSPFogPlane *)
                    global_structure_bsp->fog_planes.pointer)[fog & 0x7fff];
                if (fog_plane->material_type != -1) {
                    float ni = fog_plane->plane.vector.i;
                    float nj = fog_plane->plane.vector.j;
                    float nk = fog_plane->plane.vector.k;

                    uint16_t fog_palette_index = ((ScenarioStructureBSPFogRegion *)
                        global_structure_bsp->fog_regions.pointer)[fog_plane->front_region].fog;
                    ScenarioStructureBSPFogPalette *palette = &((ScenarioStructureBSPFogPalette *)
                        global_structure_bsp->fog_palette.pointer)[fog_palette_index];
                    uint32_t fog_tag_index = palette->fog.tag_id.index;
                    float world_offset = *(float *)((uint8_t *)halo::cache::globals().tag_instances[fog_tag_index].data + 0x74);
                    float d = fog_plane->plane.w - world_offset;
                    float side_a = (ni * origin->x + nk * origin->z + nj * origin->y) - d;
                    float side_b = ni * delta->i + nk * delta->k + nj * delta->j;
                    if ((0.0f < side_a) != (0.0f < side_b) &&
                        (float)fabs((double)side_a) < (float)fabs((double)side_b) &&
                        0.0001f <= (float)fabs((double)side_b) &&
                        -(side_a / side_b) < result->t) {
                        result->t = -(side_a / side_b);
                        result->plane.normal.i = ni;
                        result->plane.normal.j = nj;
                        result->plane.normal.k = nk;
                        result->type = 0;
                        result->plane.d = d;
                        if (0.0f <= side_a) {
                            result->material_type = fog_plane->material_type;
                            hit = 1;
                        } else {
                            real_plane3d negated;
                            halo::math::plane3d_negate(negated, *((real_plane3d *)&result->plane.normal));
                            result->plane.normal.i = negated.normal.i;
                            result->plane.normal.j = negated.normal.j;
                            result->plane.normal.k = negated.normal.k;
                            result->plane.d = negated.d;
                            result->material_type = 0x1c;
                            hit = 1;
                        }
                    }
                }
            }
        }

        if ((object_type_mask & 1) != 0 && seg_result.leaf_count > 0) {
            int32_t stamp;
            int32_t i;

            if ((flags & _collision_test_object_type_mask_default) == 0) {
                flags |= _collision_test_object_type_mask_default;
            }
            halo::structures::globals().cluster_flood_stamp++;
            object_globals_pointer->collecting_in_clusters = 1;
            stamp = object_cluster_stamp + 1;
            halo::structures::globals().cluster_flood_in_progress = 1;
            object_cluster_stamp = stamp;

            for (i = 0; i < seg_result.leaf_count; i++) {
                int32_t leaf = seg_result.leaves[i];
                int16_t cluster_index = (leaf == -1) ? -1 :
                    ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;

                if (halo::structures::globals().cluster_visit_stamp[cluster_index] != halo::structures::globals().cluster_flood_stamp) {
                    datum_index ref;

                    halo::structures::globals().cluster_visit_stamp[cluster_index] = halo::structures::globals().cluster_flood_stamp;
                    ref = collideable_cluster_first[cluster_index];
                    while (ref != k_datum_index_none) {
                        object_cluster_reference *node = (object_cluster_reference *)
                            collideable_object_references->data + (ref & 0xffff);
                        datum_index object_index = node->object_index;
                        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

                        if (obj->cluster_stamp != stamp) {
                            obj->cluster_stamp = stamp;
                            if (halo::physics::object_collision_test_ray_nearby_chain(object_index, flags, segment_flags, origin, delta,
                                    exclude_object_index, result) != 0) {
                                hit = 1;
                            }
                        }
                        ref = node->next_reference;
                    }
                }
            }

            object_globals_pointer->collecting_in_clusters = 0;
            halo::structures::globals().cluster_flood_in_progress = 0;
        }

        if (hit == 0) {
            result->t = 1.0f;
        }
        point = &result->point;
        point->x = result->t * delta->i + origin->x;
        point->y = result->t * delta->j + origin->y;
        point->z = result->t * delta->k + origin->z;

        if ((flags & _collision_test_flag_unstick) != 0 && hit != 0) {
            int32_t resolved_leaf = last_leaf_ref->leaf_index;

            if (resolved_leaf != -1) {
                int32_t new_leaf = halo::physics::bsp3d_node_find_leaf(0, global_structure_collision_bsp, point);
                if (new_leaf != resolved_leaf) {
                    point->x += result->plane.normal.i * 0.00024414062f;
                    point->y += result->plane.normal.j * 0.00024414062f;
                    point->z += result->plane.normal.k * 0.00024414062f;
                    scenario_location_from_point((bsp_leaf_reference *)last_leaf_ref, point);
                    if (last_leaf_ref->leaf_index == -1) {
                        float facing = delta->i * result->plane.normal.i + delta->j * result->plane.normal.j +
                            delta->k * result->plane.normal.k;
                        float step = (facing == 0.0f) ? 0.03125f :
                            0.00024414062f / (float)fabs((double)facing);

                        while (1) {
                            float t = result->t - step;
                            if (t <= 0.0f) {
                                t = 0.0f;
                            }
                            result->t = t;
                            point->x = t * delta->i + origin->x;
                            point->y = t * delta->j + origin->y;
                            point->z = t * delta->k + origin->z;
                            resolved_leaf = halo::physics::bsp3d_node_find_leaf(0, global_structure_collision_bsp, point);
                            last_leaf_ref->leaf_index = resolved_leaf;
                            last_leaf_ref->cluster_index = (resolved_leaf == -1) ? -1 :
                                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[resolved_leaf & 0x7fffffff].cluster;
                            if (result->t <= 0.0f) {
                                break;
                            }
                            if (last_leaf_ref->leaf_index != -1) {
                                return hit;
                            }
                        }
                    }
                }
            }
        }
    }

    return hit;
}

}

namespace halo::physics {

/**
 * Sweeps a movement segment between two explicit endpoints: builds delta = target - origin and
 * forwards to collision_test_movement_segment, which does the actual BSP/object collision test.
 *
 * @address 0x401a20
 */
uint8_t CollisionWorld::test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result)
{
    real_vector3d delta;

    delta.i = target->x - origin->x;
    delta.j = target->y - origin->y;
    delta.k = target->z - origin->z;

    return halo::physics::collision_test_movement_segment(flags, origin, &delta, exclude_object_index, result);
}

}

namespace halo::physics {

/**
 * Builds an object_collision_context for object_index. Fails (returns 0) when the Object tag has
 * no collision_model reference (TagID at Object+0x7c invalid); otherwise fills object_index, the
 * ModelCollisionGeometry definition, the per-region active-permutation byte array
 * (object+0x180) and the node matrix array (object + the int16 "nodes offset" at object+0x1f2).
 *
 * Original register convention: EDI -> object_index, ECX -> out_context.
 *
 * @address 0x504e10
 */
uint8_t CollisionWorld::context_build(uint32_t object_index, object_collision_context *out_context)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if (object_tag->collision_model.tag_id.index != 0xffff ||
        object_tag->collision_model.tag_id.id != 0xffff) {
        out_context->object_index = object_index;
        out_context->definition =
            halo::cache::globals().tag_instances[object_tag->collision_model.tag_id.index & 0xffff].data;
        out_context->region_permutations = (uint8_t *)obj + 0x180;
        out_context->nodes = (uint8_t *)obj + ((object *)obj)->nodes.offset;
        return 1;
    }
    return 0;
}

}

extern "C" { extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius); }
extern "C" { extern void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model); }
namespace halo::physics {

/**
 * Tests a world-space sphere (origin, radius_scale) against every collision node of context's
 * object, one node at a time, building physics_model proxies (via
 * physics_shape_build_proxies_from_query) for every node whose active BSP permutation the
 * sphere actually touches. Unlike the segment/pill node tests this never narrows a shared
 * fraction -- every touched node contributes its own proxies to model.
 *
 * Original register convention: stack -> context, origin, radius_scale, margin, thickness, model.
 *
 * @address 0x505200
 */
uint8_t CollisionWorld::context_gather_sphere_shapes(object_collision_context *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model)
{
    ModelCollisionGeometry *definition = (ModelCollisionGeometry *)context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation_byte = context->region_permutations[(int16_t)node->region];

            if ((int32_t)node->bsps.count > 0) {
                int32_t permutation = permutation_byte;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (permutation > (int32_t)node->bsps.count - 1) {
                    permutation = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[permutation];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_matrix4x3 inverse_matrix;
                    real_point3d local_center;
                    collision_bsp_sphere_result sphere_result;

                    halo::math::matrix4x3_inverse(&inverse_matrix, *(&((real_matrix4x3 *)context->nodes)[node_index]));
                    halo::math::matrix4x3_transform_point(local_center, *origin, inverse_matrix);

                    if (halo::physics::collision_bsp_query_sphere_init(bsp, 0, &sphere_result, 0, &local_center,
                                                         inverse_matrix.scale * radius_scale)) {
                        halo::physics::physics_shape_build_proxies_from_query(&sphere_result,
                            &((real_matrix4x3 *)context->nodes)[node_index], bsp, margin, thickness,
                            context->object_index, model);
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

}

namespace halo::physics {

/**
 * Tests a world-space swept sphere (origin, delta, radius * radius_scale) against every
 * collision node of context's object, one node at a time, the pill counterpart of
 * object_collision_context_test_segment. Keeps scanning every node, narrowing the shared
 * fraction on each hit so later nodes only need to beat whatever is left of the sweep; returns
 * whether any node was hit at all.
 *
 * Original register convention: stack -> context, origin, delta, radius_scale, out_result.
 *
 * @address 0x5050b0
 */
uint8_t CollisionWorld::context_test_pill(object_collision_context *context, real_point3d *origin, real_vector3d *delta, float radius_scale, object_node_collision_result *out_result)
{
    ModelCollisionGeometry *definition = (ModelCollisionGeometry *)context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    out_result->segment.t = 3.4028235e+38f;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation_byte = context->region_permutations[node->region];

            if (permutation_byte != 0xff && (int32_t)node->bsps.count > 0) {
                int32_t permutation = permutation_byte;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (permutation > (int32_t)node->bsps.count - 1) {
                    permutation = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[permutation];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_matrix4x3 inverse_matrix;
                    real_point3d local_origin;
                    real_vector3d local_delta;
                    collision_bsp_pill_result pill_result;

                    halo::math::matrix4x3_inverse(&inverse_matrix, *(&((real_matrix4x3 *)context->nodes)[node_index]));
                    halo::math::matrix4x3_transform_point(local_origin, *origin, inverse_matrix);
                    halo::math::matrix4x3_transform_vector(local_delta, *delta, inverse_matrix);

                    if (halo::physics::collision_bsp_query_pill_init(bsp, &pill_result, &local_origin, &local_delta,
                                                       inverse_matrix.scale * radius_scale,
                                                       out_result->segment.t)) {
                        out_result->node_index = (int16_t)node_index;
                        out_result->region_index = (int16_t)node->region;
                        out_result->permutation_index = (int16_t)permutation;
                        out_result->segment.t = pill_result.t;
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

}

namespace halo::physics {

/**
 * Walks every collision node of context's object, transforms point into each node's currently
 * active BSP permutation's local space, and reports whether ANY node resolves to no leaf at
 * all (the bsp3d_node_find_leaf "solid" sentinel, as opposed to an ordinary leaf) -- meaning
 * the point is embedded in that node's collision geometry. Nodes whose region has no active
 * permutation, or whose active BSP is empty, are skipped.
 *
 * Original register convention: EBX -> context, stack -> point (0x504f1b `mov esi,[esp+0x24]`).
 *
 * @address 0x504e90
 */
uint32_t CollisionWorld::context_test_point(object_collision_context *context, real_point3d *point)
{
    ModelCollisionGeometry *definition = (ModelCollisionGeometry *)context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation = context->region_permutations[node->region];

            if (node->bsps.count > 0) {
                int32_t bsp_index = permutation;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (bsp_index > (int32_t)node->bsps.count - 1) {
                    bsp_index = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[bsp_index];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_point3d local_point;

                    halo::math::matrix4x3_inverse_transform_point(*(&((real_matrix4x3 *)context->nodes)[node_index]),
                                                       local_point, *point);
                    if (halo::physics::bsp3d_node_find_leaf(0, bsp, &local_point) == 0xffffffff) {
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
 * Tests a world-space segment (origin, delta) against every collision node of context's object,
 * one node at a time: inverts that node's local-to-world matrix, transforms the segment into
 * the node's local space, and runs it through the node's currently active BSP permutation via
 * collision_bsp_query_segment_init. Keeps scanning every node (never stops early), narrowing
 * out_result->segment.t on each hit so later nodes are only tested against whatever is left of
 * the segment; returns whether any node was hit at all.
 * collision_bsp_query_segment_init in EAX (callers pass 3 or their own type mask; they clean 0x14 bytes). The
 * draft had four, so origin/delta/out_result were read one slot early and the flags were a constant 0.
 *
 * Original register convention: stack -> context, flags, origin, delta, out_result.
 *
 * @address 0x504f60
 */
uint8_t CollisionWorld::context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result)
{
    ModelCollisionGeometry *definition = (ModelCollisionGeometry *)context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    out_result->segment.t = 3.4028235e+38f;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation_byte = context->region_permutations[node->region];

            if ((int32_t)node->bsps.count > 0) {
                int32_t permutation = permutation_byte;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (permutation > (int32_t)node->bsps.count - 1) {
                    permutation = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[permutation];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_matrix4x3 inverse_matrix;
                    real_point3d local_origin;
                    real_vector3d local_delta;

                    halo::math::matrix4x3_inverse(&inverse_matrix, *(&((real_matrix4x3 *)context->nodes)[node_index]));
                    halo::math::matrix4x3_transform_point(local_origin, *origin, inverse_matrix);
                    halo::math::matrix4x3_transform_vector(local_delta, *delta, inverse_matrix);

                    if (halo::physics::collision_bsp_query_segment_init(flags, &out_result->segment, bsp, 0, 0,
                                                          &local_origin, &local_delta,
                                                          out_result->segment.t)) {
                        out_result->node_index = (int16_t)node_index;
                        out_result->region_index = (int16_t)node->region;
                        out_result->permutation_index = (int16_t)permutation;
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

}

extern "C" { extern datum_index object_resolve_collideable_reference(datum_index *next_reference, int16_t cluster_index); }
namespace halo::physics {

/**
 * When flags selects one of the cluster-relative contact tests (bits 5-7), resolves position's
 * containing leaf and, for the "test the leaf's cluster object group" variant (bit 7), walks
 * every object referenced by that cluster via object_collision_test_nearby_chain. Returns 1
 * immediately if position falls outside the BSP entirely (nothing to test against) or if any
 * referenced object reports a hit.
 *
 * Original register convention: EDI -> position, stack -> flags, exclude_object_index.
 *
 * @address 0x505490
 */
uint8_t CollisionWorld::test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index)
{
    if ((flags & 0xe0) != 0) {
        int32_t leaf_index = halo::physics::bsp3d_node_find_leaf(0, global_structure_collision_bsp, position);

        if (leaf_index == -1) {
            return 1;
        }
        if ((flags >> 7 & 1) != 0) {
            ScenarioStructureBSPLeaf *leaves =
                (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
            datum_index next_reference;
            datum_index object_index =
                object_resolve_collideable_reference(&next_reference,
                    (int16_t)*(uint16_t *)((uint8_t *)leaves + (leaf_index & 0x7fffffff) * 0x10 + 8));

            while (object_index != k_datum_index_none) {
                if (halo::physics::object_collision_test_nearby_chain(object_index, flags, position,
                                                         exclude_object_index)) {
                    return 1;
                }
                if (next_reference == k_datum_index_none) {
                    object_index = k_datum_index_none;
                } else {
                    object_cluster_reference *ref =
                        (object_cluster_reference *)collideable_object_references->data +
                        (next_reference & 0xffff);
                    object_index = ref->object_index;
                    next_reference = ref->next_reference;
                }
            }
        }
    }
    return 0;
}

}

namespace halo::physics {

/**
 * Walks the object chain starting at start_object_index (following object.next_object), testing
 * each object matching type_mask that overlaps position's bounding sphere and is not
 * exclude_object_index: vehicles with flag bit 0x400000 set are tested against their
 * physics mass points, every other matching object against its collision-node geometry.
 * Recurses into first_child_object for every object visited, and into next_object for the walk
 * itself. Returns as soon as any object reports a hit.
 *
 * Original register convention: stack -> start_object_index, type_mask, position, exclude_object_index.
 *
 * @address 0x505350
 */
uint8_t CollisionWorld::test_nearby_chain(uint32_t start_object_index, uint32_t type_mask, real_point3d *position, uint32_t exclude_object_index)
{
    uint32_t object_index = start_object_index;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (object_index != exclude_object_index && (obj->flags & 1) == 0) {
            uint8_t type = (uint8_t)obj->type;

            if ((type_mask & (1u << ((type + 8) & 0x1f))) != 0) {
                float dx = obj->bounding_center.x - position->x;
                float dy = obj->bounding_center.y - position->y;
                float dz = obj->bounding_center.z - position->z;

                if (dy * dy + dz * dz + dx * dx <= obj->bounding_radius * obj->bounding_radius) {
                    if (((1 << (type & 0x1f)) & 2) == 0 || (type_mask & 0x400000) == 0) {
                        object_collision_context node_ctx;

                        if (halo::physics::object_collision_context_build(object_index, &node_ctx) &&
                            halo::physics::object_collision_context_test_point(&node_ctx, position)) {
                            return 1;
                        }
                    } else {
                        object_physics_context phys_ctx;
                        int16_t hit_index;

                        if (halo::physics::object_physics_context_build(object_index, &phys_ctx) &&
                            halo::physics::object_physics_test_point_against_mass_points(&phys_ctx, position,
                                                                           &hit_index)) {
                            return 1;
                        }
                    }

                    if (obj->first_child_object != k_datum_index_none) {
                        if (halo::physics::object_collision_test_nearby_chain(obj->first_child_object, type_mask,
                                                                 position, exclude_object_index)) {
                            return 1;
                        }
                    }
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);

    return 0;
}

}

extern "C" { extern int16_t model_collision_geometry_resolve_material_type(int16_t material_index, ModelCollisionGeometry *definition); }
extern "C" { extern uint8_t object_physics_test_ray_against_mass_points(real_point3d *world_origin, real_vector3d *world_direction, object_physics_ray_result *out_result, object_physics_context *context); }
namespace halo::physics {

/**
 * Walks the object chain starting at start_object_index exactly like
 * object_collision_test_nearby_chain, but against a swept ray (origin, delta, radius_scale)
 * instead of a point, keeping the single closest hit across the whole chain in *out_result
 * (which the caller must pre-seed, t = FLT_MAX). Vehicles with flag bit 0x400000 set are tested
 * against their physics mass points; every other matching object against its collision-node
 * segment geometry, whose local-space hit plane is transformed back to world space (and negated
 * on a back-face hit) before being written into *out_result. Recurses into first_child_object
 * for every object visited, and into next_object for the walk itself; returns whether the chain
 *
 * Original register convention: stack -> start_object_index, type_mask, test_flags, origin, delta,.
 *
 * @address 0x5055b0
 */
uint8_t CollisionWorld::test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask, uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *out_result)
{
    uint32_t object_index = start_object_index;
    uint8_t improved = 0;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (object_index != exclude_object_index && (obj->flags & 1) == 0) {
            uint8_t type = (uint8_t)obj->type;

            if ((type_mask & (1u << ((type + 8) & 0x1f))) != 0 &&

                halo::math::ray_intersects_sphere_test(obj->bounding_center, *origin, *delta,
                                            obj->bounding_radius)) {
                if (((1 << (type & 0x1f)) & 2) == 0 || (type_mask & 0x400000) == 0) {
                    object_collision_context node_ctx;

                    if (halo::physics::object_collision_context_build(object_index, &node_ctx)) {
                        object_node_collision_result node_result;

                        if (halo::physics::object_collision_context_test_segment(&node_ctx, test_flags, origin, delta,
                                                                    &node_result) &&
                            node_result.segment.t < out_result->t) {
                            real_matrix4x3 *node_matrix =
                                &((real_matrix4x3 *)node_ctx.nodes)[node_result.node_index];

                            out_result->t = node_result.segment.t;
                            out_result->type = 3;
                            halo::math::matrix4x3_transform_plane(*((real_plane3d *)&out_result->plane.normal),
                                                       *node_matrix,
                                                       *(real_plane3d *)node_result.segment.plane);
                            if (node_result.segment.plane_index < 0) {
                                halo::math::plane3d_negate(*((real_plane3d *)&out_result->plane.normal),
                                               *((real_plane3d *)&out_result->plane.normal));
                            }
                            out_result->material_type = halo::physics::model_collision_geometry_resolve_material_type(
                                node_result.segment.material_index,
                                (ModelCollisionGeometry *)node_ctx.definition);
                            out_result->region_index = node_result.region_index;
                            out_result->node_index = node_result.node_index;
                            out_result->permutation_index = node_result.permutation_index;
                            out_result->plane_index = node_result.segment.plane_index;
                            out_result->object_index = object_index;
                            out_result->surface_index = node_result.segment.surface_index;
                            out_result->surface_flags = node_result.segment.surface_flags;
                            out_result->breakable_surface_index = node_result.segment.breakable_surface_index;
                            out_result->collision_material_index = node_result.segment.material_index;
                            improved = 1;
                        }
                    }
                } else {
                    object_physics_context phys_ctx;

                    if (halo::physics::object_physics_context_build(object_index, &phys_ctx)) {
                        object_physics_ray_result ray_result;

                        if (halo::physics::object_physics_test_ray_against_mass_points(origin, delta, &ray_result,
                                                                          &phys_ctx) &&
                            ray_result.t < out_result->t) {
                            out_result->t = ray_result.t;
                            out_result->plane.normal.i = ray_result.plane_i;
                            out_result->plane.normal.j = ray_result.plane_j;
                            out_result->plane.normal.k = ray_result.plane_k;
                            out_result->plane.d = ray_result.plane_d;
                            out_result->type = 3;
                            out_result->material_type = -1;
                            out_result->object_index = object_index;
                            out_result->region_index = -1;
                            out_result->node_index = -1;
                            out_result->permutation_index = -1;
                            out_result->plane_index = 0xffffffff;
                            out_result->surface_index = -1;
                            out_result->surface_flags = 0;
                            out_result->breakable_surface_index = 0;
                            out_result->collision_material_index = -1;
                            improved = 1;
                        }
                    }
                }

                if (obj->first_child_object != k_datum_index_none) {
                    if (halo::physics::object_collision_test_ray_nearby_chain(obj->first_child_object, type_mask,
                            test_flags, origin, delta, exclude_object_index, out_result)) {
                        improved = 1;
                    }
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);

    return improved;
}

}
