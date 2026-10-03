/* standalone/data/slice04.cpp -- engine globals 0x0069c648..0x006b1318 as extern "C" definitions (slice 4 of the
   globals->C conversion). Initial values are the retail .data/.rdata image bytes; pointers inside them are
   expressed in C (function names, string literals, &other_global). The marker comment before each definition
   ("0xADDR size N: name") is read by tools/globals_check_slice04.py.
   Not converted (still EQU in standalone/globals.asm), because their extents overlap other symbols: see the
   commit message / PLAN notes.

   All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "code_refs.hpp"
#include <stdint.h>

extern "C" {

extern char k_empty_string[];

/* 0x0069c648 size 4: rasterizer_present_counter_low */
int32_t rasterizer_present_counter_low;

/* 0x0069c64c size 4: rasterizer_present_counter_high */
int32_t rasterizer_present_counter_high;

/* 0x0069c65c size 4: rasterizer_default_z_near */
float rasterizer_default_z_near = 0.0625f;

/* 0x0069c660 size 4: rasterizer_default_z_far */
float rasterizer_default_z_far = 1024.0f;

/* 0x0069c664 size 8: rasterizer_frustum_z_values */
uint32_t rasterizer_frustum_z_values[2] = {
    0x3c400000u, 0x44800000u,
};

/* 0x0069c66c size 16: rasterizer_capture_surfaces */
void * rasterizer_capture_surfaces[4];

/* 0x0069c67c size 2: render_force_flag */
int16_t render_force_flag;

/* 0x0069c67e size 2: rasterizer_maximum_skinning_nodes */
int16_t rasterizer_maximum_skinning_nodes = 63;

/* 0x0069c680 size 1: rasterizer_software_vertex_processing */
uint8_t rasterizer_software_vertex_processing;

/* 0x0069c682 size 2: rasterizer_texture_stage_count */
int16_t rasterizer_texture_stage_count = 4;

/* 0x0069c684 size 4: renderer_unknown_69c684 */
uint32_t renderer_unknown_69c684 = 0xff202020u;

/* 0x0069c688 size 1: rasterizer_caps_flag_688 */
uint8_t rasterizer_caps_flag_688;

/* 0x0069c689 size 1: rasterizer_caps_flag_689 */
uint8_t rasterizer_caps_flag_689;

/* 0x0069c68a size 1: rasterizer_caps_flag_68a */
uint8_t rasterizer_caps_flag_68a;

/* 0x0069c694 size 4: rasterizer_frame_index */
int32_t rasterizer_frame_index = 12;

/* 0x0069c698 size 4: chat_gui_find_object_arg */
void * chat_gui_find_object_arg = (void *)L"KeystoneEditbox";

/* 0x0069c69c size 4: chat_listbox_gui_find_object_arg */
void * chat_listbox_gui_find_object_arg = (void *)L"KeystoneChatLog";

/* 0x0069c6a0 size 4: rasterizer_device_type */
uint32_t rasterizer_device_type = 0x1u;

/* 0x0069c6a4 size 4: rasterizer_window_style */
uint32_t rasterizer_window_style = 0xcf0000u;

/* 0x0069c6a8 size 1: rasterizer_fog_enabled */
uint8_t rasterizer_fog_enabled = 0x1;

/* 0x0069c6ac size 2: rasterizer_shader_stage_config */
int16_t rasterizer_shader_stage_config = -1;

/* 0x0069c6b0 size 4: decal_vertex_cache_last_queried */
int32_t decal_vertex_cache_last_queried = -1;

/* 0x0069c6b8 size 68: rasterizer_underwater_material */
uint8_t rasterizer_underwater_material[68] = {
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
};

/* 0x0069c6fc size 2: rasterizer_projected_light_shader_variant */
int16_t rasterizer_projected_light_shader_variant = -1;

/* 0x0069c708 size 68: rasterizer_default_material */
uint8_t rasterizer_default_material[68] = {
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
    0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x80, 0x3f,
};

/* 0x0069c74c size 1: rasterizer_render_states_dirty */
uint8_t rasterizer_render_states_dirty = 0x1;

