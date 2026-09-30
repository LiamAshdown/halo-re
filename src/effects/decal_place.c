// decal_place  (Ghidra: FUN_0044edc0; phase 2 guessed "decal_new" -- WRONG, that name belongs to 0x44dd90 which this
// function calls)
// address 0x44edc0, size 6111 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x44edc0..0x4505aa, replacing the 0.15 skeleton whose stopgap
// skipped every decal. Walkthrough, per pass of the next_decal_in_chain loop:
//  1. (unless the previous Decal had geometry_inherited_by_next_decal_in_chain) build the tangent basis: a random
//     rotation around the hit normal (vector3d_build_perpendicular, b = normal x a), or -- no_random_rotation (flag 8)
//     with the incoming direction against the surface -- a basis from the direction (sapien_snap_to_axis, flag 0x20,
//     snaps it to its major axis first, retrying with the direction projected onto the plane when degenerate); the
//     placement matrix is {forward = b cos - a sin, left = b sin + a cos, up = normal, position = hit point}. The
//     sequence is random over the map bitmap's sequences unless the caller passes one, and the radius is
//     lerp(Decal.radius) * radius_scale (0 meaning 1).
//  2. the sprite rectangle / world extent box: a sprite bitmap (type 3) goes through 0x44db30, anything else is
//     {-r, r, -r*aspect, r*aspect} over uv {0, 1, 0, 1}. Non object-attached decals first make sure the bitmap is
//     resident (texture_cache_get, a miss drops the decal).
//  3. decal_build_projection, then decal_flood_surfaces over the BSP from placement->surface_index. Types that allow
//     wrapping (k_decal_type_parameters +0x0c) then take the too-steep fallback surfaces in coplanar groups, fold the
//     placement around the group's edge nearest the decal plane (matrix4x3_from_axis_angle) and flood the group with
//     the folded projection.
//  4. vertices get a 1/256 lift along the averaged normal (when every folded normal stays within 0.5), uv bytes from
//     the sprite rectangle, and each clipped polygon is emitted as 6-vertex blocks of two fan triangles into the
//     decal's vertex cache block (allocated at 64 bytes per block, locked x1.5 = 0x60), then the decal datum shares
//     the block's handle (decal_new EAX).
// blam-cc: stack -> (decal_tag_index, placement, direction, radius_scale, object_attached, sequence_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "projectiles.h"
#include "game.h"
#include "fn_rasterizer.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern tag_instance *tag_instances;                  // 0x0087bc14
extern data_array *decal_data;                       // 0x0087abe4
extern random_seed effect_random_seed;               // 0x00719cd4
extern game_time_globals *game_time; // 0x006f1d6c
extern const decal_type_parameters k_decal_type_parameters[4]; // 0x006573f8
extern cache *rasterizer_decal_vertex_cache_handle;  // 0x0071d1c0, the decal geometry cache
extern void *rasterizer_decal_vertex_cache;          // 0x0071d1bc, IDirect3DVertexBuffer
extern int16_t rasterizer_vertex_buffer_lock_state;  // 0x0069c632

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, EAX out, ECX a, stack b
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern double floor(double x); // 0x623e40, CRT
extern double sqrt(double x);
extern double fabs(double x);
extern double cos(double x);
extern double sin(double x);
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern void *texture_cache_get(void *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // 0x444550, EAX bitmap, stack (wait, allocate_if_missing)
extern int16_t vector3d_major_axis_index(real_vector3d *v); // 0x44d820, EAX
extern void structure_lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale,
    real *out_extent, real *out_sprite_rect, const Decal *decal_definition);
    // 0x44db30 (the decal sprite rectangle builder), EDX out_sprite_rect, EDI decal_definition
extern datum_index decal_new(datum_index requested_handle, int16_t cluster_index, int16_t layer,
    datum_index insert_before, uint8_t object_attached); // 0x44dd90, EAX requested_handle
