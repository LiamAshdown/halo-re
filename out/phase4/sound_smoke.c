#include <stddef.h>
#include "tags.h"
#include "memory.h"
#include "sound.h"

// Structs that hold pointers only match halo.exe with 32-bit pointers; those checks use the _P
// forms, which are gated on PTRS32 and only fire with -m32. Everything else is checked either way.
#define PTRS32 (sizeof(void *) == 4)
#define CHECK_SIZE(t, n) typedef char check_size_##t[(sizeof(t) == (n)) ? 1 : -1]
#define CHECK_OFFSET(t, f, n) typedef char check_offset_##t##_##f[(offsetof(t, f) == (n)) ? 1 : -1]
#define CHECK_SIZE_P(t, n) typedef char check_size_##t[(!PTRS32 || sizeof(t) == (n)) ? 1 : -1]
#define CHECK_OFFSET_P(t, f, n) typedef char check_offset_##t##_##f[(!PTRS32 || offsetof(t, f) == (n)) ? 1 : -1]
#define CHECK_P(name, expr) typedef char name[(!PTRS32 || (expr)) ? 1 : -1]

// struct sizes
CHECK_SIZE(sound_location, 0x40);
CHECK_SIZE(sound_class_definition, 0x2c);
CHECK_SIZE(sound_class_gain, 0x0c);
CHECK_SIZE(game_looping_sound, 0x34);
CHECK_SIZE(game_sound_globals, 0x0c);
CHECK_SIZE_P(sound, 0xb0);
CHECK_SIZE(looping_sound, 0xe4);
CHECK_SIZE_P(sound_channel, 0x18);
CHECK_SIZE(sound_listener, 0x44);
CHECK_SIZE_P(sound_listener_parameters, 0x34);
CHECK_SIZE(sound_channel_spatial, 0x24);
CHECK_SIZE(sound_channel_parameters, 0x20);
CHECK_SIZE(sound_driver_parameters, 0x14);
CHECK_SIZE_P(sound_driver, 0x40);
CHECK_SIZE(sound_channel_binding, 0x04);
CHECK_SIZE_P(sound_ogg_memory_file, 0x10);
CHECK_SIZE_P(sound_stream_decoder, 0x5cc);
CHECK_SIZE_P(directsound_channel, 0x678);
CHECK_SIZE(sound_wave_format, 0x12);
CHECK_SIZE_P(sound_buffer_description, 0x24);
CHECK_SIZE_P(sound_effect_object_vtable, 0x24);
CHECK_SIZE_P(sound_effect_object, 0x1c);
CHECK_SIZE_P(sound_eax_effect_object, 0xe8);

// sound_location
CHECK_OFFSET(sound_location, scale, 0x04);
CHECK_OFFSET(sound_location, position, 0x0c);
CHECK_OFFSET(sound_location, forward, 0x18);
CHECK_OFFSET(sound_location, up, 0x24);
CHECK_OFFSET(sound_location, leaf_index, 0x30);
CHECK_OFFSET(sound_location, cluster_index, 0x34);
CHECK_OFFSET(sound_location, obstruction, 0x38);
CHECK_OFFSET(sound_location, occlusion, 0x3c);

// sound_class_definition
CHECK_OFFSET(sound_class_definition, minimum_replace_time, 0x04);
CHECK_OFFSET(sound_class_definition, dialog, 0x08);
CHECK_OFFSET(sound_class_definition, priority, 0x0a);
CHECK_OFFSET(sound_class_definition, discard_on_cache_miss, 0x0c);
CHECK_OFFSET(sound_class_definition, eax_value, 0x10);
CHECK_OFFSET(sound_class_definition, default_minimum_distance, 0x18);
CHECK_OFFSET(sound_class_definition, default_maximum_distance, 0x1c);
CHECK_OFFSET(sound_class_definition, muted, 0x28);

// game_looping_sound
CHECK_OFFSET(game_looping_sound, flags, 0x04);
CHECK_OFFSET(game_looping_sound, scale, 0x08);
CHECK_OFFSET(game_looping_sound, definition_index, 0x0c);
CHECK_OFFSET(game_looping_sound, object_index, 0x10);
CHECK_OFFSET(game_looping_sound, last_update, 0x14);
CHECK_OFFSET(game_looping_sound, function_index, 0x18);
CHECK_OFFSET(game_looping_sound, node_index, 0x1a);
CHECK_OFFSET(game_looping_sound, position, 0x1c);
CHECK_OFFSET(game_looping_sound, forward, 0x28);

