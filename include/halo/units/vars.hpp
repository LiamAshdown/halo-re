/**
 * @file include/halo/units/vars.hpp
 * Addresses of the engine variables the units module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/units_vars.hpp.
 */
#pragma once

namespace halo::units {

/** Address table of the engine variables owned by the units module. */
struct Vars {
    void *ai_marker_name_a;
    void *ai_update_stagger;
    void *biped_detach_from_flipped_vehicle;
    void *control_binding_device_type;
    void *g_006966e4;
    void *global_identity_quaternion_pointer;
    void *global_zero_vector2d_pointer;
    void *global_zero_vector3d_pointer;
    void *ground_adjust_physics_model;
    void *is_dedicated_server_flag;
    void *k_biped_minimum_age_ticks;
    void *k_default_resting_plane;
    void *k_vehicle_minimum_age_ticks;
    void *network_object_index_cache;
    void *object_network_id_table;
    void *object_type_definitions_ex;
    void *object_update_gate_globals;
    void *placement_offset_table;
    void *s_blur_permutation;
    void *s_left_hand_marker;
    void *s_stand;
    void *unit_base_animation_state_names;
    void *unit_dialogue_variant_counter;
    void *unit_ground_adjust_node_positions;
    void *unit_speech_fallback_index;
    void *unit_speech_priority_table;
    void *unit_speech_repeat_seconds;
    void *unit_updates_suppressed;
    void *vehicle_network_update_period;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::units
