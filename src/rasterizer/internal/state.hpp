#pragma once

/**
 * @file src/rasterizer/internal/state.hpp
 * Shared declarations for the rasterizer implementation files: the engine globals and external functions
 * (C linkage) and the prototypes of the halo::rasterizer implementation functions.
 */

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "halo/rasterizer/layout_checks.hpp"
#include "render.h"
#include "interface.h"
#include <string.h>
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>
#include "effects.h"
#include "crt.h"
#include <wchar.h>
#include <stdlib.h>
#include "game.h"
#include <stdio.h>
#include "bitmaps.h"
#include "halo/rasterizer/render_device.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/globals.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"


inline auto &direct3d_create9_procedure = halo::link::ref<void *(__stdcall *)(uint32_t sdk_version)>(halo::rasterizer::vars().direct3d_create9_procedure);

inline auto &bitmap_format_bits_per_pixel = halo::link::ref<int8_t []>(halo::rasterizer::vars().bitmap_format_bits_per_pixel);
inline auto &cinematic_screen_effect_state = halo::link::ref<cinematic_screen_effect_globals *>(halo::cutscene::vars().cinematic_screen_effect_state);
inline auto &rasterizer_default_z_near = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_default_z_near);
inline auto &rasterizer_time = halo::link::ref<rasterizer_frame_time>(halo::rasterizer::vars().rasterizer_time);
inline auto &rasterizer_caps = halo::link::ref<d3d_caps9>(halo::rasterizer::vars().rasterizer_caps);
#ifndef HALO_LINKED_unknown_0071d275
#define HALO_LINKED_unknown_0071d275
inline auto &unknown_0071d275 = halo::link::ref<uint8_t>(halo::rasterizer::vars().unknown_0071d275);
#endif
#ifndef HALO_LINKED_unknown_0071d276
#define HALO_LINKED_unknown_0071d276
inline auto &unknown_0071d276 = halo::link::ref<uint8_t>(halo::rasterizer::vars().unknown_0071d276);
#endif
inline auto &decals_for_all_responses = halo::link::ref<uint8_t>(halo::effects::vars().decals_for_all_responses);
inline auto &rasterizer_decal_vertex_cache_handle = halo::link::ref<::cache *>(halo::effects::vars().rasterizer_decal_vertex_cache_handle);
inline auto &text_rendering_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().text_rendering_enabled);
inline auto &rasterizer_window = halo::link::ref<rasterizer_window_parameters>(halo::rasterizer::vars().rasterizer_window);
inline auto &g_font_glyph_cache = halo::link::ref<font_glyph_cache>(halo::rasterizer::vars().g_font_glyph_cache);
inline auto &rasterizer_frame_index = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_frame_index);
inline auto &render_viewport_top = halo::link::ref<int16_t [2]>(halo::ui::vars().render_viewport_top);
inline auto &render_viewport_bottom = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().render_viewport_bottom);
inline auto &screen_safe_area_right = halo::link::ref<int16_t [2]>(halo::game::vars().screen_safe_area_right);
inline auto &screen_safe_area_bottom = halo::link::ref<int16_t [2]>(halo::game::vars().screen_safe_area_bottom);
inline auto &rasterizer_gamma_disabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_gamma_disabled);
inline auto &rasterizer_gamma_captured = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_gamma_captured);
inline auto &rasterizer_gamma_exponent = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_gamma_exponent);
inline auto &rasterizer_game_gamma_ramp = halo::link::ref<d3d_gamma_ramp>(halo::rasterizer::vars().rasterizer_game_gamma_ramp);
inline auto &rasterizer_gamma_high_bit_17 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_gamma_high_bit_17);
inline auto &rasterizer_fullscreen = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fullscreen);
inline auto &rasterizer_device = halo::link::ref<void *>(halo::game::vars().rasterizer_device);
inline auto &rasterizer_window_handle = halo::link::ref<HWND>(halo::rasterizer::vars().rasterizer_window_handle);
inline auto &rasterizer_misc_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_misc_vertex_buffer);
inline auto &transparent_geometry_groups = halo::link::ref<transparent_geometry_group *>(halo::rasterizer::vars().transparent_geometry_groups);
inline auto &transparent_geometry_groups_secondary = halo::link::ref<transparent_geometry_group *>(halo::rasterizer::vars().transparent_geometry_groups_secondary);
inline auto &transparent_geometry_group_sorted_indices = halo::link::ref<int16_t *>(halo::rasterizer::vars().transparent_geometry_group_sorted_indices);
inline auto &transparent_geometry_group_count = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_count);
inline auto &transparent_geometry_group_secondary_count = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_secondary_count);
inline auto &rasterizer_software_vertex_processing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_software_vertex_processing);
inline auto &rasterizer_vertex_declarations = halo::link::ref<rasterizer_vertex_declaration [k_rasterizer_vertex_type_count]>(halo::networking::vars().rasterizer_vertex_declarations);
inline auto &rasterizer_vertex_sizes = halo::link::ref<int16_t [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_vertex_sizes);
inline auto &rasterizer_dynamic_index_slots = halo::link::ref<rasterizer_dynamic_index_slot [k_rasterizer_dynamic_vertex_slots]>(halo::rasterizer::vars().rasterizer_dynamic_index_slots);
inline auto &rasterizer_dynamic_index_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_dynamic_index_buffer);
inline auto &rasterizer_scratch_memory = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_scratch_memory);
inline auto &rasterizer_scratch_memory_used = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_scratch_memory_used);
inline auto &rasterizer_blend_src_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_src_table);
inline auto &rasterizer_blend_dest_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_dest_table);
inline auto &rasterizer_blend_op_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_op_table);
inline auto &rasterizer_maximum_skinning_nodes = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_maximum_skinning_nodes);
inline auto &rasterizer_skinning_palette = halo::link::ref<rasterizer_skinning_matrix [63]>(halo::rasterizer::vars().rasterizer_skinning_palette);
inline auto &rasterizer_globals_data = halo::link::ref<GlobalsRasterizerData *>(halo::game::vars().rasterizer_globals_data);
inline auto &rasterizer_bound_bitmap_size_a = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_a);
inline auto &rasterizer_node_part_indices = halo::link::ref<uint8_t *>(halo::rasterizer::vars().rasterizer_node_part_indices);
inline auto &rasterizer_node_part_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_node_part_count);
inline auto &rasterizer_desktop_gamma_ramp = halo::link::ref<d3d_gamma_ramp>(halo::rasterizer::vars().rasterizer_desktop_gamma_ramp);
inline auto &shell_argc = halo::link::ref<int32_t>(halo::shell::vars().shell_argc);
inline auto &shell_argv = halo::link::ref<char **>(halo::shell::vars().shell_argv);
inline auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
inline auto &rasterizer_ui_text_constants = halo::link::ref<float [20]>(halo::rasterizer::vars().rasterizer_ui_text_constants);
#ifndef HALO_LINKED_global_globals
#define HALO_LINKED_global_globals
inline auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
#endif
inline auto &lens_flare_object_visibility_table = halo::link::ref<lens_flare_object_visibility [k_lens_flare_object_visibility_slots]>(halo::rasterizer::vars().lens_flare_object_visibility_table);
inline auto &lens_flare_marker_visibility = halo::link::ref<uint8_t [0x10008]>(halo::rasterizer::vars().lens_flare_marker_visibility);
inline auto &lens_flare_instance_count = halo::link::ref<int32_t>(halo::rasterizer::vars().lens_flare_instance_count);
inline auto &rasterizer_model_ambient_reflection_tint = halo::link::ref<float *>(halo::cutscene::vars().rasterizer_model_ambient_reflection_tint);
inline auto &decal_data = halo::link::ref<data_array *>(halo::effects::vars().decal_data);
inline auto &decal_vertex_cache_last_queried = halo::link::ref<datum_index>(halo::rasterizer::vars().decal_vertex_cache_last_queried);
inline auto &rasterizer_desktop_display_mode = halo::link::ref<d3d_display_mode>(halo::ui::vars().rasterizer_desktop_display_mode);
inline auto &rasterizer_present_parameters = halo::link::ref<d3d_present_parameters>(halo::rasterizer::vars().rasterizer_present_parameters);
inline auto &os_platform = halo::link::ref<int32_t>(halo::ui::vars().os_platform);
inline auto &screenshot_scale = halo::link::ref<int16_t>(halo::main::vars().screenshot_scale);
inline auto &lens_flare_instances = halo::link::ref<lens_flare_instance [0x400]>(halo::rasterizer::vars().lens_flare_instances);
inline auto &lens_flare_instance_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().lens_flare_instance_overflow);
inline auto &lens_flare_occlusion_queries_supported = halo::link::ref<uint8_t>(halo::rasterizer::vars().lens_flare_occlusion_queries_supported);
inline auto &rasterizer_effect_pool_scratch = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_effect_pool_scratch);
inline auto &rasterizer_caps_flag_68a = halo::link::ref<uint8_t>(halo::ui::vars().rasterizer_caps_flag_68a);
inline auto &config_decal_z_bias = halo::link::ref<uint32_t>(halo::shell::vars().config_decal_z_bias);
inline auto &config_decal_slope_z_bias = halo::link::ref<uint32_t>(halo::shell::vars().config_decal_slope_z_bias);
inline auto &rasterizer_dynamic_vertex_caches = halo::link::ref<rasterizer_dynamic_vertex_cache [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_dynamic_vertex_caches);
inline auto &rasterizer_dynamic_vertex_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_vertex_slot_count);
inline auto &rasterizer_dynamic_index_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_index_count);
inline auto &rasterizer_dynamic_index_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_index_slot_count);
inline auto &transparent_geometry_group_drawn_bits = halo::link::ref<uint32_t [12]>(halo::rasterizer::vars().transparent_geometry_group_drawn_bits);
inline auto &rasterizer_light_count = halo::link::ref<int32_t>(halo::objects::vars().rasterizer_light_count);
inline auto &rasterizer_render_target_capture_done = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_target_capture_done);
inline auto &rasterizer_render_target_capture_requested = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_target_capture_requested);
inline auto &renderer_texture_quality = halo::link::ref<int8_t>(halo::ui::vars().renderer_texture_quality);
inline auto &rasterizer_bitmap_format_to_d3dformat = halo::link::ref<int32_t []>(halo::rasterizer::vars().rasterizer_bitmap_format_to_d3dformat);
inline auto &rasterizer_cube_face_to_d3d_face = halo::link::ref<int16_t [6]>(halo::rasterizer::vars().rasterizer_cube_face_to_d3d_face);
inline auto &config_disable_buffering = halo::link::ref<uint32_t>(halo::shell::vars().config_disable_buffering);
inline auto &screenshots = halo::link::ref<uint32_t>(halo::main::vars().screenshots);
#ifndef HALO_LINKED_shell_window
#define HALO_LINKED_shell_window
inline auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
#endif
inline auto &video_force_mode_flag = halo::link::ref<uint8_t>(halo::ui::vars().video_force_mode_flag);
inline auto &game_time_force_single_tick = halo::link::ref<uint32_t>(halo::game::vars().game_time_force_single_tick);
inline auto &rasterizer_device_lost = halo::link::ref<uint8_t>(halo::main::vars().rasterizer_device_lost);
inline auto &rasterizer_pending_clear = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_pending_clear);
inline auto &game_window_top_left = halo::link::ref<uint32_t>(halo::main::vars().game_window_top_left);
inline auto &game_window_bottom_right = halo::link::ref<uint32_t>(halo::networking::vars().game_window_bottom_right);
inline auto &rasterizer_present_counter_low = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_low);
inline auto &rasterizer_present_counter_high = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_high);
inline auto &shell_window_proc = halo::link::ref<void *>(halo::rasterizer::vars().shell_window_proc);
inline auto &rasterizer_window_style = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_window_style);
inline auto &shell_instance = halo::link::ref<void *>(halo::shell::vars().shell_instance);
inline auto &shell_window_class_name = halo::link::ref<char []>(halo::rasterizer::vars().shell_window_class_name);
inline auto &shell_window_title = halo::link::ref<char []>(halo::rasterizer::vars().shell_window_title);
inline auto &rasterizer_window_icon_bitmap = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_icon_bitmap);
inline auto &rasterizer_window_icon_dc = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_icon_dc);
inline auto &rasterizer_decal_layer = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_layer);
inline auto &rasterizer_decal_blend_mode = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_blend_mode);
inline auto &rasterizer_decal_bitmap_frame = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_bitmap_frame);
inline auto &rasterizer_decal_bitmap_tag = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_decal_bitmap_tag);
inline auto &console_debug_toggle_689441 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689441);
inline auto &rasterizer_decal_vertex_cache = halo::link::ref<void *>(halo::effects::vars().rasterizer_decal_vertex_cache);
inline auto &rasterizer_vertex_buffer_lock_state = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_lock_state);
inline auto &decal_grid_block = halo::link::ref<uint32_t *>(halo::effects::vars().decal_grid_block);
inline auto &rasterizer_vertex_shaders = halo::link::ref<rasterizer_vertex_shader [k_rasterizer_vertex_shaders]>(halo::networking::vars().rasterizer_vertex_shaders);
#ifndef HALO_LINKED_game_state_base
#define HALO_LINKED_game_state_base
inline auto &game_state_base = halo::link::ref<uint8_t *>(halo::saved_games::vars().game_state_base);
#endif
#ifndef HALO_LINKED_game_state_cursor
#define HALO_LINKED_game_state_cursor
inline auto &game_state_cursor = halo::link::ref<int32_t>(halo::saved_games::vars().game_state_cursor);
#endif
#ifndef HALO_LINKED_game_state_crc
#define HALO_LINKED_game_state_crc
inline auto &game_state_crc = halo::link::ref<uint32_t>(halo::saved_games::vars().game_state_crc);
#endif
inline auto &rasterizer_detail_object_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_detail_object_vertex_buffer);
#ifndef HALO_LINKED_console_debug_toggle_689404
#define HALO_LINKED_console_debug_toggle_689404
inline auto &console_debug_toggle_689404 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689404);
#endif
inline auto &console_debug_value_689430 = halo::link::ref<float>(halo::rasterizer::vars().console_debug_value_689430);
inline auto &global_scenario = halo::link::ref<Scenario *>(halo::hs::vars().global_scenario);
#ifndef HALO_LINKED_global_structure_bsp
#define HALO_LINKED_global_structure_bsp
inline auto &global_structure_bsp = halo::link::ref<ScenarioStructureBSP *>(halo::ai::vars().global_structure_bsp);
#endif
inline auto &rasterizer_vertex_buffer_slot_high_water = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_slot_high_water);
inline auto &rasterizer_vertex_buffer_slots = halo::link::ref<rasterizer_vertex_buffer_slot [k_rasterizer_vertex_buffer_slots]>(halo::rasterizer::vars().rasterizer_vertex_buffer_slots);
inline auto &lens_flare_occlusion_queries = halo::link::ref<void *[k_lens_flare_occlusion_queries]>(halo::rasterizer::vars().lens_flare_occlusion_queries);
inline auto &rasterizer_effect_pool = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_effect_pool);
inline auto &rasterizer_effect_defines = halo::link::ref<d3dx_macro [2]>(halo::rasterizer::vars().rasterizer_effect_defines);
inline auto &rasterizer_effects = halo::link::ref<rasterizer_effect_slot [k_rasterizer_pixel_shader_effects]>(halo::rasterizer::vars().rasterizer_effects);
inline auto &rasterizer_shader_file_name = halo::link::ref<const char *>(halo::rasterizer::vars().rasterizer_shader_file_name);
inline auto &config_force_shader = halo::link::ref<int32_t>(halo::shell::vars().config_force_shader);
inline auto &vertex_elements_environment_uncompressed = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_uncompressed);
inline auto &vertex_elements_environment_lightmap = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_lightmap);
inline auto &vertex_elements_model = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model);
inline auto &vertex_elements_dynamic = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_dynamic);
inline auto &vertex_elements_dynamic_screen = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_dynamic_screen);
inline auto &vertex_elements_debug = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_debug);
inline auto &vertex_elements_decal = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_decal);
inline auto &vertex_elements_detail_object = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_detail_object);
inline auto &vertex_elements_environment_uncompressed_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_uncompressed_ff);
inline auto &vertex_elements_environment_lightmap_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_lightmap_ff);
inline auto &vertex_elements_model_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model_ff);
inline auto &vertex_elements_model_processed = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model_processed);
inline auto &vertex_elements_unlit_zsprite = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_unlit_zsprite);
inline auto &vertex_elements_screen_transformed_lit = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_screen_transformed_lit);
inline auto &vertex_elements_screen_transformed_lit_specular = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_screen_transformed_lit_specular);
inline auto &vertex_elements_environment_single_stream_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_single_stream_ff);
inline auto &rasterizer_triangle_buffer_primitive_types = halo::link::ref<uint32_t [2]>(halo::rasterizer::vars().rasterizer_triangle_buffer_primitive_types);
inline auto &rasterizer_vertex_buffer_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_slot_count);
inline auto &rasterizer_dynamic_vertex_slots = halo::link::ref<rasterizer_dynamic_vertex_slot [k_rasterizer_dynamic_vertex_slots]>(halo::game::vars().rasterizer_dynamic_vertex_slots);
inline auto &rasterizer_dynamic_index_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_dynamic_index_overflow);
inline auto &render_force_flag = halo::link::ref<int16_t>(halo::rasterizer::vars().render_force_flag);
inline auto &rasterizer_dynamic_vertex_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_dynamic_vertex_overflow);
inline auto &chat_gui_root_handle = halo::link::ref<void *>(halo::ui::vars().chat_gui_root_handle);
inline auto &keystone_current_directory = halo::link::ref<uint32_t>(halo::rasterizer::vars().keystone_current_directory);
inline auto &chat_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_gui_find_object_arg);
inline auto &chat_listbox_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_listbox_gui_find_object_arg);
inline auto &rasterizer_render_targets = halo::link::ref<rasterizer_render_target [k_rasterizer_render_targets]>(halo::rasterizer::vars().rasterizer_render_targets);
inline auto &rasterizer_active_render_target = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_active_render_target);
inline auto &rasterizer_render_target_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_render_target_vertex_buffer);
inline auto &rasterizer_render_target_index_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_render_target_index_buffer);
inline auto &rasterizer_frame_started = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_frame_started);
inline auto &rasterizer_in_scene = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_in_scene);
inline auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
inline auto &rasterizer_ui_render_failed = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_ui_render_failed);
inline auto &console_debug_toggle_68941d = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941d);
inline auto &rasterizer_fog_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fog_enabled);
inline auto &video_gamma_current = halo::link::ref<int32_t>(halo::ui::vars().video_gamma_current);
inline auto &rasterizer_frustum_z_values = halo::link::ref<uint32_t [2]>(halo::rasterizer::vars().rasterizer_frustum_z_values);
inline auto &k_render_identity_matrix_ptr = halo::link::ref<real_matrix4x3 *>(halo::effects::vars().k_render_identity_matrix_ptr);
inline auto &rasterizer_depth_prepass_vertex_shader = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_depth_prepass_vertex_shader);
inline auto &rasterizer_capture_surfaces = halo::link::ref<void *[4]>(halo::rasterizer::vars().rasterizer_capture_surfaces);
inline auto &rasterizer_glass_draw_procedures = halo::link::ref<void *[3]>(halo::rasterizer::vars().rasterizer_glass_draw_procedures);
inline auto &rasterizer_bound_bitmap_size_b = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_b);
inline auto &rasterizer_direct3d = halo::link::ref<void *>(halo::ui::vars().rasterizer_direct3d);
inline auto &rasterizer_caps_flag_688 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_caps_flag_688);
inline auto &rasterizer_caps_flag_689 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_caps_flag_689);
inline auto &rasterizer_device_type = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_device_type);
inline auto &rasterizer_texture_stage_count = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_texture_stage_count);
inline auto &d3d_adapter = halo::link::ref<uint32_t>(halo::ui::vars().d3d_adapter);
inline auto &rasterizer_use_fx_file = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_use_fx_file);
inline auto &shell_direct3d = halo::link::ref<void *>(halo::rasterizer::vars().shell_direct3d);
inline auto &rasterizer_window_requested = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_window_requested);
inline auto &windowed = halo::link::ref<int32_t>(halo::rasterizer::vars().windowed);
inline auto &checkfpu = halo::link::ref<int32_t>(halo::main::vars().checkfpu);
inline auto &width640 = halo::link::ref<int32_t>(halo::rasterizer::vars().width640);
inline auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
inline auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
inline auto &graphics_device_id = halo::link::ref<uint32_t>(halo::shell::vars().graphics_device_id);
inline auto &graphics_vendor_id = halo::link::ref<uint32_t>(halo::rasterizer::vars().graphics_vendor_id);
inline auto &video_memory = halo::link::ref<uint32_t>(halo::ui::vars().video_memory);
inline auto &required_video_memory = halo::link::ref<uint32_t>(halo::rasterizer::vars().required_video_memory);
#ifndef HALO_LINKED_unknown_00721eac
#define HALO_LINKED_unknown_00721eac
inline auto &unknown_00721eac = halo::link::ref<void (*)(void *engine)>(halo::rasterizer::vars().unknown_00721eac);
#endif
inline auto &lens_flare_applied_key = halo::link::ref<lens_flare_batch_key>(halo::rasterizer::vars().lens_flare_applied_key);
inline auto &lens_flare_batches = halo::link::ref<lens_flare_batch [k_lens_flare_batch_slots]>(halo::rasterizer::vars().lens_flare_batches);
inline auto &lens_flare_current_key = halo::link::ref<lens_flare_batch_key>(halo::rasterizer::vars().lens_flare_current_key);
inline auto &lens_flare_batch_clock = halo::link::ref<uint32_t>(halo::rasterizer::vars().lens_flare_batch_clock);
inline auto &console_debug_toggle_689425 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689425);
inline auto &lens_flare_vertex_specular = halo::link::ref<uint32_t>(halo::rasterizer::vars().lens_flare_vertex_specular);
inline auto &console_debug_toggle_689424 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689424);
inline auto &transparent_geometry_group_overflow_d = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_d);
inline auto &rasterizer_lights = halo::link::ref<rasterizer_light [k_rasterizer_maximum_lights]>(halo::objects::vars().rasterizer_lights);
inline auto &rasterizer_default_material = halo::link::ref<uint8_t [0x44]>(halo::rasterizer::vars().rasterizer_default_material);
inline auto &rasterizer_fixed_function_light_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_fixed_function_light_count);
inline auto &rasterizer_active_model_context = halo::link::ref<rasterizer_model_draw_context *>(halo::rasterizer::vars().rasterizer_active_model_context);
inline auto &rasterizer_model_scratch_valid = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_model_scratch_valid);
inline auto &rasterizer_active_model_mode = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_active_model_mode);
inline auto &rasterizer_motion_sensor_ready = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_motion_sensor_ready);
#ifndef HALO_LINKED_local_player_globals
#define HALO_LINKED_local_player_globals
inline auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
#endif
inline auto &rasterizer_identity_vertex_constants = halo::link::ref<const float [5][4]>(halo::rasterizer::vars().rasterizer_identity_vertex_constants);
inline auto &console_debug_toggle_68941f = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941f);
inline auto &rasterizer_object_shadow_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_object_shadow_color);
inline auto &rasterizer_object_shadow_radius = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_object_shadow_radius);
inline auto &rasterizer_object_shadow_projection = halo::link::ref<real_matrix4x3>(halo::rasterizer::vars().rasterizer_object_shadow_projection);
inline auto &rasterizer_object_shadow_model_context = halo::link::ref<rasterizer_model_draw_context *>(halo::rasterizer::vars().rasterizer_object_shadow_model_context);
inline auto &rasterizer_object_shadow_prepared = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_prepared);
inline auto &rasterizer_object_shadow_model_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_model_active);
inline auto &rasterizer_object_shadow_window_restored = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_window_restored);
inline auto &rasterizer_object_shadow_blur_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_object_shadow_blur_quad);
inline auto &rasterizer_object_shadow_border_lines = halo::link::ref<rasterizer_screen_vertex [8]>(halo::rasterizer::vars().rasterizer_object_shadow_border_lines);
inline auto &rasterizer_needs_reset = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_needs_reset);
inline auto &zoom_static_tint_r = halo::link::ref<ColorRGB>(halo::ui::vars().zoom_static_tint_r);
inline auto &rasterizer_projected_light_shader_variant = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_projected_light_shader_variant);
inline auto &rasterizer_projected_light_luminance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_projected_light_luminance);
inline auto &rasterizer_projected_light = halo::link::ref<rasterizer_projected_light_constants>(halo::rasterizer::vars().rasterizer_projected_light);
inline auto &rasterizer_projected_light_has_cube_map = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_projected_light_has_cube_map);
inline auto &rasterizer_projected_light_cube_map = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_projected_light_cube_map);
inline auto &console_debug_toggle_689422 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689422);
inline auto &rasterizer_screen_quad_vertices = halo::link::ref<float [4][6]>(halo::rasterizer::vars().rasterizer_screen_quad_vertices);
inline auto &console_debug_toggle_6893f9 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f9);
inline auto &game_screen_rect = halo::link::ref<int16_t>(halo::main::vars().game_screen_rect);
inline auto &rasterizer_bound_bitmap_size_c = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_c);
inline auto &screen_effect_techniques = halo::link::ref<uint32_t [k_rasterizer_screen_effect_techniques]>(halo::rasterizer::vars().screen_effect_techniques);
inline auto &console_debug_toggle_689428 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689428);
inline auto &rasterizer_screen_effect_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_screen_effect_quad);
inline auto &screen_flash_techniques = halo::link::ref<void *[6]>(halo::rasterizer::vars().screen_flash_techniques);
inline auto &rasterizer_screen_flash_effect = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_screen_flash_effect);
inline auto &console_debug_toggle_689427 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689427);
inline auto &rasterizer_water_draw_procedure = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_water_draw_procedure);
inline auto &console_debug_toggle_6893fc = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893fc);
inline auto &global_white_color = halo::link::ref<const ColorRGB *>(halo::effects::vars().global_white_color);
inline auto &rasterizer_shader_stage_config = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_shader_stage_config);
inline auto &environment_techniques_multipurpose = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_multipurpose);
inline auto &environment_techniques_no = halo::link::ref<int32_t [12]>(halo::rasterizer::vars().environment_techniques_no);
inline auto &environment_techniques_self_illumination = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_self_illumination);
inline auto &environment_techniques_plain = halo::link::ref<int32_t [12]>(halo::rasterizer::vars().environment_techniques_plain);
inline auto &environment_techniques_reflection = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_reflection);
inline auto &environment_techniques_change_color = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_change_color);
inline auto &rasterizer_shader_technique_name_suffixes = halo::link::ref<const char [][0x80]>(halo::rasterizer::vars().rasterizer_shader_technique_name_suffixes);
inline auto &shader_environment_draw_simple = halo::link::ref<void *>(halo::rasterizer::vars().shader_environment_draw_simple);
inline auto &shader_environment_draw = halo::link::ref<void *>(halo::rasterizer::vars().shader_environment_draw);
inline auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
inline auto &rasterizer_camera_position = halo::link::ref<float [3]>(halo::rasterizer::vars().rasterizer_camera_position);
inline auto &rasterizer_camera_forward = halo::link::ref<float [3]>(halo::rasterizer::vars().rasterizer_camera_forward);
inline auto &rasterizer_fog_flags = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fog_flags);
inline auto &rasterizer_fog_atmospheric_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_fog_atmospheric_color);
inline auto &rasterizer_fog_atmospheric_max_density = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_max_density);
inline auto &rasterizer_fog_atmospheric_min_distance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_min_distance);
inline auto &rasterizer_fog_atmospheric_max_distance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_max_distance);
inline auto &rasterizer_fog_plane = halo::link::ref<float [4]>(halo::rasterizer::vars().rasterizer_fog_plane);
inline auto &rasterizer_fog_planar_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_fog_planar_color);
inline auto &environment_effect_slot = halo::link::ref<rasterizer_effect_slot>(halo::rasterizer::vars().environment_effect_slot);
inline auto &environment_techniques_ps14 = halo::link::ref<uint32_t []>(halo::rasterizer::vars().environment_techniques_ps14);
inline auto &rasterizer_model_vertex_declaration = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_vertex_declaration);
inline auto &rasterizer_lightmap_bitmap_missing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_lightmap_bitmap_missing);
inline auto &rasterizer_lightmap_bitmap = halo::link::ref<BitmapData *>(halo::rasterizer::vars().rasterizer_lightmap_bitmap);
inline auto &rasterizer_environment_lightmap = halo::link::ref<BitmapData *>(halo::rasterizer::vars().rasterizer_environment_lightmap);
inline auto &console_debug_toggle_6893f1 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f1);
inline auto &rasterizer_active_environment_effect = halo::link::ref<rasterizer_effect_slot *>(halo::rasterizer::vars().rasterizer_active_environment_effect);
inline auto &rasterizer_environment_lightmap_missing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_environment_lightmap_missing);
inline auto &rasterizer_camouflage_fade_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_camouflage_fade_active);
inline auto &rasterizer_camouflage_fade = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_camouflage_fade);
inline auto &rasterizer_model_effect_vector = halo::link::ref<float [4]>(halo::rasterizer::vars().rasterizer_model_effect_vector);
inline auto &rasterizer_transparent_vertex_shader_table = halo::link::ref<int16_t []>(halo::rasterizer::vars().rasterizer_transparent_vertex_shader_table);
inline auto &rasterizer_first_map_bitmap_types = halo::link::ref<const int16_t [4]>(halo::rasterizer::vars().rasterizer_first_map_bitmap_types);
inline auto &rasterizer_first_map_address_modes = halo::link::ref<const uint32_t [4]>(halo::rasterizer::vars().rasterizer_first_map_address_modes);
inline auto &rasterizer_transparent_extended_vertex_shader_table = halo::link::ref<int16_t []>(halo::rasterizer::vars().rasterizer_transparent_extended_vertex_shader_table);
inline auto &rasterizer_extended_first_map_bitmap_types = halo::link::ref<const int16_t [4]>(halo::rasterizer::vars().rasterizer_extended_first_map_bitmap_types);
inline auto &rasterizer_extended_first_map_address_modes = halo::link::ref<const uint32_t [4]>(halo::rasterizer::vars().rasterizer_extended_first_map_address_modes);
inline auto &rasterizer_chicago_color_function_stage_states = halo::link::ref<uint32_t [][3]>(halo::rasterizer::vars().rasterizer_chicago_color_function_stage_states);
inline auto &console_debug_toggle_689423 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689423);
inline auto &rasterizer_shadow_screen_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_shadow_screen_quad);
inline auto &rasterizer_sun_glow_blur_offsets = halo::link::ref<const float [8][4]>(halo::rasterizer::vars().rasterizer_sun_glow_blur_offsets);
inline auto &console_debug_toggle_6893ed = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893ed);
inline auto &transparent_geometry_group_environment_immediate = halo::link::ref<transparent_geometry_group>(halo::rasterizer::vars().transparent_geometry_group_environment_immediate);
inline auto &transparent_geometry_group_overflow_c = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_c);
inline auto &model_render_first_person = halo::link::ref<uint8_t>(halo::rasterizer::vars().model_render_first_person);
inline auto &transparent_geometry_group_last_drawn_key = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_last_drawn_key);
inline auto &rasterizer_secondary_groups_drawn = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_secondary_groups_drawn);
inline auto &rasterizer_render_states_dirty = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_states_dirty);
inline auto &rasterizer_model_scratch_node_matrices = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_node_matrices);
inline auto &rasterizer_model_scratch_node_count = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_model_scratch_node_count);
inline auto &rasterizer_model_scratch_lighting = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_lighting);
inline auto &rasterizer_model_scratch_function_source = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_function_source);
inline auto &console_debug_toggle_6893eb = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893eb);
inline auto &debug_print_enabled_flag = halo::link::ref<int16_t>(halo::game::vars().debug_print_enabled_flag);
inline auto &console_debug_meter_period = halo::link::ref<float>(halo::rasterizer::vars().console_debug_meter_period);
inline auto &console_debug_meter_values = halo::link::ref<float [4]>(halo::rasterizer::vars().console_debug_meter_values);
inline auto &rasterizer_widescreen_camouflage_scale = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_widescreen_camouflage_scale);
inline auto &console_debug_toggle_6893fb = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893fb);
inline auto &transparent_geometry_group_immediate = halo::link::ref<transparent_geometry_group>(halo::rasterizer::vars().transparent_geometry_group_immediate);
inline auto &transparent_geometry_group_overflow_b = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_b);
inline auto &transparent_geometry_group_overflow_a = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_a);
inline auto &rasterizer_underwater_tint_jitter_r = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_r);
inline auto &rasterizer_underwater_tint_jitter_g = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_g);
inline auto &rasterizer_underwater_tint_jitter_b = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_b);
inline auto &rasterizer_underwater_material = halo::link::ref<uint8_t [0x44]>(halo::rasterizer::vars().rasterizer_underwater_material);
inline auto &rasterizer_water_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_water_enabled);
inline auto &water_fade_plane_distance = halo::link::ref<float>(halo::rasterizer::vars().water_fade_plane_distance);
inline auto &water_fade_factor_a = halo::link::ref<float>(halo::rasterizer::vars().water_fade_factor_a);
inline auto &water_fade_factor_b = halo::link::ref<float>(halo::rasterizer::vars().water_fade_factor_b);
inline auto &rasterizer_water_ripple_quad = halo::link::ref<float [4][6]>(halo::rasterizer::vars().rasterizer_water_ripple_quad);
inline auto &render_window_index = halo::link::ref<int16_t>(halo::render::vars().render_window_index);
inline auto &text_shadow_color_argb = halo::link::ref<uint32_t>(halo::cutscene::vars().text_shadow_color_argb);
inline auto &font_glyph_cache_slots = halo::link::ref<uint8_t []>(halo::rasterizer::vars().font_glyph_cache_slots);
inline auto &transparent_geometry_group_draw_cursor = halo::link::ref<int16_t>(halo::rasterizer::vars().transparent_geometry_group_draw_cursor);

#include "halo/rasterizer/api.hpp"
