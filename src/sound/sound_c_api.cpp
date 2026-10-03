/**
 * @file src/sound/sound_c_api.cpp
 * The sound module's C ABI: one extern "C" function per original symbol, same name, signature and calling
 * convention, forwarding to the halo::sound implementation. The link tables, the code-pointer slots,
 * standalone/data/*.c and the unconverted modules call these names.
 */

#include "internal/state.hpp"

extern "C" void sound_environment_update(uint32_t *out_environment_ptr, void **out_environment_slot, uint8_t *out_changed)
{
    halo::sound::spatial::environment_update(out_environment_ptr, out_environment_slot, out_changed);
}

extern "C" void game_sound_initialize(void)
{
    halo::sound::game_looping::initialize();
}

extern "C" void game_sound_revert_scripting_sounds(void)
{
    halo::sound::game_looping::revert_scripting_sounds();
}

extern "C" void game_sound_reconcile_scripting_state(void)
{
    halo::sound::game_looping::reconcile_scripting_state();
}

extern "C" datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name, int16_t function_index)
{
    return halo::sound::game_looping::create(object_index, definition_index, marker_name, function_index);
}

extern "C" datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint)
{
    return halo::sound::instances::start_at_object_marker(object_index, position, forward, definition_index, node_index, scale, first_person_hint);
}

extern "C" datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale)
{
    return halo::sound::instances::start_at_location(definition_index, placement, scale);
}

extern "C" datum_index sound_start_unspatialized(datum_index definition_index, float scale)
{
    return halo::sound::instances::start_unspatialized(definition_index, scale);
}

extern "C" void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale)
{
    halo::sound::instances::impulse_start(object_index, definition_index, scale);
}

extern "C" int32_t sound_impulse_time(datum_index sound_tag_handle)
{
    return halo::sound::instances::impulse_time(sound_tag_handle);
}

extern "C" void sound_looping_predict(datum_index looping_definition)
{
    halo::sound::looping::predict(looping_definition);
}

extern "C" void sound_looping_start(datum_index definition_index, datum_index object_index, float scale)
{
    halo::sound::looping::start(definition_index, object_index, scale);
}

extern "C" void sound_looping_stop(datum_index looping_definition)
{
    halo::sound::looping::stop(looping_definition);
}

extern "C" void sound_looping_set_scale(datum_index looping_definition, float gain)
{
    halo::sound::looping::set_scale(looping_definition, gain);
}

extern "C" void sound_looping_set_alternate(datum_index looping_definition, uint8_t alternate)
{
    halo::sound::looping::set_alternate(looping_definition, alternate);
}

extern "C" datum_index sound_looping_start_ambient(datum_index object_index, datum_index definition_index, float scale)
{
    return halo::sound::looping::start_ambient(object_index, definition_index, scale);
}

extern "C" void game_looping_sound_touch_if_valid(datum_index looping_sound_index)
{
    halo::sound::game_looping::touch_if_valid(looping_sound_index);
}

extern "C" void game_looping_sound_update(datum_index looping_sound_index, int32_t *root_location)
{
    halo::sound::game_looping::update_sound(looping_sound_index, root_location);
}

extern "C" void game_sound_update(void)
{
    halo::sound::game_looping::update();
}

extern "C" uint8_t sound_location_object_marker(datum_index owner, void *callback_data, sound_location *location)
{
    return halo::sound::instances::object_marker_location_proc(owner, callback_data, location);
}

extern "C" void sound_build_cluster_range_bitmap(void)
{
    halo::sound::spatial::build_cluster_range_bitmap();
}

extern "C" void sound_compute_obstruction_occlusion(sound_location *location, int16_t listener_index, float reference_distance)
{
    halo::sound::view(location)->compute_obstruction_occlusion(listener_index, reference_distance);
}

extern "C" uint32_t sound_looping_definition_has_music_loop(datum_index looping_definition)
{
    return halo::sound::looping::definition_has_music_loop(looping_definition);
}

