#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/sound/api.hpp"

extern "C" {
extern data_array *particle_data;
extern uint8_t particle_advance_frame(datum_index particle_handle);
extern uint8_t particle_next_sequence(datum_index particle_handle);
extern void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index, real intensity);
extern datum_index effect_new_with_color(uint32_t definition_index, uint32_t creator, real_vector3d *velocity, int32_t count, char **names, real_point3d *points, real_vector3d *vectors, float a_scale, float b_scale, int32_t color, int32_t tint, int32_t force);
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
extern const real_vector3d *global_down3d_pointer;
extern char *particle_impact_vector_names[2];
extern data_array *object_data;
extern ScenarioStructureBSP *global_structure_bsp;
extern player_globals *local_player_globals;
extern uint8_t *first_person_weapon_interfaces;
extern int32_t render_frame_index;
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern uint16_t effect_random_uint16(void);
extern int effect_random_int_between(int16_t minimum, int16_t maximum);
extern real particle_current_radius(datum_index particle_handle);
extern void object_sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color, real_vector3d *base_map_color, uint8_t wait_for_textures);
extern void particle_impact(datum_index particle_handle);
extern double sqrt(double x);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern uint8_t any_local_player_within_10_units(real_point3d *position);
extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset);
extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt);
extern uint8_t particle_advance_animation(datum_index particle_handle, real delta_time);
extern uint8_t particle_update_motion(datum_index particle_handle, real delta_time);
void particle_new(particle_creation_data *creation_data);
void particles_delete_by_first_person_weapon(uint8_t first_person_weapon_index);
void particles_refresh_structure_locations();
void particles_update(real delta_time);
}

namespace halo::effects {

/**
 * Consumes `delta_time` seconds of a particle's animation clock, calling particle_advance_frame
 * once per whole animation period until the remaining time runs out. A tag flagged
 * animate_once_per_frame instead advances exactly one frame per nonzero call, and a tag flagged
 * animation_stops_at_rest with the particle currently at rest does not animate at all.
 *
 * @address 0x4560c0
 */
uint8_t particle_ref::advance_animation(real delta_time)
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    uint8_t has_frame = 1;

    if ((tag->flags & 0x2) != 0   &&
        (self->flags & _particle_at_rest_bit) != 0) {
        return has_frame;
    }

    if ((tag->flags & 0x8) != 0  ) {
        if (delta_time != 0.0f) {
            return halo::effects::particle_advance_frame(particle_handle);
        }
        return has_frame;
    }

    if (self->animation_timer == -1.0f) {
        has_frame = halo::effects::particle_advance_frame(particle_handle);
        self->animation_timer = 0.0f;
    }

    if (delta_time > 0.0f) {
        while (has_frame != 0) {
            real remaining = self->inverse_animation_period - self->animation_timer;

            if (remaining > delta_time) {
                self->animation_timer += delta_time;
                break;
            }

            has_frame = halo::effects::particle_advance_frame(particle_handle);
            delta_time -= remaining;
            if (delta_time <= 0.0f) {
                return has_frame;
            }
        }
    }

    return has_frame;
}

/**
 * Steps a particle's frame index by one in the direction its flags request, resetting the per
 * frame animation timer either way. When the walk runs off the end (or start) of the current
 * sequence's sprite list it rolls the next sequence instead.
 *
 * @address 0x456000
 */
uint8_t particle_ref::advance_frame()
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[tag->bitmap.tag_id.index].data;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;

    self->animation_timer = 0.0f;

    if ((self->flags & _particle_animating_backwards_bit) == 0) {
        int32_t sprite_count = sequences[self->sequence_index].sprites.count;

        if (self->frame_index + 1 < sprite_count) {
            self->frame_index += 1;
            return 1;
        }
        {
            uint8_t has_frame = halo::effects::particle_next_sequence(particle_handle);
            self->frame_index = 0;
            return has_frame;
        }
    } else {
        if (self->frame_index > 0) {
            self->frame_index -= 1;
            return 1;
        }
        {
            uint8_t has_frame = halo::effects::particle_next_sequence(particle_handle);
            if (has_frame != 0) {
                self->frame_index = (int16_t)(sequences[self->sequence_index].sprites.count - 1);
                return has_frame;
            }
            return 0;
        }
    }
}

