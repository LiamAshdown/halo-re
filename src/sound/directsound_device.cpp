/**
 * @file src/sound/directsound_device.cpp
 * DirectSound implementation of the AudioDevice interface.
 */

#include "halo/sound/directsound.hpp"
#include "internal/state.hpp"
#include "halo/sound/api.hpp"

namespace halo::sound {

namespace {

/** Absolute value of a float. */
float sound_listener_update_fabsf(float x) { return (x < 0.0f) ? -x : x; }

}  // namespace

void DirectSoundDevice::dispose(void)
{
    int32_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        void *buffer = directsound_channels[i].buffer;
        void *buffer_3d = directsound_channels[i].buffer_3d;

        if (buffer_3d != 0) {
            void **vtable = halo::sound::com_methods(buffer_3d);
            void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
            release(buffer_3d);
        }
        if (buffer != 0) {
            void **vtable = halo::sound::com_methods(buffer);
            void (__stdcall *stop)(void *) = (void (__stdcall *)(void *))vtable[halo::sound::dsound_slot::sb_stop];
            void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
            stop(buffer);
            release(buffer);
        }
    }
    directsound_channel_count = 0;

    if (global_sound_effect_object != 0) {
        void **vtable = halo::sound::com_methods(global_sound_effect_object);
        void (__stdcall *shutdown)(void *) = (void (__stdcall *)(void *))vtable[0];
        shutdown(global_sound_effect_object);
        free(global_sound_effect_object);
        global_sound_effect_object = 0;
        sound_effect_object_state = 0;
    }

    if (directsound_listener != 0) {
        void **vtable = halo::sound::com_methods(directsound_listener);
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        release(directsound_listener);
        directsound_listener = 0;
    }

    if (directsound_primary_buffer != 0) {
        void **vtable = halo::sound::com_methods(directsound_primary_buffer);
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        release(directsound_primary_buffer);
        directsound_primary_buffer = 0;
    }

    if (directsound != 0) {
        void **vtable = halo::sound::com_methods(directsound);
        int32_t (__stdcall *set_cooperative_level)(void *, void *, uint32_t) =
            (int32_t (__stdcall *)(void *, void *, uint32_t))vtable[halo::sound::dsound_slot::ds_set_cooperative_level];
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        void *active_window = GetActiveWindow();

        set_cooperative_level(directsound, active_window, 1);
        release(directsound);
        directsound = 0;
    }

    directsound_initialized = 0;
}

void DirectSoundDevice::end_frame(void)
{
    char text[0x2000];
    int16_t i;

    if (directsound_deferred_dirty != 0) {
        halo::sound::com_method<directsound_listener_commit_proc>(directsound_listener, halo::sound::dsound_slot::lst_commit_deferred_settings)(directsound_listener);
        directsound_deferred_dirty = 0;
    }

    if (directsound_paused == 0) {
        if (directsound_fade != 1.0f) {
            double fade = (double)directsound_fade + 0.05;

            directsound_fade = (float)(fade < 1.0 ? fade : 1.0);
        }
    } else if (directsound_fade != 0.0f) {
        double fade = (double)directsound_fade - 0.05;

        directsound_fade = (float)(fade > 0.0 ? fade : 0.0);

        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (channel->state != _directsound_channel_idle || channel->streaming != 0) {
                float gain = directsound_fade * channel->gain;
                int32_t volume = -10000;

                if (gain != 0.0f) {
                    volume = (int32_t)(log10((double)gain) * 2000.0);
                    if (volume < -10000) {
                        volume = -10000;
                    } else if (volume > 0) {
                        volume = 0;
                    }
                }
                halo::sound::com_method<directsound_buffer_set_volume_proc>(channel->buffer, halo::sound::dsound_slot::sb_set_volume)(channel->buffer, volume);
            }
        }

        if (directsound_fade == 0.0f) {
            for (i = 0; i < directsound_channel_count; i++) {
                directsound_channel *channel = &directsound_channels[i];

                if (channel->state != _directsound_channel_idle || channel->streaming != 0) {
                    halo::sound::com_method<directsound_buffer_stop_proc>(channel->buffer, halo::sound::dsound_slot::sb_stop)(channel->buffer);
                }
            }
        }
    }

    channels::update_streaming();

    if (debug_sound_channel_details != 0) {
        hud_text_draw_background_mode = 1;
        text_tab_stops = 0x118;
        text[0] = '\0';
        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (i >= 0x20) {
                continue;
            }
            if (channel->state != _directsound_channel_idle) {
                const char *current = channel->source != 0 ? (const char *)channel->source : k_empty_string;
                const char *next = channel->next_source != 0 ? (const char *)channel->next_source : k_empty_string;

                sprintf(text + strlen(text), "%1.2f %1.2f %s(%s)", (double)channel->gain, (double)channel->pitch,
                    current, next);
            }
            sprintf(text + strlen(text), "|t");
            if ((i & 1) != 0) {
                sprintf(text + strlen(text), "|n");
            }
        }
    } else if (debug_sound_channels != 0) {
        int32_t counts[4] = { 0, 0, 0, 0 };

        text_tab_stops = 0x118;
        hud_text_draw_background_mode = 1;
        text[0] = '\0';
        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (channel->state == _directsound_channel_idle) {
                continue;
            }
            if (channel->type_flags == sound_channel_type_flag_table[0]) {
                counts[0]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[1]) {
                counts[1]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[2]) {
                counts[2]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[3]) {
                counts[3]++;
            }
        }
        sprintf(text, "|n|n%i / %i mono 3D channels.|n", counts[0], (int32_t)driver_parameters.channel_counts[0]);
        sprintf(text + strlen(text), "%i / %i mono channels.|n", counts[1], (int32_t)driver_parameters.channel_counts[1]);
        sprintf(text + strlen(text), "%i / %i stereo channels.|n", counts[2], (int32_t)driver_parameters.channel_counts[2]);
        sprintf(text + strlen(text), "%i / %i 44k stereo channels.|n", counts[3],
            (int32_t)driver_parameters.channel_counts[3]);
    }
}

