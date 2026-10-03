#include "halo/hs/hs3_commands.hpp"
#include "halo/hs/hs3_machine.hpp"
#include "halo/hs/hs3_objects.hpp"
#include "halo/hs/hs3_parser.hpp"
#include "halo/hs/api.hpp"

extern "C" {
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count];
extern char *hs_type_names[k_hs_type_count];
extern char *hs_script_type_names[k_hs_script_type_count];
extern uint16_t hs_object_type_masks[6];
extern char hs_compile_error_buffer[k_hs_error_buffer_size];
extern data_array *hs_thread_data;
extern data_array *hs_globals_data;
extern data_array *hs_syntax_data;
extern uint8_t hs_runtime_active;
extern uint8_t hs_syntax_data_is_local;
extern int16_t hs_current_thread_index;
extern int32_t hs_compile_error_offset;
extern char *hs_compile_error;
extern char *hs_compiled_source;
extern int32_t hs_compiled_source_length;
extern uint8_t hs_set_forbidden;
extern uint8_t hs_reload_pending;
extern uint8_t hs_preserve_token_case;
extern uint8_t hs_postprocessing;
extern uint8_t hs_blocking_forbidden;
extern uint16_t hs_autocomplete_gametype_mask;
}

namespace halo::hs {

static_assert(k_function_definition_count == k_hs_function_count && k_builtin_global_count == k_hs_builtin_global_count && k_type_count == k_hs_type_count && k_script_type_count == k_hs_script_type_count && k_error_buffer_size == k_hs_error_buffer_size);

Globals &globals()
{
    static Globals instance{::hs_thread_data, ::hs_globals_data, ::hs_syntax_data, ::hs_runtime_active, ::hs_syntax_data_is_local, ::hs_current_thread_index, ::hs_compile_error_offset, ::hs_compile_error, ::hs_compiled_source, ::hs_compiled_source_length, ::hs_set_forbidden, ::hs_reload_pending, ::hs_preserve_token_case, ::hs_postprocessing, ::hs_blocking_forbidden, ::hs_autocomplete_gametype_mask, ::hs_function_definitions, ::hs_global_definitions, ::hs_type_names, ::hs_script_type_names, ::hs_object_type_masks, ::hs_compile_error_buffer};
    return instance;
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_looping_set_scale; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47fef0
 */
void hs_evaluate_sound_looping_set_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_looping_set_scale(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_looping_start; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47fe60
 */
void hs_evaluate_sound_looping_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_looping_start(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_looping_stop; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47feb0
 */
void hs_evaluate_sound_looping_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_looping_stop(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_effects_gain; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x480150
 */
void hs_evaluate_sound_set_effects_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_effects_gain(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_env; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x481600
 */
void hs_evaluate_sound_set_env(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_env(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_factor; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4817a0
 */
void hs_evaluate_sound_set_factor(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_factor(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_gain; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47ac10
 */
void hs_evaluate_sound_set_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_gain(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_master_gain; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x480070
 */
void hs_evaluate_sound_set_master_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_master_gain(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_music_gain; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4800e0
 */
void hs_evaluate_sound_set_music_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_music_gain(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_rolloff; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x481750
 */
void hs_evaluate_sound_set_rolloff(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_rolloff(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::SoundCommands::evaluate_sound_set_supplementary_buffers; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x4816a0
 */
void hs_evaluate_sound_set_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::SoundCommands().evaluate_sound_set_supplementary_buffers(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_structure_bsp_index; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47f670
 */
void hs_evaluate_structure_bsp_index(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_structure_bsp_index(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_ban; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482c60
 */
void hs_evaluate_sv_ban(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_ban(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_banlist; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482a10
 */
void hs_evaluate_sv_banlist(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_banlist(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_end_game; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482bc0
 */
void hs_evaluate_sv_end_game(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_end_game(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_get_player_action_queue_length; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x4825f0
 */
void hs_evaluate_sv_get_player_action_queue_length(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_get_player_action_queue_length(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_kick; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482c10
 */
void hs_evaluate_sv_kick(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_kick(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_map; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482ad0
 */
void hs_evaluate_sv_map(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_map(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_map_next; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482aa0
 */
void hs_evaluate_sv_map_next(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_map_next(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_map_reset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482ac0
 */
void hs_evaluate_sv_map_reset(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_map_reset(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_mapcycle; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482e10
 */
void hs_evaluate_sv_mapcycle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_mapcycle(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_mapcycle_add; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482e30
 */
void hs_evaluate_sv_mapcycle_add(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_mapcycle_add(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_mapcycle_begin; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482df0
 */
void hs_evaluate_sv_mapcycle_begin(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_mapcycle_begin(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_mapcycle_del; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482e80
 */
void hs_evaluate_sv_mapcycle_del(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_mapcycle_del(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_parameters_dump; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482510
 */
void hs_evaluate_sv_parameters_dump(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_parameters_dump(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_parameters_reload; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4824e0
 */
void hs_evaluate_sv_parameters_reload(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_parameters_reload(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_players; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482c00
 */
void hs_evaluate_sv_players(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_players(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_status; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482c50
 */
void hs_evaluate_sv_status(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_status(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::evaluate_sv_unban; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482a20
 */
void hs_evaluate_sv_unban(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ServerCommands().evaluate_sv_unban(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_switch_bsp; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47f620
 */
void hs_evaluate_switch_bsp(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_switch_bsp(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_thread_sleep; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482640
 */
void hs_evaluate_thread_sleep(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_thread_sleep(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_track_remote_player_position_updates; forwards to
 * the C++ implementation unchanged.
 *
 * @address 0x482560
 */
void hs_evaluate_track_remote_player_position_updates(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_track_remote_player_position_updates(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ArgumentEvaluator::typed_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48a850
 */
int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first)
{
    return halo::hs::part3::ArgumentEvaluator().typed_arguments(thread_index, parameter_count, expected_types, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_ui_widget_show_path; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4814d0
 */
void hs_evaluate_ui_widget_show_path(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_ui_widget_show_path(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_unbind; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482430
 */
void hs_evaluate_unbind(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_unbind(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_aim_without_turning; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bcb0
 */
void hs_evaluate_unit_aim_without_turning(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_aim_without_turning(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_can_blink; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47b810
 */
void hs_evaluate_unit_can_blink(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_can_blink(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_close; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47b8f0
 */
void hs_evaluate_unit_close(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_close(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_custom_animation_at_frame; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bbb0
 */
void hs_evaluate_unit_custom_animation_at_frame(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_custom_animation_at_frame(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_doesnt_drop_items; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c650
 */
void hs_evaluate_unit_doesnt_drop_items(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_doesnt_drop_items(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_enter_vehicle; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47be40
 */
void hs_evaluate_unit_enter_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_enter_vehicle(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_exit_vehicle; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bfa0
 */
void hs_evaluate_unit_exit_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_exit_vehicle(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_get_current_flashlight_state; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x47c830
 */
void hs_evaluate_unit_get_current_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_get_current_flashlight_state(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_get_custom_animation_time; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47ba00
 */
void hs_evaluate_unit_get_custom_animation_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_get_custom_animation_time(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_get_health; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c400
 */
void hs_evaluate_unit_get_health(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_get_health(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_get_shield; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c480
 */
void hs_evaluate_unit_get_shield(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_get_shield(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_get_total_grenade_count; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c500
 */
void hs_evaluate_unit_get_total_grenade_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_get_total_grenade_count(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_has_weapon; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c580
 */
void hs_evaluate_unit_has_weapon(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_has_weapon(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_has_weapon_readied; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c5f0
 */
void hs_evaluate_unit_has_weapon_readied(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_has_weapon_readied(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_impervious; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c690
 */
void hs_evaluate_unit_impervious(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_impervious(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_is_playing_custom_animation; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x47bc20
 */
void hs_evaluate_unit_is_playing_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_is_playing_custom_animation(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_kill_silent; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47b9a0
 */
void hs_evaluate_unit_kill_silent(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_kill_silent(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_open; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47b8a0
 */
void hs_evaluate_unit_open(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_open(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_current_vitality; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c0b0
 */
void hs_evaluate_unit_set_current_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_current_vitality(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_desired_flashlight_state; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x47c7a0
 */
void hs_evaluate_unit_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_desired_flashlight_state(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_emotion; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47bd40
 */
void hs_evaluate_unit_set_emotion(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_emotion(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_emotion_animation; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bf50
 */
void hs_evaluate_unit_set_emotion_animation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_emotion_animation(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_enterable_by_player; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bdb0
 */
void hs_evaluate_unit_set_enterable_by_player(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_enterable_by_player(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_maximum_vitality; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47bfe0
 */
void hs_evaluate_unit_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_maximum_vitality(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_set_seat; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c260
 */
void hs_evaluate_unit_set_seat(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_set_seat(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_solo_player_integrated_night_vision_is_active;
 * forwards to the C++ implementation unchanged.
 *
 * @address 0x47c730
 */
void hs_evaluate_unit_solo_player_integrated_night_vision_is_active(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_solo_player_integrated_night_vision_is_active(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_stop_custom_animation; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47ba60
 */
void hs_evaluate_unit_stop_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_stop_custom_animation(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_unit_suspended; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47c6e0
 */
void hs_evaluate_unit_suspended(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_unit_suspended(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_units_set_current_vitality; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c100
 */
void hs_evaluate_units_set_current_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_units_set_current_vitality(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_units_set_desired_flashlight_state; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x47c750
 */
void hs_evaluate_units_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_units_set_desired_flashlight_state(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::UnitCommands::evaluate_units_set_maximum_vitality; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c060
 */
void hs_evaluate_units_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::UnitCommands().evaluate_units_set_maximum_vitality(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ArgumentEvaluator::variadic_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48ad60
 */
char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values)
{
    return halo::hs::part3::ArgumentEvaluator().variadic_arguments(thread_index, value, out_count, out_values);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_driver; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c340
 */
void hs_evaluate_vehicle_driver(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_driver(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_hover; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4801c0
 */
void hs_evaluate_vehicle_hover(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_hover(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_load_magic; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c150
 */
void hs_evaluate_vehicle_load_magic(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_load_magic(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_riders; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c300
 */
void hs_evaluate_vehicle_riders(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_riders(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_test_seat_list; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47be90
 */
void hs_evaluate_vehicle_test_seat_list(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_test_seat_list(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VehicleCommands::evaluate_vehicle_unload; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47c1b0
 */
void hs_evaluate_vehicle_unload(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VehicleCommands().evaluate_vehicle_unload(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_version; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x47f6a0
 */
void hs_evaluate_version(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_version(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VolumeCommands::evaluate_volume_teleport_players_not_inside; forwards to
 * the C++ implementation unchanged.
 *
 * @address 0x47a420
 */
void hs_evaluate_volume_teleport_players_not_inside(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VolumeCommands().evaluate_volume_teleport_players_not_inside(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VolumeCommands::evaluate_volume_test_object; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47a470
 */
void hs_evaluate_volume_test_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VolumeCommands().evaluate_volume_test_object(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VolumeCommands::evaluate_volume_test_objects; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47a4f0
 */
void hs_evaluate_volume_test_objects(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VolumeCommands().evaluate_volume_test_objects(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::VolumeCommands::evaluate_volume_test_objects_all; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x47a550
 */
void hs_evaluate_volume_test_objects_all(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::VolumeCommands().evaluate_volume_test_objects_all(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptCommands::evaluate_wake; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x489a20
 */
void hs_evaluate_wake(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::part3::ScriptCommands().evaluate_wake(function_index, thread_index, first);
}

/**
 * Free-function entry point for halo::hs::part3::FunctionTable::find_function_by_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483520
 */
int16_t hs_find_function_by_name(char *name)
{
    return halo::hs::part3::FunctionTable().find_function_by_name(name);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::find_global_by_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483480
 */
hs_global_reference hs_find_global_by_name(char *name)
{
    return halo::hs::part3::GlobalTable().find_global_by_name(name);
}

/**
 * Free-function entry point for halo::hs::part3::FunctionTable::format_function_signature; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x484300
 */
void hs_format_function_signature(int16_t function_index, char *out)
{
    halo::hs::part3::FunctionTable().format_function_signature(function_index, out);
}

/**
 * Free-function entry point for halo::hs::part3::TypeRules::gametype_flag_satisfied; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4835b0
 */
char hs_gametype_flag_satisfied(uint8_t bit_index, uint8_t flags)
{
    return halo::hs::part3::TypeRules().gametype_flag_satisfied(bit_index, flags);
}

/**
 * Free-function entry point for halo::hs::part3::TypeRules::gametype_flags_applicable; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483600
 */
uint8_t hs_gametype_flags_applicable(uint8_t flags)
{
    return halo::hs::part3::TypeRules().gametype_flags_applicable(flags);
}

/**
 * Free-function entry point for halo::hs::part3::FunctionTable::get_parameter_indices; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x484fb0
 */
char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices)
{
    return halo::hs::part3::FunctionTable().get_parameter_indices(function_name, required_count, node_index, out_indices);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::get_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x483450
 */
char *hs_global_get_name(hs_global_reference global)
{
    return halo::hs::part3::GlobalTable().get_name(global);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::get_type; forwards to the C++ implementation unchanged.
 *
 * @address 0x483420
 */
hs_type_t hs_global_get_type(hs_global_reference global)
{
    return halo::hs::part3::GlobalTable().get_type(global);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::get_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a720
 */
int32_t hs_global_get_value(hs_global_reference reference)
{
    return halo::hs::part3::GlobalTable().get_value(reference);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::read_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x48aec0
 */
void hs_global_read_value(hs_global_reference reference)
{
    halo::hs::part3::GlobalTable().read_value(reference);
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::write_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x48b030
 */
void hs_global_write_value(hs_global_reference reference)
{
    halo::hs::part3::GlobalTable().write_value(reference);
}

/**
 * Free-function entry point for halo::hs::part3::FunctionTable::help_print_function; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4841b0
 */
void hs_help_print_function(char *name)
{
    halo::hs::part3::FunctionTable().help_print_function(name);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_boolean; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x489aa0
 */
void hs_inspect_boolean(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_boolean(type, value, buffer);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_enum; forwards to the C++ implementation unchanged.
 *
 * @address 0x489b50
 */
void hs_inspect_enum(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_enum(type, value, buffer);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_long; forwards to the C++ implementation unchanged.
 *
 * @address 0x489b10
 */
void hs_inspect_long(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_long(type, value, buffer);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_real; forwards to the C++ implementation unchanged.
 *
 * @address 0x489ad0
 */
void hs_inspect_real(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_real(type, value, buffer);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_short; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x489af0
 */
void hs_inspect_short(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_short(type, value, buffer);
}

/**
 * Free-function entry point for halo::hs::part3::ValueInspector::inspect_string; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x489b30
 */
void hs_inspect_string(int16_t type, int32_t value, char *buffer)
{
    halo::hs::part3::ValueInspector().inspect_string(type, value, buffer);
}


/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_angle_predicate_helper; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4878f0
 */
uint8_t hs_object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees)
{
    return halo::hs::part3::ScriptObjects().object_angle_predicate_helper(object_index, viewer_unit, angle_degrees);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_create_name_index_if_absent; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487cf0
 */
void hs_object_create_name_index_if_absent(int32_t name_index)
{
    halo::hs::part3::ScriptObjects().object_create_name_index_if_absent(name_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_detach_and_place_at_location; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487f50
 */
void hs_object_detach_and_place_at_location(int16_t location_index, datum_index object_index, char detach_from_parent, char reorient)
{
    halo::hs::part3::ScriptObjects().object_detach_and_place_at_location(location_index, object_index, detach_from_parent, reorient);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_hierarchy_test; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487c10
 */
char hs_object_hierarchy_test(datum_index object_index)
{
    return halo::hs::part3::ScriptObjects().object_hierarchy_test(object_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_any_angle_match; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4879b0
 */
uint32_t hs_object_list_any_angle_match(datum_index header_index, datum_index target_object, float angle_degrees)
{
    return halo::hs::part3::ScriptObjects().object_list_any_angle_match(header_index, target_object, angle_degrees);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_any_angle_match_gated; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487ad0
 */
uint32_t hs_object_list_any_angle_match_gated(datum_index header_index, int16_t gate, float angle_degrees)
{
    return halo::hs::part3::ScriptObjects().object_list_any_angle_match_gated(header_index, gate, angle_degrees);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_collect_player_units; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487630
 */
datum_index hs_object_list_collect_player_units(void)
{
    return halo::hs::part3::ScriptObjects().object_list_collect_player_units();
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_for_each; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x488740
 */
void hs_object_list_for_each(datum_index header_index)
{
    halo::hs::part3::ScriptObjects().object_list_for_each(header_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_new_singleton; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x48ac10
 */
datum_index hs_object_list_new_singleton(datum_index object_index)
{
    return halo::hs::part3::ScriptObjects().object_list_new_singleton(object_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_list_test_trigger_volume; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487820
 */
char hs_object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index, char all_mode)
{
    return halo::hs::part3::ScriptObjects().object_list_test_trigger_volume(trigger_volume_index, header_index, all_mode);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_name_cache_validate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x487d20
 */
void hs_object_name_cache_validate(int16_t object_name_index)
{
    halo::hs::part3::ScriptObjects().object_name_cache_validate(object_name_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_name_destroy; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487d90
 */
void hs_object_name_destroy(int32_t object_name_index)
{
    halo::hs::part3::ScriptObjects().object_name_destroy(object_name_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_names_for_each; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487ef0
 */
void hs_object_names_for_each(void (*callback)(int32_t index), uint32_t predicate_arg)
{
    halo::hs::part3::ScriptObjects().object_names_for_each(callback, predicate_arg);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_orient; forwards to the C++ implementation unchanged.
 *
 * @address 0x48ab80
 */
int32_t hs_object_orient(float x)
{
    return halo::hs::part3::ScriptObjects().object_orient(x);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_runtime_cleanup; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487dd0
 */
void hs_object_runtime_cleanup(void)
{
    halo::hs::part3::ScriptObjects().object_runtime_cleanup();
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::object_set_health_fraction; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x488600
 */
void hs_object_set_health_fraction(datum_index object_index, float fraction)
{
    halo::hs::part3::ScriptObjects().object_set_health_fraction(object_index, fraction);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::hs_object_set_permutation_by_name; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x488670
 */
void hs_object_set_permutation_by_name(datum_index object_index, void *permutation_name, char *name)
{
    halo::hs::part3::ScriptObjects().hs_object_set_permutation_by_name(object_index, permutation_name, name);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptObjects::objects_delete_by_type; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4887d0
 */
void hs_objects_delete_by_type(uint32_t tag_id)
{
    halo::hs::part3::ScriptObjects().objects_delete_by_type(tag_id);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::hs_parse; forwards to the C++ implementation unchanged.
 *
 * @address 0x486420
 */
char hs_parse(datum_index node_index, hs_type_t expected_type)
{
    return halo::hs::part3::Parser().hs_parse(node_index, expected_type);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_ai; forwards to the C++ implementation unchanged.
 *
 * @address 0x487150
 */
char hs_parse_ai(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_ai(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_ai_command_list; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4871a0
 */
char hs_parse_ai_command_list(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_ai_command_list(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_arithmetic; forwards to the C++ implementation unchanged.
 *
 * @address 0x484ea0
 */
char hs_parse_arithmetic(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_arithmetic(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_begin; forwards to the C++ implementation unchanged.
 *
 * @address 0x484600
 */
char hs_parse_begin(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_begin(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_boolean; forwards to the C++ implementation unchanged.
 *
 * @address 0x486a10
 */
char hs_parse_boolean(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_boolean(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cond; forwards to the C++ implementation unchanged.
 *
 * @address 0x484b40
 */
char hs_parse_cond(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_cond(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cond_recursive; forwards to the C++ implementation unchanged.
 *
 * @address 0x4848f0
 */
datum_index hs_parse_cond_recursive(datum_index cond_node_index, datum_index pair_index)
{
    return halo::hs::part3::Parser().parse_cond_recursive(cond_node_index, pair_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_conversation; forwards to the C++ implementation unchanged.
 *
 * @address 0x487200
 */
char hs_parse_conversation(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_conversation(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cutscene_camera_point; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487090
 */
char hs_parse_cutscene_camera_point(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_cutscene_camera_point(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cutscene_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x487060
 */
char hs_parse_cutscene_flag(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_cutscene_flag(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cutscene_recording; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4870f0
 */
char hs_parse_cutscene_recording(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_cutscene_recording(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_cutscene_title; forwards to the C++ implementation unchanged.
 *
 * @address 0x4870c0
 */
char hs_parse_cutscene_title(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_cutscene_title(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_device_group; forwards to the C++ implementation unchanged.
 *
 * @address 0x487120
 */
char hs_parse_device_group(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_device_group(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_function_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487440
 */
char hs_parse_function_arguments(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_function_arguments(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_hud_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4873c0
 */
char hs_parse_hud_message(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_hud_message(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_if; forwards to the C++ implementation unchanged.
 *
 * @address 0x484780
 */
char hs_parse_if(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_if(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_inspect; forwards to the C++ implementation unchanged.
 *
 * @address 0x485460
 */
char hs_parse_inspect(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_inspect(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_integer; forwards to the C++ implementation unchanged.
 *
 * @address 0x486b80
 */
char hs_parse_integer(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_integer(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_logical; forwards to the C++ implementation unchanged.
 *
 * @address 0x484db0
 */
char hs_parse_logical(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_logical(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_navpoint; forwards to the C++ implementation unchanged.
 *
 * @address 0x487360
 */
char hs_parse_navpoint(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_navpoint(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_nonprimitive; forwards to the C++ implementation unchanged.
 *
 * @address 0x486710
 */
char hs_parse_nonprimitive(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_nonprimitive(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x4872f0
 */
char hs_parse_object(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_object(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_object_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x487400
 */
char hs_parse_object_list(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_object_list(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_object_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x487230
 */
char hs_parse_object_name(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_object_name(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_primitive; forwards to the C++ implementation unchanged.
 *
 * @address 0x486480
 */
char hs_parse_primitive(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_primitive(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_real; forwards to the C++ implementation unchanged.
 *
 * @address 0x486ae0
 */
char hs_parse_real(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_real(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_scenario_datum; forwards to the C++ implementation unchanged.
 *
 * @address 0x486f90
 */
char hs_parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array, int32_t stride)
{
    return halo::hs::part3::Parser().parse_scenario_datum(node_index, name_offset, array, stride);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_script; forwards to the C++ implementation unchanged.
 *
 * @address 0x486c80
 */
char hs_parse_script(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_script(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x484be0
 */
char hs_parse_set(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_set(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_sleep; forwards to the C++ implementation unchanged.
 *
 * @address 0x485280
 */
char hs_parse_sleep(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_sleep(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_sleep_until; forwards to the C++ implementation unchanged.
 *
 * @address 0x485310
 */
char hs_parse_sleep_until(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_sleep_until(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_starting_profile; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4871d0
 */
char hs_parse_starting_profile(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_starting_profile(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_string; forwards to the C++ implementation unchanged.
 *
 * @address 0x486c50
 */
char hs_parse_string(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_string(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_string_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x487530
 */
char hs_parse_string_arguments(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_string_arguments(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_tag_reference; forwards to the C++ implementation unchanged.
 *
 * @address 0x486ce0
 */
char hs_parse_tag_reference(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_tag_reference(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_trigger_volume; forwards to the C++ implementation unchanged.
 *
 * @address 0x487030
 */
char hs_parse_trigger_volume(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_trigger_volume(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_two_numeric_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x485060
 */
char hs_parse_two_numeric_arguments(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_two_numeric_arguments(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_two_object_arguments; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x485150
 */
char hs_parse_two_object_arguments(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_two_object_arguments(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_unit; forwards to the C++ implementation unchanged.
 *
 * @address 0x4854f0
 */
char hs_parse_unit(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_unit(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_variable; forwards to the C++ implementation unchanged.
 *
 * @address 0x486560
 */
char hs_parse_variable(datum_index node_index)
{
    return halo::hs::part3::Parser().parse_variable(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::parse_wake; forwards to the C++ implementation unchanged.
 *
 * @address 0x4853c0
 */
char hs_parse_wake(int16_t function_index, datum_index node_index)
{
    return halo::hs::part3::Parser().parse_wake(function_index, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::rebuild_source; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483e20
 */
char hs_rebuild_source(void)
{
    return halo::hs::part3::SourceTokenizer().rebuild_source();
}

/**
 * Free-function entry point for halo::hs::part3::Parser::report_expected_enum_values; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x486dc0
 */
char hs_report_expected_enum_values(datum_index node_index)
{
    return halo::hs::part3::Parser().report_expected_enum_values(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::reposition_players_outside_trigger_volume; forwards to the
 * C++ implementation unchanged.
 *
 * @address 0x487750
 */
void hs_reposition_players_outside_trigger_volume(int32_t trigger_volume_index, int32_t location_index)
{
    halo::hs::part3::ScriptRuntime().reposition_players_outside_trigger_volume(trigger_volume_index, location_index);
}

/**
 * Free-function entry point for halo::hs::part3::Parser::resolve_identifier_as_function_or_script; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x486680
 */
void hs_resolve_identifier_as_function_or_script(datum_index node_index)
{
    halo::hs::part3::Parser().resolve_identifier_as_function_or_script(node_index);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::runtime_initialize; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x489e70
 */
void hs_runtime_initialize(void)
{
    halo::hs::part3::ScriptRuntime().runtime_initialize();
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::runtime_update; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48a1a0
 */
void hs_runtime_update(void)
{
    halo::hs::part3::ScriptRuntime().runtime_update();
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::scenario_scripts_initialize; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x489ef0
 */
void hs_scenario_scripts_initialize(void)
{
    halo::hs::part3::ScriptRuntime().scenario_scripts_initialize();
}

/**
 * Free-function entry point for halo::hs::part3::FunctionTable::script_find_by_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4833a0
 */
int16_t hs_script_find_by_name(char *name)
{
    return halo::hs::part3::FunctionTable().script_find_by_name(name);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::scripts_compile_and_link; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483190
 */
char hs_scripts_compile_and_link(char restore_previous)
{
    return halo::hs::part3::ScriptRuntime().scripts_compile_and_link(restore_previous);
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::scripts_free; forwards to the C++ implementation unchanged.
 *
 * @address 0x4832b0
 */
void hs_scripts_free(void)
{
    halo::hs::part3::ScriptRuntime().scripts_free();
}

/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::scripts_reload; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x483250
 */
void hs_scripts_reload(void)
{
    halo::hs::part3::ScriptRuntime().scripts_reload();
}

/**
 * Free-function entry point for halo::hs::part3::GlobalTable::sound_get_gain_reference; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x488b10
 */
float *hs_sound_get_gain_reference(char *name)
{
    return halo::hs::part3::GlobalTable().sound_get_gain_reference(name);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::source_buffer_append; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4856f0
 */
char *hs_source_buffer_append(char *text, uint32_t length)
{
    return halo::hs::part3::SourceTokenizer().source_buffer_append(text, length);
}

/**
 * Free-function entry point for halo::hs::part3::TypeRules::string_is_empty; forwards to the C++ implementation unchanged.
 *
 * @address 0x48aaf0
 */
char hs_string_is_empty(char *s)
{
    return halo::hs::part3::TypeRules().string_is_empty(s);
}


/**
 * Free-function entry point for halo::hs::part3::ScriptRuntime::syntax_node_garbage_collect; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x483310
 */
void hs_syntax_node_garbage_collect(void)
{
    halo::hs::part3::ScriptRuntime().syntax_node_garbage_collect();
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::evaluate_step; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a370
 */
void hs_thread_evaluate_step(uint32_t thread_index)
{
    halo::hs::part3::ThreadMachine().evaluate_step(thread_index);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::find_by_script_index; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48a960
 */
datum_index hs_thread_find_by_script_index(int16_t script_index)
{
    return halo::hs::part3::ThreadMachine().find_by_script_index(script_index);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::find_by_script_name; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48a9f0
 */
datum_index hs_thread_find_by_script_name(char *name)
{
    return halo::hs::part3::ThreadMachine().find_by_script_name(name);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a2f0
 */
datum_index hs_thread_new(int32_t script_index, uint8_t type)
{
    return halo::hs::part3::ThreadMachine().create(script_index, type);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::pop_frame; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a770
 */
void hs_thread_pop_frame(uint32_t thread_index)
{
    halo::hs::part3::ThreadMachine().pop_frame(thread_index);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::push; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a560
 */
void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address)
{
    halo::hs::part3::ThreadMachine().push(node, thread_index, result_address);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::restart; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a790
 */
void hs_thread_restart(uint32_t thread_index)
{
    halo::hs::part3::ThreadMachine().restart(thread_index);
}

/**
 * Free-function entry point for halo::hs::part3::ThreadMachine::return_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a640
 */
void hs_thread_return(int32_t value, uint32_t thread_index)
{
    halo::hs::part3::ThreadMachine().return_value(value, thread_index);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::tokenize; forwards to the C++ implementation unchanged.
 *
 * @address 0x486120
 */
datum_index hs_tokenize(char **cursor)
{
    return halo::hs::part3::SourceTokenizer().tokenize(cursor);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::tokenize_nonprimitive; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x486290
 */
void hs_tokenize_nonprimitive(datum_index node_index, char **cursor)
{
    halo::hs::part3::SourceTokenizer().tokenize_nonprimitive(node_index, cursor);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::tokenize_primitive; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4861b0
 */
void hs_tokenize_primitive(char **cursor, datum_index node_index)
{
    halo::hs::part3::SourceTokenizer().tokenize_primitive(cursor, node_index);
}

/**
 * Free-function entry point for halo::hs::part3::TypeRules::type_mask_is_subset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48ac60
 */
char hs_type_mask_is_subset(int16_t subtype_index, int16_t supertype_index)
{
    return halo::hs::part3::TypeRules().type_mask_is_subset(subtype_index, supertype_index);
}

/**
 * Free-function entry point for halo::hs::part3::TypeRules::types_are_compatible; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48ac90
 */
char hs_types_are_compatible(hs_type_t dest_type, hs_type_t source_type)
{
    return halo::hs::part3::TypeRules().types_are_compatible(dest_type, source_type);
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::verify_source_offset; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x4858a0
 */
char hs_verify_source_offset(int32_t offset)
{
    return halo::hs::part3::SourceTokenizer().verify_source_offset(offset);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::map_list_matching_substring_evaluate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x483010
 */
void map_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().map_list_matching_substring_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::get_first; forwards to the C++ implementation unchanged.
 *
 * @address 0x48b2f0
 */
int32_t object_list_get_first(datum_index header_index, object_list_iterator *iterator_out)
{
    return halo::hs::part3::ObjectLists().get_first(header_index, iterator_out);
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::nth_reference; forwards to the C++ implementation unchanged.
 *
 * @address 0x488570
 */
int32_t object_list_nth_reference(datum_index header_index, int16_t n)
{
    return halo::hs::part3::ObjectLists().nth_reference(header_index, n);
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::reference_add; forwards to the C++ implementation unchanged.
 *
 * @address 0x48b2a0
 */
void object_list_reference_add(datum_index header_index, datum_index object_index)
{
    halo::hs::part3::ObjectLists().reference_add(header_index, object_index);
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::reference_chain_delete; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x48b220
 */
void object_list_reference_chain_delete(data_array *reference_array, datum_index chain_head)
{
    halo::hs::part3::ObjectLists().reference_chain_delete(reference_array, chain_head);
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::dispose_empty; forwards to the C++ implementation unchanged.
 *
 * @address 0x48b340
 */
void object_lists_dispose_empty(void)
{
    halo::hs::part3::ObjectLists().dispose_empty();
}

/**
 * Free-function entry point for halo::hs::part3::ObjectLists::initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x48b250
 */
void object_lists_initialize(void)
{
    halo::hs::part3::ObjectLists().initialize();
}

/**
 * Free-function entry point for halo::hs::part3::SourceTokenizer::skip_whitespace; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x486350
 */
void skip_whitespace(char **cursor)
{
    halo::hs::part3::SourceTokenizer().skip_whitespace(cursor);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_ban_penalty_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482cb0
 */
void sv_ban_penalty_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_ban_penalty_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_banlist_file_evaluate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482da0
 */
void sv_banlist_file_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_banlist_file_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_friendly_fire_evaluate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482f70
 */
void sv_friendly_fire_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_friendly_fire_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_maxplayers_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482b70
 */
void sv_maxplayers_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_maxplayers_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_name_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482ed0
 */
void sv_name_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_name_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_password_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482f20
 */
void sv_password_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_password_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_rcon_password_evaluate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x482b20
 */
void sv_rcon_password_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_rcon_password_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_single_flag_force_reset_evaluate; forwards to the C++
 * implementation unchanged.
 *
 * @address 0x4830b0
 */
void sv_single_flag_force_reset_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_single_flag_force_reset_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_timelimit_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482fc0
 */
void sv_timelimit_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_timelimit_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_tk_cooldown_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482d50
 */
void sv_tk_cooldown_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_tk_cooldown_evaluate(function_index, thread, first);
}

/**
 * Free-function entry point for halo::hs::part3::ServerCommands::sv_tk_grace_evaluate; forwards to the C++ implementation
 * unchanged.
 *
 * @address 0x482d00
 */
void sv_tk_grace_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::part3::ServerCommands().sv_tk_grace_evaluate(function_index, thread, first);
}

}
