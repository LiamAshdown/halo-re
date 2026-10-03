/**
 * @file src/sound/sound_instances.cpp
 * Playing sound instances: starting, stopping, fading and per-update gain.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"

namespace halo::sound {

namespace {

/** Current performance-counter time in milliseconds. */
int32_t sound_fade_now_ms(void)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
}

/** Releases the unused cached sample pages of a sound tag. */
void sound_release_unused_pages(TagID tag_id)
{
    Sound *definition;
    int32_t range_index, permutation_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;

    definition = (Sound *)halo::cache::globals().tag_instances[tag_id.index].data;
    for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++) {
        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + range_index;
        for (permutation_index = 0; permutation_index < (int32_t)pitch_range->permutations.count;
             permutation_index++) {
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count == 0) {
                    halo::memory::cache_evict_entry((datum_index)permutation->samples_pointer, halo::cache::globals().sound_cache);
                    permutation->samples_pointer = 0xffffffff;
                    permutation->cache_page = 0;
                }
            }
        }
    }
}

/** Returns the object record of an object index. */
uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

}  // namespace

namespace instances {

datum_index start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint)
{
    sound_object_marker_data marker;
    sound_location location;

    marker.position = *position;
    marker.forward = *forward;
    marker.node_index = node_index;
    location.type = _sound_location_absolute;
    location.gain = 1.0f;
    location.cluster_index = -1;

    if (instances::object_marker_location_proc(object_index, &marker, &location) == 0) {
        return k_datum_index_none;
    }

    location.scale = scale;
    return instances::play_new(definition_index, &location, object_index, instances::object_marker_location_proc, &marker,
        sizeof(sound_object_marker_data), first_person_hint);
}

datum_index start_at_location(datum_index definition_index, sound_placement *placement, float scale)
{
    sound_location location;

    location.position = placement->position;
    location.forward = placement->forward;
    location.velocity = placement->velocity;
    location.leaf_index = placement->leaf_index;
    location.cluster_index = placement->cluster_index;
    location.unknown_36 = placement->unknown_2a;
    location.obstruction = 0.0f;
    location.occlusion = 0.0f;
    location.type = _sound_location_absolute;
    location.scale = scale;
    location.gain = 1.0f;

    return instances::play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 0);
}

datum_index start_unspatialized(datum_index definition_index, float scale)
{
    sound_location location = { 0 };

    location.type = _sound_location_none;
    location.scale = scale;
    location.gain = 1.0f;

    return instances::play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 0);
}

void impulse_start(datum_index object_index, datum_index definition_index, float scale)
{
    Sound *tag;
    datum_index new_sound;

    if (definition_index == k_datum_index_none) {
        return;
    }

    tag = (Sound *)halo::cache::globals().tag_instances[definition_index & 0xffff].data;
    instances::impulse_fade_out(*(datum_index *)&tag->scripting_sound);
    tag->scripting_time = ((int32_t)tag->longest_permutation_length * 30) / 1000 + game_time->game_time;

    if (scale < 0.0f) {
        scale = 0.0f;
    } else if (scale > 1.0f) {
        scale = 1.0f;
    }

    if (object_index == k_datum_index_none) {
        sound_location location;

        location.type = _sound_location_none;
        location.scale = scale;
        location.gain = 1.0f;
        new_sound = instances::play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0,
            (void *)0, 0, 0);
    } else {
        object_marker marker;
        Point3D position;
        Vector3D forward;
        int16_t node_index;

        if ((int16_t)object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1) != 0) {
            position = *(Point3D *)&marker.transform.position;
            forward = *(Vector3D *)&marker.transform.forward;
            node_index = marker.node_index;
        } else {
            position = *(Point3D *)global_origin3d_pointer;
            forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
            node_index = 0;
        }

        new_sound = instances::start_at_object_marker(object_index, &position, &forward, definition_index, node_index,
            scale, 0);
        if (new_sound != k_datum_index_none) {
            object_type_definitions_notify_0x58(object_index, definition_index, new_sound);
        }
    }

    *(datum_index *)&tag->scripting_sound = new_sound;
}