extern "C" void game_sound_stop_loops_conflicting_with_music(void)
{
    halo::sound::game_looping::stop_loops_conflicting_with_music();
}

extern "C" uint32_t ov_read_thunk(void *destination, uint32_t size, uint32_t count, sound_ogg_memory_file *file)
{
    return halo::sound::stream::read_memory_file(destination, size, count, file);
}

extern "C" int32_t ov_seek_thunk(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    return halo::sound::stream::seek_memory_file(file, offset_low, offset_high, whence);
}

extern "C" int32_t ov_close_thunk(sound_ogg_memory_file *file)
{
    return halo::sound::stream::close_memory_file(file);
}

extern "C" int32_t ov_tell_thunk(sound_ogg_memory_file *file)
{
    return halo::sound::stream::tell_memory_file(file);
}

extern "C" int32_t sound_ogg_seek_callback(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    return halo::sound::view(file)->seek_to(offset_low, offset_high, whence);
}

extern "C" uint8_t sound_ogg_stream_open(sound_stream_decoder *decoder, void *data, int32_t size)
{
    return halo::sound::view(decoder)->open_stream(data, size);
}

extern "C" void sound_ogg_error_to_string(int32_t vorbis_error_code)
{
    halo::sound::stream::error_to_string(vorbis_error_code);
}

extern "C" int32_t sound_ogg_stream_read(sound_stream_decoder *decoder, char *buffer, int32_t size, char *want_crosslap)
{
    return halo::sound::view(decoder)->read_stream(buffer, size, want_crosslap);
}

extern "C" void sound_class_update_gain_fade(int32_t ticks)
{
    halo::sound::classes::update_gain_fade(ticks);
}

extern "C" void sound_class_set_gain_by_name(char *name, float gain, int16_t ticks)
{
    halo::sound::classes::set_gain_by_name(name, gain, ticks);
}

extern "C" void sound_class_set_muted_by_name(uint8_t enabled, char *name)
{
    halo::sound::classes::set_muted_by_name(enabled, name);
}

extern "C" float sound_definition_maximum_distance(datum_index sound_definition)
{
    return halo::sound::definitions::maximum_distance(sound_definition);
}

extern "C" int16_t sound_permutation_pick_for_pitch(int16_t pitch_range_index, Sound *tag, float target_pitch)
{
    return halo::sound::definitions::pick_pitch_range(pitch_range_index, tag, target_pitch);
}

extern "C" int16_t sound_permutation_pick_random(int16_t pitch_range_index, int16_t explicit_permutation_index, Sound *tag)
{
    return halo::sound::definitions::pick_permutation(pitch_range_index, explicit_permutation_index, tag);
}

extern "C" int32_t sound_linear_gain_to_attenuation(float gain, int32_t maximum)
{
    return halo::sound::gain::linear_to_attenuation(gain, maximum);
}

extern "C" void sound_stream_decoder_close_slot(sound_stream_decoder *decoder, uint8_t smart_toggle)
{
    halo::sound::view(decoder)->close_slot(smart_toggle);
}

extern "C" int32_t sound_pcm_buffer_read(uint32_t *position, SoundPermutation *permutation, uint32_t *bytes_read_out, uint32_t requested_size, void *destination)
{
    return halo::sound::stream::pcm_buffer_read(position, permutation, bytes_read_out, requested_size, destination);
}

extern "C" uint32_t sound_ogg_buffer_fill(SoundPermutation *permutation, void *destination, uint32_t requested_size, char *want_crosslap, sound_stream_decoder *decoder, uint32_t *bytes_filled_out)
{
    return halo::sound::view(decoder)->fill_buffer(permutation, destination, requested_size, want_crosslap, bytes_filled_out);
}

extern "C" void sound_directsound_probe_channel_pools(int32_t *mono3d_count, uint32_t mono3d_requested, int32_t *mono_count, uint32_t mono_requested, int32_t *stereo_count, uint32_t stereo_requested, int32_t *stereo44k_count, uint32_t stereo44k_requested, uint32_t pool_mask)
{
    halo::sound::directsound_device().probe_channel_pools(mono3d_count, mono3d_requested, mono_count, mono_requested, stereo_count, stereo_requested, stereo44k_count, stereo44k_requested, pool_mask);
}

