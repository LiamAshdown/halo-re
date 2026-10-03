/**
 * @file standalone/data/link/units_vars.hpp
 * Link names of the engine variables owned by the units module (halo::units::vars()). The data image defines them under these C
 * names; only src/units/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char ai_marker_name_a[];
extern char ai_update_stagger[];
extern char biped_detach_from_flipped_vehicle[];
extern char control_binding_device_type[];
extern char g_006966e4[];
extern char global_identity_quaternion_pointer[];
extern char global_zero_vector2d_pointer[];
extern char global_zero_vector3d_pointer[];
extern char ground_adjust_physics_model[];
extern char is_dedicated_server_flag[];
extern char k_biped_minimum_age_ticks[];
extern char k_default_resting_plane[];
extern char k_vehicle_minimum_age_ticks[];
extern char network_object_index_cache[];
extern char object_network_id_table[];
extern char object_type_definitions_ex[];
extern char object_update_gate_globals[];
extern char placement_offset_table[];
extern char s_blur_permutation[];
extern char s_left_hand_marker[];
extern char s_stand[];
extern char unit_base_animation_state_names[];
extern char unit_dialogue_variant_counter[];
extern char unit_ground_adjust_node_positions[];
extern char unit_speech_fallback_index[];
extern char unit_speech_priority_table[];
extern char unit_speech_repeat_seconds[];
extern char unit_updates_suppressed[];
extern char vehicle_network_update_period[];
}