/* 0x0069c750 size 3072: rasterizer_shader_technique_name_suffixes */
char rasterizer_shader_technique_name_suffixes[24][0x80] = {
    "MaskDetailBeforeReflectionBiasedMultiply",
    "MaskDetailBeforeReflectionMultiply",
    "MaskDetailBeforeReflectionBiasedAdd",
    "MaskDetailAfterReflectionBiasedMultiply",
    "MaskDetailAfterReflectionMultiply",
    "MaskDetailAfterReflectionBiasedAdd",
    "MaskInverseDetailBeforeReflectionBiasedMultiply",
    "MaskInverseDetailBeforeReflectionMultiply",
    "MaskInverseDetailBeforeReflectionBiasedAdd",
    "MaskInverseDetailAfterReflectionBiasedMultiply",
    "MaskInverseDetailAfterReflectionMultiply",
    "MaskInverseDetailAfterReflectionBiasedAdd",
    "MaskDetailBeforeReflectionBiasedMultiplyComplexFog",
    "MaskDetailBeforeReflectionMultiplyComplexFog",
    "MaskDetailBeforeReflectionBiasedAddComplexFog",
    "MaskDetailAfterReflectionBiasedMultiplyComplexFog",
    "MaskDetailAfterReflectionMultiplyComplexFog",
    "MaskDetailAfterReflectionBiasedAddComplexFog",
    "MaskInverseDetailBeforeReflectionBiasedMultiplyComplexFog",
    "MaskInverseDetailBeforeReflectionMultiplyComplexFog",
    "MaskInverseDetailBeforeReflectionBiasedAddComplexFog",
    "MaskInverseDetailAfterReflectionBiasedMultiplyComplexFog",
    "MaskInverseDetailAfterReflectionMultiplyComplexFog",
    "MaskInverseDetailAfterReflectionBiasedAddComplexFog",
};

/* 0x0069d350 size 2: rasterizer_active_render_target */
int16_t rasterizer_active_render_target = -1;

/* 0x0069d358 size 180: rasterizer_render_targets */
uint32_t rasterizer_render_targets[45] = {
    0x280u, 0x1e0u, 0x15u, 0x0u, 0x0u, 0x280u,
    0x1e0u, 0x15u, 0x0u, 0x0u, 0x140u, 0xf0u,
    0x15u, 0x0u, 0x0u, 0x80u, 0x80u, 0x16u,
    0x0u, 0x0u, 0x80u, 0x80u, 0x16u, 0x0u,
    0x0u, 0x40u, 0x40u, 0x16u, 0x0u, 0x0u,
    0x40u, 0x40u, 0x15u, 0x0u, 0x0u, 0x40u,
    0x40u, 0x16u, 0x0u, 0x0u, 0x80u, 0x80u,
    0x16u,
};

/* 0x0069e550 size 1: rasterizer_object_shadow_window_restored */
uint8_t rasterizer_object_shadow_window_restored = 0x1;

/* 0x0069e558 size 216: rasterizer_transparent_vertex_shader_table */
int16_t rasterizer_transparent_vertex_shader_table[108] = {
    37, 44, 42, 40, 46, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    39, 45, 43, 41, 47, 38, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    37, 44, 42, 40, 46, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,
};

/* 0x0069e630 size 216: rasterizer_transparent_extended_vertex_shader_table */
int16_t rasterizer_transparent_extended_vertex_shader_table[108] = {
    37, 44, 42, 40, 46, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    39, 45, 43, 41, 47, 38, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    37, 44, 42, 40, 46, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,
};

/* 0x0069e708 size 4: lens_flare_vertex_specular */
uint32_t lens_flare_vertex_specular = 0xffffffu;

/* 0x0069e710 size 156: rasterizer_chicago_color_function_stage_states */
uint32_t rasterizer_chicago_color_function_stage_states[39] = {
    0x2u, 0x1u, 0x0u, 0x2u, 0x2u, 0x0u,
    0x4u, 0x2u, 0x1u, 0x5u, 0x2u, 0x1u,
    0x7u, 0x2u, 0x1u, 0x9u, 0x2u, 0x1u,
    0x9u, 0x1u, 0x2u, 0xau, 0x2u, 0x1u,
    0xau, 0x1u, 0x2u, 0x10u, 0x2u, 0x1u,
    0x10u, 0x1u, 0x2u, 0xdu, 0x2u, 0x1u,
    0xdu, 0x1u, 0x2u,
};

/* 0x0069e7ac size 4: game_state_before_save_proc */
void * game_state_before_save_proc = function_do_nothing;

/* 0x0069e7b0 size 4: game_state_revert_proc */
void * game_state_revert_proc = game_sound_revert_scripting_sounds;

