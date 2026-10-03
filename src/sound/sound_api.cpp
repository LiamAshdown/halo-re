/**
 * @file src/sound/sound_api.cpp
 * The sound module's public API (include/halo/sound/api.hpp): forwards the calls other modules and the data tables make to the
 * halo::sound implementation, and exposes the module's engine globals.
 */

#include "internal/state.hpp"
#include "halo/sound/api.hpp"

namespace halo::sound {

Globals &globals()
{
    static Globals instance{::game_looping_sound_data, ::game_sound_globals_ptr, ::looping_sound_data, ::sound_data, ::sound_time,
                            ::sound_enabled, ::sound_disabled, ::sound_music_gain, ::current_sound_driver};
    return instance;
}



void game_sound_initialize(void)
{
    halo::sound::game_looping::initialize();
}

void game_sound_revert_scripting_sounds(void)
{
    halo::sound::game_looping::revert_scripting_sounds();
}

void game_sound_reconcile_scripting_state(void)
{
    halo::sound::game_looping::reconcile_scripting_state();
}

datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name, int16_t function_index)
{
    return halo::sound::game_looping::create(object_index, definition_index, marker_name, function_index);
}

datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint)
{
    return halo::sound::instances::start_at_object_marker(object_index, position, forward, definition_index, node_index, scale, first_person_hint);
}

datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale)
{
    return halo::sound::instances::start_at_location(definition_index, placement, scale);
}

datum_index sound_start_unspatialized(datum_index definition_index, float scale)
{
    return halo::sound::instances::start_unspatialized(definition_index, scale);
}

void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale)
{
    halo::sound::instances::impulse_start(object_index, definition_index, scale);
}

int32_t sound_impulse_time(datum_index sound_tag_handle)
{
    return halo::sound::instances::impulse_time(sound_tag_handle);
}

void sound_looping_predict(datum_index looping_definition)
{
    halo::sound::looping::predict(looping_definition);
}

void sound_looping_start(datum_index definition_index, datum_index object_index, float scale)
{
    halo::sound::looping::start(definition_index, object_index, scale);
}

void sound_looping_stop(datum_index looping_definition)
{
    halo::sound::looping::stop(looping_definition);
}

void sound_looping_set_scale(datum_index looping_definition, float gain)
{
    halo::sound::looping::set_scale(looping_definition, gain);
}

void sound_looping_set_alternate(datum_index looping_definition, uint8_t alternate)
{
    halo::sound::looping::set_alternate(looping_definition, alternate);
}







void game_sound_update(void)
{
    halo::sound::game_looping::update();
}





























void sound_class_set_gain_by_name(char *name, float gain, int16_t ticks)
{
    halo::sound::classes::set_gain_by_name(name, gain, ticks);
}

void sound_class_set_muted_by_name(uint8_t enabled, char *name)
{
    halo::sound::classes::set_muted_by_name(enabled, name);
}

float sound_definition_maximum_distance(datum_index sound_definition)
{
    return halo::sound::definitions::maximum_distance(sound_definition);
}















uint8_t sound_driver_initialize(sound_driver_parameters *parameters)
{
    return halo::sound::directsound_device().initialize(parameters);
}



void sound_driver_dispose(void)
{
    halo::sound::directsound_device().dispose();
}



void sound_driver_end_frame(void)
{
    halo::sound::directsound_device().end_frame();
}

void sound_driver_begin_frame(void)
{
    halo::sound::directsound_device().begin_frame();
}

void sound_driver_stop_all(void)
{
    halo::sound::directsound_device().stop_all();
}

void sound_driver_set_paused(uint8_t paused)
{
    halo::sound::directsound_device().set_paused(paused);
}

void sound_listener_update(sound_listener_parameters *parameters)
{
    halo::sound::directsound_device().set_listener(parameters);
}























void sound_driver_set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality)
{
    halo::sound::directsound_device().set_quality(unknown, eax_enabled, quality);
}

void sound_pause(void)
{
    halo::sound::engine::pause();
}

void sound_resume(void)
{
    halo::sound::engine::resume();
}

void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force)
{
    halo::sound::directsound_device().set_eax_enabled(eax_enabled, force);
}

uint8_t sound_driver_eax_available(void)
{
    return halo::sound::directsound_device().eax_available();
}



void sound_driver_channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap)
{
    halo::sound::directsound_device().channel_play(channel_index, source, unused, sound_class, crosslap);
}

void sound_driver_channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class)
{
    halo::sound::directsound_device().channel_continue(channel_index, unused, sound_class);
}

void sound_driver_channel_stop(int16_t channel_index)
{
    halo::sound::directsound_device().channel_stop(channel_index);
}

int32_t sound_driver_channel_get_state(int16_t channel_index)
{
    return halo::sound::directsound_device().channel_get_state(channel_index);
}

void sound_driver_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    halo::sound::directsound_device().channel_set_spatial(channel_index, spatialized, spatial, obstruction, occlusion, underwater, sound_class);
}

void sound_driver_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)
{
    halo::sound::directsound_device().channel_set_parameters(channel_index, parameters, unknown);
}



