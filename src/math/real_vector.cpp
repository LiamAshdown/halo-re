/**
 * @file src/math/real_vector.cpp
 * Real vectors and points: length, normalize, cross/dot products, projections, angles.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/glm_interop.hpp"

#include "tags.h"
#include "halo/physics/api.hpp"
#include "halo/game/api.hpp"


namespace halo::math {

real vector3d_magnitude_squared(const real_vector3d &v)
{
    const glm::vec3 v3 = to_glm(v);
    return glm::dot(v3, v3);
}

real vector3d_distance_squared(const real_point3d &a, const real_point3d &b)
{
    return (a.z - b.z) * (a.z - b.z) +
           (a.x - b.x) * (a.x - b.x) +
           (a.y - b.y) * (a.y - b.y);
}

real vector2d_normalize_with_length(real_vector2d &v)
{
    real length;
    real inv_length;

    length = (real)sqrt((double)(v.j * v.j + v.i * v.i));
    if (0.0001f <= (real)fabs((double)length)) {
        inv_length = 1.0f / length;
        v.i = inv_length * v.i;
        v.j = inv_length * v.j;
        return length;
    }
    return 0.0f;
}

void point3d_add_scaled(real_point3d &out, const real_vector3d &direction, const real_point3d &base, real scale)
{
    out = point_from_glm(scale * to_glm(direction) + to_glm(base));
}

real vector3d_length(const real_vector3d &v)
{
    return (real)sqrt((double)(v.k * v.k + v.j * v.j + v.i * v.i));
}

real vector3d_normalize_with_length(real_vector3d &v)
{
    real length;
    real inv_length;

    length = (real)sqrt((double)(v.k * v.k + v.j * v.j + v.i * v.i));
    if (0.0001f <= (real)fabs((double)length)) {
        inv_length = 1.0f / length;
        v.i = inv_length * v.i;
        v.j = inv_length * v.j;
        v.k = inv_length * v.k;
        return length;
    }
    return 0.0f;
}

void vector3d_cross_product(real_vector3d &out, const real_vector3d &a, const real_vector3d &b)
{
    out.i = a.k * b.j - b.k * a.j;
    out.j = b.k * a.i - b.i * a.k;
    out.k = b.i * a.j - a.i * b.j;
}

real vector3d_distance(const real_point3d &a, const real_point3d &b)
{
    return (real)sqrt((double)((a.z - b.z) * (a.z - b.z) +
                                (a.y - b.y) * (a.y - b.y) +
                                (a.x - b.x) * (a.x - b.x)));
}

int16_t vector3d_major_axis_index(const real_vector3d &v)
{
    real x = (real)fabs((double)v.i);
    real y = (real)fabs((double)v.j);
    real z = (real)fabs((double)v.k);

    if (!(z >= y) || !(z >= x)) {
        return !(y >= x) ? 0 : 1;
    }
    return 2;
}

float vector3d_scalar_triple_product(const real_vector3d &a, const real_vector3d &b, const real_vector3d &c)
{
    return glm::dot(glm::cross(to_glm(a), to_glm(b)), to_glm(c));
}

void vector3d_positive_modulo(const real_vector3d &v, real_vector3d &out, float period)
{
    out.i = (float)fmod((double)v.i, (double)period) + (v.i < 0.0f ? period : 0.0f);
    out.j = (float)fmod((double)v.j, (double)period) + (v.j < 0.0f ? period : 0.0f);
    out.k = (float)fmod((double)v.k, (double)period) + (v.k < 0.0f ? period : 0.0f);
}

void vector2d_normalize(real_vector2d &v)
{
    real length_squared;
    real inv_length;

    length_squared = v.j * v.j + v.i * v.i;
    if (length_squared != 0.0f) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        v.i = inv_length * v.i;
        v.j = inv_length * v.j;
    }
}

void vector3d_normalize(real_vector3d &v)
{
    real length_squared;
    real inv_length;

    length_squared = v.k * v.k + v.j * v.j + v.i * v.i;
    if (length_squared != 0.0f) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        v.i = inv_length * v.i;
        v.j = inv_length * v.j;
        v.k = inv_length * v.k;
    }
}

real vector3d_cross_product_length(const real_vector3d &a, const real_vector3d &b)
{
    real cross_i;
    real cross_j;
    real cross_k;

    cross_i = a.k * b.j - b.k * a.j;
    cross_j = b.k * a.i - a.k * b.i;
    cross_k = a.i * b.j - b.i * a.j;

    return (real)sqrt((double)(cross_i * cross_i + cross_k * cross_k + cross_j * cross_j));
}

real vector2d_angle_between(const real_vector2d &a, const real_vector2d &b)
{
    real dot;
    real angle;

    dot = a.j * b.j + b.i * a.i;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }

    angle = (real)acos((double)dot);
    if (a.j * b.i - a.i * b.j < 0.0f) {
        angle = -angle;
    }
    return angle;
}

real vector3d_angle_between_4cd4f0(const real_vector3d &a, const real_vector3d &b)
{
    real length_products;
    real dot;
    real cos_double_angle;
    real angle;

    angle = 0.0f;
    length_products = (a.k * a.k + a.j * a.j + a.i * a.i) *
                       (b.k * b.k + b.j * b.j + b.i * b.i);
    if (length_products != 0.0f) {
        dot = b.i * a.i + b.j * a.j + b.k * a.k;
        cos_double_angle = (dot / length_products) * dot;
        cos_double_angle = (cos_double_angle + cos_double_angle) - 1.0f;
        if (cos_double_angle < -1.0f) {
            cos_double_angle = -1.0f;
        } else if (1.0f < cos_double_angle) {
            cos_double_angle = 1.0f;
        }

        angle = (real)acos((double)cos_double_angle);
        angle = angle * 0.5f;
        if (dot < 0.0f) {
            angle = 3.1415927f - angle;
        }
    }
    return angle;
}

real vector3d_angle_between_4cd5e0(const real_vector3d &a, const real_vector3d &b)
{
    real dot;

    if (b.i == a.i && b.j == a.j && b.k == a.k) {
        return 0.0f;
    }

    dot = b.i * a.i + b.j * a.j + b.k * a.k;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }
    return (real)acos((double)dot);
}

void vector3d_build_perpendicular(real_vector3d &out, const real_vector3d &dir)
{
    real abs_i;
    real abs_j;
    real abs_k;

    abs_i = (real)fabs((double)dir.i);
    abs_j = (real)fabs((double)dir.j);
    abs_k = (real)fabs((double)dir.k);

    if ((abs_i < abs_j) != (abs_i == abs_j) && (abs_i < abs_k) != (abs_i == abs_k)) {
        out.i = 0.0f;
        out.j = dir.k;
        out.k = -dir.j;
        return;
    }
    if ((abs_j < abs_k) == (abs_j == abs_k)) {
        out.i = dir.j;
        out.j = -dir.i;
        out.k = 0.0f;
        return;
    }
    out.j = 0.0f;
    out.i = -dir.k;
    out.k = dir.i;
}

void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, const real_vector3d &axis, const real_vector3d &v, real_vector3d *perp_out)
{
    real dot;
    real_vector3d local_parallel;
    real_vector3d *parallel;

    dot = v.i * axis.i + axis.k * v.k + v.j * axis.j;

    parallel = parallel_out;
    if (parallel == 0) {
        parallel = &local_parallel;
    }
    parallel->i = dot * axis.i;
    parallel->j = dot * axis.j;
    parallel->k = dot * axis.k;

    if (perp_out != 0) {
        perp_out->i = v.i - parallel->i;
        perp_out->j = v.j - parallel->j;
        perp_out->k = v.k - parallel->k;
    }
}

void vector3d_project_onto_axis(real_vector3d &parallel_out, const real_vector3d &axis, const real_vector3d &v, real_vector3d &perp_out)
{
    real axis_length_squared;
    real t;

    axis_length_squared = axis.k * axis.k + axis.j * axis.j + axis.i * axis.i;
    if (axis_length_squared != 0.0f) {
        t = (v.j * axis.j + v.i * axis.i + v.k * axis.k) / axis_length_squared;
        parallel_out.i = t * axis.i;
        parallel_out.j = t * axis.j;
        parallel_out.k = t * axis.k;
        perp_out.i = v.i - parallel_out.i;
        perp_out.j = v.j - parallel_out.j;
        perp_out.k = v.k - parallel_out.k;
        return;
    }
    parallel_out.i = 0.0f;
    parallel_out.j = 0.0f;
    parallel_out.k = 0.0f;
    perp_out = v;
}

void point3d_project_onto_line(const real_point3d &point, const real_vector3d &direction, const real_point3d &line_origin, real_point3d &out_result)
{
    real_vector3d delta;
    real t;

    delta.i = point.x - line_origin.x;
    delta.j = point.y - line_origin.y;
    delta.k = point.z - line_origin.z;

    t = (delta.i * direction.i + delta.j * direction.j + delta.k * direction.k) /
        (direction.i * direction.i + direction.j * direction.j + direction.k * direction.k);

    out_result.x = t * direction.i + line_origin.x;
    out_result.y = t * direction.j + line_origin.y;
    out_result.z = t * direction.k + line_origin.z;
}

void vector3d_delta_toward_gravity_biased_clamp_length(const real_point3d &origin, const real_point3d &target, real_vector3d *out_delta, real max_length_aligned, real max_length_default)
{
    real dot_delta_target;
    real delta_length_squared;
    real target_length_squared;

    out_delta->i = target.x - origin.x;
    out_delta->j = target.y - origin.y;
    out_delta->k = (target.z - origin.z) + halo::physics::globals().gravity;

    dot_delta_target = target.y * out_delta->j + out_delta->k * target.z + target.x * out_delta->i;

    if (0.0001f < dot_delta_target) {
        delta_length_squared = out_delta->j * out_delta->j + out_delta->i * out_delta->i + out_delta->k * out_delta->k;
        target_length_squared = target.x * target.x + target.z * target.z + target.y * target.y;
        halo::game::vector3d_clamp_length(out_delta,
            (max_length_aligned - max_length_default) *
                ((dot_delta_target * dot_delta_target) / delta_length_squared / target_length_squared) +
            max_length_default);
    } else {
        halo::game::vector3d_clamp_length(out_delta, max_length_default);
    }
}

}  // namespace halo::math