/* 0x0069e7b4 size 52: game_state_after_load_procs */
void * game_state_after_load_procs[13] = {
    scenario_structure_bsp_switch_after_load, sound_stop_all,
    game_sound_reconcile_scripting_state, observer_initialize,
    update_queues_revert, decal_geometry_cache_restore_procs,
    function_do_nothing, function_do_nothing,
    detail_objects_invalidate, game_state_after_load_restore_time,
    players_rebind_local_player_after_load, director_game_state_loaded,
    hud_messaging_clear_after_load,
};

/* 0x0069e7e8 size 1: checkpoint_sort_newest_first */
uint8_t checkpoint_sort_newest_first = 0x1;

/* 0x0069e7f0 size 72: player_color_table */
uint32_t player_color_table[18] = {
    0xffffffu, 0x0u, 0xfe0000u, 0x201e3u, 0x707e71u, 0xffff01u,
    0xff01u, 0xff56b9u, 0xab10f4u, 0x1ffffu, 0x6493edu, 0xff7f00u,
    0x1ecc91u, 0x6401u, 0x603814u, 0xc69c6cu, 0x9d0b0eu, 0xf5999eu,
};

/* 0x0069e838 size 152: default_game_variant_procs */
void * default_game_variant_procs[38] = {
    game_engine_variant_defaults_classic_slayer, game_engine_variant_defaults_classic_slayer_pro,
    game_engine_variant_defaults_classic_elimination, game_engine_variant_defaults_classic_phantoms,
    game_engine_variant_defaults_classic_endurance, game_engine_variant_defaults_classic_rockets,
    game_engine_variant_defaults_classic_snipers, game_engine_variant_defaults_classic_oddball,
    game_engine_variant_defaults_classic_reverse_tag, game_engine_variant_defaults_classic_accumulation,
    game_engine_variant_defaults_classic_juggernaut, game_engine_variant_defaults_classic_stalker,
    game_engine_variant_defaults_classic_king, game_engine_variant_defaults_classic_king_pro,
    game_engine_variant_defaults_classic_crazy_king, game_engine_variant_defaults_classic_race,
    game_engine_variant_defaults_classic_rally, game_engine_variant_defaults_classic_ctf,
    game_engine_variant_defaults_classic_invasion, game_engine_variant_defaults_classic_iron_ctf,
    game_engine_variant_defaults_classic_ctf_pro, game_engine_variant_defaults_classic_team_race,
    game_engine_variant_defaults_classic_team_rally, game_engine_variant_defaults_classic_team_oddball,
    game_engine_variant_defaults_classic_team_king, game_engine_variant_defaults_classic_team_slayer,
    game_engine_variant_defaults_slayer, game_engine_variant_defaults_oddball,
    game_engine_variant_defaults_juggernaut, game_engine_variant_defaults_king,
    game_engine_variant_defaults_crazy_king, game_engine_variant_defaults_race,
    game_engine_variant_defaults_stalker, game_engine_variant_defaults_assault,
    game_engine_variant_defaults_team_slayer, game_engine_variant_defaults_team_oddball,
    game_engine_variant_defaults_team_king, game_engine_variant_defaults_team_race,
};

/* 0x0069e8d0 size 1: playlist_profiles_need_defaults */
uint8_t playlist_profiles_need_defaults = 0x1;

/* 0x0069e8d4 size 4: global_scenario_index */
int32_t global_scenario_index = -1;

/* 0x0069e8d8 size 2: global_structure_bsp_index */
int16_t global_structure_bsp_index = -1;

/* 0x0069e8dc size 52: structure_bsp_activate_procedures */
void * structure_bsp_activate_procedures[13] = {
    objects_recompute_cluster_membership, object_lights_refresh_transforms,
    ai_unassigned_actors_attach_to_structure_bsp, effects_refresh_structure_locations,
    particles_refresh_structure_locations, particle_system_resolve_local_players,
    contrail_refresh_lightmap, decal_rehash_object_decals,
    structure_runtime_decals_mark_dirty, observer_update_location,
    players_structure_bsp_switch_regroup, sounds_refresh_structure_locations,
    scenario_objects_place_for_structure_bsp_on_activate,
};

/* 0x0069e910 size 40: structure_bsp_deactivate_procedures */
void * structure_bsp_deactivate_procedures[10] = {
    objects_delete_unparented_of_type_mask, object_sweep_refresh_cluster_membership,
    object_lights_detach_from_structure_bsp, ai_reset_fire_group_assignments,
    function_do_nothing, function_do_nothing,
    function_do_nothing, function_do_nothing,
    structure_runtime_decals_evict, decals_detach_from_structure_bsp,
};

