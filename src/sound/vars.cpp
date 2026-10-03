/**
 * @file src/sound/vars.cpp
 * Binds halo::sound::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/sound/vars.hpp"
#include "link/sound_vars.hpp"
#include "halo/sound/api.hpp"

namespace halo::sound {

const Vars &vars()
{
    static const Vars table{
        adpcm_index_table,
        adpcm_step_table,
        camera_leaf,
        current_sound_driver,
        debug_sound,
        debug_sound_channel_details,
        debug_sound_channels,
        directsound,
        directsound_binding_count,
        directsound_bindings,
        directsound_caps,
        directsound_channel_count,
        directsound_channels,
        directsound_deferred_dirty,
        directsound_environment_cache,
        directsound_fade,
        directsound_first_channel_of_type,
        directsound_hardware_3d_channel_count,
        directsound_hardware_mode,
        directsound_listener_cached,
        directsound_paused,
        directsound_primary_buffer,
        directsound_rolloff_factor,
        driver_parameters,
        ds3dalg_hrtf_full,
        error_text_buffer,
        game_sound_globals_ptr,
        iid_directsound_3d_buffer,
        iid_directsound_3d_listener,
        k_sound_decode_procs,
        k_sound_sample_rates,
        local_player_0_cluster_index,
        looping_sound_data,
        sound_channel_count,
        sound_channel_parameters_proc_ptr,
        sound_channel_type_flag_table,
        sound_channels,
        sound_class_definitions,
        sound_class_names,
        sound_cluster_audible_bitmap,
        sound_data,
        sound_decode_proc,
        sound_delay_per_world_unit,
        sound_dialog_ducking_gain,
        sound_dialog_unspatialized,
        sound_disabled,
        sound_drivers,
        sound_ducking_gain,
        sound_eax1_vtable,
        sound_eax20_buffer_property_guid,
        sound_eax20_listener_property_guid,
        sound_eax20_underwater_direct_gain,
        sound_eax2_vtable,
        sound_eax30_buffer_property_guid,
        sound_eax30_listener_property_guid,
        sound_eax3_vtable,
        sound_eax_listener_property_guid,
        sound_eax_property_set_guid,
        sound_effect_object_state,
        sound_effects_gain,
        sound_enabled,
        sound_environment,
        sound_fade_curve_exponent,
        sound_fade_duration_scale,
        sound_idle_update_active,
        sound_initialized,
        sound_listeners,
        sound_looping_audibility_check,
        sound_music_gain,
        sound_ogg_underrun_count,
        sound_stopping_all,
        sound_time,
        sound_time_delta,
        sound_underwater_direct_gain,
        sound_update_toggle,
    };
    return table;
}

}  // namespace halo::sound
