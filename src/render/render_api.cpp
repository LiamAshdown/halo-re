#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/render/api.hpp"
#include "halo/core/link.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/rasterizer/api.hpp"

static auto &render_camera_world_to_view = halo::link::ref<real_matrix4x3>(halo::render::vars().render_camera_world_to_view);
static auto &render_viewport_left = halo::link::ref<int16_t>(halo::render::vars().render_viewport_left);
static auto &render_viewport_bottom = halo::link::ref<int16_t>(halo::rasterizer::vars().render_viewport_bottom);
static auto &render_viewport_right = halo::link::ref<int16_t>(halo::render::vars().render_viewport_right);
static auto &render_time_since_frame = halo::link::ref<float>(halo::render::vars().render_time_since_frame);
static auto &render_force_flag = halo::link::ref<int16_t>(halo::rasterizer::vars().render_force_flag);

namespace halo::render {

Globals &globals()
{
    static Globals instance{::render_camera_world_to_view, ::render_viewport_left, ::render_viewport_bottom, ::render_viewport_right, ::render_time_since_frame, ::render_force_flag};
    return instance;
}

void billboard_system_frame_init(void)
{
    halo::render::billboard::frame_init();
}

void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode,
    real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade,
    uint32_t flags)
{
    halo::render::sprite::draw(data, sequence_index, sprite_index, mode, origin, direction, rotation, scale, color, fade, flags);
}

int16_t build_sprite_get_group(build_sprite_data *data, BitmapData *bitmap)
{
    return halo::render::SpriteBuilder(data).get_group(bitmap);
}

void build_sprite_rotational(build_sprite_data *data, uint32_t flags, int16_t first_sequence_index,
    int16_t sprite_index, real_point3d *origin, real_vector3d *axis, float rotation, float scale, ColorARGB *color,
    float fade)
{
    halo::render::SpriteBuilder(data).rotational(flags, first_sequence_index, sprite_index, origin, axis, rotation, scale, color, fade);
}

void build_sprites_end(build_sprite_data *data)
{
    halo::render::sprite::sprites_end(data);
}

void chimera__render_camera_build_frustum(float *frustum_bounds, render_camera *camera,
    render_frustum *frustum, uint8_t build_projection)
{
    halo::render::camera::build_frustum(frustum_bounds, camera, frustum, build_projection);
}

float cinematic_screen_effect_get_script_value(int16_t index)
{
    return halo::render::cinematic_screen_effect::get_script_value(index);
}

void cinematic_screen_effect_set_convolution(int16_t convolution_type, int16_t extra_passes,
    float radius_lower_bound, float radius_upper_bound, float duration)
{
    halo::render::cinematic_screen_effect::set_convolution(convolution_type, extra_passes, radius_lower_bound, radius_upper_bound, duration);
}

void cinematic_screen_effect_set_filter(float light_enhancement_lower, float light_enhancement_upper,
    float desaturation_lower, float desaturation_upper, uint8_t is_additive, float duration)
{
    halo::render::cinematic_screen_effect::set_filter(light_enhancement_lower, light_enhancement_upper, desaturation_lower, desaturation_upper, is_additive, duration);
}

void cinematic_screen_effect_set_video(int16_t overbright_mode, float noise_intensity)
{
    halo::render::cinematic_screen_effect::set_video(overbright_mode, noise_intensity);
}

cinematic_screen_effect_globals *cinematic_screen_effect_update(cinematic_screen_effect_globals *input)
{
    return halo::render::cinematic_screen_effect::update(input);
}

real contrail_compute_edge_fade_factor(real_vector3d *direction, real_point3d *point, int16_t fade_mode,
    uint8_t *flags)
{
    return halo::render::contrails::compute_edge_fade_factor(direction, point, fade_mode, flags);
}

void fg_add_sample(int32_t index, float sample)
{
    halo::render::fg::add_sample(index, sample);
}

void fg_render(uint8_t render_graph, uint8_t render_infos)
{
    halo::render::fg::draw(render_graph, render_infos);
}

real object_compute_level_of_detail_pixels(datum_index object_index)
{
    return halo::render::object_cache::compute_level_of_detail_pixels(object_index);
}

render_lighting *object_get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels)
{
    return halo::render::object_cache::get_cached_render_lighting(object_index, level_of_detail_pixels);
}

datum_index object_get_cached_render_state(datum_index object_index, real level_of_detail_pixels)
{
    return halo::render::object_cache::get_cached_render_state(object_index, level_of_detail_pixels);
}

void object_render_state_refresh(datum_index cache_index, datum_index object_index,
    real level_of_detail_pixels, uint8_t full_sample)
{
    halo::render::object_cache::render_state_refresh(cache_index, object_index, level_of_detail_pixels, full_sample);
}

void *rasterizer_dynamic_index_slot_lock(int32_t slot_index)
{
    return halo::render::rasterizer::dynamic_index_slot_lock(slot_index);
}

void rasterizer_effect_slot_release_active(void)
{
    halo::render::rasterizer::effect_slot_release_active();
}

