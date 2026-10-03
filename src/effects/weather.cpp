#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern weather_instance weather_instances[1];
extern int32_t weather_instance_count;
extern data_array *weather_particle_data;
extern datum_index weather_particle_new(int16_t instance_index, int16_t type_index);
extern ScenarioStructureBSP *global_structure_bsp;
extern float render_camera_global;
extern float camera_position_y;
extern float camera_position_z;
extern const uint32_t k_particle_render_constant[3];
extern void weather_instance_update(int16_t instance_index);
extern void render_camera_facing_frame_build(real reference);
extern uint8_t *structure_weather_polyhedra_find_within_radius(real radius);
extern int16_t render_frustum_test_bounding_box(uint32_t mode);
extern void build_sprite();
extern void build_sprites_end(void);
extern float render_time_since_frame;
extern void weather_instance_adjust_count(int16_t instance_index, int16_t type_index, real target_value);
extern void weather_particle_update(datum_index weather_particle_handle, int16_t type_index, int16_t instance_index);
extern double fmod(double x, double y);
extern void effect_random_direction_from_table(real_point3d *out);
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt);
extern int32_t weather_frame_counter;
extern int16_t weather_particle_system_count;
extern weather_particle_system_state weather_wind_states[8];
extern double atan2(double y, double x);
extern double sqrt(double x);
extern double cos(double x);
extern double sin(double x);
void weather_instance_activate(datum_index definition_index, int16_t instance_index, real intensity);
void weather_instance_build_render_geometry(int16_t instance_index);
void weather_instance_deactivate(int16_t instance_index);
void weather_update();
}

/**
 * Calls halo::math::vector3d_positive_modulo with the single float argument the weather code was reversed with;
 * the function takes (vector, out, period), so the call reads whatever the caller left behind.
 * Unresolved: the weather code still has to be reversed to name the real arguments.
 */