extern "C" uint8_t sound_driver_initialize(sound_driver_parameters *parameters)
{
    return halo::sound::directsound_device().initialize(parameters);
}

extern "C" uint8_t sound_channel_create(int16_t channel_index, uint16_t type_flags)
{
    return halo::sound::directsound_device().create_channel(channel_index, type_flags);
}

extern "C" void sound_driver_dispose(void)
{
    halo::sound::directsound_device().dispose();
}

extern "C" void sound_update_streaming_channels(void)
{
    halo::sound::channels::update_streaming();
}

extern "C" void sound_driver_end_frame(void)
{
    halo::sound::directsound_device().end_frame();
}

extern "C" void sound_driver_begin_frame(void)
{
    halo::sound::directsound_device().begin_frame();
}

extern "C" void sound_driver_stop_all(void)
{
    halo::sound::directsound_device().stop_all();
}

extern "C" void sound_driver_set_paused(uint8_t paused)
{
    halo::sound::directsound_device().set_paused(paused);
}

extern "C" void sound_listener_update(sound_listener_parameters *parameters)
{
    halo::sound::directsound_device().set_listener(parameters);
}

extern "C" void sound_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    halo::sound::directsound_device().commit_spatial(channel_index, spatialized, spatial, obstruction, occlusion, underwater, sound_class);
}

extern "C" void sound_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update)
{
    halo::sound::directsound_device().commit_parameters(channel_index, parameters, update);
}

extern "C" uint32_t sound_channel_refresh_cursor(int16_t channel_index)
{
    return halo::sound::directsound_device().refresh_cursor(channel_index);
}

extern "C" void sound_channel_stream_update(int16_t channel_index, uint8_t unused)
{
    halo::sound::directsound_device().stream_update(channel_index, unused);
}

extern "C" uint8_t sound_channel_lock_and_fill(int16_t channel_index, uint32_t fill_size)
{
    return halo::sound::directsound_device().lock_and_fill(channel_index, fill_size);
}

extern "C" void sound_channel_fill_pcm_data(int16_t channel_index, uint8_t *destination, int32_t base_position, uint8_t *crosslap, int32_t byte_count)
{
    halo::sound::directsound_device().fill_pcm_data(channel_index, destination, base_position, crosslap, byte_count);
}

extern "C" int32_t sound_channel_restore_buffer(void *buffer, uint8_t *was_restored_out)
{
    return halo::sound::directsound_device().restore_buffer(buffer, was_restored_out);
}

extern "C" void sound_channel_queue_source(int16_t channel_index, SoundPermutation *source, int16_t sound_class, uint8_t crosslap)
{
    halo::sound::directsound_device().queue_source(channel_index, source, sound_class, crosslap);
}

extern "C" void sound_channel_reset(int16_t channel_index)
{
    halo::sound::directsound_device().reset_channel(channel_index);
}

extern "C" uint32_t sound_channel_claim_if_finished(int16_t channel_index)
{
    return halo::sound::directsound_device().claim_if_finished(channel_index);
}

extern "C" directsound_channel_state sound_channel_check_loop_boundary(int16_t channel_index)
{
    return halo::sound::directsound_device().check_loop_boundary(channel_index);
}

extern "C" void sound_driver_set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality)
{
    halo::sound::directsound_device().set_quality(unknown, eax_enabled, quality);
}

extern "C" void sound_pause(void)
{
    halo::sound::engine::pause();
}

extern "C" void sound_resume(void)
{
    halo::sound::engine::resume();
}

extern "C" void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force)
{
    halo::sound::directsound_device().set_eax_enabled(eax_enabled, force);
}

extern "C" uint8_t sound_driver_eax_available(void)
{
    return halo::sound::directsound_device().eax_available();
}

extern "C" void sound_channel_bind_hardware(int16_t logical_channel_index)
{
    halo::sound::directsound_device().bind_hardware(logical_channel_index);
}

