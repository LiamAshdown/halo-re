/**
 * @file src/sound/directsound_setup.cpp
 * DirectSound device bring-up and channel pool probing.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/sound/directsound.hpp"
#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/sound/api.hpp"

constexpr int k_probe_pool_capacity = 77;
template <typename T>
inline void *&vtable_slot(T object, uint32_t slot) { return halo::sound::com_methods(object)[slot]; }
constexpr int k_probe_all_pools = 0x116;
constexpr int k_channel_budget_iteration_limit = 0x200;

namespace halo::sound {

namespace {

typedef struct { void **vtable; } com_object;

typedef int32_t (__stdcall *direct_sound_create8_proc)(void *device_guid, void **direct_sound, void *outer);

typedef int32_t (__stdcall *directsound_set_cooperative_level_proc)(void *self, void *window, uint32_t level);

typedef int32_t (__stdcall *directsound_get_caps_proc)(void *self, void *caps);

typedef int32_t (__stdcall *directsound_buffer_set_format_proc)(void *self, sound_wave_format *format);

typedef int32_t (__stdcall *directsound_listener_set_factor_proc)(void *self, float value, uint32_t apply);

enum {
    _channel_pool_mono_3d = 0,
    _channel_pool_mono,
    _channel_pool_stereo,
    _channel_pool_stereo_44k
};

/** Creates up to `requested` DirectSound buffers of one format to find how many the hardware supports, optionally querying their 3D interfaces, and stores the count. */
void sound_directsound_probe_pool(int32_t *out_count, int32_t requested, uint32_t samples_per_second,
    uint16_t channels, uint16_t block_align, uint32_t flags, uint8_t query_3d,
    com_object **buffers, com_object **buffers_3d)
{
    sound_wave_format format;
    sound_buffer_description desc;
    int32_t i;

    format.format_tag = 1;
    format.channels = channels;
    format.samples_per_second = samples_per_second;
    format.average_bytes_per_second = samples_per_second * block_align;
    format.block_align = block_align;
    format.bits_per_sample = 16;
    format.extra_size = 0;

    desc.size = 0x24;
    desc.flags = flags;
    desc.buffer_bytes = format.average_bytes_per_second;
    desc.reserved = 0;
    desc.format = &format;
    desc.algorithm_3d[0] = 0;
    desc.algorithm_3d[1] = 0;
    desc.algorithm_3d[2] = 0;
    desc.algorithm_3d[3] = 0;

    for (i = 0; i < requested; i++) {
        void **directsound_vtable = halo::sound::com_methods(directsound);
        int32_t (__stdcall *create_sound_buffer)(void *, sound_buffer_description *, com_object **, void *) =
            (int32_t (__stdcall *)(void *, sound_buffer_description *, com_object **, void *))directsound_vtable[3];
        int32_t hr = create_sound_buffer(directsound, &desc, &buffers[i], (void *)0);

        if (hr != 0) {
            break;
        }

        if (query_3d) {
            int32_t (__stdcall *query_interface)(com_object *, uint8_t *, com_object **) =
                (int32_t (__stdcall *)(com_object *, uint8_t *, com_object **))buffers[i]->vtable[0];
            hr = query_interface(buffers[i], iid_directsound_3d_buffer, &buffers_3d[i]);
            if (hr != 0) {
                break;
            }
        }

        *out_count += 1;
    }
}

/** Probes all four channel pools with the requested counts. */
void sound_driver_probe(int32_t counts[k_sound_channel_type_count], const int32_t requested[k_sound_channel_type_count])
{
    directsound_device().probe_channel_pools(&counts[0], requested[0], &counts[1], requested[1],
        &counts[2], requested[2], &counts[3], requested[3], k_probe_all_pools);
}

/** True while a pool is below its target size and has not been marked saturated. */
uint8_t sound_driver_pool_below(const int32_t counts[k_sound_channel_type_count],
    const uint8_t saturated[k_sound_channel_type_count], int32_t pool)
{
    static const int32_t targets[k_sound_channel_type_count] = { 51, 10, 8, 8 };

    return counts[pool] < targets[pool] && !saturated[pool];
}

}  // namespace