static void vector3d_positive_modulo_unresolved(real reference)
{
    using call_t = void (*)(real);
    reinterpret_cast<call_t>(&halo::math::vector3d_positive_modulo)(reference);
}

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
        slot->first_particle = (datum_index)0xffffffff;

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        slot->target_count = (type->particle_count[1] - type->particle_count[0]) *
            (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f + type->particle_count[0];
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
        if (weather_particle_new(instance_index, type_index) == (datum_index)0xffffffff) {
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
 * See file header: a very low confidence, offset-for-offset transliteration of the rasterizer
 * facing weather render geometry builder. Behaviour is not guaranteed to be preserved past the
 * outer per-particle-type loop structure and the update/skip calls each type makes.
 *
 * @address 0x458bf0
 */
void weather_instance_ref::build_render_geometry()
{
    int16_t instance_index = slot;
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
    int32_t type_index;

    weather_instance_update(instance_index);

    for (type_index = 0; type_index < (int32_t)tag->particle_types.count; type_index++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + type_index;
        weather_instance_type *slot = &instance->types[type_index];

        if (slot->particle_count != 0) {
            uint8_t *regions = structure_weather_polyhedra_find_within_radius(slot->field_extent);
            (void)regions;
            render_camera_facing_frame_build(slot->field_extent);
            vector3d_positive_modulo_unresolved(slot->field_extent);

            build_sprites_end();
        }
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

        while (slot->first_particle != (datum_index)0xffffffff) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)slot->first_particle];
            datum_index next = p->next_particle;

            halo::memory::datum_delete(weather_particle_data, slot->first_particle);
            slot->particle_count -= 1;
            slot->first_particle = next;
        }
    }

    weather_instance_count--;
    instance->definition_index = (datum_index)0xffffffff;
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

    instance->delta_time = render_time_since_frame;
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

        weather_instance_adjust_count(instance_index, (int16_t)i,
            (1.0f - fade_out) * fade_in * instance->intensity * slot->target_count);

        particle_index = slot->first_particle;
        while (particle_index != (datum_index)0xffffffff) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)particle_index];

            p->frame = p->animation_rate * instance->delta_time + p->frame;
            p->frame = (float)fmod(p->frame, 1.0f);
            p->rotation = (real)((((particle_index & 1) != 0) ? -1 : 1)) * p->rotation_rate *
                instance->delta_time + p->rotation;

            weather_particle_update(particle_index, (int16_t)i, instance_index);

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

    if (handle != (datum_index)0xffffffff) {
        weather_instance *instance = &weather_instances[instance_index];
        weather_instance_type *slot = &instance->types[type_index];
        WeatherParticleSystem *system_tag =
            (WeatherParticleSystem *)halo::cache::globals().tag_instances[(uint16_t)instance->definition_index].data;
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)system_tag->particle_types.pointer + type_index;
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[type->sprite_bitmap.tag_id.index].data;
        weather_particle *p = &((weather_particle *)weather_particle_data->data)[(uint16_t)handle];

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.x = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.y = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.z = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;

        p->velocity.i = 0.0f;
        p->velocity.j = 0.0f;
        p->velocity.k = 0.0f;

        effect_random_direction_from_table((real_point3d *)&p->acceleration);
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        {
            real magnitude = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (type->acceleration_magnitude[1] - type->acceleration_magnitude[0]) +
                type->acceleration_magnitude[0];
            p->acceleration.i = magnitude * p->acceleration.i;
            p->acceleration.j = magnitude * p->acceleration.j;
            p->acceleration.k = magnitude * p->acceleration.k;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->radius = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->particle_radius[1] - type->particle_radius[0]) + type->particle_radius[0];
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->animation_rate = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->animation_rate[1] - type->animation_rate[0]) + type->animation_rate[0];
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->rotation_rate = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->rotation_rate[1] - type->rotation_rate[0]) + type->rotation_rate[0];

        if ((type->flags & 4) == 0) {
            p->rotation = 0.0f;
        } else {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            p->rotation = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->sequence_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)bitmap->bitmap_group_sequence.count) >> 16);
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        {
            BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
            p->frame = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (real)(int32_t)sequences[p->sequence_index].sprites.count;
        }

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        color_interpolate((ColorRGB *)((uint8_t *)type + 0x148), (ColorRGB *)((uint8_t *)type + 0x138),
            (ColorRGB *)&p->color, *(uint32_t *)&((struct WeatherParticleSystemParticleType *)type)->flags,
            (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f);

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        p->alpha = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
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
        target_length = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
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

        point_physics_tick(&p->velocity, flags_arg, physics,
            (bsp_leaf_reference *)((uint8_t *)instance + 0x10), (uint32_t)instance->cluster_index,
            &p->position, (real_vector3d *)0, (real_vector3d *)0, &material_type, p->radius,
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

    vector3d_positive_modulo_unresolved(instance->types[type_index].field_extent);
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
    int32_t palette_count = *(int32_t *)&global_structure_bsp->weather_palette.count;
    ScenarioStructureBSPWeatherPalette *palette =
        (ScenarioStructureBSPWeatherPalette *)global_structure_bsp->weather_palette.pointer;
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

        if (*(uint32_t *)&row->wind.tag_id == 0xffffffffu) {
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

            yaw_base = (real)atan2((double)row->wind_direction.j, (double)row->wind_direction.i);
            pitch_base = (real)atan2((double)row->wind_direction.k,
                (double)sqrt((double)(row->wind_direction.i * row->wind_direction.i +
                                       row->wind_direction.j * row->wind_direction.j)));

            pitch = wind->yaw_walk * wind_tag->variation_area.pitch * 0.5f + pitch_base;
            yaw = wind->pitch_walk * wind_tag->variation_area.yaw * 0.5f + yaw_base;

            cos_pitch = (real)cos((double)pitch);
            wind->direction_i = (real)cos((double)yaw) * cos_pitch;
            wind->direction_j = (real)sin((double)yaw) * cos_pitch;
            wind->direction_k = (real)sin((double)pitch);

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

extern "C" {

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