// sound
CHECK_OFFSET_P(sound, play_state, 0x02);
CHECK_OFFSET_P(sound, flags, 0x04);
CHECK_OFFSET_P(sound, listener_index, 0x06);
CHECK_OFFSET_P(sound, definition_index, 0x08);
CHECK_OFFSET_P(sound, owner_index, 0x0c);
CHECK_OFFSET_P(sound, location_proc, 0x10);
CHECK_OFFSET_P(sound, location, 0x14);
CHECK_OFFSET_P(sound, callback_data, 0x54);
CHECK_OFFSET_P(sound, start_time, 0x84);
CHECK_OFFSET_P(sound, pitch, 0x88);
CHECK_OFFSET_P(sound, channel_index, 0x8c);
CHECK_OFFSET_P(sound, pitch_range_index, 0x8e);
CHECK_OFFSET_P(sound, permutation_index, 0x90);
CHECK_OFFSET_P(sound, fade_curve, 0x92);
CHECK_OFFSET_P(sound, track_index, 0x94);
CHECK_OFFSET_P(sound, pending_definition_index, 0x98);
CHECK_OFFSET_P(sound, fade_start_gain, 0x9c);
CHECK_OFFSET_P(sound, fade_end_gain, 0xa0);
CHECK_OFFSET_P(sound, fade_start_time, 0xa4);
CHECK_OFFSET_P(sound, fade_end_time, 0xa8);
CHECK_OFFSET_P(sound, first_person, 0xac);

// looping_sound
CHECK_OFFSET(looping_sound, definition_index, 0x04);
CHECK_OFFSET(looping_sound, owner, 0x08);
CHECK_OFFSET(looping_sound, location, 0x0c);
CHECK_OFFSET(looping_sound, update_toggle, 0x4c);
CHECK_OFFSET(looping_sound, alternate, 0x4d);
CHECK_OFFSET(looping_sound, finished, 0x4e);
CHECK_OFFSET(looping_sound, active_sound_count, 0x50);
CHECK_OFFSET(looping_sound, state, 0x52);
CHECK_OFFSET(looping_sound, detail_next_time, 0x54);
CHECK_OFFSET(looping_sound, track_sounds, 0xd4);

// sound_channel (0x00724a60 base: 0x00724a64 flags, 0x00724a68 time, 0x00724a70 / 74 permutations)
CHECK_OFFSET_P(sound_channel, type_flags, 0x04);
CHECK_OFFSET_P(sound_channel, play_time, 0x08);
CHECK_OFFSET_P(sound_channel, current_pitch, 0x0c);
CHECK_OFFSET_P(sound_channel, current_permutation, 0x10);
CHECK_OFFSET_P(sound_channel, next_permutation, 0x14);

// sound_listener (0x00725218 base: 0x0072521c matrix, 0x00725244 position, 0x00725250 velocity)
CHECK_OFFSET(sound_listener, scale, 0x04);
CHECK_OFFSET(sound_listener, position, 0x2c);
CHECK_OFFSET(sound_listener, velocity, 0x38);
CHECK_OFFSET_P(sound_listener_parameters, velocity, 0x24);
CHECK_OFFSET_P(sound_listener_parameters, environment, 0x30);

// sound_driver (.data 0x0069f4c8)
CHECK_OFFSET_P(sound_driver, initialize, 0x04);
CHECK_OFFSET_P(sound_driver, set_listener, 0x0c);
CHECK_OFFSET_P(sound_driver, channel_play, 0x18);
CHECK_OFFSET_P(sound_driver, set_paused, 0x28);
CHECK_OFFSET_P(sound_driver, channel_set_spatial, 0x30);
CHECK_OFFSET_P(sound_driver, set_quality, 0x38);
CHECK_OFFSET_P(sound_driver, eax_available, 0x3c);
CHECK_OFFSET(sound_driver_parameters, channel_counts, 0x02);
CHECK_OFFSET(sound_driver_parameters, slot_counts, 0x0a);

// directsound_channel (0x00725430 base)
CHECK_OFFSET_P(directsound_channel, sound_class, 0x04);
CHECK_OFFSET_P(directsound_channel, streaming, 0x09);
CHECK_OFFSET_P(directsound_channel, position, 0x0c);
CHECK_OFFSET_P(directsound_channel, cone_orientation, 0x18);
CHECK_OFFSET_P(directsound_channel, velocity, 0x24);
CHECK_OFFSET_P(directsound_channel, type_flags, 0x38);
CHECK_OFFSET_P(directsound_channel, gain, 0x3c);
CHECK_OFFSET_P(directsound_channel, occlusion, 0x48);
CHECK_OFFSET_P(directsound_channel, eax_value, 0x60);
CHECK_OFFSET_P(directsound_channel, buffer_size, 0x68);
CHECK_OFFSET_P(directsound_channel, frequency, 0x6c);
CHECK_OFFSET_P(directsound_channel, volume, 0x70);
CHECK_OFFSET_P(directsound_channel, write_cursor, 0x78);
CHECK_OFFSET_P(directsound_channel, source_end_cursor, 0x7c);
CHECK_OFFSET_P(directsound_channel, streaming_bytes, 0x84);
CHECK_OFFSET_P(directsound_channel, source, 0x88);
CHECK_OFFSET_P(directsound_channel, next_source, 0x8c);
CHECK_OFFSET_P(directsound_channel, source_crosslap, 0x90);
CHECK_OFFSET_P(directsound_channel, source_started, 0x98);
CHECK_OFFSET_P(directsound_channel, decoder, 0xa0);
CHECK_OFFSET_P(directsound_channel, buffer, 0x670);
CHECK_OFFSET_P(directsound_channel, buffer_3d, 0x674);
CHECK_OFFSET_P(sound_stream_decoder, ogg_vorbis_file, 0x08);
CHECK_OFFSET_P(sound_stream_decoder, open, 0x5a8);
CHECK_OFFSET_P(sound_stream_decoder, active_file, 0x5a9);
CHECK_OFFSET_P(sound_stream_decoder, memory_files, 0x5ac);
// 0x00725a78 (cleared by 0x547f60) is channel 0 decoder.open
CHECK_P(check_decoder_open_in_channel, offsetof(directsound_channel, decoder) + 0x5a8 == 0x648);

