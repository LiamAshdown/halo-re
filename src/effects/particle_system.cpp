#include "halo/effects/effects.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/render/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern void effect_random_direction_from_table(real_point3d *out);
extern data_array *particle_system_data;
extern data_array *particle_system_particle_data;
extern uint8_t particle_systems_enabled;
extern const ColorARGB *global_white_argb;
extern const ColorRGB *global_white_color;
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern uint8_t particle_system_update(float delta_time, datum_index handle);
extern real_point3d *global_zero_vector3d_pointer;
extern void object_get_root_location(int32_t *out, uint32_t object_index);
extern int16_t current_local_player_index;
extern uint8_t *first_person_weapon_interfaces;
extern const real_vector3d *global_origin3d_pointer;
extern void (*particle_creation_physics_table[3])(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker);
extern player_globals *local_player_globals;
}

namespace halo::effects {

/**
 * Member form of the original particle_creation_physics_default: creation physics default.
 *
 * @address 0x455310
 */
void particle_system_view::creation_physics_default(int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    particle_system * system = record;
    (void)type_index;

    particle->position.x = marker->node_transform.position.x;
    particle->position.y = marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z;

    particle->velocity.x = system->velocity.i;
    particle->velocity.y = system->velocity.j;
    particle->velocity.z = system->velocity.k;
}

/**
 * ParticleSystem.particle_creation_physics dispatch table entry 1, "explosion". Draws a random
 * direction from the shared sphere-point table, scales its horizontal and vertical components
 * separately by the type's physics constants (optionally forcing the vertical component
 * positive), offsets the particle's spawn position by that scaled impulse relative to the
 * marker, sets its horizontal-only launch direction (then rotates it 90 degrees about world up),
 * and combines the scaled impulse with the system's own velocity into unknown_28.
 *
 * @address 0x4554e0
 */
void particle_system_view::creation_physics_explosion(int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    particle_system * system = record;
    ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
        (*(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data + 0x60)) +
        (int32_t)type_index * 0x80);
    float *physics_constants = *(float **)&((struct ParticleSystemType *)particle_type)->physics_constants.pointer;
    float k0 = physics_constants[0];
    float k1 = physics_constants[1];
    float k2 = physics_constants[2];
    real_point3d direction;
    float scaled_x, scaled_y, scaled_z;

    halo::effects::effect_random_direction_from_table(&direction);

    scaled_x = k0 * direction.x;
    scaled_y = k0 * direction.y;
    scaled_z = k1 * direction.z;
    if (*(uint8_t *)&system->burst_mirror_z != 0) {
        scaled_z = (scaled_z < 0.0f) ? -scaled_z : scaled_z;
    }

    particle->position.x = scaled_x + marker->node_transform.position.x;
    particle->position.y = scaled_y + marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z + scaled_z;

    particle->direction.i = scaled_x;
    particle->direction.j = scaled_y;
    particle->direction.k = 0.0f;

    particle->velocity.x = scaled_x * k2 + system->velocity.i;
    particle->velocity.y = scaled_y * k2 + system->velocity.j;
    particle->velocity.z = k2 * scaled_z + system->velocity.k;

    halo::math::vector3d_rotate_about_axis(particle->direction, *halo::math::globals().global_up3d_pointer, 1.0f, 0.0f);
}

/**
 * ParticleSystem.particle_creation_physics dispatch table entry 2, "jet". Blends a random sphere-
 * table direction with the marker's forward axis (weighted by the type's physics constants) into
 * unknown_28, spawns the particle at the marker's position, and derives its sprite direction as
 * the cross product of unknown_28 with either world up or the marker's forward axis, depending
 * on whether the third physics constant is nonzero.
 *
 * @address 0x455610
 */
void particle_system_view::creation_physics_jet(int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    particle_system * system = record;
    ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
        (*(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data + 0x60)) +
        (int32_t)type_index * 0x80);
    float *physics_constants = *(float **)&((struct ParticleSystemType *)particle_type)->physics_constants.pointer;
    float k0 = physics_constants[0];
    float k1 = physics_constants[1];
    float k2 = physics_constants[2];
    float random_weight = k1 * k0 * 0.033333335f;
    float forward_weight = (1.0f - k1) * k0 * 0.033333335f;
    int16_t table_index;

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    table_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
        (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);

    particle->velocity.x = halo::math::globals().sphere_point_table[table_index].x * random_weight +
        forward_weight * marker->node_transform.forward.i + system->velocity.i;
    particle->velocity.y = halo::math::globals().sphere_point_table[table_index].y * random_weight +
        forward_weight * marker->node_transform.forward.j + system->velocity.j;
    particle->velocity.z = halo::math::globals().sphere_point_table[table_index].z * random_weight +
        forward_weight * marker->node_transform.forward.k + system->velocity.k;

    particle->position.x = marker->node_transform.position.x;
    particle->position.y = marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z;

    if (k2 != 0.0f) {
        halo::math::vector3d_cross_product(*((real_vector3d *)&particle->direction),
            *halo::math::globals().global_up3d_pointer, *((real_vector3d *)&particle->velocity));
    } else {
        halo::math::vector3d_cross_product(*((real_vector3d *)&particle->direction),
            *((real_vector3d *)&particle->velocity), marker->node_transform.forward);
    }
}

