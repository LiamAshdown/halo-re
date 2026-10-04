/**
 * @file include/halo/hs/vars.hpp
 * Addresses of the engine variables the hs module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/hs_vars.hpp.
 */
#pragma once

namespace halo::hs {

/** Address table of the engine variables owned by the hs module. */
struct Vars {
    void *object_prediction;
    void *jetpack;
    void *bump_possession;
    void *ai_debug_gate_87abc6;
    void *camera_script_time_remaining;
    void *cheat_super_jump;
    void *director_camera_mode;
    void *director_camera_target;
    void *directsound_eax_enabled;
    void *directsound_listener;
    void *g_00689481;
    void *g_0087abc0;
    void *g_0087abc5;
    void *g_0087abc7;
    void *global_scenario;
    void *hs_autocomplete_count;
    void *hs_autocomplete_gametype_mask;
    void *hs_autocomplete_maximum_count;
    void *hs_autocomplete_prefix;
    void *hs_autocomplete_procedures;
    void *hs_autocomplete_results;
    void *hs_blocking_forbidden;
    void *hs_comparison_types;
    void *hs_compile_error;
    void *hs_compile_error_buffer;
    void *hs_compile_error_offset;
    void *hs_compile_release_source;
    void *hs_compiled_source;
    void *hs_compiled_source_length;
    void *hs_compiled_source_owned;
    void *hs_compiling;
    void *hs_current_thread_index;
    void *hs_empty_string;
    void *hs_enum_definitions;
    void *hs_function_definitions;
    void *hs_global_definitions;
    void *hs_globals_data;
    void *hs_newline_characters;
    void *hs_object_type_masks;
    void *hs_parse_primitive_procedures;
    void *hs_postprocessing;
    void *hs_preserve_token_case;
    void *hs_reload_pending;
    void *hs_runtime_active;
    void *hs_script_type_names;
    void *hs_set_forbidden;
    void *hs_space_characters;
    void *hs_syntax_data;
    void *hs_syntax_data_dirty;
    void *hs_syntax_data_header_byte_swap_definition;
    void *hs_syntax_data_is_local;
    void *hs_syntax_node_byte_swap_definition;
    void *hs_tag_group_for_type;
    void *hs_thread_data;
    void *hs_type_conversion_procedures;
    void *hs_type_inspectors;
    void *hs_type_names;
    void *hs_type_sizes;
    void *magic_seat_animation_state_0069fde0;
    void *main_globals_byte_0071973d;
    void *main_globals_byte_0071973e;
    void *main_globals_byte_0071974e;
    void *main_globals_byte_00719751;
    void *main_globals_byte_00719752;
    void *main_globals_byte_00719753;
    void *main_globals_byte_0071976c;
    void *main_globals_dword_00719740;
    void *main_globals_dword_00719744;
    void *main_globals_word_0071974c;
    void *main_globals_word_0071976e;
    void *message_delta_config_text_buffer;
    void *object_headers;
    void *playback_requested_00719768;
    void *player_control_look_rates_0070facc;
    void *sound_listener_doppler_factor;
    void *sound_listener_rolloff_factor;
    void *sound_supplementary_buffers_00746122;
    void *ui_widget_show_path_flag;
    void *director_camera_target_changed;
    void *unknown_006894ba;
    void *recover_saved_games_hack;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::hs