/**
 * Interpolates a particle's current render radius between its Particle tag's radius_animation
 * bounds by its lifetime fraction (age / lifespan), then scales by the particle's own random
 * scale factor.
 *
 * @address 0x4566f0
 */
real particle_ref::current_radius()
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

    return ((tag->radius_animation[1] - tag->radius_animation[0]) * (self->age / self->lifespan) +
            tag->radius_animation[0]) * self->scale;
}

/**
 * Fires the particle's death effect or sound (if its Particle tag has one) and then always
 * deletes the particle.
 *
 * @address 0x456550
 */
void particle_ref::impact()
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

    if (*(uint32_t *)&tag->death_effect.tag_id != 0xffffffffu) {
        halo::effects::particle_impact_response_dispatch(self, *(tag_group *)&tag->death_effect.tag_fourcc,
            *(datum_index *)&tag->death_effect.tag_id, 0.0f);
    }

    halo::memory::datum_delete(particle_data, particle_handle);
}

/**
 * REWRITTEN from objdump. The particle's velocity (+0x48) is scaled by 1/30 (per tick). An 'effe' tag spawns
 * effect_new_with_color(tag, -1, &scaled velocity, 2, {"velocity", "gravity"}, {position, position},
 * {normalize(+0x3c), down}, intensity, 0, 0, 0, 0). A 'snd!' tag plays at {position, forward, scaled
 * velocity, the particle's leaf/cluster} scaled by intensity. The draft passed neither the particle nor the
 * tag index, so the effect had no definition and no position, and the sound had no placement.
 *
 * @address 0x4565a0
 */
void particle_ref::impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index, real intensity)
{
    real_vector3d velocity;

    velocity.i = self->velocity.i * 0.033333335f;
    velocity.j = self->velocity.j * 0.033333335f;
    velocity.k = self->velocity.k * 0.033333335f;
    if (fourcc == 0x65666665) {
        real_point3d points[2];
        real_vector3d vectors[2];

        points[0] = self->position;
        points[1] = self->position;
        vectors[0] = self->direction;
        vectors[1] = *global_down3d_pointer;
        halo::math::vector3d_normalize_with_length(vectors[0]);
        halo::effects::effect_new_with_color(definition_index, 0xffffffff, &velocity, 2, (uint32_t)(uintptr_t)particle_impact_vector_names, points,
            (uint32_t)(uintptr_t)vectors, intensity, 0.0f, 0, 0, 0);
    } else if (fourcc == 0x736e6421) {
        sound_placement placement;

        placement.position = *(Point3D *)&self->position;
        placement.forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
        placement.velocity = *(Vector3D *)&velocity;
        *(bsp_leaf_reference *)&placement.leaf_index = self->location;
        halo::sound::sound_start_at_location(definition_index, &placement, intensity);
    }
}

/**
 * Creates a new individual particle from a particle_creation_data block: resolves its spawn
 * position (explicit world position, an object marker, or a first person weapon marker), rolls
 * its lifespan/animation-rate/flags, folds in gravity for a world space particle, samples ambient
 * (and optionally diffuse) lighting into its colour, and rolls its starting sequence and frame.
 * Aborts without creating anything if the definition index is -1 or the spawn position's BSP
 * leaf cannot be resolved, or if no local player can currently see the target cluster.
 *
 * @address 0x455740
 */
