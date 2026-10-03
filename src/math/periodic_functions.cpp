/**
 * @file src/math/periodic_functions.cpp
 * Periodic (wave) and transition (easing) function tables and evaluators.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "win32.h"
#include "tags.h"

namespace halo::math {

/** Seed of the random stream the periodic function tables are generated from. */
inline constexpr uint32_t k_periodic_function_noise_seed = 0x20f3f660;

namespace {

/** A wave whose value is the same at every phase (one, zero). */
class constant_wave final : public periodic_wave {
public:
    explicit constexpr constant_wave(real value) : value(value) {}
    real sample(const wave_phase &, random_seed &) const override { return value; }

private:
    real value;
};

/** cos(2*pi*phase), on the linear or the noise-warped phase. */
class cosine_wave final : public periodic_wave {
public:
    explicit constexpr cosine_wave(bool warped) : warped(warped) {}
    real sample(const wave_phase &phase, random_seed &) const override
    {
        return (real)cos((double)((warped ? phase.warped : phase.linear) * 6.2831855f));
    }

private:
    bool warped;
};

/** Triangle wave folded from frac(phase): 2f below 0.5, 1 - 2(f - 0.5) above. */
class diagonal_wave final : public periodic_wave {
public:
    explicit constexpr diagonal_wave(bool warped) : warped(warped) {}
    real sample(const wave_phase &phase, random_seed &) const override
    {
        const real frac = (real)fmod((double)(warped ? phase.warped : phase.linear), 1.0);
        if (0.5f <= frac) {
            return 1.0f - ((frac - 0.5f) + (frac - 0.5f));
        }
        return frac + frac;
    }

private:
    bool warped;
};

/** Sawtooth frac(phase); the table builder does not rescale it (already in [0, 1)). */
class slide_wave final : public periodic_wave {
public:
    explicit constexpr slide_wave(bool warped) : warped(warped) {}
    real sample(const wave_phase &phase, random_seed &) const override
    {
        return (real)fmod((double)(warped ? phase.warped : phase.linear), 1.0);
    }

private:
    bool warped;
};

/** White noise: one draw of the builder's copy of random_seed_global, which is written back after every draw. */
class noise_wave final : public periodic_wave {
public:
    real sample(const wave_phase &, random_seed &seed) const override
    {
        const real value = random_stream(seed).next_real();
        globals().random_seed_global = seed;
        return value;
    }
};

/** Sum of four cosines and two sines of the linear phase (jitter and wander share it). */
class jitter_wave final : public periodic_wave {
public:
    real sample(const wave_phase &phase, random_seed &) const override
    {
        const real t = phase.linear;
        const real a = (real)cos((double)(t * 0.8975979f));
        const real b = (real)cos((double)(t * 25.132742f));
        const real c = (real)cos((double)(t * 43.9823f));
        const real d = (real)sin((double)(t * 1.5707964f));
        const real e = (real)sin((double)(t * 3.1415927f));
        const real f = (real)cos((double)(t * 6.2831855f));
        return f * e + (d * c + b * a) * 0.5f;
    }
};

/** frac(warped phase) squared. */
class spark_wave final : public periodic_wave {
public:
    real sample(const wave_phase &phase, random_seed &) const override
    {
        const real value = (real)fmod((double)phase.warped, 1.0);
        return value * value;
    }
};

/** t, the linear ramp. */
class linear_curve final : public transition_curve {
public:
    real value(real t) const override { return t; }
};

/** pow(t, exponent) through the CRT pow. */
class power_curve final : public transition_curve {
public:
    explicit constexpr power_curve(double exponent) : exponent(exponent) {}
    real value(real t) const override { return (real)pow((double)t, exponent); }

private:
    double exponent;
};

/** (sin(t*pi - pi/2) + 1) / 2, the raised cosine. */
class cosine_curve final : public transition_curve {
public:
    real value(real t) const override { return ((real)sin((double)(t * 3.1415927f - 1.5707964f)) + 1.0f) * 0.5f; }
};

const constant_wave k_wave_one(1.0f);
const constant_wave k_wave_zero(0.0f);
const cosine_wave k_wave_cosine(false);
const cosine_wave k_wave_cosine_variable_period(true);
const diagonal_wave k_wave_diagonal(false);
const diagonal_wave k_wave_diagonal_variable_period(true);
const slide_wave k_wave_slide(false);
const slide_wave k_wave_slide_variable_period(true);
const noise_wave k_wave_noise;
const jitter_wave k_wave_jitter;
const spark_wave k_wave_spark;

/** The registry, indexed by periodic_function_type; jitter and wander are the same wave. */
const periodic_wave *const k_periodic_waves[k_periodic_function_count] = {
    &k_wave_one, &k_wave_zero, &k_wave_cosine, &k_wave_cosine_variable_period, &k_wave_diagonal,
    &k_wave_diagonal_variable_period, &k_wave_slide, &k_wave_slide_variable_period, &k_wave_noise, &k_wave_jitter,
    &k_wave_jitter, &k_wave_spark,
};

const linear_curve k_curve_linear;
const power_curve k_curve_early(0.5);
const power_curve k_curve_very_early(0.25);
const power_curve k_curve_late(2.0);
const power_curve k_curve_very_late(4.0);
const cosine_curve k_curve_cosine;

/** The registry, indexed by transition_function_type. */
const transition_curve *const k_transition_curves[k_transition_function_count] = {
    &k_curve_linear, &k_curve_early, &k_curve_very_early, &k_curve_late, &k_curve_very_late, &k_curve_cosine,
};

}  // namespace

const periodic_wave *periodic_wave_for(periodic_function_type type)
{
    const int32_t index = static_cast<int32_t>(type);
    return (index >= 0 && index < k_periodic_function_count) ? k_periodic_waves[index] : nullptr;
}

