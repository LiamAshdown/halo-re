#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/effects/effects.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/render/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/api.hpp"
static auto &camera_forward_x = halo::link::ref<real_vector3d>(halo::effects::vars().camera_forward_x);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &render_frustum_global = halo::link::ref<render_frustum>(halo::render::vars().render_frustum_global);

static auto &weather_instances = halo::link::ref<weather_instance [1]>(halo::effects::vars().weather_instances);
static auto &weather_instance_count = halo::link::ref<int32_t>(halo::effects::vars().weather_instance_count);
static auto &weather_particle_data = halo::link::ref<data_array *>(halo::game::vars().weather_particle_data);
static auto &render_camera_global = halo::link::ref<real_point3d>(halo::render::vars().render_camera_global);
static auto &camera_position_z = halo::link::ref<float>(halo::effects::vars().camera_position_z);
static auto &weather_frame_counter = halo::link::ref<int32_t>(halo::effects::vars().weather_frame_counter);
static auto &weather_particle_system_count = halo::link::ref<int16_t>(halo::effects::vars().weather_particle_system_count);
static auto &weather_wind_states = halo::link::ref<weather_particle_system_state [8]>(halo::effects::vars().weather_wind_states);

namespace halo::effects {

/**
 * Activates a weather instance for the given WeatherParticleSystem tag: resets its timers to the
 * given intensity, and seeds each of its particle types with a random target particle count and
 * its fade distance, with no particles yet.
 *
 * @address 0x457e20
 */
void weather_instance_ref::activate(datum_index definition_index, real intensity)
{
    int16_t instance_index = slot;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag = (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)definition_index].data;
    int32_t i;

    instance->definition_index = definition_index;
    instance->intensity = intensity;
    instance->elapsed_time = 0.0f;
    instance->delta_time = 0.0f;
    weather_instance_count++;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + i;
        weather_instance_type *slot = &instance->types[i];

        slot->particle_count = 0;
        slot->first_particle = k_datum_index_none;

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        slot->target_count = (type->particle_count[1] - type->particle_count[0]) *
            (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale + type->particle_count[0];
        slot->field_extent = type->fade_out_end_distance;
    }
}

/**
 * Grows or shrinks one weather instance particle type slot's live particle count toward `target`
 * (see file header for how the caller computes it): creates particles (stopping early if the
 * pool is exhausted) while short, or deletes particles from the head of the list while over.
 *
 * @address 0x457fc0
 */
void weather_instance_ref::adjust_count(int16_t type_index, real target_value)
{
    int16_t instance_index = slot;
    weather_instance_type *slot = &weather_instances[instance_index].types[type_index];
    int32_t target = (int32_t)target_value;

    if (target < 0) {
        target = 0;
    }

    while (slot->particle_count < target) {
        if (halo::effects::weather_particle_new(instance_index, type_index) == k_datum_index_none) {
            break;
        }
    }

    while (target < slot->particle_count) {
        weather_particle *p =
            &((weather_particle *)weather_particle_data->data)[(uint16_t)slot->first_particle];
        datum_index next = p->next_particle;

        halo::memory::datum_delete(weather_particle_data, slot->first_particle);
        slot->particle_count -= 1;
        slot->first_particle = next;
    }
}

/**
 * Builds the sprites of one weather instance for the current view. For every particle type with live particles it
 * takes the weather polyhedra (shelters) near the camera, builds the camera facing frustum planes out to the
 * type's field extent and tiles the extent-sized particle box around the camera cell: the cells of the 3x3x3
 * block that intersect the view frustum. Each particle is placed in the first visible cell whose copy of the box
 * puts it inside the frustum, dropped when it is outside the type's fade distances or inside a shelter, and
 * submitted as a sprite faded by its distance.
 *
 * @address 0x458bf0
 */