int32_t impulse_time(datum_index sound_tag_handle)
{
    int32_t remaining = 0;

    if (sound_tag_handle != k_datum_index_none) {
        Sound *tag = (Sound *)halo::cache::globals().tag_instances[sound_tag_handle & 0xffff].data;

        if ((int32_t)tag->scripting_time != -1) {
            remaining = (int32_t)tag->scripting_time - game_time->game_time;
            if (remaining < 1) {
                remaining = 0;
            }
        }
    }

    return remaining;
}

uint8_t object_marker_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    sound_object_marker_data *marker = (sound_object_marker_data *)callback_data;
    int32_t root_location[2];
    int16_t node_index;
    object *obj;
    real_matrix4x3 *node_matrix;

    if (object_try_and_get(owner, 0xffffffff) == 0) {
        return 0;
    }

    object_get_root_location(root_location, owner);
    if ((int16_t)root_location[1] == -1) {
        return 0;
    }

    node_index = marker->node_index == -1 ? 0 : marker->node_index;
    obj = ((object_header *)object_data->data)[owner & 0xffff].data;
    node_matrix = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);

    location->leaf_index = root_location[0];
    *(int32_t *)&location->cluster_index = root_location[1];
    halo::math::matrix4x3_transform_point(*((real_point3d *)&location->position), *((real_point3d *)&marker->position), *node_matrix);
    halo::math::matrix4x3_transform_normal(*((real_vector3d *)&location->forward), *((real_vector3d *)&marker->forward), *node_matrix);
    object_get_root_object_velocities(owner, (real_vector3d *)&location->velocity, (real_vector3d *)0);
    return 1;
}

void fade_out_and_stop_all(void)
{
    if (sound_paused == 0) {
        float deadline;
        datum_index index;

        if (sound_initialized == 0 || sound_enabled == 0 || sound_disabled != 0) {
            goto stop;
        }

        deadline = (float)halo::cseries::time_query_performance_counter_ms();
        index = halo::memory::datum_next(-1, sound_data);
        if (index != k_datum_index_none) {
            do {
                instances::schedule_gain_fade(k_datum_index_none, _sound_fade_linear, 0.3f, index);
                index = halo::memory::datum_next((int16_t)index, sound_data);
            } while (index != k_datum_index_none);

            deadline += 300.0f;
            for (;;) {
                int32_t now = sound_fade_now_ms();
                float now_unsigned = (float)now;

                if (now < 0) {
                    now_unsigned += 4294967296.0f;
                }
                if (!(now_unsigned < deadline)) {
                    break;
                }
                engine::idle_update();
            }
        }

        if (sound_paused == 0) {
            goto stop;
        }
    }

    sound_paused = 0;
    if (current_sound_driver != 0) {
        audio_device().set_paused(0);
    }
    sound_time = sound_fade_now_ms();

stop:
    instances::stop_all();
    if (looping_sound_data != 0 && looping_sound_data->valid != 0) {
        halo::memory::data_delete_all(looping_sound_data);
    }
}

