#include "halo/core/collision_flags.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/core/libm.hpp"

static auto &decal_data = halo::link::ref<data_array *>(halo::effects::vars().decal_data);
static auto &decal_grid_block = halo::link::ref<decal_grid *>(halo::effects::vars().decal_grid_block);
static auto &rasterizer_decal_vertex_cache_handle = halo::link::ref<cache *>(halo::effects::vars().rasterizer_decal_vertex_cache_handle);
static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);
static auto &decal_clip_buffers = halo::link::ref<real_point2d [2][12]>(halo::effects::vars().decal_clip_buffers);
static auto &k_decal_type_parameters = halo::link::ref<const decal_type_parameters [4]>(halo::effects::vars().k_decal_type_parameters);
static auto &decals_enabled = halo::link::ref<uint8_t>(halo::effects::vars().decals_enabled);
static auto &decals_for_all_responses = halo::link::ref<uint8_t>(halo::effects::vars().decals_for_all_responses);

namespace halo::effects {

/**
 * Builds a decal_projection: copies the placement matrix and box verbatim, derives the dominant
 * projection axis from the placement's up vector, and flattens the box's four world-space
 * corners onto the two remaining axes, along with the edge gradients and inverse determinant
 * decal_flood_surfaces and decal_place use to turn a surface point into a uv pair.
 *
 * @address 0x44e460
 */
void decal_ref::build_projection(real_matrix4x3 *placement, real *box, decal_projection *out)
{
    real corner[3];
    int major_axis;
    uint8_t normal_positive;
    const projection_axis_pair *axes;

    out->placement = *placement;

    out->plane_i = box[0];
    out->plane_j = box[1];
    out->plane_k = box[2];
    out->plane_d = box[3];

    out->transformed_i = placement->up.i;
    out->transformed_j = placement->up.j;
    out->transformed_k = placement->up.k;
    out->transformed_d = out->transformed_i * placement->position.x +
        out->transformed_j * placement->position.y + out->transformed_k * placement->position.z;

    {
        real ai = out->transformed_i < 0.0f ? -out->transformed_i : out->transformed_i;
        real aj = out->transformed_j < 0.0f ? -out->transformed_j : out->transformed_j;
        real ak = out->transformed_k < 0.0f ? -out->transformed_k : out->transformed_k;

        if (ak < aj || ak < ai) {
            major_axis = (aj < ai) ? 0 : 1;
        } else {
            major_axis = 2;
        }
    }
    out->major_axis = (int16_t)major_axis;

    {
        real *component = &out->transformed_i;
        normal_positive = component[major_axis] > 0.0f;
    }
    out->normal_positive = normal_positive;

    axes = &halo::math::globals().k_projection_axes[major_axis * 2 + normal_positive];

    corner[0] = box[0] * placement->forward.i + box[2] * placement->left.i + placement->position.x;
    corner[1] = box[0] * placement->forward.j + box[2] * placement->left.j + placement->position.y;
    corner[2] = box[0] * placement->forward.k + box[2] * placement->left.k + placement->position.z;
    out->corners[0].u = corner[axes->i];
    out->corners[0].v = corner[axes->j];

    corner[0] = box[1] * placement->forward.i + box[2] * placement->left.i + placement->position.x;
    corner[1] = box[1] * placement->forward.j + box[2] * placement->left.j + placement->position.y;
    corner[2] = box[1] * placement->forward.k + box[2] * placement->left.k + placement->position.z;
    out->corners[1].u = corner[axes->i];
    out->corners[1].v = corner[axes->j];

    corner[0] = box[1] * placement->forward.i + box[3] * placement->left.i + placement->position.x;
    corner[1] = box[1] * placement->forward.j + box[3] * placement->left.j + placement->position.y;
    corner[2] = box[1] * placement->forward.k + box[3] * placement->left.k + placement->position.z;
    out->corners[2].u = corner[axes->i];
    out->corners[2].v = corner[axes->j];

    corner[0] = box[0] * placement->forward.i + box[3] * placement->left.i + placement->position.x;
    corner[1] = box[0] * placement->forward.j + box[3] * placement->left.j + placement->position.y;
    corner[2] = box[0] * placement->forward.k + box[3] * placement->left.k + placement->position.z;
    out->corners[3].u = corner[axes->i];
    out->corners[3].v = corner[axes->j];

    out->du_edge0 = out->corners[1].u - out->corners[0].u;
    out->dv_edge0 = out->corners[1].v - out->corners[0].v;
    out->du_edge1 = out->corners[3].u - out->corners[0].u;
    out->dv_edge1 = out->corners[3].v - out->corners[0].v;
    out->inverse_determinant = 1.0f / (out->dv_edge1 * out->du_edge0 - out->dv_edge0 * out->du_edge1);
}

/**
 * Clears the temporary flag (and its budget counter) on every live decal, and when
 * `clear_object_attached` is set, also clears the object-attached flag and its counter.
 *
 * @address 0x44e220
 */
void decal_ref::clear_flags(uint8_t clear_object_attached)
{
    if (decal_data->valid) {
        data_iterator iterator;
        decal *self;

        iterator.data = decal_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        self = (decal *)halo::memory::data_iterator_next(&iterator);

        while (self != 0) {
            if ((self->flags & _decal_temporary_bit) != 0) {
                self->flags = self->flags & ~_decal_temporary_bit;
                decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
            }
            if (clear_object_attached != 0 && (self->flags & _decal_object_attached_bit) != 0) {
                self->flags = self->flags & ~_decal_object_attached_bit;
                decal_grid_block->object_count = decal_grid_block->object_count - 1;
            }
            self = (decal *)halo::memory::data_iterator_next(&iterator);
        }
    }
}

/**
 * Unlinks a decal from its list (a cluster row or the object-attached list) and deletes it.
 *
 * @address 0x44e3c0
 */
void decal_ref::destroy()
{
    datum_index decal_index = datum;
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    datum_index next = self->next_decal;
    datum_index previous = self->previous_decal;

    if (next != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)next].previous_decal = previous;
    }

    if (previous != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)previous].next_decal = next;
    } else if (self->cluster_index == -1) {
        decal_grid_block->first_object_decal = next;
    } else {
        decal_grid_block->cluster_first[self->layer][self->cluster_index] = next;
    }

    halo::memory::datum_delete(decal_data, decal_index);
}

