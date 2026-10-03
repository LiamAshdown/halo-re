#include "halo/core/lcg.hpp"
#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

extern "C" {
extern double cos(double x);
extern double sin(double x);
float effect_distribution_function_evaluate(EffectDistributionFunction_t type, float fraction);
void effect_random_direction_from_table(real_point3d *out);
void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset);
real effect_random_fraction();
int16_t effect_random_int_between(int16_t minimum, int16_t maximum);
real effect_random_scaled_range(uint32_t flags, real scale, real base_min, real base_max, uint8_t bit_index);
uint32_t effect_random_uint16();
void effect_random_velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
}
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);

namespace halo::effects {

/**
 * Member form of the original effect_distribution_function_evaluate: evaluate.
 *
 * @address 0x453290
 */
float effect_random::evaluate(EffectDistributionFunction_t type, float fraction)
{
    if (fraction == -1.0f) {
        return 0.0f;
    }
    switch (type) {
    case effectdistributionfunction_start:
        return 1.0f;
    case effectdistributionfunction_end:
        return (fraction < 1.0f) ? 0.0f : 1.0f;
    case effectdistributionfunction_buildup:
        return fraction * fraction;
    case effectdistributionfunction_falloff:
        return (2.0f - fraction) * fraction;
    case effectdistributionfunction_buildup_and_falloff:
        return (3.0f - (fraction + fraction)) * fraction * fraction;
    default:
        return fraction;
    }
}

/**
 * Picks a pseudo-random unit vector out of the shared quasi-uniform sphere point table.
 *
 * @address 0x4505e0
 */
void effect_random::direction_from_table(real_point3d *out)
{
    int16_t index;

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
    *out = halo::math::globals().sphere_point_table[index];
}

/**
 * Rolls a random magnitude in [min, max); if non-zero, scales a random unit vector out of
 * sphere_point_table by it, otherwise returns the global origin point.
 * FIXED (objdump 0x451450..0x451469): effect_property_random_value is called with EDX = 3 (the property bit)
 * and EBX / ESI / EDI passed straight through from the caller (the effect and its part's a / b scale bits;
 * effect_event_apply 0x452f5e..0x452f61). The draft passed bit 0 and zeros.
 *
 * @address 0x451450
 */
void effect_random::direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset)
{
    real magnitude = halo::effects::effect_property_random_value(3, self, a_bitset, b_bitset, seed, min, max);

    if (magnitude != 0.0f) {
        int16_t index;

        *seed = *seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((*seed >> k_random_value_shift) * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);

        out->x = magnitude * halo::math::globals().sphere_point_table[index].x;
        out->y = magnitude * halo::math::globals().sphere_point_table[index].y;
        out->z = magnitude * halo::math::globals().sphere_point_table[index].z;
    } else {
        *out = *global_origin3d_pointer;
    }
}

/**
 * Returns the next pseudo-random fraction in [0, 1).
 *
 * @address 0x4505b0
 */
real effect_random::fraction()
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale;
}

/**
 * Returns a pseudo-random integer in [minimum, maximum).
 * the result is 16-bit: the original's upper 16 bits are whatever the caller left in ECX (minimum's register)
 *
 * @address 0x44c800
 */
int16_t effect_random::int_between(int16_t minimum, int16_t maximum)
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return (int16_t)((int)((uint32_t)(((int)maximum - (int)minimum) * (int)(halo::math::globals().effect_random_seed >> k_random_value_shift)) >> 16) +
                     minimum);
}

/**
 * Returns a random value in [base_min, base_max), where base_min and (base_max - base_min) are
 * each multiplied by `scale` when their respective flag bit (bit_index and bit_index + 1) is set
 * in `flags`.
 *
 * @address 0x44c840
 */
real effect_random::scaled_range(uint32_t flags, real scale, real base_min, real base_max, uint8_t bit_index)
{
    real lower = base_min;
    real span;

    if ((flags & (1u << (bit_index & 0x1f))) != 0) {
        lower = scale * base_min;
    }
    span = base_max - base_min;
    if ((flags & (1u << ((bit_index + 1) & 0x1f))) != 0) {
        span = span * scale;
    }
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale * span + lower;
}

/**
 * Member form of the original effect_random_uint16: uint16.
 *
 * @address 0x44da40
 */
uint32_t effect_random::uint16()
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return halo::math::globals().effect_random_seed >> k_random_value_shift;
}

/**
 * Copies `direction` into `out_direction`, rotates it by a random angle (up to `angle`, scaled
 * by the A/B scales when bit 2 of either bitset is set) about a random axis, and writes
 * out_direction scaled by a random magnitude in [min, max) into `out_velocity`.
 *
 * @address 0x451310
 */
void effect_random::velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset)
{
    real magnitude = halo::effects::effect_property_random_value(0, self, a_bitset, b_bitset, seed, min, max);
    real angle = angle_max;

    *out_direction = *direction;

    if ((a_bitset & 4) != 0) {
        angle = angle_max * self->a_scale;
    }
    if ((b_bitset & 4) != 0) {
        angle = angle * self->b_scale;
    }

    *seed = *seed * k_random_multiplier + k_random_increment;
    angle = (real)(*seed >> k_random_value_shift) * halo::k_unit_word_scale * angle;

    if (angle != 0.0f) {
        real sin_angle;
        real cos_angle;
        real_vector3d axis;
        int16_t index;

        cos_angle = (real)cos(angle);
        *seed = *seed * k_random_multiplier + k_random_increment;
        sin_angle = (real)sin(angle);

        index = (int16_t)(((*seed >> k_random_value_shift) * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
        axis.i = halo::math::globals().sphere_point_table[index].x;
        axis.j = halo::math::globals().sphere_point_table[index].y;
        axis.k = halo::math::globals().sphere_point_table[index].z;

        halo::math::vector3d_rotate_about_axis(*out_direction, axis, sin_angle, cos_angle);
    }

    out_velocity->i = magnitude * out_direction->i;
    out_velocity->j = magnitude * out_direction->j;
    out_velocity->k = magnitude * out_direction->k;
}

}

namespace halo::effects {

float effect_distribution_function_evaluate(EffectDistributionFunction_t type, float fraction)
{
    return halo::effects::effect_random::evaluate(type, fraction);
}

void effect_random_direction_from_table(real_point3d *out)
{
    halo::effects::effect_random::direction_from_table(out);
}

void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset)
{
    halo::effects::effect_random::direction_vector(seed, out, min, max, self, a_bitset, b_bitset);
}

real effect_random_fraction()
{
    return halo::effects::effect_random::fraction();
}

int16_t effect_random_int_between(int16_t minimum, int16_t maximum)
{
    return halo::effects::effect_random::int_between(minimum, maximum);
}

real effect_random_scaled_range(uint32_t flags, real scale, real base_min, real base_max, uint8_t bit_index)
{
    return halo::effects::effect_random::scaled_range(flags, scale, base_min, base_max, bit_index);
}

uint32_t effect_random_uint16()
{
    return halo::effects::effect_random::uint16();
}

void effect_random_velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset)
{
    halo::effects::effect_random::velocity_vector(self, seed, direction, out_direction, out_velocity, min, max, angle_max, a_bitset, b_bitset);
}

}