void particle_ref::create(particle_creation_data *creation_data)
{
    Particle *tag;
    real_point3d position;
    int32_t leaf;
    int16_t cluster = -1;
    uint32_t visible;

    if (creation_data->definition_index == (datum_index)0xffffffff) {
        return;
    }
    tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)creation_data->definition_index].data;

    if (creation_data->object_index == (datum_index)0xffffffff) {
        position = creation_data->position;
    } else if (creation_data->first_person == 0) {
        object *obj = ((object_header *)object_data->data)[creation_data->object_index & 0xffff].data;
        real_matrix4x3 *marker = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset +
            creation_data->marker_index * 0x34);
        halo::math::matrix4x3_transform_point(position, creation_data->position, *marker);
    } else {
        real_matrix4x3 *marker = (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
            creation_data->first_person_weapon_index * 0x1ea0 +
            (uint16_t)creation_data->marker_index * 0x34);
        halo::math::matrix4x3_transform_point(position, creation_data->position, *marker);
    }

    leaf = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &position);
    if (leaf == -1) {
        return;
    }
    cluster = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + ((uint32_t)leaf & 0x7fffffff) * 0x10 + 8);

    visible = *(uint32_t *)((uint8_t *)local_player_globals + 0x58 + (cluster >> 5) * 4) &
        (1u << (cluster & 0x1f));
    if (visible == 0) {
        return;
    }

    {
        datum_index handle = halo::memory::datum_new(particle_data);

        if (handle != (datum_index)0xffffffff) {
            particle *self = &((particle *)particle_data->data)[handle & 0xffff];
            real speed;

            self->flags = 0;
            if ((tag->flags & 0x1) != 0) {
                self->flags |= (uint16_t)(halo::effects::effect_random_uint16() & 1);
            }
            if ((tag->flags & 0x400) != 0) {
                self->flags |= (uint16_t)(halo::effects::effect_random_uint16() & 4);
            }
            if ((tag->flags & 0x800) != 0) {
                self->flags |= (uint16_t)(halo::effects::effect_random_uint16() & 8);
            }
            self->flags = (creation_data->third_person_only == 0) ? (self->flags & ~0x10) : (self->flags | 0x10);
            self->flags = (creation_data->first_person_only == 0) ? (self->flags & ~0x20) : (self->flags | 0x20);
            self->flags = (creation_data->first_person == 0) ? (self->flags & ~0x40) : (self->flags | 0x40);

            self->definition_index = creation_data->definition_index;
            self->first_person_weapon_index = creation_data->first_person_weapon_index;
            self->object_index = creation_data->object_index;
            self->marker_index = creation_data->marker_index;
            self->sequence_state = _particle_sequence_state_new;
            self->last_update_tick = render_frame_index;

            speed = halo::math::random_range_real(tag->lifespan[0], tag->lifespan[1]);
            if (speed > 0.7f) {
                speed = (speed - 0.7f) / (real)local_player_globals->local_player_count + 0.7f;
            }
            self->lifespan = speed;

            self->animation_timer = -1.0f;

            if (tag->animation_rate[1] == 0.0f) {
                self->inverse_animation_period = 3.4028235e+38f;
            } else {
                self->inverse_animation_period = 1.0f / halo::math::random_range_real(tag->animation_rate[0],
                                                                           tag->animation_rate[1]);
            }

            self->location.leaf_index = leaf;
            self->location.cluster_index = cluster;

            self->position = creation_data->position;
            self->direction = *(real_vector3d *)&creation_data->direction;
            self->velocity = creation_data->velocity;
            self->rotation = creation_data->rotation;

            if (self->object_index == (datum_index)0xffffffff) {
                real radius = halo::effects::particle_current_radius(handle);
                PointPhysics *physics = (PointPhysics *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data;
                real fold = radius * physics->mass_scale * radius * radius;

                self->velocity.i = self->velocity.i + fold * creation_data->gravity.i;
                self->velocity.j = self->velocity.j + fold * creation_data->gravity.j;
                self->velocity.k = self->velocity.k + fold * creation_data->gravity.k;
            }

            self->angular_velocity = creation_data->angular_velocity;
            self->scale = creation_data->scale;
            self->color = creation_data->color;

            if ((tag->flags & 0x200) == 0 || (tag->flags & 0x40) != 0) {
                real_vector3d ambient, incident;

                object_sample_ambient_lightmap_point(&position, &ambient, &incident, 0);
                if ((tag->flags & 0x200) == 0) {
                    self->color.red = self->color.red * ambient.i;
                    self->color.green = self->color.green * ambient.j;
                    self->color.blue = self->color.blue * ambient.k;
                }
                if ((tag->flags & 0x40) != 0) {
                    self->color.red = self->color.red * incident.i;
                    self->color.green = self->color.green * incident.j;
                    self->color.blue = self->color.blue * incident.k;
                }
            }

            if (halo::effects::particle_next_sequence(handle) != 0) {
                Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[tag->bitmap.tag_id.index].data;
                BitmapGroupSequence *sequence =
                    (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + self->sequence_index;

                if ((tag->flags & 0x4) != 0) {
                    int16_t roll = (int16_t)halo::effects::effect_random_int_between(0, (int16_t)sequence->sprites.count);
                    self->frame_index = roll + (((-(int16_t)((self->flags & 1) != 0)) & 2) - 1);
                    return;
                }
                if ((self->flags & 1) != 0) {
                    self->frame_index = (int16_t)sequence->sprites.count;
                    return;
                }
                self->frame_index = -1;
            }
        }
    }
}

/**
 * Walks a particle's sequence state machine (new -> initial -> looping -> final -> finished),
 * rolling a random frame out of the Particle tag's sequence bounds each time a state is
 * (re)entered, and clamps the result against the number of sequences the live bitmap actually
 * has. Returns nonzero while the particle still has a valid frame to render; on failure it
 * defers to particle_impact and reports false.
 *
 * @address 0x455e60
 */
uint8_t particle_ref::next_sequence()
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[tag->bitmap.tag_id.index].data;

    self->sequence_index = -1;

    if (self->sequence_state == _particle_sequence_state_new) {
        if (tag->initial_sequence_count > 0) {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->initial_sequence_count) >> 16) + tag->first_sequence_index;
        }
        self->sequence_state = _particle_sequence_state_initial;
    }

    if (self->sequence_index == -1 && self->sequence_state == _particle_sequence_state_initial) {
        self->sequence_state = _particle_sequence_state_looping;
    }
    if (self->sequence_state == _particle_sequence_state_looping) {
        if (!(self->age < self->lifespan) || tag->looping_sequence_count < 1) {
            self->sequence_state = self->sequence_state + 1;
        } else {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->looping_sequence_count) >> 16) +
                tag->initial_sequence_count + tag->first_sequence_index;
        }
    }

    if (self->sequence_index == -1 && self->sequence_state == _particle_sequence_state_final) {
        if (tag->final_sequence_count > 0) {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->final_sequence_count) >> 16) +
                tag->looping_sequence_count + tag->initial_sequence_count + tag->first_sequence_index;
        }
        self->sequence_state = self->sequence_state + 1;
    }

    if (self->sequence_index != -1 && bitmap->bitmap_group_sequence.count != 0) {
        int32_t clamped = self->sequence_index;
        int32_t max_index = (int32_t)bitmap->bitmap_group_sequence.count - 1;

        if (clamped < 0) {
            self->sequence_index = 0;
            return 1;
        }
        if (max_index < clamped) {
            clamped = max_index;
        }
        self->sequence_index = (int16_t)clamped;
        return 1;
    }

    halo::effects::particle_impact(particle_handle);
    return 0;
}