/**
 * Clears the object-attached flag (and its cached render geometry) of every object-attached
 * decal in `cluster_index`, or of every object-attached decal in the whole table when
 * `cluster_index` is -1.
 *
 * @address 0x44e310
 */
void decal_ref::evict_object_decals(int16_t cluster_index)
{
    if (decal_data->valid) {
        int layer;

        for (layer = 0; layer < 5; layer++) {
            datum_index current;

            if (cluster_index == -1) {
                if (layer != 0) {
                    continue;
                }
                current = decal_grid_block->first_object_decal;
            } else {
                current = decal_grid_block->cluster_first[layer][cluster_index];
            }

            while (current != k_datum_index_none) {
                decal *self = &((decal *)decal_data->data)[(uint16_t)current];
                datum_index next = self->next_decal;

                if ((self->flags & _decal_object_attached_bit) != 0) {
                    self->flags = self->flags & ~_decal_object_attached_bit;
                    decal_grid_block->object_count = decal_grid_block->object_count - 1;
                    halo::memory::cache_evict_entry(current, rasterizer_decal_vertex_cache_handle);
                }

                current = next;
            }
        }
    }
}

/**
 * Walks the edge loop of BSP surface `surface_index`, clipping the decal's projected quad
 * against each edge in turn. Where an edge's opposite surface is reachable (the decal's
 * placement sphere, scaled by k_decal_type_parameters[decal_type].radius_scale, crosses the
 * edge), that neighbouring surface index is appended to `surface_queue` for the caller to flood
 * into next. On the surfaces the decal actually covers (is_first_surface set, and the surface
 * within maximum_edge_angle of the decal plane) the clipped polygon is appended to
 * `accumulator`. Surfaces that fail the tight angle test but pass the looser fallback_edge_angle one go
 * on `fallback_queue` instead.
 *
 * @address 0x44e730
 */
