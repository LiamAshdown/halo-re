/**
 * @file src/sound/spatial.cpp
 * Listener, environment and range handling for spatialized sounds.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"

namespace halo::sound {

namespace spatial {

void environment_update(uint32_t *out_environment_ptr, void **out_environment_slot, uint8_t *out_changed)
{
    ScenarioStructureBSP *structure_bsp = global_structure_bsp;
    uint32_t sound_tag_id = 0xffffffff;
    uint32_t environment_default = 0xffffffff;
    uint8_t is_water = 0;
    ScenarioStructureBSPCluster *cluster_record;
    int16_t fog_id;

    if (*(int32_t *)&local_player_globals->local_players  == -1 || local_player_0_cluster_index == -1) {
        goto skip_environment_lookup;
    }

    cluster_record = (ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer + local_player_0_cluster_index;

    {
        int16_t region = scenario_location_fog_region(&camera_leaf, &camera_point);
        if (region != -1) {
            region = (int16_t)((ScenarioStructureBSPFogRegion *)structure_bsp->fog_regions.pointer)[region].fog;
        }
        if (region == -1) {
            fog_id = -0x8000;
        } else {
            uint32_t fog_tag_id = *(uint32_t *)&((ScenarioStructureBSPFogPalette *)structure_bsp->fog_palette.pointer)[region].fog.tag_id;
            if (fog_tag_id == 0xffffffff) {
                fog_id = -0x8000;
            } else {
                Fog *fog_tag = (Fog *)tag_instances[fog_tag_id & 0xffff].data;
                uint32_t env_tag = *(uint32_t *)&fog_tag->sound_environment.tag_id;
                if (env_tag == 0xffffffff) {
                    fog_id = -0x8000;
                } else {
                    SoundEnvironment *env_tag_data = (SoundEnvironment *)tag_instances[env_tag & 0xffff].data;
                    if (env_tag_data->priority < -0x7fff) {
                        fog_id = -0x8000;
                    } else {
                        fog_id = env_tag_data->priority;
                        environment_default = *(uint32_t *)&fog_tag->background_sound.tag_id;
                        is_water = *(uint8_t *)&fog_tag->flags & 1;
                        sound_tag_id = env_tag;
                    }
                }
            }
        }
    }

    {
        int16_t sound_environment_index = (int16_t)cluster_record->sound_environment;
        if (sound_environment_index != -1) {
            uint32_t override_tag = *(uint32_t *)&((ScenarioStructureBSPSoundEnvironmentPalette *)structure_bsp->sound_environment_palette.pointer)[sound_environment_index].sound_environment.tag_id;
            if (override_tag != 0xffffffff) {
                SoundEnvironment *override_data = (SoundEnvironment *)tag_instances[override_tag & 0xffff].data;
                if (fog_id < override_data->priority) {
                    int16_t background_sound_index = (int16_t)cluster_record->background_sound;
                    is_water = 0;
                    sound_tag_id = override_tag;
                    if (background_sound_index == -1 || background_sound_index >= (int32_t)structure_bsp->background_sound_palette.count) {
                        environment_default = 0xffffffff;
                    } else {
                        environment_default = *(uint32_t *)&((ScenarioStructureBSPBackgroundSoundPalette *)structure_bsp->background_sound_palette.pointer)[background_sound_index].background_sound.tag_id;
                    }
                }
            }
        }
    }

skip_environment_lookup:
    {
        uint32_t *source;
        SoundEnvironment *dest = &global_scenario_game_globals->sound_environment;

        if (sound_tag_id == 0xffffffff) {
            source = (uint32_t *)&k_default_sound_environment;
        } else {
            source = (uint32_t *)tag_instances[sound_tag_id & 0xffff].data;
        }

        if (is_water == global_scenario_game_globals->sound_environment_is_water) {
            static const float k_clamp[12] = {
                0.03f, 0.03f, 0.3f, 0.1f, 0.03f, 0.03f, 0.09f, 0.03f, 0.003f, 0.03f, 0.03f, 600.0f
            };
            float *dest_f = (float *)&dest->room_intensity;
            const float *source_f = (const float *)((const uint8_t *)source + 8);
            int32_t i;

            for (i = 0; i < 12; i++) {
                float delta = source_f[i] - dest_f[i];
                float clamp = k_clamp[i];
                if (delta < -clamp) delta = -clamp;
                else if (delta > clamp) delta = clamp;
                dest_f[i] = dest_f[i] + delta;
            }
            *out_changed = 0;
        } else {
            uint32_t *dest_words = (uint32_t *)dest;
            int32_t i;
            for (i = 0; i < 0x12; i++) {
                dest_words[i] = source[i];
            }
            global_scenario_game_globals->sound_environment_is_water = is_water;
            *out_changed = 1;
        }

        *out_environment_ptr = environment_default;
        *out_environment_slot = (void *)dest;
    }
}

void build_cluster_range_bitmap(void)
{
    int32_t cluster_count = (int32_t)global_structure_bsp->clusters.count;
    uint8_t *distance_table = (uint8_t *)global_structure_bsp->sound_pas_data.pointer;
    int32_t word_count = (cluster_count + 0x1f) >> 5;
    int32_t i;

    for (i = 0; i < word_count; i++) {
        sound_cluster_audible_bitmap[i] = 0;
    }

    if (local_player_globals->local_players[0] != (datum_index)k_datum_index_none &&
        observers[0].camera.cluster_index != -1 && cluster_count > 0) {
        int16_t listener_cluster = observers[0].camera.cluster_index;
        int32_t cluster;

        for (cluster = 0; cluster < cluster_count; cluster++) {
            uint8_t distance;

            if (cluster == listener_cluster) {
                distance = 0;
            } else {
                int16_t high = cluster;
                int16_t low = listener_cluster;
                int32_t index;

                if (cluster < listener_cluster) {
                    high = listener_cluster;
                    low = cluster;
                }

                index = (int16_t)((uint16_t)(cluster_count - 1) * low - ((low + 1) * (int32_t)low) / 2 - 1 + high);
                distance = distance_table[index];
            }

            if ((float)(distance & 0x7f) * 2.015748f < 256.0f) {
                sound_cluster_audible_bitmap[cluster >> 5] |= 1u << (cluster & 0x1f);
            }
        }
    }
}

void refresh_structure_locations(void)
{
    datum_index handle;

    if (!sound_initialized || !sound_enabled || sound_disabled) {
        return;
    }
    for (handle = halo::memory::datum_next(-1, sound_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, sound_data)) {
        sound *entry = (sound *)sound_data->data + (handle & 0xffff);
        uint32_t leaf;

        if (entry->location.type != 1) {
            continue;
        }
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&entry->location.position);
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            entry->location.cluster_index = -1;
        } else {
            entry->location.cluster_index = (int16_t)
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
        }
    }
}

void update_listener(void)
{
    uint8_t underwater;
    observer_camera *camera;
    sound_listener *listener;
    sound_listener_parameters params;
    datum_index water_sound_tag;

    listener = &sound_listeners[0];

    if (!game_time->initialized) {
        return;
    }
    if (!game_time->active && !game_time->paused) {
        return;
    }

    if (local_player_globals->local_players[0] == 0xffffffff) {
        listener->valid = 0;
        goto push_listener_parameters;
    }
    listener->valid = 1;

    camera = &observers[0].camera;
    underwater = scenario_location_get_water_and_weather((real_point3d *)&camera->position,
        (bsp_leaf_reference *)&camera->leaf_index, (int16_t *)0);
    if (listener->underwater != underwater) {
        sound_location location = { 0 };
        location.type = _sound_location_none;
        location.scale = 1.0f;
        location.gain = 1.0f;

        if (underwater) {
            if (0 < global_globals->sounds.count) {
                water_sound_tag = *(datum_index *)&((GlobalsSound *)global_globals->sounds.pointer)[0].sound.tag_id;
                if (water_sound_tag != k_datum_index_none) {
                    instances::play_new(water_sound_tag, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 1);
                }
            }
        } else {
            if (1 < global_globals->sounds.count) {
                water_sound_tag = *(datum_index *)&((GlobalsSound *)global_globals->sounds.pointer)[1].sound.tag_id;
                if (water_sound_tag != k_datum_index_none) {
                    instances::play_new(water_sound_tag, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 1);
                }
            }
        }
    }
    listener->underwater = underwater;

    halo::math::matrix4x3_from_forward_up(*((real_vector3d *)&camera->up), *((real_vector3d *)&camera->forward),
        *((real_matrix4x3 *)&listener->scale));

    listener->position = camera->position;

    halo::math::matrix4x3_inverse_transform_vector(*((real_vector3d *)&listener->velocity), *((real_vector3d *)&camera->velocity),
        *((real_matrix4x3 *)&listener->scale));

push_listener_parameters:
    params.position = *(Point3D *)global_zero_vector3d_pointer;
    params.forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
    params.up = *(Vector3D *)halo::math::globals().global_up3d_pointer;
    params.velocity = *(Vector3D *)global_origin3d_pointer;
    params.environment = &sound_environment;
    audio_device().set_listener(&params);
}

float location_distance_squared(int16_t listener_index, sound_location *location)
{
    sound_listener *listener;

    if (location->type == _sound_location_none) {
        return 0.0f;
    }
    if (location->type == _sound_location_listener_relative) {
        return location->position.z * location->position.z + location->position.y * location->position.y +
               location->position.x * location->position.x;
    }
    if (location->type != _sound_location_absolute) {
        { union { sound_location *p; float f; } bits; bits.p = location; return bits.f; }
    }

    listener = &sound_listeners[listener_index];
    return (listener->position.y - location->position.y) * (listener->position.y - location->position.y) +
           (listener->position.x - location->position.x) * (listener->position.x - location->position.x) +
           (listener->position.z - location->position.z) * (listener->position.z - location->position.z);
}

float location_distance(int16_t listener_index, sound_location *location)
{
    sound_listener *listener;

    if (location->type == _sound_location_none) {
        return 0.0f;
    }
    if (location->type == _sound_location_listener_relative) {
        return sqrt(location->position.z * location->position.z + location->position.y * location->position.y +
                      location->position.x * location->position.x);
    }
    if (location->type != _sound_location_absolute) {
        { union { sound_location *p; float f; } bits; bits.p = location; return bits.f; }
    }

    listener = &sound_listeners[listener_index];
    return sqrt((listener->position.y - location->position.y) * (listener->position.y - location->position.y) +
                 (listener->position.x - location->position.x) * (listener->position.x - location->position.x) +
                 (listener->position.z - location->position.z) * (listener->position.z - location->position.z));
}

void update_range_and_ducking(void)
{
    uint8_t no_player_has_a_unit;
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    float max_distance;
    int16_t listener_index;
    uint8_t saw_dialog_class;
    float step, delta, clamped_step;

    no_player_has_a_unit = local_player_globals->no_player_has_a_unit;
    saw_dialog_class = 0;

    sound_handle = halo::memory::datum_next(-1, sound_data);
    while (sound_handle != 0xffffffff) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

        if ((instance->channel_index != -1 && channels::release_detail_buffers(instance->channel_index) == 0 &&
             instance->play_state != _sound_play_loop && instance->play_state != _sound_play_loop_stopping) ||
            (instances::invoke_location_proc(sound_handle) == 0 && !sound_paused)) {
            instances::stop(sound_handle);
            goto next_sound;
        }

        max_distance = definition->maximum_distance;
        if (max_distance == 0.0f) {
            max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
        }
        listener_index = view(&instance->location)->check_audibility(max_distance);
        instances::render_debug(sound_handle);

        if (definition->sound_class == soundclass_scripted_dialog_player ||
            definition->sound_class == soundclass_scripted_dialog_other ||
            definition->sound_class == soundclass_scripted_dialog_force_unspatialized) {
            saw_dialog_class = 1;
        }

        if (listener_index == -1) {
            if (!(instance->flags & _sound_out_of_range_bit)) {
                instances::schedule_gain_fade(0xffffffff, 0, 2.0f, sound_handle);
                instance->flags |= _sound_out_of_range_bit;
            }
        } else {
            instance->listener_index = listener_index;
            if (instance->flags & _sound_out_of_range_bit) {
                instances::schedule_gain_fade(sound_handle, 0, 0.5f, 0xffffffff);
                instance->flags &= ~_sound_out_of_range_bit;
            }
        }

        if (no_player_has_a_unit) {
            if (definition->sound_class == soundclass_scripted_dialog_player) {
                if (instance->channel_index == -1) {
                    instances::stop(sound_handle);
                    goto next_sound;
                }
                instances::schedule_gain_fade(0xffffffff, 0, 0.3f, sound_handle);
            } else if (definition->sound_class == soundclass_scripted_dialog_other && instance->channel_index == -1) {
                instances::stop(sound_handle);
                goto next_sound;
            }
        }

    next_sound:
        sound_handle = halo::memory::datum_next((int16_t)sound_handle, sound_data);
    }

    if (saw_dialog_class) {
        step = sound_time_delta * 0.03f;
        delta = sound_dialog_ducking_gain - sound_ducking_gain;
    } else {
        step = sound_time_delta * 0.007f;
        delta = 1.0f - sound_ducking_gain;
    }
    clamped_step = -step;
    if (-step <= delta) {
        clamped_step = step;
        if (delta <= step) {
            sound_ducking_gain = delta + sound_ducking_gain;
            return;
        }
    }
    sound_ducking_gain = clamped_step + sound_ducking_gain;
}

}  // namespace spatial

void Location::compute_obstruction_occlusion(int16_t listener_index, float reference_distance)
{
    observer_camera *listener = (observer_camera *)0;
    int16_t listener_cluster;
    float distance;

    if (listener_index != -1) {
        listener = &observers[listener_index].camera;
    }

    this->obstruction = 0.6f;
    this->occlusion = 1.0f;
    if (this->cluster_index == -1) {
        return;
    }

    listener_cluster = listener->cluster_index;
    if (listener_cluster == -1) {
        return;
    }

    distance = (float)(cluster_sound_distance_lookup(this->cluster_index, listener_cluster,
                                              (ScenarioStructureBSP *)global_structure_bsp ) & 0x7f) * 2.015748f;
    if (!(distance < 256.0f)) {
        return;
    }

    {
        int32_t cluster_count = (int32_t)global_structure_bsp->clusters.count;
        uint32_t *occlusion_bitmap = (uint32_t *)global_structure_bsp->cluster_data.pointer;
        int32_t word_index = ((cluster_count + 0x1f) >> 5) * listener_cluster + (this->cluster_index >> 5);

        if ((occlusion_bitmap[word_index] & (1u << (this->cluster_index & 0x1f))) != 0) {
            real_vector3d delta;
            collision_result result;

            this->obstruction = 0.45f;
            delta.i = this->position.x - listener->position.x;
            delta.j = this->position.y - listener->position.y;
            delta.k = this->position.z - listener->position.z;

            if (collision_test_movement_segment(0xc0e1, (real_point3d *)&listener->position, &delta, 0xffffffff,
                    &result) == 0) {
                this->obstruction = 0.0f;
                this->occlusion = 0.0f;
            }
        }
    }

    if (this->obstruction != 0.0f) {
        float occlusion = 1.0f - reference_distance / (distance + reference_distance);

        this->occlusion = occlusion;
        occlusion = occlusion * 1.4f;
        if (occlusion < 0.0f) {
            this->occlusion = 0.0f;
            return;
        }
        if (occlusion > 1.0f) {
            occlusion = 1.0f;
        }
        this->occlusion = occlusion;
    }
}

int16_t Location::check_audibility(float max_distance)
{
    float nearest_distance_squared;
    int16_t listener_index;

    listener_index = -1;

    if (this->type == _sound_location_none) {
        return 0;
    }

    if (this->type == _sound_location_listener_relative) {
        if (spatial::location_distance_squared(-1, this) < max_distance) {
            return 0;
        }
    } else {
        nearest_distance_squared = 3.4028235e+38f;
        if (sound_listeners[0].valid) {
            float distance_squared = spatial::location_distance_squared(0, this);
            if (distance_squared < 3.4028235e+38f) {
                nearest_distance_squared = distance_squared;
                listener_index = 0;
            }
        }
        if (listener_index != -1) {
            view(this)->compute_obstruction_occlusion(listener_index, (float)sqrt(nearest_distance_squared));
        }
        if (max_distance * max_distance < nearest_distance_squared || *(uint32_t *)&this->occlusion == 0x3f800000) {
            return -1;
        }
    }

    return listener_index;
}


}  // namespace halo::sound
