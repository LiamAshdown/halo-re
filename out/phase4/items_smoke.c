#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "items.h"

// struct sizes, each one fixed by an object_type_definition row at 0x0069bfdc
typedef char check_item_data[(sizeof(item_data) == 0x38) ? 1 : -1];
typedef char check_weapon_data[(sizeof(weapon_data) == 0x114) ? 1 : -1];
typedef char check_equipment_data[(sizeof(equipment_data) == 0x68) ? 1 : -1];
typedef char check_garbage_data[(sizeof(garbage_data) == 0x18) ? 1 : -1];
typedef char check_object_plus_item[(0x1f4 + sizeof(item_data) == 0x22c) ? 1 : -1];
typedef char check_item_plus_weapon[(0x22c + sizeof(weapon_data) == 0x340) ? 1 : -1];
typedef char check_item_plus_equipment[(0x22c + sizeof(equipment_data) == 0x294) ? 1 : -1];
typedef char check_item_plus_garbage[(0x22c + sizeof(garbage_data) == 0x244) ? 1 : -1];

// sub-records
typedef char check_weapon_trigger_state[(sizeof(weapon_trigger_state) == 0x28) ? 1 : -1];
typedef char check_weapon_magazine_state[(sizeof(weapon_magazine_state) == 0x0c) ? 1 : -1];
typedef char check_weapon_network_state[(sizeof(weapon_network_state) == 0x2c) ? 1 : -1];
typedef char check_equipment_network_state[(sizeof(equipment_network_state) == 0x24) ? 1 : -1];
typedef char check_weapon_hud_magazine[(sizeof(weapon_hud_magazine_state) == 0x0a) ? 1 : -1];
typedef char check_weapon_hud_ammo_state[(sizeof(weapon_hud_ammo_state) == 0x20) ? 1 : -1];
typedef char check_equipment_creation_message[(sizeof(equipment_creation_message) == 0x58) ? 1 : -1];
typedef char check_weapon_creation_message[(sizeof(weapon_creation_message) == 0x58) ? 1 : -1];
typedef char check_magazine_ammo_message[(sizeof(weapon_magazine_ammo_message) == 0x0a) ? 1 : -1];
typedef char check_ammo_pickup_message[(sizeof(weapon_ammo_pickup_message) == 0x08) ? 1 : -1];

// the item_data offsets item_update and item_compute_rotation pin, as object offsets
typedef char check_item_detonation[(0x1f4 + __builtin_offsetof(item_data, detonation_countdown) == 0x1f8) ? 1 : -1];
typedef char check_item_ignore_object[(0x1f4 + __builtin_offsetof(item_data, ignore_object_index) == 0x200) ? 1 : -1];
typedef char check_item_resting_object[(0x1f4 + __builtin_offsetof(item_data, resting_object_index) == 0x208) ? 1 : -1];
typedef char check_item_contact_point[(0x1f4 + __builtin_offsetof(item_data, contact_point) == 0x20c) ? 1 : -1];
typedef char check_item_rotation_axis[(0x1f4 + __builtin_offsetof(item_data, rotation_axis) == 0x218) ? 1 : -1];
typedef char check_item_rotation_sine[(0x1f4 + __builtin_offsetof(item_data, rotation_sine) == 0x224) ? 1 : -1];

