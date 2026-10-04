#include "halo/math/constants.hpp"
#include "halo/core/lcg.hpp"
#include "halo/effects/effects.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/api.hpp"

static auto &decal_data = halo::link::ref<data_array *>(halo::effects::vars().decal_data);
static auto &k_decal_type_parameters = halo::link::ref<const decal_type_parameters [4]>(halo::effects::vars().k_decal_type_parameters);
static auto &rasterizer_decal_vertex_cache_handle = halo::link::ref<cache *>(halo::effects::vars().rasterizer_decal_vertex_cache_handle);
static auto &rasterizer_decal_vertex_cache = halo::link::ref<void *>(halo::effects::vars().rasterizer_decal_vertex_cache);

namespace halo::effects {

namespace {
typedef struct decal_place_vertex {
    real_point3d position;
    uint32_t texcoord;
} decal_place_vertex;
}

/**
 * File-local helper used by decal_place.
 */
static real decal_place_random_fraction(void)
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(int32_t)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale;
}

/**
 * 0x44f0bb..0x44f1ff: scale to unit length unless shorter than 1e-4
 */
static void decal_place_normalize_guarded(real_vector3d *v)
{
    real length = (real)halo::libm::sqrt((double)(v->k * v->k + v->j * v->j + v->i * v->i));

    if (!(halo::libm::fabs((double)length) < (double)1.0e-4f)) {
        real inverse = 1.0f / length;

        v->i = v->i * inverse;
        v->j = v->j * inverse;
        v->k = v->k * inverse;
    }
}

/**
 * 0x44ee7c..0x44ef52 / 0x44efda..0x44f0ad (sapien_snap_to_axis): the unit axis of `axis_source`'s largest
 * component, pushed off the surface by the normal (towards the side given by `side`), then a = v x normal and
 * b = a x normal. The first attempt takes `side` from dot(normal, direction), the retry from dot(axis, normal).
 */
static void decal_place_snap_basis(real_vector3d *axis_source, const real_vector3d *side_direction,
    real_vector3d *normal, real_vector3d *a, real_vector3d *b)
{
    real_vector3d v;
    int16_t axis = halo::math::vector3d_major_axis_index(*axis_source);
    real sign = (((real *)axis_source)[axis] > 0.0f) ? 1.0f : -1.0f;
    real side;

    v.i = 0.0f;
    v.j = 0.0f;
    v.k = 0.0f;
    ((real *)&v)[axis] = sign;

    if (side_direction != 0) {
        side = normal->j * side_direction->j + normal->k * side_direction->k + side_direction->i * normal->i;
    } else {
        side = v.k * normal->k + v.j * normal->j + v.i * normal->i;
    }
    if (side > 0.0f) {
        v.i = v.i + normal->i;
        v.j = v.j + normal->j;
        v.k = v.k + normal->k;
    } else {
        v.i = v.i - normal->i;
        v.j = v.j - normal->j;
        v.k = v.k - normal->k;
    }
    halo::math::vector3d_normalize_with_length(v);
    halo::math::vector3d_cross_product(*a, v, *normal);
    halo::math::vector3d_cross_product(*b, *a, *normal);
}

/**
 * A collision BSP plane by signed index (the sign bit selects the back face).
 */
static void decal_place_surface_plane(real_plane3d *out, uint32_t signed_plane_index)
{
    const real_plane3d *plane = &((const real_plane3d *)halo::physics::globals().structure_collision_bsp->planes.pointer)
        [signed_plane_index & 0x7fffffff];

    if ((int32_t)signed_plane_index < 0) {
        out->normal.i = -plane->normal.i;
        out->normal.j = -plane->normal.j;
        out->normal.k = -plane->normal.k;
        out->d = -plane->d;
    } else {
        *out = *plane;
    }
}

/**
 * w rotated (not translated) by m
 */
static void decal_place_rotate(real_vector3d *out, const real_matrix4x3 *m, const real_vector3d *w)
{
    out->i = m->up.i * w->k + m->left.i * w->j + m->forward.i * w->i;
    out->j = m->up.j * w->k + m->left.j * w->j + m->forward.j * w->i;
    out->k = m->up.k * w->k + m->left.k * w->j + m->forward.k * w->i;
}