/**
 * Member form of the original particle_system_advance_particle_state: advance particle state.
 *
 * @address 0x454450
 */
void particle_system_ref::advance_particle_state(particle_system_particle *particle, ParticleSystemType *type)
{
    int16_t direction = particle->ping_pong_forward ? 1 : -1;
    int16_t next = particle->state_index + direction;

    particle->next_state_index = next;

    if (next < 0 || (int32_t)type->particle_states.count <= next) {
        if ((type->flags & 0x04) != 0 && 0 < (int32_t)type->particle_states.count) {
            if ((type->flags & 0x08) == 0) {
                particle->next_state_index = 0;
                return;
            }

            {
                int32_t reflected = (int32_t)particle->state_index - direction;

                if (reflected < 0) {
                    particle->next_state_index = 0;
                    particle->ping_pong_forward = (particle->ping_pong_forward == 0);
                    return;
                }

                {
                    int32_t last = (int32_t)type->particle_states.count - 1;

                    if (last < reflected) {
                        reflected = last;
                    }
                    particle->next_state_index = (int16_t)reflected;
                    particle->ping_pong_forward = (particle->ping_pong_forward == 0);
                }
            }
        } else {
            particle->state_index = -1;
            particle->next_state_index = -1;
        }
    }
}

/**
 * Member form of the original particle_system_advance_type_state: advance type state.
 *
 * @address 0x4543b0
 */
void particle_system_view::advance_type_state(particle_system_type_state *state, ParticleSystemType *type)
{
    particle_system * system = record;
    int16_t direction = state->ping_pong_forward ? 1 : -1;
    int16_t next = state->state_index + direction;

    state->next_state_index = next;

    if (next < 0 || (int32_t)type->states.count <= next) {
        if ((type->flags & 0x01) != 0 && system->object_index != (datum_index)0xffffffff &&
            0 < (int32_t)type->states.count) {
            if ((type->flags & 0x02) == 0) {
                state->next_state_index = 0;
                return;
            }

            {
                int32_t reflected = (int32_t)state->state_index - direction;

                if (reflected < 0) {
                    state->next_state_index = 0;
                    state->ping_pong_forward = (state->ping_pong_forward == 0);
                    return;
                }

                {
                    int32_t last = (int32_t)type->states.count - 1;

                    if (last < reflected) {
                        reflected = last;
                    }
                    state->next_state_index = (int16_t)reflected;
                    state->ping_pong_forward = (state->ping_pong_forward == 0);
                }
            }
        } else {
            state->state_index = -1;
            state->next_state_index = -1;
        }
    }
}

/**
 * Member form of the original particle_system_delete: destroy.
 *
 * @address 0x453f60
 */
void particle_system_ref::destroy()
{
    datum_index handle = datum;
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition = (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data;
    int32_t i;

    for (i = 0; i < (int32_t)definition->particle_types.count; i++) {
        datum_index particle_handle = system->type_states[i].first_particle;

        while (particle_handle != (datum_index)0xffffffff) {
            particle_system_particle *particle =
                &((particle_system_particle *)particle_system_particle_data->data)[particle_handle & 0xffff];
            datum_index next = particle->next_particle;

            halo::memory::datum_delete(particle_system_particle_data, particle_handle);
            particle_handle = next;
        }
    }

    halo::memory::datum_delete(particle_system_data, handle);
}

/**
 * Member form of the original particle_system_new_at_point: new at point.
 *
 * @address 0x453600
 */
datum_index particle_system_ref::new_at_point(uint32_t definition_index, real_point3d *position, real_vector3d *velocity, ColorARGB *color, float scale)
{
    datum_index handle = (datum_index)0xffffffff;

    if (particle_systems_enabled != 0) {
        handle = halo::memory::datum_new(particle_system_data);
        if (handle != (datum_index)0xffffffff) {
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & 0xffff];
            real_vector3d incident_scratch;

            system->definition_index = definition_index;
            system->object_index = (datum_index)0xffffffff;
            system->position = *position;
            system->velocity = *velocity;
            system->color = *color;
            system->scale = scale;
            system->flags |= _particle_system_emitting_bit;

            halo::objects::object_sample_ambient_lightmap_point(&system->position,
                (real_vector3d *)&system->ambient_color, &incident_scratch, 0);

            if (!halo::effects::particle_system_new_type_states(handle)) {
                halo::memory::datum_delete(particle_system_data, handle);
                return (datum_index)0xffffffff;
            }
        }
    }
    return handle;
}

/**
 * Member form of the original particle_system_new_on_marker: new on marker.
 *
 * @address 0x4536f0
 */