// effect objects
CHECK_OFFSET_P(sound_effect_object_vtable, apply_listener, 0x1c);
CHECK_OFFSET_P(sound_effect_object, supported_properties, 0x08);
CHECK_OFFSET_P(sound_effect_object, property_set, 0x18);
CHECK_OFFSET_P(sound_eax_effect_object, channel_property_sets, 0x1c);

// tag layouts this module leans on (types/tags.h)
CHECK_OFFSET(Sound, sound_class, 0x04);
CHECK_OFFSET(Sound, zero_skip_fraction_modifier, 0x3c);
CHECK_OFFSET(Sound, one_pitch_modifier, 0x5c);
CHECK_OFFSET(Sound, channel_count, 0x6c);
CHECK_OFFSET(Sound, format, 0x6e);
CHECK_OFFSET(Sound, promotion_count, 0x80);
CHECK_OFFSET(Sound, longest_permutation_length, 0x84);
CHECK_OFFSET(Sound, scripting_time, 0x90);
CHECK_OFFSET(Sound, scripting_sound, 0x94);
CHECK_OFFSET(Sound, pitch_ranges, 0x98);
CHECK_OFFSET(SoundPitchRange, bend_bounds, 0x24);
CHECK_OFFSET(SoundPitchRange, playback_rate, 0x30);
CHECK_OFFSET(SoundPitchRange, last_permutation_index, 0x38);
CHECK_OFFSET(SoundPitchRange, permutations, 0x3c);
CHECK_OFFSET(SoundPermutation, format, 0x28);
CHECK_OFFSET(SoundPermutation, samples_pointer, 0x2c);
CHECK_OFFSET(SoundPermutation, buffer_size, 0x38);
CHECK_OFFSET(SoundPermutation, samples, 0x40);
CHECK_OFFSET(SoundPermutation, mouth_data, 0x54);
CHECK_OFFSET(SoundLooping, runtime_scripting_sound, 0x1c);
CHECK_OFFSET(SoundLooping, maximum_distance, 0x20);
CHECK_OFFSET(SoundLooping, tracks, 0x3c);
CHECK_OFFSET(SoundLooping, detail_sounds, 0x48);
CHECK_OFFSET(SoundLoopingTrack, loop, 0x40);
CHECK_OFFSET(SoundLoopingTrack, alternate_end, 0x90);
CHECK_OFFSET(SoundLoopingDetail, gain, 0x18);
CHECK_OFFSET(SoundLoopingDetail, flags, 0x1c);
CHECK_OFFSET(SoundLoopingDetail, yaw_bounds, 0x50);
CHECK_OFFSET(SoundLoopingDetail, distance_bounds, 0x60);
CHECK_SIZE(SoundEnvironment, 0x48);
CHECK_SIZE(SoundLoopingTrack, 0xa0);

// array spans that fix k_maximum_sound_channels and the table bounds
CHECK_P(check_channel_span, 0x00725430 + 81 * sizeof(directsound_channel) == 0x00746028);
typedef char check_binding_span[(0x007252e4 + 81 * sizeof(sound_channel_binding) == 0x00725428) ? 1 : -1];
CHECK_P(check_slot_span, 0x00724a60 + 81 * sizeof(sound_channel) == 0x007251f8);
typedef char check_listener_span[(0x00725218 + sizeof(sound_listener) == 0x0072525c) ? 1 : -1];
typedef char check_environment_span[(0x0072525c + sizeof(SoundEnvironment) == 0x007252a4) ? 1 : -1];
typedef char check_class_span[(0x0069eae0 + 51 * sizeof(sound_class_definition) == 0x0069f3a4) ? 1 : -1];
CHECK_P(check_driver_span, 0x0069f4c8 + sizeof(sound_driver) == 0x0069f508);
typedef char check_parameters_span[(0x0069f514 + sizeof(sound_driver_parameters) == 0x0069f528) ? 1 : -1];
