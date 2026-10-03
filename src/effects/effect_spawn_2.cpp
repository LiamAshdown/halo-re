#include "halo/core/lcg.hpp"
#include "halo/core/datum.hpp"
#include "halo/effects/effects.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/effects/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/ai/api.hpp"

static auto &particle_spawn_debug_mode = halo::link::ref<uint8_t>(halo::effects::vars().particle_spawn_debug_mode);
static auto &first_person_weapon_interfaces = halo::link::ref<uint8_t *>(halo::ui::vars().first_person_weapon_interfaces);
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);

namespace halo::effects {

/**
 * Rotates a point by a marker's real_matrix4x3 transform (scale, then forward/left/up, then
 * translate) -- the same formula as src/math/matrix4x3_transform_point.c, duplicated inline
 * because the disassembly never calls it directly at either of the two sites that need it.
 */
static void effect_spawn_particles_transform_point(real_point3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    if (m->scale != 1.0f) {
        x *= m->scale;
        y *= m->scale;
        z *= m->scale;
    }
    out->x = x * m->forward.i + y * m->left.i + z * m->up.i + m->position.x;
    out->y = x * m->forward.j + y * m->left.j + z * m->up.j + m->position.y;
    out->z = x * m->forward.k + y * m->left.k + z * m->up.k + m->position.z;
}

/**
 * Rotates a direction by a marker's real_matrix4x3 (scale, then forward/left/up, no translate) --
 * the same formula as src/math/matrix4x3_transform_normal.c, duplicated inline for the same
 * reason as above.
 */
static void effect_spawn_particles_transform_normal(real_vector3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    if (m->scale != 1.0f) {
        x *= m->scale;
        y *= m->scale;
        z *= m->scale;
    }
    out->i = x * m->forward.i + y * m->left.i + z * m->up.i;
    out->j = x * m->forward.j + y * m->left.j + z * m->up.j;
    out->k = x * m->forward.k + y * m->left.k + z * m->up.k;
}

/**
 * The same rotation WITHOUT the matrix scale. The original applies `scale` to the velocity
 * vector before rotating it through both the marker and the node transform, but never to the
 * direction vector -- at either site. Kept as its own helper so the asymmetry is visible.
 */
static void effect_spawn_particles_rotate_unscaled(real_vector3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    out->i = x * m->forward.i + y * m->left.i + z * m->up.i;
    out->j = x * m->forward.j + y * m->left.j + z * m->up.j;
    out->k = x * m->forward.k + y * m->left.k + z * m->up.k;
}

/**
 * For the current event's EffectParticle list, rolls how many of each type should spawn this
 * tick (the difference of the distribution function evaluated at this tick's and last tick's
 * event fraction, scaled by the type's already-rolled particle count), then for every qualifying
 * marker of that type's EffectLocation spawns that many individual particles: each one's
 * position/orientation is built by transforming a randomized offset and two randomized
 * direction/velocity-like vectors through the marker's transform (and, if the marker names a
 * real node or first-person-weapon marker, through that node's transform too), its colour is
 * interpolated from the type's tint bounds, and the result is hCommitted through particle_new.
 *
 * @address 0x451f90
 */
void effect_view::spawn_particles()
{
    effect * self = record;
    Effect *tag;
    EffectEvent *event;
    real previous_fraction;
    real current_fraction;
    int16_t type_index;

    if (particle_spawn_debug_mode == 0) {
        return;
    }
    tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    event = &((EffectEvent *)tag->events.pointer)[self->event_index];
    previous_fraction = self->previous_event_fraction;
    current_fraction = (self->event_duration > 0.0f) ? self->event_time / self->event_duration : 1.0f;

    for (type_index = 0; (int32_t)type_index < (int32_t)event->particles.count; type_index++) {
        uint8_t *pt = (uint8_t *)event->particles.pointer + (int32_t)type_index * 0xe8;
        int16_t location = *(int16_t *)(pt + 0x08);
        int16_t violence_mode = *(int16_t *)(pt + 0x02);
        uint16_t create = *(uint16_t *)(pt + 0x04);
        real count_scale;
        int16_t current_count;
        int16_t spawn_count;
        datum_index marker_handle;
        effect_location_marker *entry;

        if (location < 0 || (int32_t)location >= (int32_t)tag->locations.count) {
            continue;
        }
        if ((((uint8_t *)self)[2] >> 6 & 1) != 0 ? violence_mode == 1 : violence_mode == 2) {
            continue;
        }
        count_scale = (real)(int32_t)self->particle_counts[type_index];
        current_count = (int16_t)(int32_t)(halo::effects::effect_distribution_function_evaluate(
            (EffectDistributionFunction_t)*(uint16_t *)(pt + 0x68), current_fraction) * count_scale);
        spawn_count = (int16_t)((uint16_t)current_count - (int32_t)(halo::effects::effect_distribution_function_evaluate(
            (EffectDistributionFunction_t)*(uint16_t *)(pt + 0x68), previous_fraction) * count_scale));
        if (particle_spawn_debug_mode == 1) {
            spawn_count = (int16_t)(int32_t)((real)(int32_t)spawn_count * 0.5f);
        }
        if (spawn_count <= 0) {
            continue;
        }

        marker_handle = self->location_markers[location];
        for (entry = halo::effects::effect_marker_next(self, &marker_handle, create); entry != 0;
             entry = halo::effects::effect_marker_next(self, &marker_handle, create)) {
            uint16_t remaining;

            if (entry->marker_index != halo::k_word_none && (entry->marker_index & 0x8000) != 0 &&
                *(int32_t *)(first_person_weapon_interfaces + self->first_person_weapon_index * 0x1ea0 + 8) == -1) {
                continue;
            }
            remaining = (uint16_t)spawn_count;
            do {
                uint32_t a_bits = *(uint32_t *)(pt + 0xe0);
                uint32_t b_bits = *(uint32_t *)(pt + 0xe4);
                real radius0 = *(real *)(pt + 0x70);
                real base_radius = radius0;
                real radius_span;
                real radius;
                uint32_t radius_word;
                int16_t sample_index;
                real_point3d sample;
                real_matrix4x3 *m = &entry->transform;
                particle_creation_data record;
                real_point3d position;
                real_vector3d direction;
                real_vector3d velocity;
                uint8_t create_ok;
                real frac;
                uint32_t flags;

                if ((a_bits & 0x80) != 0) {
                    base_radius *= self->a_scale;
                }
                if ((b_bits & 0x80) != 0) {
                    base_radius *= self->b_scale;
                }
                radius_span = *(real *)(pt + 0x74) - radius0;
                if ((a_bits & 0x100) != 0) {
                    radius_span *= self->a_scale;
                }
                if ((b_bits & 0x100) != 0) {
                    radius_span *= self->b_scale;
                }
                halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                radius_word = halo::math::globals().effect_random_seed;
                halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                sample_index = (int16_t)(((halo::math::globals().effect_random_seed >> 16) * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
                sample = halo::math::globals().sphere_point_table[sample_index];
                radius = (real)(int32_t)(radius_word >> 16) * halo::k_unit_word_scale * radius_span + base_radius;

                effect_spawn_particles_transform_point(&record.position, *(real *)(pt + 0x14),
                    *(real *)(pt + 0x18), *(real *)(pt + 0x1c), m);
                record.position.x += sample.x * radius;
                record.position.y += sample.y * radius;
                record.position.z += sample.z * radius;
                {
                    real_vector3d raw_direction, raw_velocity;

                    halo::effects::effect_random_velocity_vector(self, &halo::math::globals().effect_random_seed, (real_vector3d *)(pt + 0x20),
                        &raw_direction, &raw_velocity, *(real *)(pt + 0x84), *(real *)(pt + 0x88),
                        *(real *)(pt + 0x8c), a_bits, (uint8_t)b_bits);
                    effect_spawn_particles_rotate_unscaled((real_vector3d *)&record.direction, raw_direction.i, raw_direction.j,
                        raw_direction.k, m);
                    effect_spawn_particles_transform_normal(&record.velocity, raw_velocity.i, raw_velocity.j,
                        raw_velocity.k, m);
                }

                if (entry->marker_index != halo::k_word_none) {
                    real_matrix4x3 *node;
                    int16_t node_index = (int16_t)(entry->marker_index & 0x7fff);

                    if ((entry->marker_index & 0x8000) != 0) {
                        node = (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
                            self->first_person_weapon_index * 0x1ea0 + node_index * 0x34);
                    } else {
                        uint8_t *owner = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(uint16_t)self->object_index].data;

                        node = (real_matrix4x3 *)(owner + ((object *)owner)->nodes.offset + node_index * 0x34);
                    }
                    effect_spawn_particles_transform_point(&position, record.position.x, record.position.y,
                        record.position.z, node);
                    effect_spawn_particles_rotate_unscaled(&direction, record.direction.x, record.direction.y,
                        record.direction.z, node);
                    effect_spawn_particles_transform_normal(&velocity, record.velocity.i, record.velocity.j,
                        record.velocity.k, node);
                } else {
                    position = record.position;
                    direction = *(real_vector3d *)&record.direction;
                    velocity = record.velocity;
                }

                switch (*(int16_t *)(pt + 0x00)) {
                case 0:
                    create_ok = 1;
                    break;
                case 1:
                    create_ok = !halo::scenario::scenario_location_get_water_and_weather(&position, &self->location, 0);
                    break;
                case 2:
                    create_ok = halo::scenario::scenario_location_get_water_and_weather(&position, &self->location, 0);
                    break;
                default:
                    create_ok = 0;
                    break;
                }
                if (!create_ok) {
                    continue;
                }

                record.definition_index = *(datum_index *)(pt + 0x60);
                flags = *(uint32_t *)(pt + 0x64);
                if ((flags & 1) != 0) {
                    record.object_index = self->object_index;
                    record.marker_index = (entry->marker_index == halo::k_word_none) ? -1 : (int16_t)(entry->marker_index & 0x7fff);
                    record.gravity = *(real_vector3d *)global_origin3d_pointer;
                } else {
                    if (self->tint_source.proc != 0) {
                        void (*tint_proc)(real_vector3d *, real_point3d *, void *) =
                            (void (*)(real_vector3d *, real_point3d *, void *))(uintptr_t)self->tint_source.proc;
                        tint_proc(&record.gravity, &position, (void *)(uintptr_t)self->tint_source.data);
                    } else {
                        record.gravity = *(real_vector3d *)global_origin3d_pointer;
                    }
                    record.position = position;
                    *(real_vector3d *)&record.direction = direction;
                    record.object_index = halo::k_dword_none;
                    record.marker_index = -1;
                    record.velocity.i = self->velocity.i * 30.0f + velocity.i;
                    record.velocity.j = self->velocity.j * 30.0f + velocity.j;
                    record.velocity.k = self->velocity.k * 30.0f + velocity.k;
                }
                record.scale = halo::effects::effect_property_random_value(9, self, a_bits, b_bits, &halo::math::globals().effect_random_seed,
                    *(real *)(pt + 0xa0), *(real *)(pt + 0xa4));
                record.angular_velocity = halo::effects::effect_property_random_value(3, self, *(uint32_t *)(pt + 0xe0),
                    *(uint32_t *)(pt + 0xe4), &halo::math::globals().effect_random_seed, *(real *)(pt + 0x90), *(real *)(pt + 0x94));
                if ((pt[0x64] & 2) != 0) {
                    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                    record.rotation = (real)(int32_t)(halo::math::globals().effect_random_seed >> 16) * halo::k_unit_word_scale * 6.2831855f;
                } else {
                    record.rotation = 0.0f;
                }
                if ((*(uint32_t *)(pt + 0xe0) & 0x800) == 0 && (*(uint32_t *)(pt + 0xe4) & 0x800) == 0) {
                    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
                    frac = (real)(int32_t)(halo::math::globals().effect_random_seed >> 16) * halo::k_unit_word_scale;
                } else {
                    frac = ((*(uint32_t *)(pt + 0xe0) & 0x800) != 0) ? self->a_scale : 1.0f;
                    if ((*(uint32_t *)(pt + 0xe4) & 0x800) != 0) {
                        frac *= self->b_scale;
                    }
                }
                flags = *(uint32_t *)(pt + 0x64);
                halo::bitmaps::color_interpolate((ColorRGB *)(pt + 0xc4), (ColorRGB *)(pt + 0xb4),
                    (ColorRGB *)&record.color.red, static_cast<color_interpolation_flags>((flags >> 3) & 3), frac);
                record.color.alpha = (1.0f - frac) * *(real *)(pt + 0xb0) + frac * *(real *)(pt + 0xc0);
                if ((flags & 4) != 0) {
                    record.color.red *= self->color.red;
                    record.color.green *= self->color.green;
                    record.color.blue *= self->color.blue;
                }
                *(int16_t *)&record.first_person_weapon_index = self->first_person_weapon_index;
                record.first_person = (entry->marker_index != halo::k_word_none && (entry->marker_index & 0x8000) != 0);
                record.third_person_only = (create == 2);
                record.first_person_only = (create == 1);
                halo::effects::particle_new(&record);
            } while (--remaining != 0);
        }
    }
    self->previous_event_fraction = current_fraction;
}

}

namespace halo::effects {

void effect_spawn_particles(effect *self)
{
    halo::effects::effect_view(self).spawn_particles();
}

}
