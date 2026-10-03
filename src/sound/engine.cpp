/**
 * @file src/sound/engine.cpp
 * The sound engine lifecycle: initialize, update, pause, resume and device reopening.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"
#include "halo/memory/api.hpp"

namespace halo::sound {

namespace {

/** Zeroes a block of dwords and releases it with GlobalFree. */
void sound_dispose_zero_and_free(void *block, int32_t dword_count)
{
    uint32_t *words = (uint32_t *)block;
    int32_t i;
    for (i = 0; i < dword_count; i++) {
        words[i] = 0;
    }
    GlobalFree(block);
}

}  // namespace

namespace engine {

void pause(void)
{
    instances::stop_all();

    if (sound_paused != 1) {
        sound_paused = 1;
        if (current_sound_driver != 0) {
            audio_device().set_paused(1);
        }
    }

    sound_cache_release_unused();
}

void resume(void)
{
    if (shell_window_proc_bypass != 0) {
        return;
    }

    directsound_device().set_eax_enabled((directsound_eax_enabled != 0 && directsound_eax_available != 0) ? 1 : 0, 1);

    if (sound_paused != 0) {
        sound_paused = 0;
        if (current_sound_driver != 0) {
            audio_device().set_paused(0);
        }
        sound_time = time_query_performance_counter_ms();
    }
}

void initialize(void)
{
    sound_initialized = 0;
    sound_enabled = 1;
    sound_cache_new();

    if (sound_disabled == 0) {
        sound_environment = k_default_sound_environment;
        sound_ducking_gain = 1.0f;
        sound_music_gain = 1.0f;
        sound_master_gain = 1.0f;
        sound_effects_gain = 1.0f;
        sound_permutation_limit = 0;
        sound_channel_parameters_proc_ptr = channels::apply_default_parameters;

        if (driver_parameters.driver_index >= 0 && driver_parameters.driver_index < 2) {
            sound_driver *driver = sound_drivers[driver_parameters.driver_index];

            if (driver != (sound_driver *)0 && driver->type == driver_parameters.driver_index) {
                current_sound_driver = driver;
                sound_data = halo::memory::data_new(sizeof(sound), (char *)"sounds", k_maximum_sounds);

                if (sound_data != (data_array *)0) {
                    looping_sound_data = halo::memory::data_new(sizeof(looping_sound), (char *)"looping sounds", k_maximum_looping_sounds);
                }

                if (sound_data != (data_array *)0 && looping_sound_data != (data_array *)0) {
                    uint8_t initialized;

                    audio_device().set_quality(0, 0, 1);
                    initialized = audio_device().initialize(&driver_parameters);

                    if (initialized != 0) {
                        int16_t index = 0;
                        int32_t type;

                        sound_data->valid = 1;
                        halo::memory::data_delete_all(sound_data);
                        looping_sound_data->valid = 1;
                        halo::memory::data_delete_all(looping_sound_data);

                        for (type = 0; type < 4; type++) {
                            int16_t slot_count = driver_parameters.slot_counts[type];

                            sound_channel_count += slot_count;

                            if (slot_count > 0) {
                                uint16_t type_flags = sound_channel_type_flag_table[type];
                                int16_t remaining = slot_count;

                                do {
                                    sound_channels[index].sound_index = (datum_index)0xffffffff;
                                    sound_channels[index].type_flags = type_flags;
                                    sound_channels[index].current_permutation = (SoundPermutation *)0;
                                    sound_channels[index].next_permutation = (SoundPermutation *)0;
                                    index++;
                                    remaining--;
                                } while (remaining != 0);
                            }
                        }

                        sound_initialized = 1;

                        if (audio_device().eax_available() != 0) {
                            sound_channel_parameters_proc_ptr = channels::apply_eax_parameters;
                        }
                    }
                }
            }
        }
    }
}

uint8_t reopen_device(sound_driver_parameters *new_parameters)
{
    datum_index index = halo::memory::datum_next(-1, sound_data);
    uint8_t initialized;

    while (index != k_datum_index_none) {
        instances::stop(index);
        index = halo::memory::datum_next((int16_t)index, sound_data);
    }

    audio_device().dispose();
    sound_initialized = 0;
    sound_enabled = 1;
    sound_channel_count = 0;
    sound_channel_parameters_proc_ptr = channels::apply_default_parameters;

    initialized = audio_device().initialize(new_parameters);

    if (initialized != 0) {
        int16_t channel_index = 0;
        int32_t type;

        for (type = 0; type < 4; type++) {
            int16_t slot_count = new_parameters->slot_counts[type];

            sound_channel_count += slot_count;

            if (slot_count > 0) {
                uint16_t type_flags = sound_channel_type_flag_table[type];
                int16_t remaining = slot_count;

                do {
                    sound_channels[channel_index].sound_index = (datum_index)0xffffffff;
                    sound_channels[channel_index].type_flags = type_flags;
                    sound_channels[channel_index].current_permutation = (SoundPermutation *)0;
                    sound_channels[channel_index].next_permutation = (SoundPermutation *)0;
                    channel_index++;
                    remaining--;
                } while (remaining != 0);
            }
        }

        sound_initialized = 1;

        if (audio_device().eax_available() != 0) {
            sound_channel_parameters_proc_ptr = channels::apply_eax_parameters;
        }
    }

    return initialized;
}

void dispose(void)
{
    if (sound_initialized != 0) {
        audio_device().dispose();
        sound_data->valid = 0;
        looping_sound_data->valid = 0;
        sound_initialized = 0;
    }

    if (sound_data != (data_array *)0) {
        sound_dispose_zero_and_free(sound_data, 0xe);
    }

    if (looping_sound_data != (data_array *)0) {
        sound_dispose_zero_and_free(looping_sound_data, 0xe);
    }

    sound_cache_initialized = 0;
    sound_dispose_zero_and_free(sound_cache_entries, 0xe);
    sound_dispose_zero_and_free(sound_cache, 0x11);
    sound_cache_base = (void *)0;
}

void update(void)
{
    if (sound_disabled != 0) {
        return;
    }

    if (game_time->paused == 0 && (console_globals_data.active == 0 || current_game_engine != 0)) {
        if (sound_paused != 0) {
            sound_paused = 0;
            if (current_sound_driver != 0) {
                audio_device().set_paused(0);
            }
            sound_time = time_query_performance_counter_ms();
        }
    } else if (sound_paused != 1) {
        sound_paused = 1;
        if (current_sound_driver != 0) {
            audio_device().set_paused(1);
        }
    }

    {
        float saved_delta = sound_time_delta;
        int32_t saved_time = sound_time;

        if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
            uint8_t due;

            engine::update_clock();
            due = (uint32_t)(sound_time - saved_time) > 0x20;

            if (due) {
                audio_device().begin_frame();
            }

            if (sound_paused == 0 && due) {
                classes::update_gain_fade((int32_t)sound_time_delta);
                spatial::update_listener();
                looping::update_states();
                spatial::update_range_and_ducking();
                channels::assign();
                instances::update_active();
                sound_update_toggle = sound_update_toggle == 0;
                saved_time = sound_time;
                saved_delta = sound_time_delta;
            }

            sound_time_delta = saved_delta;
            sound_time = saved_time;

            if (due) {
                audio_device().end_frame();
            }
        }
    }

    if (sound_paused == 0) {
        sound_cache->age += 1;
    }
}

void idle_update(void)
{
    float saved_delta = sound_time_delta;
    int32_t saved_time = sound_time;

    sound_idle_update_active = 1;

    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        uint8_t due;

        engine::update_clock();
        due = (uint32_t)(sound_time - saved_time) > 0x20;

        if (due) {
            audio_device().begin_frame();
        }

        if (sound_paused == 0 && due) {
            instances::update_active();
            saved_time = sound_time;
            saved_delta = sound_time_delta;
        }

        sound_time_delta = saved_delta;
        sound_time = saved_time;

        if (due) {
            audio_device().end_frame();
        }
    }

    sound_cache->age += 1;
    sound_idle_update_active = 0;
}

void update_clock(void)
{
    large_integer counter;
    int32_t new_time;
    float old_time;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    new_time = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    old_time = (float)sound_time;
    sound_time = new_time;
    sound_time_delta = ((float)new_time - old_time) * 0.03f;
}

}  // namespace engine


}  // namespace halo::sound
