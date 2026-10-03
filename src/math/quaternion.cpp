/**
 * @file src/math/quaternion.cpp
 * Real_quaternion: normalize, multiply, lerp, rotate, matrix conversions.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "tags.h"

namespace halo::math {

namespace {

real m4x3_elem(const real_matrix4x3 *m, int row, int col)
{
    const real_vector3d *r;
    r = (row == 0) ? &m->forward : (row == 1) ? &m->left : &m->up;
    return (col == 0) ? r->i : (col == 1) ? r->j : r->k;
}

real m3x3_elem(const real_matrix3x3 *m, int row, int col)
{
    const real_vector3d *r;
    r = (row == 0) ? &m->forward : (row == 1) ? &m->left : &m->up;
    return (col == 0) ? r->i : (col == 1) ? r->j : r->k;
}

}  // namespace

void matrix4x3_from_quaternion(const real_quaternion &q, real_matrix4x3 &out)
{
    real len_sq;
    real s;
    real si, sj, sk;
    real sii, sjj, skk;
    real sij, sik, sjk;
    real siw, sjw, skw;

    len_sq = q.w * q.w + q.k * q.k + q.j * q.j + q.i * q.i;
    s = (len_sq == 0.0f) ? 0.0f : 2.0f / len_sq;

    si = s * q.i;
    sj = s * q.j;
    sk = s * q.k;
    sii = si * q.i;
    sjj = sj * q.j;
    skk = sk * q.k;
    sij = si * q.j;
    sik = si * q.k;
    sjk = sj * q.k;
    siw = si * q.w;
    sjw = sj * q.w;
    skw = sk * q.w;

    out.scale = 1.0f;
    out.position.x = 0.0f;
    out.position.y = 0.0f;
    out.position.z = 0.0f;

    out.forward.i = 1.0f - (sjj + skk);
    out.forward.j = sij - skw;
    out.forward.k = sik + sjw;
    out.left.i = sij + skw;
    out.left.j = 1.0f - (skk + sii);
    out.left.k = sjk - siw;
    out.up.i = sik - sjw;
    out.up.j = sjk + siw;
    out.up.k = 1.0f - (sjj + sii);
}

void quaternion_from_matrix4x3(const real_matrix4x3 *m, real_quaternion &out)
{
    real trace;
    real s;
    int i, j, k;
    real vec[3];

    trace = m4x3_elem(m, 0, 0) + m4x3_elem(m, 1, 1) + m4x3_elem(m, 2, 2);
    if (0.0f < trace) {
        s = (real)sqrt((double)(trace + 1.0f));
        out.w = s * 0.5f;
        s = 0.5f / s;
        out.i = (m4x3_elem(m, 2, 1) - m4x3_elem(m, 1, 2)) * s;
        out.j = (m4x3_elem(m, 0, 2) - m4x3_elem(m, 2, 0)) * s;
        out.k = (m4x3_elem(m, 1, 0) - m4x3_elem(m, 0, 1)) * s;
        return;
    }

    i = (m4x3_elem(m, 0, 0) < m4x3_elem(m, 1, 1)) ? 1 : 0;
    if (m4x3_elem(m, i, i) < m4x3_elem(m, 2, 2)) {
        i = 2;
    }
    j = globals().k_quaternion_next_index_matrix4x3[i];
    k = globals().k_quaternion_next_index_matrix4x3[j];

    s = (real)sqrt((double)((m4x3_elem(m, i, i) - (m4x3_elem(m, j, j) + m4x3_elem(m, k, k))) + 1.0f));
    vec[i] = s * 0.5f;
    if (s != 0.0f) {
        s = 0.5f / s;
    }
    vec[j] = (m4x3_elem(m, i, j) + m4x3_elem(m, j, i)) * s;
    vec[k] = (m4x3_elem(m, i, k) + m4x3_elem(m, k, i)) * s;

    out.i = vec[0];
    out.j = vec[1];
    out.w = (m4x3_elem(m, k, j) - m4x3_elem(m, j, k)) * s;
    out.k = vec[2];
}

real_quaternion * quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out)
{
    real trace;
    real s;
    int i, j, k;
    real vec[3];

    trace = m3x3_elem(m, 1, 1) + m3x3_elem(m, 0, 0) + m3x3_elem(m, 2, 2);
    if (0.0f < trace) {
        s = (real)sqrt((double)(trace + 1.0f));
        out->w = s * 0.5f;
        s = 0.5f / s;
        out->i = (m3x3_elem(m, 2, 1) - m3x3_elem(m, 1, 2)) * s;
        out->j = (m3x3_elem(m, 0, 2) - m3x3_elem(m, 2, 0)) * s;
        out->k = (m3x3_elem(m, 1, 0) - m3x3_elem(m, 0, 1)) * s;
        return out;
    }

    i = (m3x3_elem(m, 0, 0) < m3x3_elem(m, 1, 1)) ? 1 : 0;
    if (m3x3_elem(m, i, i) < m3x3_elem(m, 2, 2)) {
        i = 2;
    }
    j = globals().k_quaternion_next_index_matrix3x3[i];
    k = globals().k_quaternion_next_index_matrix3x3[j];

    s = (real)sqrt((double)((m3x3_elem(m, i, i) - (m3x3_elem(m, j, j) + m3x3_elem(m, k, k))) + 1.0f));
    vec[i] = s * 0.5f;
    if (s != 0.0f) {
        s = 0.5f / s;
    }
    vec[j] = (m3x3_elem(m, i, j) + m3x3_elem(m, j, i)) * s;
    vec[k] = (m3x3_elem(m, i, k) + m3x3_elem(m, k, i)) * s;

    out->i = vec[0];
    out->j = vec[1];
    out->w = (m3x3_elem(m, k, j) - m3x3_elem(m, j, k)) * s;
    out->k = vec[2];
    return out;
}

void quaternion_normalize(real_quaternion &q)
{
    real length_squared;
    real inv_length;

    length_squared = q.w * q.w + q.k * q.k + q.j * q.j + q.i * q.i;
    if (0.0f < length_squared) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        q.i = inv_length * q.i;
        q.j = inv_length * q.j;
        q.k = inv_length * q.k;
        q.w = inv_length * q.w;
        return;
    }
    q.i = 0.0f;
    q.j = 0.0f;
    q.k = 0.0f;
    q.w = 1.0f;
}

void quaternion_to_axis_angle(const real_quaternion &quat, real_vector3d *axis_out, real &angle_out)
{
    real w;
    real length;
    real half_angle;

    axis_out->i = quat.i;
    axis_out->j = quat.j;
    axis_out->k = quat.k;
    w = quat.w;

    length = vector3d_normalize_with_length(*axis_out);
    half_angle = (real)atan2((double)length, (double)w);
    angle_out = half_angle + half_angle;

    if (3.1415927f < half_angle + half_angle) {
        axis_out->i = -axis_out->i;
        axis_out->j = -axis_out->j;
        axis_out->k = -axis_out->k;
        angle_out = 6.2831855f - angle_out;
    }
}

void quaternion_multiply(const real_quaternion *a, const real_quaternion *b, real_quaternion *out)
{
    real_quaternion local;
    const real_quaternion *pa;
    const real_quaternion *pb;

    pa = a;
    pb = b;
    if (b == out) {
        local = *b;
        pb = &local;
    }
    if (a == out) {
        local = *a;
        pa = &local;
    }

    out->i = (pb->w * pa->i + pa->k * pb->j + pa->w * pb->i) - pa->j * pb->k;
    out->j = (pb->w * pa->j + pa->w * pb->j + pb->k * pa->i) - pb->i * pa->k;
    out->k = (pb->w * pa->k + pa->j * pb->i + pa->w * pb->k) - pa->i * pb->j;
    out->w = ((pa->w * pb->w - pb->i * pa->i) - pa->j * pb->j) - pb->k * pa->k;
}

void quaternion_lerp(const real_quaternion &a, const real_quaternion &b, real_quaternion &out, real t)
{
    real one_minus_t;
    real signed_t;

    one_minus_t = 1.0f - t;
    signed_t = t;
    if (a.w * b.w + b.i * a.i + a.j * b.j + a.k * b.k < 0.0f) {
        signed_t = -t;
    }

    out.i = signed_t * a.i + one_minus_t * b.i;
    out.j = one_minus_t * b.j + signed_t * a.j;
    out.k = one_minus_t * b.k + signed_t * a.k;
    out.w = one_minus_t * b.w + signed_t * a.w;
}

void quaternion_rotate_vector(const real_quaternion &q, const real_vector3d &v, real_vector3d &out)
{
    real a;
    real b;
    real c;

    a = (q.w * q.w) * 2.0f - 1.0f;
    b = (q.j * v.j + q.k * v.k + q.i * v.i) * 2.0f;
    c = q.w + q.w;

    out.i = a * v.i + b * q.i + (q.j * v.k - q.k * v.j) * c;
    out.j = (q.k * v.i - v.k * q.i) * c + a * v.j + b * q.j;
    out.k = (q.i * v.j - q.j * v.i) * c + a * v.k + b * q.k;
}

}  // namespace halo::math