datum_index particle_system_ref::new_on_marker(uint32_t definition_index, uint32_t object_index, int16_t attachment_index)
{
    datum_index handle = (datum_index)0xffffffff;

    if (particle_systems_enabled != 0) {
        handle = halo::memory::datum_new(particle_system_data);
        if (handle != (datum_index)0xffffffff) {
            object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & 0xffff].data;
            ObjectAttachment *attachment = (ObjectAttachment *)(*(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[
                obj->definition_tag & 0xffff].data + 0x144) + attachment_index * 0x48);
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & 0xffff];
            object_marker marker;
            float function_value;

            system->definition_index = definition_index;
            system->object_index = object_index;
            system->attachment_index = attachment_index;
            system->scale_function_index = (int16_t)(attachment->primary_scale - 1);

            if (attachment->change_color == 0) {
                system->color = *global_white_argb;
            } else {
                system->color.red = obj->change_colors[attachment->change_color].red;
                system->color.green = obj->change_colors[attachment->change_color].green;
                system->color.blue = obj->change_colors[attachment->change_color].blue;
                system->color.alpha = 1.0f;
            }

            halo::objects::object_get_node_local_transform(object_index, attachment->marker.string, &marker, 1);
            system->position = marker.node_transform.position;

            halo::objects::object_get_root_object_velocities(object_index, &system->velocity,
                                               (real_vector3d *)0);
            system->velocity.i *= 30.0f;
            system->velocity.j *= 30.0f;
            system->velocity.k *= 30.0f;

            system->ambient_color = *global_white_color;

            if (halo::objects::object_function_get_value(object_index, system->scale_function_index,
                                           &function_value)) {
                system->flags |= _particle_system_emitting_bit;
            } else {
                system->flags &= ~(uint32_t)_particle_system_emitting_bit;
            }

            if (!halo::effects::particle_system_new_type_states(handle)) {
                halo::memory::datum_delete(particle_system_data, handle);
                return (datum_index)0xffffffff;
            }
        }
    }
    return handle;
}

/**
 * Member form of the original particle_system_new_type_states: new type states.
 *
 * @address 0x4538b0
 */
uint8_t particle_system_ref::new_type_states()
{
    datum_index handle = datum;
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition =
        (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data;
    uint8_t all_types_ok = 1;
    uint8_t any_type_ok = 0;
    int32_t leaf_index;
    int32_t i;

    leaf_index = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &system->position);
    system->location.leaf_index = leaf_index;
    system->location.cluster_index = (leaf_index == -1) ? (int16_t)0xffff :
        *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer +
                      (leaf_index & 0x7fffffff) * 0x10 + 8);
    system->flags |= _particle_system_in_update_bit;

    if (0 < (int32_t)definition->particle_types.count) {
        for (i = 0; i < (int32_t)definition->particle_types.count; i++) {
            ParticleSystemType *type =
                &((ParticleSystemType *)definition->particle_types.pointer)[i];
            particle_system_type_state *state = &system->type_states[i];

            if (type->states.count == 0) {
                all_types_ok = 0;
            } else {
                ParticleSystemTypeStates *first_state =
                    (ParticleSystemTypeStates *)type->states.pointer;

                state->state_index = 0;
                state->next_state_index = -1;
                state->ping_pong_forward = 1;
                state->particle_count = 0;
                state->first_particle = (datum_index)0xffffffff;

                if (0 < (int32_t)type->states.count) {
                    float duration = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed,
                        first_state->duration_bounds[0], first_state->duration_bounds[1]);
                    any_type_ok = 1;
                    state->state_time_remaining = duration;
                    state->state_duration = duration;
                }
            }
        }

        if (any_type_ok) {
            if (all_types_ok) {
                halo::effects::particle_system_update(0.001f, handle);
            }
            return all_types_ok;
        }
    }
    return 0;
}

/**
 * File-local helper used by particle_system_render.
 */
static float particle_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

/**
 * One group of sprites for one state: bitmap/shader from `state_definition`, drawn with `weight`.
 */
static void particle_build_state_sprite(uint8_t *type, uint8_t *state_definition, uint8_t *current_state, int32_t frame,
    real_point3d *position, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float weight)
{
    build_sprite_data data;
    uint32_t mode;

    data.bitmap_group_index = *(datum_index *)(state_definition + 0x3c);
    data.maximum_sprite_count = 2;
    data.shader = (uint32_t)(state_definition + 0xb8);
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    if (*(int16_t *)(type + 0x28) == 1) {
        mode = (type[0x20] & 0x80) ? 3 : 1;
        halo::render::build_sprite_rotational(&data, mode, (int16_t)*(uint16_t *)(state_definition + 0x40), (int16_t)frame, position,
            direction, rotation, scale, color, weight);
    } else {
        halo::render::build_sprite(&data, *(int16_t *)(state_definition + 0x40), (int16_t)frame, (int16_t)*(uint16_t *)(type + 0x2a),
            position, direction, rotation, scale, color, weight, 1);
    }
    *(uint32_t *)((uint8_t *)data.shader + 0x98) = *(uint32_t *)(current_state + 0x80);
    halo::render::build_sprites_end(&data);
}