void decal_ref::flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator, int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type, int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue, uint16_t *fallback_queue_count)
{
    ModelCollisionGeometryBSPSurface *surfaces;
    ModelCollisionGeometryBSPEdge *edges;
    ModelCollisionGeometryBSPVertex *bsp_vertices;
    ModelCollisionGeometryBSPSurface *surface;
    const projection_axis_pair *axes;
    real_plane3d surface_plane;
    real angle;
    int16_t queued_count = 0;
    int16_t fallback_count = 0;

    if (surface_index == -1) {
        return;
    }

    surfaces = (ModelCollisionGeometryBSPSurface *)halo::physics::globals().structure_collision_bsp->surfaces.pointer;
    edges = (ModelCollisionGeometryBSPEdge *)halo::physics::globals().structure_collision_bsp->edges.pointer;
    bsp_vertices = (ModelCollisionGeometryBSPVertex *)halo::physics::globals().structure_collision_bsp->vertices.pointer;
    surface = &surfaces[surface_index];

    if (is_first_surface != 0) {
        queued_count = (int16_t)*surface_queue_count;
        fallback_count = (int16_t)*fallback_queue_count;
    }

    halo::structures::structure_bsp_plane_fetch_signed(&surface_plane, global_structure_collision_bsp, (int32_t)surface->plane);
    angle = halo::math::vector3d_angle_between_4cd5e0(*((const real_vector3d *)&projection->transformed_i), surface_plane.normal);

    axes = &halo::math::globals().k_projection_axes[projection->major_axis * 2 + projection->normal_positive];

    if (is_first_surface == 0 ||
        angle <= k_decal_type_parameters[decal_type].maximum_edge_angle * 0.017453292f) {
        int32_t edge_index = (int32_t)surface->first_edge;
        uint16_t edge_ordinal = 0;
        int16_t vertex_count = 4;
        uint32_t edge_bitmask = 0;
        uint8_t clipped_flag = 0;
        real_point2d *polygon = (real_point2d *)&projection->corners[0];
        real_point2d *clip_out = &decal_clip_buffers[0][0];
        real_point2d previous;
        real_point2d current;
        real_plane2d edge_plane;

        previous.x = 0.0f;
        previous.y = 0.0f;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            int surface_is_right = ((int32_t)edge->right_surface == surface_index);
            uint32_t far_slot = (uint32_t)!surface_is_right;
            const real_point3d *point =
                (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[far_slot]].point;

            clip_out = &decal_clip_buffers[edge_ordinal & 1][0];

            if ((int16_t)edge_ordinal == 0) {
                const real_point3d *other =
                    (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                previous.x = (&other->x)[axes->i];
                previous.y = (&other->x)[axes->j];
            }

            current.x = (&point->x)[axes->i];
            current.y = (&point->x)[axes->j];

            if (halo::math::plane2d_from_points(&edge_plane, previous, current) == 0) {
                vertex_count = 0;
            } else {
                vertex_count = halo::math::polygon2d_clip_to_plane(clip_out, vertex_count, polygon,
                    edge_plane, 12, &edge_bitmask, &clipped_flag, 0.0f);

                if (is_first_surface != 0 && clipped_flag != 0 && queued_count < 0x400) {
                    const real_point3d *other =
                        (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                    real_vector3d along_edge;
                    along_edge.i = other->x - point->x;
                    along_edge.j = other->y - point->y;
                    along_edge.k = other->z - point->z;

                    if (halo::math::ray_intersects_sphere_test(projection->placement.position,
                            *(real_point3d *)point, along_edge,
                            radius * k_decal_type_parameters[decal_type].radius_scale) != 0) {
                        int32_t neighbour = (int32_t)(&edge->left_surface)[far_slot];
                        int16_t i = 0;

                        while (neighbour != -1) {
                            if (queued_count <= i) {
                                surface_queue[queued_count] = neighbour;
                                queued_count = queued_count + 1;
                                break;
                            }
                            if (surface_queue[i] == neighbour) {
                                neighbour = -1;
                            }
                            i = i + 1;
                        }
                    }
                }
            }

            edge_index = (int32_t)(&edge->forward_edge)[surface_is_right];
            previous = current;
            edge_ordinal = edge_ordinal + 1;
        } while (edge_index != (int32_t)surface->first_edge &&
                 (polygon = clip_out, vertex_count > 0));

        if (vertex_count > 2 &&
            vertex_count <= 0x400 - accumulator->vertex_count &&
            (surface->flags & 0x0b) == 0) {
            int16_t i;

            accumulator->visited_surfaces[accumulator->visited_surface_count] = vertex_count;
            accumulator->visited_surface_count = accumulator->visited_surface_count + 1;

            for (i = 0; i < vertex_count; i++) {
                real_point2d *clipped = &clip_out[i];
                decal_flood_vertex_record *out_vertex =
                    &accumulator->vertices[accumulator->vertex_count];
                real du = clipped->x - projection->corners[0].u;
                real dv = clipped->y - projection->corners[0].v;

                out_vertex->u = (du * projection->dv_edge1 - dv * projection->du_edge1) *
                    projection->inverse_determinant;
                out_vertex->v = -((du * projection->dv_edge0 - dv * projection->du_edge0) *
                    projection->inverse_determinant);

                halo::math::decal_plane_solve_third_axis((real_point3d *)out_vertex, projection->normal_positive,
                    projection->major_axis, &surface_plane, *clipped);

                if ((edge_bitmask & (1u << (i & 0x1f))) == 0) {
                    out_vertex->position.x += surface_plane.normal.i * 0.00390625f;
                    out_vertex->position.y += surface_plane.normal.j * 0.00390625f;
                    out_vertex->position.z += surface_plane.normal.k * 0.00390625f;
                }

                accumulator->vertex_count = accumulator->vertex_count + 1;
            }
        }
    } else {
        int32_t edge_index = (int32_t)surface->first_edge;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            int surface_is_right = ((int32_t)edge->right_surface == surface_index);
            const real_point3d *point =
                (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[(uint32_t)!surface_is_right]].point;

            if (queued_count < 0x400) {
                const real_point3d *other =
                    (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                real_vector3d along_edge;
                along_edge.i = other->x - point->x;
                along_edge.j = other->y - point->y;
                along_edge.k = other->z - point->z;

                if (halo::math::ray_intersects_sphere_test(projection->placement.position,
                        *(real_point3d *)point, along_edge,
                        radius * k_decal_type_parameters[decal_type].radius_scale) != 0) {
                    int32_t neighbour =
                        (int32_t)(&edge->left_surface)[(uint32_t)!surface_is_right];
                    int16_t i = 0;

                    while (neighbour != -1) {
                        if (queued_count <= i) {
                            surface_queue[queued_count] = neighbour;
                            queued_count = queued_count + 1;
                            break;
                        }
                        if (surface_queue[i] == neighbour) {
                            neighbour = -1;
                        }
                        i = i + 1;
                    }
                }
            }

            edge_index = (int32_t)(&edge->forward_edge)[surface_is_right];
        } while (edge_index != (int32_t)surface->first_edge);

        if (angle <= k_decal_type_parameters[decal_type].fallback_edge_angle * 0.017453292f &&
            fallback_count < 0x400) {
            fallback_queue[fallback_count] = surface_index;
            fallback_count = fallback_count + 1;
        }
    }

    if (is_first_surface != 0) {
        *surface_queue_count = (uint16_t)queued_count;
        *fallback_queue_count = (uint16_t)fallback_count;
    }
}

/**
 * Inserts `decal_index` at the head of decal_grid's (layer, cluster_index) list.
 *
 * @address 0x44dd30
 */
void decal_ref::link(int16_t cluster_index, int16_t layer)
{
    datum_index decal_index = datum;
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    datum_index *head = &decal_grid_block->cluster_first[layer][cluster_index];
    datum_index old_head = *head;

    self->previous_decal = k_datum_index_none;
    self->next_decal = old_head;
    self->cluster_index = cluster_index;
    self->layer = layer;

    if (old_head != k_datum_index_none) {
        ((decal *)decal_data->data)[(uint16_t)old_head].previous_decal = decal_index;
    }

    *head = decal_index;
}

/**
 * Allocates the decal datum at requested_handle. A non object-attached decal is randomly classed temporary
 * (k_decal_permanent_percent), and if that pushes the temporary count over k_maximum_temporary_decals, temporary
 * decals are un-flagged (each with a k_decal_evict_percent chance, or always when off the grid) until the count
 * drops to k_temporary_decal_eviction_target. The new decal is spliced in before `insert_before`, or linked at the
 * head of the (layer, cluster_index) list.
 *
 * @address 0x44dd90
 */
datum_index decal_ref::create(datum_index requested_handle, int16_t cluster_index, int16_t layer, datum_index insert_before, uint8_t object_attached)
{
    datum_index handle = halo::memory::datum_new_at_index_with_salt(requested_handle, decal_data);
    decal *self;

    if (handle == k_datum_index_none) {
        return handle;
    }
    self = &((decal *)decal_data->data)[(uint16_t)handle];

    if (object_attached != 0) {
        self->flags = _decal_object_attached_bit;
        decal_grid_block->object_count = decal_grid_block->object_count + 1;
    } else {
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        if ((int32_t)((halo::math::globals().effect_random_seed >> k_random_value_shift) * 100) < k_decal_permanent_percent) {
            self->flags = _decal_temporary_bit;
            decal_grid_block->temporary_count = decal_grid_block->temporary_count + 1;

            if (decal_grid_block->temporary_count > k_maximum_temporary_decals) {
                data_iterator iterator;
                int16_t restarts = 0;

                iterator.data = decal_data;
                iterator.next_index = 0;
                iterator.index = k_datum_index_none;
                iterator.signature = (uint32_t)(uintptr_t)decal_data ^ k_data_iterator_signature;

                while (decal_grid_block->temporary_count > k_temporary_decal_eviction_target) {
                    decal *candidate = (decal *)halo::memory::data_iterator_next(&iterator);

                    if (candidate == 0) {
                        restarts = (int16_t)(restarts + 1);
                        iterator.data = decal_data;
                        iterator.next_index = 0;
                        iterator.index = k_datum_index_none;
                        iterator.signature = (uint32_t)(uintptr_t)decal_data ^ k_data_iterator_signature;
                        if (restarts >= k_decal_eviction_attempt_limit + 1) {
                            return k_datum_index_none;
                        }
                    } else if ((candidate->flags & _decal_temporary_bit) != 0) {
                        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                        if ((int32_t)((halo::math::globals().effect_random_seed >> k_random_value_shift) * 100) < k_decal_evict_percent ||
                            candidate->cluster_index == -1) {
                            candidate->flags = candidate->flags & ~_decal_temporary_bit;
                            decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
                        }
                    }
                }
            }
        } else {
            self->flags = 0;
        }
    }

    if (insert_before != k_datum_index_none) {
        decal *before = &((decal *)decal_data->data)[(uint16_t)insert_before];
        datum_index previous = before->previous_decal;

        if (previous == k_datum_index_none) {
            decal_grid_block->cluster_first[layer][cluster_index] = handle;
        } else {
            ((decal *)decal_data->data)[(uint16_t)previous].next_decal = handle;
        }
        before->previous_decal = handle;
        self->next_decal = insert_before;
        self->cluster_index = cluster_index;
        self->previous_decal = handle;
        self->layer = layer;
        return handle;
    }

    halo::effects::decal_link(cluster_index, handle, layer);
    return handle;
}

/**
 * Re-probes every object-attached decal against the structure BSP, and once it resolves to a
 * valid cluster, unlinks it from the object-attached list and relinks it into that cluster's row
 * of decal_grid.
 *
 * @address 0x44e000
 */
void decal_ref::rehash_object_decals()
{
    if (decal_data->valid) {
        datum_index decal_index = decal_grid_block->first_object_decal;

        while (decal_index != k_datum_index_none) {
            decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
            datum_index next = self->next_decal;
            int32_t leaf = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &self->position);

            if (leaf != -1) {
                int16_t cluster = *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer +
                    (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

                if (cluster != -1) {
                    if (next != k_datum_index_none) {
                        ((decal *)decal_data->data)[(uint16_t)next].previous_decal = self->previous_decal;
                    }
                    if (self->previous_decal == k_datum_index_none) {
                        decal_grid_block->first_object_decal = next;
                    } else {
                        ((decal *)decal_data->data)[(uint16_t)self->previous_decal].next_decal = next;
                    }

                    halo::effects::decal_link(cluster, decal_index, self->layer);
                }
            }

            decal_index = next;
        }
    }
}

/**
 * Member form of the original decal_spawn_for_response: spawn for response.
 *
 * @address 0x44ece0
 */
void decal_ref::spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin, real_vector3d *direction, real radius, int32_t marker_index)
{
    uint8_t allowed = 1;
    random_seed saved_seed = 0;
    collision_result result;

    if (decals_for_all_responses == 0 &&
        (deterministic != 1 ||
            *(int16_t *)((uint8_t *)halo::cache::globals().tag_instances[response_tag_index & halo::k_slot_mask].data + 4) != 3)) {
        allowed = 0;
    }
    if (decals_enabled == 0 || !allowed) {
        return;
    }
    if (deterministic != 0) {
        uint32_t *words = (uint32_t *)origin;

        saved_seed = halo::math::globals().effect_random_seed;
        halo::math::globals().effect_random_seed = words[2] ^ words[1] ^ words[0] ^ 0xdeadc0de;
    }
    if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface | halo::collision_test_flag::unstick), origin, direction, k_datum_index_none, &result) &&
        result.type == 2 &&
        (*(uint8_t *)halo::cache::globals().tag_instances[response_tag_index & halo::k_slot_mask].data & 0x10) == 0) {
        halo::effects::decal_place(response_tag_index, &result, direction, radius, deterministic, (int16_t)marker_index);
    }
    if (deterministic != 0) {
        halo::math::globals().effect_random_seed = saved_seed;
    }
}

