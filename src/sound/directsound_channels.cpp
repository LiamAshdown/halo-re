/**
 * @file src/sound/directsound_channels.cpp
 * DirectSound hardware channel streaming, parameters and spatialization.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/sound/directsound.hpp"
#include "internal/state.hpp"
#include "halo/shell/api.hpp"

namespace halo::sound {

namespace {

/** Absolute value of a float. */
float sound_channel_set_spatial_fabsf(float x) { return (x < 0.0f) ? -x : x; }

/** Absolute value of a float. */
float sound_linear_gain_to_attenuation_fabs(float x) { return (x < 0.0f) ? -x : x; }

}  // namespace

void DirectSoundDevice::commit_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer_3d;
    int32_t (__stdcall *set_mode)(void *, uint32_t, uint32_t) = (int32_t (__stdcall *)(void *, uint32_t, uint32_t))vtable[halo::sound::dsound_slot::b3d_set_mode];
    int32_t (__stdcall *set_position)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::b3d_set_position];
    int32_t (__stdcall *set_cone_orientation)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::b3d_set_cone_orientation];
    int32_t (__stdcall *set_velocity)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::b3d_set_velocity];
    uint8_t mode_changed = 0;
    uint8_t dialog_class = (sound_class >= soundclass_scripted_dialog_player && sound_class <= soundclass_scripted_dialog_force_unspatialized);

    if (channel->spatialized != spatialized || directsound_initialized == 0) {
        if (halo::shell::globals().head_relative_speech == 0 || spatialized != 0 || !dialog_class) {
            set_mode(channel->buffer_3d, (spatialized != 0) ? 0u : 2u, 1);
        } else {
            set_mode(channel->buffer_3d, 1, 1);
            set_position(channel->buffer_3d, 0.0f, 0.0f, 0.0f, 1);
        }
        channel->spatialized = spatialized;
        mode_changed = 1;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->position.x - channel->position.x) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->position.y - channel->position.y) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->position.z - channel->position.z) >= 0.05f)) ||
        directsound_initialized == 0) {
        if (halo::shell::globals().head_relative_speech == 0 || spatialized != 0 || !dialog_class) {
            set_position(channel->buffer_3d, spatial->position.x, -spatial->position.y, spatial->position.z, 1);
        } else {
            set_position(channel->buffer_3d, 0.0f, 0.0f, 0.0f, 1);
        }
        channel->position = spatial->position;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->forward.i - channel->cone_orientation.i) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->forward.j - channel->cone_orientation.j) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->forward.k - channel->cone_orientation.k) >= 0.05f)) ||
        directsound_initialized == 0) {
        set_cone_orientation(channel->buffer_3d, spatial->forward.i, -spatial->forward.j,
            spatial->forward.k, 1);
        channel->cone_orientation = spatial->forward;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->velocity.i - channel->velocity.i) >= 0.01f ||
          sound_channel_set_spatial_fabsf(spatial->velocity.j - channel->velocity.j) >= 0.01f ||
          sound_channel_set_spatial_fabsf(spatial->velocity.k - channel->velocity.k) >= 0.01f)) ||
        directsound_initialized == 0) {
        set_velocity(channel->buffer_3d, spatial->velocity.i, -spatial->velocity.j, spatial->velocity.k, 1);
        channel->velocity = spatial->velocity;
        directsound_deferred_dirty = 1;
    }

    if (sound_channel_set_spatial_fabsf(obstruction - channel->obstruction) >= 0.001f ||
        sound_channel_set_spatial_fabsf(occlusion - channel->occlusion) >= 0.001f ||
        channel->underwater != underwater || mode_changed || directsound_initialized == 0) {
        channel->obstruction = obstruction;
        channel->occlusion = occlusion;
        channel->underwater = underwater;

        if (global_sound_effect_object != 0) {
            sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;

            if (fx_vtable->channel_supported(global_sound_effect_object) != 0) {
                fx_vtable->apply_channel(global_sound_effect_object, channel_index);
            }
        }
    }
}