void rasterizer_frame_statistics_draw(void)
{
    halo::render::frame_statistics::draw();
}

void rasterizer_frame_statistics_graph_init(void)
{
    halo::render::frame_statistics::graph_init();
}

void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics, uint8_t dropped)
{
    halo::render::frame_statistics::sample(statistics, dropped);
}

uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index,
    int16_t bitmap_index)
{
    return halo::render::rasterizer::lens_flare_set_current_key(second_bitmap_tag_index, bitmap_tag_index, bitmap_index);
}

void rasterizer_lens_flare_set_vertex_specular(float intensity)
{
    halo::render::rasterizer::lens_flare_set_vertex_specular(intensity);
}

void render_billboard_build_orientation_basis(build_sprite_data *data, int16_t render_type,
    real_vector3d *position, real_vector3d *normal, billboard_basis *out)
{
    halo::render::SpriteBuilder(data).billboard_build_orientation_basis(render_type, position, normal, out);
}

void render_billboard_compute_scale(build_sprite_data *data, float *scale, int16_t render_type,
    real_point3d *position, BitmapData *bitmap)
{
    halo::render::SpriteBuilder(data).billboard_compute_scale(scale, render_type, position, bitmap);
}

real render_billboard_compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type)
{
    return halo::render::billboard::compute_view_fade(a, b, render_type);
}

uint32_t render_camera_compute_frustum_bounds(render_camera *camera, float bounds_out[4],
    float bounds_in[4])
{
    return halo::render::camera::compute_frustum_bounds(camera, bounds_out, bounds_in);
}

void render_camera_compute_projection_skew(render_camera *camera, float bounds_out[4])
{
    halo::render::camera::compute_projection_skew(camera, bounds_out);
}

void render_camera_facing_frame_build(float *out, float distance)
{
    halo::render::camera::facing_frame_build(out, distance);
}

void render_camera_mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror,
    render_camera *out_camera)
{
    halo::render::camera::mirror(source_camera, mirror, out_camera);
}

void render_camera_projection_zrange_push_pop_set(render_frustum *frustum, float z_near, float z_far)
{
    halo::render::camera::projection_zrange_push_pop_set(frustum, z_near, z_far);
}

void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source)
{
    halo::render::frame::cinematic_screen_effect_update(time_source);
}

void render_contrail(contrail *c, Contrail *definition, int16_t instance)
{
    halo::render::contrails::draw(c, definition, instance);
}

void render_contrails(uint32_t render_type_flags)
{
    halo::render::contrails::render_all(render_type_flags);
}

int render_device_is_ready(void)
{
    return halo::render::frame::device_is_ready();
}

void render_frame(Point2DInt *screenshot_tile, render_view *views, int16_t count,
    Point2DInt *screenshot_page, float time_since_tick, float time_since_frame)
{
    halo::render::frame::draw(screenshot_tile, views, count, screenshot_page, time_since_tick, time_since_frame);
}

uint8_t render_frustum_classify_point_side_planes(render_frustum *frustum, real_point3d *point)
{
    return halo::render::frustum::classify_point_side_planes(frustum, point);
}

real render_frustum_compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum)
{
    return halo::render::frustum::compute_box_overlap_area(box, frustum);
}

void render_frustum_compute_screen_clip_bounds(float out[4], render_frustum *frustum)
{
    halo::render::frustum::compute_screen_clip_bounds(out, frustum);
}

int16_t render_frustum_test_bounding_box(render_frustum *frustum, real_rectangle3d *box, uint8_t validate)
{
    return halo::render::frustum::test_bounding_box(frustum, box, validate);
}

int16_t render_frustum_test_sphere(render_frustum *frustum, real_point3d *center, float radius)
{
    return halo::render::frustum::test_sphere(frustum, center, radius);
}

uint8_t render_initialize(void)
{
    return halo::render::frame::initialize();
}

void render_lighting_disable_workaround(void)
{
    halo::render::lighting::disable_workaround();
}

void render_lighting_step_direction_toward(real_vector3d *current, real_vector3d *target, float max_delta)
{
    halo::render::lighting::step_direction_toward(current, target, max_delta);
}

void render_lighting_step_vector3_toward(float *current, float *target, float max_delta)
{
    halo::render::lighting::step_vector3_toward(current, target, max_delta);
}

void render_lighting_step_vector4_toward(float *current, float *target, float max_delta)
{
    halo::render::lighting::step_vector4_toward(current, target, max_delta);
}

int16_t render_local_player_gunner_seat_visible(int16_t local_player_index)
{
    return halo::render::frame::local_player_gunner_seat_visible(local_player_index);
}

void render_nonplayer_frame(uint32_t nonplayer, render_view *view)
{
    halo::render::frame::nonplayer_frame(nonplayer, view);
}

void render_object(object_render_data *data)
{
    halo::render::ObjectRenderData(data).draw();
}

void render_object_get_cull_sphere(datum_index object_index, real_point3d *center, float *radius)
{
    halo::render::object_pass::_get_cull_sphere(object_index, center, radius);
}