datum_index play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint)
{
    Sound *tag = (Sound *)halo::cache::globals().tag_instances[definition_index & 0xffff].data;
    datum_index handle = k_datum_index_none;

    if (tag->sound_class == soundclass_scripted_dialog_player ||
        tag->sound_class == soundclass_scripted_dialog_other ||
        tag->sound_class == soundclass_scripted_dialog_force_unspatialized) {
        int32_t suppress_until = ((int32_t)tag->longest_permutation_length * 30) / 1000 + 10 + game_time->game_time;
        if (ai_communication_quiet_until_tick < suppress_until) {
            ai_communication_quiet_until_tick = suppress_until;
        }
        if (sound_dialog_unspatialized != 0) {
            location->type = _sound_location_none;
        }
    }
    if (tag->sound_class == soundclass_scripted_dialog_force_unspatialized) {
        location->type = _sound_location_none;
    }

    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        if ((tag->format != soundformat_xbox_adpcm && tag->format != soundformat_ogg_vorbis &&
             tag->format != soundformat_16_bit_pcm) ||
            ((tag->channel_count != 0 || tag->sample_rate != 0) && tag->channel_count != 1)) {
            return k_datum_index_none;
        }

        if ((location->scale != 0.0f || tag->zero_gain_modifier != 0.0f)) {
            halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
            if (((tag->one_skip_fraction_modifier - tag->zero_skip_fraction_modifier) * location->scale +
                 tag->zero_skip_fraction_modifier) * tag->skip_fraction <
                (float)(halo::math::globals().effect_random_seed >> 16) * 1.5259022e-05f) {
                float maximum_distance = definitions::maximum_distance(definition_index);

                if (definitions::has_audible_permutations(*(TagID *)&definition_index) != 0) {
                    int16_t bucket = view(location)->check_audibility(maximum_distance);

                    if (bucket != -1) {
                        int16_t retrigger = definitions::check_promotion(*(TagID *)&definition_index);

                        if (retrigger != 0) {
                            if (retrigger != 1) {
                                return k_datum_index_none;
                            }
                            return instances::play_new(*(uint32_t *)&tag->promotion_sound.tag_id, location,
                                owner_index, location_proc, callback_data, callback_data_size, first_person_hint);
                        }

                        handle = halo::memory::datum_new(sound_data);
                        if (handle != k_datum_index_none) {
                            sound *self = &((sound *)sound_data->data)[(uint16_t)handle];
                            int32_t delay = (int32_t)(spatial::location_distance(bucket, location) * sound_delay_per_world_unit);

                            self->first_person = 0;
                            if (((uint8_t)first_person_hint != 0 &&
                                 (tag->sound_class == soundclass_weapon_fire ||
                                  (tag->sound_class == soundclass_weapon_ready &&
                                   cinematic_globals_ptr[10] == 0 && cinematic_globals_ptr[9] == 0) ||
                                  tag->sound_class == soundclass_weapon_reload ||
                                  tag->sound_class == soundclass_weapon_empty ||
                                  tag->sound_class == soundclass_weapon_charge ||
                                  tag->sound_class == soundclass_weapon_overheat ||
                                  tag->sound_class == soundclass_weapon_idle)) ||
                                (cinematic_globals_ptr[9] != 0 && location->type != 0 &&
                                 (tag->sound_class == soundclass_scripted_dialog_player ||
                                  tag->sound_class == soundclass_scripted_dialog_other))) {
                                self->first_person = 1;
                            }

                            self->definition_index = definition_index;
                            self->channel_index = -1;
                            self->listener_index = bucket;
                            self->play_state = _sound_play_impulse;

                            self->pitch = definitions::compute_random_pitch(tag->random_pitch_bounds[0], tag->random_pitch_bounds[1],
                                tag->zero_pitch_modifier, tag->one_pitch_modifier, location->scale);
                            self->flags = 0;
                            self->owner_index = owner_index;
                            self->location = *location;
                            self->location_proc = location_proc;

                            if (location_proc != 0) {
                                uint8_t *dst = self->callback_data;
                                uint8_t *src = (uint8_t *)callback_data;
                                int16_t count = (int16_t)callback_data_size;
                                int16_t i;
                                for (i = 0; i < count; i++) {
                                    dst[i] = src[i];
                                }
                            }

                            self->pitch_range_index = definitions::pick_pitch_range(-1, tag, self->pitch);
                            self->permutation_index = definitions::pick_permutation(self->pitch_range_index, -1, tag);
                            self->fade_end_time = 0;
                            self->fade_start_time = 0;
                            self->track_index = -1;

                            {
                                SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + self->pitch_range_index;
                                halo::cache::sound_cache_touch(1, 0, 0, (SoundPermutation *)range->permutations.pointer + self->permutation_index);
                            }

                            if (delay <= 250) {
                                self->start_time = sound_time;
                                return handle;
                            }
                            self->flags |= _sound_delayed_start_bit;
                            self->start_time = sound_time + delay;
                        }
                    }
                }
            }
        }
    }

    return handle;
}

void impulse_fade_out(datum_index sound_index)
{
    int16_t slot = (int16_t)sound_index;
    int16_t salt = (int16_t)(sound_index >> 16);
    int16_t identifier;

    if (sound_data == (data_array *)0 || sound_index == k_datum_index_none) {
        return;
    }
    if (slot < 0 || slot >= sound_data->maximum_count) {
        return;
    }
    identifier = *(int16_t *)((uint8_t *)sound_data->data + (int32_t)sound_data->size * slot);
    if (identifier == 0 || (salt != 0 && identifier != salt)) {
        return;
    }
    if (((sound *)sound_data->data)[sound_index & 0xffff].play_state == _sound_play_impulse) {
        instances::schedule_gain_fade(k_datum_index_none, _sound_fade_linear, 0.3f, sound_index);
    }
}