void DirectSoundDevice::commit_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    float effective_gain = directsound_fade * parameters->gain;

    if (sound_linear_gain_to_attenuation_fabs(effective_gain - channel->gain) >= 0.001f ||
        directsound_initialized == 0) {
        int32_t volume;

        if (effective_gain == 0.0f) {
            volume = -10000;
        } else {
            volume = (int32_t)(log10((double)effective_gain) * 2000.0);
            if (volume < -10000) {
                volume = -10000;
            } else if (volume > 0) {
                volume = 0;
            }
        }

        {
            void **vtable = *(void ***)channel->buffer;
            int32_t (__stdcall *set_volume)(void *, int32_t) = (int32_t (__stdcall *)(void *, int32_t))vtable[halo::sound::dsound_slot::sb_set_volume];
            channel->volume = volume;
            set_volume(channel->buffer, volume);
        }
        channel->gain = effective_gain;
    }

    if (update == 0) {
        if (sound_linear_gain_to_attenuation_fabs(parameters->pitch - channel->pitch) >= 0.001f ||
            directsound_initialized == 0) {
            float hertz = parameters->pitch *
                (float)k_sound_sample_rates[(channel->type_flags & _sound_channel_44khz_bit) >> 2];
            int32_t frequency;

            if (hertz < 188.0f) {
                hertz = 188.0f;
            } else if (hertz > 191983.0f) {
                hertz = 191983.0f;
            }
            frequency = (int32_t)hertz;

            if (directsound_hardware_mode != 3) {
                void **vtable = *(void ***)channel->buffer;
                int32_t (__stdcall *set_frequency)(void *, int32_t) = (int32_t (__stdcall *)(void *, int32_t))vtable[halo::sound::dsound_slot::sb_set_frequency];
                set_frequency(channel->buffer, frequency);
            }
            channel->pitch = parameters->pitch;
            channel->frequency = (int16_t)frequency;
            directsound_deferred_dirty = 1;
        }

        if ((channel->type_flags & _sound_channel_3d_bit) != 0) {
            void **vtable_3d = *(void ***)channel->buffer_3d;

            if (sound_linear_gain_to_attenuation_fabs(parameters->maximum_distance - channel->maximum_distance) >= 0.05f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_max_distance)(void *, float, uint32_t) =
                    (int32_t (__stdcall *)(void *, float, uint32_t))vtable_3d[halo::sound::dsound_slot::b3d_set_max_distance];
                set_max_distance(channel->buffer_3d, parameters->maximum_distance, 1);
                channel->maximum_distance = parameters->maximum_distance;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->minimum_distance - channel->minimum_distance) >= 0.05f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_min_distance)(void *, float, uint32_t) =
                    (int32_t (__stdcall *)(void *, float, uint32_t))vtable_3d[halo::sound::dsound_slot::b3d_set_min_distance];
                set_min_distance(channel->buffer_3d, parameters->minimum_distance, 1);
                channel->minimum_distance = parameters->minimum_distance;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->inner_cone_angle - channel->inner_cone_angle) >= 0.034906585f ||
                sound_linear_gain_to_attenuation_fabs(parameters->outer_cone_angle - channel->outer_cone_angle) >= 0.034906585f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_cone_angles)(void *, uint32_t, uint32_t, uint32_t) =
                    (int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t))vtable_3d[halo::sound::dsound_slot::b3d_set_cone_angles];
                uint32_t outer_degrees = (uint32_t)(int32_t)(parameters->outer_cone_angle * 57.29578f);
                uint32_t inner_degrees = (uint32_t)(int32_t)(parameters->inner_cone_angle * 57.29578f);
                set_cone_angles(channel->buffer_3d, inner_degrees, outer_degrees, 1);
                channel->inner_cone_angle = parameters->inner_cone_angle;
                channel->outer_cone_angle = parameters->outer_cone_angle;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->outer_cone_gain - channel->cone_outside_gain) >= 0.001f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_cone_outside_volume)(void *, int32_t, uint32_t) =
                    (int32_t (__stdcall *)(void *, int32_t, uint32_t))vtable_3d[halo::sound::dsound_slot::b3d_set_cone_outside_volume];
                int32_t volume = gain::linear_to_attenuation(parameters->outer_cone_gain, 0);
                set_cone_outside_volume(channel->buffer_3d, volume, 1);
                channel->cone_outside_gain = parameters->outer_cone_gain;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->eax_value - channel->eax_value) >= 0.001f ||
                directsound_initialized == 0) {
                channel->eax_value = parameters->eax_value;
                if (global_sound_effect_object != 0) {
                    sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;
                    if (fx_vtable->channel_supported(global_sound_effect_object) != 0) {
                        fx_vtable->apply_channel(global_sound_effect_object, channel_index);
                    }
                }
            }
        }
    }
}