// the weapon_data offsets weapon_update, weapon_fire_trigger and the network trio pin
typedef char check_weapon_control_flags[(0x22c + __builtin_offsetof(weapon_data, control_flags) == 0x230) ? 1 : -1];
typedef char check_weapon_primary_trigger[(0x22c + __builtin_offsetof(weapon_data, primary_trigger) == 0x234) ? 1 : -1];
typedef char check_weapon_state_field[(0x22c + __builtin_offsetof(weapon_data, state) == 0x238) ? 1 : -1];
typedef char check_weapon_action_ticks[(0x22c + __builtin_offsetof(weapon_data, action_ticks) == 0x23a) ? 1 : -1];
typedef char check_weapon_heat[(0x22c + __builtin_offsetof(weapon_data, heat) == 0x23c) ? 1 : -1];
typedef char check_weapon_age[(0x22c + __builtin_offsetof(weapon_data, age) == 0x240) ? 1 : -1];
typedef char check_weapon_charged[(0x22c + __builtin_offsetof(weapon_data, charged_fraction) == 0x244) ? 1 : -1];
typedef char check_weapon_ready_timer[(0x22c + __builtin_offsetof(weapon_data, ready_timer) == 0x248) ? 1 : -1];
typedef char check_weapon_tracked[(0x22c + __builtin_offsetof(weapon_data, tracked_object_index) == 0x250) ? 1 : -1];
typedef char check_weapon_alternate[(0x22c + __builtin_offsetof(weapon_data, alternate_shots_loaded) == 0x25c) ? 1 : -1];
typedef char check_weapon_triggers[(0x22c + __builtin_offsetof(weapon_data, triggers) == 0x260) ? 1 : -1];
typedef char check_weapon_magazines[(0x22c + __builtin_offsetof(weapon_data, magazines) == 0x2b0) ? 1 : -1];
typedef char check_weapon_overheat_effect[(0x22c + __builtin_offsetof(weapon_data, overheat_effect_handle) == 0x2cc) ? 1 : -1];
typedef char check_weapon_last_fire[(0x22c + __builtin_offsetof(weapon_data, last_fire_game_time) == 0x2d0) ? 1 : -1];
typedef char check_weapon_pred_unloaded[(0x22c + __builtin_offsetof(weapon_data, predicted_rounds_unloaded) == 0x2d4) ? 1 : -1];
typedef char check_weapon_pred_loaded[(0x22c + __builtin_offsetof(weapon_data, predicted_rounds_loaded) == 0x2d8) ? 1 : -1];
typedef char check_weapon_net_valid[(0x22c + __builtin_offsetof(weapon_data, network_state_valid) == 0x2e0) ? 1 : -1];
typedef char check_weapon_net_sequence[(0x22c + __builtin_offsetof(weapon_data, network_sequence) == 0x2e2) ? 1 : -1];
typedef char check_weapon_net_state[(0x22c + __builtin_offsetof(weapon_data, network_state) == 0x2e4) ? 1 : -1];
typedef char check_weapon_last_update_valid[(0x22c + __builtin_offsetof(weapon_data, last_update_valid) == 0x310) ? 1 : -1];
typedef char check_weapon_last_update_state[(0x22c + __builtin_offsetof(weapon_data, last_update_state) == 0x314) ? 1 : -1];

// the network state fields weapon_build_creation_message names, as object offsets
typedef char check_net_rounds_unloaded[(0x2e4 + __builtin_offsetof(weapon_network_state, rounds_unloaded) == 0x308) ? 1 : -1];
typedef char check_net_age[(0x2e4 + __builtin_offsetof(weapon_network_state, age) == 0x30c) ? 1 : -1];