void weather_instance_ref::build_render_geometry()
{
    int16_t instance_index = slot;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
    ScenarioStructureBSP *bsp = halo::scenario::globals().structure_bsp;
    int32_t type_index;

    halo::effects::weather_instance_update(instance_index);

    for (type_index = 0; type_index < (int32_t)tag->particle_types.count; type_index++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + type_index;
        weather_instance_type *state = &instance->types[type_index];
        float extent;
        int16_t shelter_indices[8];
        int16_t shelter_count;
        real_plane3d planes[5];
        real_vector3d camera_remainder;
        real_point3d cell_origin;
        float cell_offsets[3];
        float box_min[3];
        float box_max[3];
        real_point3d cells[27];
        float cell_plane_distance[27][5];
        int16_t cell_count;
        int32_t plane_index;
        build_sprite_data sprites;
        datum_index particle_index;

        if (state->particle_count == 0) {
            continue;
        }

        extent = state->field_extent;
        shelter_count = halo::structures::structure_weather_polyhedra_find_within_radius(shelter_indices, extent);
        halo::render::render_camera_facing_frame_build((float *)planes, extent);
        halo::math::vector3d_positive_modulo(*(real_vector3d *)&render_camera_global, camera_remainder, extent);

        cell_origin.x = render_camera_global.x - camera_remainder.i;
        cell_origin.y = render_camera_global.y - camera_remainder.j;
        cell_origin.z = render_camera_global.z - camera_remainder.k;
        for (plane_index = 0; plane_index < 5; plane_index++) {
            cell_plane_distance[0][plane_index] = cell_origin.z * planes[plane_index].normal.k +
                cell_origin.x * planes[plane_index].normal.i + cell_origin.y * planes[plane_index].normal.j;
        }
        cells[0] = cell_origin;
        cell_count = 1;

        cell_offsets[0] = -extent;
        cell_offsets[1] = 0.0f;
        cell_offsets[2] = extent;
        box_min[0] = cell_origin.x;
        box_min[1] = cell_origin.y;
        box_min[2] = cell_origin.z;
        box_max[0] = cell_origin.x + extent;
        box_max[1] = cell_origin.y + extent;
        box_max[2] = cell_origin.z + extent;

        for (int16_t x = 0; x < 3; x++) {
            for (int16_t y = 0; y < 3; y++) {
                for (int16_t z = 0; z < 3; z++) {
                    real_rectangle3d box;

                    if (x == 1 && y == 1 && z == 1) {
                        continue;
                    }
                    box.x.lower = cell_offsets[x] + box_min[0];
                    box.x.upper = box_max[0] + cell_offsets[x];
                    box.y.lower = box_min[1] + cell_offsets[y];
                    box.y.upper = box_max[1] + cell_offsets[y];
                    box.z.lower = cell_offsets[z] + box_min[2];
                    box.z.upper = cell_offsets[z] + box_max[2];
                    if (halo::render::render_frustum_test_bounding_box(&render_frustum_global, &box, 1) != 0) {
                        cells[cell_count].x = box.x.lower;
                        cells[cell_count].y = box.y.lower;
                        cells[cell_count].z = box.z.lower;
                        for (plane_index = 0; plane_index < 5; plane_index++) {
                            cell_plane_distance[cell_count][plane_index] = planes[plane_index].normal.k * cells[cell_count].z +
                                planes[plane_index].normal.i * cells[cell_count].x +
                                planes[plane_index].normal.j * cells[cell_count].y;
                        }
                        cell_count++;
                    }
                }
            }
        }

        sprites.bitmap_group_index = *(datum_index *)((uint8_t *)type + 0x1a0);
        sprites.maximum_sprite_count = state->particle_count;
        sprites.shader = (uint32_t)(uintptr_t)((uint8_t *)type + 0x1a8);
        sprites.sprite_count = 0;
        sprites.flags = 4;
        sprites.centroid = *global_zero_vector3d_pointer;
        sprites.group_count = 0;

        for (particle_index = state->first_particle; particle_index != k_datum_index_none;) {
            weather_particle *particle = &((weather_particle *)weather_particle_data->data)[particle_index & halo::k_slot_mask];
            float particle_plane_distance[5];
            int16_t cell;

            for (plane_index = 0; plane_index < 5; plane_index++) {
                particle_plane_distance[plane_index] = planes[plane_index].normal.i * particle->position.x +
                    planes[plane_index].normal.k * particle->position.z +
                    planes[plane_index].normal.j * particle->position.y - planes[plane_index].d;
            }

            for (cell = 0; cell < cell_count; cell++) {
                bool inside_frustum = true;

                for (plane_index = 0; plane_index < 5 && inside_frustum; plane_index++) {
                    inside_frustum = cell_plane_distance[cell][plane_index] + particle_plane_distance[plane_index] < 0.0f;
                }
                if (inside_frustum) {
                    break;
                }
            }

            if (cell < cell_count) {
                real_point3d position;
                float depth;
                float fade_out_limit;
                float fade_in;
                float fade_out;

                fade_out_limit = (type->fade_out_end_distance <= extent) ? type->fade_out_end_distance : extent;
                position.x = cells[cell].x + particle->position.x;
                position.y = cells[cell].y + particle->position.y;
                position.z = cells[cell].z + particle->position.z;
                depth = (position.z - render_camera_global.z) * camera_forward_x.k +
                    (position.y - render_camera_global.y) * camera_forward_x.j +
                    (position.x - render_camera_global.x) * camera_forward_x.i;

                if (depth > type->fade_in_start_distance && depth < fade_out_limit) {
                    int16_t shelter;
                    bool sheltered = false;

                    fade_in = (depth - type->fade_in_start_distance) / (type->fade_in_end_distance - type->fade_in_start_distance);
                    fade_in = (fade_in < 0.0f) ? 0.0f : ((fade_in > 1.0f) ? 1.0f : fade_in);
                    fade_out = (depth - type->fade_out_start_distance) / (fade_out_limit - type->fade_out_start_distance);
                    fade_out = (fade_out < 0.0f) ? 0.0f : ((fade_out > 1.0f) ? 1.0f : fade_out);
                    fade_out = 1.0f - fade_out;

                    for (shelter = 0; shelter < shelter_count && !sheltered; shelter++) {
                        ScenarioStructureBSPWeatherPolyhedron *polyhedron =
                            (ScenarioStructureBSPWeatherPolyhedron *)(uintptr_t)bsp->weather_polyhedra.pointer + shelter_indices[shelter];
                        ScenarioStructureBSPWeatherPolyhedronPlane *shelter_planes =
                            (ScenarioStructureBSPWeatherPolyhedronPlane *)(uintptr_t)polyhedron->planes.pointer;
                        int32_t plane_count = (int32_t)polyhedron->planes.count;
                        int16_t passed = 0;

                        while (passed < plane_count) {
                            float distance = position.z * shelter_planes[passed].plane.vector.k +
                                position.y * shelter_planes[passed].plane.vector.j +
                                position.x * shelter_planes[passed].plane.vector.i - shelter_planes[passed].plane.w;
                            if (distance < 0.0f) {
                                break;
                            }
                            passed++;
                        }
                        sheltered = passed == plane_count;
                    }

                    if (!sheltered) {
                        real_vector3d *direction = (type->render_direction_source == 1)
                            ? &particle->acceleration : &particle->velocity;
                        uint16_t mode = (uint16_t)type->render_mode;

                        if (mode != 0 &&
                            direction->k * direction->k + direction->j * direction->j + direction->i * direction->i == 0.0f) {
                            direction = halo::math::globals().global_up3d_pointer;
                        }
                        halo::render::build_sprite(&sprites, particle->sequence_index, (int16_t)(int32_t)particle->frame, (int16_t)mode,
                                     &position, direction, particle->rotation,
                                     (particle->radius + particle->radius) * type->sprite_size,
                                     (ColorARGB *)&particle->alpha, fade_out * fade_in, 0);
                    }
                }
            }

            particle_index = particle->next_particle;
        }

        halo::render::build_sprites_end(&sprites);
    }
}