uint32_t DirectSoundDevice::refresh_cursor(int16_t channel_index)
{
    void *buffer = directsound_channels[channel_index].buffer;
    void **vtable = *(void ***)buffer;
    int32_t (__stdcall *get_current_position)(void *, uint32_t *, uint32_t *) =
        (int32_t (__stdcall *)(void *, uint32_t *, uint32_t *))vtable[halo::sound::dsound_slot::sb_get_current_position];
    uint32_t play_cursor;
    uint32_t write_cursor;

    get_current_position(buffer, &play_cursor, &write_cursor);
    return play_cursor;
}

void DirectSoundDevice::stream_update(int16_t channel_index, uint8_t unused)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    int32_t buffer_size = channel->buffer_size;
    int32_t play_cursor;
    int32_t write_cursor;
    int32_t delta;
    void **vtable;

    (void)unused;
    if (channel->state == _directsound_channel_idle && channel->streaming != 1) {
        return;
    }

    vtable = *(void ***)channel->buffer;
    ((directsound_buffer_get_current_position_proc)vtable[halo::sound::dsound_slot::sb_get_current_position])(channel->buffer, &play_cursor, &write_cursor);

    if (channel->streaming == 0) {
        delta = play_cursor - channel->write_cursor;
        if (delta < 0) {
            delta += buffer_size;
        }
        if (delta != 0) {
            lock_and_fill(channel_index, (uint32_t)delta);
            channel->write_cursor = play_cursor;
        }
    } else if (channel->streaming_bytes == -1) {
        delta = write_cursor > play_cursor ? play_cursor - write_cursor + buffer_size : play_cursor - write_cursor;
        if (delta != 0) {
            channel->write_cursor = write_cursor;
            lock_and_fill(channel_index, (uint32_t)delta);
            channel->streaming_bytes = delta;
            channel->write_cursor = play_cursor;
        }
    } else {
        delta = play_cursor - channel->write_cursor;
        if (delta < 0) {
            delta += buffer_size;
        }
        if (delta != 0) {
            lock_and_fill(channel_index, (uint32_t)delta);
            channel->write_cursor = play_cursor;
            channel->streaming_bytes += delta;
            if (channel->streaming_bytes < 0) {
                channel->streaming_bytes = buffer_size;
            }
        }
    }
}

uint8_t DirectSoundDevice::lock_and_fill(int16_t channel_index, uint32_t fill_size)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer;
    void *ptr1;
    uint32_t bytes1;
    void *ptr2;
    uint32_t bytes2;

    if (((directsound_buffer_lock_proc)vtable[halo::sound::dsound_slot::sb_lock])(channel->buffer, channel->write_cursor, fill_size,
            &ptr1, &bytes1, &ptr2, &bytes2, 0) < 0) {
        return 0;
    }

    fill_pcm_data(channel_index, (uint8_t *)ptr1, channel->write_cursor, &channel->source_crosslap,
        (int32_t)bytes1);
    if (ptr2 != (void *)0) {
        fill_pcm_data(channel_index, (uint8_t *)ptr2, 0, &channel->source_crosslap, (int32_t)bytes2);
    }

    vtable = *(void ***)channel->buffer;
    if (((directsound_buffer_unlock_proc)vtable[halo::sound::dsound_slot::sb_unlock])(channel->buffer, ptr1, bytes1, ptr2, bytes2) < 0) {
        return 0;
    }
    return 1;
}

void DirectSoundDevice::fill_pcm_data(int16_t channel_index, uint8_t *destination, int32_t base_position, uint8_t *crosslap, int32_t byte_count)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    uint32_t produced = (uint32_t)(int32_t)channel_index;

    while (byte_count > 0) {
        SoundPermutation *source = channel->source;

        if (source != (SoundPermutation *)0 && channel->source_started == 0) {
            channel->source_started = 1;
            if (source->format == soundformat_16_bit_pcm || source->format == soundformat_xbox_adpcm) {
                channel->decoder.position = 0;
            } else if (source->format == soundformat_ogg_vorbis) {
                view(&channel->decoder)->close_slot(*crosslap);
            }
        }

        source = channel->source;
        if (source == (SoundPermutation *)0) {
            int32_t i;

            for (i = 0; i < byte_count; i++) {
                destination[i] = 0;
            }
            return;
        }

        if (source->format == soundformat_16_bit_pcm || source->format == soundformat_xbox_adpcm) {
            if (stream::pcm_buffer_read((uint32_t *)&channel->decoder.position, source, &produced,
                    (uint32_t)byte_count, destination) != 0) {
                return;
            }
        } else if (source->format == soundformat_ogg_vorbis) {
            if (view(&channel->decoder)->fill_buffer(source, destination, (uint32_t)byte_count, (char *)crosslap, &produced) != 0) {
                return;
            }
        }

        destination += produced & 0xfffffffe;
        channel->source_started = 0;
        byte_count -= (int32_t)produced;
        if (channel->source_end_cursor != -1) {
            channel->state = _directsound_channel_playing;
        }
        channel->source_end_cursor = (int32_t)produced + base_position;
        channel->source = channel->next_source;
        channel->source_crosslap = channel->next_source_crosslap;
        channel->next_source_crosslap = 0;
        channel->next_source = (SoundPermutation *)0;
    }
}