// the trigger fields weapon_update pins, as object offsets for trigger 0 and trigger 1
typedef char check_trigger0_effect_state[(0x260 + __builtin_offsetof(weapon_trigger_state, effect_state) == 0x261) ? 1 : -1];
typedef char check_trigger1_effect_state[(0x260 + 0x28 + __builtin_offsetof(weapon_trigger_state, effect_state) == 0x289) ? 1 : -1];
typedef char check_trigger0_effect_ticks[(0x260 + __builtin_offsetof(weapon_trigger_state, effect_state_ticks) == 0x262) ? 1 : -1];
typedef char check_trigger0_flags[(0x260 + __builtin_offsetof(weapon_trigger_state, flags) == 0x264) ? 1 : -1];
typedef char check_trigger0_effect_index[(0x260 + __builtin_offsetof(weapon_trigger_state, firing_effect_index) == 0x26a) ? 1 : -1];
typedef char check_trigger0_effect_rounds[(0x260 + __builtin_offsetof(weapon_trigger_state, firing_effect_rounds) == 0x26c) ? 1 : -1];
typedef char check_trigger0_firing_rate[(0x260 + __builtin_offsetof(weapon_trigger_state, firing_rate) == 0x270) ? 1 : -1];
typedef char check_trigger0_ejection[(0x260 + __builtin_offsetof(weapon_trigger_state, ejection_port_recovery) == 0x274) ? 1 : -1];
typedef char check_trigger0_illumination[(0x260 + __builtin_offsetof(weapon_trigger_state, illumination_recovery) == 0x278) ? 1 : -1];
typedef char check_trigger0_error[(0x260 + __builtin_offsetof(weapon_trigger_state, error) == 0x27c) ? 1 : -1];
typedef char check_trigger0_effect_handle[(0x260 + __builtin_offsetof(weapon_trigger_state, effect_handle) == 0x280) ? 1 : -1];
typedef char check_trigger0_empty_ticks[(0x260 + __builtin_offsetof(weapon_trigger_state, empty_ticks) == 0x284) ? 1 : -1];
typedef char check_trigger1_ejection[(0x260 + 0x28 + __builtin_offsetof(weapon_trigger_state, ejection_port_recovery) == 0x29c) ? 1 : -1];

// the magazine fields item_add_ammunition and weapon_set_ammo_counts pin
typedef char check_mag0_rounds_unloaded[(0x2b0 + __builtin_offsetof(weapon_magazine_state, rounds_unloaded) == 0x2b6) ? 1 : -1];
typedef char check_mag0_rounds_loaded[(0x2b0 + __builtin_offsetof(weapon_magazine_state, rounds_loaded) == 0x2b8) ? 1 : -1];
typedef char check_mag1_state[(0x2b0 + 0x0c == 0x2bc) ? 1 : -1];
typedef char check_mag1_rounds_unloaded[(0x2b0 + 0x0c + __builtin_offsetof(weapon_magazine_state, rounds_unloaded) == 0x2c2) ? 1 : -1];
typedef char check_mag1_rounds_loaded[(0x2b0 + 0x0c + __builtin_offsetof(weapon_magazine_state, rounds_loaded) == 0x2c4) ? 1 : -1];

// the equipment network block equipment_create_from_creation_message pins
typedef char check_equipment_net_valid[(0x22c + __builtin_offsetof(equipment_data, network_state_valid) == 0x244) ? 1 : -1];
typedef char check_equipment_net_state[(0x22c + __builtin_offsetof(equipment_data, network_state) == 0x248) ? 1 : -1];
typedef char check_equipment_net_velocity[(0x248 + __builtin_offsetof(equipment_network_state, velocity) == 0x254) ? 1 : -1];
typedef char check_equipment_net_angular[(0x248 + __builtin_offsetof(equipment_network_state, angular_velocity) == 0x260) ? 1 : -1];
typedef char check_equipment_last_update[(0x22c + __builtin_offsetof(equipment_data, last_update_state) == 0x270) ? 1 : -1];

