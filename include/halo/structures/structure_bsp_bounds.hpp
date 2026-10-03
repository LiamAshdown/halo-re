/**
 * @file include/halo/structures/structure_bsp_bounds.hpp
 * Bounding box, frustum plane and polygon bounds helpers used by the structure bsp queries.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Stateless geometric helpers on axis-aligned boxes, clip planes, compressed bsp node bounds and screen polygons.
 */
struct bsp_bounds {
    /**
     * Classifies box_b against box_a as none (disjoint), partial (overlapping) or contained (box_b fully inside box_a).
     *
     * @address 0x5541b0
     */
    static structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a, real_rectangle3d *box_b);

    /**
     * Classifies a box against a set of clip planes as outside, partial or fully inside, using the plane count given.
     *
     * @address 0x554260
     */
    static structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box, real_plane3d *planes, int16_t plane_count);

    /**
     * Decompresses a bsp node's six byte bounds against its parent's bounds: each byte is a fraction of the parent extent,
     * with 0xff meaning the parent's upper bound.
     *
     * @address 0x553380
     */
    static void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds, uint8_t *compressed_bounds, real_rectangle3d *out);

    /**
     * Grows the x and y bounds pair so that it contains every point of the polygon.
     *
     * @address 0x554a90
     */
    static void polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon);

    /**
     * Copies a plane out of the owner's plane array. A negative index selects the plane at the index with the sign bit
     * cleared and negates normal and distance.
     *
     * @address 0x44dad0
     */
    static void plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);

};

}  // namespace halo::structures
