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

extern "C" {

extern int8_t bitmap_format_bits_per_pixel[];
extern cinematic_screen_effect_globals *cinematic_screen_effect_state;
extern float rasterizer_default_z_near;
extern rasterizer_frame_time rasterizer_time;
extern d3d_caps9 rasterizer_caps;
extern uint8_t unknown_0071d275;
extern uint8_t unknown_0071d276;
extern uint8_t *texture_cache;
extern uint8_t decals_for_all_responses;
extern uint8_t *rasterizer_decal_vertex_cache_handle;
extern uint8_t text_rendering_enabled;
extern rasterizer_window_parameters rasterizer_window;
extern font_glyph_cache g_font_glyph_cache;
extern int32_t rasterizer_frame_index;
extern int16_t render_viewport_top[2];
extern int16_t render_viewport_bottom[2];
extern int16_t screen_safe_area_right[2];
extern int16_t screen_safe_area_bottom[2];
extern uint8_t rasterizer_gamma_disabled;
extern int32_t rasterizer_gamma_captured;
extern int32_t rasterizer_gamma_exponent;
extern d3d_gamma_ramp rasterizer_game_gamma_ramp;
extern uint8_t rasterizer_gamma_high_bit_17;
extern uint8_t rasterizer_fullscreen;
extern void *rasterizer_device;
extern HWND rasterizer_window_handle;
extern void *rasterizer_misc_vertex_buffer;
extern transparent_geometry_group *transparent_geometry_groups;
extern transparent_geometry_group *transparent_geometry_groups_secondary;
extern int16_t *transparent_geometry_group_sorted_indices;
extern int32_t transparent_geometry_group_count;
extern int32_t transparent_geometry_group_secondary_count;
extern uint8_t rasterizer_software_vertex_processing;
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count];
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count];
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots];
extern void *rasterizer_dynamic_index_buffer;
extern int32_t config_safe_mode;
extern void *rasterizer_scratch_memory;
extern uint32_t rasterizer_scratch_memory_used;
extern int32_t config_min_max_blend_op_is_broken;
extern uint32_t rasterizer_blend_src_table[16];
extern uint32_t rasterizer_blend_dest_table[16];
extern uint32_t rasterizer_blend_op_table[16];
extern float g_007c1228;
extern float g_007c122c;
extern float g_007c1230;
extern float g_007c1234;
extern float g_007c1238;
extern float g_007c123c;
extern float g_007c1290;
extern float g_007c1294;
extern float g_007c1298;
extern float g_007c129c;
extern float g_007c12a0;
extern float g_007c12a4;
extern float g_007c12a8;
extern float g_007c12ac;
extern float g_007c12b0;
extern float g_007c12b4;
extern float g_007c12b8;
extern float g_007c12bc;
extern float g_007c12c4;
extern float g_007c12c8;
extern float g_007c12cc;
extern float g_007c12d0;
extern float g_007c12d4;
extern float g_007c12d8;
extern float g_007c13c0[16];
extern float g_007c13d0;
extern float g_007c13d4;
extern float g_007c13e0;
extern float g_007c13e4;
extern float g_007c13f0;
extern int16_t rasterizer_maximum_skinning_nodes;
extern rasterizer_skinning_matrix rasterizer_skinning_palette[63];
extern tag_instance *tag_instances;
extern GlobalsRasterizerData *rasterizer_globals_data;
extern uint8_t console_debug_toggle_689409;
extern int16_t rasterizer_bound_bitmap_size_a[2];
extern uint8_t *rasterizer_node_part_indices;
extern int32_t rasterizer_node_part_count;
extern d3d_gamma_ramp rasterizer_desktop_gamma_ramp;
extern int32_t shell_argc;
extern char **shell_argv;
extern int32_t safe_mode;
extern uint32_t config_transparent_decal_z_bias;
extern uint32_t config_transparent_decal_slope_z_bias;
extern float rasterizer_ui_text_constants[20];
extern Globals *global_globals;
extern lens_flare_object_visibility lens_flare_object_visibility_table[k_lens_flare_object_visibility_slots];
extern uint8_t lens_flare_marker_visibility[0x10008];
extern int32_t lens_flare_instance_count;
extern float *rasterizer_model_ambient_reflection_tint;
extern data_array *decal_data;
extern datum_index decal_vertex_cache_last_queried;
extern d3d_display_mode rasterizer_desktop_display_mode;
extern d3d_present_parameters rasterizer_present_parameters;
extern int32_t os_platform;
extern uint8_t unknown_006893ff;
extern int16_t unknown_00719aac;
extern int16_t screenshot_scale;
extern lens_flare_instance lens_flare_instances[0x400];
extern uint8_t lens_flare_instance_overflow;
extern uint8_t lens_flare_occlusion_queries_supported;
extern void *rasterizer_effect_pool_scratch;
extern int16_t unknown_00746fbc;
extern uint8_t rasterizer_caps_flag_68a;
extern uint8_t unknown_00689426;
extern uint32_t config_decal_z_bias;
extern uint32_t config_decal_slope_z_bias;
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count];
extern int32_t rasterizer_dynamic_vertex_slot_count;
extern int32_t rasterizer_dynamic_index_count;
extern int32_t rasterizer_dynamic_index_slot_count;
extern uint32_t unknown_0071d160;
extern uint32_t transparent_geometry_group_drawn_bits[12];
extern int32_t rasterizer_light_count;
extern uint16_t unknown_0071d1b4;
extern uint8_t rasterizer_render_target_capture_done;
extern uint8_t rasterizer_render_target_capture_requested;
extern uint8_t console_debug_toggle_6893e4;
extern uint8_t console_debug_toggle_6893e6;
extern int8_t renderer_texture_quality;
extern int32_t rasterizer_bitmap_format_to_d3dformat[];
extern int16_t rasterizer_cube_face_to_d3d_face[6];
extern uint32_t config_disable_buffering;
extern uint8_t unknown_0071d18d;
extern uint32_t screenshots;
extern void *shell_window;
extern uint8_t video_force_mode_flag;
extern uint32_t game_time_force_single_tick;
extern uint8_t rasterizer_device_lost;
extern uint8_t rasterizer_pending_clear;
extern uint32_t game_window_top_left;
extern uint32_t game_window_bottom_right;
extern int32_t rasterizer_present_counter_low;
extern int32_t rasterizer_present_counter_high;
extern void *shell_window_proc;
extern uint32_t rasterizer_window_style;
extern void *shell_instance;
extern char shell_window_class_name[];
extern char shell_window_title[];
extern void *shell_module_handle;
extern void *rasterizer_window_icon_bitmap;
extern void *rasterizer_window_icon_dc;
extern int16_t rasterizer_decal_layer;
extern int16_t rasterizer_decal_blend_mode;
extern int16_t rasterizer_decal_bitmap_frame;
extern uint32_t rasterizer_decal_bitmap_tag;
extern uint8_t unknown_0071d1c4;
extern uint8_t console_debug_toggle_689441;
extern void *rasterizer_decal_vertex_cache;
extern int16_t rasterizer_vertex_buffer_lock_state;
extern uint32_t *decal_grid_block;
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders];
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern void *rasterizer_detail_object_vertex_buffer;
extern uint8_t console_debug_toggle_689404;
extern float console_debug_value_689430;
extern Scenario *global_scenario;
extern ScenarioStructureBSP *global_structure_bsp;
extern int32_t rasterizer_vertex_buffer_slot_high_water;
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries];
extern void *rasterizer_effect_pool;
extern d3dx_macro rasterizer_effect_defines[2];
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects];
extern const char *rasterizer_shader_file_name;
extern int32_t config_force_shader;
extern const d3d_vertex_element9 vertex_elements_environment_uncompressed[];
extern const d3d_vertex_element9 vertex_elements_environment_lightmap[];
extern const d3d_vertex_element9 vertex_elements_model[];
extern const d3d_vertex_element9 vertex_elements_dynamic[];
extern const d3d_vertex_element9 vertex_elements_dynamic_screen[];
extern const d3d_vertex_element9 vertex_elements_debug[];
extern const d3d_vertex_element9 vertex_elements_decal[];
extern const d3d_vertex_element9 vertex_elements_detail_object[];
extern const d3d_vertex_element9 vertex_elements_environment_uncompressed_ff[];
extern const d3d_vertex_element9 vertex_elements_environment_lightmap_ff[];
extern const d3d_vertex_element9 vertex_elements_model_ff[];
extern const d3d_vertex_element9 vertex_elements_model_processed[];
extern const d3d_vertex_element9 vertex_elements_unlit_zsprite[];
extern const d3d_vertex_element9 vertex_elements_screen_transformed_lit[];
extern const d3d_vertex_element9 vertex_elements_screen_transformed_lit_specular[];
extern const d3d_vertex_element9 vertex_elements_environment_single_stream_ff[];
extern uint32_t rasterizer_triangle_buffer_primitive_types[2];
extern int32_t rasterizer_vertex_buffer_slot_count;
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots];
extern uint8_t rasterizer_dynamic_index_overflow;
extern uint8_t console_debug_toggle_6893f7;
extern int16_t render_force_flag;
extern uint8_t rasterizer_dynamic_vertex_overflow;
extern void *chat_gui_root_handle;
extern void *(*unknown_00721ea0)(void *hwnd, void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
extern uint32_t keystone_current_directory;
extern void *chat_gui_find_object_arg;
extern void *chat_listbox_gui_find_object_arg;
extern int32_t (*unknown_00721eb4)(void *engine, void *path, void *key, uint32_t flags, void *rect, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
extern int32_t (*unknown_00721eb8)(void *engine, void *key);
extern void (*unknown_00721edc)(int32_t document, uint32_t a);
extern void (*unknown_00721ec8)(int32_t document);
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets];
extern int16_t rasterizer_active_render_target;
extern void *rasterizer_render_target_vertex_buffer;
extern void *rasterizer_render_target_index_buffer;
extern uint8_t rasterizer_frame_started;
extern uint8_t rasterizer_in_scene;
extern uint32_t config_linear_texture_addressing;
extern uint8_t chat_dialog_open;
extern int32_t (*unknown_00721ebc)(void *engine);
extern int32_t rasterizer_ui_render_failed;
extern uint8_t console_debug_toggle_6893f4;
extern uint8_t console_debug_toggle_68941d;
extern uint8_t config_use_anisotropic_filter;
extern uint8_t rasterizer_fog_enabled;
extern int32_t video_gamma_current;
extern uint32_t rasterizer_frustum_z_values[2];
extern real_matrix4x3 *k_render_identity_matrix_ptr;
extern uint32_t rasterizer_depth_prepass_vertex_shader;
extern void *rasterizer_capture_surfaces[4];
extern void *rasterizer_glass_draw_procedures[3];
extern int16_t rasterizer_bound_bitmap_size_b[2];
extern void *rasterizer_direct3d;
extern uint8_t rasterizer_caps_flag_688;
extern uint8_t rasterizer_caps_flag_689;
extern uint32_t rasterizer_device_type;
extern int16_t rasterizer_texture_stage_count;
extern uint32_t d3d_adapter;
extern uint8_t rasterizer_use_fx_file;
extern void *shell_direct3d;
extern void *(__stdcall *direct3d_create9_procedure)(uint32_t sdk_version);
extern int32_t rasterizer_window_requested;
extern int32_t windowed;
extern int32_t checkfpu;
extern int32_t config_disable_driver_management;
extern int32_t width640;
extern uint32_t physical_memory;
extern uint32_t cpu_speed;
extern int32_t config_use_fixed_function;
extern int32_t config_unsupported_card;
extern int32_t config_prototype_card;
extern int32_t config_old_driver;
extern int32_t config_old_sound_driver;
extern int32_t config_invalid_driver;
extern int32_t config_invalid_sound_driver;
extern int32_t config_disable_render_targets;
extern int32_t config_disable_alpha_render_targets;
extern uint32_t graphics_device_id;
extern uint32_t graphics_vendor_id;
extern uint32_t video_memory;
extern uint32_t required_video_memory;
extern uint8_t crc32_lookup_table_initialized;
extern crc32_table crc32_lookup_table;
extern void (*unknown_00721eac)(void *engine);
extern lens_flare_batch_key lens_flare_applied_key;
extern lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots];
extern lens_flare_batch_key lens_flare_current_key;
extern uint32_t lens_flare_batch_clock;
extern uint8_t console_debug_toggle_689425;
extern void *unknown_0069da10;
extern uint32_t lens_flare_vertex_specular;
extern uint8_t console_debug_toggle_689424;
extern uint8_t transparent_geometry_group_overflow_d;
extern uint8_t console_debug_toggle_6893f3;
extern rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights];
extern uint8_t rasterizer_default_material[0x44];
extern int32_t rasterizer_fixed_function_light_count;
extern uint32_t unknown_006e1b58;
extern uint8_t console_debug_toggle_6893ec;
extern uint8_t console_debug_toggle_689421;
extern rasterizer_model_draw_context *rasterizer_active_model_context;
extern uint8_t rasterizer_model_scratch_valid;
extern uint8_t unknown_0071d1fb;
extern uint8_t unknown_0071d1fc;
extern uint8_t unknown_0071d1fd;
extern int16_t rasterizer_active_model_mode;
extern float unknown_007c047c;
extern uint8_t console_debug_toggle_689403;
extern uint8_t rasterizer_motion_sensor_ready;
extern player_globals *local_player_globals;
extern const float rasterizer_identity_vertex_constants[5][4];
extern uint8_t console_debug_toggle_6893f2;
extern uint8_t console_debug_toggle_68941f;
extern ColorRGB rasterizer_object_shadow_color;
extern float rasterizer_object_shadow_radius;
extern real_matrix4x3 rasterizer_object_shadow_projection;
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context;
extern uint8_t rasterizer_object_shadow_prepared;
extern uint8_t rasterizer_object_shadow_model_active;
extern uint8_t rasterizer_object_shadow_window_restored;
extern uint8_t console_debug_toggle_68941e;
extern rasterizer_dynamic_screen_vertex rasterizer_object_shadow_blur_quad[4];
extern rasterizer_screen_vertex rasterizer_object_shadow_border_lines[8];
extern uint8_t unknown_0071d1b0;
extern uint8_t rasterizer_needs_reset;
extern float unknown_00689418;
extern uint32_t renderer_unknown_69c684;
extern ColorRGB zoom_static_tint_r;
extern uint8_t console_debug_toggle_6893f6;
extern int16_t rasterizer_projected_light_shader_variant;
extern float rasterizer_projected_light_luminance;
extern rasterizer_projected_light_constants rasterizer_projected_light;
extern uint8_t rasterizer_projected_light_has_cube_map;
extern uint32_t rasterizer_projected_light_cube_map;
extern uint8_t console_debug_toggle_689422;
extern float rasterizer_screen_quad_vertices[4][6];
extern uint8_t unknown_006893f6;
extern uint8_t unknown_006893f7;
extern uint8_t unknown_006893f8;
extern uint8_t console_debug_toggle_6893f9;
extern uint8_t unknown_006893fd;
extern uint8_t unknown_0068941d;
extern int16_t unknown_0069c640;
extern int16_t unknown_0069c642;
extern int16_t unknown_0069c63e;
extern int16_t game_screen_rect;
extern int16_t rasterizer_bound_bitmap_size_c[2];
extern uint8_t config_linear_texture_addressing_zoom;
extern uint32_t screen_effect_techniques[k_rasterizer_screen_effect_techniques];
extern uint8_t console_debug_toggle_689428;
extern uint32_t config_use_alternate_convolve_mask;
extern rasterizer_dynamic_screen_vertex rasterizer_screen_effect_quad[4];
extern void *screen_flash_techniques[6];
extern void *rasterizer_screen_flash_effect;
extern uint8_t console_debug_toggle_689427;
extern void *unknown_007c048c;
extern void *unknown_007c0490;
extern void *unknown_007c0494;
extern void *rasterizer_water_draw_procedure;
extern uint8_t console_debug_toggle_689407;
extern uint8_t console_debug_toggle_689408;
extern uint8_t console_debug_toggle_6893fc;
extern const ColorRGB *global_white_color;
extern uint8_t unknown_006893ef;
extern int16_t rasterizer_shader_stage_config;
extern uint8_t console_debug_toggle_6893fa;
extern int32_t environment_techniques_multipurpose[24];
extern int32_t environment_techniques_no[12];
extern int32_t environment_techniques_self_illumination[24];
extern int32_t environment_techniques_plain[12];
extern int32_t environment_techniques_reflection[24];
extern int32_t environment_techniques_change_color[24];
extern const char rasterizer_shader_technique_name_suffixes[][0x80];
extern void *shader_environment_draw_simple;
extern void *shader_environment_draw;
extern uint32_t rasterizer_device_version;
extern float rasterizer_camera_position[3];
extern float rasterizer_camera_forward[3];
extern uint8_t rasterizer_fog_flags;
extern ColorRGB rasterizer_fog_atmospheric_color;
extern float rasterizer_fog_atmospheric_max_density;
extern float rasterizer_fog_atmospheric_min_distance;
extern float rasterizer_fog_atmospheric_max_distance;
extern float rasterizer_fog_plane[4];
extern ColorRGB rasterizer_fog_planar_color;
extern rasterizer_effect_slot environment_effect_slot;
extern uint32_t environment_techniques_ps14[];
extern uint32_t rasterizer_model_vertex_declaration;
extern uint8_t rasterizer_lightmap_bitmap_missing;
extern BitmapData *rasterizer_lightmap_bitmap;
extern uint8_t unknown_006e0a04;
extern BitmapData *rasterizer_environment_lightmap;
extern uint8_t console_debug_toggle_6893f1;
extern uint8_t console_debug_toggle_68941c;
extern uint8_t console_debug_toggle_6893f8;
extern rasterizer_effect_slot *rasterizer_active_environment_effect;
extern uint8_t rasterizer_environment_lightmap_missing;
extern uint8_t rasterizer_camouflage_fade_active;
extern float rasterizer_camouflage_fade;
extern float rasterizer_model_effect_vector[4];
extern int16_t rasterizer_transparent_vertex_shader_table[];
extern const int16_t rasterizer_first_map_bitmap_types[4];
extern const uint32_t rasterizer_first_map_address_modes[4];
extern int16_t rasterizer_transparent_extended_vertex_shader_table[];
extern const int16_t rasterizer_extended_first_map_bitmap_types[4];
extern const uint32_t rasterizer_extended_first_map_address_modes[4];
extern uint32_t rasterizer_chicago_color_function_stage_states[][3];
extern uint8_t console_debug_toggle_689423;
extern rasterizer_dynamic_screen_vertex rasterizer_shadow_screen_quad[4];
extern const float rasterizer_sun_glow_blur_offsets[8][4];
extern uint32_t config_linear_texture_addressing_sun;
extern uint8_t console_debug_toggle_6893ed;
extern transparent_geometry_group transparent_geometry_group_environment_immediate;
extern uint8_t transparent_geometry_group_overflow_c;
extern uint8_t model_render_first_person;
extern int32_t transparent_geometry_group_last_drawn_key;
extern uint8_t rasterizer_secondary_groups_drawn;
extern uint8_t rasterizer_render_states_dirty;
extern void *rasterizer_model_scratch_node_matrices;
extern int16_t rasterizer_model_scratch_node_count;
extern void *rasterizer_model_scratch_lighting;
extern void *rasterizer_model_scratch_function_source;
extern uint8_t console_debug_toggle_6893eb;
extern int16_t debug_print_enabled_flag;
extern float console_debug_meter_period;
extern float console_debug_meter_values[4];
extern uint8_t rasterizer_widescreen_camouflage_scale;
extern uint8_t unknown_0071d1fa;
extern uint8_t console_debug_toggle_6893fb;
extern transparent_geometry_group transparent_geometry_group_immediate;
extern uint8_t transparent_geometry_group_overflow_b;
extern uint8_t transparent_geometry_group_overflow_a;
extern uint8_t console_debug_toggle_689400;
extern float renderer_unknown_68940c;
extern float rasterizer_underwater_tint_jitter_r;
extern float rasterizer_underwater_tint_jitter_g;
extern float rasterizer_underwater_tint_jitter_b;
extern uint8_t rasterizer_underwater_material[0x44];
extern uint8_t rasterizer_water_enabled;
extern float water_fade_plane_distance;
extern float water_fade_factor_a;
extern float water_fade_factor_b;
extern uint32_t renderer_unknown_6e1af8;
extern float rasterizer_water_ripple_quad[4][6];
extern int16_t render_window_index;
extern uint32_t text_shadow_color_argb;
extern uint8_t font_glyph_cache_slots[];
extern int16_t transparent_geometry_group_draw_cursor;

}  // extern "C"

