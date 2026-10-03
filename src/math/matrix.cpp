/**
 * @file src/math/matrix.cpp
 * Real_matrix3x3 / real_matrix4x3: build, invert, multiply (scalar, sse, 3dnow!), transform.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/glm_interop.hpp"
#include "halo/math/globals.hpp"

#include "tags.h"
#include "memory.h"
#include "halo/camera/api.hpp"

namespace halo::math {

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
    out = matrix4x3_from_glm(glm::mat3(to_glm(forward), glm::cross(to_glm(up), to_glm(forward)), to_glm(up)), 1.0f,
                             glm::vec3(0.0f));
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

void matrix4x3_from_forward_up_position(const real_vector3d *up, const real_vector3d *forward, const real_point3d &position, real_matrix4x3 *out)
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
    glm::vec3 p = to_glm(point);

    if (m.scale != 1.0f) {
        p = p * m.scale;
    }
    out = point_from_glm(rotation_to_glm(m) * p + to_glm(m.position));
}

void matrix4x3_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m)
{
    glm::vec3 v3 = to_glm(v);

    if (m.scale != 1.0f) {
        v3 = v3 * m.scale;
    }
    out = vector_from_glm(rotation_to_glm(m) * v3);
}

void matrix4x3_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m)
{
    out = vector_from_glm(rotation_to_glm(m) * to_glm(normal));
}

void matrix4x3_transform_plane(real_plane3d &out, const real_matrix4x3 &m, const real_plane3d &plane)
{
    out.normal = vector_from_glm(rotation_to_glm(m) * to_glm(plane.normal));
    out.d = m.position.x * out.normal.i + out.normal.k * m.position.z +
             m.position.y * out.normal.j + plane.d * m.scale;
}

void matrix4x3_inverse_transform_point(const real_matrix4x3 &m, real_point3d &out, const real_point3d &point)
{
    if (m.scale != 0.0f) {
        glm::vec3 d = to_glm(point) - to_glm(m.position);
        if (m.scale != 1.0f) {
            d = (1.0f / m.scale) * d;
        }
        out = point_from_glm(d * rotation_to_glm(m));
        return;
    }
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;
}

void matrix4x3_inverse_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m)
{
    glm::vec3 v3 = to_glm(v);

    if (m.scale != 1.0f) {
        v3 = (1.0f / m.scale) * v3;
    }
    out = vector_from_glm(v3 * rotation_to_glm(m));
}

void matrix4x3_inverse_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m)
{
    out = vector_from_glm(to_glm(normal) * rotation_to_glm(m));
}

void matrix4x3_multiply(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out)
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

void matrix4x3_multiply_sse(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out)
{
    matrix4x3_multiply(a, b, out);
}

void matrix4x3_multiply_3dnow(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out)
{
    const glm::mat3 a_rotation = rotation_to_glm(*a);
    const glm::mat3 rotation = a_rotation * rotation_to_glm(*b);
    const glm::vec3 position = a_rotation * to_glm(b->position) * a->scale + to_glm(a->position);

    *out = matrix4x3_from_glm(rotation, b->scale * a->scale, position);
}

void matrix3x3_transpose(real_matrix3x3 *out, const real_matrix3x3 *in)
{
    *out = matrix3x3_from_glm(glm::transpose(to_glm(*in)));
}

void matrix3x3_from_forward_up(const real_vector3d &up, const real_vector3d &forward, real_matrix3x3 &out)
{
    out.forward = forward;
    out.left = vector_from_glm(glm::cross(to_glm(up), to_glm(forward)));
    out.up = up;
}

void matrix3x3_multiply(real_matrix3x3 *out, const real_matrix3x3 *a, const real_matrix3x3 *b)
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
    *out = vector_from_glm(to_glm(m) * to_glm(*v));
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
    if (!halo::camera::vector3d_is_unit_length((Vector3D *)forward)) return 0;
    if (!halo::camera::vector3d_is_unit_length((Vector3D *)left)) return 0;
    if (!halo::camera::vector3d_is_unit_length((Vector3D *)up)) return 0;

    if (!halo::camera::real_approximately_equal(forward->i * left->i + forward->j * left->j + forward->k * left->k, 0.0f))
        return 0;
    if (!halo::camera::real_approximately_equal(forward->i * up->i + forward->j * up->j + forward->k * up->k, 0.0f))
        return 0;
    if (!halo::camera::real_approximately_equal(left->i * up->i + left->j * up->j + left->k * up->k, 0.0f))
        return 0;

    return 1;
}

void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    if (vector3d_normalize_with_length(*forward) == 0.0f) {
        *forward = *globals().global_forward3d_pointer;
    }
    if (vector3d_normalize_with_length(*up) == 0.0f) {
        *up = *globals().global_up3d_pointer;
    }

    *left = vector_from_glm(glm::cross(to_glm(*up), to_glm(*forward)));
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *left = *globals().global_left3d_pointer;
    }

    *up = vector_from_glm(glm::cross(to_glm(*forward), to_glm(*left)));
    if (vector3d_normalize_with_length(*up) == 0.0f) {
        *up = *globals().global_up3d_pointer;
    }

    *left = vector_from_glm(glm::cross(to_glm(*up), to_glm(*forward)));
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *left = *globals().global_forward3d_pointer;
    }
}

void real_matrix4x3_rotation_from_forward(const real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    *up = *globals().global_up3d_pointer;

    *left = vector_from_glm(glm::cross(to_glm(*up), to_glm(*forward)));
    if (vector3d_normalize_with_length(*left) == 0.0f) {
        *up = *globals().global_forward3d_pointer;
        vector3d_cross_product(*left, *forward, *up);
        vector3d_normalize_with_length(*left);
    }

    *up = vector_from_glm(glm::cross(to_glm(*forward), to_glm(*left)));
    vector3d_normalize_with_length(*up);
}

}  // namespace halo::math