void DirectSoundDevice::probe_channel_pools(int32_t *mono3d_count, uint32_t mono3d_requested, int32_t *mono_count, uint32_t mono_requested, int32_t *stereo_count, uint32_t stereo_requested, int32_t *stereo44k_count, uint32_t stereo44k_requested, uint32_t pool_mask)
{
    com_object *buffers[k_probe_pool_capacity];
    com_object *buffers_3d[k_probe_pool_capacity];
    int32_t used = 0;
    int32_t i;

    for (i = 0; i < k_probe_pool_capacity; i++) {
        buffers[i] = (com_object *)0;
        buffers_3d[i] = (com_object *)0;
    }

    if ((pool_mask & 2) != 0 && mono3d_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)mono3d_requested, 22050, 1, 2, 0x180b4, 1,
            buffers + 1, buffers_3d);
        *mono3d_count += count;
        used += count;
    }

    if ((pool_mask & 4) != 0 && mono_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)mono_requested, 22050, 1, 2, 0x180a4, 0,
            buffers + used, buffers_3d);
        *mono_count += count;
        used += count;
    }

    if ((pool_mask & 0x10) != 0 && stereo_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)stereo_requested, 22050, 2, 4, 0x180a4, 0,
            buffers + used, buffers_3d);
        *stereo_count += count;
        used += count;
    }

    if ((pool_mask & 0x100) != 0 && stereo44k_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)stereo44k_requested, 44100, 2, 4, 0x180a4, 0,
            buffers + used, buffers_3d);
        *stereo44k_count += count;
        used += count;
    }

    for (i = 0; i < k_probe_pool_capacity; i++) {
        if (buffers_3d[i] != (com_object *)0) {
            void (__stdcall *release)(com_object *) = (void (__stdcall *)(com_object *))buffers_3d[i]->vtable[2];
            release(buffers_3d[i]);
            buffers_3d[i] = (com_object *)0;
        }
        if (buffers[i] != (com_object *)0) {
            void (__stdcall *release)(com_object *) = (void (__stdcall *)(com_object *))buffers[i]->vtable[2];
            release(buffers[i]);
            buffers[i] = (com_object *)0;
        }
    }
}

