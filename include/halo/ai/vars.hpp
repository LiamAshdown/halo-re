/**
 * @file include/halo/ai/vars.hpp
 * Addresses of the engine variables the ai module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/ai_vars.hpp.
 */
#pragma once

namespace halo::ai {

/** Address table of the engine variables owned by the ai module. */
struct Vars {
    void *DAT_00655ab4;
    void *DAT_00656b24;
    void *actor_avoidance_a_bearing;
    void *actor_avoidance_a_elevation;
    void *actor_avoidance_a_radius;
    void *actor_avoidance_b_bearing;
    void *actor_avoidance_b_elevation;
    void *actor_avoidance_b_radius;
    void *actor_avoidance_circle;
    void *actor_avoidance_near_weights;
    void *actor_avoidance_ray_weights;
    void *actor_avoidance_samples_a;
    void *actor_avoidance_samples_b;
    void *actor_combat_status_min_grade;
    void *actor_control_animation_state_table;
    void *actor_data;
    void *actor_dialogue_variant_offset_1a;
    void *actor_dialogue_variant_offset_2a;
    void *actor_dialogue_variant_offset_3a;
    void *actor_dialogue_variant_scale_1b;
    void *actor_dialogue_variant_scale_23b;
    void *actor_dialogue_variant_scale_2a;
    void *actor_dialogue_variant_table_a;
    void *actor_dialogue_variant_table_b;
    void *actor_dialogue_variant_table_c;
    void *actor_dialogue_variant_table_d;
    void *actor_dialogue_variant_table_e;
    void *actor_dialogue_variant_table_f;
    void *actor_dialogue_variant_table_g;
    void *actor_dodge_table;
    void *actor_firing_position_reject_rules;
    void *actor_firing_position_score_rules;
    void *actor_lookup_table_006555a8;
    void *actor_mode_definitions;
    void *actor_mode_guard_look_weights_a5;
    void *actor_mode_guard_look_weights_a6;
    void *actor_mode_guard_look_weights_ambush;
    void *actor_mode_guard_look_weights_idle;
    void *actor_mode_uncover_look_weights_active;
    void *actor_type_procs;
    void *actor_vocalization_duration;
    void *actor_vocalization_variant;
    void *ai_actor_mode_dispatch_table;
    void *ai_communication_class_follow_up;
    void *ai_communication_class_look_marker;
    void *ai_communication_class_no_actor_class;
    void *ai_communication_class_priority;
    void *ai_communication_class_repeat_delay;
    void *ai_communication_class_tail_seconds;
    void *ai_communication_direction_table;
    void *ai_communication_event_definitions;
    void *ai_communication_lines;
    void *ai_communication_quiet_until_tick;
    void *ai_communication_selector_delay_seconds;
    void *ai_conversation_data;
    void *ai_default_2d_direction;
    void *ai_globals_ptr;
    void *ai_marker_name_b;
    void *ai_pursuit_data;
    void *ai_vocalization_line_table;
    void *communication_line_base;
    void *communication_line_count;
    void *conversation_index_lookup;
    void *conversation_line_base;
    void *conversation_line_count;
    void *encounter_data;
    void *encounter_platoon_states;
    void *encounter_squad_states;
    void *game_time;
    void *global_down3d_pointer;
    void *global_forward2d_pointer;
    void *global_origin3d_pointer;
    void *global_structure_bsp;
    void *k_random_scale_65536;
    void *k_real_one;
    void *k_real_point_six;
    void *k_real_zero;
    void *order_code_mode_data_expect;
    void *prop_array_name;
    void *prop_data;
    void *qsort_candidate_base;
    void *qsort_candidate_count;
    void *swarm_component_data;
    void *swarm_data;
    void *team_pair_data;
    void *ticks_per_second;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::ai
