/**
 * @file include/halo/math/interpolation.hpp
 * Interpolation, lerps, cubic curves and the bounded acceleration ramp profile.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * out = t*a + (1-t)*b.
 *
 * Original register convention: EAX -> out, ECX -> a, EDX -> b, stack -> t.
 * @address 0x004cd8c0
 */
void vector3d_lerp(real_vector3d &out, const real_vector3d &a, const real_vector3d &b, real t);

/**
 * *out = clamp(lerp(a, b, t), 0, 1).
 *
 * Original register convention: ECX -> out, stack -> (a, b, t).
 * @address 0x004cd900
 */
void real_lerp_clamped(real &out, real a, real b, real t);

/**
 * Moves `value` toward `target` with bounded acceleration and speed, optionally wrapping around [range_min,
 * range_max], and clamps the result to the range. Returns 1 when it snapped onto the target this step.
 *
 * Original register convention: EDX(DL) -> wrap, ESI -> velocity, EDI -> value, stack -> (target, accel,.
 * @address 0x004cf360
 */
uint8_t real_seek_toward_clamped(int wrap, real &velocity, real &value, real target, real accel, real max_speed, real range_min, real range_max);

/**
 * Returns the largest byte b with lerp(lo, hi, b/255) <= threshold, starting from the truncated inverse lerp
 * and stepping down; byte 255 maps to exactly `hi`.
 * @address 0x004cf7a0
 */
uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold);

/**
 * out = (v2 - v0)*w2 + (v1 - v0)*w1 + v0.
 *
 * Original register convention: EAX -> out, ECX -> v1, EDX -> v2, ESI -> v0, stack -> w2, w1.
 * @address 0x004f06d0
 */
void vector3d_barycentric_interpolate(real_vector3d &out, const real_vector3d &v1, const real_vector3d &v2, const real_vector3d &v0, float w2, float w1);

/**
 * Evaluates Newton's divided-difference cubic through the four samples (x0,y0)..(x3,y3) at `x`, in double
 * precision.
 *
 * Original register convention: cdecl(y0, y1, y2, y3, x0, x1, x2, x3, x).
 * @address 0x004fca60
 */
float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3, float x0, float x1, float x2, float x3, float x);

/**
 * Interpolates each component through four control points at the four parameter values with
 * cubic_interpolate_divided_difference.
 *
 * Original register convention: EBX -> p1, EDI -> p2, ESI -> p3, stack -> out, p0, t0, t1, t2, t3, t.
 * @address 0x004fcb00
 */
void vector3d_cubic_interpolate(real_vector3d &out, const real_vector3d &p0, const real_vector3d &p1, const real_vector3d &p2, const real_vector3d &p3, float t0, float t1, float t2, float t3, float t);

/**
 * Returns where `value` lies between ref_k0 (0) and ref_k1 (1), clamped to [0, 1]; the references may be in
 * either order.
 *
 * Original register convention: stack -> (value, ref_k0, ref_k1).
 * @address 0x00507430
 */
real real_inverse_lerp_clamped(real value, real ref_k0, real ref_k1);

/**
 * Builds a three-phase (accelerate / coast / decelerate) motion profile that brings a position error and
 * velocity to rest within max_velocity and max_acceleration. Recurses once with negated inputs when the error
 * points the other way.
 *
 * Original register convention: stack -> (position_error, initial_velocity, max_velocity, max_acceleration,
 * profile).
 * @address 0x00564580
 */
void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity, real max_acceleration, bounded_ramp_profile *profile);

/**
 * When both profiles are live, stretches the coast phase of the one that finishes sooner so both last the same
 * total time.
 *
 * Original register convention: ECX -> profile_a, EDX -> profile_b, stack -> max_acceleration.
 * @address 0x00564840
 */
void bounded_ramp_profile_synchronize(bounded_ramp_profile *profile_a, bounded_ramp_profile *profile_b, real max_acceleration);

/**
 * Advances a ramp profile by `time` from (start_position, start_velocity), writing the reached position and
 * velocity. Returns nonzero once `time` runs past every phase; a profile already in its dead zone returns that
 * flag unchanged.
 *
 * Original register convention: ECX -> profile, stack -> (time, start_position, out_position, start_velocity,
 * out_velocity).
 * @address 0x00564990
 */
uint8_t bounded_ramp_profile_evaluate(const bounded_ramp_profile &profile, real time, real start_position, real &out_position, real start_velocity, real &out_velocity);

}  // namespace halo::math