uint8_t DirectSoundDevice::initialize(sound_driver_parameters *parameters)
{
    uint8_t success = 0;
    uint32_t caps[0x18];
    sound_buffer_description description;
    sound_wave_format format;
    uint32_t sample_rate;
    uint8_t probe_valid;
    int32_t counts[k_sound_channel_type_count];
    int32_t type;
    sound_listener_parameters listener;

    directsound_initialized = 0;
    directsound_paused = 0;
    directsound_fade = 1.0f;

    if (halo::shell::globals().nosound != 0 ||
        ((direct_sound_create8_proc)direct_sound_create8)((void *)0, &directsound, (void *)0) < 0 ||
        ((directsound_set_cooperative_level_proc)vtable_slot(directsound, halo::sound::dsound_slot::ds_set_cooperative_level))(directsound, halo::shell::globals().window, 2) < 0) {
        dispose();
        return 0;
    }

    caps[0] = 0x60;
    if (((directsound_get_caps_proc)vtable_slot(directsound, halo::sound::dsound_slot::ds_get_caps))(directsound, caps) < 0) {
        dispose();
        return 0;
    }
    for (type = 0; type < 0x18; type++) {
        ((uint32_t *)directsound_caps)[type] = caps[type];
    }

    description.size = 0x24;
    description.flags = 0x11;
    description.buffer_bytes = 0;
    description.reserved = 0;
    description.format = (sound_wave_format *)0;
    description.algorithm_3d[0] = description.algorithm_3d[1] = 0;
    description.algorithm_3d[2] = description.algorithm_3d[3] = 0;
    if (((directsound_create_sound_buffer_proc)vtable_slot(directsound, halo::sound::dsound_slot::ds_create_sound_buffer))(directsound, &description,
            &directsound_primary_buffer, (void *)0) < 0) {
        dispose();
        return 0;
    }

    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    if (directsound_quality == 2 && caps[3] > 22050) {
        sample_rate = 44100;
    } else if (caps[3] >= 22050) {
        sample_rate = 22050;
    } else {
        sample_rate = (caps[3] < 11025) ? 0 : 11025;
    }

    format.format_tag = 1;
    format.channels = 2;
    format.samples_per_second = sample_rate;
    format.average_bytes_per_second = sample_rate * 4;
    format.block_align = 4;
    format.bits_per_sample = 16;
    if (((directsound_buffer_set_format_proc)vtable_slot(directsound_primary_buffer, halo::sound::dsound_slot::sb_set_format))(directsound_primary_buffer, &format) < 0) {
        dispose();
        return 0;
    }

    parameters->driver_index = 0;
    probe_valid = 1;
    directsound_eax_available = 1;
    directsound_hardware_3d_channel_count = 0;
    if (!directsound_eax_enabled) {
        sound_effect_object_state = 2;
        directsound_hardware_mode = -1;
    }

    {
        static const int32_t minimums[k_sound_channel_type_count] = { 16, 2, 2, 2 };
        sound_driver_probe(counts, minimums);
    }
    if (counts[0] < 16 || counts[1] + counts[2] + counts[3] < 6) {
        probe_valid = 0;
    }

    if (directsound_eax_enabled && probe_valid) {
        int32_t requested[k_sound_channel_type_count] = { 16, 2, 2, 2 };
        int32_t previous[k_sound_channel_type_count];
        uint8_t saturated[k_sound_channel_type_count] = { 0, 0, 0, 0 };
        uint8_t shrank = 0;
        uint8_t done = 0;
        bool exhausted = false;
        int32_t pool = _channel_pool_mono_3d;
        int32_t iterations = 0;
        int32_t i;

        counts[0] = 16;
        counts[1] = counts[2] = counts[3] = 2;

        for (;;) {
            iterations++;
            for (i = 0; i < k_sound_channel_type_count; i++) {
                previous[i] = counts[i];
                counts[i] = 0;
            }

            requested[pool]++;
            sound_driver_probe(counts, requested);
            if (counts[pool] == previous[pool]) {
                saturated[pool] = 1;
            }

            if (previous[0] > counts[0] || previous[1] > counts[1] ||
                previous[2] > counts[2] || previous[3] > counts[3]) {
                shrank = 1;
                saturated[pool] = 1;
            }

            if (counts[0] < 16 || counts[1] < 2 || counts[2] < 2 || counts[3] < 2 || shrank) {
                counts[0] = counts[1] = counts[2] = counts[3] = 0;
                if ((saturated[0] && saturated[1] && saturated[2] && saturated[3]) || !shrank) {
                    done = 1;
                }
                switch (pool) {
                case _channel_pool_mono_3d: requested[0]--; break;
                case _channel_pool_mono: requested[1]--; break;
                case _channel_pool_stereo: requested[2]--; break;
                case _channel_pool_stereo_44k:
                    requested[3]--;
                    requested[2]--;
                    break;
                }
                sound_driver_probe(counts, requested);
                shrank = 0;
            }

            if (saturated[0] && saturated[1] && saturated[2] && saturated[3]) {
                break;
            }

            if (iterations >= k_channel_budget_iteration_limit) {
                static const int32_t minimums[k_sound_channel_type_count] = { 16, 2, 2, 2 };

                counts[0] = counts[1] = counts[2] = counts[3] = 0;
                sound_driver_probe(counts, minimums);
                break;
            }

            if (counts[0] >= 22 || saturated[0]) {
                pool = (pool >= 3) ? 0 : pool + 1;
                switch (pool) {
                case _channel_pool_mono_3d:
                    if (sound_driver_pool_below(counts, saturated, 0)) break;
                    pool = _channel_pool_mono;
                case _channel_pool_mono:
                    if (sound_driver_pool_below(counts, saturated, 1)) break;
                    pool = _channel_pool_stereo;
                case _channel_pool_stereo:
                    if (sound_driver_pool_below(counts, saturated, 2)) break;
                    pool = _channel_pool_stereo_44k;
                case _channel_pool_stereo_44k:
                    if (sound_driver_pool_below(counts, saturated, 3)) break;
                    if (sound_driver_pool_below(counts, saturated, 0)) {
                        pool = _channel_pool_mono_3d;
                    } else if (sound_driver_pool_below(counts, saturated, 1)) {
                        pool = _channel_pool_mono;
                    } else if (sound_driver_pool_below(counts, saturated, 2)) {
                        pool = _channel_pool_stereo;
                    } else {
                        exhausted = true;
                    }
                    break;
                }
            }

            if (done || exhausted) {
                break;
            }
        }
    } else {
        if (!probe_valid) {
            directsound_eax_available = 0;
        }
        switch (directsound_quality) {
        case 0: counts[0] = 22; counts[1] = 2; counts[2] = 2; counts[3] = 2; break;
        case 1: counts[0] = 24; counts[1] = 3; counts[2] = 3; counts[3] = 3; break;
        case 2: counts[0] = 26; counts[1] = 4; counts[2] = 4; counts[3] = 4; break;
        default: break;
        }
    }

    for (type = 0; type < k_sound_channel_type_count; type++) {
        parameters->channel_counts[type] = (int16_t)counts[type];
    }
    for (type = 0; type < k_sound_channel_type_count; type++) {
        parameters->slot_counts[type] = (int16_t)counts[type];
    }

    if (((directsound_query_interface_proc)vtable_slot(directsound_primary_buffer, 0x00))(directsound_primary_buffer,
            iid_directsound_3d_listener, &directsound_listener) < 0 ||
        ((directsound_listener_set_factor_proc)vtable_slot(directsound_listener, halo::sound::dsound_slot::lst_set_distance_factor))(directsound_listener, 3.048f, 0) < 0 ||
        ((directsound_listener_set_factor_proc)vtable_slot(directsound_listener, halo::sound::dsound_slot::lst_set_rolloff_factor))(directsound_listener,
            directsound_rolloff_factor, 0) < 0) {
        dispose();
        return 0;
    }
    ((directsound_listener_set_factor_proc)vtable_slot(directsound_listener, halo::sound::dsound_slot::lst_set_doppler_factor))(directsound_listener, 0.0f, 0);

    {
        int16_t binding_index = 0;
        int16_t slot;

        directsound_binding_count = 0;
        for (type = 0; (int16_t)type < k_sound_channel_type_count; type++) {
            for (slot = 0; slot < parameters->slot_counts[type]; slot++) {
                directsound_binding_count++;
                directsound_bindings[binding_index].channel_type = (int16_t)type;
                directsound_bindings[binding_index].hardware_channel_index = -1;
                binding_index++;
            }
        }
    }

    {
        int32_t channel_index = 0;
        int16_t i;

        for (type = 0; type < k_sound_channel_type_count; type++) {
            directsound_first_channel_of_type[type] = (int16_t)channel_index;
            for (i = 0; i < parameters->channel_counts[type]; i++) {
                directsound_channel_count++;
                if (create_channel((int16_t)channel_index, sound_channel_type_flag_table[type])) {
                    channel_index++;
                } else {
                    directsound_channel_count--;
                    parameters->channel_counts[type]--;
                    parameters->slot_counts[type]--;
                }
            }
        }
    }

    success = directsound_channel_count > 0;

    listener.position.x = listener.position.y = listener.position.z = 0.0f;
    listener.velocity.i = listener.velocity.j = listener.velocity.k = 0.0f;
    listener.forward.i = halo::math::globals().global_forward3d_pointer->i;
    listener.forward.j = halo::math::globals().global_forward3d_pointer->j;
    listener.forward.k = halo::math::globals().global_forward3d_pointer->k;
    listener.up.i = halo::math::globals().global_up3d_pointer->i;
    listener.up.j = halo::math::globals().global_up3d_pointer->j;
    listener.up.k = halo::math::globals().global_up3d_pointer->k;
    listener.environment = (SoundEnvironment *)&k_default_sound_environment;
    set_listener(&listener);

    if (success) {
        directsound_initialized = 1;
        return success;
    }

    dispose();
    return success;
}