/**
 * Recomputes one decal's alpha from its age. A cluster decal (not object-attached) that has
 * outlived its lifetime is evicted from the geometry cache; one inside its final decay_time
 * window fades linearly to 0.
 *
 * @address 0x44dc30
 */
void decal_ref::update_fade()
{
    datum_index decal_index = datum;
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    real age = (real)(halo::game::globals().game_time->game_time - self->creation_game_time) * 0.033333335f;

    self->alpha = 0xff;

    if ((self->flags & _decal_object_attached_bit) == 0) {
        if (self->lifetime != 0.0f && !(age < self->lifetime)) {
            if ((self->flags & _decal_temporary_bit) != 0) {
                self->flags = self->flags & ~_decal_temporary_bit;
                decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
            }
            halo::memory::cache_evict_entry(decal_index, rasterizer_decal_vertex_cache_handle);
            return;
        }

        if (self->lifetime > 0.0f && self->decay_time > 0.0f) {
            real remaining = self->lifetime - age;

            if (remaining < self->decay_time) {
                real fade = (remaining / self->decay_time) * 255.0f;

                self->alpha = (uint8_t)halo::libm::lrint((double)fade);
            }
        }
    }
}

/**
 * Member form of the original decals_detach_from_structure_bsp: detach from structure bsp.
 *
 * @address 0x44e140
 */