/**
 * Member form of the original particle_system_render: render.
 *
 * @address 0x454bf0
 */
void particle_system_ref::render()
{
    datum_index particle_system_handle = datum;
    uint8_t *system = (uint8_t *)particle_system_data->data + (particle_system_handle & 0xffff) * 0x158;
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[((particle_system *)system)->definition_index & 0xffff].data;
    int16_t type_index;

    for (type_index = 0; type_index < *(int32_t *)(definition + 0x5c); type_index++) {
        uint8_t *type = *(uint8_t **)(definition + 0x60) + type_index * 0x80;
        uint8_t *type_state = system + 0x58 + type_index * 0x40;
        uint16_t particle_index;

        if (*(int16_t *)type_state == -1 || (*(uint32_t *)(type + 0x20) & 0x100)) {
            continue;
        }
        for (particle_index = *(uint16_t *)(type_state + 0x3c); particle_index != 0xffff; ) {
            uint8_t *particle = (uint8_t *)particle_system_particle_data->data + particle_index * 0x80;
            int16_t cluster = *(int16_t *)(particle + 0x18);

            if (particle[3] && (halo::structures::globals().cluster_visible_bits[cluster >> 5] & (1u << (cluster & 0x1f)))) {
                uint8_t *states = *(uint8_t **)(type + 0x78);
                uint8_t *current = states + *(int16_t *)(particle + 8) * 0x178;
                uint8_t *next = 0;
                real_point3d position;
                real_vector3d direction;
                float fraction = 1.0f;
                float inverse = 0.0f;
                float scale;
                float color[4];
                float drawn[4];
                uint8_t *bitmap;
                uint8_t *sequence;
                int16_t sequence_index;
                int32_t frame;
                float vx = *(float *)(particle + 0x34);
                float vy = *(float *)(particle + 0x38);
                float vz = *(float *)(particle + 0x3c);
                float *m = (float *)&halo::render::globals().camera_world_to_view;

                halo::math::matrix4x3_transform_point(position, *(real_point3d *)(particle + 0x1c), halo::render::globals().camera_world_to_view);
                direction.i = vx * m[1] + vy * m[4] + vz * m[7];
                direction.j = vx * m[2] + vy * m[5] + vz * m[8];
                direction.k = vx * m[3] + vy * m[6] + vz * m[9];

                if (*(int16_t *)(particle + 0xa) == -1) {
                    scale = *(float *)(particle + 0x48) * *(float *)(type_state + 0xc);
                    color[0] = *(float *)(particle + 0x54) * *(float *)(type_state + 0x18);
                    color[1] = *(float *)(particle + 0x58) * *(float *)(type_state + 0x1c);
                    color[2] = *(float *)(particle + 0x5c) * *(float *)(type_state + 0x20);
                    color[3] = *(float *)(particle + 0x60) * *(float *)(type_state + 0x24);
                } else {
                    next = states + *(int16_t *)(particle + 0xa) * 0x178;
                    fraction = particle_clamp01(*(float *)(particle + 0xc) / *(float *)(particle + 0x10));
                    inverse = 1.0f - fraction;
                    scale = (inverse * *(float *)(particle + 0x64) + fraction * *(float *)(particle + 0x48)) *
                        *(float *)(type_state + 0xc);
                    color[0] = (inverse * *(float *)(particle + 0x70) + fraction * *(float *)(particle + 0x54)) *
                        *(float *)(type_state + 0x18);
                    color[1] = (inverse * *(float *)(particle + 0x74) + fraction * *(float *)(particle + 0x58)) *
                        *(float *)(type_state + 0x1c);
                    color[2] = (inverse * *(float *)(particle + 0x78) + fraction * *(float *)(particle + 0x5c)) *
                        *(float *)(type_state + 0x20);
                    color[3] = (inverse * *(float *)(particle + 0x7c) + fraction * *(float *)(particle + 0x60)) *
                        *(float *)(type_state + 0x24);
                    if (*(int16_t *)(current + 0xb8 + 0x2a) == *(int16_t *)(next + 0xb8 + 0x2a) &&
                        *(int16_t *)(current + 0xb8 + 0x2e) == *(int16_t *)(next + 0xb8 + 0x2e) &&
                        *(int16_t *)(current + 0x40) == *(int16_t *)(next + 0x40)) {
                        fraction = 1.0f;
                        inverse = 0.0f;
                    }
                }

                bitmap = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)(current + 0x3c) & 0xffff].data;
                sequence_index = *(int16_t *)(current + 0x40);
                if (*(int16_t *)(type + 0x28) == 1) {
                    sequence_index++;
                }
                sequence = *(uint8_t **)(bitmap + 0x58) + sequence_index * 0x40;
                if (*(uint32_t *)(particle + 0x44) == 0xbf800000) {
                    int16_t count = *(int16_t *)(sequence + 0x34);
                    int16_t picked;

                    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
                    picked = (int16_t)(((uint32_t)count * (halo::math::globals().effect_random_seed >> 0x10)) >> 0x10);
                    *(float *)(particle + 0x44) = (float)picked;
                    frame = picked;
                } else {
                    int32_t value = (int16_t)(int32_t)*(float *)(particle + 0x44);
                    int32_t remainder = value % *(int32_t *)(sequence + 0x34);

                    frame = remainder;
                    if ((int16_t)remainder < 0) {
                        frame = (int32_t)(((uint32_t)remainder & 0xffff0000u) |
                            (uint16_t)((int16_t)remainder + *(int16_t *)(sequence + 0x34)));
                    }
                }

                if (fraction > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (*(int16_t *)(current + 0xe2) == 0) {
                        drawn[1] *= *(float *)&((particle_system *)system)->ambient_color;
                        drawn[2] *= *(float *)(system + 0x4c);
                        drawn[3] *= *(float *)(system + 0x50);
                    }
                    particle_build_state_sprite(type, current, current, frame, &position, &direction,
                        *(float *)(particle + 0x40), scale, (ColorARGB *)drawn, fraction);
                }
                if (inverse > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (*(int16_t *)(current + 0xe2) == 0) {
                        drawn[1] *= *(float *)&((particle_system *)system)->ambient_color;
                        drawn[2] *= *(float *)(system + 0x4c);
                        drawn[3] *= *(float *)(system + 0x50);
                    }
                    position.z += 0.001f;
                    particle_build_state_sprite(type, next, current, frame, &position, &direction,
                        *(float *)(particle + 0x40), scale, (ColorARGB *)drawn, inverse);
                }
            }
            particle_index = *(uint16_t *)(particle + 4);
        }
    }
}