/* 0x0069e940 size 184: glow_sprite_shader */
uint32_t glow_sprite_shader[46] = {
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x1u, 0x30000u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x6269746du,
    (uint32_t)k_empty_string, 0x0u, 0xffffffffu, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u,
};

/* 0x0069e9f8 size 184: antenna_sprite_shader */
uint32_t antenna_sprite_shader[46] = {
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x1u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x6269746du,
    (uint32_t)k_empty_string, 0x0u, 0xffffffffu, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x1u,
};

/* 0x0069eab0 size 4: security_write_access_state */
int32_t security_write_access_state = -1;

/* 0x0069eab4 size 4: shell_instance_mode_value */
int32_t shell_instance_mode_value = -1;

/* 0x0069eab8 size 36: shell_instance_mutex_names */
void * shell_instance_mutex_names[9] = {
    (void *)"Global\\{cff2f1a0-500e-4fae-a789-ceaa827d8c08}", (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}1",
    (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}2", (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}3",
    (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}4", (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}5",
    (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}6", (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}7",
    (void *)"Global\\{38642482-262e-4413-bd01-01b4ccd937ae}8",
};

/* 0x0069eae0 size 2244: sound_class_definitions */
uint32_t sound_class_definitions[561] = {
    0x20003u, 0x64u, 0x40000u, 0x0u, 0x3f000000u, 0x0u,
    0x3fb33333u, 0x41000000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x10004u,
    0xc8u, 0x50000u, 0x1u, 0x3f000000u, 0x0u, 0x41000000u,
    0x42f00000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x10004u, 0x0u, 0x40000u, 0x1u,
    0x3f000000u, 0x0u, 0x40800000u, 0x428c0000u, 0x3f800000u, 0x3f800000u,
    0x0u, 0x10004u, 0x1f4u, 0x40000u, 0x1u, 0x3f000000u,
    0x0u, 0x3f800000u, 0x41100000u, 0x3f800000u, 0x3f800000u, 0x0u,
    0x10004u, 0x1f4u, 0x40000u, 0x1u, 0x3f000000u, 0x0u,
    0x3f800000u, 0x41100000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x10004u,
    0x3cu, 0x40000u, 0x1u, 0x3f000000u, 0x0u, 0x3f800000u,
    0x41100000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x10004u, 0x1f4u,
    0x40000u, 0x1u, 0x3f000000u, 0x0u, 0x3f800000u, 0x41100000u,
    0x3f800000u, 0x3f800000u, 0x0u, 0x10004u, 0x1f4u, 0x40000u,
    0x1u, 0x3f000000u, 0x0u, 0x3f800000u, 0x41100000u, 0x3f800000u,
    0x3f800000u, 0x0u, 0x10004u, 0x1f4u, 0x40000u, 0x1u,
    0x3f000000u, 0x0u, 0x3f800000u, 0x41100000u, 0x3f800000u, 0x3f800000u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x10002u,
    0x64u, 0x30000u, 0x1u, 0x3f000000u, 0x0u, 0x3f000000u,
    0x40400000u, 0x0u, 0x3f800000u, 0x0u, 0x10002u, 0x64u,
    0x30000u, 0x0u, 0x3f000000u, 0x0u, 0x3f000000u, 0x40400000u,
    0x0u, 0x3f800000u, 0x0u, 0x10002u, 0x3e8u, 0x30000u,
    0x0u, 0x3f000000u, 0x0u, 0x3f000000u, 0x40400000u, 0x0u,
    0x3f800000u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x10004u, 0xc8u, 0x30000u, 0x0u, 0x3f000000u, 0x0u,
    0x3f666666u, 0x41200000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x10004u,
    0x64u, 0x30001u, 0x1u, 0x3f4ccccdu, 0x0u, 0x40400000u,
    0x41a00000u, 0x0u, 0x3f800000u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x20004u, 0x190u, 0x30000u, 0x0u,
    0x3f000000u, 0x0u, 0x3fb33333u, 0x41000000u, 0x3f800000u, 0x3f800000u,
    0x0u, 0x20004u, 0x64u, 0x30000u, 0x1u, 0x3f666666u,
    0x0u, 0x3fb33333u, 0x41000000u, 0x3f800000u, 0x3f800000u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x10004u, 0x64u,
    0x20000u, 0x1u, 0x3f000000u, 0x0u, 0x3f666666u, 0x40a00000u,
    0x3f800000u, 0x3f800000u, 0x0u, 0x10004u, 0x64u, 0x20000u,
    0x1u, 0x3f000000u, 0x0u, 0x3f666666u, 0x40a00000u, 0x3f800000u,
    0x3f800000u, 0x0u, 0x10004u, 0x64u, 0x10000u, 0x1u,
    0x3f000000u, 0x0u, 0x3f666666u, 0x40a00000u, 0x3f800000u, 0x3f800000u,
    0x0u, 0x10004u, 0x64u, 0x10000u, 0x1u, 0x3f000000u,
    0x0u, 0x3f666666u, 0x40a00000u, 0x3f800000u, 0x3f800000u, 0x0u,
    0x10004u, 0x64u, 0x10000u, 0x1u, 0x3f000000u, 0x0u,
    0x3f000000u, 0x40400000u, 0x3f800000u, 0x3f800000u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x40004u, 0x64u,
    0x20000u, 0x1u, 0x3f800000u, 0x0u, 0x3f666666u, 0x40a00000u,
    0x0u, 0x3f800000u, 0x0u, 0x10004u, 0x64u, 0x10000u,
    0x1u, 0x3f800000u, 0x0u, 0x3f666666u, 0x40a00000u, 0x0u,
    0x3f800000u, 0x0u, 0x10004u, 0x64u, 0x10000u, 0x1u,
    0x3f800000u, 0x0u, 0x3f666666u, 0x40a00000u, 0x0u, 0x3f800000u,
    0x0u, 0x10004u, 0x64u, 0x10000u, 0x1u, 0x3f800000u,
    0x0u, 0x3f000000u, 0x40400000u, 0x0u, 0x3f800000u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x10004u, 0x64u, 0x40000u,
    0x1u, 0x3f800000u, 0x0u, 0x3f000000u, 0x40400000u, 0x3f800000u,
    0x3f800000u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x40004u, 0x64u,
    0x60001u, 0x1u, 0x3f4ccccdu, 0x0u, 0x40400000u, 0x41a00000u,
    0x0u, 0x3f800000u, 0x0u, 0x40004u, 0x64u, 0x30000u,
    0x1u, 0x3f4ccccdu, 0x0u, 0x40000000u, 0x40a00000u, 0x0u,
    0x3f800000u, 0x0u, 0x40004u, 0x64u, 0x50001u, 0x1u,
    0x3f4ccccdu, 0x0u, 0x40400000u, 0x41a00000u, 0x0u, 0x3f800000u,
    0x0u, 0x40004u, 0x64u, 0x60001u, 0x1u, 0x3f4ccccdu,
    0x0u, 0x40400000u, 0x41a00000u, 0x0u, 0x3f800000u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
    0x0u, 0x0u, 0x0u, 0x0u, 0x10004u, 0x64u,
    0x50000u, 0x1u, 0x3f800000u, 0x0u, 0x40400000u, 0x41a00000u,
    0x3f800000u, 0x3f800000u,
};

