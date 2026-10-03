#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * Ambient colour sampling for effect markers and the periodic randomisation.
 */
class ambient_color {
public:
    static void for_marker(int16_t weather_row, real_point3d *position, uint8_t flags, real_vector3d *out);
    static uint8_t marker_visible(bsp_leaf_reference *location, real_point3d *position, real_vector3d *out, uint32_t filter_flags);
    static void randomize();
    static void sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity);
};

/**
 * Material effect playback at an effect marker.
 */
class material_effects {
public:
    static void play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset);
};

}
}