/**
 * File-local helper used by particle_system_resolve_local_players.
 */
static int16_t particle_leaf_cluster(uint32_t leaf)
{
    if (leaf == 0xffffffff) {
        return -1;
    }
    return *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
}

/**
 * Member form of the original particle_system_resolve_local_players: resolve local players.
 *
 * @address 0x454080
 */
void particle_system_ref::resolve_local_players()
{
    datum_index handle;

    for (handle = halo::memory::datum_next(-1, particle_system_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, particle_system_data)) {
        uint8_t *system = (uint8_t *)particle_system_data->data + (handle & 0xffff) * 0x158;
        uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[((particle_system *)system)->definition_index & 0xffff].data;
        int32_t type_index;

        if (((particle_system *)system)->object_index != k_datum_index_none) {
            halo::objects::object_get_root_location((int32_t *)(system + 0x18), ((particle_system *)system)->object_index);
        } else {
            uint32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)(system + 0x20));
            int16_t cluster = particle_leaf_cluster(leaf);

            *(uint32_t *)&((particle_system *)system)->location.leaf_index = leaf;
            ((particle_system *)system)->location.cluster_index = cluster;
            if (cluster == -1) {
                halo::effects::particle_system_delete(handle);
                continue;
            }
        }
        for (type_index = 0; type_index < *(int32_t *)(definition + 0x5c); type_index++) {
            datum_index *link = (datum_index *)(system + 0x94 + type_index * 0x40);

            while (*link != k_datum_index_none) {
                uint8_t *particle = (uint8_t *)particle_system_particle_data->data + (*link & 0xffff) * 0x80;
                uint32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)(particle + 0x1c));
                int16_t cluster = particle_leaf_cluster(leaf);

                *(uint32_t *)&((particle_system_particle *)particle)->location.leaf_index = leaf;
                ((particle_system_particle *)particle)->location.cluster_index = cluster;
                if (cluster == -1) {
                    datum_index doomed = *link;

                    halo::memory::datum_delete(particle_system_particle_data, doomed);
                    *link = ((particle_system_particle *)particle)->next_particle;
                } else {
                    link = (datum_index *)(particle + 4);
                }
            }
        }
    }
}

/**
 * Member form of the original particle_system_roll_particle_state: roll particle state.
 *
 * @address 0x454250
 */
void particle_system_ref::roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states, particle_state_values *out)
{
    ParticleSystemTypeParticleState *state = &states[index];
    float color_fraction_bits;

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    color_fraction_bits = (float)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;

    out->animation_rate = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed,
        state->animation_rate[0], state->animation_rate[1]);
    out->rotation_rate = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed,
        state->rotation_rate[0], state->rotation_rate[1]);
    out->scale = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed,
        state->scale[0], state->scale[1]);

    out->color.alpha = (state->color_2.alpha - state->color_1.alpha) * color_fraction_bits +
                        state->color_1.alpha;
    out->color.red = (state->color_2.red - state->color_1.red) * color_fraction_bits +
                      state->color_1.red;
    out->color.green = (state->color_2.green - state->color_1.green) * color_fraction_bits +
                        state->color_1.green;
    out->color.blue = (state->color_2.blue - state->color_1.blue) * color_fraction_bits +
                       state->color_1.blue;
}