extern void decal_build_projection(real_matrix4x3 *placement, real *box, decal_projection *out);
    // 0x44e460, EDX placement
extern void decal_flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator,
    int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type,
    int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue,
    uint16_t *fallback_queue_count); // 0x44e730
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cb880, EAX out, ECX axis
extern real vector3d_angle_between_4cd5e0(real_vector3d *a, real_vector3d *b); // 0x4cd5e0, EAX a, ECX b
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670, ECX out, EDX dir
extern datum_index cache_allocate_block(cache *self, uint32_t requested_bytes); // 0x4d1840
extern void cache_evict_entry(datum_index handle, cache *self); // 0x4d1c20, EBX handle, EDI self

    // 0x51a770, EAX decal_index

// One decal vertex as it goes into the vertex cache: position and the u / v bytes at bits 16..23 / 8..15.
typedef struct decal_place_vertex {
    real_point3d position;
    uint32_t texcoord;
} decal_place_vertex; // size 0x10

static real decal_place_random_fraction(void)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(int32_t)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f; // 0x672b84
}

// 0x44f0bb..0x44f1ff: scale to unit length unless shorter than 1e-4
static void decal_place_normalize_guarded(real_vector3d *v)
{
    real length = (real)sqrt((double)(v->k * v->k + v->j * v->j + v->i * v->i));

    if (!(fabs((double)length) < (double)1.0e-4f)) {
        real inverse = 1.0f / length;

        v->i = v->i * inverse;
        v->j = v->j * inverse;
        v->k = v->k * inverse;
    }
}

// 0x44ee7c..0x44ef52 / 0x44efda..0x44f0ad (sapien_snap_to_axis): the unit axis of `axis_source`'s largest
// component, pushed off the surface by the normal (towards the side given by `side`), then a = v x normal and
// b = a x normal. The first attempt takes `side` from dot(normal, direction), the retry from dot(axis, normal).
static void decal_place_snap_basis(real_vector3d *axis_source, const real_vector3d *side_direction,
    real_vector3d *normal, real_vector3d *a, real_vector3d *b)
{
    real_vector3d v;
    int16_t axis = vector3d_major_axis_index(axis_source);
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
    vector3d_normalize_with_length(&v);
    vector3d_cross_product(a, &v, normal);
    vector3d_cross_product(b, a, normal);
}