void stop_all(void)
{
    datum_index sound_handle;

    if (sound_initialized) {
        sound_stopping_all = 1;
        sound_handle = halo::memory::datum_next(-1, sound_data);
        while (sound_handle != k_datum_index_none) {
            instances::stop(sound_handle);
            sound_handle = halo::memory::datum_next((int16_t)sound_handle, sound_data);
        }
        halo::memory::data_delete_all(looping_sound_data);
        audio_device().stop_all();
    }
    ai_communication_quiet_until_tick = 0;
    sound_stopping_all = 0;
}

void schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds, datum_index fade_out_handle)
{
    int32_t fade_start_time;
    int32_t fade_end_time;
    sound *instance;

    fade_start_time = sound_time - 1;
    fade_end_time = (int32_t)(duration_seconds * sound_fade_duration_scale + (float)fade_start_time);
    if (fade_end_time <= sound_time) {
        fade_end_time = sound_time;
    }

    if (fade_in_handle != k_datum_index_none) {
        instance = (sound *)((uint8_t *)sound_data->data + (fade_in_handle & 0xffff) * sizeof(sound));
        if (instance->fade_start_time == instance->fade_end_time) {
            instance->fade_start_gain = 0.0f;
        } else {
            instance->fade_start_gain = instances::evaluate_fade_gain(fade_in_handle);
        }
        instance->fade_end_gain = 1.0f;
        instance->fade_curve = fade_curve;
        instance->fade_start_time = fade_start_time;
        instance->fade_end_time = fade_end_time;
    }

    if (fade_out_handle != k_datum_index_none) {
        instance = (sound *)((uint8_t *)sound_data->data + (fade_out_handle & 0xffff) * sizeof(sound));
        instance->fade_start_gain = instances::evaluate_fade_gain(fade_out_handle);
        instance->fade_end_gain = 0.0f;
        instance->fade_curve = fade_curve;
        instance->fade_start_time = fade_start_time;
        instance->fade_end_time = fade_end_time;
    }
}

void stop(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    int16_t channel_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;
    looping_sound *owner;
    uint8_t is_scripted_dialog_class;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    channel_index = instance->channel_index;
    definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & 0xffff].data;

    if (channel_index == -1) {
        if (instance->flags & _sound_channel_requested_bit) {
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;

            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }

            is_scripted_dialog_class = definition->sound_class == soundclass_scripted_dialog_player ||
                definition->sound_class == soundclass_scripted_effect ||
                definition->sound_class == soundclass_scripted_dialog_other ||
                definition->sound_class == soundclass_scripted_dialog_force_unspatialized;
            if (is_scripted_dialog_class) {
                if (permutation->samples_pointer != 0xffffffff) {
                    entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                        (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                    if (entry->lock_count == 0) {
                        halo::cache::sound_permutation_release_page(permutation);
                    }
                } else {
                    halo::cache::sound_permutation_release_page(permutation);
                }
            }
        }
    } else {
        sound_channels[channel_index].sound_index = 0xffffffff;
        channels::release_permutations(channel_index);
        instance->channel_index = -1;

        if (*(uint32_t *)&definition->scripting_sound == sound_handle ||
            (definition->sound_class != soundclass_scripted_dialog_player &&
             definition->sound_class != soundclass_scripted_effect &&
             definition->sound_class != soundclass_scripted_dialog_other &&
             definition->sound_class != soundclass_scripted_dialog_force_unspatialized)) {
            if (instance->play_state == _sound_play_loop_end && definition->sound_class == soundclass_music) {
                owner = (looping_sound *)((uint8_t *)looping_sound_data->data +
                    (instance->owner_index & 0xffff) * sizeof(looping_sound));
                {
                    SoundLooping *looping_definition =
                        (SoundLooping *)halo::cache::globals().tag_instances[owner->definition_index & 0xffff].data;
                    int32_t track_index;
                    SoundLoopingTrack *track;

                    for (track_index = 0; track_index < looping_definition->tracks.count; track_index++) {
                        track = (SoundLoopingTrack *)looping_definition->tracks.pointer + track_index;
                        if (track->start.tag_id.index != 0xffff || track->start.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->start.tag_id);
                        }
                        if (track->end.tag_id.index != 0xffff || track->end.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->end.tag_id);
                        }
                        if (track->alternate_end.tag_id.index != 0xffff || track->alternate_end.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->alternate_end.tag_id);
                        }
                        if (track->loop.tag_id.index != 0xffff || track->loop.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->loop.tag_id);
                        }
                        if (track->alternate_loop.tag_id.index != 0xffff || track->alternate_loop.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->alternate_loop.tag_id);
                        }
                    }
                }
            }
        } else {
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count != 0) {
                    goto skip_release;
                }
            }
            halo::cache::sound_permutation_release_page(permutation);
        }
    }