int32_t DirectSoundDevice::restore_buffer(void *buffer, uint8_t *was_restored_out)
{
    void **vtable;
    uint32_t status;
    int32_t hr;

    if (buffer == (void *)0) {
        return (int32_t)0x800401f0;
    }
    if (was_restored_out != (uint8_t *)0) {
        *was_restored_out = 0;
    }

    vtable = *(void ***)buffer;
    hr = ((directsound_buffer_get_status_proc)vtable[halo::sound::dsound_slot::sb_get_status])(buffer, &status);
    if (hr < 0) {
        return hr;
    }
    if ((status & 2) == 0) {
        return 1;
    }

    do {
        if (((directsound_buffer_restore_proc)vtable[halo::sound::dsound_slot::sb_restore])(buffer) == (int32_t)0x88780096) {
            Sleep(0);
        }
    } while (((directsound_buffer_restore_proc)vtable[halo::sound::dsound_slot::sb_restore])(buffer) == (int32_t)0x88780096);

    if (was_restored_out != (uint8_t *)0) {
        *was_restored_out = 1;
    }
    return 0;
}

void DirectSoundDevice::queue_source(int16_t channel_index, SoundPermutation *source, int16_t sound_class, uint8_t crosslap)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    int32_t sample_rate = k_sound_sample_rates[(channel->type_flags & _sound_channel_44khz_bit) >> 2];
    int32_t buffer_size = (((channel->type_flags & _sound_channel_stereo_bit) != 0) + 1) * sample_rate * 6;
    uint8_t start_playback = 0;

    if (crosslap != 0 && source->format != soundformat_ogg_vorbis) {
        crosslap = 0;
    }

    switch (channel->state) {
    case _directsound_channel_queued:
        if (channel->next_source != (SoundPermutation *)0) {
            channel->next_source = source;
            channel->next_source_crosslap = 1;
            stream_update(channel_index, channel->source_crosslap);
            return;
        }
        {
            int32_t play_cursor = (int32_t)refresh_cursor(channel_index);
            int32_t source_end = channel->source_end_cursor;
            int32_t delta = play_cursor - source_end;

            if (delta < 0) {
                delta += buffer_size;
            }
            if (delta < buffer_size - sample_rate / 10) {
                channel->write_cursor = source_end;
                channel->source = source;
                channel->source_crosslap = 1;
                lock_and_fill(channel_index, (uint32_t)delta);
                channel->write_cursor = play_cursor;
                return;
            }
        }
        channel->next_source = source;
        channel->next_source_crosslap = 1;
        stream_update(channel_index, channel->source_crosslap);
        return;

    case _directsound_channel_playing:
        channel->state = _directsound_channel_queued;
        if (channel->source != (SoundPermutation *)0) {
            channel->next_source = source;
            channel->next_source_crosslap = crosslap;
        } else {
            channel->source = source;
            channel->write_cursor = channel->source_end_cursor;
        }
        stream_update(channel_index, crosslap);
        return;

    case _directsound_channel_idle: {
        int32_t fill_size = buffer_size;

        channel->state = _directsound_channel_playing;
        channel->source = source;
        if (channel->streaming != 0) {
            void **vtable = *(void ***)channel->buffer;
            int32_t play_cursor;
            int32_t write_cursor;

            ((directsound_buffer_get_current_position_proc)vtable[halo::sound::dsound_slot::sb_get_current_position])(channel->buffer, &play_cursor,
                &write_cursor);
            channel->write_cursor = write_cursor;
            channel->source_end_cursor = -1;
            if (play_cursor < write_cursor) {
                fill_size = buffer_size - write_cursor + play_cursor;
            } else if (play_cursor > write_cursor) {
                fill_size = play_cursor - write_cursor;
            } else {
                start_playback = 1;
            }
        } else {
            channel->write_cursor = 0;
            channel->source_end_cursor = -1;
            start_playback = 1;
        }

        channel->source_crosslap = 0;
        channel->sound_class = sound_class;
        channel->streaming_bytes = -1;
        if (lock_and_fill(channel_index, (uint32_t)fill_size) == 0) {
            return;
        }

        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
        directsound_deferred_dirty = 0;
        channel->streaming = 0;
        if (start_playback) {
            void **vtable = *(void ***)channel->buffer;
            uint8_t restored = 0;

            ((directsound_buffer_set_current_position_proc)vtable[halo::sound::dsound_slot::sb_set_current_position])(channel->buffer, channel->write_cursor);
            if (restore_buffer(channel->buffer, &restored) < 0) {
                return;
            }
            if (restored) {
                lock_and_fill(channel_index, (uint32_t)fill_size);
            }
            vtable = *(void ***)channel->buffer;
            ((directsound_buffer_play_proc)vtable[halo::sound::dsound_slot::sb_play])(channel->buffer, 0, 0, 1);
        }
        return;
    }

    default:
        return;
    }
}