// A collision BSP plane by signed index (the sign bit selects the back face).
static void decal_place_surface_plane(real_plane3d *out, uint32_t signed_plane_index)
{
    const real_plane3d *plane = &((const real_plane3d *)global_structure_collision_bsp->planes.pointer)
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

// w rotated (not translated) by m
static void decal_place_rotate(real_vector3d *out, const real_matrix4x3 *m, const real_vector3d *w)
{
    out->i = m->up.i * w->k + m->left.i * w->j + m->forward.i * w->i;
    out->j = m->up.j * w->k + m->left.j * w->j + m->forward.j * w->i;
    out->k = m->up.k * w->k + m->left.k * w->j + m->forward.k * w->i;
}

// 0x44f55d..0x44fe95: takes fallback_queue[first] and every later fallback surface within maximum_edge_angle of its
// plane as one group, folds the placement around the group edge closest to the decal plane and floods the group
// with the folded projection. Returns the group size (the entries are consumed, set to -1).
static int16_t decal_place_wrap_group(int32_t *fallback_queue, int16_t fallback_count, int16_t first,
    const Decal *definition, decal_projection *projection, const real_matrix4x3 *matrix, real *box, real radius,
    decal_flood_accumulator *accumulator, real_vector3d *normal_min, real_vector3d *normal_max)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)global_structure_collision_bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges;
    ModelCollisionGeometryBSPVertex *vertices;
    real maximum_angle = k_decal_type_parameters[definition->type].maximum_edge_angle * 0.017453292f;
    int32_t group[0x400];                   // ebp-0x1338
    int16_t group_count = 0;
    real_plane3d first_plane;               // ebp-0x1a0
    real_plane3d best_plane;                // ebp-0xf0
    real_point3d edge_start;                // ebp-0xa8, the far vertex of the chosen edge
    real_point3d edge_end;                  // ebp-0x1c0
    real best_low = 0.0f;                   // ebp-0x1c4
    real best_high = 0.0f;                  // ebp-0x1e4
    int32_t best_surface = -1;              // ebp-0x68
    real_vector3d *decal_normal = (real_vector3d *)&projection->transformed_i;
    decal_projection wrapped;               // ebp-0x338
    int16_t j;
    int16_t g;

    decal_place_surface_plane(&first_plane, surfaces[fallback_queue[first]].plane);
    group[group_count] = fallback_queue[first];
    fallback_queue[first] = -1;
    group_count++;

    for (j = (int16_t)(first + 1); j < fallback_count; j++) {
        if (fallback_queue[j] != -1) {
            real_plane3d plane;             // ebp-0x190

            decal_place_surface_plane(&plane, surfaces[fallback_queue[j]].plane);
            if (vector3d_angle_between_4cd5e0(&plane.normal, &first_plane.normal) <= maximum_angle) {
                group[group_count] = fallback_queue[j];
                group_count++;
                fallback_queue[j] = -1;
            }
        }
    }

    edges = (ModelCollisionGeometryBSPEdge *)global_structure_collision_bsp->edges.pointer;
    vertices = (ModelCollisionGeometryBSPVertex *)global_structure_collision_bsp->vertices.pointer;
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
            real low = (real)fabs((double)(projection->transformed_j * far_point->y +
                projection->transformed_k * far_point->z + projection->transformed_i * far_point->x -
                projection->transformed_d));
            real high = (real)fabs((double)(projection->transformed_j * near_point->y +
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
        real_vector3d axis;                 // ebp-0x54
        real length;

        axis.i = edge_end.x - edge_start.x;
        axis.j = edge_end.y - edge_start.y;
        axis.k = edge_end.z - edge_start.z;
        length = (real)sqrt((double)(axis.i * axis.i + axis.k * axis.k + axis.j * axis.j));

        if (fabs((double)length) < (double)1.0e-4f || !(length > 0.0f)) {
            wrapped = *projection;          // 0x44fe37: fold nothing
        } else {
            real inverse = 1.0f / length;
            real_vector3d crossed;          // ebp-0x1d0
            real sign = 1.0f;               // ebp-0x12c
            real angle;
            real_matrix4x3 rotation;        // ebp-0x128
            real_matrix4x3 folded;          // ebp-0x170
            real_vector3d offset;           // ebp-0x148

            axis.i = axis.i * inverse;
            axis.j = axis.j * inverse;
            axis.k = axis.k * inverse;

            crossed.i = best_plane.normal.j * decal_normal->k - best_plane.normal.k * decal_normal->j;
            crossed.j = best_plane.normal.k * decal_normal->i - decal_normal->k * best_plane.normal.i;
            crossed.k = decal_normal->j * best_plane.normal.i - best_plane.normal.j * decal_normal->i;
            if (!(crossed.i * axis.i + crossed.k * axis.k + crossed.j * axis.j < 0.0f)) {
                sign = -1.0f;
            }
            angle = vector3d_angle_between_4cd5e0(decal_normal, &best_plane.normal) * sign;
            matrix4x3_from_axis_angle(&rotation, &axis, (real)sin((double)angle), (real)cos((double)angle));

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

            decal_build_projection(&folded, box, &wrapped);

            if (!(folded.up.i > normal_min->i)) normal_min->i = folded.up.i;
            if (folded.up.i > normal_max->i) normal_max->i = folded.up.i;
            if (!(folded.up.j > normal_min->j)) normal_min->j = folded.up.j;
            if (folded.up.j > normal_max->j) normal_max->j = folded.up.j;
            if (!(folded.up.k > normal_min->k)) normal_min->k = folded.up.k;
            if (folded.up.k > normal_max->k) normal_max->k = folded.up.k;
        }
    }

    for (g = 0; g < group_count; g++) {
        decal_flood_surfaces(&wrapped, accumulator, group[g], 0, radius, (int16_t)definition->type, 0, 0, 0, 0);
    }
    return group_count;
}