skip_release:

    if (instance->play_state != 0 && instance->owner_index != 0xffffffff) {
        owner = (looping_sound *)halo::memory::datum_get(instance->owner_index, looping_sound_data);
        if (owner != 0) {
            owner->active_sound_count -= 1;
            if (owner->track_sounds[instance->track_index] == sound_handle) {
                owner->track_sounds[instance->track_index] = 0xffffffff;
            }
        }
    }

    if (*(uint32_t *)&definition->scripting_sound == sound_handle) {
        is_scripted_dialog_class = definition->sound_class == soundclass_scripted_dialog_player ||
            definition->sound_class == soundclass_scripted_effect ||
            definition->sound_class == soundclass_scripted_dialog_other ||
            definition->sound_class == soundclass_scripted_dialog_force_unspatialized;
        *(uint32_t *)&definition->scripting_sound = 0xffffffff;
        if (is_scripted_dialog_class) {
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count != 0) {
                    goto delete_datum;
                }
                halo::memory::cache_evict_entry((datum_index)permutation->samples_pointer, halo::cache::globals().sound_cache);
            }
            permutation->samples_pointer = 0xffffffff;
            permutation->cache_page = 0;
        }
    }
delete_datum:
    halo::memory::datum_delete(sound_data, sound_handle);
}

uint32_t invoke_location_proc(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    uint8_t location_proc_result;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

    if (!(instance->flags & _sound_delayed_start_bit) && instance->location_proc != 0 &&
        instance->start_time < sound_time) {
        location_proc_result = instance->location_proc(instance->owner_index, instance->callback_data,
            &instance->location);
        if (location_proc_result == 0) {
            definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & 0xffff].data;
            if (instance->play_state != 0 || sound_class_definitions[definition->sound_class].dialog != 0) {
                return 0;
            }
            instance->location_proc = 0;
        }
    }
    return 1;
}

void update_gain(int16_t channel_index, float external_gain_multiplier)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    float zero_gain, class_gain, gain_factor;

    sound_handle = sound_channels[channel_index].sound_index;
    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & 0xffff].data;

    zero_gain = definition->zero_gain_modifier;
    class_gain = classes::compute_gain(definition->sound_class);
    gain_factor = (definition->one_gain_modifier - zero_gain) * instance->location.scale + zero_gain;
    gain_factor = gain_factor * class_gain * instance->location.gain * external_gain_multiplier;

    if (instance->channel_index == -1) {
        SoundPitchRange *pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        SoundPermutation *permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        sound_channel_parameters params;

        params.gain = permutation->gain * definition->random_gain_modifier * gain_factor;
        params.pitch = instance->pitch * pitch_range->playback_rate;
        params.minimum_distance = definition->minimum_distance;
        if (params.minimum_distance == 0.0f) {
            params.minimum_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
        }
        params.inner_cone_angle = definition->inner_cone_angle;
        params.outer_cone_angle = definition->outer_cone_angle;
        params.outer_cone_gain = definition->outer_cone_gain;
        params.eax_value = sound_class_definitions[definition->sound_class].eax_value;
        params.maximum_distance = 3.4028235e+38f;

        sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
        channels::set_next_permutation(channel_index, permutation, instance->first_person,
            definition->sound_class, 0);
        instance->channel_index = channel_index;
        return;
    }

    {
        sound_channel_parameters params;
        SoundPermutation *current = sound_channels[channel_index].current_permutation;

        if (current == 0) {
            params.gain = definition->random_gain_modifier * gain_factor;
        } else {
            params.gain = current->gain * definition->random_gain_modifier * gain_factor;
        }

        sound_channel_parameters_proc_ptr(channel_index, &params, 1, definition->sound_class);
        audio_device().channel_continue(channel_index, 0, definition->sound_class);
    }
}