/**
 * Deactivates a weather instance: deletes every live particle across all of its particle type
 * slots, then marks the instance free.
 *
 * @address 0x457f00
 */
void weather_instance_ref::deactivate()
{
    int16_t instance_index = slot;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
    int32_t i;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        weather_instance_type *slot = &instance->types[i];

        while (slot->first_particle != k_datum_index_none) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)slot->first_particle];
            datum_index next = p->next_particle;

            halo::memory::datum_delete(weather_particle_data, slot->first_particle);
            slot->particle_count -= 1;
            slot->first_particle = next;
        }
    }

    weather_instance_count--;
    instance->definition_index = k_datum_index_none;
}

/**
 * Per-tick update for one weather instance: advances its elapsed/delta time, and for each
 * particle type slot, fades its target count in/out by camera height against the type's fade
 * bounds, adjusts the live particle count toward that target, and advances every live particle's
 * frame/rotation and physics.
 *
 * @address 0x458420
 */
void weather_instance_ref::update()
{
    int16_t instance_index = slot;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
    int32_t i;

    instance->delta_time = halo::render::globals().time_since_frame;
    instance->elapsed_time = instance->delta_time + instance->elapsed_time;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + i;
        weather_instance_type *slot = &instance->types[i];
        real fade_in, fade_out;
        datum_index particle_index;

        fade_in = (camera_position_z - type->fade_in_start_height) /
                  (type->fade_in_end_height - type->fade_in_start_height);
        fade_in = (fade_in < 0.0f) ? 0.0f : (fade_in > 1.0f ? 1.0f : fade_in);

        fade_out = (camera_position_z - type->fade_out_start_height) /
                   (type->fade_out_end_height - type->fade_out_start_height);
        fade_out = (fade_out < 0.0f) ? 0.0f : (fade_out > 1.0f ? 1.0f : fade_out);

        halo::effects::weather_instance_adjust_count(instance_index, (int16_t)i,
            (1.0f - fade_out) * fade_in * instance->intensity * slot->target_count);

        particle_index = slot->first_particle;
        while (particle_index != k_datum_index_none) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)particle_index];

            p->frame = p->animation_rate * instance->delta_time + p->frame;
            p->frame = (float)halo::libm::fmod(p->frame, 1.0f);
            p->rotation = (real)((((particle_index & 1) != 0) ? -1 : 1)) * p->rotation_rate *
                instance->delta_time + p->rotation;

            halo::effects::weather_particle_update(particle_index, (int16_t)i, instance_index);

            particle_index = p->next_particle;
        }
    }
}