/**
 * File-local helper used by particle_system_spawn.
 */
static real particle_roll(void)
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
    return (real)(halo::math::globals().effect_random_seed >> 0x10) * 1.5259022e-05f;
}

/**
 * Member form of the original particle_system_spawn: spawn.
 *
 * @address 0x453b10
 */
void particle_system_view::spawn(int32_t type_index, float dt)
{
    particle_system * system_record = record;
    uint8_t *system = (uint8_t *)system_record;
    uint8_t *type_state = system + 0x58 + (int16_t)type_index * 0x40;
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[((struct particle_system *)system)->definition_index & 0xffff].data;
    uint8_t *type = *(uint8_t **)(definition + 0x60) + (int16_t)type_index * 0x80;
    uint8_t initial = (uint8_t)((((struct particle_system *)system)->flags >> 1) & 1);
    uint8_t *state = initial ? 0 : *(uint8_t **)(type + 0x6c) + *(int16_t *)type_state * 0xc0;
    uint32_t type_flags = *(uint32_t *)(type + 0x20);
    datum_index object_index = ((struct particle_system *)system)->object_index;
    object_marker markers[8];
    int16_t locality;
    int16_t target;
    int16_t marker_count;
    int16_t spawned;

    locality = (int16_t)halo::effects::player_weapon_locality_for_object(object_index);
    if (locality != 0) {
        if (type_flags & 0x20000) {
            if (locality == -1 || !halo::render::render_local_player_gunner_seat_visible(current_local_player_index)) {
                return;
            }
        }
        if ((type_flags & 0x10000) && locality == 1 && halo::render::render_local_player_gunner_seat_visible(current_local_player_index)) {
            return;
        }
    }

    if (initial) {
        if (type_flags & 0x400) {
            target = (int16_t)(int32_t)((double)*(int16_t *)(type + 0x24) * ((struct particle_system *)system)->scale + 0.5);
        } else {
            target = *(int16_t *)(type + 0x24);
        }
    } else {
        double amount = (double)dt * *(float *)(type_state + 0x30);
        int32_t whole = (int32_t)amount;
        double accumulated = amount - (double)whole + *(float *)(type_state + 0x34);

        target = (int16_t)(*(uint16_t *)(type_state + 0x3a) + whole);
        *(float *)(type_state + 0x34) = (float)accumulated;
        if (accumulated > 1.0) {
            target = (int16_t)(target + 1);
            *(float *)(type_state + 0x34) = (float)(accumulated - 1.0);
        }
    }
    if (particle_systems_enabled == 1) {
        target = (int16_t)(int32_t)((double)target * 0.5);
    }
    if (*(int16_t *)(type_state + 0x3a) >= target) {
        goto done;
    }

    if (object_index != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (object_index & 0xffff) * 0xc + 8);
        uint8_t *object_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object & 0xffff].data;
        char *marker_name = (char *)(*(uint8_t **)&((struct Object *)object_tag)->attachments.pointer + ((struct particle_system *)system)->attachment_index * 0x48 + 0x10);

        marker_count = (int16_t)halo::objects::object_get_node_local_transform(object_index, marker_name, markers, 8);
        halo::objects::object_get_root_location((int32_t *)(system + 0x18), object_index);
        if (marker_count == 0) {
            datum_index weapon = *(datum_index *)(first_person_weapon_interfaces + current_local_player_index * 0x1ea0 + 8);

            if (weapon != k_datum_index_none) {
                marker_count = (int16_t)halo::interface::first_person_weapon_get_marker_data(weapon, marker_name, markers, 8);
                halo::objects::object_get_root_location((int32_t *)(system + 0x18), weapon);
            }
        }
    } else {
        markers[0].node_transform.position = *(real_point3d *)&((struct particle_system *)system)->position.x;
        *(real_vector3d *)((uint8_t *)&markers[0] + 0x3c) = *global_origin3d_pointer;
        marker_count = 1;
    }

    if (((struct particle_system *)system)->location.cluster_index == -1 || *(int16_t *)(type_state + 0x3a) >= target) {
        goto done;
    }
    for (spawned = 0; marker_count != 0 && spawned < 0x80; ) {
        datum_index handle = halo::memory::datum_new(particle_system_particle_data);
        uint8_t *particle;
        int16_t physics;
        int16_t marker_index;

        if (handle == k_datum_index_none) {
            break;
        }
        particle = (uint8_t *)particle_system_particle_data->data + (handle & 0xffff) * 0x80;
        physics = initial ? *(int16_t *)(type + 0x54) : *(int16_t *)(state + 0xb0);
        ((struct particle_system_particle *)particle)->state_index = -1;
        ((struct particle_system_particle *)particle)->next_state_index = -1;
        particle[3] = 1;
        particle[2] = 1;
        ((struct particle_system_particle *)particle)->frame = -1.0f;
        ((struct particle_system_particle *)particle)->rotation = particle_roll() * 6.2831855f;
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
        marker_index = (int16_t)(((halo::math::globals().effect_random_seed >> 0x10) * (uint32_t)(int32_t)marker_count) >> 0x10);
        particle_creation_physics_table[physics](system_record, type_index, (particle_system_particle *)particle,
            &markers[marker_index]);
        halo::scenario::scenario_location_from_point((bsp_leaf_reference *)(particle + 0x14), (real_point3d *)(particle + 0x1c));
        if (((struct particle_system_particle *)particle)->location.cluster_index != -1) {
            *(int16_t *)(type_state + 0x3a) += 1;
            ((struct particle_system_particle *)particle)->next_particle = *(datum_index *)(type_state + 0x3c);
            *(datum_index *)(type_state + 0x3c) = handle;
        } else {
            halo::memory::datum_delete(particle_system_particle_data, handle);
        }
        spawned++;
        if (*(int16_t *)(type_state + 0x3a) >= target) {
            break;
        }
    }

