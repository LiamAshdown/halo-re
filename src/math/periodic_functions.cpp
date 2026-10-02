/**
 * @file src/math/periodic_functions.cpp
 * Periodic (wave) and transition (easing) function tables and evaluators.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/math/math.hpp"

#include "win32.h"
#include "tags.h"

extern "C" {
extern periodic_function_table *periodic_function_tables[12];
extern periodic_function_table *transition_function_tables[6];
extern uint8_t periodic_functions_initialized;
extern random_seed random_seed_global;
extern double fmod(double x, double y);
extern long lrint(double x);
extern double cos(double x);
extern double sin(double x);
extern double pow(double base, double exponent);
extern int __ftol(double value);
}

namespace halo::math {

void periodic_function_tables_init()
{
    int16_t i;
    uint8_t *table;

    periodic_functions_initialized = 1;
    random_seed_global = 0x20f3f660;

    for (i = 0; i < k_periodic_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        periodic_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            periodic_functions_initialized = 0;
        } else {
            periodic_function_build_table(i, table);
        }
    }

    for (i = 0; i < k_transition_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        transition_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            periodic_functions_initialized = 0;
        } else {
            periodic_function_build_transition_table(i, table);
        }
    }
}

void periodic_function_tables_free()
{
    int16_t i;

    if (periodic_functions_initialized != 0) {
        for (i = 0; i < 12; i++) {
            GlobalFree(periodic_function_tables[i]);
        }
        for (i = 0; i < 6; i++) {
            GlobalFree(transition_function_tables[i]);
        }
        periodic_functions_initialized = 0;
    }
}

real periodic_function_evaluate(periodic_function_t type, double time)
{
    real frac_part;
    real sample_a, sample_b;
    uint32_t index;
    periodic_function_table *table;
    double scaled_time;

    if (type == 0) {
        return 1.0f;
    }
    if (periodic_functions_initialized == 0) {
        return 0.0f;
    }

    scaled_time = time * 25.600000381469727;
    frac_part = (real)fmod(scaled_time, 1.0);
    index = (uint32_t)(int32_t)lrint(scaled_time - (double)frac_part) & k_periodic_function_table_mask;
    table = periodic_function_tables[type];
    sample_a = (real)table->samples[index] * 0.003921569f;
    sample_b = (real)table->samples[(index + 1) & k_periodic_function_table_mask] * 0.003921569f;

    if (((1 << (type & 0x1f)) & k_periodic_function_wrapping_mask) == 0) {
        return sample_b * frac_part + (1.0f - frac_part) * sample_a;
    }
    if (0.75f < sample_a && sample_b < 0.25f) {
        sample_b = sample_b + 1.0f;
    }
    sample_a = sample_b * frac_part + (1.0f - frac_part) * sample_a;
    if (1.0f < sample_a) {
        return sample_a - 1.0f;
    }
    return sample_a;
}

real transition_function_evaluate(transition_function_t type, real phase)
{
    real clamped;
    real frac_part;
    int16_t index;
    periodic_function_table *table;
    double x;

    clamped = phase;
    if (0.0f <= clamped) {
        if (1.0f < clamped) {
            clamped = 1.0f;
        }
    } else {
        clamped = 0.0f;
    }

    if (type == 0) {
        return clamped;
    }
    if (periodic_functions_initialized == 0) {
        return 0.0f;
    }

    table = transition_function_tables[type];
    x = (double)(clamped * 1023.0f);
    frac_part = (real)fmod(x, 1.0);
    index = (int16_t)(int32_t)lrint(x - 0.5);
    if (index != 0x3ff) {
        return (real)table->samples[index + 1] * 0.003921569f * frac_part +
               (1.0f - frac_part) * (real)table->samples[index] * 0.003921569f;
    }
    return (real)table->samples[0x3ff] * 0.003921569f;
}

void periodic_function_build_noise_table(real *table)
{
    real cumulative;
    int32_t i;
    real c1, c2, c3;

    cumulative = 0.0f;
    for (i = 0; i < 1024; i++) {
        table[i] = cumulative;
        c1 = (real)cos((double)((real)i * 0.044792242f));
        c2 = (real)cos((double)((real)i * 0.03129321f));
        random_seed_global = (((random_seed_global * k_random_multiplier + k_random_increment) *
                                k_random_multiplier + k_random_increment) *
                               k_random_multiplier + k_random_increment) *
                              k_random_multiplier + k_random_increment;
        c3 = (real)cos((double)((real)i * 0.025157286f));
        cumulative = (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f *
                     (c3 + 1.0f + c2 + 1.0f + c1 + 1.0f + 0.25f) + cumulative + 0.25f;
    }
    for (i = 0; i < 1024; i++) {
        table[i] = (1.0f / cumulative) * table[i];
    }
}

void periodic_function_build_transition_table(transition_function_t type, uint8_t *table)
{
    int32_t i;
    real t;
    real value;
    int32_t scaled;

    value = 0.0f;

    for (i = 0; i < 1024; i++) {
        t = (real)i * 0.0009775171f;

        switch ((int32_t)type) {
        case _transition_function_linear:
            value = t;
            break;
        case _transition_function_early:
            value = (real)pow((double)t, 0.5);
            break;
        case _transition_function_very_early:
            value = (real)pow((double)t, 0.25);
            break;
        case _transition_function_late:
            value = (real)pow((double)t, 2.0);
            break;
        case _transition_function_very_late:
            value = (real)pow((double)t, 4.0);
            break;
        case _transition_function_cosine:
            value = ((real)sin((double)(t * 3.1415927f - 1.5707964f)) + 1.0f) * 0.5f;
            break;
        default:
            break;
        }

        scaled = __ftol((double)(255.0f * value));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        table[i] = (uint8_t)scaled;
    }
}

void periodic_function_build_table(periodic_function_t type, uint8_t *out)
{
    real noise[1024];
    real wave[1024];
    real minimum, maximum;
    real range;
    real t;
    real u;
    real value;
    real frac;
    uint32_t seed;
    int32_t i;
    int32_t scaled;

    minimum = 3.4028235e+38f;
    maximum = -3.4028235e+38f;
    periodic_function_build_noise_table(noise);

    value = -3.4028235e+38f;
    seed = random_seed_global;

    for (i = 0; i < 1024; i++) {
        t = (real)i * 0.027343748f;
        u = noise[i] * 28.0f;

        switch ((int32_t)type) {
        case _periodic_function_one:
            value = 1.0f;
            break;
        case _periodic_function_zero:
            value = 0.0f;
            break;
        case _periodic_function_cosine:
            value = (real)cos((double)(t * 6.2831855f));
            break;
        case _periodic_function_cosine_variable_period:
            value = (real)cos((double)(u * 6.2831855f));
            break;
        case _periodic_function_diagonal_wave:
        case _periodic_function_diagonal_wave_variable_period:
            frac = (real)fmod((double)((type == _periodic_function_diagonal_wave) ? t : u), 1.0);
            if (0.5f <= frac) {
                value = 1.0f - ((frac - 0.5f) + (frac - 0.5f));
            } else {
                value = frac + frac;
            }
            break;
        case _periodic_function_slide:
            value = (real)fmod((double)t, 1.0);
            break;
        case _periodic_function_slide_variable_period:
            value = (real)fmod((double)u, 1.0);
            break;
        case _periodic_function_noise:
            seed = seed * k_random_multiplier + k_random_increment;
            value = (real)(seed >> k_random_value_shift) * 1.5259022e-05f;
            random_seed_global = seed;
            break;
        case _periodic_function_jitter:
        case _periodic_function_wander: {
            real a = (real)cos((double)(t * 0.8975979f));
            real b = (real)cos((double)(t * 25.132742f));
            real c = (real)cos((double)(t * 43.9823f));
            real d = (real)sin((double)(t * 1.5707964f));
            real e = (real)sin((double)(t * 3.1415927f));
            real f = (real)cos((double)(t * 6.2831855f));
            value = f * e + (d * c + b * a) * 0.5f;
            break;
        }
        case _periodic_function_spark:
            value = (real)fmod((double)u, 1.0);
            value = value * value;
            break;
        default:
            break;
        }

        if (maximum < value) {
            maximum = value;
        }
        if (value < minimum) {
            minimum = value;
        }
        wave[i] = value;
    }

    if (((int32_t)1 << (int32_t)type) & k_periodic_function_wrapping_mask) {
        range = 0.0f;
    } else {
        range = maximum - minimum;
    }

    for (i = 0; i < 1024; i++) {
        value = wave[i];
        if (range != 0.0f) {
            value = (value - minimum) / range;
        }
        scaled = __ftol((double)(value * 255.0f));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        out[i] = (uint8_t)scaled;
    }
}

}  // namespace halo::math
