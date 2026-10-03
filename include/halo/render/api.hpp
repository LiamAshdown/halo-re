/**
 * @file include/halo/render/api.hpp
 * Functions of the render module that other modules and the data tables call (namespace halo::render). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct real_matrix4x3;

struct BitmapData;
struct ColorARGB;
struct Contrail;
struct Point2DInt;
struct billboard_basis;
struct build_sprite_data;
struct cinematic_screen_effect_globals;
struct contrail;
struct object;
struct object_render_data;
struct rasterizer_frame_statistics;
struct rasterizer_frame_time;
struct real_point2d;
struct real_point3d;
struct real_rectangle3d;
struct real_vector3d;
struct render_camera;
struct render_frustum;
struct render_lighting;
struct render_model_effect;
struct render_view;
struct rendered_particle_datum;
struct rendered_particle_range;
struct rendered_particle_datum;
struct structure_bsp_mirror_result;
typedef uint32_t datum_index;
typedef float real;

namespace halo::render {

/**
 * The engine globals the render module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    real_matrix4x3 &camera_world_to_view;
    int16_t &viewport_left;
    int16_t &viewport_bottom;
    int16_t &viewport_right;
    float &time_since_frame;
    int16_t &force_flag;
};

Globals &globals();

void billboard_system_frame_init(void);
void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode, real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
int16_t build_sprite_get_group(build_sprite_data *data, BitmapData *bitmap);
void build_sprite_rotational(build_sprite_data *data, uint32_t flags, int16_t first_sequence_index, int16_t sprite_index, real_point3d *origin, real_vector3d *axis, float rotation, float scale, ColorARGB *color, float fade);
void build_sprites_end(build_sprite_data *data);
void chimera__render_camera_build_frustum(float *frustum_bounds, render_camera *camera, render_frustum *frustum, uint8_t build_projection);
float cinematic_screen_effect_get_script_value(int16_t index);
void cinematic_screen_effect_set_convolution(int16_t convolution_type, int16_t extra_passes, float radius_lower_bound, float radius_upper_bound, float duration);
void cinematic_screen_effect_set_filter(float light_enhancement_lower, float light_enhancement_upper, float desaturation_lower, float desaturation_upper, uint8_t is_additive, float duration);
void cinematic_screen_effect_set_video(int16_t overbright_mode, float noise_intensity);
cinematic_screen_effect_globals * cinematic_screen_effect_update(cinematic_screen_effect_globals *input);
real contrail_compute_edge_fade_factor(real_vector3d *direction, real_point3d *point, int16_t fade_mode, uint8_t *flags);
void fg_add_sample(int32_t index, float sample);
void fg_render(uint8_t render_graph, uint8_t render_infos);
real object_compute_level_of_detail_pixels(datum_index object_index);
render_lighting * object_get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels);
datum_index object_get_cached_render_state(datum_index object_index, real level_of_detail_pixels);
void object_render_state_refresh(datum_index cache_index, datum_index object_index, real level_of_detail_pixels, uint8_t full_sample);
void * rasterizer_dynamic_index_slot_lock(int32_t slot_index);
void rasterizer_effect_slot_release_active(void);
void rasterizer_frame_statistics_draw(void);
void rasterizer_frame_statistics_graph_init(void);
void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics, uint8_t dropped);
uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index, int16_t bitmap_index);
void rasterizer_lens_flare_set_vertex_specular(float intensity);
void render_billboard_build_orientation_basis(build_sprite_data *data, int16_t render_type, real_vector3d *position, real_vector3d *normal, billboard_basis *out);
void render_billboard_compute_scale(build_sprite_data *data, float *scale, int16_t render_type, real_point3d *position, BitmapData *bitmap);
real render_billboard_compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type);
uint32_t render_camera_compute_frustum_bounds(render_camera *camera, float bounds_out[4], float bounds_in[4]);
void render_camera_compute_projection_skew(render_camera *camera, float bounds_out[4]);
void render_camera_facing_frame_build(float *out, float distance);
void render_camera_mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror, render_camera *out_camera);
void render_camera_projection_zrange_push_pop_set(render_frustum *frustum, float z_near, float z_far);
void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source);
void render_contrail(contrail *c, Contrail *definition, int16_t instance);
void render_contrails(uint32_t render_type_flags);
int render_device_is_ready(void);
void render_frame(Point2DInt *screenshot_tile, render_view *views, int16_t count, Point2DInt *screenshot_page, float time_since_tick, float time_since_frame);
uint8_t render_frustum_classify_point_side_planes(render_frustum *frustum, real_point3d *point);
real render_frustum_compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum);
void render_frustum_compute_screen_clip_bounds(float out[4], render_frustum *frustum);
int16_t render_frustum_test_bounding_box(render_frustum *frustum, real_rectangle3d *box, uint8_t validate);
int16_t render_frustum_test_sphere(render_frustum *frustum, real_point3d *center, float radius);
uint8_t render_initialize(void);
void render_lighting_disable_workaround(void);
void render_lighting_step_direction_toward(real_vector3d *current, real_vector3d *target, float max_delta);
void render_lighting_step_vector3_toward(float *current, float *target, float max_delta);
void render_lighting_step_vector4_toward(float *current, float *target, float max_delta);
int16_t render_local_player_gunner_seat_visible(int16_t local_player_index);
void render_nonplayer_frame(uint32_t nonplayer, render_view *view);
void render_object(object_render_data *data);
void render_object_get_cull_sphere(datum_index object_index, real_point3d *center, float *radius);
uint8_t render_object_is_camera_unit(datum_index object);
void render_object_list(object_render_data *data, render_model_effect *parent_effect, datum_index object_index);
uint8_t render_object_shadow_begin(object_render_data *data, float fade);
void render_object_shadow_end(object_render_data *data);
void render_object_shadows(object_render_data *data);
void render_objects(void);
void render_objects_collect(void);
void render_particles(void);
void render_player_frame(Point2DInt *screenshot_tile, render_view *view);
void render_pregame_frame(render_view *view);
uint8_t render_project_world_point_to_screen(real_point2d *screen_out, real_point3d *world_point, render_frustum *frustum, render_camera *camera);
int32_t render_rasterizer_dispatch_537800(int32_t slot_index, real_point3d *point, float radius);
void render_sky(void);
void render_sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal, real_vector3d *out_normal, build_sprite_data *data, uint8_t flags, real_point3d *out_position);
void render_window(int16_t local_player_index, render_camera *source_camera, render_frustum *source_frustum, render_camera *rasterizer_camera, render_frustum *rasterizer_frustum, int16_t rasterizer_target, uint8_t has_mirror);
void render_window_structure_lightmap_begin_0x511f90(void *bitmap_data);
void render_window_structure_lightmap_begin_0x512010(void *bitmap_data);
void render_window_structure_material_0x511f40(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x511f50(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x511f70(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x511f80(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x511fe0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x512020(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x512040(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x512070(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_material_0x5120c0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra);
void render_window_structure_transparent_0x512080(void *shader_data, int16_t shader_permutation, void *bitmap, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra, void *rendered_vertices, void *lightmap_vertices, void *coplanar_vector, void *lightmap_vertices_offset, int32_t zero);
void sort_adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom, rendered_particle_datum value, int32_t predicate);
void sort_heap_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void sort_insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void sort_introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last, int32_t ideal, int32_t predicate);
void sort_make_heap(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void sort_median(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last, int32_t predicate);
void sort_median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last, int32_t predicate);
void sort_push_heap(rendered_particle_datum *first, int32_t hole, int32_t top, rendered_particle_datum value, int32_t predicate);
void sort_rotate(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last);
rendered_particle_range * sort_unguarded_partition(rendered_particle_range *result, rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);

}