void decal_ref::detach_from_structure_bsp()
{
    int32_t cluster;
    int32_t layer;

    if (!decal_data->valid) {
        return;
    }
    for (cluster = 0; cluster < 0x200; cluster++) {
        for (layer = 0; layer < 5; layer++) {
            datum_index *cell = &decal_grid_block->cluster_first[layer][cluster];
            datum_index head = *cell;
            datum_index handle = head;

            while (handle != k_datum_index_none) {
                decal *entry = (decal *)((uint8_t *)decal_data->data + (handle & halo::k_slot_mask) * 0x38);
                datum_index next = entry->next_decal;

                entry->cluster_index = -1;
                if (next == k_datum_index_none) {
                    datum_index first = decal_grid_block->first_object_decal;

                    entry->next_decal = first;
                    if (first != k_datum_index_none) {
                        ((decal *)((uint8_t *)decal_data->data + (first & halo::k_slot_mask) * 0x38))->previous_decal = handle;
                    }
                    decal_grid_block->first_object_decal = head;
                    *cell = k_datum_index_none;
                }
                handle = next;
            }
        }
    }
}

/**
 * Registers the decal datum table, marks it valid immediately, carves decal_grid out of game
 * state (folding its size into the game-state CRC), and initializes the rasterizer's decal
 * geometry storage.
 *
 * @address 0x44df90
 */