done:
    if ((float)*(int16_t *)(type_state + 0x3a) < *(float *)(type_state + 0x2c)) {
        *(float *)(type_state + 4) = *(float *)(type_state + 4) * 0.3f;
    }
}

/**
 * ParticleSystem.system_update_physics dispatch table entry 0, the default implementation.
 * For a free-standing system (object_index == -1) whose ParticleSystem tag references a
 * point_physics tag, runs one point_physics_tick on the system's own position/velocity/location
 * with a fixed 1.0 radius, no wind probe and no collision normal/material output.
 *
 * @address 0x4552a0
 */
void particle_system_view::update_physics_default(real dt)
{
    particle_system * system = record;
    ParticleSystem *definition_tag;
    uint32_t point_physics_tag_id;

    if (system->object_index != (datum_index)-1) {
        return;
    }

    definition_tag = (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data;
    point_physics_tag_id = *(uint32_t *)&definition_tag->point_physics.tag_id;
    if (point_physics_tag_id == (uint32_t)-1) {
        return;
    }

    halo::physics::point_physics_tick(&system->velocity, 0,
        (PointPhysics *)halo::cache::globals().tag_instances[point_physics_tag_id & 0xffff].data,
        &system->location, (uint32_t)-1, &system->position, (real_vector3d *)0,
        (real_vector3d *)0, (int16_t *)0, 1.0f, dt);
}

/**
 * ParticleSystem.system_update_physics dispatch table entry 1, "explosion". Identical to the
 * default implementation (a plain jmp to it in the original binary).
 *
 * @address 0x4554d0
 */
void particle_system_view::update_physics_explosion(real dt)
{
    particle_system * system = record;
    halo::effects::particle_system_update_physics_default(system, dt);
}

/**
 * Member form of the original particle_systems_delete_all: delete all.
 *
 * @address 0x4535b0
 */
void particle_system_ref::delete_all()
{
    data_array *systems = particle_system_data;

    if (systems != (data_array *)0 && systems->valid != 0) {
        datum_index handle = halo::memory::datum_next(-1, systems);

        while (handle != (datum_index)0xffffffff) {
            halo::effects::particle_system_delete(handle);
            handle = halo::memory::datum_next((int16_t)handle, systems);
        }

        systems->valid = 0;
        particle_system_particle_data->valid = 0;
    }
}

/**
 * Per-tick driver: renders every particle system whose cluster is currently visible to a local
 * player.
 *
 * @address 0x454b40
 */
void particle_system_ref::render_all()
{
    datum_index system_index = halo::memory::datum_next(-1, particle_system_data);

    while (system_index != k_datum_index_none) {
        particle_system *system =
            &((particle_system *)particle_system_data->data)[(uint16_t)system_index];

        if (system->location.cluster_index != -1) {
            int16_t cluster = system->location.cluster_index;
            uint32_t *visible_clusters = (uint32_t *)((uint8_t *)local_player_globals + 0x58);

            if ((visible_clusters[cluster >> 5] & (1u << (cluster & 0x1f))) != 0) {
                halo::effects::particle_system_render(system_index);
            }
        }

        system_index = halo::memory::datum_next((int16_t)system_index, particle_system_data);
    }
}

/**
 * Member form of the original particle_systems_update: update all.
 *
 * @address 0x454000
 */
void particle_system_ref::update_all(float delta_time)
{
    data_array *systems = particle_system_data;
    datum_index handle = halo::memory::datum_next(-1, systems);

    while (handle != (datum_index)0xffffffff) {
        halo::effects::particle_system_update(delta_time, handle);
        handle = halo::memory::datum_next((int16_t)handle, systems);
    }
}

/**
 * ParticleSystemTypeParticleState.particle_update_physics dispatch table entry 0, the default
 * implementation. Picks the particle's current point_physics tag (or, mid-transition, blends the
 * current and next states' physics/radius by the transition fraction), runs one point_physics_
 * tick on it, and kills the particle if it hit a surface/medium its type says should kill it.
 *
 * @address 0x455350
 */
void particle_system_view::update_physics_default(int16_t type_index, real dt, particle_system_particle *particle)
{
    particle_system * system = record;
    {
        ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
            (*(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[system->definition_index & 0xffff].data + 0x60)) +
            (int32_t)type_index * 0x80);
        particle_system_type_state *type_state = &system->type_states[type_index];
        ParticleSystemTypeParticleState *states = (ParticleSystemTypeParticleState *)
            (*(uint8_t **)&((struct ParticleSystemType *)particle_type)->particle_states.pointer);
        ParticleSystemTypeParticleState *state = &states[particle->state_index];
        real radius;
        PointPhysics *physics;
        PointPhysics blended;
        uint32_t collision_flags;

        if (particle->next_state_index == -1) {
            physics = (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & 0xffff].data;
            radius = state->radius_multiplier * type_state->radius * particle_type->radius;
        } else {
            ParticleSystemTypeParticleState *next_state = &states[particle->next_state_index];
            real fraction = particle->state_time_remaining / particle->state_duration;

            if (fraction < 0.0f) {
                fraction = 0.0f;
            } else if (fraction > 1.0f) {
                fraction = 1.0f;
            }

            radius = ((1.0f - fraction) * next_state->radius_multiplier + fraction * state->radius_multiplier) *
                     type_state->radius * particle_type->radius;
            halo::physics::point_physics_interpolate(&blended,
                (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & 0xffff].data,
                (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)next_state)->point_physics.tag_id & 0xffff].data,
                fraction);
            physics = &blended;
        }

        collision_flags = halo::physics::point_physics_tick((real_vector3d *)&particle->velocity, 0, physics,
            &particle->location, (uint32_t)-1, &particle->position, (real_vector3d *)0,
            (real_vector3d *)0, (int16_t *)0, radius, dt);

        if (((collision_flags & 1) != 0 && (particle_type->flags & 0x20) != 0) ||
            ((collision_flags & 2) != 0 && (particle_type->flags & 0x10) != 0) ||
            ((collision_flags & 4) != 0 && (particle_type->flags & 0x40) != 0)) {
            particle->active = 0;
        }
    }
}

}