void DirectSoundDevice::reset_channel(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    uint8_t keep_streaming = (halo::shell::globals().enable_stop_start == 0);

    channel->source = (SoundPermutation *)0;
    channel->next_source = (SoundPermutation *)0;

    if (keep_streaming && (channel->type_flags & _sound_channel_3d_bit) != 0 &&
        channel->sound_class == soundclass_weapon_fire && sound_stopping_all == 0) {
        channel->streaming = 1;
        stream_update(channel_index, 0);
    } else {
        void **vtable = *(void ***)channel->buffer;
        int32_t (__stdcall *stop)(void *) = (int32_t (__stdcall *)(void *))vtable[halo::sound::dsound_slot::sb_stop];
        stop(channel->buffer);
        channel->streaming_bytes = -1;
        channel->streaming = 0;
    }

    channel->source_started = 0;
    channel->state = _directsound_channel_idle;
    channel->decoder.open = 0;
    channel->free = 1;
    channel->sound_class = -1;
}

uint32_t DirectSoundDevice::claim_if_finished(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer;
    int32_t (__stdcall *get_status)(void *, uint32_t *) = (int32_t (__stdcall *)(void *, uint32_t *))vtable[halo::sound::dsound_slot::sb_get_status];
    uint32_t status;
    int32_t hr = get_status(channel->buffer, &status);

    if (hr < 0) {
        return 0;
    }

    if (channel->streaming == 0) {
        if ((status & 5) != 0) {
            return 0;
        }
    } else {
        channel->streaming_bytes = -1;
    }

    channel->free = 0;
    return 1;
}

directsound_channel_state DirectSoundDevice::check_loop_boundary(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];

    if (channel->state != 0 && channel->source_end_cursor != -1) {
        void **vtable = *(void ***)channel->buffer;
        int32_t (__stdcall *get_current_position)(void *, int32_t *, int32_t *) =
            (int32_t (__stdcall *)(void *, int32_t *, int32_t *))vtable[halo::sound::dsound_slot::sb_get_current_position];
        int32_t play_cursor;
        int32_t write_cursor_unused;
        int32_t end = channel->source_end_cursor;
        int32_t write = channel->write_cursor;

        get_current_position(channel->buffer, &play_cursor, &write_cursor_unused);

        if ((end < write && end < play_cursor && play_cursor < write) ||
            (write < end && (end < play_cursor || play_cursor < write))) {
            if (channel->state == _directsound_channel_queued) {
                channel->state = _directsound_channel_playing;
                channel->source_end_cursor = -1;
                return (directsound_channel_state)channel->state;
            }
            if (channel->state == _directsound_channel_playing) {
                channel->state = _directsound_channel_idle;
            }
            channel->source_end_cursor = -1;
        }
    }

    return (directsound_channel_state)channel->state;
}

void DirectSoundDevice::bind_hardware(int16_t logical_channel_index)
{
    sound_channel_binding *binding = &directsound_bindings[logical_channel_index];
    int16_t candidate = directsound_first_channel_of_type[binding->channel_type];

    if (binding->hardware_channel_index == -1) {
        do {
            if (candidate >= directsound_channel_count ||
                directsound_channels[candidate].type_flags != sound_channel_type_flag_table[binding->channel_type]) {
                break;
            }

            if (directsound_channels[candidate].sound_channel_index == -1 &&
                (directsound_channels[candidate].free == 0 ||
                 claim_if_finished(candidate) != 0)) {
                binding->hardware_channel_index = candidate;
            }

            candidate++;
        } while (binding->hardware_channel_index == -1);

        if (binding->hardware_channel_index == -1) {
            return;
        }
    }

    directsound_channels[binding->hardware_channel_index].sound_channel_index = logical_channel_index;
}


}  // namespace halo::sound
