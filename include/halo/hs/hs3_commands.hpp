#pragma once

#include "halo/hs/hs3_types.hpp"

namespace halo::hs::part3 {

/**
 * Identifies a group of script command handlers.
 */
enum class CommandTopic : uint8_t { sound, server, unit, vehicle, volume, script };

/**
 * Common interface of the groups of script command handlers; each group is a stateless behaviour object.
 */
class CommandGroup {
public:
    virtual CommandTopic topic() const noexcept = 0;
};

/**
 * Script commands that drive the sound system (looping sounds, gains, environment, rolloff).
 */
class SoundCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::sound; }
    void evaluate_sound_looping_set_scale(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_looping_start(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_looping_stop(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_effects_gain(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_env(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_factor(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_gain(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_master_gain(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_music_gain(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_rolloff(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sound_set_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first) const;
};

/**
 * Dedicated-server script commands and server-variable accessors (bans, map cycle, player lists, server
 * parameters).
 */
class ServerCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::server; }
    void evaluate_sv_ban(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_banlist(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_end_game(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_get_player_action_queue_length(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_kick(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_map(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_map_next(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_map_reset(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_mapcycle(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_mapcycle_add(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_mapcycle_begin(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_mapcycle_del(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_parameters_dump(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_parameters_reload(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_players(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_status(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_sv_unban(int16_t function_index, uint32_t thread_index, char first) const;
    void map_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_ban_penalty_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_banlist_file_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_friendly_fire_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_maxplayers_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_name_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_password_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_rcon_password_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_single_flag_force_reset_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_timelimit_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_tk_cooldown_evaluate(int16_t function_index, datum_index thread, char first) const;
    void sv_tk_grace_evaluate(int16_t function_index, datum_index thread, char first) const;
};

/**
 * Script commands that operate on units: vitality, flashlight, seats, custom animations, weapons and vehicles
 * entry.
 */
class UnitCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::unit; }
    void evaluate_unit_aim_without_turning(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_can_blink(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_close(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_custom_animation_at_frame(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_doesnt_drop_items(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_enter_vehicle(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_exit_vehicle(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_get_current_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_get_custom_animation_time(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_get_health(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_get_shield(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_get_total_grenade_count(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_has_weapon(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_has_weapon_readied(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_impervious(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_is_playing_custom_animation(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_kill_silent(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_open(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_current_vitality(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_emotion(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_emotion_animation(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_enterable_by_player(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_set_seat(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_solo_player_integrated_night_vision_is_active(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_stop_custom_animation(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unit_suspended(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_units_set_current_vitality(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_units_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_units_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first) const;
};

/**
 * Script commands that operate on vehicles: drivers, riders, hover, cargo load and unload.
 */
class VehicleCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::vehicle; }
    void evaluate_vehicle_driver(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_vehicle_hover(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_vehicle_load_magic(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_vehicle_riders(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_vehicle_test_seat_list(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_vehicle_unload(int16_t function_index, uint32_t thread_index, char first) const;
};

/**
 * Script commands that test objects against trigger volumes and teleport players.
 */
class VolumeCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::volume; }
    void evaluate_volume_teleport_players_not_inside(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_volume_test_object(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_volume_test_objects(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_volume_test_objects_all(int16_t function_index, uint32_t thread_index, char first) const;
};

/**
 * Remaining script commands: thread sleep and wake, bsp switching, version, key unbind and misc queries.
 */
class ScriptCommands : public CommandGroup {
public:
    CommandTopic topic() const noexcept override { return CommandTopic::script; }
    void evaluate_structure_bsp_index(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_switch_bsp(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_thread_sleep(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_track_remote_player_position_updates(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_ui_widget_show_path(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_unbind(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_version(int16_t function_index, uint32_t thread_index, char first) const;
    void evaluate_wake(int16_t function_index, uint32_t thread_index, char first) const;
};

}