namespace halo::rasterizer {

int16_t bitmap_compute_mipmap_count(BitmapData *bitmap);
int32_t bitmap_compute_texture_data_size(BitmapData *bitmap);
int16_t * chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type, int16_t default_index, int16_t frame);
uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame);
uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame, rasterizer_effect_slot *effect_slot);
uint8_t color_channel_real_to_byte(float channel);
uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap);
namespace rasterizer_bind_texture_d3dx_impl { uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); }
using rasterizer_bind_texture_d3dx_impl::rasterizer_bind_texture_d3dx;
int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height);
namespace rasterizer_bitmap_create_hardware_texture_impl { uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap); }
using rasterizer_bitmap_create_hardware_texture_impl::rasterizer_bitmap_create_hardware_texture;
int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias);
namespace rasterizer_bitmap_upload_2d_mipmaps_impl { void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap); }
using rasterizer_bitmap_upload_2d_mipmaps_impl::rasterizer_bitmap_upload_2d_mipmaps;
namespace rasterizer_bitmap_upload_cubemap_mipmaps_impl { void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap); }
using rasterizer_bitmap_upload_cubemap_mipmaps_impl::rasterizer_bitmap_upload_cubemap_mipmaps;
namespace rasterizer_bitmap_upload_cubemap_mipmaps_by_face_impl { void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap); }
using rasterizer_bitmap_upload_cubemap_mipmaps_by_face_impl::rasterizer_bitmap_upload_cubemap_mipmaps_by_face;
namespace rasterizer_force_bilinear_filtering_impl { void rasterizer_force_bilinear_filtering(void); }
using rasterizer_force_bilinear_filtering_impl::rasterizer_force_bilinear_filtering;
namespace rasterizer_render_target_bind_effect_texture_impl { void rasterizer_render_target_bind_effect_texture(int16_t target_index, rasterizer_effect_slot *effect_slot, int16_t handle_index); }
using rasterizer_render_target_bind_effect_texture_impl::rasterizer_render_target_bind_effect_texture;
void * rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage);
int16_t * rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot);
uint8_t rasterizer_resolve_and_cache_submap_c(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame);
namespace rasterizer_unbind_stream_and_textures_impl { void rasterizer_unbind_stream_and_textures(void); }
using rasterizer_unbind_stream_and_textures_impl::rasterizer_unbind_stream_and_textures;
uint8_t rasterizer_validate_and_rebind_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t frame);
void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex, real_vector3d *out);
void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex, real_vector3d *out);
uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction);
real_vector3d * vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed);
void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source);
void chimera__gamma(void);
void chimera__registry_check_3(void);
void chimera__registry_check_4(void);
namespace rasterizer_fog_screen_overlay_set_states_impl { void rasterizer_fog_screen_overlay_set_states(void); }
using rasterizer_fog_screen_overlay_set_states_impl::rasterizer_fog_screen_overlay_set_states;
void rasterizer_gamma_brightness_to_exponent(rasterizer_gamma_settings *settings);
namespace rasterizer_motion_sensor_begin_impl { void rasterizer_motion_sensor_begin(void); }
using rasterizer_motion_sensor_begin_impl::rasterizer_motion_sensor_begin;
namespace rasterizer_motion_sensor_blip_draw_impl { void rasterizer_motion_sensor_blip_draw(const float *position, const float *color, float brightness, float size); }
using rasterizer_motion_sensor_blip_draw_impl::rasterizer_motion_sensor_blip_draw;
namespace rasterizer_motion_sensor_end_impl { void rasterizer_motion_sensor_end(const float *position, float sweep); }
using rasterizer_motion_sensor_end_impl::rasterizer_motion_sensor_end;
namespace rasterizer_screen_effect_compute_uv_transform_impl { void rasterizer_screen_effect_compute_uv_transform(uint32_t width, uint32_t height, weapon_screen_effect_parameters *params, int16_t pass, int16_t pass_count, uint8_t shift_down); }
using rasterizer_screen_effect_compute_uv_transform_impl::rasterizer_screen_effect_compute_uv_transform;
uint8_t rasterizer_screen_effect_init_shaders(void);
namespace rasterizer_screen_effect_render_impl { void rasterizer_screen_effect_render(weapon_screen_effect_parameters *input); }
using rasterizer_screen_effect_render_impl::rasterizer_screen_effect_render;
namespace rasterizer_screen_effect_render_fixed_function_impl { void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *input); }
using rasterizer_screen_effect_render_fixed_function_impl::rasterizer_screen_effect_render_fixed_function;
int rasterizer_screen_flash_init_shaders(void);
namespace rasterizer_screen_flash_render_impl { void rasterizer_screen_flash_render(void); }
using rasterizer_screen_flash_render_impl::rasterizer_screen_flash_render;
namespace rasterizer_sun_glow_blur_impl { int16_t rasterizer_sun_glow_blur(int16_t first, int16_t second, int16_t passes); }
using rasterizer_sun_glow_blur_impl::rasterizer_sun_glow_blur;
namespace rasterizer_sun_glow_capture_impl { void rasterizer_sun_glow_capture(const float *rect, int16_t target_index); }
using rasterizer_sun_glow_capture_impl::rasterizer_sun_glow_capture;
uint8_t rasterizer_sun_glow_project_point(real_point3d *point, float radius, float *out_screen, float *out_scale);
namespace rasterizer_sun_glow_render_impl { void rasterizer_sun_glow_render(lens_flare_instance *instance); }
using rasterizer_sun_glow_render_impl::rasterizer_sun_glow_render;
namespace rasterizer_ui_quad_draw_impl { void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices); }
using rasterizer_ui_quad_draw_impl::rasterizer_ui_quad_draw;
void rasterizer_underwater_tint_jitter_update(BitmapData *lightmap);
namespace rasterizer_underwater_tint_set_states_impl { void rasterizer_underwater_tint_set_states(void); }
using rasterizer_underwater_tint_set_states_impl::rasterizer_underwater_tint_set_states;
void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text);
void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const char *text);
void chimera__widescreen_text_scaling(void);
void font_glyph_cache_allocate_and_upload(Font *font, FontCharacter *character);
void font_glyph_cache_clear_all(void);
void rasterizer_draw_text_begin(ui_quad_render_state *state);
namespace rasterizer_draw_text_end_impl { void rasterizer_draw_text_end(void); }
using rasterizer_draw_text_end_impl::rasterizer_draw_text_end;
void rasterizer_editbox_log_dump(void);
namespace text_draw_glyph_callback_impl { void text_draw_glyph_callback(void *state, void *font, uint8_t *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height); }
using text_draw_glyph_callback_impl::text_draw_glyph_callback;
int32_t text_font_system_initialize(void);
void chimera__rasterizer_dispose_free_memory(void);
void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive);
namespace chimera__rasterizer_draw_dynamic_triangles_static_vertices2_impl { void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, rasterizer_vertex_buffer *second_stream); }
using chimera__rasterizer_draw_dynamic_triangles_static_vertices2_impl::chimera__rasterizer_draw_dynamic_triangles_static_vertices2;
void * chimera__rasterizer_memory_alloc(void *source, uint32_t size);
namespace rasterizer_dynamic_geometry_chain_draw_impl { void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer); }
using rasterizer_dynamic_geometry_chain_draw_impl::rasterizer_dynamic_geometry_chain_draw;
void rasterizer_dynamic_geometry_dispose(void);
void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count, int32_t first_primitive, int32_t dynamic_vertex_slot);
namespace rasterizer_dynamic_index_cache_draw_impl { void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot); }
using rasterizer_dynamic_index_cache_draw_impl::rasterizer_dynamic_index_cache_draw;
int32_t rasterizer_dynamic_index_cache_reserve(int32_t count);
namespace rasterizer_dynamic_light_technique_ps2_set_states_impl { void rasterizer_dynamic_light_technique_ps2_set_states(void); }
using rasterizer_dynamic_light_technique_ps2_set_states_impl::rasterizer_dynamic_light_technique_ps2_set_states;
namespace rasterizer_dynamic_vertex_cache_lock_impl { void * rasterizer_dynamic_vertex_cache_lock(int32_t slot_index); }
using rasterizer_dynamic_vertex_cache_lock_impl::rasterizer_dynamic_vertex_cache_lock;
int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count);
namespace rasterizer_dynamic_vertex_draw_impl { void rasterizer_dynamic_vertex_draw(int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot, int16_t primitive_kind); }
using rasterizer_dynamic_vertex_draw_impl::rasterizer_dynamic_vertex_draw;
namespace rasterizer_dynamic_vertex_draw_indexed_impl { void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count, int32_t dynamic_vertex_slot); }
using rasterizer_dynamic_vertex_draw_indexed_impl::rasterizer_dynamic_vertex_draw_indexed;
namespace rasterizer_dynamic_vertex_process_and_get_handle_impl { uint32_t rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_dynamic_vertex_process_and_get_handle_impl::rasterizer_dynamic_vertex_process_and_get_handle;
namespace rasterizer_geometry_draw_fixed_function_impl { void rasterizer_geometry_draw_fixed_function(uint32_t flags, int32_t dynamic_vertex_slot, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count); }
using rasterizer_geometry_draw_fixed_function_impl::rasterizer_geometry_draw_fixed_function;
namespace rasterizer_geometry_part_draw_impl { void rasterizer_geometry_part_draw(transparent_geometry_group *group); }
using rasterizer_geometry_part_draw_impl::rasterizer_geometry_part_draw;
void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode);
namespace chimera__rasterizer_set_frustum_z_func_impl { void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); }
using chimera__rasterizer_set_frustum_z_func_impl::chimera__rasterizer_set_frustum_z_func;
void display_mode_get_current(rasterizer_display_mode *out);
void rasterizer_begin_frame(rasterizer_window_parameters *source);
void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source);
namespace rasterizer_capture_and_present_impl { void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); }
using rasterizer_capture_and_present_impl::rasterizer_capture_and_present;
uint32_t rasterizer_create_game_window(int32_t height, int32_t width);
namespace rasterizer_device_reset_impl { uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters); }
using rasterizer_device_reset_impl::rasterizer_device_reset;
uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested);
namespace rasterizer_end_frame_impl { void rasterizer_end_frame(void); }
using rasterizer_end_frame_impl::rasterizer_end_frame;
int32_t rasterizer_get_refresh_rate(int32_t requested_rate);
namespace rasterizer_initialize_direct3d_impl { uint8_t rasterizer_initialize_direct3d(void); }
using rasterizer_initialize_direct3d_impl::rasterizer_initialize_direct3d;
uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out);
namespace rasterizer_reset_device_if_needed_impl { uint8_t rasterizer_reset_device_if_needed(void); }
using rasterizer_reset_device_if_needed_impl::rasterizer_reset_device_if_needed;
void rasterizer_resize_game_window(int32_t height, int32_t width);
int32_t rasterizer_round_up_resolution_height(int32_t height);
void rasterizer_select_hardware_codepaths(void);
namespace rasterizer_service_deferred_windowed_ops_impl { void rasterizer_service_deferred_windowed_ops(void); }
using rasterizer_service_deferred_windowed_ops_impl::rasterizer_service_deferred_windowed_ops;
namespace rasterizer_set_default_render_states_impl { void rasterizer_set_default_render_states(void); }
using rasterizer_set_default_render_states_impl::rasterizer_set_default_render_states;
void rasterizer_shutdown(void);
void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes);
void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices);
namespace rasterizer_model_draw_prepare_states_impl { void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode); }
using rasterizer_model_draw_prepare_states_impl::rasterizer_model_draw_prepare_states;
namespace rasterizer_model_draw_restore_states_impl { void rasterizer_model_draw_restore_states(void); }
using rasterizer_model_draw_restore_states_impl::rasterizer_model_draw_restore_states;
namespace rasterizer_object_shadow_begin_impl { uint8_t rasterizer_object_shadow_begin(const real_matrix4x3 *projection, const ColorRGB *color, float radius, float *out_radius); }
using rasterizer_object_shadow_begin_impl::rasterizer_object_shadow_begin;
namespace rasterizer_object_shadow_blur_impl { void rasterizer_object_shadow_blur(void); }
using rasterizer_object_shadow_blur_impl::rasterizer_object_shadow_blur;
namespace rasterizer_object_shadow_model_draw_impl { void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame, rasterizer_index_buffer *index_buffer, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_object_shadow_model_draw_impl::rasterizer_object_shadow_model_draw;
namespace rasterizer_object_shadow_structure_draw_impl { void rasterizer_object_shadow_structure_draw(rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count); }
using rasterizer_object_shadow_structure_draw_impl::rasterizer_object_shadow_structure_draw;
namespace rasterizer_shader_model_draw_fixed_function_impl { void rasterizer_shader_model_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot); }
using rasterizer_shader_model_draw_fixed_function_impl::rasterizer_shader_model_draw_fixed_function;
namespace rasterizer_shader_model_draw_limited_impl { void rasterizer_shader_model_draw_limited(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot); }
using rasterizer_shader_model_draw_limited_impl::rasterizer_shader_model_draw_limited;
namespace rasterizer_shader_model_draw_pixel_shader_impl { void rasterizer_shader_model_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot); }
using rasterizer_shader_model_draw_pixel_shader_impl::rasterizer_shader_model_draw_pixel_shader;
namespace rasterizer_shader_model_select_technique_impl { rasterizer_effect_slot * rasterizer_shader_model_select_technique(const ShaderModel *shader); }
using rasterizer_shader_model_select_technique_impl::rasterizer_shader_model_select_technique;
void chimera__transparent_decal_zbias(void);
void decal_and_font_system_reset(void);
void decal_geometry_cache_restore_procs(void);
uint8_t decal_vertex_cache_in_use(datum_index handle);
void decal_vertex_cache_release(datum_index handle);
void rasterizer_apply_decal_zbias(void);
void rasterizer_clear_decal_zbias(void);
namespace rasterizer_decal_index_buffer_initialize_impl { uint8_t rasterizer_decal_index_buffer_initialize(void); }
using rasterizer_decal_index_buffer_initialize_impl::rasterizer_decal_index_buffer_initialize;
namespace rasterizer_decal_pass_begin_impl { void rasterizer_decal_pass_begin(int16_t stage); }
using rasterizer_decal_pass_begin_impl::rasterizer_decal_pass_begin;
namespace rasterizer_decal_vertex_cache_lock_impl { void * rasterizer_decal_vertex_cache_lock(uint32_t decal_index, int32_t byte_count); }
using rasterizer_decal_vertex_cache_lock_impl::rasterizer_decal_vertex_cache_lock;
int rasterizer_decal_zbias_active(void);
namespace rasterizer_decals_draw_cluster_impl { void rasterizer_decals_draw_cluster(int16_t cluster_index); }
using rasterizer_decals_draw_cluster_impl::rasterizer_decals_draw_cluster;
namespace rasterizer_decals_initialize_impl { void rasterizer_decals_initialize(void); }
using rasterizer_decals_initialize_impl::rasterizer_decals_initialize;
void rasterizer_end_decal_pass(void);
namespace rasterizer_shader_decal_pass_set_states_impl { void rasterizer_shader_decal_pass_set_states(void); }
using rasterizer_shader_decal_pass_set_states_impl::rasterizer_shader_decal_pass_set_states;
void lens_flare_add_instance(lens_flare_instance *candidate);
float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode);
uint8_t * lens_flare_get_visibility_byte(lens_flare_instance *flare);
void lens_flare_render_all(void);
void lens_flare_update_samples(void);
void lens_flare_update_visibility(void);
uint8_t rasterizer_lens_flare_batch_apply_material(lens_flare_batch_key *key);
void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index);
int32_t rasterizer_lens_flare_batch_find_slot(void);
void rasterizer_lens_flare_batch_flush_all(void);
namespace rasterizer_lens_flare_batching_select_mode_impl { void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags); }
using rasterizer_lens_flare_batching_select_mode_impl::rasterizer_lens_flare_batching_select_mode;
namespace rasterizer_lens_flare_occlusion_queries_create_impl { uint8_t rasterizer_lens_flare_occlusion_queries_create(void); }
using rasterizer_lens_flare_occlusion_queries_create_impl::rasterizer_lens_flare_occlusion_queries_create;
namespace rasterizer_lens_flare_occlusion_query_get_result_impl { int32_t rasterizer_lens_flare_occlusion_query_get_result(int32_t slot_index); }
using rasterizer_lens_flare_occlusion_query_get_result_impl::rasterizer_lens_flare_occlusion_query_get_result;
void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1, uint32_t id_2);
namespace rasterizer_lens_flare_occlusion_test_issue_impl { int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position, float radius); }
using rasterizer_lens_flare_occlusion_test_issue_impl::rasterizer_lens_flare_occlusion_test_issue;
uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius, float *out_screen, float *out_inverse_w, float *out_billboard_size);
void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position, float radius, float rotation_degrees);
void structure_cluster_add_lens_flares(int16_t cluster_index);
uint8_t rasterizer_detail_object_vertex_buffer_create(void);
namespace rasterizer_detail_objects_begin_impl { void rasterizer_detail_objects_begin(void); }
using rasterizer_detail_objects_begin_impl::rasterizer_detail_objects_begin;
namespace rasterizer_detail_objects_draw_impl { void rasterizer_detail_objects_draw(const rasterizer_detail_object_batches *list); }
using rasterizer_detail_objects_draw_impl::rasterizer_detail_objects_draw;
void rasterizer_detail_objects_expand_quad_vertices(int32_t quad_count, uint32_t *vertices, const uint8_t *instances, const DetailObjectCollection *collection, const rasterizer_detail_object_draw *draw);
namespace rasterizer_detail_objects_vertex_buffer_fill_impl { void rasterizer_detail_objects_vertex_buffer_fill(rasterizer_detail_object_batches *list); }
using rasterizer_detail_objects_vertex_buffer_fill_impl::rasterizer_detail_objects_vertex_buffer_fill;
void * rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf, uint8_t not_dynamic);
uint8_t rasterizer_dx9_effects_initialize(void);
int32_t rasterizer_dx9_pixel_shader_effect_load(int32_t effect_index, const void *data, uint32_t size);
uint8_t rasterizer_dx9_pixel_shaders_load_all(void);
void rasterizer_dx9_pixel_shaders_release(void);
namespace rasterizer_dx9_shaders_init_effect_impl { int32_t rasterizer_dx9_shaders_init_effect(int32_t effect_index); }
using rasterizer_dx9_shaders_init_effect_impl::rasterizer_dx9_shaders_init_effect;
void rasterizer_dx9_shaders_release_all(void);
void rasterizer_dx9_vertex_declarations_release(void);
uint8_t rasterizer_dx9_vertex_shaders_initialize(void);
namespace rasterizer_dx9_vertex_shaders_load_all_impl { uint32_t rasterizer_dx9_vertex_shaders_load_all(void); }
using rasterizer_dx9_vertex_shaders_load_all_impl::rasterizer_dx9_vertex_shaders_load_all;
uint8_t rasterizer_dx9_vertex_shaders_reload(void);
void * rasterizer_get_capture_surface(uint8_t *object, void *fallback);
namespace rasterizer_index_buffer_create_impl { uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out, const void *source); }
using rasterizer_index_buffer_create_impl::rasterizer_index_buffer_create;
void rasterizer_ksml_ui_shutdown(void);
uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path);
namespace rasterizer_misc_vertex_buffer_create_impl { uint8_t rasterizer_misc_vertex_buffer_create(void); }
using rasterizer_misc_vertex_buffer_create_impl::rasterizer_misc_vertex_buffer_create;
namespace rasterizer_render_target_capture_frame_impl { void rasterizer_render_target_capture_frame(void); }
using rasterizer_render_target_capture_frame_impl::rasterizer_render_target_capture_frame;
namespace rasterizer_render_target_dispose_impl { void rasterizer_render_target_dispose(void); }
using rasterizer_render_target_dispose_impl::rasterizer_render_target_dispose;
namespace rasterizer_render_target_initialize_impl { uint8_t rasterizer_render_target_initialize(void); }
using rasterizer_render_target_initialize_impl::rasterizer_render_target_initialize;
namespace rasterizer_render_target_set_active_impl { void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); }
using rasterizer_render_target_set_active_impl::rasterizer_render_target_set_active;
uint8_t rasterizer_resource_file_verify_signature(uint8_t *buffer, uint32_t size);
namespace rasterizer_vertex_buffer_create_impl { uint8_t rasterizer_vertex_buffer_create(rasterizer_vertex_buffer *record, int16_t vertex_type, int32_t count, uint32_t *source_data, int32_t second_stream, uint32_t size); }
using rasterizer_vertex_buffer_create_impl::rasterizer_vertex_buffer_create;
int32_t rasterizer_vertex_buffer_slot_allocate(int32_t vertex_type, uint32_t fvf, uint32_t length);
void rasterizer_vertex_buffer_slot_recreate_lost(void);
uint8_t rasterizer_dx9_shaders_initialize(void);
namespace rasterizer_dx9_vertex_declarations_create_impl { uint8_t rasterizer_dx9_vertex_declarations_create(void); }
using rasterizer_dx9_vertex_declarations_create_impl::rasterizer_dx9_vertex_declarations_create;
namespace rasterizer_render_loading_screen_impl { void rasterizer_render_loading_screen(int32_t mode); }
using rasterizer_render_loading_screen_impl::rasterizer_render_loading_screen;
void rasterizer_glass_diffuse_draw(transparent_geometry_group *group);
namespace rasterizer_glass_diffuse_draw_fixed_function_impl { void rasterizer_glass_diffuse_draw_fixed_function(transparent_geometry_group *group); }
using rasterizer_glass_diffuse_draw_fixed_function_impl::rasterizer_glass_diffuse_draw_fixed_function;
void rasterizer_glass_draw_procedures_select(void);
namespace rasterizer_glass_reflection_draw_impl { void rasterizer_glass_reflection_draw(transparent_geometry_group *group, int16_t reflection_kind); }
using rasterizer_glass_reflection_draw_impl::rasterizer_glass_reflection_draw;
namespace rasterizer_glass_reflection_draw_fixed_function_impl { void rasterizer_glass_reflection_draw_fixed_function(transparent_geometry_group *group, uint32_t reflection_kind); }
using rasterizer_glass_reflection_draw_fixed_function_impl::rasterizer_glass_reflection_draw_fixed_function;
namespace rasterizer_glass_tint_draw_impl { void rasterizer_glass_tint_draw(transparent_geometry_group *group); }
using rasterizer_glass_tint_draw_impl::rasterizer_glass_tint_draw;
namespace rasterizer_glass_tint_draw_fixed_function_impl { void rasterizer_glass_tint_draw_fixed_function(transparent_geometry_group *group); }
using rasterizer_glass_tint_draw_fixed_function_impl::rasterizer_glass_tint_draw_fixed_function;
namespace rasterizer_shader_transparent_chicago_draw_impl { void rasterizer_shader_transparent_chicago_draw(transparent_geometry_group *group, uint8_t attached); }
using rasterizer_shader_transparent_chicago_draw_impl::rasterizer_shader_transparent_chicago_draw;
namespace rasterizer_shader_transparent_chicago_extended_draw_impl { void rasterizer_shader_transparent_chicago_extended_draw(transparent_geometry_group *group, uint8_t attached); }
using rasterizer_shader_transparent_chicago_extended_draw_impl::rasterizer_shader_transparent_chicago_extended_draw;
namespace rasterizer_shader_transparent_chicago_extended_set_texture_stages_impl { uint8_t rasterizer_shader_transparent_chicago_extended_set_texture_stages(const ShaderTransparentChicagoExtended *shader); }
using rasterizer_shader_transparent_chicago_extended_set_texture_stages_impl::rasterizer_shader_transparent_chicago_extended_set_texture_stages;
namespace rasterizer_shader_transparent_chicago_set_texture_stages_impl { uint8_t rasterizer_shader_transparent_chicago_set_texture_stages(const ShaderTransparentChicago *shader); }
using rasterizer_shader_transparent_chicago_set_texture_stages_impl::rasterizer_shader_transparent_chicago_set_texture_stages;
namespace rasterizer_shader_transparent_plasma_draw_impl { void rasterizer_shader_transparent_plasma_draw(transparent_geometry_group *group); }
using rasterizer_shader_transparent_plasma_draw_impl::rasterizer_shader_transparent_plasma_draw;
namespace rasterizer_water_draw_fixed_function_impl { void rasterizer_water_draw_fixed_function(transparent_geometry_group *group); }
using rasterizer_water_draw_fixed_function_impl::rasterizer_water_draw_fixed_function;
namespace rasterizer_water_draw_pixel_shader_impl { void rasterizer_water_draw_pixel_shader(transparent_geometry_group *group); }
using rasterizer_water_draw_pixel_shader_impl::rasterizer_water_draw_pixel_shader;
namespace rasterizer_water_fade_compute_and_set_states_impl { void rasterizer_water_fade_compute_and_set_states(void); }
using rasterizer_water_fade_compute_and_set_states_impl::rasterizer_water_fade_compute_and_set_states;
namespace rasterizer_water_ripple_draw_impl { void rasterizer_water_ripple_draw(rasterizer_vertex_buffer *vertex_buffer, const Shader *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count); }
using rasterizer_water_ripple_draw_impl::rasterizer_water_ripple_draw;
namespace rasterizer_water_update_ripple_texture_impl { void rasterizer_water_update_ripple_texture(void *water_shader); }
using rasterizer_water_update_ripple_texture_impl::rasterizer_water_update_ripple_texture;
void rasterizer_light_cone_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer);
namespace rasterizer_light_cone_set_orientation_constants_impl { void rasterizer_light_cone_set_orientation_constants(int32_t light_index); }
using rasterizer_light_cone_set_orientation_constants_impl::rasterizer_light_cone_set_orientation_constants;
namespace rasterizer_light_cone_set_texture_stage_states_impl { void rasterizer_light_cone_set_texture_stage_states(void); }
using rasterizer_light_cone_set_texture_stage_states_impl::rasterizer_light_cone_set_texture_stage_states;
namespace rasterizer_light_disable_all_impl { void rasterizer_light_disable_all(void); }
using rasterizer_light_disable_all_impl::rasterizer_light_disable_all;
namespace rasterizer_light_set_impl { void rasterizer_light_set(rasterizer_light *light); }
using rasterizer_light_set_impl::rasterizer_light_set;
void rasterizer_light_set_point_constants(int32_t light_index, int16_t slot, rasterizer_point_light_constants *dest_base);
namespace rasterizer_prepare_lighting_constants_impl { void rasterizer_prepare_lighting_constants(render_lighting *lighting); }
using rasterizer_prepare_lighting_constants_impl::rasterizer_prepare_lighting_constants;
void rasterizer_projected_light_constants_build(int32_t light_index);
void rasterizer_projected_light_constants_build_cube_map(int32_t light_index);
namespace rasterizer_set_fog_constants_impl { void rasterizer_set_fog_constants(const render_fog *fog); }
using rasterizer_set_fog_constants_impl::rasterizer_set_fog_constants;
namespace rasterizer_set_shader_stage_config_impl { void rasterizer_set_shader_stage_config(int16_t mode); }
using rasterizer_set_shader_stage_config_impl::rasterizer_set_shader_stage_config;
namespace rasterizer_shader_technique_for_name_impl { void * rasterizer_shader_technique_for_name(void *effect, const char *name); }
using rasterizer_shader_technique_for_name_impl::rasterizer_shader_technique_for_name;
uint8_t rasterizer_shader_environment_build_technique_table(void);
void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer);
void rasterizer_shader_environment_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot);
namespace rasterizer_shader_environment_draw_pixel_shader_impl { void rasterizer_shader_environment_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot); }
using rasterizer_shader_environment_draw_pixel_shader_impl::rasterizer_shader_environment_draw_pixel_shader;
namespace rasterizer_shader_environment_draw_single_stream_impl { void rasterizer_shader_environment_draw_single_stream(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot); }
using rasterizer_shader_environment_draw_single_stream_impl::rasterizer_shader_environment_draw_single_stream;
namespace rasterizer_shader_environment_dynamic_mirror_draw_impl { void rasterizer_shader_environment_dynamic_mirror_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_dynamic_mirror_draw_impl::rasterizer_shader_environment_dynamic_mirror_draw;
namespace rasterizer_shader_environment_lightmap_draw_impl { void rasterizer_shader_environment_lightmap_draw(uint8_t *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer); }
using rasterizer_shader_environment_lightmap_draw_impl::rasterizer_shader_environment_lightmap_draw;
namespace rasterizer_shader_environment_lightmap_draw_single_stream_impl { void rasterizer_shader_environment_lightmap_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_lightmap_draw_single_stream_impl::rasterizer_shader_environment_lightmap_draw_single_stream;
namespace rasterizer_shader_environment_lightmap_draw_two_stream_impl { void rasterizer_shader_environment_lightmap_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_lightmap_draw_two_stream_impl::rasterizer_shader_environment_lightmap_draw_two_stream;
namespace rasterizer_shader_environment_lightmap_specular_draw_impl { void rasterizer_shader_environment_lightmap_specular_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_lightmap_specular_draw_impl::rasterizer_shader_environment_lightmap_specular_draw;
namespace rasterizer_shader_environment_projected_light_draw_impl { void rasterizer_shader_environment_projected_light_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_projected_light_draw_impl::rasterizer_shader_environment_projected_light_draw;
namespace rasterizer_shader_environment_reflection_draw_impl { void rasterizer_shader_environment_reflection_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_reflection_draw_impl::rasterizer_shader_environment_reflection_draw;
void rasterizer_shader_environment_select_draw_functions(void);
namespace rasterizer_shader_environment_self_illumination_draw_impl { void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_self_illumination_draw_impl::rasterizer_shader_environment_self_illumination_draw;
namespace rasterizer_shader_environment_self_illumination_draw_single_stream_impl { void rasterizer_shader_environment_self_illumination_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_self_illumination_draw_single_stream_impl::rasterizer_shader_environment_self_illumination_draw_single_stream;
namespace rasterizer_shader_environment_self_illumination_draw_two_stream_impl { void rasterizer_shader_environment_self_illumination_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); }
using rasterizer_shader_environment_self_illumination_draw_two_stream_impl::rasterizer_shader_environment_self_illumination_draw_two_stream;
void rasterizer_shader_environment_set_lightmap(BitmapData *lightmap);
namespace rasterizer_shader_environment_technique_draw_impl { void rasterizer_shader_environment_technique_draw(rasterizer_vertex_buffer *vertex_buffer, const ShaderEnvironment *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count); }
using rasterizer_shader_environment_technique_draw_impl::rasterizer_shader_environment_technique_draw;
namespace rasterizer_shader_environment_technique_multipurpose_set_states_impl { void rasterizer_shader_environment_technique_multipurpose_set_states(void); }
using rasterizer_shader_environment_technique_multipurpose_set_states_impl::rasterizer_shader_environment_technique_multipurpose_set_states;
namespace rasterizer_shader_environment_technique_ps2_set_states_impl { void rasterizer_shader_environment_technique_ps2_set_states(void); }
using rasterizer_shader_environment_technique_ps2_set_states_impl::rasterizer_shader_environment_technique_ps2_set_states;
namespace rasterizer_shader_environment_technique_self_illumination_set_states_impl { void rasterizer_shader_environment_technique_self_illumination_set_states(void); }
using rasterizer_shader_environment_technique_self_illumination_set_states_impl::rasterizer_shader_environment_technique_self_illumination_set_states;
int rasterizer_transparent_decals_enabled(void);
transparent_geometry_group * rasterizer_transparent_geometry_group_build(transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot, const real_point3d *position);
void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached);
namespace rasterizer_transparent_geometry_group_draw_active_camouflage_impl { void rasterizer_transparent_geometry_group_draw_active_camouflage(transparent_geometry_group *group); }
using rasterizer_transparent_geometry_group_draw_active_camouflage_impl::rasterizer_transparent_geometry_group_draw_active_camouflage;
void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag);
void rasterizer_transparent_geometry_group_new(Shader *shader, int16_t shader_permutation, uint32_t lightmap_bitmap, uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count, uint32_t vertex_buffer, ColorARGB *tint, uint32_t lighting, uint32_t flags, real_point3d *world_position);
void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot, int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags, real_point3d *world_position, Shader *shader);
transparent_geometry_group * transparent_geometry_group_allocate(void);
transparent_geometry_group * transparent_geometry_group_allocate_secondary(void);
int transparent_geometry_group_compare(int16_t *a, int16_t *b);
void transparent_geometry_group_draw_all(uint8_t resort);
transparent_geometry_group * transparent_geometry_group_get_next_sorted(transparent_geometry_group *group);
uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group);
int32_t transparent_geometry_group_index_from_pointer(transparent_geometry_group *group);
void transparent_geometry_group_set_drawn_bit(transparent_geometry_group *group, uint8_t clear);
void transparent_geometry_group_sort(void);
uint8_t transparent_geometry_group_test_drawn_bit(transparent_geometry_group *group);
int32_t transparent_geometry_pool_initialize(void);

}  // namespace halo::rasterizer
