/**
 * @file src/math/polygon.cpp
 * 2d/3d polygons: sutherland-hodgman clipping, convex hull, containment.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"

#include "tags.h"

namespace halo::math {

int16_t polygon2d_points_classify(real_point2d *points, int16_t count)
{
    int16_t state;
    int16_t i;
    real_point2d first_point;
    real_plane2d edge;
    real_point2d *p;

    state = -1;
    i = 0;
    while (i < count) {
        if (state == -1) {
            first_point = points[i];
            state = 0;
        } else if (state == 0) {
            if (plane2d_from_points(&edge, first_point, points[i]) != 0) {
                state = 1;
            }
        } else if (state == 1) {
            p = &points[i];
            if (0.0001 <= fabs((double)((edge.normal.i * p->x + edge.normal.j * p->y) - edge.d))) {
                state = 2;
            }
        }
        i = i + 1;
        if (2 <= state) {
            break;
        }
    }
    return state;
}

int16_t polygon2d_convex_hull_build(real_point2d *points, int16_t count, int16_t *hull)
{
    int16_t classification;
    int16_t hull_count;
    int16_t current;
    int16_t start;
    int16_t i;
    real min_y;
    real min_x;
    double cumulative_angle;
    int duplicate_seen;

    classification = polygon2d_points_classify(points, count);
    if (classification != 2) {
        return 0;
    }

    min_y = 3.4028235e+38f;
    min_x = 3.4028235e+38f;
    start = 0;
    for (i = 0; i < count; i++) {
        real y = points[i].y;
        real x = points[i].x;
        if ((y < min_y - 0.0001f) ||
            (y < min_y && x < min_x + 0.0001f) ||
            (y < min_y + 0.0001f && x < min_x - 0.0001f)) {
            min_x = x;
            min_y = y;
            start = i;
        }
    }

    hull_count = 0;
    current = start;
    cumulative_angle = 0.0;
    duplicate_seen = 0;

    for (;;) {
        real_point2d *cur_pt;
        int16_t best_next;
        double best_angle;

        hull[hull_count] = current;
        hull_count++;
        if (!(hull_count < count)) {
            break;
        }

        best_angle = 3.4028235e+38;
        best_next = current;
        cur_pt = &points[current];
        for (i = 0; i < count; i++) {
            real_point2d *cand = &points[i];
            if (cand->x != cur_pt->x || cand->y != cur_pt->y) {
                double angle = atan2((double)(cand->y - cur_pt->y), (double)(cand->x - cur_pt->x));
                double delta = angle - cumulative_angle;
                while (delta < -0.0001) {
                    delta += 6.2831855;
                }
                if (delta < best_angle) {
                    best_angle = delta;
                    best_next = i;
                }
            }
        }
        cumulative_angle += best_angle;
        current = best_next;

        if (!duplicate_seen) {
            if (0.0001f <= (real)fabs((double)(points[current].x - points[hull[0]].x)) ||
                0.0001f <= (real)fabs((double)(points[current].y - points[hull[0]].y))) {
                duplicate_seen = 1;
            }
        }
        if (current == hull[0]) {
            return hull_count;
        }
        if (!duplicate_seen) {
            continue;
        }
        if (0.0001f <= (real)fabs((double)(points[current].x - points[hull[0]].x)) ||
            0.0001f <= (real)fabs((double)(points[current].y - points[hull[0]].y))) {
            continue;
        }
        return hull_count;
    }

    {
        int16_t trim = (int16_t)(hull_count - 2);
        int16_t n;

        if (trim < 1) {
            return count;
        }
        while (hull[trim] != hull[hull_count - 1]) {
            trim--;
            if (trim < 1) {
                return count;
            }
        }
        n = (int16_t)((hull_count - 1) - trim);
        if (0 < n) {
            for (i = 0; i < n; i++) {
                hull[i] = hull[trim + i];
            }
            return n;
        }
        return count;
    }
}

uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count, const real_point2d &point, real tolerance)
{
    int16_t i;
    int16_t next;
    real edge_dx, edge_dy, dist_sq, cross;
    real tolerance_sq = tolerance * tolerance;

    if (0 < count) {
        for (i = 0; i < count; i++) {
            next = (int16_t)(i + 1);
            if (count <= next) {
                next = 0;
            }
            edge_dx = vertices[next].x - vertices[i].x;
            edge_dy = vertices[next].y - vertices[i].y;
            dist_sq = edge_dx * edge_dx + edge_dy * edge_dy;
            if (dist_sq != 0.0f) {
                cross = (point.x - vertices[i].x) * edge_dy - edge_dx * (point.y - vertices[i].y);
                if (0.0f < cross) {
                    if (dist_sq * tolerance_sq < cross * cross) {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, const real_point2d &point, real margin)
{
    int16_t i;
    int16_t next;
    real edge_dx, edge_dy, cross;

    if (0 < count) {
        for (i = 0; i < count; i++) {
            next = (int16_t)(i + 1);
            if (count <= next) {
                next = 0;
            }
            edge_dx = vertices[next].x - vertices[i].x;
            edge_dy = vertices[next].y - vertices[i].y;
            cross = edge_dx * (point.y - vertices[i].y) - (point.x - vertices[i].x) * edge_dy;
            if (cross < -margin) {
                return 0;
            }
        }
        return 1;
    }
    return 1;
}

int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices, int16_t clip_point_count, real_point2d *clip_points, int16_t maximum_count, real_point2d *out, real epsilon)
{
    real_plane2d edge;
    real_point2d scratch[2][512];
    int16_t i;
    int16_t previous;
    real_point2d *dest;
    int16_t j;

    if (clip_point_count <= 0) {
        return vertex_count;
    }

    i = 0;
    while (0 < vertex_count) {
        previous = (i == 0) ? (int16_t)(clip_point_count - 1) : (int16_t)(i - 1);

        dest = out;
        if ((int32_t)i != (int32_t)clip_point_count - 1) {
            dest = scratch[i & 1];
        }

        if (plane2d_from_points(&edge, clip_points[previous], clip_points[i]) == 0) {
            for (j = 0; j < vertex_count; j++) {
                dest[j] = vertices[j];
            }
        } else {
            vertex_count = polygon2d_clip_to_plane(dest, vertex_count, vertices, edge,
                                                    maximum_count, 0, 0, epsilon);
            if (vertex_count == -1) {
                return -1;
            }
        }

        i = i + 1;
        vertices = dest;
        if (clip_point_count <= i) {
            return vertex_count;
        }
    }
    return vertex_count;
}

int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in, const real_plane2d &plane, int16_t max_count, uint32_t *edge_bitmask, uint8_t *clipped_flag, real epsilon)
{
    real_point2d scratch[512];
    real_point2d *prev;
    real_point2d *cur;
    int16_t output_count;
    int16_t next_count;
    int16_t i;
    uint32_t edge_bits;
    int saw_kept_vertex;
    int saw_discarded_vertex;
    int prev_side;
    int cur_side;
    real dist;

    output_count = 0;
    saw_kept_vertex = 0;
    saw_discarded_vertex = 0;
    edge_bits = 0;
    if (clipped_flag != 0) {
        *clipped_flag = 0;
    }
    if (in == out) {
        for (i = 0; i < count; i++) {
            scratch[i] = in[i];
        }
        in = scratch;
    }

    prev = &in[count - 1];
    prev_side = (0.0f <= (prev->x * plane.normal.i + prev->y * plane.normal.j) - plane.d);

    if (count < 1) {
        output_count = 0;
        goto after_loop;
    }

    for (i = 0; i < count; i++) {
        cur = &in[i];
        dist = (plane.normal.i * cur->x + cur->y * plane.normal.j) - plane.d;
        cur_side = (0.0f <= dist);
        if (dist <= epsilon) {
            if (dist < -epsilon) {
                saw_discarded_vertex = 1;
            }
        } else {
            saw_kept_vertex = 1;
        }
        next_count = output_count;
        if (cur_side != prev_side) {
            real denom, t;

            if (output_count == max_count) {
                output_count = -1;
                goto overflow;
            }
            if (clipped_flag != 0) {
                *clipped_flag = 1;
            }
            denom = (prev->y - cur->y) * plane.normal.j + (prev->x - cur->x) * plane.normal.i;
            if (denom == 0.0f) {
                t = 0.0f;
            } else {
                t = (-1.0f / denom) * ((plane.normal.i * cur->x + cur->y * plane.normal.j) - plane.d);
                if (0.0f <= t) {
                    if (1.0f < t) {
                        t = 1.0f;
                    }
                } else {
                    t = 0.0f;
                }
            }
            out[output_count].x = t * (prev->x - cur->x) + cur->x;
            edge_bits |= 1u << (output_count & 0x1f);
            next_count = (int16_t)(output_count + 1);
            out[output_count].y = t * (prev->y - cur->y) + cur->y;
            if (next_count != 1 &&
                (((real)fabs((double)(out[next_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[0].y)) < epsilon) ||
                 ((real)fabs((double)(out[next_count - 1].x - out[next_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[next_count - 2].y)) < epsilon))) {
                next_count = output_count;
            }
            goto after_edge;
        }
    after_edge:
        output_count = next_count;
        if (cur_side) {
            if (next_count == max_count) {
                output_count = -1;
                goto overflow;
            }
            out[next_count] = *cur;
            if (edge_bitmask == 0 || ((1u << (i & 0x1f)) & *edge_bitmask) == 0) {
                edge_bits &= ~(1u << (next_count & 0x1f));
            } else {
                edge_bits |= 1u << (next_count & 0x1f);
            }
            output_count = (int16_t)(next_count + 1);
            if (output_count != 1 &&
                ((((real)fabs((double)(out[output_count - 1].x - out[0].x)) < epsilon &&
                   (real)fabs((double)(out[output_count - 1].y - out[0].y)) < epsilon) ||
                  ((real)fabs((double)(out[output_count - 1].x - out[output_count - 2].x)) < epsilon &&
                   (real)fabs((double)(out[output_count - 1].y - out[output_count - 2].y)) < epsilon)))) {
                output_count = next_count;
            }
        }
        prev_side = cur_side;
        prev = cur;
    }

    if (output_count != -1) {
        if (output_count < 3) {
            output_count = 0;
            goto after_loop;
        }
        goto degenerate_check;
    }

overflow:
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    goto done;

after_loop:
degenerate_check:
    if (!saw_kept_vertex) {
        output_count = 0;
        goto done;
    }
    if (saw_discarded_vertex) {
        goto done;
    }
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    output_count = count;

done:
    if (edge_bitmask != 0) {
        *edge_bitmask = edge_bits;
    }
    return output_count;
}

int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, const real_plane3d &plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar)
{
    real_point3d scratch[512];
    real_point3d *prev;
    real_point3d *cur;
    int16_t output_count;
    int16_t next_count;
    int16_t i;
    int saw_kept_vertex;
    char saw_discarded_vertex;
    int prev_side;
    int cur_side;
    real dist;

    output_count = 0;
    saw_kept_vertex = 0;
    saw_discarded_vertex = 0;
    if (clipped_flag != 0) {
        *clipped_flag = 0;
    }
    if (in == out) {
        for (i = 0; i < count; i++) {
            scratch[i] = in[i];
        }
        in = scratch;
    }

    prev = &in[count - 1];
    prev_side = (0.0f <= (plane.normal.i * prev->x + prev->z * plane.normal.k + prev->y * plane.normal.j) - plane.d);

    if (count < 1) {
        output_count = 0;
        goto tail;
    }

    for (i = 0; i < count; i++) {
        cur = &in[i];
        dist = (cur->z * plane.normal.k + plane.normal.j * cur->y + plane.normal.i * cur->x) - plane.d;
        cur_side = (0.0f <= dist);
        if (dist <= epsilon) {
            if (dist < -epsilon) {
                saw_discarded_vertex = 1;
            }
        } else {
            saw_kept_vertex = 1;
        }
        next_count = output_count;
        if (cur_side != prev_side) {
            real denom, t;

            if (output_count == max_count) {
                output_count = -1;
                goto overflow;
            }
            if (clipped_flag != 0) {
                *clipped_flag = 1;
            }
            denom = (prev->z - cur->z) * plane.normal.k + (prev->y - cur->y) * plane.normal.j +
                    (prev->x - cur->x) * plane.normal.i;
            t = -(((cur->z * plane.normal.k + plane.normal.j * cur->y + plane.normal.i * cur->x) - plane.d) / denom);
            if (0.0f <= t) {
                if (1.0f < t) {
                    t = 1.0f;
                }
            } else {
                t = 0.0f;
            }
            out[output_count].x = t * (prev->x - cur->x) + cur->x;
            out[output_count].y = (prev->y - cur->y) * t + cur->y;
            out[output_count].z = t * (prev->z - cur->z) + cur->z;
            next_count = (int16_t)(output_count + 1);
            if (next_count != 1 &&
                (((real)fabs((double)(out[next_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[0].y)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].z - out[0].z)) < epsilon) ||
                 ((real)fabs((double)(out[next_count - 1].x - out[next_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[next_count - 2].y)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].z - out[next_count - 2].z)) < epsilon))) {
                next_count = output_count;
            }
        }
        output_count = next_count;
        if (cur_side) {
            if (max_count <= next_count) {
                output_count = -1;
                goto overflow;
            }
            out[next_count] = *cur;
            output_count = (int16_t)(next_count + 1);
            if (output_count != 1 &&
                (((real)fabs((double)(out[output_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].y - out[0].y)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].z - out[0].z)) < epsilon) ||
                 ((real)fabs((double)(out[output_count - 1].x - out[output_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].y - out[output_count - 2].y)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].z - out[output_count - 2].z)) < epsilon))) {
                output_count = next_count;
            }
        }
        prev_side = cur_side;
        prev = cur;
    }

    if (output_count == -1) {
        goto overflow;
    }
    if (output_count < 3) {
        output_count = 0;
    }

tail:
    if (saw_kept_vertex) {
        if (saw_discarded_vertex != 0) {
            return output_count;
        }
    } else if (saw_discarded_vertex != 0 || keep_coplanar == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    return count;

overflow:
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    return output_count;
}

}  // namespace halo::math