// the tag-side offsets every accessor in items.h quotes
typedef char check_item_flags_tag[(__builtin_offsetof(Item, item_flags) == 0x17c) ? 1 : -1];
typedef char check_item_material_effects[(__builtin_offsetof(Item, material_effects) == 0x248) ? 1 : -1];
typedef char check_item_collision_sound[(__builtin_offsetof(Item, collision_sound) == 0x258) ? 1 : -1];
typedef char check_item_detonation_delay[(__builtin_offsetof(Item, detonation_delay) == 0x2e0) ? 1 : -1];
typedef char check_equipment_pickup_sound[(__builtin_offsetof(Equipment, pickup_sound) == 0x310) ? 1 : -1];
typedef char check_weapon_flags_tag[(__builtin_offsetof(Weapon, weapon_flags) == 0x308) ? 1 : -1];
typedef char check_weapon_label[(__builtin_offsetof(Weapon, label) == 0x30c) ? 1 : -1];
typedef char check_weapon_secondary_mode[(__builtin_offsetof(Weapon, secondary_trigger_mode) == 0x32c) ? 1 : -1];
typedef char check_weapon_max_alternate[(__builtin_offsetof(Weapon, maximum_alternate_shots_loaded) == 0x32e) ? 1 : -1];
typedef char check_weapon_heat_recovery[(__builtin_offsetof(Weapon, heat_recovery_threshold) == 0x34c) ? 1 : -1];
typedef char check_weapon_overheated_threshold[(__builtin_offsetof(Weapon, overheated_threshold) == 0x350) ? 1 : -1];
typedef char check_weapon_heat_loss[(__builtin_offsetof(Weapon, heat_loss_rate) == 0x35c) ? 1 : -1];
typedef char check_weapon_zoom_levels[(__builtin_offsetof(Weapon, zoom_levels) == 0x3da) ? 1 : -1];
typedef char check_weapon_age_heat_penalty[(__builtin_offsetof(Weapon, age_heat_recovery_penalty) == 0x440) ? 1 : -1];
typedef char check_weapon_fp_animations[(__builtin_offsetof(Weapon, first_person_animations) == 0x46c) ? 1 : -1];
typedef char check_weapon_type_tag[(__builtin_offsetof(Weapon, weapon_type) == 0x4e2) ? 1 : -1];
typedef char check_weapon_magazines_tag[(__builtin_offsetof(Weapon, magazines) == 0x4f0) ? 1 : -1];
typedef char check_weapon_triggers_tag[(__builtin_offsetof(Weapon, triggers) == 0x4fc) ? 1 : -1];
typedef char check_weapon_magazine_tag[(sizeof(WeaponMagazine) == 0x70) ? 1 : -1];
typedef char check_magazine_reserved_max[(__builtin_offsetof(WeaponMagazine, rounds_reserved_maximum) == 0x08) ? 1 : -1];
typedef char check_magazine_loaded_max[(__builtin_offsetof(WeaponMagazine, rounds_loaded_maximum) == 0x0a) ? 1 : -1];
typedef char check_magazine_rounds_reloaded[(__builtin_offsetof(WeaponMagazine, rounds_reloaded) == 0x18) ? 1 : -1];
typedef char check_weapon_trigger_tag[(sizeof(WeaponTrigger) == 0x114) ? 1 : -1];
typedef char check_trigger_magazine[(__builtin_offsetof(WeaponTrigger, magazine) == 0x20) ? 1 : -1];
typedef char check_trigger_rounds_per_shot[(__builtin_offsetof(WeaponTrigger, rounds_per_shot) == 0x22) ? 1 : -1];
typedef char check_trigger_min_rounds[(__builtin_offsetof(WeaponTrigger, minimum_rounds_loaded) == 0x24) ? 1 : -1];
typedef char check_trigger_charging_time[(__builtin_offsetof(WeaponTrigger, charging_time) == 0x48) ? 1 : -1];
typedef char check_trigger_charged_time[(__builtin_offsetof(WeaponTrigger, charged_time) == 0x4c) ? 1 : -1];
typedef char check_trigger_overcharged[(__builtin_offsetof(WeaponTrigger, overcharged_action) == 0x50) ? 1 : -1];
typedef char check_trigger_spew_time[(__builtin_offsetof(WeaponTrigger, spew_time) == 0x58) ? 1 : -1];
typedef char check_trigger_projectile[(__builtin_offsetof(WeaponTrigger, projectile) == 0x94) ? 1 : -1];
typedef char check_trigger_ejection_time[(__builtin_offsetof(WeaponTrigger, ejection_port_recovery_time) == 0xa4) ? 1 : -1];
typedef char check_trigger_illum_time[(__builtin_offsetof(WeaponTrigger, illumination_recovery_time) == 0xa8) ? 1 : -1];
typedef char check_trigger_heat_per_round[(__builtin_offsetof(WeaponTrigger, heat_generated_per_round) == 0xb8) ? 1 : -1];
typedef char check_trigger_age_per_round[(__builtin_offsetof(WeaponTrigger, age_generated_per_round) == 0xbc) ? 1 : -1];
typedef char check_trigger_overload_time[(__builtin_offsetof(WeaponTrigger, overload_time) == 0xc4) ? 1 : -1];
typedef char check_trigger_illum_rate[(__builtin_offsetof(WeaponTrigger, illumination_recovery_rate) == 0xf0) ? 1 : -1];
typedef char check_trigger_firing_accel[(__builtin_offsetof(WeaponTrigger, firing_acceleration_rate) == 0xf8) ? 1 : -1];
typedef char check_trigger_error_accel[(__builtin_offsetof(WeaponTrigger, error_acceleration_rate) == 0x100) ? 1 : -1];
typedef char check_trigger_firing_effects[(__builtin_offsetof(WeaponTrigger, firing_effects) == 0x108) ? 1 : -1];