extern "C" void sound_driver_channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap)
{
    halo::sound::directsound_device().channel_play(channel_index, source, unused, sound_class, crosslap);
}

extern "C" void sound_driver_channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class)
{
    halo::sound::directsound_device().channel_continue(channel_index, unused, sound_class);
}

extern "C" void sound_driver_channel_stop(int16_t channel_index)
{
    halo::sound::directsound_device().channel_stop(channel_index);
}

extern "C" directsound_channel_state sound_driver_channel_get_state(int16_t channel_index)
{
    return halo::sound::directsound_device().channel_get_state(channel_index);
}

extern "C" void sound_driver_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    halo::sound::directsound_device().channel_set_spatial(channel_index, spatialized, spatial, obstruction, occlusion, underwater, sound_class);
}

extern "C" void sound_driver_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)
{
    halo::sound::directsound_device().channel_set_parameters(channel_index, parameters, unknown);
}

extern "C" uint8_t sound_channel_type_flags_match(int16_t compressed_requested, int16_t stereo_requested, uint16_t sample_rate_44khz_requested, uint16_t flags, int16_t requested_3d)
{
    return halo::sound::channels::type_flags_match(compressed_requested, stereo_requested, sample_rate_44khz_requested, flags, requested_3d);
}

extern "C" void sound_set_master_gain(float gain)
{
    halo::sound::classes::set_master_gain(gain);
}

extern "C" void sound_set_music_gain(float gain)
{
    halo::sound::classes::set_music_gain(gain);
}

extern "C" void sound_set_effects_gain(float gain)
{
    halo::sound::classes::set_effects_gain(gain);
}

extern "C" void sound_initialize(void)
{
    halo::sound::engine::initialize();
}

extern "C" uint8_t sound_reopen_device(sound_driver_parameters *new_parameters)
{
    return halo::sound::engine::reopen_device(new_parameters);
}

extern "C" void sound_fade_out_and_stop_all(void)
{
    halo::sound::instances::fade_out_and_stop_all();
}

extern "C" void sound_dispose(void)
{
    halo::sound::engine::dispose();
}

extern "C" void sound_update(void)
{
    halo::sound::engine::update();
}

extern "C" void sound_idle_update(void)
{
    halo::sound::engine::idle_update();
}

extern "C" void sounds_refresh_structure_locations(void)
{
    halo::sound::spatial::refresh_structure_locations();
}

extern "C" datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint)
{
    return halo::sound::instances::play_new(definition_index, location, owner_index, location_proc, callback_data, callback_data_size, first_person_hint);
}

extern "C" void sound_impulse_fade_out(datum_index sound_index)
{
    halo::sound::instances::impulse_fade_out(sound_index);
}

extern "C" void sound_looping_datum_touch(int32_t reference)
{
    halo::sound::looping::touch(reference);
}

extern "C" uint8_t sound_looping_set_state(int32_t owner, datum_index definition_index, sound_location *location, int16_t state, uint8_t alternate, float fade_duration)
{
    return halo::sound::looping::set_state(owner, definition_index, location, state, alternate, fade_duration);
}

extern "C" void sound_stop_all(void)
{
    halo::sound::instances::stop_all();
}

extern "C" void sound_update_clock(void)
{
    halo::sound::engine::update_clock();
}

extern "C" float sound_compute_random_pitch(float pitch_bounds_min, float pitch_bounds_max, float zero_pitch_modifier, float one_pitch_modifier, float distance_scale)
{
    return halo::sound::definitions::compute_random_pitch(pitch_bounds_min, pitch_bounds_max, zero_pitch_modifier, one_pitch_modifier, distance_scale);
}

extern "C" uint32_t sound_definition_has_audible_permutations(TagID sound_tag_id)
{
    return halo::sound::definitions::has_audible_permutations(sound_tag_id);
}

extern "C" void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds, datum_index fade_out_handle)
{
    halo::sound::instances::schedule_gain_fade(fade_in_handle, fade_curve, duration_seconds, fade_out_handle);
}