void update_active(void)
{
    int16_t channel_index;
    datum_index sound_handle;
    sound *instance;
    Sound *definition;

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        sound_handle = sound_channels[channel_index].sound_index;
        if (sound_handle == 0xffffffff) {
            continue;
        }

        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & 0xffff].data;

        {
            float gain = instances::evaluate_fade_gain(sound_handle);

            if (gain == 0.0f && instance->fade_end_gain == 0.0f) {
                instances::stop(sound_handle);
                sound_channels[channel_index].sound_index = 0xffffffff;
                continue;
            }

            if (!(sound_channels[channel_index].type_flags & _sound_channel_3d_bit)) {
                if (instance->location.type == _sound_location_absolute ||
                    instance->location.type == _sound_location_listener_relative) {
                    real_point3d transformed = *(real_point3d *)&instance->location.position;
                    float min_distance, max_distance, distance, attenuation;

                    if (instance->location.type == _sound_location_absolute) {
                        halo::math::matrix4x3_inverse_transform_point(*((real_matrix4x3 *)&sound_listeners[instance->listener_index].scale),
                            transformed, *((real_point3d *)&instance->location.position));
                    }

                    distance = (float)sqrt((double)(transformed.x * transformed.x + transformed.y * transformed.y +
                        transformed.z * transformed.z));
                    min_distance = definition->minimum_distance;
                    if (min_distance == 0.0f) {
                        min_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
                    }
                    max_distance = definition->maximum_distance;
                    if (max_distance == 0.0f) {
                        max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
                    }
                    attenuation = 1.0f - (distance - min_distance) / (max_distance - min_distance);
                    if (attenuation < 0.0f) {
                        attenuation = 0.0f;
                    } else if (attenuation > 1.0f) {
                        attenuation = 1.0f;
                    }
                    gain = attenuation * gain;
                }
            } else if (instance->location.type == _sound_location_absolute) {
                real_matrix4x3 *listener_matrix = (real_matrix4x3 *)&sound_listeners[instance->listener_index].scale;
                if (!instance->first_person) {
                    real_point3d transformed_position;
                    real_vector3d transformed_forward;
                    real_vector3d transformed_velocity;
                    sound_channel_spatial spatial;

                    halo::math::matrix4x3_inverse_transform_point(*listener_matrix, transformed_position,
                        *((real_point3d *)&instance->location.position));
                    halo::math::matrix4x3_inverse_transform_normal(transformed_forward, *((real_vector3d *)&instance->location.forward),
                        *listener_matrix);
                    halo::math::matrix4x3_inverse_transform_vector(transformed_velocity, *((real_vector3d *)&instance->location.velocity),
                        *listener_matrix);
                    spatial.position = *(Point3D *)&transformed_position;
                    spatial.forward = *(Vector3D *)&transformed_forward;
                    spatial.velocity.i = transformed_velocity.i * 30.0f - sound_listeners[instance->listener_index].velocity.i;
                    spatial.velocity.j = transformed_velocity.j * 30.0f - sound_listeners[instance->listener_index].velocity.j;
                    spatial.velocity.k = transformed_velocity.k * 30.0f - sound_listeners[instance->listener_index].velocity.k;

                    audio_device().channel_set_spatial(channel_index, 1, &spatial,
                        instance->location.obstruction, instance->location.occlusion,
                        sound_listeners[instance->listener_index].underwater, definition->sound_class);
                } else {
                    sound_channel_spatial default_spatial;
                    default_spatial.position = *(Point3D *)global_zero_vector3d_pointer;
                    default_spatial.forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
                    default_spatial.velocity = *(Vector3D *)global_origin3d_pointer;
                    audio_device().channel_set_spatial(channel_index, 0, &default_spatial, 0.0f, 0.0f,
                        sound_listeners[instance->listener_index].underwater, definition->sound_class);
                }
            } else if (instance->location.type == _sound_location_listener_relative) {
                audio_device().channel_set_spatial(channel_index, 1,
                    (sound_channel_spatial *)&instance->location.position, 0.0f, 0.0f, 0, definition->sound_class);
            }

            if (instance->play_state == _sound_play_impulse) {
                instances::update_gain(channel_index, gain);
            } else {
                looping::update_gain(channel_index, gain);
            }

            if (sound_class_definitions[definition->sound_class].dialog != 0 &&
                instance->location_proc == instances::object_marker_location_proc) {
                SoundPermutation *current_permutation = sound_channels[channel_index].current_permutation;
                float lip_sync_value;

                if (current_permutation == 0 || current_permutation->mouth_data.size == 0) {
                    lip_sync_value = 0.0f;
                } else {
                    int16_t tick = (int16_t)(int32_t)sound_channels[channel_index].play_time;
                    int32_t clamped_tick;
                    int32_t mouth_data_count = current_permutation->mouth_data.size - 1;

                    if (tick < 0) {
                        clamped_tick = 0;
                    } else {
                        clamped_tick = tick;
                        if (mouth_data_count < tick) {
                            clamped_tick = mouth_data_count;
                        }
                    }
                    lip_sync_value = (float)((uint8_t *)current_permutation->mouth_data.pointer)[clamped_tick] *
                        0.003921569f;
                }

                if (game_looping_sound_data->valid &&
                    object_try_and_get(instance->owner_index, 3) != 0) {
                    unit_accumulate_clamped_offset(instance->owner_index, lip_sync_value);
                }
            }
        }
    }
}