void sound_set_master_gain(float gain)
{
    halo::sound::classes::set_master_gain(gain);
}

void sound_set_music_gain(float gain)
{
    halo::sound::classes::set_music_gain(gain);
}

void sound_set_effects_gain(float gain)
{
    halo::sound::classes::set_effects_gain(gain);
}

void sound_initialize(void)
{
    halo::sound::engine::initialize();
}



void sound_fade_out_and_stop_all(void)
{
    halo::sound::instances::fade_out_and_stop_all();
}

void sound_dispose(void)
{
    halo::sound::engine::dispose();
}

void sound_update(void)
{
    halo::sound::engine::update();
}

void sound_idle_update(void)
{
    halo::sound::engine::idle_update();
}

void sounds_refresh_structure_locations(void)
{
    halo::sound::spatial::refresh_structure_locations();
}

datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint)
{
    return halo::sound::instances::play_new(definition_index, location, owner_index, location_proc, callback_data, callback_data_size, first_person_hint);
}

void sound_impulse_fade_out(datum_index sound_index)
{
    halo::sound::instances::impulse_fade_out(sound_index);
}





void sound_stop_all(void)
{
    halo::sound::instances::stop_all();
}





















































































uint8_t sound_scenery_create(datum_index object_index)
{
    return halo::sound::instances::scenery_create(object_index);
}

int32_t sound_decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size)
{
    return halo::sound::codec::decode_dispatch(channel_count, destination, source, source_size);
}



int32_t sound_adpcm_decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    return halo::sound::codec::decode_mono(source, destination, block_count, block_size, samples_per_block, out_0, out_1);
}

int32_t sound_adpcm_decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    return halo::sound::codec::decode_stereo(source, destination, block_count, block_size, samples_per_block, out_0, out_1);
}

void sound_eax1_effect_shutdown(sound_effect_object *this_object)
{
    halo::sound::eax1_backend().shutdown(this_object);
}

int32_t sound_eax1_effect_initialize(sound_effect_object *this_object, directsound_channel *channel, int32_t unused)
{
    return halo::sound::eax1_backend().initialize(this_object, channel, unused);
}

void sound_eax1_effect_apply_listener(sound_effect_object *this_object, SoundEnvironment *environment)
{
    halo::sound::eax1_backend().apply_listener(this_object, environment);
}

void __thiscall sound_eax1_effect_set_environment_index(sound_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax1_backend().set_environment_index(this_object, environment);
}

void __thiscall sound_eax1_effect_set_room_gain(sound_effect_object *this_object, float gain)
{
    return halo::sound::eax1_backend().set_room_gain(this_object, gain);
}





int32_t __thiscall sound_effect_object_listener_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().listener_supported(this_object);
}

int32_t __thiscall sound_effect_object_channel_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().channel_supported(this_object);
}

void __thiscall sound_eax20_effect_shutdown(sound_eax_effect_object *this_object)
{
    return halo::sound::eax2_backend().shutdown(reinterpret_cast<sound_effect_object *>(this_object));
}

int __thiscall sound_eax20_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused)
{
    return halo::sound::eax2_backend().initialize(reinterpret_cast<sound_effect_object *>(this_object), listener, unused);
}

int32_t __thiscall sound_eax_effect_initialize_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax2_backend().initialize_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}

void __thiscall sound_eax20_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax2_backend().apply_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}

void __thiscall sound_eax20_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
{
    return halo::sound::eax2_backend().apply_listener(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

void __thiscall sound_eax20_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax2_backend().set_environment_index(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

void __thiscall sound_eax20_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
{
    return halo::sound::eax2_backend().set_room_gain(reinterpret_cast<sound_effect_object *>(this_object), gain);
}

void __thiscall sound_eax30_effect_shutdown(sound_eax_effect_object *this_object)
{
    return halo::sound::eax3_backend().shutdown(reinterpret_cast<sound_effect_object *>(this_object));
}

int __thiscall sound_eax30_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused)
{
    return halo::sound::eax3_backend().initialize(reinterpret_cast<sound_effect_object *>(this_object), listener, unused);
}

void __thiscall sound_eax30_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax3_backend().apply_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}



void __thiscall sound_eax30_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
{
    return halo::sound::eax3_backend().apply_listener(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

void __thiscall sound_eax30_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax3_backend().set_environment_index(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

void __thiscall sound_eax30_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
{
    return halo::sound::eax3_backend().set_room_gain(reinterpret_cast<sound_effect_object *>(this_object), gain);
}

int32_t __thiscall sound_eax1_effect_initialize_channel(sound_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax1_backend().initialize_channel(this_object, channel_index);
}

int32_t __thiscall sound_eax1_effect_channel_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().channel_supported(this_object);
}

void __thiscall sound_eax1_effect_apply_channel(sound_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax1_backend().apply_channel(this_object, channel_index);
}









void __cdecl sound_effects_object_reinitialize(int enable)
{
    return halo::sound::effects::reinitialize(enable);
}

}