/* 0x0069f3a8 size 204: sound_class_names */
void * sound_class_names[51] = {
    (void *)"projectile_impact", (void *)"projectile_detonation",
    k_empty_string, k_empty_string,
    (void *)"weapon_fire", (void *)"weapon_ready",
    (void *)"weapon_reload", (void *)"weapon_empty",
    (void *)"weapon_charge", (void *)"weapon_overheat",
    (void *)"weapon_idle", k_empty_string,
    k_empty_string, (void *)"object_impacts",
    (void *)"particle_impacts", (void *)"slow_particle_impacts",
    k_empty_string, k_empty_string,
    (void *)"unit_footsteps", (void *)"unit_dialog",
    k_empty_string, k_empty_string,
    (void *)"vehicle_collision", (void *)"vehicle_engine",
    k_empty_string, k_empty_string,
    (void *)"device_door", (void *)"device_force_field",
    (void *)"device_machinery", (void *)"device_nature",
    (void *)"device_computers", k_empty_string,
    (void *)"music", (void *)"ambient_nature",
    (void *)"ambient_machinery", (void *)"ambient_computers",
    k_empty_string, k_empty_string,
    k_empty_string, (void *)"first_person_damage",
    k_empty_string, k_empty_string,
    k_empty_string, k_empty_string,
    (void *)"scripted_dialog_player", (void *)"scripted_effect",
    (void *)"scripted_dialog_other", (void *)"scripted_dialog_force_unspatialized",
    k_empty_string, k_empty_string,
    (void *)"game_event",
};