extern "C" int16_t sound_definition_check_promotion(TagID sound_tag_id)
{
    return halo::sound::definitions::check_promotion(sound_tag_id);
}

extern "C" float sound_compute_class_gain(SoundClass_t sound_class)
{
    return halo::sound::classes::compute_gain(sound_class);
}

extern "C" void sound_instance_stop(datum_index sound_handle)
{
    halo::sound::instances::stop(sound_handle);
}

extern "C" void sound_update_listener(void)
{
    halo::sound::spatial::update_listener();
}

extern "C" int16_t sound_location_check_audibility(sound_location *location, float max_distance)
{
    return halo::sound::view(location)->check_audibility(max_distance);
}

extern "C" float sound_location_distance_squared(int16_t listener_index, sound_location *location)
{
    return halo::sound::spatial::location_distance_squared(listener_index, location);
}

extern "C" float sound_location_distance(int16_t listener_index, sound_location *location)
{
    return halo::sound::spatial::location_distance(listener_index, location);
}

extern "C" uint32_t sound_instance_invoke_location_proc(datum_index sound_handle)
{
    return halo::sound::instances::invoke_location_proc(sound_handle);
}

extern "C" void sound_update_range_and_ducking(void)
{
    halo::sound::spatial::update_range_and_ducking();
}

extern "C" void sound_assign_channels(void)
{
    halo::sound::channels::assign();
}

extern "C" void sound_build_channel_candidates(sound_channel_candidate_list *out, datum_index sound_handle)
{
    halo::sound::channels::build_candidates(out, sound_handle);
}

extern "C" int16_t sound_pick_channel_for_instance(datum_index sound_handle)
{
    return halo::sound::channels::pick_for_instance(sound_handle);
}

extern "C" int16_t sound_find_lowest_priority_channel(datum_index sound_handle)
{
    return halo::sound::channels::find_lowest_priority(sound_handle);
}

extern "C" int16_t sound_pick_replaceable_channel(datum_index sound_handle, int16_t *candidate_channels, int16_t count)
{
    return halo::sound::channels::pick_replaceable(sound_handle, candidate_channels, count);
}

extern "C" uint32_t sound_compare_priority(datum_index sound_a, datum_index sound_b, float distance_a_squared)
{
    return halo::sound::channels::compare_priority(sound_a, sound_b, distance_a_squared);
}

extern "C" void sound_update_instance_gain(int16_t channel_index, float external_gain_multiplier)
{
    halo::sound::instances::update_gain(channel_index, external_gain_multiplier);
}

extern "C" void sound_update_active_instances(void)
{
    halo::sound::instances::update_active();
}

extern "C" void sound_channel_set_next_permutation(int16_t channel_index, SoundPermutation *permutation, int16_t unknown, int16_t sound_class, uint8_t streaming)
{
    halo::sound::channels::set_next_permutation(channel_index, permutation, unknown, sound_class, streaming);
}

extern "C" int32_t sound_linear_gain_to_millibels_clamped(int32_t minimum, float gain, int32_t bias_and_maximum)
{
    return halo::sound::gain::linear_to_millibels_clamped(minimum, gain, bias_and_maximum);
}

extern "C" float sound_evaluate_volume_curve(int32_t attenuation, int32_t minimum, int32_t maximum)
{
    return halo::sound::gain::evaluate_volume_curve(attenuation, minimum, maximum);
}

extern "C" void sound_channel_parameters_proc_default(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class)
{
    halo::sound::channels::apply_default_parameters(channel_index, parameters, update, sound_class);
}

extern "C" void sound_channel_parameters_proc_eax(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class)
{
    halo::sound::channels::apply_eax_parameters(channel_index, parameters, update, sound_class);
}

extern "C" int16_t sound_channel_release_detail_buffers(int16_t channel_index)
{
    return halo::sound::channels::release_detail_buffers(channel_index);
}

extern "C" void sound_channel_release_permutations(int16_t channel_index)
{
    halo::sound::channels::release_permutations(channel_index);
}