const transition_curve *transition_curve_for(transition_function_type type)
{
    const int32_t index = static_cast<int32_t>(type);
    return (index >= 0 && index < k_transition_function_count) ? k_transition_curves[index] : nullptr;
}

void periodic_function_tables_init()
{
    int16_t i;
    uint8_t *table;

    globals().periodic_functions_initialized = 1;
    globals().random_seed_global = k_periodic_function_noise_seed;

    for (i = 0; i < k_periodic_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        globals().periodic_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            globals().periodic_functions_initialized = 0;
        } else {
            periodic_function_build_table(static_cast<periodic_function_type>(i), table);
        }
    }

    for (i = 0; i < k_transition_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        globals().transition_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            globals().periodic_functions_initialized = 0;
        } else {
            periodic_function_build_transition_table(static_cast<transition_function_type>(i), table);
        }
    }
}

void periodic_function_tables_free()
{
    int16_t i;

    if (globals().periodic_functions_initialized != 0) {
        for (i = 0; i < 12; i++) {
            GlobalFree(globals().periodic_function_tables[i]);
        }
        for (i = 0; i < 6; i++) {
            GlobalFree(globals().transition_function_tables[i]);
        }
        globals().periodic_functions_initialized = 0;
    }
}

real periodic_function_evaluate(periodic_function_type type, double time)
{
    const int16_t selector = static_cast<int16_t>(type);
    real frac_part;
    real sample_a, sample_b;
    uint32_t index;
    periodic_function_table *table;
    double scaled_time;

    if (selector == 0) {
        return 1.0f;
    }
    if (globals().periodic_functions_initialized == 0) {
        return 0.0f;
    }

    scaled_time = time * 25.600000381469727;
    frac_part = (real)fmod(scaled_time, 1.0);
    index = (uint32_t)(int32_t)lrint(scaled_time - (double)frac_part) & k_periodic_function_table_mask;
    table = globals().periodic_function_tables[selector];
    sample_a = (real)table->samples[index] * 0.003921569f;
    sample_b = (real)table->samples[(index + 1) & k_periodic_function_table_mask] * 0.003921569f;

    if (((1 << (selector & 0x1f)) & k_periodic_function_wrapping_mask) == 0) {
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

real transition_function_evaluate(transition_function_type type, real phase)
{
    const int16_t selector = static_cast<int16_t>(type);
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

    if (selector == 0) {
        return clamped;
    }
    if (globals().periodic_functions_initialized == 0) {
        return 0.0f;
    }

    table = globals().transition_function_tables[selector];
    x = (double)(clamped * 1023.0f);
    frac_part = (real)fmod(x, 1.0);
    index = (int16_t)(int32_t)lrint(x - 0.5);
    if (index != k_periodic_function_table_mask) {
        return (real)table->samples[index + 1] * 0.003921569f * frac_part +
               (1.0f - frac_part) * (real)table->samples[index] * 0.003921569f;
    }
    return (real)table->samples[k_periodic_function_table_mask] * 0.003921569f;
}

void periodic_function_build_noise_table(real *table)
{
    real cumulative;
    int32_t i;
    real c1, c2, c3;
    random_stream rng = simulation_random();

    cumulative = 0.0f;
    for (i = 0; i < 1024; i++) {
        table[i] = cumulative;
        c1 = (real)cos((double)((real)i * 0.044792242f));
        c2 = (real)cos((double)((real)i * 0.03129321f));
        rng.advance();
        rng.advance();
        rng.advance();
        c3 = (real)cos((double)((real)i * 0.025157286f));
        cumulative = rng.next_real() * (c3 + 1.0f + c2 + 1.0f + c1 + 1.0f + 0.25f) + cumulative + 0.25f;
    }
    for (i = 0; i < 1024; i++) {
        table[i] = (1.0f / cumulative) * table[i];
    }
}

void periodic_function_build_transition_table(transition_function_type type, uint8_t *table)
{
    int32_t i;
    real t;
    real value;
    int32_t scaled;
    const transition_curve *curve = transition_curve_for(type);

    value = 0.0f;

    for (i = 0; i < 1024; i++) {
        t = (real)i * 0.0009775171f;

        if (curve != nullptr) {
            value = curve->value(t);
        }

        scaled = static_cast<int>((double)(255.0f * value));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        table[i] = (uint8_t)scaled;
    }
}

void periodic_function_build_table(periodic_function_type type, uint8_t *out)
{
    real noise[1024];
    real wave[1024];
    real minimum, maximum;
    real range;
    real value;
    random_seed seed;
    const periodic_wave *wave_function = periodic_wave_for(type);
    int32_t i;
    int32_t scaled;

    minimum = 3.4028235e+38f;
    maximum = -3.4028235e+38f;
    periodic_function_build_noise_table(noise);

    value = -3.4028235e+38f;
    seed = globals().random_seed_global;

    for (i = 0; i < 1024; i++) {
        const wave_phase phase = {(real)i * 0.027343748f, noise[i] * 28.0f};

        if (wave_function != nullptr) {
            value = wave_function->sample(phase, seed);
        }

        if (maximum < value) {
            maximum = value;
        }
        if (value < minimum) {
            minimum = value;
        }
        wave[i] = value;
    }

    if (((int32_t)1 << static_cast<int32_t>(type)) & k_periodic_function_wrapping_mask) {
        range = 0.0f;
    } else {
        range = maximum - minimum;
    }

    for (i = 0; i < 1024; i++) {
        value = wave[i];
        if (range != 0.0f) {
            value = (value - minimum) / range;
        }
        scaled = static_cast<int>((double)(value * 255.0f));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        out[i] = (uint8_t)scaled;
    }
}

}  // namespace halo::math
