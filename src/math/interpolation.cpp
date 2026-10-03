/**
 * @file src/math/interpolation.cpp
 * Interpolation, lerps, cubic curves and the bounded acceleration ramp profile.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/glm_interop.hpp"

#include "tags.h"

namespace halo::math {

void vector3d_lerp(real_vector3d &out, const real_vector3d &a, const real_vector3d &b, real t)
{
    const real one_minus_t = 1.0f - t;
    out = vector_from_glm(t * to_glm(a) + one_minus_t * to_glm(b));
}

void real_lerp_clamped(real &out, real a, real b, real t)
{
    real value;

    value = b * t + (1.0f - t) * a;
    if (value < 0.0f) {
        out = 0.0f;
    } else if (1.0f < value) {
        out = 1.0f;
    } else {
        out = value;
    }
}

uint8_t real_seek_toward_clamped(int wrap, real &velocity, real &value, real target, real accel, real max_speed, real range_min, real range_max)
{
    real old_velocity;
    real delta;
    real max_accel;

    old_velocity = velocity;
    delta = target - value;

    if (wrap) {
        real half_range = (range_max - range_min) * 0.5f;
        if (delta <= half_range) {
            if (delta < -half_range) {
                delta = half_range + half_range + delta;
            }
        } else {
            delta = delta - (half_range + half_range);
        }
    }

    max_accel = accel;
    if (max_speed < accel) {
        max_accel = max_speed;
    }

    if (max_accel < (real)fabs((double)(delta - old_velocity))) {
        real desired_speed_sq = (accel + accel) * (real)fabs((double)delta);
        real desired_speed = max_speed;
        real velocity_delta;
        real clamped_velocity_delta;
        real new_velocity;
        real new_value;

        if (desired_speed_sq < max_speed * max_speed) {
            desired_speed = (real)sqrt((double)desired_speed_sq);
        }
        if (delta < 0.0f) {
            desired_speed = -desired_speed;
        }

        velocity_delta = desired_speed - old_velocity;
        clamped_velocity_delta = velocity_delta;
        if (accel < (real)fabs((double)velocity_delta)) {
            clamped_velocity_delta = (velocity_delta < 0.0f) ? -accel : accel;
        }

        new_velocity = old_velocity + clamped_velocity_delta;
        new_value = clamped_velocity_delta * 0.5f + new_velocity + value;

        if (wrap) {
            if (new_value < range_min) {
                new_value = (range_max - range_min) + new_value;
            } else if (new_value > range_max) {
                new_value = new_value - (range_max - range_min);
            }
        }

        velocity = new_velocity;
        if (new_value < range_min) {
            value = range_min;
        } else {
            value = (new_value <= range_max) ? new_value : range_max;
        }
        return 0;
    }

    velocity = 0.0f;
    if (target < range_min) {
        value = range_min;
    } else {
        value = (target <= range_max) ? target : range_max;
    }
    return 1;
}

uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold)
{
    real span;
    uint8_t b;
    real value;

    span = hi - lo;
    b = (uint8_t)static_cast<int>((double)((threshold - lo) / span * 255.0f));

    while (1) {
        if (b == 0) {
            return 0;
        }
        value = hi;
        if (b != 0xff) {
            value = (real)b * 0.003921569f * span + lo;
        }
        if (value <= threshold) {
            break;
        }
        b = b - 1;
    }
    return b;
}

void vector3d_barycentric_interpolate(real_vector3d &out, const real_vector3d &v1, const real_vector3d &v2, const real_vector3d &v0, float w2, float w1)
{
    out.i = (v2.i - v0.i) * w2 + (v1.i - v0.i) * w1 + v0.i;
    out.j = (v2.j - v0.j) * w2 + (v1.j - v0.j) * w1 + v0.j;
    out.k = (v2.k - v0.k) * w2 + (v1.k - v0.k) * w1 + v0.k;
}

float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3, float x0, float x1, float x2, float x3, float x)
{
    double d_x1x2 = ((double)y2 - (double)y1) / ((double)x2 - (double)x1);
    double d_x0x1 = ((double)y1 - (double)y0) / ((double)x1 - (double)x0);
    double d_x0x1x2 = (d_x1x2 - d_x0x1) / ((double)x2 - (double)x0);

    double d_x2x3 = ((double)y3 - (double)y2) / ((double)x3 - (double)x2);
    double d_x1x2x3 = (d_x2x3 - d_x1x2) / ((double)x3 - (double)x1);
    double d_x0x1x2x3 = (d_x1x2x3 - d_x0x1x2) / ((double)x3 - (double)x0);

    return (float)(((double)x - (double)x0) *
                    (((double)x - (double)x1) *
                     (((double)x - (double)x2) * d_x0x1x2x3 + d_x0x1x2) + d_x0x1) + (double)y0);
}

void vector3d_cubic_interpolate(real_vector3d &out, const real_vector3d &p0, const real_vector3d &p1, const real_vector3d &p2, const real_vector3d &p3, float t0, float t1, float t2, float t3, float t)
{
    out.i = cubic_interpolate_divided_difference(p0.i, p1.i, p2.i, p3.i, t0, t1, t2, t3, t);
    out.j = cubic_interpolate_divided_difference(p0.j, p1.j, p2.j, p3.j, t0, t1, t2, t3, t);
    out.k = cubic_interpolate_divided_difference(p0.k, p1.k, p2.k, p3.k, t0, t1, t2, t3, t);
}

real real_inverse_lerp_clamped(real value, real ref_k0, real ref_k1)
{
    if (ref_k1 <= ref_k0) {
        if ((value < ref_k1) != (value == ref_k1)) {
            return 1.0f;
        }
        if (value < ref_k0) {
            return (ref_k0 - value) / (ref_k0 - ref_k1);
        }
    } else if ((value < ref_k0) == (value == ref_k0)) {
        if (value < ref_k1) {
            return (value - ref_k0) / (ref_k1 - ref_k0);
        }
        return 1.0f;
    }
    return 0.0f;
}

void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity, real max_acceleration, bounded_ramp_profile *profile)
{
    real half_v_over_a;
    real reach;
    real accel_time;
    real b, disc;
    real end_velocity;
    real root_1, root_2;
    uint8_t moving_forward;

    profile->start_position = position_error;
    profile->start_velocity = initial_velocity;

    if ((real)fabs((double)position_error) < 0.001f && (real)fabs((double)initial_velocity) < 0.001f) {
        profile->within_dead_zone = 1;
        *(uint32_t *)&profile->phase1_acceleration = 0;
        *(uint32_t *)&profile->phase1_duration = 0;
        *(uint32_t *)&profile->phase2_duration = 0;
        *(uint32_t *)&profile->phase3_acceleration = 0;
        *(uint32_t *)&profile->phase3_duration = 0;
        return;
    }
    profile->within_dead_zone = 0;

    half_v_over_a = (real)fabs((double)initial_velocity) / max_acceleration;
    moving_forward = (uint8_t)(initial_velocity > 0.0f);
    if (half_v_over_a * 0.5f * initial_velocity * 0.5f + position_error < 0.0f) {
        bounded_ramp_profile_build(-position_error, -initial_velocity, max_velocity, max_acceleration, profile);
        profile->start_position = -profile->start_position;
        profile->start_velocity = -profile->start_velocity;
        profile->phase1_acceleration = -profile->phase1_acceleration;
        profile->phase3_acceleration = -profile->phase3_acceleration;
        return;
    }

    reach = initial_velocity * 0.5f * half_v_over_a + position_error;
    if (reach < 0.0f) {
        *(uint32_t *)&profile->phase1_acceleration = 0;
        *(uint32_t *)&profile->phase1_duration = 0;
        *(uint32_t *)&profile->phase2_duration = 0;
        accel_time = (initial_velocity * initial_velocity) / (position_error + position_error);
        profile->phase3_acceleration = accel_time;
        profile->phase3_duration = -(initial_velocity / accel_time);
        return;
    }

    if (!moving_forward) {
        b = -max_acceleration;
        disc = (real)sqrt((double)((initial_velocity + initial_velocity) * (initial_velocity + initial_velocity) -
                      b * reach * 4.0f));
        root_1 = (-(initial_velocity + initial_velocity) - disc) / (b + b);
        root_2 = (disc - (initial_velocity + initial_velocity)) / (b + b);
        if (!(root_1 < 0.0f) && (root_2 < 0.0f || root_1 < root_2)) {
            end_velocity = root_1;
        } else if (0.0f > root_2) {
            end_velocity = 0.0f;
        } else {
            end_velocity = root_2;
        }
    } else {
        end_velocity = (real)sqrt((double)(reach / max_acceleration));
    }

    if (0.0f < max_velocity) {
        if (!moving_forward) {
            max_velocity = initial_velocity + max_velocity;
        }
        max_velocity = max_velocity / max_acceleration;
        if (max_velocity < 0.0f) {
            max_velocity = 0.0f;
        }
        if (max_velocity < end_velocity) {
            goto have_accel_time;
        }
    }
    max_velocity = end_velocity;

have_accel_time:
    profile->phase1_acceleration = -max_acceleration;
    profile->phase3_acceleration = max_acceleration;
    if (!moving_forward) {
        profile->phase1_duration = max_velocity;
        half_v_over_a = max_velocity + half_v_over_a;
    } else {
        profile->phase1_duration = max_velocity + half_v_over_a;
        half_v_over_a = max_velocity;
    }
    profile->phase3_duration = half_v_over_a;

    if (max_velocity < end_velocity) {
        real coast_velocity = -max_acceleration * profile->phase1_duration + initial_velocity;
        real coast_time = end_velocity - max_velocity;
        real term = coast_time * coast_velocity;
        profile->phase2_duration =
            ((term + term) - coast_time * coast_time * max_acceleration) / coast_velocity;
        return;
    }
    *(uint32_t *)&profile->phase2_duration = 0;
}

void bounded_ramp_profile_synchronize(bounded_ramp_profile *profile_a, bounded_ramp_profile *profile_b, real max_acceleration)
{
    real duration_a, duration_b, extra;
    bounded_ramp_profile *shorter;
    real cruise_velocity, adjusted_duration, term, radicand;

    if (profile_a->within_dead_zone != 0 || profile_b->within_dead_zone != 0) {
        return;
    }

    duration_a = profile_a->phase3_duration + profile_a->phase2_duration + profile_a->phase1_duration;
    duration_b = profile_b->phase3_duration + profile_b->phase2_duration + profile_b->phase1_duration;

    if (profile_a->phase1_duration > 0.0f && duration_a < duration_b) {
        extra = duration_b - duration_a;
        shorter = profile_a;
    } else {
        if (!(profile_b->phase1_duration > 0.0f && duration_b < duration_a)) {
            return;
        }
        extra = duration_a - duration_b;
        shorter = profile_b;
    }

    if (shorter == 0) {
        return;
    }

    term = (extra + shorter->phase2_duration) * max_acceleration;
    radicand = term * term -
        -extra * (real)fabs((double)(shorter->phase1_duration * shorter->phase1_acceleration +
                                      shorter->start_velocity)) * max_acceleration * 4.0f;
    adjusted_duration = ((real)sqrt((double)radicand) - term) / (max_acceleration + max_acceleration);

    cruise_velocity = shorter->phase1_duration > shorter->phase3_duration ?
        shorter->phase3_duration : shorter->phase1_duration;
    if (cruise_velocity < adjusted_duration) {
        adjusted_duration = cruise_velocity;
    }

    if (0.0f < adjusted_duration) {
        real new_start_velocity = (shorter->phase1_duration - adjusted_duration) * shorter->phase1_acceleration +
            shorter->start_velocity;
        shorter->phase1_duration = shorter->phase1_duration - adjusted_duration;
        shorter->phase3_duration = shorter->phase3_duration - adjusted_duration;
        shorter->phase2_duration =
            ((new_start_velocity + new_start_velocity + adjusted_duration * shorter->phase1_acceleration) *
             adjusted_duration) / new_start_velocity;
    }
}

uint8_t bounded_ramp_profile_evaluate(const bounded_ramp_profile &profile, real time, real start_position, real &out_position, real start_velocity, real &out_velocity)
{
    uint8_t coasting;
    real position;
    real velocity;
    real phase_time;

    coasting = profile.within_dead_zone;
    position = start_position;
    velocity = start_velocity;

    if (coasting == 0 && 0.0f < time) {
        if (0.0f < profile.phase1_duration) {
            phase_time = time;
            if (profile.phase1_duration < time) {
                phase_time = profile.phase1_duration;
            }
            position = (phase_time * profile.phase1_acceleration * 0.5f + start_velocity) * phase_time + position;
            velocity = phase_time * profile.phase1_acceleration + start_velocity;
            time = time - phase_time;
        }
        if (0.0f < time) {
            if (0.0f < profile.phase2_duration) {
                phase_time = time;
                if (profile.phase2_duration < time) {
                    phase_time = profile.phase2_duration;
                }
                position = velocity * phase_time + position;
                time = time - phase_time;
            }
            if (0.0f < time) {
                if (0.0f < profile.phase3_duration) {
                    phase_time = time;
                    if (profile.phase3_duration < time) {
                        phase_time = profile.phase3_duration;
                    }
                    position = (phase_time * profile.phase3_acceleration * 0.5f + velocity) * phase_time + position;
                    velocity = phase_time * profile.phase3_acceleration + velocity;
                    time = time - phase_time;
                }
                if (0.0f < time) {
                    out_position = position;
                    out_velocity = velocity;
                    return 1;
                }
            }
        }
    }

    out_position = position;
    out_velocity = velocity;
    return coasting;
}

}  // namespace halo::math