void queue_definition_switch(datum_index sound_handle, datum_index new_definition_index)
{
    sound *instance;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    if (instance->definition_index != new_definition_index) {
        instance->pending_definition_index = new_definition_index;
    }
}

void apply_pending_definition_switch(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    sound_channel_candidate_list candidates;
    int16_t *list;
    int16_t count;
    int16_t channel_index;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)halo::cache::globals().tag_instances[instance->pending_definition_index & 0xffff].data;

    instance->flags |= _sound_permutation_pending_bit;
    instance->definition_index = instance->pending_definition_index;
    instance->pending_definition_index = 0xffffffff;
    instance->pitch_range_index = definitions::pick_pitch_range(instance->pitch_range_index, definition,
        instance->pitch);
    instance->permutation_index = definitions::pick_permutation(instance->pitch_range_index, -1, definition);

    if (instance->channel_index != -1) {
        channels::build_candidates(&candidates, sound_handle);

        if (candidates.owner_match_count < candidates.owner_match_limit) {
            if (candidates.tag_match_count < candidates.tag_match_limit) {
                return;
            }
            list = candidates.tag_matches;
            count = candidates.tag_match_count;
        } else {
            list = candidates.owner_matches;
            count = candidates.owner_match_count;
        }

        channel_index = channels::pick_replaceable(sound_handle, list, count);
        if (channel_index != -1) {
            instances::stop(sound_channels[channel_index].sound_index);
            return;
        }
        instances::stop(sound_handle);
    }
}

float evaluate_fade_gain(datum_index sound_handle)
{
    sound *instance;
    float t;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

    if (instance->fade_start_time == instance->fade_end_time) {
        return 1.0f;
    }

    t = (float)(sound_time - instance->fade_start_time) / (float)(instance->fade_end_time - instance->fade_start_time);
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }

    if (instance->fade_curve == _sound_fade_power) {
        if (instance->fade_end_gain <= instance->fade_start_gain) {
            t = (float)(1.0 - pow((double)(1.0f - t), (double)(1.0f / sound_fade_curve_exponent)));
        } else {
            t = (float)pow((double)t, (double)(1.0f / sound_fade_curve_exponent));
        }
    }

    if (t == 1.0f) {
        instance->fade_end_time = 0;
        instance->fade_start_time = 0;
    }

    return (instance->fade_end_gain - instance->fade_start_gain) * t + instance->fade_start_gain;
}

void render_debug(datum_index sound_handle)
{
    sound *instance;
    char buffer[512];

    if (debug_sound) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        sprintf(buffer, "%s|n%f %f", halo::cache::globals().tag_instances[instance->definition_index & 0xffff].path,
            (double)instance->location.obstruction, (double)instance->location.occlusion);
    }
}

uint8_t scenery_create(datum_index object_index)
{
    *(uint32_t *)(object_get(object_index) + 0x10) |= 0x40000;
    return 1;
}

}  // namespace instances


}  // namespace halo::sound