/* 0x0069f4c0 size 4: directsound_rolloff_factor */
float directsound_rolloff_factor = 1.0f;

/* 0x0069f4c8 size 64: directsound_driver */
uint32_t directsound_driver[16] = {
    0x0u, (uint32_t)sound_driver_initialize, (uint32_t)sound_driver_dispose, (uint32_t)sound_listener_update,
    (uint32_t)sound_driver_begin_frame, (uint32_t)sound_driver_end_frame, (uint32_t)sound_driver_channel_play, (uint32_t)sound_driver_channel_continue,
    (uint32_t)sound_driver_channel_stop, (uint32_t)sound_driver_channel_get_state, (uint32_t)sound_driver_set_paused, (uint32_t)sound_driver_stop_all,
    (uint32_t)sound_driver_channel_set_spatial, (uint32_t)sound_driver_channel_set_parameters, (uint32_t)sound_driver_set_quality, (uint32_t)sound_driver_eax_available,
};

/* 0x0069f508 size 8: sound_drivers */
void * sound_drivers[2] = {
    &directsound_driver, 0,
};

/* 0x0069f510 size 4: sound_fade_curve_exponent */
float sound_fade_curve_exponent = 2.5f;

/* 0x0069f514 size 20: driver_parameters */
uint32_t driver_parameters[5] = {
    0x160000u, 0x20002u, 0x160002u, 0x20002u, 0x2u,
};

/* 0x0069f528 size 8: sound_channel_type_flag_table */
uint16_t sound_channel_type_flag_table[4] = {
    0x9, 0x8, 0xa, 0xe,
};

/* 0x0069fa40 size 4: picked_leaf_map_leaf */
int32_t picked_leaf_map_leaf = -1;

/* 0x0069fa44 size 4: picked_leaf_map_portal */
int32_t picked_leaf_map_portal = -1;

/* 0x0069fa48 size 2: geometry_buffer_warning */
int16_t geometry_buffer_warning = 1;

/* 0x0069fa4c size 4: k_cluster_query_radius_threshold */
float k_cluster_query_radius_threshold = 2.0f;

/* 0x0069fa50 size 4: file_root_template */
char file_root_template[4] = "\077:\\";

/* 0x0069fa58 size 4: file_enumeration_flags_value */
uint32_t file_enumeration_flags_value;

/* 0x0069fa5c size 4: file_enumeration_pos */
uint32_t file_enumeration_pos = 0xffffu;

/* 0x0069fa60 size 256: file_enumeration_path */
char file_enumeration_path[256];

/* 0x0069fb60 size 32: file_enumeration_handles */
void * file_enumeration_handles[8] = {
    (void *)-1, (void *)-1,
    (void *)-1, (void *)-1,
    (void *)-1, (void *)-1,
    (void *)-1, (void *)-1,
};

/* 0x0069fba0 size 320: file_enumeration_find_data */
uint32_t file_enumeration_find_data[80];

/* 0x0069fde0 size 2: magic_seat_animation_state_0069fde0 */
int16_t magic_seat_animation_state_0069fde0 = -1;

/* 0x0069fdfc size 4: network_console_connection_id */
int32_t network_console_connection_id = -1;

/* 0x0069fe00 size 1: network_session_host_flags_byte */
uint8_t network_session_host_flags_byte = 0x1;

/* 0x0069fe04 size 4: autopatch_update_check_state */
int32_t autopatch_update_check_state = -1;

/* 0x0069fe3c size 4: config_maximum_resolution */
int32_t config_maximum_resolution = 4096;