/**
 * Per-tick motion update for one particle. A particle at rest just re-validates its attached
 * object (if any). An object-attached particle otherwise decays its own velocity by a reduced
 * version of the PointPhysics air-friction formula and integrates position directly (no
 * collision test). A free-standing particle instead runs the full point-physics collision tick,
 * firing its collision/material effect and death-on-contact flags, and both paths settle the
 * particle to rest once its velocity drops below a small threshold (or, for a world space
 * particle, once it lands on a roughly upward-facing surface).
 * FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads ECX; particle_handle arrive(s) on the stack (2 stack argument(s)).
 *
 * @address 0x4561a0
 */
uint8_t particle_ref::update_motion(real delta_time)
{
    datum_index particle_handle = datum;
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    uint8_t settled = 0;

    if ((self->flags & _particle_at_rest_bit) != 0) {
        if (self->object_index == k_datum_index_none) {
            return 1;
        }
        if (object_try_and_get(self->object_index, 0xffffffff) != 0) {
            return 1;
        }
        halo::memory::datum_delete(particle_data, particle_handle);
        return 0;
    }

    if (self->object_index == k_datum_index_none) {
        PointPhysics *physics = (PointPhysics *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data;
        real radius = halo::effects::particle_current_radius(particle_handle);
        real_vector3d out_normal;
        int16_t out_material_type;
        uint32_t collision_flags;
        uint8_t collided;

        collision_flags = halo::physics::point_physics_tick(&self->velocity, 0, physics, &self->location,
            0xffffffff, &self->position, (real_vector3d *)0, &out_normal, &out_material_type,
            radius, delta_time);

        collided = (collision_flags & _point_physics_collided_bit) != 0;

        if (collided) {
            if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu ||
                *(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id != 0u) {
                real speed = (real)sqrt((double)(self->velocity.k * self->velocity.k +
                    self->velocity.j * self->velocity.j + self->velocity.i * self->velocity.i)) - 0.5f;
                speed = (speed < 0.0f) ? 0.0f : (speed > 1.0f ? 1.0f : speed);

                if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu) {
                    halo::effects::particle_impact_response_dispatch(self, *(tag_group *)&tag->collision_effect.tag_fourcc,
                        *(datum_index *)&tag->collision_effect.tag_id, speed);
                }
                if (*(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id != 0xffffffffu &&
                    any_local_player_within_10_units(&self->position) != 0) {
                    halo::effects::material_effects_play_at_marker(
                        *(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id,
                        8, out_material_type, (uint32_t *)&self->location,
                        *(uint32_t *)&speed, &self->position, &out_normal);
                }
            }
            if ((tag->flags & 0x20) != 0) {
                if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu) {
                    halo::memory::datum_delete(particle_data, particle_handle);
                    return 0;
                }
                halo::effects::particle_impact(particle_handle);
                return 0;
            }
        }

        if (((collision_flags & _point_physics_in_air_bit) != 0 && (tag->flags & 0x100) != 0) ||
            ((collision_flags & _point_physics_in_water_bit) != 0 && (tag->flags & 0x80) != 0)) {
            halo::effects::particle_impact(particle_handle);
            return 0;
        }

        if (collided || (collision_flags & _point_physics_hit_water_surface_bit) != 0) {
            settled = out_normal.k > 0.8f;
            self->inverse_animation_period = self->inverse_animation_period +
                tag->contact_deterioration;
        }
    } else {
        PointPhysics *physics = (PointPhysics *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data;
        real radius;
        real friction, mass_related, decay;

        if ((self->flags & _particle_first_person_bit) == 0 &&
            object_try_and_get(self->object_index, 0xffffffff) == 0) {
            halo::memory::datum_delete(particle_data, particle_handle);
            return 0;
        }

        radius = halo::effects::particle_current_radius(particle_handle);
        friction = radius * physics->air_friction * radius;
        mass_related = radius * physics->mass_scale * radius * radius;

        if (mass_related == 0.0f) {
            decay = (friction == 0.0f) ? 1.0f : 0.0f;
        } else {
            decay = 1.0f - (friction / mass_related) * delta_time;
            if (decay < 0.0f) {
                decay = 0.0f;
            } else if (decay > 1.0f) {
                decay = 1.0f;
            }
        }

        settled = 1;
        self->velocity.i = self->velocity.i * decay;
        self->velocity.j = self->velocity.j * decay;
        self->velocity.k = self->velocity.k * decay;
        self->position.x = self->position.x + self->velocity.i * delta_time;
        self->position.y = self->position.y + self->velocity.j * delta_time;
        self->position.z = self->position.z + self->velocity.k * delta_time;
    }

    if (self->velocity.k * self->velocity.k + self->velocity.j * self->velocity.j +
        self->velocity.i * self->velocity.i >= 0.0625f) {
        self->direction = self->velocity;
    } else if (settled) {
        if ((tag->flags & 0x10) != 0) {
            halo::effects::particle_impact(particle_handle);
            return 0;
        }
        self->flags |= _particle_at_rest_bit;
    }

    self->rotation = self->rotation + delta_time * self->angular_velocity;
    return 1;
}

/**
 * Deletes every first person, object-attached particle whose first_person_weapon_index matches
 * the given row -- used when a first person weapon slot is being released.
 *
 * @address 0x455c80
 */
void particle_ref::delete_by_first_person_weapon(uint8_t first_person_weapon_index)
{
    datum_index particle_index = halo::memory::datum_next(-1, particle_data);

    while (particle_index != k_datum_index_none) {
        particle *self = &((particle *)particle_data->data)[(uint16_t)particle_index];

        if (self->first_person_weapon_index == first_person_weapon_index &&
            (self->flags & _particle_first_person_bit) != 0 &&
            self->object_index != k_datum_index_none) {
            halo::memory::datum_delete(particle_data, particle_index);
        }

        particle_index = halo::memory::datum_next((int16_t)particle_index, particle_data);
    }
}

/**
 * Member form of the original particles_refresh_structure_locations: refresh structure locations.
 *
 * @address 0x455d20
 */
void particle_ref::refresh_structure_locations()
{
    datum_index handle;

    for (handle = halo::memory::datum_next(-1, particle_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, particle_data)) {
        particle *entry = (particle *)((uint8_t *)particle_data->data + (handle & 0xffff) * 0x70);
        real_point3d *point;
        uint32_t leaf;
        int16_t cluster;

        if (entry->object_index == k_datum_index_none) {
            point = &entry->position;
        } else if (entry->flags & 0x40) {
            point = (real_point3d *)(first_person_weapon_interfaces + entry->first_person_weapon_index * 0x1ea0 +
                entry->marker_index * 0x34 + 0x10b4);
        } else {
            uint8_t *owner = (uint8_t *)object_try_and_get(entry->object_index, 0xffffffff);

            if (owner == 0) {
                halo::memory::datum_delete(particle_data, handle);
                continue;
            }
            point = (real_point3d *)(owner + ((struct object *)owner)->nodes.offset + entry->marker_index * 0x34 + 0x28);
        }
        leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, point);
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            cluster = -1;
        } else {
            cluster = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
        }
        entry->location.cluster_index = cluster;
        if (cluster == -1) {
            halo::memory::datum_delete(particle_data, handle);
        }
    }
}