/**
 * Creates one new weather particle (raindrop/snowflake) for the given weather instance and
 * particle type slot: a random position inside the field box, zero velocity, a random-direction
 * random-magnitude acceleration, a random rotation/sequence/frame/colour/alpha/radius/animation
 * rate, and links it onto the slot's particle list.
 *
 * @address 0x458070
 */
datum_index weather_particle_ref::create(int16_t instance_index, int16_t type_index)
{
    datum_index handle = halo::memory::datum_new(weather_particle_data);

    if (handle != k_datum_index_none) {
        weather_instance *instance = &weather_instances[instance_index];
        weather_instance_type *slot = &instance->types[type_index];
        WeatherParticleSystem *system_tag =
            (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)system_tag->particle_types.pointer + type_index;
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[type->sprite_bitmap.tag_id.index].data;
        weather_particle *p = &((weather_particle *)weather_particle_data->data)[(uint16_t)handle];

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.x = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale * slot->field_extent;
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.y = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale * slot->field_extent;
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.z = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale * slot->field_extent;

        p->velocity.i = 0.0f;
        p->velocity.j = 0.0f;
        p->velocity.k = 0.0f;

        halo::effects::effect_random_direction_from_table((real_point3d *)&p->acceleration);
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        {
            real magnitude = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
                (type->acceleration_magnitude[1] - type->acceleration_magnitude[0]) +
                type->acceleration_magnitude[0];
            p->acceleration.i = magnitude * p->acceleration.i;
            p->acceleration.j = magnitude * p->acceleration.j;
            p->acceleration.k = magnitude * p->acceleration.k;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->radius = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
            (type->particle_radius[1] - type->particle_radius[0]) + type->particle_radius[0];
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->animation_rate = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
            (type->animation_rate[1] - type->animation_rate[0]) + type->animation_rate[0];
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->rotation_rate = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
            (type->rotation_rate[1] - type->rotation_rate[0]) + type->rotation_rate[0];

        if ((type->flags & 4) == 0) {
            p->rotation = 0.0f;
        } else {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            p->rotation = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale * 6.2831855f;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->sequence_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)bitmap->bitmap_group_sequence.count) >> 16);
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        {
            BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
            p->frame = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
                (real)(int32_t)sequences[p->sequence_index].sprites.count;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        halo::bitmaps::color_interpolate((ColorRGB *)((uint8_t *)type + 0x148), (ColorRGB *)((uint8_t *)type + 0x138),
            (ColorRGB *)&p->color, static_cast<color_interpolation_flags>(*(uint32_t *)&((struct WeatherParticleSystemParticleType *)type)->flags),
            (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale);

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->alpha = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
            (*(real *)&((struct WeatherParticleSystemParticleType *)type)->color_upper_bound - *(real *)&((struct WeatherParticleSystemParticleType *)type)->color_lower_bound) +
            *(real *)&((struct WeatherParticleSystemParticleType *)type)->color_lower_bound;

        p->next_particle = slot->first_particle;
        slot->particle_count += 1;
        slot->first_particle = handle;
    }

    return handle;
}

/**
 * Per-tick update for one weather particle: randomly re-targets its acceleration vector's
 * direction and magnitude toward the type's bounds, integrates velocity from it, runs a point
 * physics tick to move and collide the particle, and adds a tiny deterministic positional jitter
 * keyed on the particle's own handle.
 *
 * @address 0x458630
 */
void weather_particle_ref::update(int16_t type_index, int16_t instance_index)
{
    datum_index weather_particle_handle = datum;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *system_tag =
        (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
    WeatherParticleSystemParticleType *type =
        (WeatherParticleSystemParticleType *)system_tag->particle_types.pointer + type_index;
    weather_particle *p =
        &((weather_particle *)weather_particle_data->data)[(uint16_t)weather_particle_handle];

    if (type->acceleration_magnitude[0] != 0.0f || type->acceleration_magnitude[1] != 0.0f) {
        real length = halo::math::vector3d_normalize_with_length(p->acceleration);
        real old_weight = 1.0f - type->acceleration_turning_rate;
        real target_length;
        int16_t index;
        real_point3d *direction;

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        target_length = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale *
            (2.0f * type->acceleration_change_rate) - type->acceleration_change_rate + length;
        if (target_length < type->acceleration_magnitude[0]) {
            target_length = type->acceleration_magnitude[0];
        } else if (target_length > type->acceleration_magnitude[1]) {
            target_length = type->acceleration_magnitude[1];
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
        direction = &halo::math::globals().sphere_point_table[index];

        p->acceleration.i = old_weight * p->acceleration.i + direction->x * type->acceleration_turning_rate;
        p->acceleration.j = old_weight * p->acceleration.j + direction->y * type->acceleration_turning_rate;
        p->acceleration.k = old_weight * p->acceleration.k + direction->z * type->acceleration_turning_rate;

        p->acceleration.i = target_length * p->acceleration.i;
        p->acceleration.j = target_length * p->acceleration.j;
        p->acceleration.k = target_length * p->acceleration.k;

        p->velocity.i = instance->delta_time * p->acceleration.i + p->velocity.i;
        p->velocity.j = instance->delta_time * p->acceleration.j + p->velocity.j;
        p->velocity.k = instance->delta_time * p->acceleration.k + p->velocity.k;
    }

    {
        uint32_t flags_arg = (instance->in_sky != 0) ? 7u : 5u;
        PointPhysics *physics = (PointPhysics *)halo::cache::globals().tag_instances[type->physics.tag_id.index].data;
        int16_t material_type;

        halo::physics::point_physics_tick(&p->velocity, flags_arg, physics,
            (bsp_leaf_reference *)((uint8_t *)instance + 0x10), (uint32_t)instance->cluster_index,
            &p->position, nullptr, nullptr, &material_type, p->radius,
            instance->delta_time);
    }

    {
        uint32_t seed = (uint16_t)weather_particle_handle * k_random_multiplier + k_random_increment;
        int16_t index = (int16_t)(((seed >> k_random_value_shift) *
            (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
        real_point3d *direction = &halo::math::globals().sphere_point_table[index];

        p->position.x = direction->x * 0.001f + p->position.x;
        p->position.y = direction->y * 0.001f + p->position.y;
        p->position.z = direction->z * 0.001f + p->position.z;
    }

    halo::math::vector3d_positive_modulo(*(real_vector3d *)&p->position, *(real_vector3d *)&p->position, instance->types[type_index].field_extent);
}

/**
 * Per-tick driver for the global weather system's wind: for each active row of the scenario's
 * weather palette with a Wind tag, random-walks that row's magnitude/pitch/yaw perturbations and
 * rebuilds its wind direction vector from the Wind tag's base direction and variation bounds.
 *
 * @address 0x53f5c0
 */
void weather_system::update()
{
    int32_t palette_count = *(int32_t *)&halo::scenario::globals().structure_bsp->weather_palette.count;
    ScenarioStructureBSPWeatherPalette *palette =
        (ScenarioStructureBSPWeatherPalette *)halo::scenario::globals().structure_bsp->weather_palette.pointer;
    int32_t i;

    weather_frame_counter++;

    if (palette_count < 1) {
        weather_particle_system_count = (int16_t)palette_count;
        return;
    }

    for (i = 0; i < palette_count; i++) {
        ScenarioStructureBSPWeatherPalette *row = &palette[i];
        weather_particle_system_state *wind = &weather_wind_states[i];
        uint8_t *active = (uint8_t *)&weather_wind_states[0] + i * 0x20;

        if (halo::objects::tag_handle(row->wind) == halo::k_dword_none) {
            *active = 0;
        } else {
            Wind *wind_tag = (Wind *)halo::cache::globals().tag_instances[row->wind.tag_id.index].data;
            real yaw_base, pitch_base, yaw, pitch, cos_pitch;

            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            wind->magnitude_walk += (int16_t)(halo::math::globals().effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->magnitude_walk = (wind->magnitude_walk < 0.0f) ? 0.0f :
                (wind->magnitude_walk > 1.0f ? 1.0f : wind->magnitude_walk);

            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            wind->yaw_walk += (int16_t)(halo::math::globals().effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->yaw_walk = (wind->yaw_walk < -1.0f) ? -1.0f : (wind->yaw_walk > 1.0f ? 1.0f : wind->yaw_walk);

            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            wind->pitch_walk += (int16_t)(halo::math::globals().effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->pitch_walk = (wind->pitch_walk < -1.0f) ? -1.0f : (wind->pitch_walk > 1.0f ? 1.0f : wind->pitch_walk);

            wind->magnitude = (wind_tag->velocity[1] - wind_tag->velocity[0]) * wind->magnitude_walk +
                wind_tag->velocity[0];

            yaw_base = (real)halo::libm::atan2((double)row->wind_direction.j, (double)row->wind_direction.i);
            pitch_base = (real)halo::libm::atan2((double)row->wind_direction.k,
                (double)halo::libm::sqrt((double)(row->wind_direction.i * row->wind_direction.i +
                                       row->wind_direction.j * row->wind_direction.j)));

            pitch = wind->yaw_walk * wind_tag->variation_area.pitch * 0.5f + pitch_base;
            yaw = wind->pitch_walk * wind_tag->variation_area.yaw * 0.5f + yaw_base;

            cos_pitch = (real)halo::libm::cos((double)pitch);
            wind->direction_i = (real)halo::libm::cos((double)yaw) * cos_pitch;
            wind->direction_j = (real)halo::libm::sin((double)yaw) * cos_pitch;
            wind->direction_k = (real)halo::libm::sin((double)pitch);

            {
                real scale = row->wind_magnitude * wind->magnitude;
                wind->direction_i = scale * wind->direction_i;
                wind->direction_j = scale * wind->direction_j;
                wind->direction_k = scale * wind->direction_k;
            }

            *active = 1;
        }
    }

    weather_particle_system_count = (int16_t)palette_count;
}

}

namespace halo::effects {

void weather_instance_activate(datum_index definition_index, int16_t instance_index, real intensity)
{
    halo::effects::weather_instance_ref(instance_index).activate(definition_index, intensity);
}

void weather_instance_adjust_count(int16_t instance_index, int16_t type_index, real target_value)
{
    halo::effects::weather_instance_ref(instance_index).adjust_count(type_index, target_value);
}

void weather_instance_build_render_geometry(int16_t instance_index)
{
    halo::effects::weather_instance_ref(instance_index).build_render_geometry();
}

void weather_instance_deactivate(int16_t instance_index)
{
    halo::effects::weather_instance_ref(instance_index).deactivate();
}

void weather_instance_update(int16_t instance_index)
{
    halo::effects::weather_instance_ref(instance_index).update();
}

datum_index weather_particle_new(int16_t instance_index, int16_t type_index)
{
    return halo::effects::weather_particle_ref::create(instance_index, type_index);
}

void weather_particle_update(datum_index weather_particle_handle, int16_t type_index, int16_t instance_index)
{
    halo::effects::weather_particle_ref(weather_particle_handle).update(type_index, instance_index);
}

void weather_update()
{
    halo::effects::weather_system::update();
}

}
