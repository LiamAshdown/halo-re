/**
 * @file src/math/matrix.cpp
 * Real_matrix3x3 / real_matrix4x3: build, invert, multiply (scalar, sse, 3dnow!), transform.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/math/math.hpp"

#include "tags.h"
#include "memory.h"

extern "C" {
extern double cos(double x);
extern double sin(double x);
extern uint8_t vector3d_is_unit_length(real_vector3d *v);
extern uint8_t real_approximately_equal(real a, real b);
extern const real_vector3d *global_forward3d_pointer;
extern const real_vector3d *global_left3d_pointer;
extern const real_vector3d *global_up3d_pointer;
}

namespace halo::math {

namespace {

static void cross(real_vector3d *out, real_vector3d *a, real_vector3d *b)
{
    out->i = a->j * b->k - a->k * b->j;
    out->j = a->k * b->i - a->i * b->k;
    out->k = a->i * b->j - a->j * b->i;
}

}  // namespace

void matrix4x3_inverse(real_matrix4x3 *out, const real_matrix4x3 &in)
{
    real neg_x, neg_y, neg_z;
    real inv_scale;
    real t1, t2;
    int i;
    real *zero;

    if (in.scale != 0.0f) {
        neg_x = -in.position.x;
        neg_y = -in.position.y;
        neg_z = -in.position.z;
        if (in.scale == 1.0f) {
            out->scale = 1.0f;
        } else {
            inv_scale = 1.0f / in.scale;
            out->scale = inv_scale;
            neg_x = inv_scale * neg_x;
            neg_y = inv_scale * neg_y;
            neg_z = inv_scale * neg_z;
        }

        out->forward.i = in.forward.i;
        out->left.j = in.left.j;
        out->up.k = in.up.k;

        t1 = in.forward.j;
        out->forward.j = in.left.i;
        out->left.i = t1;

        t1 = in.forward.k;
        out->forward.k = in.up.i;
        out->up.i = t1;

        t1 = in.up.j;
        t2 = in.left.k;
        out->left.k = t1;
        out->up.j = t2;

        out->position.x = neg_y * out->left.i + neg_x * out->forward.i + neg_z * out->up.i;
        out->position.y = neg_y * out->left.j + neg_x * out->forward.j + neg_z * out->up.j;
        out->position.z = t1 * neg_y + neg_z * out->up.k + neg_x * out->forward.k;
    } else {
        zero = (real *)out;
        for (i = 0xd; i != 0; i--) {
            *zero = 0.0f;
            zero++;
        }
    }
}

void matrix4x3_from_axis_angle(real_matrix4x3 &out, const real_vector3d &axis, real sin_angle, real cos_angle)
{
    real one_minus_cos;
    real ii, jj, kk;
    real ij, ik, jk;

    one_minus_cos = 1.0f - cos_angle;
    ii = axis.i * axis.i;
    jj = axis.j * axis.j;
    kk = axis.k * axis.k;
    ij = axis.j * axis.i * one_minus_cos;
    ik = axis.k * axis.i * one_minus_cos;
    jk = axis.k * axis.j * one_minus_cos;

    out.scale = 1.0f;
    out.forward.i = (1.0f - ii) * cos_angle + ii;
    out.forward.j = ij + sin_angle * axis.k;
    out.left.i = ij - sin_angle * axis.k;
    out.left.j = (1.0f - jj) * cos_angle + jj;
    out.forward.k = ik - sin_angle * axis.j;
    out.up.i = ik + sin_angle * axis.j;
    out.up.k = (1.0f - kk) * cos_angle + kk;
    out.position.z = 0.0f;
    out.position.y = 0.0f;
    out.position.x = 0.0f;
    out.left.k = jk + sin_angle * axis.i;
    out.up.j = jk - sin_angle * axis.i;
}

void matrix4x3_from_forward_up(const real_vector3d &up, const real_vector3d &forward, real_matrix4x3 &out)
{
    out.scale = 1.0f;
    out.forward = forward;
    out.left.i = up.j * forward.k - up.k * forward.j;
    out.left.j = up.k * forward.i - forward.k * up.i;
    out.left.k = up.i * forward.j - up.j * forward.i;
    out.up = up;
    out.position.x = 0.0f;
    out.position.y = 0.0f;
    out.position.z = 0.0f;
}

void matrix4x3_from_euler_angles(real_matrix4x3 &out, real yaw, real pitch, real roll)
{
    real cy, sy;
    real cp, sp;
    real cyaw, syaw;

    cy = (real)cos((double)roll);
    out.scale = 1.0f;
    out.position.x = 0.0f;
    out.position.y = 0.0f;
    out.position.z = 0.0f;
    sy = (real)sin((double)roll);
    cp = (real)cos((double)pitch);
    sp = (real)sin((double)pitch);
    cyaw = (real)cos((double)yaw);
    syaw = (real)sin((double)yaw);

    out.forward.i = cyaw * cp;
    out.forward.j = syaw * cy - sp * sy * cyaw;
    out.forward.k = syaw * sy + sp * cy * cyaw;
    out.left.i = -(syaw * cp);
    out.left.j = cyaw * cy + sp * sy * syaw;
    out.left.k = cyaw * sy - sp * cy * syaw;
    out.up.i = -sp;
    out.up.j = -(cp * sy);
    out.up.k = cp * cy;
}

void matrix4x3_from_forward_up_position(real_vector3d *up, real_vector3d *forward, const real_point3d &position, real_matrix4x3 *out)
{
    matrix4x3_from_forward_up(*up, *forward, *out);
    out->position = position;
}

void matrix4x3_extract_forward_up_position(real_vector3d &out_up, real_vector3d &out_forward, const real_matrix4x3 &m, real_point3d &out_position)
{
    out_forward = m.forward;
    out_up = m.up;
    out_position = m.position;
}

void matrix4x3_transform_point(real_point3d &out, const real_point3d &point, const real_matrix4x3 &m)
{
    real x, y, z;

    x = point.x;
    y = point.y;
    z = point.z;
    if (m.scale != 1.0f) {
        x = x * m.scale;
        y = y * m.scale;
        z = z * m.scale;
    }
    out.x = x * m.forward.i + y * m.left.i + z * m.up.i + m.position.x;
    out.y = x * m.forward.j + y * m.left.j + z * m.up.j + m.position.y;
    out.z = x * m.forward.k + y * m.left.k + z * m.up.k + m.position.z;
}

void matrix4x3_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m)
{
    real i, j, k;

    i = v.i;
    j = v.j;
    k = v.k;
    if (m.scale != 1.0f) {
        i = i * m.scale;
        j = j * m.scale;
        k = k * m.scale;
    }
    out.i = i * m.forward.i + j * m.left.i + k * m.up.i;
    out.j = i * m.forward.j + j * m.left.j + k * m.up.j;
    out.k = i * m.forward.k + j * m.left.k + k * m.up.k;
}

void matrix4x3_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m)
{
    real i, j, k;

    i = normal.i;
    j = normal.j;
    k = normal.k;
    out.i = i * m.forward.i + j * m.left.i + k * m.up.i;
    out.j = i * m.forward.j + j * m.left.j + k * m.up.j;
    out.k = i * m.forward.k + j * m.left.k + k * m.up.k;
}

void matrix4x3_transform_plane(real_plane3d &out, const real_matrix4x3 &m, const real_plane3d &plane)
{
    real i, j, k;

    i = plane.normal.i;
    j = plane.normal.j;
    k = plane.normal.k;
    out.normal.i = i * m.forward.i + j * m.left.i + k * m.up.i;
    out.normal.j = i * m.forward.j + j * m.left.j + k * m.up.j;
    out.normal.k = i * m.forward.k + j * m.left.k + k * m.up.k;
    out.d = m.position.x * out.normal.i + out.normal.k * m.position.z +
             m.position.y * out.normal.j + plane.d * m.scale;
}

void matrix4x3_inverse_transform_point(const real_matrix4x3 &m, real_point3d &out, const real_point3d &point)
{
    real dx, dy, dz;
    real inv_scale;

    if (m.scale != 0.0f) {
        dx = point.x - m.position.x;
        dy = point.y - m.position.y;
        dz = point.z - m.position.z;
        if (m.scale != 1.0f) {
            inv_scale = 1.0f / m.scale;
            dx = inv_scale * dx;
            dy = inv_scale * dy;
            dz = inv_scale * dz;
        }
        out.x = dx * m.forward.i + dy * m.forward.j + dz * m.forward.k;
        out.y = dx * m.left.i + dy * m.left.j + dz * m.left.k;
        out.z = dx * m.up.i + dy * m.up.j + dz * m.up.k;
        return;
    }
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;
}

void matrix4x3_inverse_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m)
{
    real i, j, k;
    real inv_scale;

    i = v.i;
    j = v.j;
    k = v.k;
    if (m.scale != 1.0f) {
        inv_scale = 1.0f / m.scale;
        i = inv_scale * i;
        j = inv_scale * j;
        k = inv_scale * k;
    }
    out.i = i * m.forward.i + j * m.forward.j + k * m.forward.k;
    out.j = i * m.left.i + j * m.left.j + k * m.left.k;
    out.k = i * m.up.i + j * m.up.j + k * m.up.k;
}

void matrix4x3_inverse_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m)
{
    real i, j, k;

    i = normal.i;
    j = normal.j;
    k = normal.k;
    out.i = i * m.forward.i + j * m.forward.j + k * m.forward.k;
    out.j = i * m.left.i + j * m.left.j + k * m.left.k;
    out.k = i * m.up.i + j * m.up.j + k * m.up.k;
}

void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    real_matrix4x3 scratch;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }

    out->forward.i = b->forward.k * a->up.i + a->forward.i * b->forward.i + a->left.i * b->forward.j;
    out->forward.j = a->left.j * b->forward.j + a->forward.j * b->forward.i + a->up.j * b->forward.k;
    out->forward.k = a->left.k * b->forward.j + a->forward.k * b->forward.i + a->up.k * b->forward.k;
    out->left.i = a->left.i * b->left.j + b->left.k * a->up.i + a->forward.i * b->left.i;
    out->left.j = a->forward.j * b->left.i + a->left.j * b->left.j + a->up.j * b->left.k;
    out->left.k = a->forward.k * b->left.i + a->left.k * b->left.j + a->up.k * b->left.k;
    out->up.i = a->left.i * b->up.j + a->forward.i * b->up.i + b->up.k * a->up.i;
    out->up.j = b->up.j * a->left.j + b->up.k * a->up.j + a->forward.j * b->up.i;
    out->up.k = b->up.j * a->left.k + b->up.k * a->up.k + a->forward.k * b->up.i;
    out->position.x = (b->position.z * a->up.i + a->left.i * b->position.y + a->forward.i * b->position.x) * a->scale + a->position.x;
    out->position.y = (b->position.x * a->forward.j + b->position.y * a->left.j + a->up.j * b->position.z) * a->scale + a->position.y;
    out->position.z = (b->position.x * a->forward.k + b->position.y * a->left.k + a->up.k * b->position.z) * a->scale + a->position.z;
    out->scale = a->scale * b->scale;
}

void matrix4x3_multiply_sse(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    matrix4x3_multiply(a, b, out);
}

void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    real_matrix4x3 scratch;
    const float *A;
    const float *B;
    float *O = (float *)out;
    int32_t column;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }
    A = (const float *)a;
    B = (const float *)b;

    for (column = 0; column < 3; column++) {
        const float bx = B[1 + column * 3];
        const float by = B[2 + column * 3];
        const float bz = B[3 + column * 3];

        O[1 + column * 3] = (bx * A[1] + by * A[4]) + bz * A[7];
        O[2 + column * 3] = (bx * A[2] + by * A[5]) + bz * A[8];
        O[3 + column * 3] = (bx * A[3] + by * A[6]) + bz * A[9];
    }

    O[10] = ((B[10] * A[1] + B[11] * A[4]) + B[12] * A[7]) * A[0] + A[10];
    O[11] = ((B[10] * A[2] + B[11] * A[5]) + B[12] * A[8]) * A[0] + A[11];
    O[12] = ((B[10] * A[3] + B[11] * A[6]) + B[12] * A[9]) * A[0] + A[12];
    O[0] = B[0] * A[0];
}

void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in)
{
    real t;

    if (in == out) {
        t = in->forward.j;
        out->forward.j = in->left.i;
        out->left.i = t;
        t = in->forward.k;
        out->forward.k = in->up.i;
        out->up.i = t;
        t = in->left.k;
        out->left.k = in->up.j;
        out->up.j = t;
        return;
    }
    out->forward.i = in->forward.i;
    out->forward.j = in->left.i;
    out->forward.k = in->up.i;
    out->left.i = in->forward.j;
    out->left.j = in->left.j;
    out->left.k = in->up.j;
    out->up.i = in->forward.k;
    out->up.j = in->left.k;
    out->up.k = in->up.k;
}

void matrix3x3_from_forward_up(const real_vector3d &up, const real_vector3d &forward, real_matrix3x3 &out)
{
    out.forward = forward;
    out.left.i = up.j * forward.k - up.k * forward.j;
    out.left.j = up.k * forward.i - forward.k * up.i;
    out.left.k = up.i * forward.j - up.j * forward.i;
    out.up = up;
}

void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b)
{
    real_matrix3x3 scratch;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }

    out->forward.i = b->forward.k * a->up.i + a->left.i * b->forward.j + a->forward.i * b->forward.i;
    out->forward.j = a->up.j * b->forward.k + a->forward.j * b->forward.i + a->left.j * b->forward.j;
    out->forward.k = a->up.k * b->forward.k + a->forward.k * b->forward.i + a->left.k * b->forward.j;
    out->left.i = b->left.k * a->up.i + b->left.i * a->forward.i + a->left.i * b->left.j;
    out->left.j = a->forward.j * b->left.i + a->left.j * b->left.j + a->up.j * b->left.k;
    out->left.k = a->forward.k * b->left.i + a->left.k * b->left.j + a->up.k * b->left.k;
    out->up.i = a->up.i * b->up.k + a->left.i * b->up.j + a->forward.i * b->up.i;
    out->up.j = a->forward.j * b->up.i + a->left.j * b->up.j + a->up.j * b->up.k;
    out->up.k = a->forward.k * b->up.i + a->left.k * b->up.j + a->up.k * b->up.k;
}

void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, const real_matrix3x3 &m)
{
    real_vector3d snapshot;

    if (v == out) {
        snapshot = *v;
        v = &snapshot;
    }
    out->i = m.forward.i * v->i + m.left.i * v->j + m.up.i * v->k;
    out->j = m.left.j * v->j + m.forward.j * v->i + m.up.j * v->k;
    out->k = m.left.k * v->j + m.forward.k * v->i + m.up.k * v->k;
}

void euler_angles_to_basis_vectors(const real_euler_angles3d &angles, real_vector3d &up_out, real_vector3d &forward_out)
{
    real_matrix4x3 matrix;

    matrix4x3_from_euler_angles(matrix, angles.yaw, angles.pitch, angles.roll);

    forward_out = matrix.forward;
    up_out = matrix.up;
}

uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    if (!vector3d_is_unit_length(forward)) return 0;
    if (!vector3d_is_unit_length(left)) return 0;
    if (!vector3d_is_unit_length(up)) return 0;

    if (!real_approximately_equal(forward->i * left->i + forward->j * left->j + forward->k * left->k, 0.0f))
        return 0;
    if (!real_approximately_equal(forward->i * up->i + forward->j * up->j + forward->k * up->k, 0.0f))
        return 0;
    if (!real_approximately_equal(left->i * up->i + left->j * up->j + left->k * up->k, 0.0f))
        return 0;

    return 1;
}

void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    if (vector3d_normalize_with_length(*forward) == 0.0f) {
        *forward = *global_forward3d_pointer;
    }
    if (vector3d_normalize_with_length(*up) == 0.0f) {
        *up = *global_up3d_pointer;
    }

    cross(left, up, forward);
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *left = *global_left3d_pointer;
    }

    cross(up, forward, left);
    if (vector3d_normalize_with_length(*up) == 0.0f) {
        *up = *global_up3d_pointer;
    }

    cross(left, up, forward);
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *left = *global_forward3d_pointer;
    }
}

void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    *up = *global_up3d_pointer;

    cross(left, up, forward);
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *up = *global_forward3d_pointer;
        vector3d_cross_product(*left, *forward, *up);
        vector3d_normalize_with_length(*left);
    }

    cross(up, forward, left);
    vector3d_normalize_with_length(*up);
}

}  // namespace halo::math