/**
 * Per-tick driver: ages every live particle, advances its animation and motion while it still
 * has time (or life) left, triggers its impact response once its lifespan (and any final
 * sequence) has run out, and deletes particles that have gone stale (not touched for 16 ticks).
 *
 * @address 0x455b60
 */
void particle_ref::update(real delta_time)
{
    datum_index particle_index = halo::memory::datum_next(-1, particle_data);

    while (particle_index != k_datum_index_none) {
        particle *self = &((particle *)particle_data->data)[(uint16_t)particle_index];
        real age_before = self->age;
        Particle *tag = (Particle *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;

        if (render_frame_index - self->last_update_tick < 0x10) {
            self->age = delta_time + self->age;

            if (self->age < self->lifespan || age_before == 0.0f || tag->final_sequence_count != 0) {
                if (halo::effects::particle_advance_animation(particle_index, delta_time) != 0) {
                    halo::effects::particle_update_motion(particle_index, delta_time);
                }
            } else {
                halo::effects::particle_impact(particle_index);
            }
        } else {
            halo::memory::datum_delete(particle_data, particle_index);
        }

        particle_index = halo::memory::datum_next((int16_t)particle_index, particle_data);
    }
}

}

namespace halo::effects {

uint8_t particle_advance_animation(datum_index particle_handle, real delta_time)
{
    return halo::effects::particle_ref(particle_handle).advance_animation(delta_time);
}

uint8_t particle_advance_frame(datum_index particle_handle)
{
    return halo::effects::particle_ref(particle_handle).advance_frame();
}

real particle_current_radius(datum_index particle_handle)
{
    return halo::effects::particle_ref(particle_handle).current_radius();
}

void particle_impact(datum_index particle_handle)
{
    halo::effects::particle_ref(particle_handle).impact();
}

void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index, real intensity)
{
    halo::effects::particle_ref::impact_response_dispatch(self, fourcc, definition_index, intensity);
}

void particle_new(particle_creation_data *creation_data)
{
    halo::effects::particle_ref::create(creation_data);
}

uint8_t particle_next_sequence(datum_index particle_handle)
{
    return halo::effects::particle_ref(particle_handle).next_sequence();
}

uint8_t particle_update_motion(datum_index particle_handle, real delta_time)
{
    return halo::effects::particle_ref(particle_handle).update_motion(delta_time);
}

void particles_delete_by_first_person_weapon(uint8_t first_person_weapon_index)
{
    halo::effects::particle_ref::delete_by_first_person_weapon(first_person_weapon_index);
}

void particles_refresh_structure_locations()
{
    halo::effects::particle_ref::refresh_structure_locations();
}

void particles_update(real delta_time)
{
    halo::effects::particle_ref::update(delta_time);
}

}
