/**
 * @file include/halo/math/polygon.hpp
 * 2d/3d polygons: sutherland-hodgman clipping, convex hull, containment.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Classifies a point set: -1 empty, 0 all coincident, 1 collinear, 2 general position.
 *
 * Original register convention: EBX -> points, stack -> count.
 * @address 0x004caa40
 */
int16_t polygon2d_points_classify(real_point2d *points, int16_t count);

/**
 * Gift-wraps the convex hull of a point set into `hull` (indices); returns the hull size, or 0 unless the
 * points are in general position.
 *
 * Original register convention: EAX -> points, stack -> (count, hull).
 * @address 0x004caae0
 */
int16_t polygon2d_convex_hull_build(real_point2d *points, int16_t count, int16_t *hull);

/**
 * True when `point` is inside the convex polygon or within `tolerance` (true distance) outside every edge.
 *
 * Original register convention: ECX -> vertices, stack -> (count, point, tolerance).
 * @address 0x004cad80
 */
uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count, const real_point2d &point, real tolerance);

/**
 * True when `point` is inside the convex polygon or outside each edge by less than `margin`, measured as the
 * raw edge cross product.
 *
 * Original register convention: ECX -> vertices, stack -> (count, point, margin).
 * @address 0x004cae60
 */
uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, const real_point2d &point, real margin);

/**
 * Clips a polygon against every edge of a convex clip polygon (each edge built from consecutive clip points),
 * double-buffering between edges. Returns the final count or -1 on overflow.
 *
 * Original register convention: ECX -> vertex_count, EDX -> vertices,.
 * @address 0x004caee0
 */
int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices, int16_t clip_point_count, real_point2d *clip_points, int16_t maximum_count, real_point2d *out, real epsilon);

/**
 * Sutherland-Hodgman clip of a 2D polygon against one line, dropping near-duplicate vertices. Returns the
 * vertex count, 0 when everything is clipped, or -1 on output overflow (the input is then copied through). `in`
 * may alias `out`; edge_bitmask and clipped_flag may be NULL.
 *
 * Original register convention: EDX -> out, stack -> (count, in, plane, max_count, edge_bitmask, clipped_flag,
 * epsilon).
 * @address 0x004caff0
 */
int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in, const real_plane2d &plane, int16_t max_count, uint32_t *edge_bitmask, uint8_t *clipped_flag, real epsilon);

/**
 * Sutherland-Hodgman clip of a 3D polygon against one plane; same contract as polygon2d_clip_to_plane, plus
 * keep_coplanar choosing whether a polygon lying in the plane is kept.
 * @address 0x004cb380
 */
int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, const real_plane3d &plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar);

}  // namespace halo::math
