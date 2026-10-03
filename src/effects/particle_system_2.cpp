#include "halo/effects/effects.hpp"

extern "C" {
extern data_array *particle_system_data;
extern data_array *particle_system_particle_data;
extern data_array *object_data;
extern tag_instance *tag_instances;
extern random_seed effect_random_seed;
extern void (*particle_system_update_physics_table[2])(particle_system *self, float delta_time);
extern void (*particle_update_physics_table[1])(particle_system *self, int32_t type_index, float delta_time, particle_system_particle *particle);
extern void datum_delete(data_array *array, datum_index handle);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity, real_vector3d *out_angular_velocity);
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value);
extern void particle_system_spawn(particle_system *self, int32_t type_index, float dt);
extern void particle_system_delete(datum_index handle);
extern real random_real_range_seeded(random_seed *seed, real min, real max);
extern void particle_system_advance_type_state(particle_system_type_state *state, ParticleSystemType *type, particle_system *system);
extern void particle_system_advance_particle_state(particle_system_particle *particle, ParticleSystemType *type);
extern void particle_system_roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states, particle_state_values *out);
void particle_system_update(float delta_time, datum_index handle);
}

namespace halo::effects {

/**
 * Member form of the original particle_system_update: update.
 *
 * @address 0x4544f0
 */
void particle_system_ref::update(float delta_time)
{
    datum_index handle = datum;
    particle_system *self = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition = (ParticleSystem *)tag_instances[self->definition_index & 0xffff].data;
    int32_t type_index;
    int32_t types_alive = 0;

    if (self->object_index != (datum_index)0xffffffff) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        float function_value;

        if ((obj->flags & _object_needs_cluster_update_bit) != 0 &&
            object_function_get_value(self->object_index, self->scale_function_index,
                                       &function_value)) {
            self->flags |= _particle_system_emitting_bit;
        } else {
            self->flags &= ~(uint32_t)_particle_system_emitting_bit;
        }

        object_get_position(&self->position, self->object_index);
        object_get_root_object_velocities(self->object_index, &self->velocity, (real_vector3d *)0);
        self->velocity.i *= 30.0f;
        self->velocity.j *= 30.0f;
        self->velocity.k *= 30.0f;
    }

    particle_system_update_physics_table[definition->system_update_physics](self, delta_time);