/* 0x0069fe40 size 224: config_properties */
void * config_properties[56] = {
    (void *)"ForceShader", config_set_force_shader,
    (void *)"DisableDriverManagement", config_set_disable_driver_management,
    (void *)"LinearTextureAddressing", config_set_linear_texture_addressing,
    (void *)"LinearTextureAddressingZoom", config_set_linear_texture_addressing_zoom,
    (void *)"LinearTextureAddressingSun", config_set_linear_texture_addressing_sun,
    (void *)"MaximumResolution", config_set_maximum_resolution,
    (void *)"UseFixedFunction", config_set_use_fixed_function,
    (void *)"UnsupportedCard", config_set_unsupported_card,
    (void *)"DisableRenderTargets", config_set_disable_render_targets,
    (void *)"DisableAlphaRenderTargets", config_set_disable_alpha_render_targets,
    (void *)"DisableBuffering", config_set_disable_buffering,
    (void *)"UseAlternateConvolveMask", config_set_use_alternate_convolve_mask,
    (void *)"OldDriver", config_set_old_driver,
    (void *)"EnableStopStart", config_set_enable_stop_start,
    (void *)"HeadRelativeSpeech", config_set_head_relative_speech,
    (void *)"OldSoundDriver", config_set_old_sound_driver,
    (void *)"InvalidDriver", config_set_invalid_driver,
    (void *)"InvalidSoundDriver", config_set_invalid_sound_driver,
    (void *)"SafeMode", config_set_safe_mode,
    (void *)"DisableSpecular", config_set_disable_specular,
    (void *)"UseAnisotropicFilter", config_set_use_anisotropic_filter,
    (void *)"UMA", config_compute_uma_video_memory,
    (void *)"MinMaxBlendOpIsBroken", config_set_min_max_blend_op_is_broken,
    (void *)"DecalZBiasValue", config_set_decal_z_bias,
    (void *)"DecalSlopeZBiasValue", config_set_decal_slope_z_bias,
    (void *)"TransparentDecalZBiasValue", config_set_transparent_decal_z_bias,
    (void *)"TransparentDecalSlopeZBiasValue", config_set_transparent_decal_slope_z_bias,
    (void *)"PrototypeCard", config_set_prototype_card,
};

/* 0x0069ff20 size 4: shell_language_id */
uint32_t shell_language_id = 0x409u;

/* 0x0069ff24 size 4: sound_eax20_underwater_direct_gain */
float sound_eax20_underwater_direct_gain = 0.25f;

/* 0x0069ff28 size 4: sound_underwater_direct_gain */
float sound_underwater_direct_gain = 0.25f;

/* 0x006a24c8 size 512: natneg_receive_buffer */
char natneg_receive_buffer[512];

/* 0x006a26c8 size 4: negotiatorList */
void * negotiatorList;

/* 0x006a26cc size 4: matchup1ip */
uint32_t matchup1ip;

/* 0x006a26d0 size 4: matchup2ip */
uint32_t matchup2ip;

/* 0x006a26d8 size 20: qr2_local_ips */
uint32_t qr2_local_ips[5];

/* 0x006a26f0 size 256: qr2_receive_buffer */
char qr2_receive_buffer[256];

/* 0x006a27f0 size 4: qr2_local_ip_count */
int32_t qr2_local_ip_count;

/* 0x006a27f8 size 4: SBNullServer */
void * SBNullServer;

/* 0x006a2800 size 8: gcd_xor_key */
char gcd_xor_key[8];

/* 0x006a2810 size 1024: gcd_receive_buffer */
char gcd_receive_buffer[1024];

/* 0x006a2c10 size 16: gcd_master_address */
uint32_t gcd_master_address[4];

/* 0x006a2e60 size 2: gcd_own_socket */
uint16_t gcd_own_socket;

/* 0x006a2e64 size 4: gcd_game_count */
int32_t gcd_game_count;

/* 0x006a2e68 size 4: gcd_no_network */
int32_t gcd_no_network;

/* 0x006a2e6c size 4: ghiReferenceCount */
int32_t ghiReferenceCount;

/* 0x006a2e70 size 256: ptaKeyValue */
char ptaKeyValue[256];

/* 0x006a3070 size 512: ptaURL */
char ptaURL[512];

/* 0x006a3278 size 4: SBOverrideMasterServer */
void * SBOverrideMasterServer;

/* 0x006a327c size 4: ghiConnectionsLen */
int32_t ghiConnectionsLen;

/* 0x006a3280 size 4: ghiNumConnections */
int32_t ghiNumConnections;

/* 0x006a3284 size 4: ghiConnections */
void * ghiConnections;

/* 0x006a3288 size 4: ghiNextUniqueID */
int32_t ghiNextUniqueID;

/* 0x006a328c size 4: ghiLockHandle */
void * ghiLockHandle;

/* 0x006a32e8 size 4: shell_module_path */
void * shell_module_path;

/* 0x006a3f38 size 256: network_address_string */
char network_address_string[256];

/* 0x006a4038 size 4: network_connection_log_last_row_ms */
int32_t network_connection_log_last_row_ms;

