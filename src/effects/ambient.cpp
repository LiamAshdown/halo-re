#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"

extern "C" {
extern int16_t weather_particle_system_count;
extern weather_particle_system_state weather_wind_states[8];
extern ScenarioStructureBSP *global_structure_bsp;
extern const real_point3d *global_origin3d_pointer;
extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point);
extern ambient_noise_grid ambient_noise;
extern int32_t weather_frame_counter;
}

namespace halo::effects {

/**
 * Computes the ambient colour for a marker: if the marker's weather palette row has an active
 * Wind, blends an ambient_color_sample against that row's wind direction (optionally damped),
 * otherwise (or when the row is out of range / inactive) returns the default origin colour.
 *
 * @address 0x53f940
 */
void ambient_color::for_marker(int16_t weather_row, real_point3d *position, uint8_t flags, real_vector3d *out)
{
    if (weather_row >= 0 && weather_row < weather_particle_system_count &&
        *((uint8_t *)&weather_wind_states[0] + weather_row * 0x20) != 0) {
        ScenarioStructureBSPWeatherPalette *palette_row =
            (ScenarioStructureBSPWeatherPalette *)((uint8_t *)global_structure_bsp->weather_palette.pointer) +
            weather_row;
        Wind *wind_tag = (Wind *)halo::cache::globals().tag_instances[palette_row->wind.tag_id.index].data;
        weather_particle_system_state *wind = &weather_wind_states[weather_row];
        real local_variation = (flags & 1) == 0 ? wind_tag->local_variation_weight : 0.0f;
        ColorRGB sample;

        halo::effects::ambient_color_sample(&sample, position, wind_tag->local_variation_rate,
            wind_tag->local_variation_weight * wind->magnitude);

        local_variation = 1.0f - local_variation;
        out->i = local_variation * wind->direction_i + sample.red;
        out->j = local_variation * wind->direction_j + sample.green;
        out->k = local_variation * wind->direction_k + sample.blue;

        if ((flags & 2) != 0) {
            real damping = 1.0f - wind_tag->damping;
            out->i = damping * out->i;
            out->j = damping * out->j;
            out->k = damping * out->k;
        }
        return;
    }

    *out = *(const real_vector3d *)global_origin3d_pointer;
}

/**
 * Member form of the original ambient_color_marker_visible: marker visible.
 *
 * @address 0x53f860
 */
uint8_t ambient_color::marker_visible(bsp_leaf_reference *location, real_point3d *position, real_vector3d *out, uint32_t filter_flags)
{
    uint8_t in_water = 0;
    int16_t weather_row = -1;
    int16_t cluster = ((struct bsp_leaf_reference *)location)->cluster_index;

    if (cluster != -1) {
        uint32_t skip_non_water = filter_flags & 4;
        int16_t region = scenario_location_fog_region(location, skip_non_water ? (real_point3d *)0 : position);

        weather_row = *(int16_t *)((uint8_t *)global_structure_bsp->clusters.pointer + cluster * 0x68 + 8);
        if (region != -1) {
            uint8_t *fog_region = (uint8_t *)global_structure_bsp->fog_regions.pointer + region * 0x28;
            int16_t fog = *(int16_t *)(fog_region + 0x24);
            int16_t region_weather = *(int16_t *)(fog_region + 0x26);

            if (fog != -1 && region_weather != -1) {
                datum_index fog_tag = *(datum_index *)((uint8_t *)global_structure_bsp->fog_palette.pointer + fog * 0x88 + 0x2c);

                if (fog_tag != k_datum_index_none) {
                    uint8_t *fog_data = (uint8_t *)halo::cache::globals().tag_instances[fog_tag & 0xffff].data;

                    if (fog_data[0] & 1) {
                        if ((filter_flags & 8) == 0) {
                            in_water = 1;
                            weather_row = region_weather;
                        }
                    } else if (!skip_non_water) {
                        weather_row = region_weather;
                    }
                }
            }
        }
    }
    halo::effects::ambient_color_for_marker(weather_row, position, (uint8_t)filter_flags, out);
    return in_water;
}

/**
 * Rolls a fresh sphere_point_table direction into column 0 of every row of every band (row by row, the three
 * bands of a row in turn), then fills columns 1..7 of each row with the Catmull-Rom curve through the column 0
 * points of rows i-1, i, i+1 and i+2 (wrapping at 8), sampled at i + column / 8.
 *
 * @address 0x53fa70
 */
void ambient_color::randomize()
{
    int32_t row, band, column;

    for (row = 0; row < k_ambient_noise_rows; row++) {
        for (band = 0; band < k_ambient_noise_bands; band++) {
            int16_t index;

            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * k_random_multiplier + k_random_increment;
            index = (int16_t)(((halo::math::globals().random_seed_global >> 16) * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
            ambient_noise.entries[band][row][0] = *(real_vector3d *)&halo::math::globals().sphere_point_table[index];
        }
    }

    for (row = 0; row < k_ambient_noise_rows; row++) {
        int32_t previous = (row - 1) & 7;
        int32_t next = (row + 1) & 7;
        int32_t after_next = (row + 2) & 7;

        for (column = 1; column < k_ambient_noise_columns; column++) {
            float time = (float)column * 0.125f + (float)row;

            for (band = 0; band < k_ambient_noise_bands; band++) {
                halo::camera::vector3d_catmull_rom_interpolate((Vector3D *)(&ambient_noise.entries[band][row][0]),
                                                 (Vector3D *)(&ambient_noise.entries[band][after_next][0]),
                                                 (Vector3D *)(&ambient_noise.entries[band][next][0]),
                                                 (Vector3D *)(&ambient_noise.entries[band][row][column]),
                                                 (Vector3D *)(&ambient_noise.entries[band][previous][0]),
                                                 (float)(row - 1), 1.0f, time);
            }
        }
    }
}

/**
 * REWRITTEN 2026-09-27 (static loop) from objdump 0x53fc80..0x53fd5a. For each of the three bands i:
 * hashed = position[i] + weather_frame_counter * w[i] * hash_scale   (w = 0.1, 0.2, 0.07 -- TIME factors)
 * column = low 6 bits of (float)(|hashed * 8| + 2^23)                (the 2^23 trick: round to nearest)
 * out   += noise_grid[band i][column]                                 (unweighted)
 * then out *= intensity / 3. The draft used the weights as accumulation weights, mixed in the NEXT position
 * component and truncated the column.
 *
 * @address 0x53fc80
 */
void ambient_color::sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity)
{
    static const real k_band_time_scale[3] = {0.1f, 0.2f, 0.07f};
    const real *pos = (const real *)position;
    const real_vector3d *grid = &ambient_noise.entries[0][0][0];
    real scale = intensity * 0.33333334f;
    int band;

    out->red = global_origin3d_pointer->x;
    out->green = global_origin3d_pointer->y;
    out->blue = global_origin3d_pointer->z;

    for (band = 0; band < 3; band++) {
        real hashed = ((real)weather_frame_counter * k_band_time_scale[band] * hash_scale + pos[band]) * 8.0f;
        real rounded;
        uint32_t bits;
        int32_t index;

        hashed = hashed < 0.0f ? -hashed : hashed;
        rounded = hashed + 8388608.0f;
        bits = *(uint32_t *)&rounded;
        index = band * 0x40 + (int32_t)(bits & 0x3f);
        out->red += grid[index].i;
        out->green += grid[index].j;
        out->blue += grid[index].k;
    }

    out->red = scale * out->red;
    out->green = scale * out->green;
    out->blue = scale * out->blue;
}

/**
 * Member form of the original material_effects_play_at_marker: play at marker.
 *
 * @address 0x453490
 */
void material_effects::play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset)
{
    MaterialEffects *definition = (MaterialEffects *)halo::cache::globals().tag_instances[material_effects_tag & 0xffff].data;

    if (material_type < (int32_t)definition->effects.count) {
        MaterialEffectsMaterialEffect *material =
            &((MaterialEffectsMaterialEffect *)definition->effects.pointer)[material_type];

        if (sub_effect_index != -1 && sub_effect_index < (int32_t)material->materials.count) {
            MaterialEffectsMaterialEffectMaterial *entry =
                &((MaterialEffectsMaterialEffectMaterial *)
                      material->materials.pointer)[sub_effect_index];
            real_point3d spawn_position;

            spawn_position.x = offset->i * 0.01f + position->x;
            spawn_position.y = offset->j * 0.01f + position->y;
            spawn_position.z = offset->k * 0.01f + position->z;

            if (*(uint32_t *)&entry->effect.tag_id != 0xffffffffu) {
                halo::effects::effect_new_with_color(*(uint32_t *)&entry->effect.tag_id, 0xffffffff, (const real_vector3d *)0, 1, 0,
                    &spawn_position, (uint32_t)offset, *(real *)&sound_param, 0.0f, (const ColorRGB *)0,
                    (const effect_tint_source *)0, 0);
            }

            if (*(uint32_t *)&entry->sound.tag_id != 0xffffffffu) {
                struct {
                    real_point3d position;
                    real_vector3d normal;
                    real_point3d reference;
                    uint32_t bundle_word0;
                    uint32_t bundle_word1;
                } sound_args;

                sound_args.position = spawn_position;
                sound_args.normal = *offset;
                sound_args.reference = *global_origin3d_pointer;
                sound_args.bundle_word0 = location_bundle[0];
                sound_args.bundle_word1 = location_bundle[1];

                halo::sound::sound_start_at_location(*(datum_index *)&entry->sound.tag_id, (sound_placement *)&sound_args,
                    *(float *)&sound_param);
            }
        }
    }
}

}

namespace halo::effects {

void ambient_color_for_marker(int16_t weather_row, real_point3d *position, uint8_t flags, real_vector3d *out)
{
    halo::effects::ambient_color::for_marker(weather_row, position, flags, out);
}

uint8_t ambient_color_marker_visible(bsp_leaf_reference *location, real_point3d *position, real_vector3d *out, uint32_t filter_flags)
{
    return halo::effects::ambient_color::marker_visible(location, position, out, filter_flags);
}

void ambient_color_randomize()
{
    halo::effects::ambient_color::randomize();
}

void ambient_color_sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity)
{
    halo::effects::ambient_color::sample(out, position, hash_scale, intensity);
}

void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset)
{
    halo::effects::material_effects::play_at_marker(material_effects_tag, material_type, sub_effect_index, location_bundle, sound_param, position, offset);
}

}