void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction,
    real radius_scale, uint8_t object_attached, int16_t requested_sequence_index)
{
    uint8_t inherit_geometry = 0;
    int16_t sequence_index = 0;             // ebp-0x5c
    int16_t sprite_index = 0;               // ebp-0x1ec
    real radius = 0.0f;                     // ebp-0x44
    real_matrix4x3 matrix;                  // ebp-0xe0
    real sprite_rect[4];                    // ebp-0x180: left, right, top, bottom
    real box[4];                            // ebp-0x20c
    decal_projection projection;            // ebp-0x2a8
    decal_flood_accumulator accumulator;    // ebp-0xcb40
    int32_t surface_queue[0x400];           // ebp-0x7338
    int32_t fallback_queue[0x400];          // ebp-0x2338
    decal_place_vertex local_vertices[0x400]; // ebp-0x6338

    if (decal_tag_index == k_datum_index_none) {
        return;
    }

    for (;;) {
        Decal *definition = (Decal *)tag_instances[(uint16_t)decal_tag_index].data;
        Bitmap *bitmap = (Bitmap *)tag_instances[*(uint16_t *)&definition->map.tag_id].data;
        int16_t sprite_bitmap_index;        // ebp-0x28 (word)
        uint16_t queue_count;               // ebp-0x2c
        uint16_t fallback_count = 0;        // ebp-0x64
        int16_t queue_cursor;
        real_vector3d normal_min;           // ebp-0x9c / -0x94 / -0x8c
        real_vector3d normal_max;           // ebp-0x98 / -0x90 / -0x88
        real_vector3d lift;                 // ebp-0x80
        int32_t block_count = 0;            // ebp-0x14
        datum_index geometry_handle;
        datum_index decal_index;
        decal *self;
        decal_place_vertex *out;
        int16_t k;

        if (!inherit_geometry) {
            real_vector3d *normal = &placement->plane.normal;
            real_vector3d a;                // ebp-0x20
            real_vector3d b;                // ebp-0x38
            real rotation_cos;              // ebp-0x3c
            real rotation_sin;              // ebp-0x40

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
                    vector3d_cross_product(&a, direction, normal);
                    vector3d_cross_product(&b, &a, normal);
                }
            } else {
                real angle = decal_place_random_fraction() * 6.2831855f; // 0x672c20

                rotation_cos = (real)cos((double)angle);
                rotation_sin = (real)sin((double)angle);
                vector3d_build_perpendicular(&a, normal);
                b.i = a.k * normal->j - a.j * normal->k;
                b.j = a.i * normal->k - a.k * normal->i;
                b.k = a.j * normal->i - a.i * normal->j;
            }
            decal_place_normalize_guarded(&a);
            decal_place_normalize_guarded(&b);

            matrix.scale = 1.0f; // never written by the original; nothing reads it
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

                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                roll = (int32_t)((uint32_t)((int32_t)count * (int32_t)(effect_random_seed >> k_random_value_shift)) >> 16);
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

        if (bitmap->type == 3) { // sprites
            const BitmapGroupSequence *sequence =
                &((const BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer)[sequence_index];

            sprite_bitmap_index =
                (int16_t)((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index].bitmap_index;
            structure_lightmap_uv_rect_build(sequence_index, 0, radius, box, sprite_rect, definition);
        } else {
            real aspect = 1.0f;

            sprite_bitmap_index = 0;
            if ((definition->flags & 0x100) != 0) { // preserve_aspect
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
            texture_cache_get(&((BitmapData *)bitmap->bitmap_data.pointer)[sprite_bitmap_index], 0, 1) == 0) {
            return;
        }

        decal_build_projection(&matrix, box, &projection);

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
            decal_flood_surfaces(&projection, &accumulator, surface_index, 1, radius, (int16_t)definition->type,
                surface_queue, &queue_count, fallback_queue, &fallback_count);
        } while (queue_cursor < (int16_t)queue_count);

        if (*(const uint8_t *)&k_decal_type_parameters[definition->type].use_fallback_surfaces != 0 &&
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
            vector3d_normalize_with_length(&lift);
            lift.i = lift.i * 0.00390625f;
            lift.j = lift.j * 0.00390625f;
            lift.k = lift.k * 0.00390625f;
        }

        for (k = 0; k < accumulator.visited_surface_count; k++) {
            block_count += ((int32_t)accumulator.visited_surfaces[k] - 1) / 2;
        }

        geometry_handle = cache_allocate_block(rasterizer_decal_vertex_cache_handle,
            (uint32_t)((int32_t)(int16_t)block_count << 6));
        if (geometry_handle == k_datum_index_none) {
            return;
        }
        decal_index = decal_new(geometry_handle, placement->leaf.cluster_index, (int16_t)definition->layer,
            k_datum_index_none, object_attached);
        if (decal_index == k_datum_index_none) {
            cache_evict_entry(geometry_handle, rasterizer_decal_vertex_cache_handle);
            return;
        }
        self = &((decal *)decal_data->data)[(uint16_t)decal_index];
        out = (decal_place_vertex *)rasterizer_decal_vertex_cache_lock(geometry_handle,
            (int32_t)(int16_t)block_count << 6);
        if (out == 0) {
            cache_evict_entry(geometry_handle, rasterizer_decal_vertex_cache_handle);
            return;
        }

        {
            real u_span = sprite_rect[1] - sprite_rect[0];
            real v_span = sprite_rect[3] - sprite_rect[2];
            int32_t count = (uint16_t)accumulator.vertex_count;
            int32_t i;

            for (i = 0; i < count; i++) {
                const decal_flood_vertex_record *record = &accumulator.vertices[i];
                real u = u_span * record->u + sprite_rect[0];
                real v = v_span * record->v + sprite_rect[2];
                int16_t u_byte;
                int16_t v_byte;

                if (u < 0.0f) {
                    u = 0.0f;
                } else if (u > 1.0f) {
                    u = 1.0f;
                }
                if (v < 0.0f) {
                    v = 0.0f;
                } else if (v > 1.0f) {
                    v = 1.0f;
                }
                u = u * 255.0f;
                if (u < 0.0f) {
                    u = 0.0f;
                } else if (u > 254.0f) {
                    u = 254.0f;
                }
                u_byte = (int16_t)lrint((double)(real)floor((double)(u + 0.5f)));
                v = v * 255.0f;
                if (v < 0.0f) {
                    v = 0.0f;
                } else if (v > 254.0f) {
                    v = 254.0f;
                }
                v_byte = (int16_t)lrint((double)(real)floor((double)(v + 0.5f)));

                local_vertices[i].texcoord = (uint32_t)((((int32_t)u_byte << 8) | (int32_t)v_byte) << 8);
                local_vertices[i].position.x = lift.i + record->position.x;
                local_vertices[i].position.y = lift.j + record->position.y;
                local_vertices[i].position.z = lift.k + record->position.z;
            }
        }

        self->position = placement->point;
        self->creation_game_time = game_time->game_time;
        self->sequence_index = (uint8_t)sequence_index;
        self->unknown_1b = (uint8_t)sprite_bitmap_index;
        self->unknown_1a = 0;
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

            color_interpolate(&definition->color_upper_bounds, &definition->color_lower_bounds, &color,
                (uint32_t)((*(const uint8_t *)&definition->flags >> 1) & 3), fraction);
            self->color = ((uint32_t)lrint((double)(color.blue * 255.0f)) & 0xff) |
                (((uint32_t)lrint((double)(color.green * 255.0f)) & 0xff) << 8) |
                (((uint32_t)lrint((double)(color.red * 255.0f)) & 0xff) << 16) |
                ((uint32_t)lrint((double)(intensity * 255.0f)) << 24);
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

        // 0x45056c: IDirect3DVertexBuffer::Unlock
        ((int32_t (__stdcall *)(void *))(*(void ***)rasterizer_decal_vertex_cache)[0x30 / 4])(
            rasterizer_decal_vertex_cache);

        inherit_geometry = (uint8_t)(*(const uint8_t *)&definition->flags & 1);
        decal_tag_index = *(datum_index *)&definition->next_decal_in_chain.tag_id;
        rasterizer_vertex_buffer_lock_state = 0;
        if (decal_tag_index == k_datum_index_none) {
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x44edc0):

void FUN_0044edc0(uint param_1,int param_2,float *param_3,float param_4,char param_5,uint param_6)

{
  float fVar1;
  ushort uVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  ushort *puVar6;
  byte bVar7;
  short sVar8;
  float *pfVar9;
  float *pfVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  ushort *puVar14;
  float *pfVar15;
  float fVar16;
  undefined4 *puVar17;
  uint uVar18;
  undefined4 *puVar19;
  bool bVar20;
  float10 fVar21;
  float10 fVar22;
  float afStackY_2133c [20976];
  undefined1 local_cb44 [16];
  float local_cb34 [5116];
  ushort local_7b44;
  ushort local_7b42 [1024];
  ushort local_7342;
  undefined4 local_733c [1024];
  float local_633c [4096];
  float local_233c [1024];
  float local_133c [1024];
  undefined4 local_33c [36];
  undefined4 local_2ac [17];
  float local_268;
  float local_264;
  float local_260;
  float local_25c;
  float local_21c;
  float local_218;
  float local_214;
  float local_210;
  float local_20c;
  float local_208;
  float local_204;
  int local_200;
  float local_1fc;
  float local_1f8;
  float local_1f4;
  undefined4 local_1f0;
  int local_1ec;
  float local_1e8;
  float local_1e4;
  float local_1e0;
  float local_1dc;
  int *local_1d8;
  float local_1d4;
  float local_1d0;
  float local_1cc;
  float local_1c8;
  float local_1c4;
  float local_1c0;
  float local_1bc;
  float local_1b4;
  float local_1b0;
  undefined4 local_1ac;
  int local_1a8;
  float local_1a4;
  float local_1a0;
  float local_19c;
  float local_198;
  float local_194;
  float local_190;
  float local_18c;
  float local_188;
  float local_184;
  float local_180;
  float local_17c;
  float local_178;
  undefined4 local_174;
  float local_170;
  float local_16c;
  float local_168;
  float local_164;
  float local_160;
  float local_15c;
  float local_158;
  float local_154;
  float local_150;
  float local_14c;
  float local_148;
  float local_144;
  int local_13c;
  int local_138;
  uint local_134;
  float local_130;
  float local_12c;
  float local_128;
  float local_124;
  float local_120;
  float local_11c;
  float local_118;
  float local_114;
  float local_110;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f4;
  float local_f0;
  float local_ec;
  float local_e8;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  float local_d0;
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
  uint local_88;
  float local_84;
  float local_80;
  float local_7c;
  uint local_78;
  uint local_74;
  float *local_70;
  float *local_6c;
  float *local_68;
  ushort *local_64;
  uint local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float *local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float *local_30;
  ushort *local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float *local_18;
  float local_14 [3];
  float *local_8;

  [see out/phase4/effects_functions.md and tools/pack.py 0x44edc0 for the full 610-line body --
  omitted from this #if 0 block to keep this file a reasonable size; every offset this rewrite
  actually depends on is quoted and explained in the header comment and inline UNSURE notes
  above. The only material control-flow shape not reproduced above is roughly 450 lines covering
  the fallback/rejected-surface re-flood pass, the world-space triangle-fan conversion, and the
  rasterizer geometry packing -- see the file header UNSURE list.]
}
#endif