// the scenario placement record weapon_new_from_placement reads, and the three
// object_type_definition columns that name it
typedef char check_scenario_weapon_size[(sizeof(ScenarioWeapon) == 0x5c) ? 1 : -1];
typedef char check_scenario_weapon_reserved[(__builtin_offsetof(ScenarioWeapon, rounds_reserved) == 0x48) ? 1 : -1];
typedef char check_scenario_weapon_loaded[(__builtin_offsetof(ScenarioWeapon, rounds_loaded) == 0x4a) ? 1 : -1];
typedef char check_scenario_weapon_flags[(__builtin_offsetof(ScenarioWeapon, flags) == 0x4c) ? 1 : -1];
typedef char check_scenario_equipment_size[(sizeof(ScenarioEquipment) == 0x28) ? 1 : -1];
typedef char check_scenario_weapons_block[(__builtin_offsetof(Scenario, weapons) == 0x270) ? 1 : -1];
typedef char check_scenario_weapon_palette[(__builtin_offsetof(Scenario, weapon_palette) == 0x27c) ? 1 : -1];
typedef char check_scenario_equipment_block[(__builtin_offsetof(Scenario, equipment) == 0x258) ? 1 : -1];
typedef char check_scenario_equipment_palette[(__builtin_offsetof(Scenario, equipment_palette) == 0x264) ? 1 : -1];

// the object fields every item accessor in this module touches
typedef char check_object_size[(sizeof(object) == 0x1f4) ? 1 : -1];
typedef char check_object_velocity[(__builtin_offsetof(object, velocity) == 0x68) ? 1 : -1];
typedef char check_object_angular_velocity[(__builtin_offsetof(object, angular_velocity) == 0x8c) ? 1 : -1];
typedef char check_object_bounding_center[(__builtin_offsetof(object, bounding_center) == 0xa0) ? 1 : -1];
typedef char check_object_parent[(__builtin_offsetof(object, parent_object) == 0x11c) ? 1 : -1];
typedef char check_object_function_in[(__builtin_offsetof(object, function_in_values) == 0x124) ? 1 : -1];

int items_smoke(void)
{
    return (int)(sizeof(item_data) + sizeof(weapon_data) + sizeof(equipment_data) +
                 sizeof(garbage_data) + sizeof(weapon_trigger_state) +
                 sizeof(weapon_magazine_state) + sizeof(weapon_hud_ammo_state) +
                 sizeof(equipment_creation_message) + sizeof(weapon_creation_message) +
                 sizeof(weapon_magazine_ammo_message) + sizeof(weapon_ammo_pickup_message) +
                 _item_in_inventory_bit + _weapon_overheated_bit +
                 _weapon_control_primary_trigger_bit + _weapon_state_ready +
                 _weapon_trigger_effect_charged + _weapon_trigger_blur_applied_bit +
                 _weapon_magazine_chambering + k_maximum_weapon_triggers +
                 k_message_weapon_creation);
}
