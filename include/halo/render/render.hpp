#pragma once

/**
 * C++ API of the render module. The engine headers (tags.h ... render.h) must be included before this one.
 */

typedef struct rendered_particle_range {
    rendered_particle_datum *first;
    rendered_particle_datum *second;
} rendered_particle_range;

namespace halo::render {

/**
 * View over a build_sprite_data record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class SpriteBuilder {
public:
    explicit SpriteBuilder(build_sprite_data *record) : self(record) {}
    int16_t get_group(BitmapData *bitmap);
    void rotational(uint32_t flags, int16_t first_sequence_index, int16_t sprite_index, real_point3d *origin,
    real_vector3d *axis, float rotation, float scale, ColorARGB *color, float fade);
    void billboard_build_orientation_basis(int16_t render_type, real_vector3d *position, real_vector3d *normal,
    billboard_basis *out);
    void billboard_compute_scale(float *scale, int16_t render_type, real_point3d *position, BitmapData *bitmap);
    void sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal, real_vector3d *out_normal,
    uint8_t flags, real_point3d *out_position);

private:
    build_sprite_data *self;
};

}  // namespace halo::render

namespace halo::render {

/**
 * View over a object_render_data record that owns the operations the engine applies to it.
 * Holds a pointer only; the record layout is unchanged.
 */
class ObjectRenderData {
public:
    explicit ObjectRenderData(object_render_data *record) : self(record) {}
    void draw();
    void list(render_model_effect *parent_effect, datum_index object_index);
    uint8_t shadow_begin(float fade);
    void shadow_end();
    void shadows();

private:
    object_render_data *self;
};

}  // namespace halo::render

namespace halo::render::billboard {

void frame_init(void);
real compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type);

}  // namespace halo::render::billboard

namespace halo::render::sprite {

void draw(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode, real_point3d *origin,
    real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
void sprites_end(build_sprite_data *data);

}  // namespace halo::render::sprite

namespace halo::render::camera {

void build_frustum(float *frustum_bounds, render_camera *camera, render_frustum *frustum, uint8_t build_projection);
uint32_t compute_frustum_bounds(render_camera *camera, float bounds_out[4], float bounds_in[4]);
void compute_projection_skew(render_camera *camera, float bounds_out[4]);
void facing_frame_build(float *out, float distance);
void mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror, render_camera *out_camera);
void projection_zrange_push_pop_set(render_frustum *frustum, float z_near, float z_far);

}  // namespace halo::render::camera

namespace halo::render::cinematic_screen_effect {

float get_script_value(int16_t index);
void set_convolution(int16_t convolution_type, int16_t extra_passes, float radius_lower_bound,
    float radius_upper_bound, float duration);
void set_filter(float light_enhancement_lower, float light_enhancement_upper, float desaturation_lower,
    float desaturation_upper, uint8_t is_additive, float duration);
void set_video(int16_t overbright_mode, float noise_intensity);
cinematic_screen_effect_globals *update(cinematic_screen_effect_globals *input);

}  // namespace halo::render::cinematic_screen_effect

namespace halo::render::contrails {

real compute_edge_fade_factor(real_vector3d *direction, real_point3d *point, int16_t fade_mode, uint8_t *flags);
void draw(contrail *c, Contrail *definition, int16_t instance);
void render_all(uint32_t render_type_flags);

}  // namespace halo::render::contrails

namespace halo::render::fg {

void add_sample(int32_t index, float sample);
void draw(uint8_t render_graph, uint8_t render_infos);

}  // namespace halo::render::fg

namespace halo::render::object_cache {

real compute_level_of_detail_pixels(datum_index object_index);
render_lighting *get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels);
datum_index get_cached_render_state(datum_index object_index, real level_of_detail_pixels);
void render_state_refresh(datum_index cache_index, datum_index object_index, real level_of_detail_pixels,
    uint8_t full_sample);

}  // namespace halo::render::object_cache