extern "C" datum_index sound_looping_state_new(datum_index definition_index, int32_t owner, sound_location *location)
{
    return halo::sound::looping::state_new(definition_index, owner, location);
}

extern "C" void sound_update_looping_states(void)
{
    halo::sound::looping::update_states();
}

extern "C" datum_index sound_looping_create_detail_sound(datum_index owner, datum_index definition_index, int16_t track_index, int16_t play_state)
{
    return halo::sound::looping::create_detail_sound(owner, definition_index, track_index, play_state);
}

extern "C" uint8_t sound_looping_track_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    return halo::sound::looping::track_location_proc(owner, callback_data, location);
}

extern "C" uint8_t sound_looping_detail_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    return halo::sound::looping::detail_location_proc(owner, callback_data, location);
}

extern "C" void sound_instance_queue_definition_switch(datum_index sound_handle, datum_index new_definition_index)
{
    halo::sound::instances::queue_definition_switch(sound_handle, new_definition_index);
}

extern "C" void sound_instance_apply_pending_definition_switch(datum_index sound_handle)
{
    halo::sound::instances::apply_pending_definition_switch(sound_handle);
}

extern "C" void sound_update_looping_gain(int16_t channel_index, float external_gain_multiplier)
{
    halo::sound::looping::update_gain(channel_index, external_gain_multiplier);
}

extern "C" float sound_evaluate_fade_gain(datum_index sound_handle)
{
    return halo::sound::instances::evaluate_fade_gain(sound_handle);
}

extern "C" void sound_random_detail_direction(SoundLoopingDetail *detail, real_vector3d *out)
{
    halo::sound::definitions::random_detail_direction(detail, out);
}

extern "C" datum_index sound_looping_find_by_owner(int32_t owner)
{
    return halo::sound::looping::find_by_owner(owner);
}

extern "C" float sound_clamp_gain_by_ratio(float gain, float compare, float ratio)
{
    return halo::sound::gain::clamp_by_ratio(gain, compare, ratio);
}

extern "C" void render_debug_sound(datum_index sound_handle)
{
    halo::sound::instances::render_debug(sound_handle);
}

extern "C" void sound_looping_check_audibility_gate(datum_index definition_index, sound_location *location)
{
    halo::sound::looping::check_audibility_gate(definition_index, location);
}

extern "C" uint8_t sound_scenery_create(datum_index object_index)
{
    return halo::sound::instances::scenery_create(object_index);
}

extern "C" int32_t sound_decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size)
{
    return halo::sound::codec::decode_dispatch(channel_count, destination, source, source_size);
}

extern "C" int32_t sound_adpcm_decode_sample(uint8_t selector, int32_t prediction, uint32_t step)
{
    return halo::sound::codec::decode_sample(selector, prediction, step);
}

extern "C" int32_t sound_adpcm_decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    return halo::sound::codec::decode_mono(source, destination, block_count, block_size, samples_per_block, out_0, out_1);
}

extern "C" int32_t sound_adpcm_decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    return halo::sound::codec::decode_stereo(source, destination, block_count, block_size, samples_per_block, out_0, out_1);
}

extern "C" void sound_eax1_effect_shutdown(sound_effect_object *this_object)
{
    halo::sound::eax1_backend().shutdown(this_object);
}

extern "C" int32_t sound_eax1_effect_initialize(sound_effect_object *this_object, directsound_channel *channel, int32_t unused)
{
    return halo::sound::eax1_backend().initialize(this_object, channel, unused);
}

extern "C" void sound_eax1_effect_apply_listener(sound_effect_object *this_object, SoundEnvironment *environment)
{
    halo::sound::eax1_backend().apply_listener(this_object, environment);
}

extern "C" void __thiscall sound_eax1_effect_set_environment_index(sound_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax1_backend().set_environment_index(this_object, environment);
}

extern "C" void __thiscall sound_eax1_effect_set_room_gain(sound_effect_object *this_object, float gain)
{
    return halo::sound::eax1_backend().set_room_gain(this_object, gain);
}

