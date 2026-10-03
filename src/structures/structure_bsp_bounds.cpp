/**
 * @file src/structures/structure_bsp_bounds.cpp
 * Bounding box, frustum plane and polygon bounds helpers used by the structure bsp queries.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"

namespace halo::structures {

structure_bsp_overlap bsp_bounds::aabb_overlap_classify(real_rectangle3d *box_a, real_rectangle3d *box_b)
{
    if (box_b->x.lower <= box_a->x.upper && box_a->x.lower <= box_b->x.upper &&
        box_b->y.lower <= box_a->y.upper && box_a->y.lower <= box_b->y.upper &&
        box_b->z.lower <= box_a->z.upper && box_a->z.lower <= box_b->z.upper) {
        if (box_a->x.lower <= box_b->x.lower && box_b->x.upper <= box_a->x.upper &&
            box_a->y.lower <= box_b->y.lower && box_b->y.upper <= box_a->y.upper &&
            box_a->z.lower <= box_b->z.lower && box_b->z.upper <= box_a->z.upper) {
            return _structure_bsp_overlap_contained;
        }
        return _structure_bsp_overlap_partial;
    }
    return _structure_bsp_overlap_none;
}

structure_bsp_overlap bsp_bounds::frustum_planes_classify_box(real_rectangle3d *box, real_plane3d *planes, int16_t plane_count)
{
    uint8_t any_corner_outside = 0;

    for (int16_t p = 0; p < plane_count; p++) {
        real_plane3d *plane = (real_plane3d *)((uint8_t *)planes + p * sizeof(real_plane3d));
        float nx = plane->normal.i, ny = plane->normal.j, nz = plane->normal.k, d = plane->d;

        float xs[2] = { box->x.lower, box->x.upper };
        float ys[2] = { box->y.lower, box->y.upper };
        float zs[2] = { box->z.lower, box->z.upper };
        uint8_t outside_mask = 0;
        for (int corner = 0; corner < 8; corner++) {
            float cx = xs[corner & 1];
            float cy = ys[(corner >> 1) & 1];
            float cz = zs[(corner >> 2) & 1];
            if ((nx * cx + ny * cy + nz * cz) - d < 0.0f) {
                outside_mask |= (uint8_t)(1 << corner);
            }
        }
        if (outside_mask == k_box_all_corners_mask) {
            return _structure_bsp_overlap_none;
        }
        any_corner_outside |= outside_mask;
    }
    if (any_corner_outside != 0) {
        return _structure_bsp_overlap_partial;
    }
    return _structure_bsp_overlap_contained;
}

void bsp_bounds::bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds, uint8_t *compressed_bounds, real_rectangle3d *out)
{
    const real_bounds *ranges = &parent_bounds->x;
    real_bounds *outputs = &out->x;
    const uint8_t *bytes = compressed_bounds;
    int32_t axis;

    for (axis = 0; axis < 3; axis = axis + 1) {
        float lower = ranges[axis].lower;
        float upper = ranges[axis].upper;
        uint8_t low_byte = bytes[axis * 2];
        uint8_t high_byte = bytes[axis * 2 + 1];

        outputs[axis].lower = (low_byte == k_compressed_bound_none) ? upper : (float)low_byte * k_compressed_bound_scale * (upper - lower) + lower;
        outputs[axis].upper = (high_byte == k_compressed_bound_none) ? upper : (float)high_byte * k_compressed_bound_scale * (upper - lower) + lower;
    }
}

void bsp_bounds::polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon)
{
    for (int16_t i = 0; i < polygon->point_count; i++) {
        real_point2d *p = &polygon->points[i];
        if (p->x < bounds_xy[0].lower) { bounds_xy[0].lower = p->x; }
        if (bounds_xy[0].upper < p->x) { bounds_xy[0].upper = p->x; }
        if (p->y < bounds_xy[1].lower) { bounds_xy[1].lower = p->y; }
        if (bounds_xy[1].upper < p->y) { bounds_xy[1].upper = p->y; }
    }
}

void bsp_bounds::plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index)
{
    real_plane3d *plane = (real_plane3d *)((uint8_t *)*(void **)((uint8_t *)planes_owner + k_plane_owner_planes_offset) +
        (signed_index & k_index_magnitude_mask) * sizeof(real_plane3d));

    if (signed_index < 0) {
        out->normal.i = -plane->normal.i;
        out->normal.j = -plane->normal.j;
        out->normal.k = -plane->normal.k;
        out->d = -plane->d;
    } else {
        out->normal.i = plane->normal.i;
        out->normal.j = plane->normal.j;
        out->normal.k = plane->normal.k;
        out->d = plane->d;
    }
}

}  // namespace halo::structures