uint8_t render_object_is_camera_unit(datum_index object)
{
    return halo::render::object_pass::_is_camera_unit(object);
}

void render_object_list(object_render_data *data, render_model_effect *parent_effect,
    datum_index object_index)
{
    halo::render::ObjectRenderData(data).list(parent_effect, object_index);
}

uint8_t render_object_shadow_begin(object_render_data *data, float fade)
{
    return halo::render::ObjectRenderData(data).shadow_begin(fade);
}

void render_object_shadow_end(object_render_data *data)
{
    halo::render::ObjectRenderData(data).shadow_end();
}

void render_object_shadows(object_render_data *data)
{
    halo::render::ObjectRenderData(data).shadows();
}

void render_objects(void)
{
    halo::render::object_pass::s();
}

void render_objects_collect(void)
{
    halo::render::object_pass::s_collect();
}

void render_particles(void)
{
    halo::render::frame::particles();
}

void render_player_frame(Point2DInt *screenshot_tile, render_view *view)
{
    halo::render::frame::player_frame(screenshot_tile, view);
}

void render_pregame_frame(render_view *view)
{
    halo::render::frame::pregame_frame(view);
}

uint8_t render_project_world_point_to_screen(real_point2d *screen_out, real_point3d *world_point,
    render_frustum *frustum, render_camera *camera)
{
    return halo::render::frame::project_world_point_to_screen(screen_out, world_point, frustum, camera);
}

int32_t render_rasterizer_dispatch_537800(int32_t slot_index, real_point3d *point, float radius)
{
    return halo::render::frame::rasterizer_dispatch_537800(slot_index, point, radius);
}

void render_sky(void)
{
    halo::render::frame::sky();
}

void render_sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal,
    real_vector3d *out_normal, build_sprite_data *data, uint8_t flags, real_point3d *out_position)
{
    halo::render::SpriteBuilder(data).sprite_transform_point_and_normal(position, normal, out_normal, flags, out_position);
}

void render_window(int16_t local_player_index, render_camera *source_camera,
    render_frustum *source_frustum, render_camera *rasterizer_camera, render_frustum *rasterizer_frustum,
    int16_t rasterizer_target, uint8_t has_mirror)
{
    halo::render::frame::window(local_player_index, source_camera, source_frustum, rasterizer_camera, rasterizer_frustum, rasterizer_target, has_mirror);
}

void render_window_structure_lightmap_begin_0x511f90(void *bitmap_data)
{
    halo::render::window_structure::lightmap_begin_0x511f90(bitmap_data);
}

void render_window_structure_lightmap_begin_0x512010(void *bitmap_data)
{
    halo::render::window_structure::lightmap_begin_0x512010(bitmap_data);
}

void render_window_structure_material_0x511f40(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x511f40(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x511f50(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x511f50(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x511f70(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x511f70(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x511f80(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x511f80(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x511fe0(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x511fe0(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x512020(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x512020(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x512040(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x512040(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x512070(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x512070(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_material_0x5120c0(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    halo::render::window_structure::material_0x5120c0(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

void render_window_structure_transparent_0x512080(void *shader_data, int16_t shader_permutation,
    void *bitmap, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra,
    void *rendered_vertices, void *lightmap_vertices, void *coplanar_vector, void *lightmap_vertices_offset,
    int32_t zero)
{
    halo::render::window_structure::transparent_0x512080(shader_data, shader_permutation, bitmap, render_context, surface_offset, surface_count, material_extra, rendered_vertices, lightmap_vertices, coplanar_vector, lightmap_vertices_offset, zero);
}

void sort_adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom,
    rendered_particle_datum value, int32_t predicate)
{
    halo::render::particle_sort::adjust_heap(first, hole, bottom, value, predicate);
}

void sort_heap_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    halo::render::particle_sort::heap_sort(first, last, predicate);
}

void sort_insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    halo::render::particle_sort::insertion_sort(first, last, predicate);
}

void sort_introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last, int32_t ideal,
    int32_t predicate)
{
    halo::render::particle_sort::introsort_loop(first, last, ideal, predicate);
}

void sort_make_heap(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    halo::render::particle_sort::make_heap(first, last, predicate);
}

void sort_median(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last, int32_t predicate)
{
    halo::render::particle_sort::median(first, mid, last, predicate);
}

void sort_median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last, int32_t predicate)
{
    halo::render::particle_sort::median_of_three(first, mid, last, predicate);
}

void sort_push_heap(rendered_particle_datum *first, int32_t hole, int32_t top,
    rendered_particle_datum value, int32_t predicate)
{
    halo::render::particle_sort::push_heap(first, hole, top, value, predicate);
}

void sort_rotate(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last)
{
    halo::render::particle_sort::rotate(first, mid, last);
}

rendered_particle_range *sort_unguarded_partition(rendered_particle_range *result,
    rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    return halo::render::particle_sort::unguarded_partition(result, first, last, predicate);
}

}