/**
 * 0x44f55d..0x44fe95: takes fallback_queue[first] and every later fallback surface within maximum_edge_angle of its
 * plane as one group, folds the placement around the group edge closest to the decal plane and floods the group
 * with the folded projection. Returns the group size (the entries are consumed, set to -1).
 */
static int16_t decal_place_wrap_group(int32_t *fallback_queue, int16_t fallback_count, int16_t first,
    const Decal *definition, decal_projection *projection, const real_matrix4x3 *matrix, real *box, real radius,
    decal_flood_accumulator *accumulator, real_vector3d *normal_min, real_vector3d *normal_max)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)halo::physics::globals().structure_collision_bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges;
    ModelCollisionGeometryBSPVertex *vertices;
    real maximum_angle = k_decal_type_parameters[definition->type].maximum_edge_angle * halo::math::k_degrees_to_radians;
    int32_t group[0x400];
    int16_t group_count = 0;
    real_plane3d first_plane;
    real_plane3d best_plane;
    real_point3d edge_start;
    real_point3d edge_end;
    real best_low = 0.0f;
    real best_high = 0.0f;
    int32_t best_surface = -1;
    real_vector3d *decal_normal = (real_vector3d *)&projection->transformed_i;
    decal_projection wrapped;
    int16_t j;
    int16_t g;

    decal_place_surface_plane(&first_plane, surfaces[fallback_queue[first]].plane);
    group[group_count] = fallback_queue[first];
    fallback_queue[first] = -1;
    group_count++;

    for (j = (int16_t)(first + 1); j < fallback_count; j++) {
        if (fallback_queue[j] != -1) {
            real_plane3d plane;

            decal_place_surface_plane(&plane, surfaces[fallback_queue[j]].plane);
            if (halo::math::vector3d_angle_between_4cd5e0(plane.normal, first_plane.normal) <= maximum_angle) {
                group[group_count] = fallback_queue[j];
                group_count++;
                fallback_queue[j] = -1;
            }
        }
    }

    edges = (ModelCollisionGeometryBSPEdge *)halo::physics::globals().structure_collision_bsp->edges.pointer;
    vertices = (ModelCollisionGeometryBSPVertex *)halo::physics::globals().structure_collision_bsp->vertices.pointer;
    for (g = 0; g < group_count; g++) {
        int32_t surface_index = group[g];
        ModelCollisionGeometryBSPSurface *surface = &surfaces[surface_index];
        uint32_t edge_index = surface->first_edge;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            uint32_t is_right = (edge->right_surface == (uint32_t)surface_index);
            const real_point3d *far_point =
                (const real_point3d *)&vertices[(&edge->start_vertex)[is_right == 0]].point;
            const real_point3d *near_point =
                (const real_point3d *)&vertices[(&edge->start_vertex)[is_right]].point;
            real low = (real)halo::libm::fabs((double)(projection->transformed_j * far_point->y +
                projection->transformed_k * far_point->z + projection->transformed_i * far_point->x -
                projection->transformed_d));
            real high = (real)halo::libm::fabs((double)(projection->transformed_j * near_point->y +
                projection->transformed_k * near_point->z + projection->transformed_i * near_point->x -
                projection->transformed_d));

            if (low > high) {
                real swap = low;
                low = high;
                high = swap;
            }
            if (best_surface == -1 || (low <= best_low && high <= best_high)) {
                decal_place_surface_plane(&best_plane, surface->plane);
                best_low = low;
                best_high = high;
                edge_start = *far_point;
                edge_end = *near_point;
                best_surface = surface_index;
            }
            edge_index = (&edge->forward_edge)[is_right];
        } while (edge_index != surface->first_edge);
    }

    {
        real_vector3d axis;
        real length;

        axis.i = edge_end.x - edge_start.x;
        axis.j = edge_end.y - edge_start.y;
        axis.k = edge_end.z - edge_start.z;
        length = (real)halo::libm::sqrt((double)(axis.i * axis.i + axis.k * axis.k + axis.j * axis.j));

        if (halo::libm::fabs((double)length) < (double)1.0e-4f || !(length > 0.0f)) {
            wrapped = *projection;
        } else {
            real inverse = 1.0f / length;
            real_vector3d crossed;
            real sign = 1.0f;
            real angle;
            real_matrix4x3 rotation;
            real_matrix4x3 folded;
            real_vector3d offset;

            axis.i = axis.i * inverse;
            axis.j = axis.j * inverse;
            axis.k = axis.k * inverse;

            crossed.i = best_plane.normal.j * decal_normal->k - best_plane.normal.k * decal_normal->j;
            crossed.j = best_plane.normal.k * decal_normal->i - decal_normal->k * best_plane.normal.i;
            crossed.k = decal_normal->j * best_plane.normal.i - best_plane.normal.j * decal_normal->i;
            if (!(crossed.i * axis.i + crossed.k * axis.k + crossed.j * axis.j < 0.0f)) {
                sign = -1.0f;
            }
            angle = halo::math::vector3d_angle_between_4cd5e0(*decal_normal, best_plane.normal) * sign;
            halo::math::matrix4x3_from_axis_angle(rotation, axis, (real)halo::libm::sin((double)angle), (real)halo::libm::cos((double)angle));

            offset.i = matrix->position.x - edge_start.x;
            offset.j = matrix->position.y - edge_start.y;
            offset.k = matrix->position.z - edge_start.z;
            if (rotation.scale != 1.0f) {
                offset.i = rotation.scale * offset.i;
                offset.j = rotation.scale * offset.j;
                offset.k = rotation.scale * offset.k;
            }

            folded.scale = 1.0f;
            decal_place_rotate(&folded.forward, &rotation, &matrix->forward);
            decal_place_rotate(&folded.left, &rotation, &matrix->left);
            decal_place_rotate(&folded.up, &rotation, &matrix->up);
            folded.position.x = rotation.left.i * offset.j + rotation.forward.i * offset.i +
                rotation.up.i * offset.k + rotation.position.x + edge_start.x;
            folded.position.y = rotation.left.j * offset.j + rotation.forward.j * offset.i +
                rotation.up.j * offset.k + rotation.position.y + edge_start.y;
            folded.position.z = rotation.left.k * offset.j + rotation.forward.k * offset.i +
                rotation.up.k * offset.k + rotation.position.z + edge_start.z;

            halo::effects::decal_build_projection(&folded, box, &wrapped);

            if (!(folded.up.i > normal_min->i)) normal_min->i = folded.up.i;
            if (folded.up.i > normal_max->i) normal_max->i = folded.up.i;
            if (!(folded.up.j > normal_min->j)) normal_min->j = folded.up.j;
            if (folded.up.j > normal_max->j) normal_max->j = folded.up.j;
            if (!(folded.up.k > normal_min->k)) normal_min->k = folded.up.k;
            if (folded.up.k > normal_max->k) normal_max->k = folded.up.k;
        }
    }

    for (g = 0; g < group_count; g++) {
        halo::effects::decal_flood_surfaces(&wrapped, accumulator, group[g], 0, radius, (int16_t)definition->type, 0, 0, 0, 0);
    }
    return group_count;
}