void DirectSoundDevice::begin_frame(void)
{
    directsound_deferred_dirty = 0;
}

void DirectSoundDevice::stop_all(void)
{
    int16_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        reset_channel(i);
        directsound_channels[i].free = 0;
    }
}

void DirectSoundDevice::set_paused(uint8_t paused)
{
    int16_t i;

    if (paused != directsound_paused) {
        if (paused == 0) {
            for (i = 0; i < directsound_channel_count; i++) {
                directsound_channel *channel = &directsound_channels[i];

                if (channel->state != _directsound_channel_idle) {
                    channel->gain = 0.0f;
                    if (channel->buffer != 0) {
                        halo::sound::com_method<directsound_buffer_play_proc>(channel->buffer, halo::sound::dsound_slot::sb_play)(channel->buffer, 0, 0, 1);
                    }
                    stream_update(i, channel->source_crosslap);
                }
            }
        }
        directsound_paused = paused;
    }
}

void DirectSoundDevice::set_listener(sound_listener_parameters *parameters)
{
    void **vtable = halo::sound::com_methods(directsound_listener);

    if (sound_listener_update_fabsf(parameters->position.x - directsound_listener_cached.position.x) >= 0.05f ||
        sound_listener_update_fabsf(parameters->position.y - directsound_listener_cached.position.y) >= 0.05f ||
        sound_listener_update_fabsf(parameters->position.z - directsound_listener_cached.position.z) >= 0.05f ||
        directsound_initialized == 0) {
        int32_t (__stdcall *set_position)(void *, float, float, float, uint32_t) =
            (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::lst_set_position];
        set_position(directsound_listener, parameters->position.x, parameters->position.y,
            parameters->position.z, 0);
        directsound_listener_cached.position = parameters->position;
    }

    {
        float *orientation = (float *)&parameters->forward;
        uint8_t changed = directsound_initialized == 0;
        int32_t i;

        for (i = 0; i < 6; i++) {
            if (sound_listener_update_fabsf(orientation[i] - ((float *)&directsound_listener_cached.forward)[i]) >= 0.05f) {
                changed = 1;
            }
        }

        if (changed) {
            int32_t (__stdcall *set_orientation)(void *, float, float, float, float, float, float, uint32_t) =
                (int32_t (__stdcall *)(void *, float, float, float, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::lst_set_orientation];
            set_orientation(directsound_listener, orientation[0], orientation[1], orientation[2],
                orientation[3], orientation[4], orientation[5], 0);
            for (i = 0; i < 6; i++) {
                ((float *)&directsound_listener_cached.forward)[i] = orientation[i];
            }
        }
    }

    if (sound_listener_update_fabsf(parameters->velocity.i - directsound_listener_cached.velocity.i) >= 0.01f ||
        sound_listener_update_fabsf(parameters->velocity.j - directsound_listener_cached.velocity.j) >= 0.01f ||
        sound_listener_update_fabsf(parameters->velocity.k - directsound_listener_cached.velocity.k) >= 0.01f ||
        directsound_initialized == 0) {
        int32_t (__stdcall *set_velocity)(void *, float, float, float, uint32_t) =
            (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[halo::sound::dsound_slot::lst_set_velocity];
        set_velocity(directsound_listener, parameters->velocity.i, parameters->velocity.j,
            parameters->velocity.k, 0);
        directsound_listener_cached.velocity = parameters->velocity;
    }

    {
        uint32_t *environment_words = (uint32_t *)parameters->environment;
        uint32_t *cache_words = (uint32_t *)&directsound_environment_cache;
        uint8_t same = 1;
        int32_t i;

        for (i = 0; i < 0x12; i++) {
            if (environment_words[i] != cache_words[i]) {
                same = 0;
                break;
            }
        }

        if (!same || directsound_initialized == 0) {
            directsound_environment_cache = *parameters->environment;

            if (global_sound_effect_object != 0) {
                sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;

                if (fx_vtable->listener_supported(global_sound_effect_object) != 0) {
                    fx_vtable->apply_listener(global_sound_effect_object, parameters->environment);
                }
            }
        }
    }
}

void DirectSoundDevice::set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality)
{
    uint8_t eax_currently_active;
    uint8_t force = 0;

    if (quality < 0 || quality > 2) {
        quality = 1;
    }
    if (directsound_quality != quality) {
        directsound_quality = quality;
        force = 1;
    }

    if (directsound_eax_enabled == 0 || directsound_eax_available == 0 || global_sound_effect_object == 0 ||
        (global_sound_effect_object->mode != _sound_effect_object_eax1 &&
         global_sound_effect_object->mode != _sound_effect_object_eax2 &&
         global_sound_effect_object->mode != _sound_effect_object_eax3)) {
        eax_currently_active = 0;
    } else {
        eax_currently_active = 1;
    }

    if (eax_enabled != eax_currently_active) {
        effects::reinitialize(eax_enabled);
        force = 1;
    }

    set_eax_enabled((uint8_t)unknown, force);
}

void DirectSoundDevice::set_eax_enabled(uint8_t eax_enabled, uint8_t force)
{
    if (directsound_initialized == 0) {
        return;
    }

    if (eax_enabled == 0) {
        if (directsound_eax_enabled == 0 && force == 0) {
            return;
        }
        directsound_eax_enabled = 0;
    } else {
        if (directsound_eax_enabled != 0 && force == 0) {
            return;
        }
        directsound_eax_enabled = 1;
        driver_parameters.driver_index = 0;
    }

    driver_parameters.channel_counts[0] = 0x16;
    driver_parameters.channel_counts[1] = 2;
    driver_parameters.channel_counts[2] = 2;
    driver_parameters.channel_counts[3] = 2;
    driver_parameters.slot_counts[0] = 0x16;
    driver_parameters.slot_counts[1] = 2;
    driver_parameters.slot_counts[2] = 2;
    driver_parameters.slot_counts[3] = 2;
    directsound_hardware_mode = -1;

    engine::reopen_device(&driver_parameters);
}

uint8_t DirectSoundDevice::eax_available(void)
{
    if (directsound_eax_enabled != 0 && directsound_eax_available != 0 && global_sound_effect_object != 0) {
        int32_t mode = global_sound_effect_object->mode;

        if (mode == _sound_effect_object_eax1 || mode == _sound_effect_object_eax2 ||
            mode == _sound_effect_object_eax3) {
            return 1;
        }
    }
    return 0;
}

void DirectSoundDevice::channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        queue_source(directsound_bindings[channel_index].hardware_channel_index, source, sound_class,
            crosslap);
    }
}

