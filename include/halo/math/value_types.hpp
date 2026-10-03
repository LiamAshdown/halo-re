/**
 * @file include/halo/math/value_types.hpp
 * Definitions of the member functions declared on the math value types in types/math.h. Each one forwards to the
 * engine function named in its declaration, so a member call computes exactly the bits the engine computes.
 * Included by halo/math/math.hpp after the function declarations.
 */
#pragma once

#include "halo/math/geometry.hpp"
#include "halo/math/matrix.hpp"
#include "halo/math/quaternion.hpp"
#include "halo/math/real_vector.hpp"

inline real real_vector2d::normalize() { return halo::math::vector2d_normalize_with_length(*this); }

inline real real_vector3d::magnitude() const { return halo::math::vector3d_length(*this); }

inline real real_vector3d::normalize() { return halo::math::vector3d_normalize_with_length(*this); }

inline real real_point3d::distance_to(const real_point3d &o) const { return halo::math::vector3d_distance(*this, o); }

inline real real_point3d::distance_squared_to(const real_point3d &o) const { return halo::math::vector3d_distance_squared(*this, o); }

inline real_plane3d real_plane3d::negated() const
{
    real_plane3d out;
    halo::math::plane3d_negate(out, *this);
    return out;
}

inline void real_quaternion::normalize() { halo::math::quaternion_normalize(*this); }

inline real_vector3d real_quaternion::rotate(const real_vector3d &v) const
{
    real_vector3d out;
    halo::math::quaternion_rotate_vector(*this, v, out);
    return out;
}

inline real_matrix4x3 real_quaternion::to_matrix() const
{
    real_matrix4x3 out;
    halo::math::matrix4x3_from_quaternion(*this, out);
    return out;
}

inline real_quaternion operator*(const real_quaternion &a, const real_quaternion &b)
{
    real_quaternion out;
    halo::math::quaternion_multiply(&b, &a, &out);
    return out;
}

inline real_matrix3x3 real_matrix3x3::transposed() const
{
    real_matrix3x3 out;
    halo::math::matrix3x3_transpose(&out, this);
    return out;
}

inline real_matrix3x3 operator*(const real_matrix3x3 &a, const real_matrix3x3 &b)
{
    real_matrix3x3 out;
    halo::math::matrix3x3_multiply(&out, &a, &b);
    return out;
}

inline real_point3d real_matrix4x3::transform_point(const real_point3d &p) const
{
    real_point3d out;
    halo::math::matrix4x3_transform_point(out, p, *this);
    return out;
}

inline real_vector3d real_matrix4x3::transform_vector(const real_vector3d &v) const
{
    real_vector3d out;
    halo::math::matrix4x3_transform_vector(out, v, *this);
    return out;
}

inline real_vector3d real_matrix4x3::transform_normal(const real_vector3d &n) const
{
    real_vector3d out;
    halo::math::matrix4x3_transform_normal(out, n, *this);
    return out;
}

inline real_plane3d real_matrix4x3::transform_plane(const real_plane3d &p) const
{
    real_plane3d out;
    halo::math::matrix4x3_transform_plane(out, *this, p);
    return out;
}

inline real_point3d real_matrix4x3::inverse_transform_point(const real_point3d &p) const
{
    real_point3d out;
    halo::math::matrix4x3_inverse_transform_point(*this, out, p);
    return out;
}

inline real_vector3d real_matrix4x3::inverse_transform_vector(const real_vector3d &v) const
{
    real_vector3d out;
    halo::math::matrix4x3_inverse_transform_vector(out, v, *this);
    return out;
}

inline real_vector3d real_matrix4x3::inverse_transform_normal(const real_vector3d &n) const
{
    real_vector3d out;
    halo::math::matrix4x3_inverse_transform_normal(out, n, *this);
    return out;
}

inline real_matrix4x3 real_matrix4x3::inverse() const
{
    real_matrix4x3 out;
    halo::math::matrix4x3_inverse(&out, *this);
    return out;
}

inline real_quaternion real_matrix4x3::to_quaternion() const
{
    real_quaternion out;
    halo::math::quaternion_from_matrix4x3(this, out);
    return out;
}

inline real_matrix4x3 real_matrix4x3::from_euler_angles(real yaw, real pitch, real roll)
{
    real_matrix4x3 out;
    halo::math::matrix4x3_from_euler_angles(out, yaw, pitch, roll);
    return out;
}

inline real_matrix4x3 real_matrix4x3::from_axis_angle(const real_vector3d &axis, real sin_angle, real cos_angle)
{
    real_matrix4x3 out;
    halo::math::matrix4x3_from_axis_angle(out, axis, sin_angle, cos_angle);
    return out;
}

inline real_matrix4x3 real_matrix4x3::from_forward_up(const real_vector3d &forward, const real_vector3d &up, const real_point3d &position)
{
    real_matrix4x3 out;
    halo::math::matrix4x3_from_forward_up_position(&up, &forward, position, &out);
    return out;
}

inline real_matrix4x3 operator*(const real_matrix4x3 &a, const real_matrix4x3 &b)
{
    real_matrix4x3 out;
    halo::math::matrix4x3_multiply(&a, &b, &out);
    return out;
}