/**
 * Member form of the original decal_place: place.
 *
 * @address 0x44edc0
 */
void decal_ref::place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction, real radius_scale, uint8_t object_attached, int16_t requested_sequence_index)
{
    uint8_t inherit_geometry = 0;
    int16_t sequence_index = 0;
    int16_t sprite_index = 0;
    real radius = 0.0f;
    real_matrix4x3 matrix;
    real sprite_rect[4];
    real box[4];
    decal_projection projection;
    decal_flood_accumulator accumulator;
    int32_t surface_queue[0x400];
    int32_t fallback_queue[0x400];
    decal_place_vertex local_vertices[0x400];

    if (decal_tag_index == k_datum_index_none) {
        return;
    }

    for (;;) {
        Decal *definition = (Decal *)halo::cache::globals().tag_instances[(uint16_t)decal_tag_index].data;
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[*(uint16_t *)&definition->map.tag_id].data;
        int16_t sprite_bitmap_index;
        uint16_t queue_count;
        uint16_t fallback_count = 0;
        int16_t queue_cursor;
        real_vector3d normal_min;
        real_vector3d normal_max;
        real_vector3d lift;
        int32_t block_count = 0;
        datum_index geometry_handle;
        datum_index decal_index;
        decal *self;
        decal_place_vertex *out;
        int16_t k;

        if (!inherit_geometry) {
            real_vector3d *normal = &placement->plane.normal;
            real_vector3d a;
            real_vector3d b;
            real rotation_cos;
            real rotation_sin;

            if ((definition->flags & 8) != 0 &&
                normal->j * direction->j + normal->k * direction->k + direction->i * normal->i < -0.0001f) {
                rotation_cos = -1.0f;
                rotation_sin = 0.0f;
                if ((definition->flags & 0x20) != 0) {
                    decal_place_snap_basis(direction, direction, normal, &a, &b);
                    if (a.k * a.k + a.j * a.j + a.i * a.i < 0.0001f || b.k * b.k + b.j * b.j + b.i * b.i < 0.0001f) {
                        real_vector3d projected;
                        real d = -(normal->j * direction->j + normal->k * direction->k + normal->i * direction->i);

                        projected.i = d * normal->i + direction->i;
                        projected.j = d * normal->j + direction->j;
                        projected.k = d * normal->k + direction->k;
                        decal_place_snap_basis(&projected, 0, normal, &a, &b);
                    }
                } else {
                    halo::math::vector3d_cross_product(a, *direction, *normal);
                    halo::math::vector3d_cross_product(b, a, *normal);
                }
            } else {
                real angle = decal_place_random_fraction() * halo::math::k_two_pi;

                rotation_cos = (real)halo::libm::cos((double)angle);
                rotation_sin = (real)halo::libm::sin((double)angle);
                halo::math::vector3d_build_perpendicular(a, *normal);
                b.i = a.k * normal->j - a.j * normal->k;
                b.j = a.i * normal->k - a.k * normal->i;
                b.k = a.j * normal->i - a.i * normal->j;
            }
            decal_place_normalize_guarded(&a);
            decal_place_normalize_guarded(&b);

            matrix.scale = 1.0f;
            matrix.forward.i = b.i * rotation_cos - a.i * rotation_sin;
            matrix.forward.j = b.j * rotation_cos - a.j * rotation_sin;
            matrix.forward.k = b.k * rotation_cos - a.k * rotation_sin;
            matrix.left.i = b.i * rotation_sin + a.i * rotation_cos;
            matrix.left.j = b.j * rotation_sin + a.j * rotation_cos;
            matrix.left.k = b.k * rotation_sin + a.k * rotation_cos;
            matrix.up = *normal;
            matrix.position = placement->point;

            if (requested_sequence_index == -1) {
                int16_t count = (int16_t)bitmap->bitmap_group_sequence.count;
                int32_t roll;

                halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                roll = (int32_t)((uint32_t)((int32_t)count * (int32_t)(halo::math::globals().effect_random_seed >> k_random_value_shift)) >> 16);
                sequence_index = (int16_t)roll;
                if ((int32_t)(int16_t)roll >= (int32_t)bitmap->bitmap_group_sequence.count) {
                    sequence_index = (int16_t)((uint16_t)bitmap->bitmap_group_sequence.count - 1);
                }
            } else {
                sequence_index = requested_sequence_index;
            }

            sprite_index = 0;
            if (radius_scale == 0.0f) {
                radius_scale = 1.0f;
            }
            radius = (decal_place_random_fraction() * (definition->radius[1] - definition->radius[0]) +
                definition->radius[0]) * radius_scale;
        }

        if (bitmap->type == 3) {
            const BitmapGroupSequence *sequence =
                &((const BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer)[sequence_index];

            sprite_bitmap_index =
                (int16_t)((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index].bitmap_index;
            halo::structures::structure_lightmap_uv_rect_build(sequence_index, 0, radius, box, sprite_rect, definition);
        } else {
            real aspect = 1.0f;

            sprite_bitmap_index = 0;
            if ((definition->flags & 0x100) != 0) {
                const BitmapData *data = (const BitmapData *)bitmap->bitmap_data.pointer;

                aspect = (real)(int32_t)(int16_t)data->height / (real)(int32_t)(int16_t)data->width;
            }
            box[0] = -radius;
            box[1] = radius;
            sprite_rect[0] = 0.0f;
            sprite_rect[1] = 1.0f;
            sprite_rect[2] = 0.0f;
            sprite_rect[3] = 1.0f;
            box[2] = -(aspect * radius);
            box[3] = aspect * radius;
        }

        if (object_attached == 0 &&
            halo::cache::texture_cache_get(&((BitmapData *)bitmap->bitmap_data.pointer)[sprite_bitmap_index], 0, 1) == 0) {
            return;
        }

        halo::effects::decal_build_projection(&matrix, box, &projection);

        normal_min = matrix.up;
        normal_max = matrix.up;
        accumulator.visited_surface_count = 0;
        accumulator.vertex_count = 0;
        surface_queue[0] = placement->surface_index;
        queue_count = 1;
        queue_cursor = 0;
        do {
            int32_t surface_index = surface_queue[queue_cursor];

            queue_cursor++;
            halo::effects::decal_flood_surfaces(&projection, &accumulator, surface_index, 1, radius, (int16_t)definition->type,
                surface_queue, &queue_count, fallback_queue, &fallback_count);
        } while (queue_cursor < (int16_t)queue_count);

        if (k_decal_type_parameters[definition->type].wraps_fallback_surfaces != 0 &&
            (int16_t)fallback_count > 0) {
            int16_t remaining = (int16_t)fallback_count;

            for (;;) {
                int16_t index = 0;

                while (index < (int16_t)fallback_count && fallback_queue[index] == -1) {
                    index++;
                }
                if (index < (int16_t)fallback_count) {
                    remaining = (int16_t)(remaining - decal_place_wrap_group(fallback_queue, (int16_t)fallback_count,
                        index, definition, &projection, &matrix, box, radius, &accumulator, &normal_min,
                        &normal_max));
                }
                if (remaining <= 0) {
                    break;
                }
            }
        }

        if (accumulator.visited_surface_count <= 0 || accumulator.vertex_count <= 0) {
            return;
        }

        lift.i = 0.0f;
        lift.j = 0.0f;
        lift.k = 0.0f;
        if (normal_max.i - normal_min.i <= 0.5f && normal_max.j - normal_min.j <= 0.5f &&
            normal_max.k - normal_min.k <= 0.5f) {
            lift.i = normal_min.i + normal_max.i;
            lift.j = normal_min.j + normal_max.j;
            lift.k = normal_min.k + normal_max.k;
            halo::math::vector3d_normalize_with_length(lift);
            lift.i = lift.i * 0.00390625f;
            lift.j = lift.j * 0.00390625f;
            lift.k = lift.k * 0.00390625f;
        }

        for (k = 0; k < accumulator.visited_surface_count; k++) {
            block_count += ((int32_t)accumulator.visited_surfaces[k] - 1) / 2;
        }

        geometry_handle = halo::memory::cache_allocate_block(rasterizer_decal_vertex_cache_handle,
            (uint32_t)((int32_t)(int16_t)block_count << 6));
        if (geometry_handle == k_datum_index_none) {
            return;
        }
        decal_index = halo::effects::decal_new(geometry_handle, placement->leaf.cluster_index, (int16_t)definition->layer,
            k_datum_index_none, object_attached);
        if (decal_index == k_datum_index_none) {
            halo::memory::cache_evict_entry(geometry_handle, rasterizer_decal_vertex_cache_handle);
            return;
        }
        self = &((decal *)decal_data->data)[(uint16_t)decal_index];
        out = (decal_place_vertex *)halo::rasterizer::rasterizer_decal_vertex_cache_lock(geometry_handle,
            (int32_t)(int16_t)block_count << 6);
        if (out == 0) {
            halo::memory::cache_evict_entry(geometry_handle, rasterizer_decal_vertex_cache_handle);
            return;
        }

        {
            real u_span = sprite_rect[1] - sprite_rect[0];
            real v_span = sprite_rect[3] - sprite_rect[2];
            int32_t count = (uint16_t)accumulator.vertex_count;
            int32_t i;

            for (i = 0; i < count; i++) {
                const decal_flood_vertex_record *record = &accumulator.vertices[i];
                double u = (double)u_span * (double)record->u + (double)sprite_rect[0];
                double v = (double)v_span * (double)record->v + (double)sprite_rect[2];
                int16_t u_byte;
                int16_t v_byte;

                if (u < 0.0) {
                    u = 0.0;
                } else if (u > 1.0) {
                    u = 1.0;
                }
                if (v < 0.0) {
                    v = 0.0;
                } else if (v > 1.0) {
                    v = 1.0;
                }
                u = u * 255.0;
                if (u < 0.0) {
                    u = 0.0;
                } else if (u > 254.0) {
                    u = 254.0;
                }
                u_byte = (int16_t)halo::libm::lrint((double)(real)halo::libm::floor(u + 0.5));
                v = v * 255.0;
                if (v < 0.0) {
                    v = 0.0;
                } else if (v > 254.0) {
                    v = 254.0;
                }
                v_byte = (int16_t)halo::libm::lrint((double)(real)halo::libm::floor(v + 0.5));

                local_vertices[i].texcoord = (uint32_t)((((int32_t)u_byte << 8) | (int32_t)v_byte) << 8);
                local_vertices[i].position.x = lift.i + record->position.x;
                local_vertices[i].position.y = lift.j + record->position.y;
                local_vertices[i].position.z = lift.k + record->position.z;
            }
        }

        self->position = placement->point;
        self->creation_game_time = halo::game::globals().game_time->game_time;
        self->sequence_index = (uint8_t)sequence_index;
        self->sprite_bitmap_index = (uint8_t)sprite_bitmap_index;
        self->sprite_index = 0;
        self->lifetime = decal_place_random_fraction() * (definition->lifetime[1] - definition->lifetime[0]) +
            definition->lifetime[0];
        self->triangle_count = (int16_t)block_count;
        self->definition_index = decal_tag_index;
        self->decay_time = decal_place_random_fraction() * (definition->decay_time[1] - definition->decay_time[0]) +
            definition->decay_time[0];

        {
            ColorRGB color;
            real intensity = decal_place_random_fraction() * (definition->intensity[1] - definition->intensity[0]) +
                definition->intensity[0];
            real fraction = decal_place_random_fraction();

            halo::bitmaps::color_interpolate(&definition->color_upper_bounds, &definition->color_lower_bounds, &color,
                static_cast<color_interpolation_flags>((uint32_t)((*(const uint8_t *)&definition->flags >> 1) & 3)), fraction);
            self->color = ((uint32_t)halo::libm::lrint((double)color.blue * 255.0) & 0xff) |
                (((uint32_t)halo::libm::lrint((double)color.green * 255.0) & 0xff) << 8) |
                (((uint32_t)halo::libm::lrint((double)color.red * 255.0) & 0xff) << 16) |
                ((uint32_t)halo::libm::lrint((double)intensity * 255.0) << 24);
            self->alpha = 0xff;
        }

        {
            int16_t base = 0;

            for (k = 0; k < accumulator.visited_surface_count; k++) {
                int16_t n = accumulator.visited_surfaces[k];

                if (n > 2) {
                    int16_t j = 1;

                    do {
                        out[0] = local_vertices[base];
                        out[1] = local_vertices[base + j];
                        out[2] = local_vertices[base + j + 1];
                        out[3] = local_vertices[base];
                        out[4] = local_vertices[base + j + 1];
                        out[5] = (j + 2 < n) ? local_vertices[base + j + 2] : local_vertices[base];
                        out += 6;
                        j = (int16_t)(j + 2);
                    } while (j + 1 < n);
                }
                base = (int16_t)(base + n);
            }
        }

        ((int32_t (__stdcall *)(void *))(*(void ***)rasterizer_decal_vertex_cache)[0x30 / 4])(
            rasterizer_decal_vertex_cache);

        inherit_geometry = (uint8_t)(*(const uint8_t *)&definition->flags & 1);
        decal_tag_index = halo::objects::tag_handle(definition->next_decal_in_chain);
        halo::rasterizer::globals().vertex_buffer_lock_state = 0;
        if (decal_tag_index == k_datum_index_none) {
            return;
        }
    }
}

}

namespace halo::effects {

void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction, real radius_scale, uint8_t object_attached, int16_t requested_sequence_index)
{
    halo::effects::decal_ref::place(decal_tag_index, placement, direction, radius_scale, object_attached, requested_sequence_index);
}

}
