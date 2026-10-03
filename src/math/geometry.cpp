/**
 * @file src/math/geometry.cpp
 * Planes, rays, segments, spheres, cylinders, triangles: intersection and distance queries.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/glm_interop.hpp"
#include "halo/math/globals.hpp"

#include "tags.h"

namespace halo::math {

uint8_t point3d_within_horizontal_cone(const real_point3d &to_point, const real_point3d &reference, real min_cos_threshold)
{
    real x, y;
    real length;
    real dot;

    x = to_point.x;
    y = to_point.y;
    length = (real)sqrt((double)(y * y + x * x));
    if ((real)fabs((double)length) < 0.0001f) {
        return 0;
    }

    x = x * (1.0f / length);
    y = y * (1.0f / length);

    if (!(length > 0.0f)) {
        return 0;
    }

    dot = x * reference.x + y * reference.y;
    if (!(dot > min_cos_threshold)) {
        return 0;
    }
    return 1;
}

void path_find_closest_point_on_segment(const real_point3d &point, const real_point3d &segment_start, const real_point3d &segment_end, real_point3d &out)
{
    real dx = segment_end.x - segment_start.x;
    real dy = segment_end.y - segment_start.y;
    real dz = segment_end.z - segment_start.z;
    real t;

    t = ((segment_start.y - point.y) * dy +
         (segment_start.z - point.z) * dz +
         (segment_start.x - point.x) * dx) /
        (dy * dy + dx * dx + dz * dz);

    if (!(t < 0.0f) && !(t > 1.0f)) {
        out.x = dx * t + segment_start.x;
        out.y = dy * t + segment_start.y;
        out.z = dz * t + segment_start.z;
        return;
    }
    out.x = segment_end.x;
    out.y = segment_end.y;
    out.z = segment_end.z;
}

int point3d_within_radius(const real_point3d &a, const real_point3d &b, real radius)
{
    real dx = a.x - b.x;
    real dy = a.y - b.y;
    real dz = a.z - b.z;

    if (dx * dx + dz * dz + dy * dy <= radius * radius) {
        return 1;
    }
    return 0;
}

uint8_t ray2d_intersect_circle_distance(const real_vector2d &direction, const real_point2d &origin, const real_point2d &center, real &out_distance, real radius)
{
    real dx = center.x - origin.x;
    real dy = center.y - origin.y;
    real tca = dx * direction.i + dy * direction.j;

    if (tca > 0.0f) {
        real d = (dy * dy + dx * dx) - radius * radius;

        if (d <= 0.0f) {
            out_distance = 0.0f;
            return 1;
        }

        {
            real disc = tca * tca - d;
            if (disc >= 0.0f) {
                out_distance = tca - (real)sqrt((double)disc);
                return 1;
            }
        }
    }
    return 0;
}

void vector2d_tangent_edge_directions(const real_vector2d &direction, real_vector2d &edge_pos, real_vector2d &edge_neg, real distance, real extent, real &adjacent_out)
{
    real sin_ratio = 1.0f;
    real cos_ratio;
    real i, j;

    if (distance != 0.0f) {
        sin_ratio = extent / distance;
        if (sin_ratio > 1.0f) {
            sin_ratio = 1.0f;
        }
    }
    cos_ratio = (real)sqrt((double)(1.0f - sin_ratio * sin_ratio));

    i = cos_ratio * direction.i - (-sin_ratio) * direction.j;
    j = cos_ratio * direction.j + (-sin_ratio) * direction.i;
    edge_neg.i = i;
    edge_neg.j = j;

    j = cos_ratio * direction.j + sin_ratio * direction.i;
    i = cos_ratio * direction.i - sin_ratio * direction.j;
    edge_pos.j = j;
    edge_pos.i = i;

    adjacent_out = cos_ratio * distance;
}

real_point3d * decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis, const real_plane3d *plane, const real_point2d &known)
{
    int32_t axis = (int16_t)dominant_axis;
    const projection_axis_pair *axes = &globals().k_projection_axes[axis * 2 + (component_sign & 0xff)];
    float *out_f = (float *)out;
    const float *plane_f = (const float *)plane;

    out_f[axes->i] = known.x;
    out_f[axes->j] = known.y;

    if ((real)fabs((double)plane_f[axis]) < 0.0001f) {
        out_f[axis] = 0.0f;
    } else {
        out_f[axis] = (plane->d - plane_f[axes->i] * known.x - plane_f[axes->j] * known.y) / plane_f[axis];
    }

    return out;
}

real_plane2d * plane2d_from_points(real_plane2d *out_plane, const real_point2d &a, const real_point2d &b)
{
    float length;

    out_plane->normal.i = a.y - b.y;
    out_plane->normal.j = b.x - a.x;

    length = (float)sqrt((double)(out_plane->normal.j * out_plane->normal.j + out_plane->normal.i * out_plane->normal.i));

    if (length >= 0.0001f) {
        out_plane->normal.i = (1.0f / length) * out_plane->normal.i;
        out_plane->normal.j = (1.0f / length) * out_plane->normal.j;
        if (length != 0.0f) {
            out_plane->d = out_plane->normal.i * b.x + out_plane->normal.j * b.y;
            return out_plane;
        }
    }

    out_plane->d = 0.0f;
    return 0;
}

void plane3d_from_point_and_normal(real_plane3d &out, const real_vector3d &normal, const real_point3d &point)
{
    out.normal = normal;
    out.d = glm::dot(to_glm(out.normal), to_glm(point));
}

void plane3d_negate(real_plane3d &out, const real_plane3d &in)
{
    out.normal = vector_from_glm(-to_glm(in.normal));
    out.d = -in.d;
}

real point3d_distance_squared_to_segment(const real_point3d &segment_start, const real_vector3d &segment_direction, const real_point3d &point)
{
    real t;
    real dx;
    real dy;
    real dz;

    t = ((point.x - segment_start.x) * segment_direction.i +
         (point.y - segment_start.y) * segment_direction.j +
         (point.z - segment_start.z) * segment_direction.k) /
        (segment_direction.k * segment_direction.k + segment_direction.j * segment_direction.j +
         segment_direction.i * segment_direction.i);

    if (t < 0.0f) {
        t = 0.0f;
    } else if (1.0f < t) {
        t = 1.0f;
    }
    t = -t;

    dx = t * segment_direction.i + (point.x - segment_start.x);
    dy = t * segment_direction.j + (point.y - segment_start.y);
    dz = t * segment_direction.k + (point.z - segment_start.z);

    return dx * dx + dy * dy + dz * dz;
}

real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction, real_vector3d *b_direction)
{
    real_vector3d w0;
    real_vector3d cross_ab;
    real s;
    real t;
    real dx, dy, dz;

    w0.i = b_start->x - a_start->x;
    w0.j = b_start->y - a_start->y;
    w0.k = b_start->z - a_start->z;

    cross_ab.i = a_direction->j * b_direction->k - b_direction->j * a_direction->k;
    cross_ab.j = a_direction->k * b_direction->i - a_direction->i * b_direction->k;
    cross_ab.k = b_direction->j * a_direction->i - a_direction->j * b_direction->i;

    if (fabs((double)(cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k)) < 0.0001) {
        real dot_dirs = a_direction->i * b_direction->i + a_direction->j * b_direction->j + a_direction->k * b_direction->k;
        real len_a2 = a_direction->i * a_direction->i + a_direction->j * a_direction->j + a_direction->k * a_direction->k;
        real len_b2 = b_direction->i * b_direction->i + b_direction->j * b_direction->j + b_direction->k * b_direction->k;

        if (len_a2 <= 0.0001f) {
            s = 0.0f;
        } else {
            real inv_a2 = 1.0f / len_a2;
            real s0 = (w0.i * a_direction->i + w0.k * a_direction->k + w0.j * a_direction->j) * inv_a2;
            real s1 = inv_a2 * dot_dirs + s0;
            real s0_clamped = s0;
            if (s0_clamped < 0.0f) { s0_clamped = 0.0f; } else if (1.0f < s0_clamped) { s0_clamped = 1.0f; }
            if (0.0f <= s1 && s1 <= 1.0f) { s = (s1 + s0_clamped) * 0.5f; }
            else if (1.0f < s1) { s = (1.0f + s0_clamped) * 0.5f; }
            else { s = (0.0f + s0_clamped) * 0.5f; }
        }

        if (len_b2 <= 0.0001f) {
            t = 0.0f;
        } else {
            real inv_b2 = 1.0f / len_b2;
            real t0 = -((w0.k * b_direction->k + w0.i * b_direction->i + w0.j * b_direction->j) * inv_b2);
            real t1 = inv_b2 * dot_dirs + t0;
            real t0_clamped = t0;
            if (t0_clamped < 0.0f) { t0_clamped = 0.0f; } else if (1.0f < t0_clamped) { t0_clamped = 1.0f; }
            if (0.0f <= t1 && t1 <= 1.0f) { t = (t1 + t0_clamped) * 0.5f; }
            else if (1.0f < t1) { t = (t0_clamped + 1.0f) * 0.5f; }
            else { t = (t0_clamped + 0.0f) * 0.5f; }
        }
    } else {
        real denom = cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k;
        real raw_s = vector3d_scalar_triple_product(w0, *b_direction, cross_ab) / denom;
        real raw_t = vector3d_scalar_triple_product(w0, *a_direction, cross_ab) / denom;
        int s_out_of_range = raw_s < 0.0f || 1.0f < raw_s;
        int t_out_of_range = raw_t < 0.0f || 1.0f < raw_t;

        if (s_out_of_range || t_out_of_range) {
            real dist_a = 3.4028235e+38f;
            real dist_b = 3.4028235e+38f;

            if (s_out_of_range) {
                real_point3d point_on_a;
                real snapped_s = (0.0f <= raw_s) ? 1.0f : 0.0f;
                point_on_a.x = snapped_s * a_direction->i + a_start->x;
                point_on_a.y = snapped_s * a_direction->j + a_start->y;
                point_on_a.z = snapped_s * a_direction->k + a_start->z;
                dist_a = point3d_distance_squared_to_segment(*b_start, *b_direction, point_on_a);
            }
            if (t_out_of_range) {
                real_point3d point_on_b;
                real snapped_t = (0.0f <= raw_t) ? 1.0f : 0.0f;
                point_on_b.x = snapped_t * b_direction->i + b_start->x;
                point_on_b.y = snapped_t * b_direction->j + b_start->y;
                point_on_b.z = snapped_t * b_direction->k + b_start->z;
                dist_b = point3d_distance_squared_to_segment(*a_start, *a_direction, point_on_b);
            }

            return (dist_a <= dist_b) ? dist_a : dist_b;
        }

        s = raw_s;
        t = raw_t;
    }

    dx = (t * b_direction->i + b_start->x) - (s * a_direction->i + a_start->x);
    dy = (t * b_direction->j + b_start->y) - (s * a_direction->j + a_start->y);
    dz = (t * b_direction->k + b_start->z) - (s * a_direction->k + a_start->z);
    return dx * dx + dy * dy + dz * dz;
}

uint8_t ray_intersects_sphere(const real_point3d &origin, real_vector3d *normal_out, const real_vector3d &direction, real &t_out, const real_point3d &center, real radius)
{
    real oc_i;
    real oc_j;
    real oc_k;
    real dot_oc_dir;

    oc_i = origin.x - center.x;
    oc_j = origin.y - center.y;
    oc_k = origin.z - center.z;

    dot_oc_dir = oc_i * direction.i + oc_k * direction.k + oc_j * direction.j;
    if (dot_oc_dir < 0.0f) {
        real oc_len2 = oc_k * oc_k + oc_i * oc_i + oc_j * oc_j;
        real c = oc_len2 - radius * radius;

        if (c < 0.0f) {
            real inv_len = 1.0f / (real)sqrt((double)oc_len2);
            t_out = 0.0f;
            normal_out->i = oc_i * inv_len;
            normal_out->j = oc_j * inv_len;
            normal_out->k = inv_len * oc_k;
            return 1;
        }

        {
            real dir_len2 = direction.i * direction.i + direction.k * direction.k + direction.j * direction.j;
            real discriminant = dot_oc_dir * dot_oc_dir - dir_len2 * c;

            if (0.0f <= discriminant) {
                real t_numerator = -((real)sqrt((double)discriminant) + dot_oc_dir);
                if (t_numerator <= dir_len2) {
                    real t = t_numerator / dir_len2;
                    t_out = t;
                    normal_out->i = t * direction.i + oc_i;
                    normal_out->j = t * direction.j + oc_j;
                    normal_out->k = t * direction.k + oc_k;
                    vector3d_normalize(*normal_out);
                    return 1;
                }
            }
        }
    }
    return 0;
}

uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out, real_point3d *center, real_point3d *origin, real_vector3d *direction)
{
    real dx;
    real dy;
    real b;
    real c;
    real t;
    uint8_t hit;
    real axial_t;

    dx = origin->x - center->x;
    dy = origin->y - center->y;
    b = dy * direction->j + dx * direction->i;
    c = (dx * dx + dy * dy) - radius * radius;

    if (c < 0.0f) {
        t = 0.0f;
    } else {
        real a = direction->i * direction->i + direction->j * direction->j;
        real discriminant = b * b - a * c;
        real t_numerator;

        if (discriminant < 0.0f) {
            return 0;
        }
        t_numerator = -((real)sqrt((double)discriminant) + b);
        if (a < t_numerator) {
            return 0;
        }
        t = t_numerator / a;
    }

    hit = 1;
    axial_t = (t * direction->k + (origin->z - center->z)) / (height * height);

    if (0.0f <= axial_t) {
        if (axial_t <= 1.0f) {
            if (0.0f <= b) {
                return 0;
            }
            *t_out = t;
            hit_out->i = t * direction->i + dx;
            hit_out->j = t * direction->j + dy;
            vector2d_normalize(*((real_vector2d *)hit_out));
            hit_out->k = 0.0f;
        } else {
            real_point3d top_center;
            top_center.x = center->x;
            top_center.y = center->y;
            top_center.z = height + center->z;
            hit = ray_intersects_sphere(*origin, hit_out, *direction, *t_out, top_center, radius);
        }
    } else {
        hit = ray_intersects_sphere(*origin, hit_out, *direction, *t_out, *center, radius);
    }

    if (hit == 0) {
        return 0;
    }
    if (direction->i * hit_out->i + hit_out->j * direction->j + hit_out->k * direction->k <= 0.0f) {
        return hit;
    }
    return 0;
}

uint8_t ray_intersects_sphere_test(const real_point3d &center, const real_point3d &origin, const real_vector3d &direction, real radius)
{
    real from_center_i;
    real from_center_j;
    real from_center_k;
    real c;

    from_center_i = origin.x - center.x;
    from_center_j = origin.y - center.y;
    from_center_k = origin.z - center.z;
    c = (from_center_i * from_center_i + from_center_j * from_center_j + from_center_k * from_center_k) - radius * radius;

    if (c < 0.0f) {
        return 1;
    }

    {
        real b = direction.i * from_center_i + direction.j * from_center_j + direction.k * from_center_k;
        if (b < 0.0f) {
            real a = direction.i * direction.i + direction.j * direction.j + direction.k * direction.k;
            real discriminant = b * b - a * c;
            if (0.0f < discriminant) {
                real near_no_sqrt = -a - b;
                if (0.0f <= near_no_sqrt) {
                    return (discriminant <= near_no_sqrt * near_no_sqrt) ? 0 : 1;
                }
                return 1;
            }
        }
    }
    return 0;
}

real ray_intersect_sphere_distance(const real_point3d &origin, const real_point3d &center, const real_vector3d &direction, real radius)
{
    real to_center_i;
    real to_center_j;
    real to_center_k;
    real c;

    to_center_i = center.x - origin.x;
    to_center_j = center.y - origin.y;
    to_center_k = center.z - origin.z;
    c = (to_center_i * to_center_i + to_center_j * to_center_j + to_center_k * to_center_k) - radius * radius;

    if (0.0f <= c) {
        real b = direction.i * to_center_i + direction.j * to_center_j + direction.k * to_center_k;
        if (b < 0.0f) {
            real a = direction.i * direction.i + direction.j * direction.j + direction.k * direction.k;
            real discriminant = b * b - a * c;
            if (0.0f < discriminant) {
                return (-b - (real)sqrt((double)discriminant)) / a;
            }
        }
    } else {
        return 0.0f;
    }
    return 3.4028235e+38f;
}

uint8_t triangle_point_barycentric_2d(const real_point3d &a, const real_point3d &v_ecx, const real_point3d &v_edx, const real_point3d &p, real &out_u, real &out_v)
{
    real e0[3];
    real e1[3];
    real e2[3];
    real n[3];
    real dot_n_e0;
    real plane_tolerance;
    int dominant_axis;
    int axis_index;
    int i;
    int j;
    real e1_i, e1_j;
    real e0_i, e0_j;
    real denom_cross;
    real numerator;
    real denom;

    e0[0] = p.x - a.x; e0[1] = p.y - a.y; e0[2] = p.z - a.z;
    e1[0] = v_edx.x - a.x; e1[1] = v_edx.y - a.y; e1[2] = v_edx.z - a.z;
    e2[0] = v_ecx.x - a.x; e2[1] = v_ecx.y - a.y; e2[2] = v_ecx.z - a.z;

    n[0] = e2[2] * e1[1] - e2[1] * e1[2];
    n[1] = e1[2] * e2[0] - e2[2] * e1[0];
    n[2] = e2[1] * e1[0] - e1[1] * e2[0];

    dot_n_e0 = n[0] * e0[0] + n[2] * e0[2] + n[1] * e0[1];
    dot_n_e0 = dot_n_e0 * dot_n_e0;
    plane_tolerance = (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]) * 0.0001f;

    if (plane_tolerance <= dot_n_e0) {
        return 0;
    }

    if ((real)fabs((double)n[2]) < (real)fabs((double)n[1]) || (real)fabs((double)n[2]) < (real)fabs((double)n[0])) {
        if ((real)fabs((double)n[1]) < (real)fabs((double)n[0])) {
            dominant_axis = 0;
        } else {
            dominant_axis = 1;
        }
    } else {
        dominant_axis = 2;
    }

    axis_index = (0.0f < n[dominant_axis] ? 1 : 0) + dominant_axis * 2;
    i = globals().k_projection_axes[axis_index].i;
    j = globals().k_projection_axes[axis_index].j;

    e1_i = e1[i];
    e1_j = e1[j];
    e0_i = e0[i];
    e0_j = e0[j];

    denom_cross = e0_j * e1_i - e1_j * e0_i;
    if (denom_cross < 0.0f) {
        return 0;
    }

    numerator = e0_i * e2[j] - e0_j * e2[i];
    if (numerator < 0.0f) {
        return 0;
    }

    denom = e2[j] * e1_i - e1_j * e2[i];
    if (numerator + denom_cross <= denom) {
        real inv_denom = 1.0f / denom;
        out_u = numerator * inv_denom;
        out_v = inv_denom * denom_cross;
        return 1;
    }
    return 0;
}

int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius)
{
    real_vector3d w0;
    real_vector3d cross_ab;
    real s;
    real t;
    real dx, dy, dz;

    w0.i = b_start->x - a_start->x;
    w0.j = b_start->y - a_start->y;
    w0.k = b_start->z - a_start->z;

    cross_ab.i = a_direction->j * b_direction->k - b_direction->j * a_direction->k;
    cross_ab.j = a_direction->k * b_direction->i - a_direction->i * b_direction->k;
    cross_ab.k = b_direction->j * a_direction->i - a_direction->j * b_direction->i;

    if (fabs((double)(cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k)) < 0.0001) {
        real dot_dirs = a_direction->i * b_direction->i + a_direction->j * b_direction->j + a_direction->k * b_direction->k;
        real len_a2 = a_direction->i * a_direction->i + a_direction->j * a_direction->j + a_direction->k * a_direction->k;
        real len_b2 = b_direction->i * b_direction->i + b_direction->j * b_direction->j + b_direction->k * b_direction->k;

        if (len_a2 <= 0.0001f) {
            s = 0.0f;
        } else {
            real inv_a2 = 1.0f / len_a2;
            real s0 = (w0.i * a_direction->i + w0.k * a_direction->k + w0.j * a_direction->j) * inv_a2;
            real s1 = inv_a2 * dot_dirs + s0;
            real s0_clamped = s0;
            if (s0_clamped < 0.0f) { s0_clamped = 0.0f; } else if (1.0f < s0_clamped) { s0_clamped = 1.0f; }
            if (0.0f <= s1 && s1 <= 1.0f) { s = (s1 + s0_clamped) * 0.5f; }
            else if (1.0f < s1) { s = (1.0f + s0_clamped) * 0.5f; }
            else { s = (0.0f + s0_clamped) * 0.5f; }
        }

        if (len_b2 <= 0.0001f) {
            t = 0.0f;
        } else {
            real inv_b2 = 1.0f / len_b2;
            real t0 = -((w0.k * b_direction->k + w0.i * b_direction->i + w0.j * b_direction->j) * inv_b2);
            real t1 = inv_b2 * dot_dirs + t0;
            real t0_clamped = t0;
            if (t0_clamped < 0.0f) { t0_clamped = 0.0f; } else if (1.0f < t0_clamped) { t0_clamped = 1.0f; }
            if (0.0f <= t1 && t1 <= 1.0f) { t = (t1 + t0_clamped) * 0.5f; }
            else if (1.0f < t1) { t = (t0_clamped + 1.0f) * 0.5f; }
            else { t = (t0_clamped + 0.0f) * 0.5f; }
        }
    } else {
        real denom = cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k;
        real raw_s = vector3d_scalar_triple_product(w0, *b_direction, cross_ab) / denom;
        real raw_t = vector3d_scalar_triple_product(w0, *a_direction, cross_ab) / denom;
        int s_out_of_range = raw_s < 0.0f || 1.0f < raw_s;
        int t_out_of_range = raw_t < 0.0f || 1.0f < raw_t;

        if (!s_out_of_range && !t_out_of_range) {
            s = raw_s;
            t = raw_t;
        } else {
            if (s_out_of_range) {
                real_point3d point_on_a;
                real snapped_s = (0.0f <= raw_s) ? 1.0f : 0.0f;
                point_on_a.x = snapped_s * a_direction->i + a_start->x;
                point_on_a.y = snapped_s * a_direction->j + a_start->y;
                point_on_a.z = snapped_s * a_direction->k + a_start->z;
                if (ray_intersects_sphere_test(point_on_a, *b_start, *b_direction, radius)) {
                    return 1;
                }
            }
            if (t_out_of_range) {
                real_point3d point_on_b;
                real snapped_t = (0.0f <= raw_t) ? 1.0f : 0.0f;
                point_on_b.x = snapped_t * b_direction->i + b_start->x;
                point_on_b.y = snapped_t * b_direction->j + b_start->y;
                point_on_b.z = snapped_t * b_direction->k + b_start->z;
                if (ray_intersects_sphere_test(point_on_b, *a_start, *a_direction, radius)) {
                    return 1;
                }
            }
            return 0;
        }
    }

    dx = (t * b_direction->i + b_start->x) - (s * a_direction->i + a_start->x);
    dy = (t * b_direction->j + b_start->y) - (s * a_direction->j + a_start->y);
    dz = (t * b_direction->k + b_start->z) - (s * a_direction->k + a_start->z);
    if (dx * dx + dy * dy + dz * dz <= radius * radius) {
        return 1;
    }
    return 0;
}

uint8_t vector3d_projection_band_test(const real_vector3d &axis, const real_point3d &point_a, const real_point3d &point_b, real radius, real max_distance, real sin_max_angle, real cos_max_angle)
{
    real dx = point_b.x - point_a.x;
    real dy = point_b.y - point_a.y;
    real dz = point_b.z - point_a.z;
    real projection = dy * axis.j + dz * axis.k + dx * axis.i;

    if (-radius <= projection) {
        real upper = radius + max_distance;
        if (upper >= projection) {
            real lhs = radius * radius + (radius * sin_max_angle + radius * sin_max_angle + projection) * projection;
            real rhs = (dx * dx + dy * dy + dz * dz) * cos_max_angle * cos_max_angle;
            if (rhs <= lhs) {
                return 1;
            }
        }
    }
    return 0;
}

uint8_t plane3d_intersect_three(real_plane3d &p1, real_plane3d &p2, real_plane3d &p3, real_point3d &out)
{
    real det;
    real abs_det;

    det = vector3d_scalar_triple_product(p1.normal, p2.normal, p3.normal);
    abs_det = (real)fabs((double)det);

    if (abs_det < 0.0001f) {
        return 0;
    }

    {
        real inv_det = 1.0f / det;

        real cross_23_x = p2.normal.j * p3.normal.k - p3.normal.j * p2.normal.k;
        real cross_23_y = p2.normal.k * p3.normal.i - p2.normal.i * p3.normal.k;
        real cross_23_z = p3.normal.j * p2.normal.i - p2.normal.j * p3.normal.i;

        real cross_13_x = p1.normal.k * p3.normal.j - p1.normal.j * p3.normal.k;
        real cross_13_y = p1.normal.i * p3.normal.k - p1.normal.k * p3.normal.i;
        real cross_13_z = p1.normal.j * p3.normal.i - p1.normal.i * p3.normal.j;

        real cross_21_x = p2.normal.k * p1.normal.j - p1.normal.k * p2.normal.j;
        real cross_21_y = p1.normal.k * p2.normal.i - p2.normal.k * p1.normal.i;
        real cross_21_z = p2.normal.j * p1.normal.i - p1.normal.j * p2.normal.i;

        out.x = (cross_23_x * p1.d + cross_13_x * p2.d + cross_21_x * p3.d) * inv_det;
        out.y = (cross_23_y * p1.d + cross_13_y * p2.d + cross_21_y * p3.d) * inv_det;
        out.z = (cross_23_z * p1.d + cross_13_z * p2.d + cross_21_z * p3.d) * inv_det;
    }
    return 1;
}

uint8_t plane3d_intersect_pair_to_line(real_vector3d &direction_out, const real_plane3d &p2, const real_plane3d &p1, real_point3d &point_out)
{
    real len2;

    direction_out.i = p2.normal.k * p1.normal.j - p1.normal.k * p2.normal.j;
    direction_out.j = p1.normal.k * p2.normal.i - p1.normal.i * p2.normal.k;
    direction_out.k = p1.normal.i * p2.normal.j - p2.normal.i * p1.normal.j;

    len2 = direction_out.k * direction_out.k + direction_out.j * direction_out.j + direction_out.i * direction_out.i;
    if ((real)fabs((double)len2) < 0.0001f) {
        return 0;
    }

    {
        real point_x_partial = (direction_out.k * p2.normal.j - direction_out.j * p2.normal.k) * p1.d;
        real point_y_partial = (p2.normal.k * direction_out.i - direction_out.k * p2.normal.i) * p1.d;
        real point_z_partial = (direction_out.j * p2.normal.i - direction_out.i * p2.normal.j) * p1.d;
        real inv_len2 = 1.0f / len2;

        point_out.x = ((p1.normal.k * direction_out.j - direction_out.k * p1.normal.j) * p2.d + point_x_partial) * inv_len2;
        point_out.y = ((direction_out.k * p1.normal.i - p1.normal.k * direction_out.i) * p2.d + point_y_partial) * inv_len2;
        point_out.z = ((direction_out.i * p1.normal.j - direction_out.j * p1.normal.i) * p2.d + point_z_partial) * inv_len2;
    }
    return 1;
}

}  // namespace halo::math
