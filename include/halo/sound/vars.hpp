/**
 * @file include/halo/sound/vars.hpp
 * Addresses of the engine variables the sound module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/sound_vars.hpp.
 */
#pragma once

namespace halo::sound {

/** Address table of the engine variables owned by the sound module. */
struct Vars {
    void *adpcm_index_table;
    void *adpcm_step_table;
    void *camera_leaf;
    void *current_sound_driver;
    void *debug_sound;
    void *debug_sound_channel_details;
    void *debug_sound_channels;
    void *directsound;
    void *directsound_binding_count;
    void *directsound_bindings;
    void *directsound_caps;
    void *directsound_channel_count;
    void *directsound_channels;
    void *directsound_deferred_dirty;
    void *directsound_environment_cache;
    void *directsound_fade;
    void *directsound_first_channel_of_type;
    void *directsound_hardware_3d_channel_count;
    void *directsound_hardware_mode;
    void *directsound_listener_cached;
    void *directsound_paused;
    void *directsound_primary_buffer;
    void *directsound_rolloff_factor;
    void *driver_parameters;
    void *ds3dalg_hrtf_full;
    void *error_text_buffer;
    void *game_sound_globals_ptr;
    void *iid_directsound_3d_buffer;
    void *iid_directsound_3d_listener;
    void *k_sound_decode_procs;
    void *k_sound_sample_rates;
    void *local_player_0_cluster_index;
    void *looping_sound_data;
    void *sound_channel_count;
    void *sound_channel_parameters_proc_ptr;
    void *sound_channel_type_flag_table;
    void *sound_channels;
    void *sound_class_definitions;
    void *sound_class_names;
    void *sound_cluster_audible_bitmap;
    void *sound_data;
    void *sound_decode_proc;
    void *sound_delay_per_world_unit;
    void *sound_dialog_ducking_gain;
    void *sound_dialog_unspatialized;
    void *sound_disabled;
    void *sound_drivers;
    void *sound_ducking_gain;
    void *sound_eax1_vtable;
    void *sound_eax20_buffer_property_guid;
    void *sound_eax20_listener_property_guid;
    void *sound_eax20_underwater_direct_gain;
    void *sound_eax2_vtable;
    void *sound_eax30_buffer_property_guid;
    void *sound_eax30_listener_property_guid;
    void *sound_eax3_vtable;
    void *sound_eax_listener_property_guid;
    void *sound_eax_property_set_guid;
    void *sound_effect_object_state;
    void *sound_effects_gain;
    void *sound_enabled;
    void *sound_environment;
    void *sound_fade_curve_exponent;
    void *sound_fade_duration_scale;
    void *sound_idle_update_active;
    void *sound_initialized;
    void *sound_listeners;
    void *sound_looping_audibility_check;
    void *sound_music_gain;
    void *sound_ogg_underrun_count;
    void *sound_stopping_all;
    void *sound_time;
    void *sound_time_delta;
    void *sound_underwater_direct_gain;
    void *sound_update_toggle;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::sound
