/**
 * @file include/halo/items/api.hpp
 * Functions of the items module that other modules and the data tables call (namespace halo::items). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct ScenarioEquipment;
struct ScenarioWeapon;
struct real_point3d;
struct real_vector3d;
struct update_record;
struct weapon_hud_ammo_state;
typedef uint32_t datum_index;
typedef float real;

namespace halo::items {

void equipment_apply_network_update(datum_index item_index, uint32_t *update_record);
void equipment_build_creation_message(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, uint32_t object_flags);
int32_t equipment_build_network_update(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
void equipment_create_from_creation_message(void *incoming_record);
void equipment_definition_play_pickup_sound(uint32_t equipment_tag_id);
uint8_t equipment_is_old_enough(uint32_t object_index);
void equipment_network_baseline_take(uint32_t item_index);
uint8_t equipment_new(uint32_t object_index);
void equipment_new_from_placement(uint32_t equipment_object_index, ScenarioEquipment *placement);
void equipment_pickup_play_sound(uint32_t object_index);
void equipment_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3);
uint8_t garbage_new(uint32_t object_index);
int32_t garbage_update(uint32_t object_index);
void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer);
void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index, real_vector3d *normal, real_point3d *point);
uint32_t item_any_detonating();
void item_compute_rotation(uint32_t object_index);
void item_detonation_timer_start(uint32_t object_index);
uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position);
uint8_t item_new(uint32_t object_index);
void item_set_holder(uint32_t item_index, datum_index holder_index);
void item_stamp_age_timestamp(uint32_t object_index);
uint8_t item_update(uint32_t item_index);
void trigger_create_projectiles(uint32_t item_index, int16_t trigger_index, uint32_t role);
int32_t weapon_add_ammunition(void **message_record);
void weapon_apply_ammo_correction(void **message_record);
void weapon_apply_ammo_correction_and_resync(void **message_record);
void weapon_apply_network_update(datum_index item_index, uint32_t *update_record);
void weapon_build_creation_message(datum_index item_index, uint32_t unused_param_2, uint32_t unused_param_3, uint32_t object_flags);
void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out);
int32_t weapon_build_network_update(uint32_t item_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
real weapon_clamp_zoom_fov(datum_index item_index, int16_t zoom_level, real base_fov);
void weapon_create_from_creation_message(void *incoming_record);
uint32_t weapon_fire_trigger(datum_index item_index, int16_t trigger_index);
void weapon_force_settled_state(datum_index item_index);
int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index, int16_t category, int16_t mode);
char * weapon_get_label(datum_index item_index);
int32_t weapon_get_next_zoom_level(int32_t current_level, datum_index item_index);
real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level);
int32_t weapon_has_active_state(datum_index item_index);
uint8_t weapon_is_old_enough(uint32_t object_index);
uint8_t weapon_is_out_of_ammo(datum_index item_index);
int32_t weapon_is_reloading(datum_index item_index);
void weapon_magazine_begin_chamber(datum_index item_index, int16_t magazine_index);
void weapon_magazine_reload_tick(datum_index item_index, int16_t magazine_index);
void weapon_magazine_reload_tick_predicted(datum_index item_index, int16_t magazine_index);
uint32_t weapon_must_be_readied(datum_index item_index);
void weapon_network_baseline_take(uint32_t item_index);
uint8_t weapon_new(uint32_t object_index);
datum_index weapon_new_from_placement(datum_index weapon_object_index, ScenarioWeapon *placement);
void weapon_notify_ammo_pickup(datum_index item_index, int16_t magazine_index, int16_t rounds);
void weapon_notify_reload_begin(datum_index item_index, int16_t magazine_index);
void weapon_notify_reload_cancel(datum_index item_index, int16_t magazine_index);
void weapon_notify_reload_step(datum_index item_index, int16_t magazine_index);
uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a, real scale_b);
void weapon_predict_ammo(void **message_record);
uint32_t weapon_prevents_grenade_throwing(datum_index item_index);
uint32_t weapon_prevents_melee_attack(datum_index item_index);
int32_t weapon_put_away(datum_index item_index, int8_t force);
void weapon_ready(datum_index item_index);
void weapon_reload_recovery_finish(datum_index item_index);
void weapon_reset_triggers(datum_index item_index);
void weapon_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3);
void weapon_set_ammo_counts(datum_index item_index, int16_t *reserve_counts);
void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger);
void weapon_set_loaded_ammo_fraction(datum_index item_index, real fraction);
void weapon_set_ready_timer(datum_index item_index, real value);
int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force);
void weapon_set_state_indicator_flags(datum_index item_index);
uint32_t weapon_stop_object_effect(datum_index item_index, datum_index tag_id);
uint32_t weapon_transfer_ammunition(datum_index target_item_index, datum_index source_item_index, int16_t requesting_player_index, int16_t *out_transferred);
void weapon_trigger_barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index, int16_t distribution_function, real distribution_angle, uint32_t flags);
void weapon_trigger_become_charged(datum_index item_index, int16_t trigger_index);
void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted);
void weapon_trigger_continue_burst(datum_index item_index, int16_t trigger_index);
void weapon_trigger_effect_clear(datum_index item_index, int16_t trigger_index);
void weapon_trigger_effect_set_out_of_ammo(datum_index item_index, int16_t trigger_index);
void weapon_trigger_effect_set_state(datum_index item_index, int16_t trigger_index, int8_t state, int16_t counter);
void weapon_trigger_enter_recovery(datum_index item_index, int16_t trigger_index);
void weapon_trigger_finish_shot(datum_index item_index, int16_t trigger_index);
void weapon_trigger_fire_or_reload(datum_index item_index, int16_t trigger_index, int8_t force);
uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index, real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range, uint8_t *out_used_straight_line);
real weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire);
real weapon_trigger_get_charge_fraction(datum_index item_index, int16_t trigger_index);
void weapon_trigger_handle_empty(datum_index item_index, int16_t trigger_index);
real weapon_trigger_projectile_time_fraction(datum_index item_index, int16_t trigger_index, real elapsed);
int32_t weapon_trigger_ready_to_fire(datum_index item_index, int16_t trigger_index);
void weapon_trigger_reset_tracking(datum_index item_index, int16_t trigger_index);
int32_t weapon_triggers_idle(datum_index item_index);
int32_t weapon_update(datum_index item_index);
void weapon_update_function_values(uint32_t object_index);

}