uint8_t DirectSoundDevice::create_channel(int16_t channel_index, uint16_t type_flags)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    sound_wave_format format;
    sound_buffer_description desc;
    uint16_t channels;
    uint32_t samples_per_second;
    int32_t hr;
    void **vtable;

    channel->sound_channel_index = -1;
    channel->sound_class = -1;
    channel->streaming_bytes = -1;
    channel->type_flags = type_flags;
    channel->free = 0;
    channel->source = (SoundPermutation *)0;
    channel->next_source = (SoundPermutation *)0;
    channel->state = _directsound_channel_idle;
    channel->streaming = 0;

    channels = (uint16_t)(((type_flags & _sound_channel_stereo_bit) != 0) + 1);
    samples_per_second = (type_flags & _sound_channel_44khz_bit) != 0 ? 44100 : 22050;

    format.format_tag = 1;
    format.channels = channels;
    format.samples_per_second = samples_per_second;
    format.average_bytes_per_second = (uint32_t)(uint16_t)(channels * 2) * samples_per_second;
    format.block_align = (uint16_t)(channels * 2);
    format.bits_per_sample = 16;
    format.extra_size = 0;
    channel->buffer_size = (int32_t)(format.average_bytes_per_second * 3);

    desc.size = 0x24;
    desc.flags = (directsound_eax_enabled == 0 || directsound_eax_available == 0) ? 0x100a8 : 0x100a0;
    desc.buffer_bytes = (uint32_t)channel->buffer_size;
    desc.reserved = 0;
    desc.format = &format;
    desc.algorithm_3d[0] = 0;
    desc.algorithm_3d[1] = 0;
    desc.algorithm_3d[2] = 0;
    desc.algorithm_3d[3] = 0;

    if ((type_flags & _sound_channel_3d_bit) != 0) {
        win32_dsbcaps caps;

        desc.flags |= directsound_hardware_mode == 3 ? 0x210 : 0x10;
        if (directsound_eax_enabled == 0 || directsound_eax_available == 0) {
            desc.algorithm_3d[0] = ds3dalg_hrtf_full[0];
            desc.algorithm_3d[1] = ds3dalg_hrtf_full[1];
            desc.algorithm_3d[2] = ds3dalg_hrtf_full[2];
            desc.algorithm_3d[3] = ds3dalg_hrtf_full[3];
        }

        vtable = halo::sound::com_methods(directsound);
        if (((directsound_create_sound_buffer_proc)vtable[3])(directsound, &desc, &channel->buffer, (void *)0) < 0) {
            return 0;
        }

        caps.size = 0x14;
        caps.flags = 0;
        caps.buffer_bytes = 0;
        caps.unlock_transfer_rate = 0;
        caps.play_cpu_overhead = 0;
        vtable = halo::sound::com_methods(channel->buffer);
        hr = ((directsound_buffer_get_caps_proc)vtable[3])(channel->buffer, &caps);
        if (hr < 0) {
            return 0;
        }
        if ((caps.flags & 4) != 0) {
            directsound_hardware_3d_channel_count += 1;
        }
    } else {
        vtable = halo::sound::com_methods(directsound);
        hr = ((directsound_create_sound_buffer_proc)vtable[3])(directsound, &desc, &channel->buffer, (void *)0);
    }

    if (hr < 0) {
        return 1;
    }

    if ((type_flags & _sound_channel_3d_bit) == 0) {
        channel->buffer_3d = (void *)0;
    } else {
        vtable = halo::sound::com_methods(channel->buffer);
        if (((directsound_query_interface_proc)vtable[0])(channel->buffer, iid_directsound_3d_buffer,
                &channel->buffer_3d) < 0) {
            channel->buffer_3d = (void *)0;
        } else {
            sound_channel_spatial spatial = { 0 };

            spatial.forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;

            if (sound_effect_object_state == 0) {
                sound_effect_object_state = 2;
                directsound_hardware_mode = effects::detect_mode(channel_index, channel);
                if (directsound_hardware_mode != -1) {
                    sound_effect_object_state = 1;
                }
            }
            if (sound_effect_object_state == 1 &&
                channel_index < directsound_hardware_3d_channel_count &&
                effects::initialize_channel(channel_index) == 0) {
                effects::shutdown();
                sound_effect_object_state = 2;
            }

            commit_spatial(channel_index, 0, &spatial, 0.0f, 0.0f, 0, 0);
        }
    }

    {
        sound_channel_parameters parameters = { 0 };

        parameters.pitch = 1.0f;
        parameters.gain = 1.0f;
        commit_parameters(channel_index, &parameters, 0);
    }
    return 1;
}


}  // namespace halo::sound