    for (type_index = 0; type_index < (int32_t)definition->particle_types.count; type_index++) {
        ParticleSystemType *type =
            &((ParticleSystemType *)definition->particle_types.pointer)[type_index];
        particle_system_type_state *state = &self->type_states[type_index];

        if ((type->flags & 0x100) != 0) {
            continue;
        }
        state->state_time_remaining -= delta_time;
        if (state->state_index == -1) {
            continue;
        }

        {
            ParticleSystemTypeStates *current_state;
            uint8_t skip_interpolation = 0;

            for (;;) {
                current_state = &((ParticleSystemTypeStates *)type->states.pointer)[state->state_index];
                if (0.0f <= state->state_time_remaining) {
                    break;
                }

                {
                    int16_t next_index = state->next_state_index;
                    float min_bound, max_bound, rolled;

                    if (next_index == -1) {
                        particle_system_advance_type_state(state, type, self);
                        min_bound = current_state->transition_time_bounds[0];
                        max_bound = current_state->transition_time_bounds[1];
                    } else {
                        state->state_index = next_index;
                        state->next_state_index = -1;
                        current_state =
                            &((ParticleSystemTypeStates *)type->states.pointer)[next_index];
                        min_bound = current_state->duration_bounds[0];
                        max_bound = current_state->duration_bounds[1];
                    }

                    rolled = random_real_range_seeded(&effect_random_seed, min_bound, max_bound);
                    state->state_duration = rolled;
                    state->state_time_remaining += rolled;

                    if (state->state_index == -1) {
                        skip_interpolation = 1;
                        break;
                    }
                }
            }

            if (!skip_interpolation) {
                if (state->next_state_index == -1) {
                    state->scale = current_state->scale_multiplier;
                    state->animation_rate = current_state->animation_rate_multiplier;
                    state->rotation_rate = current_state->rotation_rate_multiplier;
                    state->color = current_state->color_multiplier;
                    state->radius = current_state->radius_multiplier;
                    state->minimum_particle_count = current_state->minimum_particle_count;
                    state->particle_creation_rate = current_state->particle_creation_rate;
                } else {
                    ParticleSystemTypeStates *next_state = &((ParticleSystemTypeStates *)
                        type->states.pointer)[state->next_state_index];
                    float fraction = state->state_time_remaining / state->state_duration;

                    fraction = (fraction < 0.0f) ? 0.0f : (1.0f < fraction ? 1.0f : fraction);

                    state->scale = (1.0f - fraction) * next_state->scale_multiplier +
                                   fraction * current_state->scale_multiplier;
                    state->animation_rate = (1.0f - fraction) * next_state->animation_rate_multiplier +
                                   fraction * current_state->animation_rate_multiplier;
                    state->rotation_rate = (1.0f - fraction) * next_state->rotation_rate_multiplier +
                                   fraction * current_state->rotation_rate_multiplier;
                    state->color.alpha = (1.0f - fraction) * next_state->color_multiplier.alpha +
                                   fraction * current_state->color_multiplier.alpha;
                    state->color.red = (1.0f - fraction) * next_state->color_multiplier.red +
                                   fraction * current_state->color_multiplier.red;
                    state->color.green = (1.0f - fraction) * next_state->color_multiplier.green +
                                   fraction * current_state->color_multiplier.green;
                    state->color.blue = (1.0f - fraction) * next_state->color_multiplier.blue +
                                   fraction * current_state->color_multiplier.blue;
                    state->radius = (1.0f - fraction) * next_state->radius_multiplier +
                                   fraction * current_state->radius_multiplier;
                    state->minimum_particle_count =
                        (1.0f - fraction) * next_state->minimum_particle_count +
                        fraction * current_state->minimum_particle_count;
                    state->particle_creation_rate =
                        (1.0f - fraction) * next_state->particle_creation_rate +
                        fraction * current_state->particle_creation_rate;
                }

                if ((type->flags & 0x200) != 0) {
                    state->color.alpha *= self->color.alpha;
                    state->color.red *= self->color.red;
                    state->color.green *= self->color.green;
                    state->color.blue *= self->color.blue;
                }
                if ((type->flags & 0x800) != 0) {
                    state->minimum_particle_count *= self->scale;
                }
                if ((type->flags & 0x1000) != 0) {
                    state->particle_creation_rate *= self->scale;
                }
                if ((type->flags & 0x2000) != 0) {
                    state->scale *= self->scale;
                }
                if ((type->flags & 0x4000) != 0) {
                    state->animation_rate *= self->scale;
                }
                if ((type->flags & 0x8000) != 0) {
                    state->rotation_rate *= self->scale;
                }
            }

            if (state->state_index == -1) {
                continue;
            }

            {
                uint16_t particle_index;
                uint16_t previous_particle = 0xffff;

                if ((self->flags & _particle_system_emitting_bit) != 0) {
                    particle_system_spawn(self, type_index, delta_time);
                }

                particle_index = (uint16_t)state->first_particle;
                while (particle_index != 0xffff) {
                    particle_system_particle *particle = &((particle_system_particle *)
                        particle_system_particle_data->data)[particle_index];

                    particle->state_time_remaining -= delta_time;

                    if (particle->state_index == -1 && 0 < (int32_t)type->particle_states.count) {
                        ParticleSystemTypeParticleState *initial_state =
                            (ParticleSystemTypeParticleState *)type->particle_states.pointer;
                        float rolled;

                        particle->state_index = 0;
                        rolled = random_real_range_seeded(&effect_random_seed,
                            initial_state->duration_bounds[0], initial_state->duration_bounds[1]);
                        particle->state_duration = rolled;
                        particle->state_time_remaining = rolled;
                        particle_system_roll_particle_state(0,
                            (ParticleSystemTypeParticleState *)type->particle_states.pointer,
                            &particle->values);
                    }

                    if (particle->active == 0) {
                        particle->state_index = -1;
                    }

                    if (particle->state_index != -1) {
                        while (particle->state_index != -1 && particle->state_time_remaining < 0.0f) {
                            ParticleSystemTypeParticleState *particle_states =
                                (ParticleSystemTypeParticleState *)type->particle_states.pointer;
                            int16_t next_index = particle->next_state_index;
                            ParticleSystemTypeParticleState *entry;
                            float min_bound, max_bound, rolled;

                            if (next_index == -1) {
                                particle_system_advance_particle_state(particle, type);
                                entry = &particle_states[particle->state_index];
                                min_bound = entry->transition_time_bounds[0];
                                max_bound = entry->transition_time_bounds[1];
                            } else {
                                particle->state_index = next_index;
                                particle->next_state_index = -1;
                                entry = &particle_states[next_index];
                                min_bound = entry->duration_bounds[0];
                                max_bound = entry->duration_bounds[1];
                            }

                            rolled = random_real_range_seeded(&effect_random_seed, min_bound, max_bound);
                            particle->state_duration = rolled;
                            particle->state_time_remaining += rolled;

                            if (particle->next_state_index == -1) {
                                particle->values = particle->next_values;
                            } else {
                                particle_system_roll_particle_state(particle->next_state_index,
                                    particle_states, &particle->next_values);
                            }
                        }

                        if (particle->state_index != -1) {
                            float animation_rate, rotation_rate;

                            if (particle->next_state_index == -1) {
                                particle->rotation += particle->values.rotation_rate *
                                                       state->rotation_rate * delta_time;
                                animation_rate = particle->values.animation_rate;
                            } else {
                                float fraction = particle->state_time_remaining /
                                                  particle->state_duration;
                                fraction = (fraction < 0.0f) ? 0.0f :
                                           (1.0f < fraction ? 1.0f : fraction);
                                rotation_rate = fraction * particle->values.rotation_rate +
                                                (1.0f - fraction) * particle->next_values.rotation_rate;
                                particle->rotation += rotation_rate * state->rotation_rate * delta_time;
                                animation_rate = fraction * particle->values.animation_rate +
                                                 (1.0f - fraction) * particle->next_values.animation_rate;
                            }
                            particle->frame += animation_rate * state->animation_rate * delta_time;

                            particle_update_physics_table[current_state->particle_update_physics](
                                self, type_index, delta_time, particle);

                            previous_particle = particle_index;
                            particle_index = (uint16_t)particle->next_particle;
                            continue;
                        }
                    }

                    if (previous_particle == 0xffff) {
                        state->first_particle = particle->next_particle;
                    } else {
                        ((particle_system_particle *)particle_system_particle_data->data)
                            [previous_particle].next_particle = particle->next_particle;
                    }
                    datum_delete(particle_system_particle_data, particle_index);
                    particle_index = (uint16_t)particle->next_particle;
                    state->particle_count = state->particle_count - 1;
                }
                types_alive++;
            }
        }
    }

    self->flags &= ~(uint32_t)_particle_system_in_update_bit;
    if (types_alive == 0 && self->object_index == (datum_index)0xffffffff) {
        particle_system_delete(handle);
    }
}

}

extern "C" {

void particle_system_update(float delta_time, datum_index handle)
{
    halo::effects::particle_system_ref(handle).update(delta_time);
}

}