namespace halo::effects {

void particle_creation_physics_default(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    halo::effects::particle_system_view(system).creation_physics_default(type_index, particle, marker);
}

void particle_creation_physics_explosion(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    halo::effects::particle_system_view(system).creation_physics_explosion(type_index, particle, marker);
}

void particle_creation_physics_jet(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker)
{
    halo::effects::particle_system_view(system).creation_physics_jet(type_index, particle, marker);
}

void particle_system_advance_type_state(particle_system_type_state *state, ParticleSystemType *type, particle_system *system)
{
    halo::effects::particle_system_view(system).advance_type_state(state, type);
}

void particle_system_spawn(particle_system *system_record, int32_t type_index, float dt)
{
    halo::effects::particle_system_view(system_record).spawn(type_index, dt);
}

void particle_system_update_physics_default(particle_system *system, real dt)
{
    halo::effects::particle_system_view(system).update_physics_default(dt);
}

void particle_system_update_physics_explosion(particle_system *system, real dt)
{
    halo::effects::particle_system_view(system).update_physics_explosion(dt);
}

void particle_update_physics_default(particle_system *system, int16_t type_index, real dt, particle_system_particle *particle)
{
    halo::effects::particle_system_view(system).update_physics_default(type_index, dt, particle);
}

void particle_system_advance_particle_state(particle_system_particle *particle, ParticleSystemType *type)
{
    halo::effects::particle_system_ref::advance_particle_state(particle, type);
}

void particle_system_delete(datum_index handle)
{
    halo::effects::particle_system_ref(handle).destroy();
}

datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position, real_vector3d *velocity, ColorARGB *color, float scale)
{
    return halo::effects::particle_system_ref::new_at_point(definition_index, position, velocity, color, scale);
}

datum_index particle_system_new_on_marker(uint32_t definition_index, uint32_t object_index, int16_t attachment_index)
{
    return halo::effects::particle_system_ref::new_on_marker(definition_index, object_index, attachment_index);
}

uint8_t particle_system_new_type_states(datum_index handle)
{
    return halo::effects::particle_system_ref(handle).new_type_states();
}

void particle_system_render(datum_index particle_system_handle)
{
    halo::effects::particle_system_ref(particle_system_handle).render();
}

void particle_system_resolve_local_players()
{
    halo::effects::particle_system_ref::resolve_local_players();
}

void particle_system_roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states, particle_state_values *out)
{
    halo::effects::particle_system_ref::roll_particle_state(index, states, out);
}

void particle_systems_delete_all()
{
    halo::effects::particle_system_ref::delete_all();
}

void particle_systems_render()
{
    halo::effects::particle_system_ref::render_all();
}

void particle_systems_update(float delta_time)
{
    halo::effects::particle_system_ref::update_all(delta_time);
}

}
