/**
 * @file include/halo/sound/api.hpp
 * Functions of the sound module that other modules and the data tables call (namespace halo::sound). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct Point3D;
struct data_array;
struct game_sound_globals;
struct sound_driver;
struct SoundEnvironment;
struct SoundPermutation;
struct Vector3D;
struct directsound_channel;
struct sound_channel_parameters;
struct sound_channel_spatial;
struct sound_driver_parameters;
struct sound_eax_effect_object;
struct sound_effect_object;
struct sound_listener_parameters;
struct sound_location;
struct sound_placement;
typedef uint32_t datum_index;
typedef uint8_t (*sound_location_proc)(datum_index owner, void *callback_data, sound_location *location);

namespace halo::sound {

/**
 * The engine globals the sound module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    data_array *&game_looping_sound_data;
    game_sound_globals *&game_sound_state;
    data_array *&looping_sound_data;
    data_array *&sound_data;
    int32_t &time;
    uint8_t &enabled;
    uint8_t &disabled;
    float &music_gain;
    sound_driver *&current_driver;
};

Globals &globals();

void game_sound_initialize(void);
void game_sound_revert_scripting_sounds(void);
void game_sound_reconcile_scripting_state(void);
datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name, int16_t function_index);
datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);
datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
datum_index sound_start_unspatialized(datum_index definition_index, float scale);
void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale);
int32_t sound_impulse_time(datum_index sound_tag_handle);
void sound_looping_predict(datum_index looping_definition);
void sound_looping_start(datum_index definition_index, datum_index object_index, float scale);
void sound_looping_stop(datum_index looping_definition);
void sound_looping_set_scale(datum_index looping_definition, float gain);
void sound_looping_set_alternate(datum_index looping_definition, uint8_t alternate);
void game_sound_update(void);
void sound_class_set_gain_by_name(char *name, float gain, int16_t ticks);
void sound_class_set_muted_by_name(uint8_t enabled, char *name);
float sound_definition_maximum_distance(datum_index sound_definition);
uint8_t sound_driver_initialize(sound_driver_parameters *parameters);
void sound_driver_dispose(void);
void sound_driver_end_frame(void);
void sound_driver_begin_frame(void);
void sound_driver_stop_all(void);
void sound_driver_set_paused(uint8_t paused);
void sound_listener_update(sound_listener_parameters *parameters);
void sound_driver_set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality);
void sound_pause(void);
void sound_resume(void);
void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force);
uint8_t sound_driver_eax_available(void);
void sound_driver_channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap);
void sound_driver_channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class);
void sound_driver_channel_stop(int16_t channel_index);
int32_t sound_driver_channel_get_state(int16_t channel_index);
void sound_driver_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class);
void sound_driver_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown);
void sound_set_master_gain(float gain);
void sound_set_music_gain(float gain);
void sound_set_effects_gain(float gain);
void sound_initialize(void);
void sound_fade_out_and_stop_all(void);
void sound_dispose(void);
void sound_update(void);
void sound_idle_update(void);
void sounds_refresh_structure_locations(void);
datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint);
void sound_impulse_fade_out(datum_index sound_index);
void sound_stop_all(void);
uint8_t sound_scenery_create(datum_index object_index);
int32_t sound_decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size);
int32_t sound_adpcm_decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1);
int32_t sound_adpcm_decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1);
void sound_eax1_effect_shutdown(sound_effect_object *this_object);
int32_t sound_eax1_effect_initialize(sound_effect_object *this_object, directsound_channel *channel, int32_t unused);
void sound_eax1_effect_apply_listener(sound_effect_object *this_object, SoundEnvironment *environment);
void __thiscall sound_eax1_effect_set_environment_index(sound_effect_object *this_object, int32_t environment);
void __thiscall sound_eax1_effect_set_room_gain(sound_effect_object *this_object, float gain);
int32_t __thiscall sound_effect_object_listener_supported(sound_effect_object *this_object);
int32_t __thiscall sound_effect_object_channel_supported(sound_effect_object *this_object);
void __thiscall sound_eax20_effect_shutdown(sound_eax_effect_object *this_object);
int __thiscall sound_eax20_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused);
int32_t __thiscall sound_eax_effect_initialize_channel(sound_eax_effect_object *this_object, int32_t channel_index);
void __thiscall sound_eax20_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index);
void __thiscall sound_eax20_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment);
void __thiscall sound_eax20_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment);
void __thiscall sound_eax20_effect_set_room_gain(sound_eax_effect_object *this_object, float gain);
void __thiscall sound_eax30_effect_shutdown(sound_eax_effect_object *this_object);
int __thiscall sound_eax30_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused);
void __thiscall sound_eax30_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index);
void __thiscall sound_eax30_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment);
void __thiscall sound_eax30_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment);
void __thiscall sound_eax30_effect_set_room_gain(sound_eax_effect_object *this_object, float gain);
int32_t __thiscall sound_eax1_effect_initialize_channel(sound_effect_object *this_object, int32_t channel_index);
int32_t __thiscall sound_eax1_effect_channel_supported(sound_effect_object *this_object);
void __thiscall sound_eax1_effect_apply_channel(sound_effect_object *this_object, int32_t channel_index);
void __cdecl sound_effects_object_reinitialize(int enable);

}