extern "C" int32_t sound_gain_to_directsound_volume(float gain, int32_t maximum)
{
    return halo::sound::gain::to_directsound_volume(gain, maximum);
}

extern "C" int __cdecl sound_gain_to_millibels(float gain)
{
    return halo::sound::gain::to_millibels(gain);
}

extern "C" int32_t __thiscall sound_effect_object_listener_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().listener_supported(this_object);
}

extern "C" int32_t __thiscall sound_effect_object_channel_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().channel_supported(this_object);
}

extern "C" void __thiscall sound_eax20_effect_shutdown(sound_eax_effect_object *this_object)
{
    return halo::sound::eax2_backend().shutdown(reinterpret_cast<sound_effect_object *>(this_object));
}

extern "C" int __thiscall sound_eax20_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused)
{
    return halo::sound::eax2_backend().initialize(reinterpret_cast<sound_effect_object *>(this_object), listener, unused);
}

extern "C" int32_t __thiscall sound_eax_effect_initialize_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax2_backend().initialize_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}

extern "C" void __thiscall sound_eax20_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax2_backend().apply_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}

extern "C" void __thiscall sound_eax20_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
{
    return halo::sound::eax2_backend().apply_listener(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

extern "C" void __thiscall sound_eax20_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax2_backend().set_environment_index(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

extern "C" void __thiscall sound_eax20_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
{
    return halo::sound::eax2_backend().set_room_gain(reinterpret_cast<sound_effect_object *>(this_object), gain);
}

extern "C" void __thiscall sound_eax30_effect_shutdown(sound_eax_effect_object *this_object)
{
    return halo::sound::eax3_backend().shutdown(reinterpret_cast<sound_effect_object *>(this_object));
}

extern "C" int __thiscall sound_eax30_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener, int32_t unused)
{
    return halo::sound::eax3_backend().initialize(reinterpret_cast<sound_effect_object *>(this_object), listener, unused);
}

extern "C" void __thiscall sound_eax30_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax3_backend().apply_channel(reinterpret_cast<sound_effect_object *>(this_object), channel_index);
}

extern "C" float sound_reverb_size_scale(float value)
{
    return halo::sound::gain::reverb_size_scale(value);
}

extern "C" void __thiscall sound_eax30_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
{
    return halo::sound::eax3_backend().apply_listener(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

extern "C" void __thiscall sound_eax30_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    return halo::sound::eax3_backend().set_environment_index(reinterpret_cast<sound_effect_object *>(this_object), environment);
}

extern "C" void __thiscall sound_eax30_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
{
    return halo::sound::eax3_backend().set_room_gain(reinterpret_cast<sound_effect_object *>(this_object), gain);
}

extern "C" int32_t __thiscall sound_eax1_effect_initialize_channel(sound_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax1_backend().initialize_channel(this_object, channel_index);
}

extern "C" int32_t __thiscall sound_eax1_effect_channel_supported(sound_effect_object *this_object)
{
    return halo::sound::eax1_backend().channel_supported(this_object);
}

extern "C" void __thiscall sound_eax1_effect_apply_channel(sound_effect_object *this_object, int32_t channel_index)
{
    return halo::sound::eax1_backend().apply_channel(this_object, channel_index);
}

extern "C" int32_t sound_effects_object_detect_mode(int16_t channel_index, directsound_channel *channel)
{
    return halo::sound::effects::detect_mode(channel_index, channel);
}

extern "C" void __cdecl sound_effects_object_shutdown(void)
{
    return halo::sound::effects::shutdown();
}

extern "C" int32_t sound_effects_object_initialize_channel(int16_t channel_index)
{
    return halo::sound::effects::initialize_channel(channel_index);
}

extern "C" int __cdecl sound_effects_object_apply_all_channels(void)
{
    return halo::sound::effects::apply_all_channels();
}

extern "C" void __cdecl sound_effects_object_reinitialize(int enable)
{
    return halo::sound::effects::reinitialize(enable);
}
