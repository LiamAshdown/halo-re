/**
 * @file src/sound/gain.cpp
 * Gain, millibel and attenuation conversions.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"

namespace halo::sound {

namespace {

/** Truncates a double to a 64-bit integer and keeps the low 32 bits, as the original __ftol did; out-of-range and NaN values give 0. */
int32_t ftol_low_dword(double value)
{
    if (value != value || value >= 9.2233720368547758e18 || value < -9.2233720368547758e18) return 0;
    return (int32_t)(long long)value;
}

}  // namespace

namespace gain {

int32_t linear_to_attenuation(float gain, int32_t maximum)
{
    if (gain != 0.0f) {
        int32_t result = ftol_low_dword(log10((double)gain) * 2000.0 + (double)maximum);

        if (k_sound_minimum_volume <= result) {
            if (maximum < result) {
                result = maximum;
            }
            return result;
        }
    }

    return k_sound_minimum_volume;
}

int32_t linear_to_millibels_clamped(int32_t minimum, float gain, int32_t bias_and_maximum)
{
    int32_t result;

    if (gain != 0.0f) {
        result = ftol_low_dword(log10((double)gain) * 2000.0 + (float)bias_and_maximum);
        if (minimum <= result) {
            if (bias_and_maximum < result) {
                result = bias_and_maximum;
            }
            return result;
        }
    }
    return minimum;
}

float evaluate_volume_curve(int32_t attenuation, int32_t minimum, int32_t maximum)
{
    float value;

    if (attenuation == k_sound_minimum_volume) {
        return (float)minimum;
    }

    value = (float)pow(10.0, (double)attenuation * 0.0005);
    if ((float)minimum <= value && value <= (float)maximum) {
        return value;
    }
    if (value < (float)minimum) {
        return (float)minimum;
    }
    return (float)maximum;
}

float clamp_by_ratio(float gain, float compare, float ratio)
{
    float scaled;

    if (ratio != 0.0f && gain != compare) {
        if (compare < gain) {
            scaled = compare * ratio;
            if (scaled < gain) {
                return scaled;
            }
            return gain;
        }
        scaled = compare / ratio;
        if (gain <= scaled) {
            return scaled;
        }
    }
    return gain;
}

int32_t to_directsound_volume(float gain, int32_t maximum)
{
    int32_t volume;

    if (gain != 0.0f) {
        volume = ftol_low_dword(log10((double)gain) * 2000.0 + (double)maximum);
        if (volume > k_sound_minimum_volume - 1) {
            if (maximum < volume) {
                volume = maximum;
            }
            return volume;
        }
    }
    return k_sound_minimum_volume;
}

int to_millibels(float gain)
{
    int32_t millibels;

    if (1.0f - gain != 0.0f) {
        millibels = ftol_low_dword(log10((double)(1.0f - gain)) * 2000.0);
        if (millibels > k_sound_minimum_volume - 1) {
            if (millibels > 0) {
                millibels = 0;
            }
            return millibels;
        }
    }
    return k_sound_minimum_volume;
}

float reverb_size_scale(float value)
{
    if (value <= 20.0f) {
        return 1000.0f;
    }
    if (value >= 20000.0f) {
        return 20000.0f;
    }
    if (value == 5000.0f) {
        return 5000.0f;
    }
    return value * 5.005005e-05f * 19000.0f;
}

}  // namespace gain


}  // namespace halo::sound
