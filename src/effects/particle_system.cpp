#include "halo/core/lcg.hpp"
#include "halo/scenario/leaf.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/effects/effects.hpp"
#include "halo/effects/particle_system_tags.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/render/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/effects/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/units/api.hpp"

static auto &particle_system_data = halo::link::ref<data_array *>(halo::effects::vars().particle_system_data);
static auto &particle_system_particle_data = halo::link::ref<data_array *>(halo::effects::vars().particle_system_particle_data);
static auto &particle_systems_enabled = halo::link::ref<uint8_t>(halo::effects::vars().particle_systems_enabled);
static auto &global_white_argb = halo::link::ref<const ColorARGB *>(halo::networking::vars().global_white_argb);
static auto &global_white_color = halo::link::ref<const ColorRGB *>(halo::effects::vars().global_white_color);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &first_person_weapon_interfaces = halo::link::ref<first_person_weapon_interface *>(halo::ui::vars().first_person_weapon_interfaces);
static auto &global_origin3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &particle_creation_physics_table = halo::link::ref<void (*[3])(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker)>(halo::effects::vars().particle_creation_physics_table);

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
    ParticleSystemType *particle_type = particle_system_type_at(system, type_index);
    ParticleSystemPhysicsConstant *physics_constants = (ParticleSystemPhysicsConstant *)particle_type->physics_constants.pointer;
    float k0 = physics_constants[0].k;
    float k1 = physics_constants[1].k;
    float k2 = physics_constants[2].k;
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
    ParticleSystemType *particle_type = particle_system_type_at(system, type_index);
    ParticleSystemPhysicsConstant *physics_constants = (ParticleSystemPhysicsConstant *)particle_type->physics_constants.pointer;
    float k0 = physics_constants[0].k;
    float k1 = physics_constants[1].k;
    float k2 = physics_constants[2].k;
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
        if (particle_type_has(type, particle_type_flag::particle_states_loop) && 0 < (int32_t)type->particle_states.count) {
            if (!particle_type_has(type, particle_type_flag::particle_states_ping_pong)) {
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
        if (particle_type_has(type, particle_type_flag::type_states_loop) && system->object_index != k_datum_index_none &&
            0 < (int32_t)type->states.count) {
            if (!particle_type_has(type, particle_type_flag::type_states_ping_pong)) {
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
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & halo::k_slot_mask];
    ParticleSystem *definition = (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & halo::k_slot_mask].data;
    int32_t i;

    for (i = 0; i < (int32_t)definition->particle_types.count; i++) {
        datum_index particle_handle = system->type_states[i].first_particle;

        while (particle_handle != k_datum_index_none) {
            particle_system_particle *particle =
                &((particle_system_particle *)particle_system_particle_data->data)[particle_handle & halo::k_slot_mask];
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
    datum_index handle = k_datum_index_none;

    if (particle_systems_enabled != 0) {
        handle = halo::memory::datum_new(particle_system_data);
        if (handle != k_datum_index_none) {
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & halo::k_slot_mask];
            real_vector3d incident_scratch;

            system->definition_index = definition_index;
            system->object_index = k_datum_index_none;
            system->position = *position;
            system->velocity = *velocity;
            system->color = *color;
            system->scale = scale;
            system->flags |= _particle_system_emitting_bit;

            halo::objects::object_sample_ambient_lightmap_point(&system->position,
                (real_vector3d *)&system->ambient_color, &incident_scratch, 0);

            if (!halo::effects::particle_system_new_type_states(handle)) {
                halo::memory::datum_delete(particle_system_data, handle);
                return k_datum_index_none;
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
    datum_index handle = k_datum_index_none;

    if (particle_systems_enabled != 0) {
        handle = halo::memory::datum_new(particle_system_data);
        if (handle != k_datum_index_none) {
            object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
            Object *object_definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;
            ObjectAttachment *attachment = (ObjectAttachment *)object_definition->attachments.pointer + attachment_index;
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & halo::k_slot_mask];
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
                                               nullptr);
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
                return k_datum_index_none;
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
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & halo::k_slot_mask];
    ParticleSystem *definition =
        (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & halo::k_slot_mask].data;
    uint8_t all_types_ok = 1;
    uint8_t any_type_ok = 0;
    int32_t leaf_index;
    int32_t i;

    leaf_index = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &system->position);
    system->location.leaf_index = leaf_index;
    system->location.cluster_index = (leaf_index == -1) ? (int16_t)0xffff :
        halo::scenario::structure_leaf_cluster(leaf_index);
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
                state->first_particle = k_datum_index_none;

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
static void particle_build_state_sprite(ParticleSystemType *type, ParticleSystemTypeParticleState *state_definition,
    ParticleSystemTypeParticleState *current_state, int32_t frame, real_point3d *position, real_vector3d *direction,
    float rotation, float scale, ColorARGB *color, float weight)
{
    build_sprite_data data;
    uint32_t mode;
    uint8_t *shader_block = particle_state_shader_block(state_definition);

    data.bitmap_group_index = *(datum_index *)&state_definition->bitmaps.tag_id;
    data.maximum_sprite_count = 2;
    data.shader = (uint32_t)(uintptr_t)shader_block;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    if (type->complex_sprite_render_mode == 1) {
        mode = particle_type_has(type, particle_type_flag::rotational_sprites_animate_sideways) ? 3 : 1;
        halo::render::build_sprite_rotational(&data, mode, (int16_t)state_definition->sequence_index, (int16_t)frame, position,
            direction, rotation, scale, color, weight);
    } else {
        halo::render::build_sprite(&data, (int16_t)state_definition->sequence_index, (int16_t)frame, (int16_t)particle_type_sprite_parameter(type),
            position, direction, rotation, scale, color, weight, 1);
    }
    *(float *)(particle_state_shader_block(state_definition) + k_particle_shader_average_radius_offset) = current_state->radius_multiplier;
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
    particle_system *system = &((particle_system *)particle_system_data->data)[particle_system_handle & halo::k_slot_mask];
    ParticleSystem *definition = particle_system_definition(system);
    int16_t type_index;

    for (type_index = 0; type_index < (int32_t)definition->particle_types.count; type_index++) {
        ParticleSystemType *type = particle_system_type_at(definition, type_index);
        particle_system_type_state *type_state = &system->type_states[type_index];
        uint16_t particle_index;

        if (type_state->state_index == -1 || particle_type_has(type, particle_type_flag::disabled)) {
            continue;
        }
        for (particle_index = type_state->first_particle; particle_index != halo::k_word_none; ) {
            particle_system_particle *particle = &((particle_system_particle *)particle_system_particle_data->data)[particle_index];
            int16_t cluster = particle->location.cluster_index;

            if (particle->active && (halo::structures::globals().cluster_visible_bits[cluster >> 5] & (1u << (cluster & 0x1f)))) {
                ParticleSystemTypeParticleState *states = (ParticleSystemTypeParticleState *)type->particle_states.pointer;
                ParticleSystemTypeParticleState *current = &states[particle->state_index];
                ParticleSystemTypeParticleState *next = 0;
                real_point3d position;
                real_vector3d direction;
                float fraction = 1.0f;
                float inverse = 0.0f;
                float scale;
                float color[4];
                float drawn[4];
                Bitmap *bitmap;
                BitmapGroupSequence *sequence;
                int16_t sequence_index;
                int32_t frame;
                float vx = particle->direction.i;
                float vy = particle->direction.j;
                float vz = particle->direction.k;
                const real_matrix4x3 &view = halo::render::globals().camera_world_to_view;

                halo::math::matrix4x3_transform_point(position, particle->position, halo::render::globals().camera_world_to_view);
                direction.i = vx * view.forward.i + vy * view.left.i + vz * view.up.i;
                direction.j = vx * view.forward.j + vy * view.left.j + vz * view.up.j;
                direction.k = vx * view.forward.k + vy * view.left.k + vz * view.up.k;

                if (particle->next_state_index == -1) {
                    scale = particle->values.scale * type_state->scale;
                    color[0] = particle->values.color.alpha * type_state->color.alpha;
                    color[1] = particle->values.color.red * type_state->color.red;
                    color[2] = particle->values.color.green * type_state->color.green;
                    color[3] = particle->values.color.blue * type_state->color.blue;
                } else {
                    next = &states[particle->next_state_index];
                    fraction = particle_clamp01(particle->state_time_remaining / particle->state_duration);
                    inverse = 1.0f - fraction;
                    scale = (inverse * particle->next_values.scale + fraction * particle->values.scale) * type_state->scale;
                    color[0] = (inverse * particle->next_values.color.alpha + fraction * particle->values.color.alpha) * type_state->color.alpha;
                    color[1] = (inverse * particle->next_values.color.red + fraction * particle->values.color.red) * type_state->color.red;
                    color[2] = (inverse * particle->next_values.color.green + fraction * particle->values.color.green) * type_state->color.green;
                    color[3] = (inverse * particle->next_values.color.blue + fraction * particle->values.color.blue) * type_state->color.blue;
                    if (current->framebuffer_blend_function == next->framebuffer_blend_function &&
                        current->bitmap_flags == next->bitmap_flags &&
                        current->sequence_index == next->sequence_index) {
                        fraction = 1.0f;
                        inverse = 0.0f;
                    }
                }

                bitmap = (Bitmap *)halo::cache::globals().tag_instances[*(datum_index *)&current->bitmaps.tag_id & halo::k_slot_mask].data;
                sequence_index = (int16_t)current->sequence_index;
                if (type->complex_sprite_render_mode == 1) {
                    sequence_index++;
                }
                sequence = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence_index;
                if (particle->frame == -1.0f) {
                    int16_t count = (int16_t)sequence->sprites.count;
                    int16_t picked;

                    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
                    picked = (int16_t)(((uint32_t)count * (halo::math::globals().effect_random_seed >> 0x10)) >> 0x10);
                    particle->frame = (float)picked;
                    frame = picked;
                } else {
                    int32_t value = (int16_t)(int32_t)particle->frame;
                    int32_t remainder = value % (int32_t)sequence->sprites.count;

                    frame = remainder;
                    if ((int16_t)remainder < 0) {
                        frame = (int32_t)(((uint32_t)remainder & 0xffff0000u) |
                            (uint16_t)((int16_t)remainder + (int16_t)sequence->sprites.count));
                    }
                }

                if (fraction > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (current->framebuffer_blend_function == 0) {
                        drawn[1] *= system->ambient_color.red;
                        drawn[2] *= system->ambient_color.green;
                        drawn[3] *= system->ambient_color.blue;
                    }
                    particle_build_state_sprite(type, current, current, frame, &position, &direction,
                        particle->rotation, scale, (ColorARGB *)drawn, fraction);
                }
                if (inverse > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (current->framebuffer_blend_function == 0) {
                        drawn[1] *= system->ambient_color.red;
                        drawn[2] *= system->ambient_color.green;
                        drawn[3] *= system->ambient_color.blue;
                    }
                    position.z += 0.001f;
                    particle_build_state_sprite(type, next, current, frame, &position, &direction,
                        particle->rotation, scale, (ColorARGB *)drawn, inverse);
                }
            }
            particle_index = (uint16_t)particle->next_particle;
        }
    }
}

/**
 * File-local helper used by particle_system_resolve_local_players.
 */
static int16_t particle_leaf_cluster(uint32_t leaf)
{
    if (leaf == halo::k_dword_none) {
        return -1;
    }
    return halo::scenario::structure_leaf_cluster(leaf);
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
        particle_system *system = &((particle_system *)particle_system_data->data)[handle & halo::k_slot_mask];
        ParticleSystem *definition = particle_system_definition(system);
        int32_t type_index;

        if (system->object_index != k_datum_index_none) {
            halo::objects::object_get_root_location((int32_t *)&system->location, system->object_index);
        } else {
            uint32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &system->position);
            int16_t cluster = particle_leaf_cluster(leaf);

            *(uint32_t *)&system->location.leaf_index = leaf;
            system->location.cluster_index = cluster;
            if (cluster == -1) {
                halo::effects::particle_system_delete(handle);
                continue;
            }
        }
        for (type_index = 0; type_index < (int32_t)definition->particle_types.count; type_index++) {
            datum_index *link = &system->type_states[type_index].first_particle;

            while (*link != k_datum_index_none) {
                particle_system_particle *particle = &((particle_system_particle *)particle_system_particle_data->data)[*link & halo::k_slot_mask];
                uint32_t leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &particle->position);
                int16_t cluster = particle_leaf_cluster(leaf);

                *(uint32_t *)&particle->location.leaf_index = leaf;
                particle->location.cluster_index = cluster;
                if (cluster == -1) {
                    datum_index doomed = *link;

                    halo::memory::datum_delete(particle_system_particle_data, doomed);
                    *link = particle->next_particle;
                } else {
                    link = &particle->next_particle;
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
    color_fraction_bits = (float)(halo::math::globals().effect_random_seed >> k_random_value_shift) * halo::k_unit_word_scale;

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
    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
    return (real)(halo::math::globals().effect_random_seed >> 0x10) * halo::k_unit_word_scale;
}

/**
 * Member form of the original particle_system_spawn: spawn.
 *
 * @address 0x453b10
 */
void particle_system_view::spawn(int32_t type_index, float dt)
{
    particle_system *system = record;
    particle_system_type_state *type_state = &system->type_states[(int16_t)type_index];
    ParticleSystemType *type = particle_system_type_at(system, (int16_t)type_index);
    bool initial = (system->flags & _particle_system_in_update_bit) != 0;
    ParticleSystemTypeStates *state = initial ? 0 : &((ParticleSystemTypeStates *)type->states.pointer)[type_state->state_index];
    datum_index object_index = system->object_index;
    object_marker markers[8];
    int16_t locality;
    int16_t target;
    int16_t marker_count;
    int16_t spawned;

    locality = (int16_t)halo::effects::player_weapon_locality_for_object(object_index);
    if (locality != 0) {
        if (particle_type_has(type, particle_type_flag::do_not_draw_in_third_person)) {
            if (locality == -1 || !halo::render::render_local_player_gunner_seat_visible(halo::interface::globals().current_local_player_index)) {
                return;
            }
        }
        if (particle_type_has(type, particle_type_flag::do_not_draw_in_first_person) && locality == 1 && halo::render::render_local_player_gunner_seat_visible(halo::interface::globals().current_local_player_index)) {
            return;
        }
    }

    if (initial) {
        if (particle_type_has(type, particle_type_flag::initial_count_scales_with_effect)) {
            target = (int16_t)(int32_t)((double)(int16_t)type->initial_particle_count * system->scale + 0.5);
        } else {
            target = (int16_t)type->initial_particle_count;
        }
    } else {
        double amount = (double)dt * type_state->particle_creation_rate;
        int32_t whole = (int32_t)amount;
        double accumulated = amount - (double)whole + type_state->creation_fraction;

        target = (int16_t)(type_state->particle_count + whole);
        type_state->creation_fraction = (float)accumulated;
        if (accumulated > 1.0) {
            target = (int16_t)(target + 1);
            type_state->creation_fraction = (float)(accumulated - 1.0);
        }
    }
    if (particle_systems_enabled == 1) {
        target = (int16_t)(int32_t)((double)target * 0.5);
    }
    if (type_state->particle_count < target) {
        if (object_index != k_datum_index_none) {
            object *owner = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
            Object *object_tag = (Object *)halo::cache::globals().tag_instances[owner->definition_tag & halo::k_slot_mask].data;
            char *marker_name = (char *)((ObjectAttachment *)object_tag->attachments.pointer + system->attachment_index)->marker.string;

            marker_count = (int16_t)halo::objects::object_get_node_local_transform(object_index, marker_name, markers, 8);
            halo::objects::object_get_root_location((int32_t *)&system->location, object_index);
            if (marker_count == 0) {
                datum_index weapon = first_person_weapon_interfaces[halo::interface::globals().current_local_player_index].weapon_index;

                if (weapon != k_datum_index_none) {
                    marker_count = (int16_t)halo::interface::first_person_weapon_get_marker_data(weapon, marker_name, markers, 8);
                    halo::objects::object_get_root_location((int32_t *)&system->location, weapon);
                }
            }
        } else {
            markers[0].node_transform.position = system->position;
            markers[0].node_transform.forward = *global_origin3d_pointer;
            marker_count = 1;
        }

        if (system->location.cluster_index != -1 && type_state->particle_count < target) {
            for (spawned = 0; marker_count != 0 && spawned < 0x80; ) {
                datum_index handle = halo::memory::datum_new(particle_system_particle_data);
                particle_system_particle *particle;
                int16_t physics;
                int16_t marker_index;

                if (handle == k_datum_index_none) {
                    break;
                }
                particle = &((particle_system_particle *)particle_system_particle_data->data)[handle & halo::k_slot_mask];
                physics = initial ? (int16_t)type->particle_creation_physics : (int16_t)state->particle_creation_physics;
                particle->state_index = -1;
                particle->next_state_index = -1;
                particle->active = 1;
                particle->ping_pong_forward = 1;
                particle->frame = -1.0f;
                particle->rotation = particle_roll() * 6.2831855f;
                halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
                marker_index = (int16_t)(((halo::math::globals().effect_random_seed >> 0x10) * (uint32_t)(int32_t)marker_count) >> 0x10);
                particle_creation_physics_table[physics](system, type_index, particle, &markers[marker_index]);
                halo::scenario::location_view(&particle->location).from_point(&particle->position);
                if (particle->location.cluster_index != -1) {
                    type_state->particle_count += 1;
                    particle->next_particle = type_state->first_particle;
                    type_state->first_particle = handle;
                } else {
                    halo::memory::datum_delete(particle_system_particle_data, handle);
                }
                spawned++;
                if (type_state->particle_count >= target) {
                    break;
                }
            }
        }
    }
    if ((float)type_state->particle_count < type_state->minimum_particle_count) {
        type_state->state_time_remaining = type_state->state_time_remaining * 0.3f;
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

    definition_tag = (ParticleSystem *)halo::cache::globals().tag_instances[system->definition_index & halo::k_slot_mask].data;
    point_physics_tag_id = *(uint32_t *)&definition_tag->point_physics.tag_id;
    if (point_physics_tag_id == (uint32_t)-1) {
        return;
    }

    halo::physics::point_physics_tick(&system->velocity, 0,
        (PointPhysics *)halo::cache::globals().tag_instances[point_physics_tag_id & halo::k_slot_mask].data,
        &system->location, (uint32_t)-1, &system->position, nullptr,
        nullptr, nullptr, 1.0f, dt);
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

    if (systems != nullptr && systems->valid != 0) {
        datum_index handle = halo::memory::datum_next(-1, systems);

        while (handle != k_datum_index_none) {
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
            uint32_t *visible_clusters = &halo::game::globals().local_player_globals->cluster_pvs[0x10];

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

    while (handle != k_datum_index_none) {
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
        ParticleSystemType *particle_type = particle_system_type_at(system, type_index);
        particle_system_type_state *type_state = &system->type_states[type_index];
        ParticleSystemTypeParticleState *states = (ParticleSystemTypeParticleState *)particle_type->particle_states.pointer;
        ParticleSystemTypeParticleState *state = &states[particle->state_index];
        real radius;
        PointPhysics *physics;
        PointPhysics blended;
        uint32_t collision_flags;

        if (particle->next_state_index == -1) {
            physics = (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & halo::k_slot_mask].data;
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
                (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & halo::k_slot_mask].data,
                (PointPhysics *)halo::cache::globals().tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)next_state)->point_physics.tag_id & halo::k_slot_mask].data,
                fraction);
            physics = &blended;
        }

        collision_flags = halo::physics::point_physics_tick((real_vector3d *)&particle->velocity, 0, physics,
            &particle->location, (uint32_t)-1, &particle->position, nullptr,
            nullptr, nullptr, radius, dt);

        if (((collision_flags & 1) != 0 && particle_type_has(particle_type, particle_type_flag::particles_die_in_air)) ||
            ((collision_flags & 2) != 0 && particle_type_has(particle_type, particle_type_flag::particles_die_in_water)) ||
            ((collision_flags & 4) != 0 && particle_type_has(particle_type, particle_type_flag::particles_die_on_ground))) {
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
