/**
 * @file src/units/vars.cpp
 * Binds halo::units::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/units/vars.hpp"
#include "link/units_vars.hpp"
#include "halo/units/api.hpp"

namespace halo::units {

const Vars &vars()
{
    static const Vars table{
        ai_marker_name_a,
        ai_update_stagger,
        biped_detach_from_flipped_vehicle,
        control_binding_device_type,
        g_006966e4,
        global_identity_quaternion_pointer,
        global_zero_vector2d_pointer,
        global_zero_vector3d_pointer,
        ground_adjust_physics_model,
        is_dedicated_server_flag,
        k_biped_minimum_age_ticks,
        k_default_resting_plane,
        k_vehicle_minimum_age_ticks,
        network_object_index_cache,
        object_network_id_table,
        object_type_definitions_ex,
        object_update_gate_globals,
        placement_offset_table,
        s_blur_permutation,
        s_left_hand_marker,
        s_stand,
        unit_base_animation_state_names,
        unit_dialogue_variant_counter,
        unit_ground_adjust_node_positions,
        unit_speech_fallback_index,
        unit_speech_priority_table,
        unit_speech_repeat_seconds,
        unit_updates_suppressed,
        vehicle_network_update_period,
    };
    return table;
}

}  // namespace halo::units