namespace halo::render::rasterizer {

void *dynamic_index_slot_lock(int32_t slot_index);
void effect_slot_release_active(void);
uint8_t lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index, int16_t bitmap_index);
void lens_flare_set_vertex_specular(float intensity);

}  // namespace halo::render::rasterizer

namespace halo::render::frame_statistics {

void draw(void);
void graph_init(void);
void sample(rasterizer_frame_statistics *statistics, uint8_t dropped);

}  // namespace halo::render::frame_statistics

namespace halo::render::frame {

void cinematic_screen_effect_update(rasterizer_frame_time *time_source);
int device_is_ready(void);
void draw(Point2DInt *screenshot_tile, render_view *views, int16_t count, Point2DInt *screenshot_page,
    float time_since_tick, float time_since_frame);
uint8_t initialize(void);
int16_t local_player_gunner_seat_visible(int16_t local_player_index);
void nonplayer_frame(uint32_t nonplayer, render_view *view);
void particles(void);
void player_frame(Point2DInt *screenshot_tile, render_view *view);
void pregame_frame(render_view *view);
uint8_t project_world_point_to_screen(real_point2d *screen_out, real_point3d *world_point, render_frustum *frustum,
    render_camera *camera);
int32_t rasterizer_dispatch_537800(int32_t slot_index, real_point3d *point, float radius);
void sky(void);
void window(int16_t local_player_index, render_camera *source_camera, render_frustum *source_frustum,
    render_camera *rasterizer_camera, render_frustum *rasterizer_frustum, int16_t rasterizer_target,
    uint8_t has_mirror);

}  // namespace halo::render::frame

namespace halo::render::frustum {

uint8_t classify_point_side_planes(render_frustum *frustum, real_point3d *point);
real compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum);
void compute_screen_clip_bounds(float out[4], render_frustum *frustum);
int16_t test_bounding_box(render_frustum *frustum, real_rectangle3d *box, uint8_t validate);
int16_t test_sphere(render_frustum *frustum, real_point3d *center, float radius);

}  // namespace halo::render::frustum

namespace halo::render::lighting {

void disable_workaround(void);
void step_direction_toward(real_vector3d *current, real_vector3d *target, float max_delta);
void step_vector3_toward(float *current, float *target, float max_delta);
void step_vector4_toward(float *current, float *target, float max_delta);

}  // namespace halo::render::lighting

namespace halo::render::object_pass {

void _get_cull_sphere(datum_index object_index, real_point3d *center, float *radius);
uint8_t _is_camera_unit(datum_index object);
void s(void);
void s_collect(void);

}  // namespace halo::render::object_pass

namespace halo::render::window_structure {

void lightmap_begin_0x511f90(void *bitmap_data);
void lightmap_begin_0x512010(void *bitmap_data);
void material_0x511f40(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x511f50(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x511f70(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x511f80(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x511fe0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x512020(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x512040(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x512070(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void material_0x5120c0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra);
void transparent_0x512080(void *shader_data, int16_t shader_permutation, void *bitmap, int32_t render_context,
    int32_t surface_offset, int16_t surface_count, void *material_extra, void *rendered_vertices,
    void *lightmap_vertices, void *coplanar_vector, void *lightmap_vertices_offset, int32_t zero);

}  // namespace halo::render::window_structure

namespace halo::render::particle_sort {

void adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom, rendered_particle_datum value,
    int32_t predicate);
void heap_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last, int32_t ideal, int32_t predicate);
void make_heap(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate);
void median(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last,
    int32_t predicate);
void median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last,
    int32_t predicate);
void push_heap(rendered_particle_datum *first, int32_t hole, int32_t top, rendered_particle_datum value,
    int32_t predicate);
void rotate(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last);
rendered_particle_range *unguarded_partition(rendered_particle_range *result, rendered_particle_datum *first,
    rendered_particle_datum *last, int32_t predicate);

}  // namespace halo::render::particle_sort