void DirectSoundDevice::channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        stream_update(directsound_bindings[channel_index].hardware_channel_index, unused);
    }
}

void DirectSoundDevice::channel_stop(int16_t channel_index)
{
    sound_channel_binding *binding = &directsound_bindings[channel_index];

    if (binding->hardware_channel_index != -1) {
        reset_channel(binding->hardware_channel_index);
        directsound_channels[binding->hardware_channel_index].sound_channel_index = -1;
        binding->hardware_channel_index = -1;
    }
}

directsound_channel_state DirectSoundDevice::channel_get_state(int16_t channel_index)
{
    int16_t hardware_channel_index = directsound_bindings[channel_index].hardware_channel_index;

    if (hardware_channel_index != -1) {
        return check_loop_boundary(hardware_channel_index);
    }
    return _directsound_channel_idle;
}

void DirectSoundDevice::channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        commit_spatial(directsound_bindings[channel_index].hardware_channel_index, spatialized, spatial,
            obstruction, occlusion, underwater, sound_class);
    }
}

void DirectSoundDevice::channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        commit_parameters(directsound_bindings[channel_index].hardware_channel_index, parameters, unknown);
    }
}

namespace {

constinit DirectSoundDevice g_directsound_device;

}  // namespace

/**
 * Returns the DirectSound device instance. It is constant-initialized, so it is usable before any static
 * initializer has run.
 */
DirectSoundDevice &directsound_device()
{
    return g_directsound_device;
}

/**
 * Returns the audio device the engine drives; today always the DirectSound device.
 */
AudioDevice &audio_device()
{
    return g_directsound_device;
}

}  // namespace halo::sound