void decal_ref::initialize()
{
    uint32_t block_size = sizeof(decal_grid);

    decal_data = (data_array *)halo::saved_games::game_state_new("decals", k_maximum_decals, sizeof(decal));
    ((uint8_t *)decal_data)[0x25] = 1;

    decal_grid_block = (decal_grid *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + sizeof(decal_grid);
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&block_size, 4);

    halo::rasterizer::rasterizer_decals_initialize();
}

/**
 * Per-tick driver: recomputes the fade alpha of every live decal.
 *
 * @address 0x44e2b0
 */
void decal_ref::update_fade_all()
{
    if (decal_data->valid) {
        data_iterator iterator;

        iterator.data = decal_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        while (halo::memory::data_iterator_next(&iterator) != 0) {
            halo::effects::decal_update_fade(iterator.index);
        }
    }
}

}

namespace halo::effects {

void decal_build_projection(real_matrix4x3 *placement, real *box, decal_projection *out)
{
    halo::effects::decal_ref::build_projection(placement, box, out);
}

void decal_clear_flags(uint8_t clear_object_attached)
{
    halo::effects::decal_ref::clear_flags(clear_object_attached);
}

void decal_delete(datum_index decal_index)
{
    halo::effects::decal_ref(decal_index).destroy();
}

void decal_evict_object_decals(int16_t cluster_index)
{
    halo::effects::decal_ref::evict_object_decals(cluster_index);
}

void decal_flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator, int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type, int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue, uint16_t *fallback_queue_count)
{
    halo::effects::decal_ref::flood_surfaces(projection, accumulator, surface_index, is_first_surface, radius, decal_type, surface_queue, surface_queue_count, fallback_queue, fallback_queue_count);
}

void decal_link(int16_t cluster_index, datum_index decal_index, int16_t layer)
{
    halo::effects::decal_ref(decal_index).link(cluster_index, layer);
}

datum_index decal_new(datum_index requested_handle, int16_t cluster_index, int16_t layer, datum_index insert_before, uint8_t object_attached)
{
    return halo::effects::decal_ref::create(requested_handle, cluster_index, layer, insert_before, object_attached);
}

void decal_rehash_object_decals()
{
    halo::effects::decal_ref::rehash_object_decals();
}

void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin, real_vector3d *direction, real radius, int32_t marker_index)
{
    halo::effects::decal_ref::spawn_for_response(response_tag_index, deterministic, origin, direction, radius, marker_index);
}

void decal_update_fade(datum_index decal_index)
{
    halo::effects::decal_ref(decal_index).update_fade();
}

void decals_detach_from_structure_bsp()
{
    halo::effects::decal_ref::detach_from_structure_bsp();
}

void decals_initialize()
{
    halo::effects::decal_ref::initialize();
}

void decals_update_fade()
{
    halo::effects::decal_ref::update_fade_all();
}

}
