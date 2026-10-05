#pragma once

/**
 * @file src/rasterizer/internal/state.hpp
 * Shared declarations for the rasterizer implementation files: the engine globals and external functions
 * (C linkage) and the prototypes of the halo::rasterizer implementation functions.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "halo/rasterizer/layout_checks.hpp"
#include "halo/rasterizer/draw_procedures.hpp"
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
#include "halo/core/shared_links.hpp"
#include "halo/core/shared_state_links.hpp"


static auto &direct3d_create9_procedure = halo::link::ref<void *(__stdcall *)(uint32_t sdk_version)>(halo::rasterizer::vars().direct3d_create9_procedure);

static auto &bitmap_format_bits_per_pixel = halo::link::ref<int8_t []>(halo::rasterizer::vars().bitmap_format_bits_per_pixel);
static auto &cinematic_screen_effect_state = halo::link::ref<cinematic_screen_effect_globals *>(halo::cutscene::vars().cinematic_screen_effect_state);
static auto &rasterizer_default_z_near = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_default_z_near);
static auto &rasterizer_time = halo::link::ref<rasterizer_frame_time>(halo::rasterizer::vars().rasterizer_time);
static auto &rasterizer_caps = halo::link::ref<d3d_caps9>(halo::rasterizer::vars().rasterizer_caps);
static auto &decals_for_all_responses = halo::link::ref<uint8_t>(halo::effects::vars().decals_for_all_responses);
static auto &rasterizer_decal_vertex_cache_handle = halo::link::ref<::cache *>(halo::effects::vars().rasterizer_decal_vertex_cache_handle);
static auto &text_rendering_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().text_rendering_enabled);
static auto &rasterizer_window = halo::link::ref<rasterizer_window_parameters>(halo::rasterizer::vars().rasterizer_window);
static auto &g_font_glyph_cache = halo::link::ref<font_glyph_cache>(halo::rasterizer::vars().g_font_glyph_cache);
static auto &rasterizer_frame_index = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_frame_index);
static auto &render_viewport_top = halo::link::ref<int16_t [2]>(halo::ui::vars().render_viewport_top);
static auto &render_viewport_bottom = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().render_viewport_bottom);
static auto &screen_safe_area_right = halo::link::ref<int16_t [2]>(halo::game::vars().screen_safe_area_right);
static auto &screen_safe_area_bottom = halo::link::ref<int16_t [2]>(halo::game::vars().screen_safe_area_bottom);
static auto &rasterizer_gamma_disabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_gamma_disabled);
static auto &rasterizer_gamma_captured = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_gamma_captured);
static auto &rasterizer_gamma_exponent = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_gamma_exponent);
static auto &rasterizer_game_gamma_ramp = halo::link::ref<d3d_gamma_ramp>(halo::rasterizer::vars().rasterizer_game_gamma_ramp);
static auto &rasterizer_gamma_high_bit_17 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_gamma_high_bit_17);
static auto &rasterizer_fullscreen = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fullscreen);
static auto &rasterizer_device = halo::link::ref<void *>(halo::game::vars().rasterizer_device);
static auto &rasterizer_window_handle = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_handle);
static auto &rasterizer_misc_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_misc_vertex_buffer);
static auto &transparent_geometry_groups = halo::link::ref<transparent_geometry_group *>(halo::rasterizer::vars().transparent_geometry_groups);
static auto &transparent_geometry_groups_secondary = halo::link::ref<transparent_geometry_group *>(halo::rasterizer::vars().transparent_geometry_groups_secondary);
static auto &transparent_geometry_group_sorted_indices = halo::link::ref<int16_t *>(halo::rasterizer::vars().transparent_geometry_group_sorted_indices);
static auto &transparent_geometry_group_count = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_count);
static auto &transparent_geometry_group_secondary_count = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_secondary_count);
static auto &rasterizer_software_vertex_processing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_software_vertex_processing);
static auto &rasterizer_vertex_declarations = halo::link::ref<rasterizer_vertex_declaration [k_rasterizer_vertex_type_count]>(halo::networking::vars().rasterizer_vertex_declarations);
static auto &rasterizer_vertex_sizes = halo::link::ref<int16_t [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_vertex_sizes);
static auto &rasterizer_dynamic_index_slots = halo::link::ref<rasterizer_dynamic_index_slot [k_rasterizer_dynamic_vertex_slots]>(halo::rasterizer::vars().rasterizer_dynamic_index_slots);
static auto &rasterizer_dynamic_index_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_dynamic_index_buffer);
static auto &rasterizer_scratch_memory = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_scratch_memory);
static auto &rasterizer_scratch_memory_used = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_scratch_memory_used);
static auto &rasterizer_blend_src_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_src_table);
static auto &rasterizer_blend_dest_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_dest_table);
static auto &rasterizer_blend_op_table = halo::link::ref<uint32_t [16]>(halo::rasterizer::vars().rasterizer_blend_op_table);
static auto &rasterizer_maximum_skinning_nodes = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_maximum_skinning_nodes);
static auto &rasterizer_skinning_palette = halo::link::ref<rasterizer_skinning_matrix [63]>(halo::rasterizer::vars().rasterizer_skinning_palette);
static auto &rasterizer_globals_data = halo::link::ref<GlobalsRasterizerData *>(halo::game::vars().rasterizer_globals_data);
static auto &rasterizer_bound_bitmap_size_a = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_a);
static auto &rasterizer_node_part_indices = halo::link::ref<uint8_t *>(halo::rasterizer::vars().rasterizer_node_part_indices);
static auto &rasterizer_node_part_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_node_part_count);
static auto &rasterizer_desktop_gamma_ramp = halo::link::ref<d3d_gamma_ramp>(halo::rasterizer::vars().rasterizer_desktop_gamma_ramp);
static auto &shell_argc = halo::link::ref<int32_t>(halo::shell::vars().shell_argc);
static auto &shell_argv = halo::link::ref<char **>(halo::shell::vars().shell_argv);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &rasterizer_ui_text_constants = halo::link::ref<float [20]>(halo::rasterizer::vars().rasterizer_ui_text_constants);
static auto &lens_flare_object_visibility_table = halo::link::ref<lens_flare_object_visibility [k_lens_flare_object_visibility_slots]>(halo::rasterizer::vars().lens_flare_object_visibility_table);
static auto &lens_flare_marker_visibility = halo::link::ref<uint8_t [0x10008]>(halo::rasterizer::vars().lens_flare_marker_visibility);
static auto &lens_flare_instance_count = halo::link::ref<int32_t>(halo::rasterizer::vars().lens_flare_instance_count);
static auto &rasterizer_model_ambient_reflection_tint = halo::link::ref<float *>(halo::cutscene::vars().rasterizer_model_ambient_reflection_tint);
static auto &decal_data = halo::link::ref<data_array *>(halo::effects::vars().decal_data);
static auto &decal_vertex_cache_last_queried = halo::link::ref<datum_index>(halo::rasterizer::vars().decal_vertex_cache_last_queried);
static auto &rasterizer_desktop_display_mode = halo::link::ref<d3d_display_mode>(halo::ui::vars().rasterizer_desktop_display_mode);
static auto &rasterizer_present_parameters = halo::link::ref<d3d_present_parameters>(halo::rasterizer::vars().rasterizer_present_parameters);
static auto &os_platform = halo::link::ref<int32_t>(halo::ui::vars().os_platform);
static auto &screenshot_scale = halo::link::ref<int16_t>(halo::main::vars().screenshot_scale);
static auto &lens_flare_instances = halo::link::ref<lens_flare_instance [0x400]>(halo::rasterizer::vars().lens_flare_instances);
static auto &lens_flare_instance_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().lens_flare_instance_overflow);
static auto &lens_flare_occlusion_queries_supported = halo::link::ref<uint8_t>(halo::rasterizer::vars().lens_flare_occlusion_queries_supported);
static auto &rasterizer_effect_pool_scratch = halo::link::ref<void **>(halo::rasterizer::vars().rasterizer_effect_pool_scratch);
static auto &rasterizer_caps_flag_68a = halo::link::ref<uint8_t>(halo::ui::vars().rasterizer_caps_flag_68a);
static auto &config_decal_z_bias = halo::link::ref<uint32_t>(halo::shell::vars().config_decal_z_bias);
static auto &config_decal_slope_z_bias = halo::link::ref<uint32_t>(halo::shell::vars().config_decal_slope_z_bias);
static auto &rasterizer_dynamic_vertex_caches = halo::link::ref<rasterizer_dynamic_vertex_cache [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_dynamic_vertex_caches);
static auto &rasterizer_dynamic_vertex_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_vertex_slot_count);
static auto &rasterizer_dynamic_index_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_index_count);
static auto &rasterizer_dynamic_index_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_dynamic_index_slot_count);
static auto &transparent_geometry_group_drawn_bits = halo::link::ref<uint32_t [12]>(halo::rasterizer::vars().transparent_geometry_group_drawn_bits);
static auto &rasterizer_light_count = halo::link::ref<int32_t>(halo::objects::vars().rasterizer_light_count);
static auto &rasterizer_render_target_capture_done = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_target_capture_done);
static auto &rasterizer_render_target_capture_requested = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_target_capture_requested);
static auto &renderer_texture_quality = halo::link::ref<int8_t>(halo::ui::vars().renderer_texture_quality);
static auto &rasterizer_bitmap_format_to_d3dformat = halo::link::ref<int32_t []>(halo::rasterizer::vars().rasterizer_bitmap_format_to_d3dformat);
static auto &rasterizer_cube_face_to_d3d_face = halo::link::ref<int16_t [6]>(halo::rasterizer::vars().rasterizer_cube_face_to_d3d_face);
static auto &config_disable_buffering = halo::link::ref<uint32_t>(halo::shell::vars().config_disable_buffering);
static auto &screenshots = halo::link::ref<uint32_t>(halo::main::vars().screenshots);
static auto &video_force_mode_flag = halo::link::ref<uint8_t>(halo::ui::vars().video_force_mode_flag);
static auto &game_time_force_single_tick = halo::link::ref<uint32_t>(halo::game::vars().game_time_force_single_tick);
static auto &rasterizer_device_lost = halo::link::ref<uint8_t>(halo::main::vars().rasterizer_device_lost);
static auto &rasterizer_pending_clear = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_pending_clear);
static auto &game_window_top_left = halo::link::ref<uint32_t>(halo::main::vars().game_window_top_left);
static auto &game_window_bottom_right = halo::link::ref<uint32_t>(halo::networking::vars().game_window_bottom_right);
static auto &rasterizer_present_counter_low = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_low);
static auto &rasterizer_present_counter_high = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_high);
static auto &shell_window_proc = halo::link::ref<void *>(halo::rasterizer::vars().shell_window_proc);
static auto &rasterizer_window_style = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_window_style);
static auto &shell_instance = halo::link::ref<void *>(halo::shell::vars().shell_instance);
static auto &shell_window_class_name = halo::link::ref<char []>(halo::rasterizer::vars().shell_window_class_name);
static auto &shell_window_title = halo::link::ref<char []>(halo::rasterizer::vars().shell_window_title);
static auto &rasterizer_window_icon_bitmap = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_icon_bitmap);
static auto &rasterizer_window_icon_dc = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_icon_dc);
static auto &rasterizer_decal_layer = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_layer);
static auto &rasterizer_decal_blend_mode = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_blend_mode);
static auto &rasterizer_decal_bitmap_frame = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_bitmap_frame);
static auto &rasterizer_decal_bitmap_tag = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_decal_bitmap_tag);
static auto &console_debug_toggle_689441 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689441);
static auto &rasterizer_decal_vertex_cache = halo::link::ref<void *>(halo::effects::vars().rasterizer_decal_vertex_cache);
static auto &rasterizer_vertex_buffer_lock_state = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_lock_state);
static auto &decal_grid_block = halo::link::ref<uint32_t *>(halo::effects::vars().decal_grid_block);
static auto &rasterizer_vertex_shaders = halo::link::ref<rasterizer_vertex_shader [k_rasterizer_vertex_shaders]>(halo::networking::vars().rasterizer_vertex_shaders);
static auto &rasterizer_detail_object_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_detail_object_vertex_buffer);
static auto &console_debug_value_689430 = halo::link::ref<float>(halo::rasterizer::vars().console_debug_value_689430);
static auto &global_scenario = halo::link::ref<Scenario *>(halo::hs::vars().global_scenario);
static auto &rasterizer_vertex_buffer_slot_high_water = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_slot_high_water);
static auto &rasterizer_vertex_buffer_slots = halo::link::ref<rasterizer_vertex_buffer_slot [k_rasterizer_vertex_buffer_slots]>(halo::rasterizer::vars().rasterizer_vertex_buffer_slots);
static auto &lens_flare_occlusion_queries = halo::link::ref<void *[k_lens_flare_occlusion_queries]>(halo::rasterizer::vars().lens_flare_occlusion_queries);
static auto &rasterizer_effect_pool = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_effect_pool);
static auto &rasterizer_effect_defines = halo::link::ref<d3dx_macro [2]>(halo::rasterizer::vars().rasterizer_effect_defines);
static auto &rasterizer_effects = halo::link::ref<rasterizer_effect_slot [k_rasterizer_pixel_shader_effects]>(halo::rasterizer::vars().rasterizer_effects);
static auto &rasterizer_shader_file_name = halo::link::ref<const char *>(halo::rasterizer::vars().rasterizer_shader_file_name);
static auto &config_force_shader = halo::link::ref<int32_t>(halo::shell::vars().config_force_shader);
static auto &vertex_elements_environment_uncompressed = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_uncompressed);
static auto &vertex_elements_environment_lightmap = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_lightmap);
static auto &vertex_elements_model = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model);
static auto &vertex_elements_dynamic = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_dynamic);
static auto &vertex_elements_dynamic_screen = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_dynamic_screen);
static auto &vertex_elements_debug = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_debug);
static auto &vertex_elements_decal = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_decal);
static auto &vertex_elements_detail_object = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_detail_object);
static auto &vertex_elements_environment_uncompressed_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_uncompressed_ff);
static auto &vertex_elements_environment_lightmap_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_lightmap_ff);
static auto &vertex_elements_model_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model_ff);
static auto &vertex_elements_model_processed = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_model_processed);
static auto &vertex_elements_unlit_zsprite = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_unlit_zsprite);
static auto &vertex_elements_screen_transformed_lit = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_screen_transformed_lit);
static auto &vertex_elements_screen_transformed_lit_specular = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_screen_transformed_lit_specular);
static auto &vertex_elements_environment_single_stream_ff = halo::link::ref<const d3d_vertex_element9 []>(halo::rasterizer::vars().vertex_elements_environment_single_stream_ff);
static auto &rasterizer_triangle_buffer_primitive_types = halo::link::ref<uint32_t [2]>(halo::rasterizer::vars().rasterizer_triangle_buffer_primitive_types);
static auto &rasterizer_vertex_buffer_slot_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_slot_count);
static auto &rasterizer_dynamic_vertex_slots = halo::link::ref<rasterizer_dynamic_vertex_slot [k_rasterizer_dynamic_vertex_slots]>(halo::game::vars().rasterizer_dynamic_vertex_slots);
static auto &rasterizer_dynamic_index_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_dynamic_index_overflow);
static auto &render_force_flag = halo::link::ref<int16_t>(halo::rasterizer::vars().render_force_flag);
static auto &rasterizer_dynamic_vertex_overflow = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_dynamic_vertex_overflow);
static auto &chat_gui_root_handle = halo::link::ref<void *>(halo::ui::vars().chat_gui_root_handle);
static auto &keystone_current_directory = halo::link::ref<uint32_t>(halo::rasterizer::vars().keystone_current_directory);
static auto &chat_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_gui_find_object_arg);
static auto &chat_listbox_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_listbox_gui_find_object_arg);
static auto &rasterizer_render_targets = halo::link::ref<rasterizer_render_target [k_rasterizer_render_targets]>(halo::rasterizer::vars().rasterizer_render_targets);
static auto &rasterizer_active_render_target = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_active_render_target);
static auto &rasterizer_render_target_vertex_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_render_target_vertex_buffer);
static auto &rasterizer_render_target_index_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_render_target_index_buffer);
static auto &rasterizer_frame_started = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_frame_started);
static auto &rasterizer_in_scene = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_in_scene);
static auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
static auto &rasterizer_ui_render_failed = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_ui_render_failed);
static auto &console_debug_toggle_68941d = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941d);
static auto &rasterizer_fog_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fog_enabled);
static auto &video_gamma_current = halo::link::ref<int32_t>(halo::ui::vars().video_gamma_current);
static auto &rasterizer_frustum_z_values = halo::link::ref<float [2]>(halo::rasterizer::vars().rasterizer_frustum_z_values);
static auto &k_render_identity_matrix_ptr = halo::link::ref<real_matrix4x3 *>(halo::effects::vars().k_render_identity_matrix_ptr);
static auto &rasterizer_depth_prepass_vertex_shader = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_depth_prepass_vertex_shader);
static auto &rasterizer_capture_surfaces = halo::link::ref<void *[4]>(halo::rasterizer::vars().rasterizer_capture_surfaces);
static auto &rasterizer_glass_draw_procedures = halo::link::ref<halo::rasterizer::glass_draw_procedures>(halo::rasterizer::vars().rasterizer_glass_draw_procedures);
static auto &rasterizer_bound_bitmap_size_b = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_b);
static auto &rasterizer_direct3d = halo::link::ref<void *>(halo::ui::vars().rasterizer_direct3d);
static auto &rasterizer_caps_flag_688 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_caps_flag_688);
static auto &rasterizer_caps_flag_689 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_caps_flag_689);
static auto &rasterizer_device_type = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_device_type);
static auto &rasterizer_texture_stage_count = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_texture_stage_count);
static auto &d3d_adapter = halo::link::ref<uint32_t>(halo::ui::vars().d3d_adapter);
static auto &rasterizer_use_fx_file = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_use_fx_file);
static auto &shell_direct3d = halo::link::ref<void *>(halo::rasterizer::vars().shell_direct3d);
static auto &rasterizer_window_requested = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_window_requested);
static auto &windowed = halo::link::ref<int32_t>(halo::rasterizer::vars().windowed);
static auto &checkfpu = halo::link::ref<int32_t>(halo::main::vars().checkfpu);
static auto &width640 = halo::link::ref<int32_t>(halo::rasterizer::vars().width640);
static auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
static auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
static auto &graphics_device_id = halo::link::ref<uint32_t>(halo::shell::vars().graphics_device_id);
static auto &graphics_vendor_id = halo::link::ref<uint32_t>(halo::rasterizer::vars().graphics_vendor_id);
static auto &video_memory = halo::link::ref<uint32_t>(halo::ui::vars().video_memory);
static auto &required_video_memory = halo::link::ref<uint32_t>(halo::rasterizer::vars().required_video_memory);
static auto &lens_flare_applied_key = halo::link::ref<lens_flare_batch_key>(halo::rasterizer::vars().lens_flare_applied_key);
static auto &lens_flare_batches = halo::link::ref<lens_flare_batch [k_lens_flare_batch_slots]>(halo::rasterizer::vars().lens_flare_batches);
static auto &lens_flare_current_key = halo::link::ref<lens_flare_batch_key>(halo::rasterizer::vars().lens_flare_current_key);
static auto &lens_flare_batch_clock = halo::link::ref<uint32_t>(halo::rasterizer::vars().lens_flare_batch_clock);
static auto &console_debug_toggle_689425 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689425);
static auto &lens_flare_vertex_specular = halo::link::ref<uint32_t>(halo::rasterizer::vars().lens_flare_vertex_specular);
static auto &console_debug_toggle_689424 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689424);
static auto &transparent_geometry_group_overflow_d = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_d);
static auto &rasterizer_lights = halo::link::ref<rasterizer_light [k_rasterizer_maximum_lights]>(halo::objects::vars().rasterizer_lights);
static auto &rasterizer_default_material = halo::link::ref<uint8_t [0x44]>(halo::rasterizer::vars().rasterizer_default_material);
static auto &rasterizer_fixed_function_light_count = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_fixed_function_light_count);
static auto &rasterizer_active_model_context = halo::link::ref<rasterizer_model_draw_context *>(halo::rasterizer::vars().rasterizer_active_model_context);
static auto &rasterizer_model_scratch_valid = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_model_scratch_valid);
static auto &rasterizer_active_model_mode = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_active_model_mode);
static auto &rasterizer_motion_sensor_ready = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_motion_sensor_ready);
static auto &rasterizer_identity_vertex_constants = halo::link::ref<const float [5][4]>(halo::rasterizer::vars().rasterizer_identity_vertex_constants);
static auto &console_debug_toggle_68941f = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941f);
static auto &rasterizer_object_shadow_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_object_shadow_color);
static auto &rasterizer_object_shadow_radius = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_object_shadow_radius);
static auto &rasterizer_object_shadow_projection = halo::link::ref<real_matrix4x3>(halo::rasterizer::vars().rasterizer_object_shadow_projection);
static auto &rasterizer_object_shadow_model_context = halo::link::ref<rasterizer_model_draw_context *>(halo::rasterizer::vars().rasterizer_object_shadow_model_context);
static auto &rasterizer_object_shadow_prepared = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_prepared);
static auto &rasterizer_object_shadow_model_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_model_active);
static auto &rasterizer_object_shadow_window_restored = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_window_restored);
static auto &rasterizer_object_shadow_blur_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_object_shadow_blur_quad);
static auto &rasterizer_object_shadow_border_lines = halo::link::ref<rasterizer_screen_vertex [8]>(halo::rasterizer::vars().rasterizer_object_shadow_border_lines);
static auto &rasterizer_needs_reset = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_needs_reset);
static auto &zoom_static_tint_r = halo::link::ref<ColorRGB>(halo::ui::vars().zoom_static_tint_r);
static auto &rasterizer_projected_light_shader_variant = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_projected_light_shader_variant);
static auto &rasterizer_projected_light_luminance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_projected_light_luminance);
static auto &rasterizer_projected_light = halo::link::ref<rasterizer_projected_light_constants>(halo::rasterizer::vars().rasterizer_projected_light);
static auto &rasterizer_projected_light_has_cube_map = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_projected_light_has_cube_map);
static auto &rasterizer_projected_light_cube_map = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_projected_light_cube_map);
static auto &console_debug_toggle_689422 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689422);
static auto &rasterizer_screen_quad_vertices = halo::link::ref<float [4][6]>(halo::rasterizer::vars().rasterizer_screen_quad_vertices);
static auto &console_debug_toggle_6893f9 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f9);
static auto &game_screen_rect = halo::link::ref<int16_t>(halo::main::vars().game_screen_rect);
static auto &rasterizer_bound_bitmap_size_c = halo::link::ref<int16_t [2]>(halo::rasterizer::vars().rasterizer_bound_bitmap_size_c);
static auto &screen_effect_techniques = halo::link::ref<uint32_t [k_rasterizer_screen_effect_techniques]>(halo::rasterizer::vars().screen_effect_techniques);
static auto &console_debug_toggle_689428 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689428);
static auto &rasterizer_screen_effect_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_screen_effect_quad);
static auto &screen_flash_techniques = halo::link::ref<void *[6]>(halo::rasterizer::vars().screen_flash_techniques);
static auto &rasterizer_screen_flash_effect = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_screen_flash_effect);
static auto &console_debug_toggle_689427 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689427);
static auto &rasterizer_water_draw_procedure = halo::link::ref<halo::rasterizer::group_draw_procedure>(halo::rasterizer::vars().rasterizer_water_draw_procedure);
static auto &console_debug_toggle_6893fc = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893fc);
static auto &global_white_color = halo::link::ref<const ColorRGB *>(halo::effects::vars().global_white_color);
static auto &rasterizer_shader_stage_config = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_shader_stage_config);
static auto &environment_techniques_multipurpose = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_multipurpose);
static auto &environment_techniques_no = halo::link::ref<int32_t [12]>(halo::rasterizer::vars().environment_techniques_no);
static auto &environment_techniques_self_illumination = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_self_illumination);
static auto &environment_techniques_plain = halo::link::ref<int32_t [12]>(halo::rasterizer::vars().environment_techniques_plain);
static auto &environment_techniques_reflection = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_reflection);
static auto &environment_techniques_change_color = halo::link::ref<int32_t [24]>(halo::rasterizer::vars().environment_techniques_change_color);
static auto &rasterizer_shader_technique_name_suffixes = halo::link::ref<const char [][0x80]>(halo::rasterizer::vars().rasterizer_shader_technique_name_suffixes);
static auto &shader_environment_draw_simple = halo::link::ref<halo::rasterizer::part_draw_procedure>(halo::rasterizer::vars().shader_environment_draw_simple);
static auto &shader_environment_draw = halo::link::ref<halo::rasterizer::part_draw_procedure>(halo::rasterizer::vars().shader_environment_draw);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &rasterizer_camera_position = halo::link::ref<float [3]>(halo::rasterizer::vars().rasterizer_camera_position);
static auto &rasterizer_camera_forward = halo::link::ref<float [3]>(halo::rasterizer::vars().rasterizer_camera_forward);
static auto &rasterizer_fog_flags = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fog_flags);
static auto &rasterizer_fog_atmospheric_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_fog_atmospheric_color);
static auto &rasterizer_fog_atmospheric_max_density = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_max_density);
static auto &rasterizer_fog_atmospheric_min_distance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_min_distance);
static auto &rasterizer_fog_atmospheric_max_distance = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_fog_atmospheric_max_distance);
static auto &rasterizer_fog_plane = halo::link::ref<float [4]>(halo::rasterizer::vars().rasterizer_fog_plane);
static auto &rasterizer_fog_planar_color = halo::link::ref<ColorRGB>(halo::rasterizer::vars().rasterizer_fog_planar_color);
static auto &environment_effect_slot = halo::link::ref<rasterizer_effect_slot>(halo::rasterizer::vars().environment_effect_slot);
static auto &environment_techniques_ps14 = halo::link::ref<uint32_t []>(halo::rasterizer::vars().environment_techniques_ps14);
static auto &rasterizer_model_vertex_declaration = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_vertex_declaration);
static auto &rasterizer_lightmap_bitmap_missing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_lightmap_bitmap_missing);
static auto &rasterizer_lightmap_bitmap = halo::link::ref<BitmapData *>(halo::rasterizer::vars().rasterizer_lightmap_bitmap);
static auto &rasterizer_environment_lightmap = halo::link::ref<BitmapData *>(halo::rasterizer::vars().rasterizer_environment_lightmap);
static auto &console_debug_toggle_6893f1 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f1);
static auto &rasterizer_active_environment_effect = halo::link::ref<rasterizer_effect_slot *>(halo::rasterizer::vars().rasterizer_active_environment_effect);
static auto &rasterizer_environment_lightmap_missing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_environment_lightmap_missing);
static auto &rasterizer_camouflage_fade_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_camouflage_fade_active);
static auto &rasterizer_camouflage_fade = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_camouflage_fade);
static auto &rasterizer_model_effect_vector = halo::link::ref<float [4]>(halo::rasterizer::vars().rasterizer_model_effect_vector);
static auto &rasterizer_transparent_vertex_shader_table = halo::link::ref<int16_t []>(halo::rasterizer::vars().rasterizer_transparent_vertex_shader_table);
static auto &rasterizer_first_map_bitmap_types = halo::link::ref<const int16_t [4]>(halo::rasterizer::vars().rasterizer_first_map_bitmap_types);
static auto &rasterizer_first_map_address_modes = halo::link::ref<const uint32_t [4]>(halo::rasterizer::vars().rasterizer_first_map_address_modes);
static auto &rasterizer_transparent_extended_vertex_shader_table = halo::link::ref<int16_t []>(halo::rasterizer::vars().rasterizer_transparent_extended_vertex_shader_table);
static auto &rasterizer_extended_first_map_bitmap_types = halo::link::ref<const int16_t [4]>(halo::rasterizer::vars().rasterizer_extended_first_map_bitmap_types);
static auto &rasterizer_extended_first_map_address_modes = halo::link::ref<const uint32_t [4]>(halo::rasterizer::vars().rasterizer_extended_first_map_address_modes);
static auto &rasterizer_chicago_color_function_stage_states = halo::link::ref<uint32_t [][3]>(halo::rasterizer::vars().rasterizer_chicago_color_function_stage_states);
static auto &console_debug_toggle_689423 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689423);
static auto &rasterizer_shadow_screen_quad = halo::link::ref<rasterizer_dynamic_screen_vertex [4]>(halo::rasterizer::vars().rasterizer_shadow_screen_quad);
static auto &rasterizer_sun_glow_blur_offsets = halo::link::ref<const float [8][4]>(halo::rasterizer::vars().rasterizer_sun_glow_blur_offsets);
static auto &console_debug_toggle_6893ed = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893ed);
static auto &transparent_geometry_group_environment_immediate = halo::link::ref<transparent_geometry_group>(halo::rasterizer::vars().transparent_geometry_group_environment_immediate);
static auto &transparent_geometry_group_overflow_c = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_c);
static auto &model_render_first_person = halo::link::ref<uint8_t>(halo::rasterizer::vars().model_render_first_person);
static auto &transparent_geometry_group_last_drawn_key = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_last_drawn_key);
static auto &rasterizer_secondary_groups_drawn = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_secondary_groups_drawn);
static auto &rasterizer_render_states_dirty = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_states_dirty);
static auto &rasterizer_model_scratch_node_matrices = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_node_matrices);
static auto &rasterizer_model_scratch_node_count = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_model_scratch_node_count);
static auto &rasterizer_model_scratch_lighting = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_lighting);
static auto &rasterizer_model_scratch_function_source = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_model_scratch_function_source);
static auto &console_debug_toggle_6893eb = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893eb);
static auto &debug_print_enabled_flag = halo::link::ref<int16_t>(halo::game::vars().debug_print_enabled_flag);
static auto &console_debug_meter_period = halo::link::ref<float>(halo::rasterizer::vars().console_debug_meter_period);
static auto &console_debug_meter_values = halo::link::ref<float [4]>(halo::rasterizer::vars().console_debug_meter_values);
static auto &rasterizer_widescreen_camouflage_scale = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_widescreen_camouflage_scale);
static auto &console_debug_toggle_6893fb = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893fb);
static auto &transparent_geometry_group_immediate = halo::link::ref<transparent_geometry_group>(halo::rasterizer::vars().transparent_geometry_group_immediate);
static auto &transparent_geometry_group_overflow_b = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_b);
static auto &transparent_geometry_group_overflow_a = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_geometry_group_overflow_a);
static auto &rasterizer_underwater_tint_jitter_r = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_r);
static auto &rasterizer_underwater_tint_jitter_g = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_g);
static auto &rasterizer_underwater_tint_jitter_b = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_underwater_tint_jitter_b);
static auto &rasterizer_underwater_material = halo::link::ref<uint8_t [0x44]>(halo::rasterizer::vars().rasterizer_underwater_material);
static auto &rasterizer_water_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_water_enabled);
static auto &water_fade_plane_distance = halo::link::ref<float>(halo::rasterizer::vars().water_fade_plane_distance);
static auto &water_fade_factor_a = halo::link::ref<float>(halo::rasterizer::vars().water_fade_factor_a);
static auto &water_fade_factor_b = halo::link::ref<float>(halo::rasterizer::vars().water_fade_factor_b);
static auto &rasterizer_water_ripple_quad = halo::link::ref<float [4][6]>(halo::rasterizer::vars().rasterizer_water_ripple_quad);
static auto &render_window_index = halo::link::ref<int16_t>(halo::render::vars().render_window_index);
static auto &text_shadow_color_argb = halo::link::ref<uint32_t>(halo::cutscene::vars().text_shadow_color_argb);
static auto &font_glyph_cache_slots = halo::link::ref<uint8_t []>(halo::rasterizer::vars().font_glyph_cache_slots);
static auto &transparent_geometry_group_draw_cursor = halo::link::ref<int16_t>(halo::rasterizer::vars().transparent_geometry_group_draw_cursor);

#include "halo/rasterizer/api.hpp"