/* 0x006a4040 size 256: network_local_hostname_buffer */
char network_local_hostname_buffer[256];

/* 0x006a4140 size 8192: network_game_receive_buffer */
uint8_t network_game_receive_buffer[8192];

/* 0x006a6140 size 4: network_summary_log_file */
void * network_summary_log_file;

/* 0x006a6144 size 4: network_high_res_clock_ms */
int32_t network_high_res_clock_ms;

/* 0x006a6148 size 8192: network_query_receive_buffer */
uint8_t network_query_receive_buffer[8192];

/* 0x006a8148 size 4: network_connection_log_start_ms */
int32_t network_connection_log_start_ms;

/* 0x006a8150 size 1: cache_file_loaded */
uint8_t cache_file_loaded;

/* 0x006a8954 size 4: tag_header */
void * tag_header;

/* 0x006a8958 size 4: structure_bsp_data */
void * structure_bsp_data;

/* 0x006a9428 size 12360: cache_file_slots */
uint32_t cache_file_slots[3090];

/* 0x006ac470 size 1: map_download_in_progress */
uint8_t map_download_in_progress;

/* 0x006ac472 size 2: map_download_slot_index */
int16_t map_download_slot_index;

/* 0x006ac474 size 32: map_download_name */
char map_download_name[32];

/* 0x006ac494 size 2: cache_file_index */
int16_t cache_file_index;

/* 0x006ac498 size 4: cache_io_event */
void * cache_io_event;

/* 0x006ac49c size 4: cache_io_thread */
void * cache_io_thread;

/* 0x006ac4a0 size 4: cache_io_requests */
void * cache_io_requests;

/* 0x006ac4a8 size 64: sounds_data_file */
uint32_t sounds_data_file[16];

/* 0x006ac4e8 size 64: bitmaps_data_file */
uint32_t bitmaps_data_file[16];

/* 0x006ac528 size 4: sound_cache_entries */
void * sound_cache_entries;

/* 0x006ac52c size 4: sound_cache_base */
void * sound_cache_base;

/* 0x006ac530 size 4: sound_cache */
void * sound_cache;

/* 0x006ac534 size 1: sound_cache_initialized */
uint8_t sound_cache_initialized;

/* 0x006ac538 size 4: texture_cache_entries */
void * texture_cache_entries;

/* 0x006ac53c size 4: texture_cache_base */
void * texture_cache_base;

/* 0x006ac540 size 4: texture_cache */
void * texture_cache;

/* 0x006ac548 size 4: map_memory */
void * map_memory;

/* 0x006ac54c size 4: tag_data_base */
void * tag_data_base;

/* 0x006ac550 size 4: texture_cache_memory */
void * texture_cache_memory;

/* 0x006ac554 size 4: sound_cache_memory */
void * sound_cache_memory;

/* 0x006ac558 size 8: camera_director_globals */
uint32_t camera_director_globals[2];

/* 0x006ac658 size 4: observer_dt */
float observer_dt;

/* 0x006ac8f8 size 8: performance_frequency */
int64_t performance_frequency;

/* 0x006ac900 size 261: profile_directory */
char profile_directory[261];

/* 0x006b0a10 size 4: recorded_animations */
void * recorded_animations;

/* 0x006b0a18 size 192: decal_clip_buffers */
float decal_clip_buffers[48];

/* 0x006b0ad8 size 4: decal_grid_block */
void * decal_grid_block;

/* 0x006b0adc size 4: effect_marker_callback_context */
void * effect_marker_callback_context;

/* 0x006b0ae0 size 4: weather_instance_count */
int32_t weather_instance_count;

/* 0x006b0ae4 size 156: weather_instances */
uint32_t weather_instances[39];

/* 0x006b0b84 size 4: team_pair_data */
void * team_pair_data;

/* 0x006b0b88 size 768: player_profile_cache */
uint32_t player_profile_cache[192];

/* 0x006b0ebc size 1: ctf_single_flag_mode */
uint8_t ctf_single_flag_mode;

/* 0x006b106c size 2: game_engine_recent_location_count */
int16_t game_engine_recent_location_count;

/* 0x006b1070 size 128: game_engine_recent_location_table */
int16_t game_engine_recent_location_table[64];

/* 0x006b10f0 size 80: multiplayer_sound_queue */
uint32_t multiplayer_sound_queue[20];

/* 0x006b1140 size 4: multiplayer_sound_queue_count */
int32_t multiplayer_sound_queue_count;

}
