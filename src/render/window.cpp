#include "crt.h"
#include "win32.h"
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
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern uint8_t render_cluster_has_sky;
extern int16_t render_cluster_sky_index;
extern Scenario *global_scenario;
extern float render_time_since_frame;
extern float sky_animation_times[9];
extern render_camera render_camera_global;
extern real_point3d *global_zero_vector3d_pointer;
extern real_matrix4x3 *k_render_identity_matrix_ptr;
extern ColorRGB *global_white_color;
extern uint8_t console_debug_toggle_6893ec;
extern uint8_t rasterizer_render_states_dirty;
extern uint8_t unknown_0071d1fa;
extern uint32_t rasterizer_device_version;
extern void *rasterizer_device;
extern void model_nodes_get_default_transforms(GBXModel *model, void *nodes);
extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
    void *out_orientations);
extern void model_nodes_build_matrices(real_point3d *position, real_vector3d *forward, GBXModel *model,
    real_matrix4x3 *matrices, void *nodes, real_vector3d *up);
extern int16_t model_markers_get_by_name(datum_index model_tag, const char *name, uint8_t *permutations,
    uint32_t reserved, real_matrix4x3 *node_matrices, uint32_t flags, object_marker *out, int32_t maximum_count);
extern void light_transient_add(datum_index light_tag, ColorRGB *color, real_point3d *position,
    real_vector3d *direction, real_vector3d *up, float intensity);
extern void render_model(TagID model_tag_id, void *node_matrices, float level_of_detail_pixels,
    uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting,
    real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index,
    uint16_t forced_shader_permutation, uint32_t flags);
extern double fmod(double x, double y);
extern double sqrt(double x);
extern double fabs(double x);
extern double sin(double x);
extern double cos(double x);
extern int32_t render_window_count;
extern int16_t current_local_player_index;
extern int16_t render_window_index;
extern render_frustum render_frustum_global;
extern render_fog render_fog_state;
extern rasterizer_frame_statistics rasterizer_frame_statistics_state;
extern rasterizer_window_parameters rasterizer_window;
extern uint8_t console_debug_toggle_69c614;
extern int16_t console_debug_toggle_6893e4;
extern uint8_t decals_for_all_responses;
extern int16_t visible_cluster_count;
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters];
extern int16_t visible_surface_count;
extern int32_t visible_surface_indices[0x4000];
extern uint8_t picked_surfaces_valid;
extern int32_t picked_surfaces_geometry;
extern ScenarioStructureBSP *global_structure_bsp;
extern int16_t render_force_flag;
extern uint32_t rasterizer_active_environment_effect;
extern int32_t transparent_geometry_group_last_drawn_key;
extern uint8_t rasterizer_secondary_groups_drawn;
extern int16_t rasterizer_decal_layer;
extern d3d_caps9 rasterizer_caps;
extern game_engine_definition *current_game_engine;
extern void structure_bsp_cluster_visibility_update(void);
extern void player_effect_build_screen_flash(render_screen_flash *out, int16_t local_player_index);
extern void rasterizer_begin_frame(rasterizer_window_parameters *source);
extern void first_person_weapon_update_zoom_static_tint(uint8_t enabled);
extern void first_person_weapon_update_active_state(void);
extern void object_lights_update_all(void);
extern void structure_picked_polygon_refresh(void);
extern void structure_picked_polygon_draw(void);
extern void lens_flare_update_samples(void);
extern void lights_apply_spot_falloff(void);
extern void lights_apply_spot_falloff_specular(void);
extern void rasterizer_decal_pass_begin(int16_t stage);
extern void rasterizer_decals_draw_cluster(int16_t cluster_index);
extern void rasterizer_end_decal_pass(void);
extern void rasterizer_fog_screen_overlay_set_states(void);
extern void rasterizer_force_bilinear_filtering(void);
extern void rasterizer_dynamic_light_technique_ps2_set_states(void);
extern void rasterizer_shader_environment_technique_multipurpose_set_states(void);
extern void rasterizer_shader_environment_technique_self_illumination_set_states(void);
extern void rasterizer_shader_decal_pass_set_states(void);
extern void rasterizer_water_fade_compute_and_set_states(void);
extern void rasterizer_set_shader_stage_config(int16_t mode);
extern void structure_leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin,
    structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices,
    int16_t surface_index_count);
extern void weather_update_local_player(void);
extern void particle_systems_render(void);
extern void transparent_geometry_group_draw_all(uint8_t resort);
extern void detail_objects_update_render_list(void);
extern void lens_flare_render_all(void);
extern void first_person_weapon_update_screen_effects(void);
extern void rasterizer_screen_flash_render(void);
extern void widget_draw_fullscreen_region(int16_t controller_index);
extern void function_do_nothing(void);
extern uint8_t console_debug_toggle_6893f7;
extern void *rasterizer_lightmap_bitmap;
extern uint8_t rasterizer_lightmap_bitmap_missing;
extern void rasterizer_shader_environment_set_lightmap(void *lightmap);
extern void *unknown_007c0494;
extern void rasterizer_object_shadow_structure_draw(void *vertex_buffer, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count);
extern void *unknown_007c0490;
extern void rasterizer_shader_environment_projected_light_draw(void *shader, int16_t frame,
    int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer);
extern void rasterizer_shader_environment_lightmap_specular_draw(void *shader, int16_t frame,
    int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer);
extern void rasterizer_shader_environment_technique_draw(void *vertex_buffer, void *shader,
    int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count);
extern void rasterizer_shader_environment_dynamic_mirror_draw(void *shader, int16_t frame, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count, void *vertex_buffer);
extern void rasterizer_shader_environment_reflection_draw(void *shader, int16_t frame, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count, void *vertex_buffer);
extern void rasterizer_water_ripple_draw(void *vertex_buffer, void *shader, int32_t dynamic_index_slot,
    int32_t first_primitive, int32_t primitive_count);
extern void rasterizer_transparent_geometry_group_new(void *shader, int16_t shader_permutation,
    uint32_t lightmap_bitmap, uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count,
    uint32_t vertex_buffer, void *tint, uint32_t lighting, uint32_t flags, void *world_position);
}

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

/**
 * Returns the raw tag id value of a TagID.
 */
static datum_index tag_id_of(TagID id)
{
    return *(datum_index *)&id;
}

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);

/**
 * Calls IDirect3DDevice::SetRenderState through the device method table.
 */
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, state, value);
}

/**
 * Draws the decals of every visible cluster of the current window.
 */
static void draw_visible_cluster_decals(void)
{
    int16_t i;

    for (i = 0; i < visible_cluster_count; i++) {
        rasterizer_decals_draw_cluster((int16_t)(uint16_t)visible_clusters[i].cluster_index);
    }
}

/**
 * The fog and depth bias reset that closes the stage 1 and stage 4 decal passes.
 */
static void reset_decal_fog_and_depth_bias(void)
{
    set_render_state(0x1c, 0);
    if ((rasterizer_caps.raster_caps & 0x04000000) != 0) {
        set_render_state(0xc3, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x02000000) != 0) {
        set_render_state(0xaf, 0);
    }
    if (rasterizer_decal_layer == 3) {
        rasterizer_set_shader_stage_config(2);
    }
}

/**
 * Runs one structure rendering pass, calling the lightmap-begin and material callbacks for each visible lightmap and shader.
 */
static void structure_pass(structure_lightmap_begin_callback lightmap_begin,
                           structure_material_callback material_cb,
                           structure_lightmap_end_callback lightmap_end,
                           structure_transparent_material_callback transparent_material_cb)
{
    structure_leaf_faces_for_each(picked_surfaces_geometry, lightmap_begin, material_cb,
                                  lightmap_end, transparent_material_cb, visible_surface_indices,
                                  visible_surface_count);
}

namespace halo::render::frame {

/**
 * Animates and draws the sky model of the current render cluster (scaled to 1/1024 around the
 * camera) and registers one transient light per sky light that has a lens flare, placed 1023.875
 * units away along the light direction.
 *
 * @address 0x00510c50
 */
void sky(void)
{
    datum_index sky_tag;
    Sky *sky;
    GBXModel *model;
    uint8_t nodes[0x800];
    real_matrix4x3 matrices[64];
    float function_values[8];
    render_lighting lighting;
    real_matrix4x3 sky_transform;
    int16_t i;

    if (!render_cluster_has_sky) {
        return;
    }
    sky_tag = 0xffffffff;
    if (render_cluster_sky_index >= 0 &&
        (int32_t)render_cluster_sky_index < (int32_t)global_scenario->skies.count) {
        sky_tag = tag_id_of(((ScenarioSky *)global_scenario->skies.pointer)[render_cluster_sky_index].sky.tag_id);
    }
    sky = 0;
    if (sky_tag != 0xffffffff) {
        sky = (Sky *)halo::cache::globals().tag_instances[(uint16_t)sky_tag].data;
    }
    model = (GBXModel *)halo::cache::globals().tag_instances[sky->model.tag_id.index].data;
    model_nodes_get_default_transforms(model, nodes);

    if (tag_id_of(sky->animation_graph.tag_id) != 0xffffffff) {
        ModelAnimations *graph =
            (ModelAnimations *)halo::cache::globals().tag_instances[sky->animation_graph.tag_id.index].data;

        for (i = 0; (int32_t)i < (int32_t)sky->animations.count; i++) {
            SkyAnimation *entry = &((SkyAnimation *)sky->animations.pointer)[i];

            if (entry->animation_index >= 0 &&
                (int32_t)entry->animation_index < (int32_t)graph->animations.count &&
                entry->period != 0.0f) {
                ModelAnimationsAnimation *animation =
                    &((ModelAnimationsAnimation *)graph->animations.pointer)[i];

                if ((int32_t)(int16_t)animation->node_count == (int32_t)model->nodes.count) {
                    float time = (float)fmod(render_time_since_frame / entry->period +
                                             sky_animation_times[i], 1.0);

                    sky_animation_times[i] = time;
                    animation_overlay_interpolated_frame_orientations(animation,
                        (float)(int32_t)(int16_t)animation->frame_count * time, nodes);
                }
            }
        }
    }

    model_nodes_build_matrices(global_zero_vector3d_pointer, halo::math::globals().global_forward3d_pointer, model, matrices, nodes, halo::math::globals().global_up3d_pointer);

    for (i = 0; (int32_t)i < (int32_t)sky->shader_functions.count; i++) {
        function_values[i] = 1.0f;
    }

    for (i = 0; (int32_t)i < (int32_t)sky->lights.count; i++) {
        SkyLight *light = &((SkyLight *)sky->lights.pointer)[i];
        real_vector3d direction;
        real_point3d position;
        real_vector3d toward_camera;
        real_vector3d up;
        real length;

        if (tag_id_of(light->lens_flare.tag_id) == 0xffffffff) {
            continue;
        }
        if (light->lens_flare_marker_name.string[0] == '\0') {
            real cos_pitch = (real)cos(light->direction.pitch);

            direction.i = (real)cos(light->direction.yaw) * cos_pitch;
            direction.j = (real)sin(light->direction.yaw) * cos_pitch;
            direction.k = (real)sin(light->direction.pitch);
        } else {
            object_marker marker;

            if (model_markers_get_by_name(tag_id_of(sky->model.tag_id), light->lens_flare_marker_name.string, 0, 0,
                                          matrices, 0, &marker, 1) == 0) {
                continue;
            }
            direction.i = marker.node_transform.position.x - render_camera_global.position.x;
            direction.j = marker.node_transform.position.y - render_camera_global.position.y;
            direction.k = marker.node_transform.position.z - render_camera_global.position.z;
            length = (real)sqrt(direction.k * direction.k + direction.j * direction.j +
                                direction.i * direction.i);
            if (fabs(length) >= 0.0001) {
                real inverse = 1.0f / length;

                direction.i = inverse * direction.i;
                direction.j = direction.j * inverse;
                direction.k = direction.k * inverse;
            }
        }

        position.x = direction.i * 1023.875f + render_camera_global.position.x;
        position.y = 1023.875f * direction.j + render_camera_global.position.y;
        position.z = direction.k * 1023.875f + render_camera_global.position.z;
        toward_camera.i = -direction.i;
        toward_camera.j = -direction.j;
        toward_camera.k = -direction.k;
        halo::math::vector3d_build_perpendicular(up, toward_camera);
        length = (real)sqrt(up.k * up.k + up.j * up.j + up.i * up.i);
        if (fabs(length) >= 0.0001) {
            real inverse = 1.0f / length;

            up.i = up.i * inverse;
            up.j = up.j * inverse;
            up.k = up.k * inverse;
        }
        light_transient_add(tag_id_of(light->lens_flare.tag_id), global_white_color, &position,
                            &toward_camera, &up, 1.0f);
    }

    sky_transform = *k_render_identity_matrix_ptr;
    sky_transform.position.x = render_camera_global.position.x * 0.99902344f;
    sky_transform.position.y = render_camera_global.position.y * 0.99902344f;
    sky_transform.position.z = render_camera_global.position.z * 0.99902344f;
    sky_transform.scale = 0.0009765625f;
    for (i = 0; (int32_t)i < (int32_t)model->nodes.count; i++) {
        halo::math::globals().matrix4x3_multiply_procedure(&sky_transform, &matrices[i], &matrices[i]);
    }

    if (console_debug_toggle_6893ec) {
        rasterizer_render_states_dirty = 1;
        unknown_0071d1fa = 1;
    }
    {
        uint8_t *raw = (uint8_t *)&lighting;
        uint32_t k;

        for (k = 0; k < sizeof(lighting); k++) {
            raw[k] = 0;
        }
    }
    lighting.ambient_color = *global_white_color;
    render_model(sky->model.tag_id, matrices, 0.0f, 0, 0, function_values, &lighting,
                 &render_camera_global.position, 0.0f, 0, 0, 0, 1);

    if (console_debug_toggle_6893ec && rasterizer_device_version < 0xffff0101) {
        d3d_set_render_state_fn set_render_state =
            (d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4];

        set_render_state(rasterizer_device, 0x89, 0);
    }
}

/**
 * Draws one window: publishes the cameras and frustum for the rest of the renderer, begins the
 * rasterizer frame, then runs every scene pass in order (sky, lights, objects, shadows, decals,
 * structure lightmap / environment / decal / water passes, game engine overlay, weather,
 * particles, contrails, transparent geometry, detail objects, lens flares, first person screen
 * effects, screen flash, fullscreen widgets) and, in the extra window, the frame statistics.
 *
 * @address 0x0050bfb0
 */
void window(int16_t local_player_index, render_camera *source_camera, render_frustum *source_frustum,
    render_camera *rasterizer_camera, render_frustum *rasterizer_frustum, int16_t rasterizer_target,
    uint8_t has_mirror)
{
    object_render_data shadow_data;
    rasterizer_window_parameters parameters;
    uint8_t *raw;
    uint32_t i;
    int16_t saved_69c67c;

    render_window_count++;
    current_local_player_index = local_player_index;
    raw = (uint8_t *)&parameters;
    for (i = 0; i < sizeof(parameters); i++) {
        raw[i] = 0;
    }
    render_camera_global = *source_camera;
    render_frustum_global = *source_frustum;
    parameters.camera = *rasterizer_camera;
    parameters.frustum = *rasterizer_frustum;
    parameters.has_mirror = has_mirror;
    parameters.type = rasterizer_target;
    parameters.window_index = render_window_index;
    parameters.fog = render_fog_state;

    structure_bsp_cluster_visibility_update();
    player_effect_build_screen_flash(&parameters.screen_flash, local_player_index);
    rasterizer_begin_frame(&parameters);
    first_person_weapon_update_zoom_static_tint(1);
    billboard_system_frame_init();
    render_sky();
    first_person_weapon_update_active_state();
    object_lights_update_all();
    render_objects();
    structure_picked_polygon_refresh();
    structure_picked_polygon_draw();
    lens_flare_update_samples();
    if (console_debug_toggle_69c614) {
        shadow_data.object_index = 0xffffffff;
        shadow_data.unknown_44 = -1;
        shadow_data.lighting = 0;
        shadow_data.shadow_pass = 1;
        shadow_data.outside_fog_plane = 1;
        shadow_data.shadow_radius = 0.0f;
        render_object_shadows(&shadow_data);
    }
    lights_apply_spot_falloff();

    if (console_debug_toggle_6893e4 == 0 && decals_for_all_responses) {
        rasterizer_decal_pass_begin(2);
        draw_visible_cluster_decals();
        rasterizer_end_decal_pass();
    }
    rasterizer_decal_pass_begin(3);
    draw_visible_cluster_decals();
    rasterizer_end_decal_pass();

    if (picked_surfaces_valid) {
        rasterizer_fog_screen_overlay_set_states();
        structure_pass(0, (structure_material_callback)render_window_structure_material_0x511f70,
                       0, 0);
        rasterizer_force_bilinear_filtering();
    }

    if (console_debug_toggle_6893e4 == 0 && decals_for_all_responses) {
        rasterizer_decal_pass_begin(0);
        draw_visible_cluster_decals();
        rasterizer_end_decal_pass();
        rasterizer_decal_pass_begin(1);
        draw_visible_cluster_decals();
        reset_decal_fog_and_depth_bias();
    }
    lights_apply_spot_falloff_specular();

    if (picked_surfaces_valid) {
        saved_69c67c = render_force_flag;
        if (*(int32_t *)&global_structure_bsp->lightmaps_bitmap.tag_id == -1 && saved_69c67c == 0) {
            render_force_flag = 1;
        }
        rasterizer_dynamic_light_technique_ps2_set_states();
        structure_pass(render_window_structure_lightmap_begin_0x511f90,
                       (structure_material_callback)render_window_structure_material_0x511fe0,
                       (structure_lightmap_end_callback)function_do_nothing, 0);
        render_force_flag = saved_69c67c;
        if (picked_surfaces_valid) {
            rasterizer_shader_environment_technique_multipurpose_set_states();
            structure_pass(render_window_structure_lightmap_begin_0x512010,
                           (structure_material_callback)render_window_structure_material_0x512020,
                           (structure_lightmap_end_callback)function_do_nothing, 0);
            rasterizer_active_environment_effect = 0;
            if (picked_surfaces_valid) {
                rasterizer_shader_environment_technique_self_illumination_set_states();
                structure_pass(0, (structure_material_callback)render_window_structure_material_0x512040,
                               0, 0);
                if (picked_surfaces_valid) {
                    rasterizer_shader_decal_pass_set_states();
                    structure_pass(0,
                        (structure_material_callback)render_window_structure_material_0x512070, 0, 0);
                    if (picked_surfaces_valid) {
                        transparent_geometry_group_last_drawn_key = 0;
                        rasterizer_secondary_groups_drawn = 0;
                        structure_pass(0, 0, 0,
                            (structure_transparent_material_callback)
                                render_window_structure_transparent_0x512080);
                        if (picked_surfaces_valid) {
                            rasterizer_water_fade_compute_and_set_states();
                            structure_pass(0,
                                (structure_material_callback)render_window_structure_material_0x5120c0,
                                0, 0);
                        }
                    }
                }
            }
        }
    }

    if (current_game_engine != 0 && current_game_engine->post_rasterize != 0) {
        ((void (*)(void))current_game_engine->post_rasterize)();
    }
    weather_update_local_player();
    render_particles();
    particle_systems_render();
    render_contrails(0xfffffff3);
    transparent_geometry_group_draw_all(1);

    rasterizer_decal_pass_begin(4);
    draw_visible_cluster_decals();
    reset_decal_fog_and_depth_bias();
    detail_objects_update_render_list();
    transparent_geometry_group_draw_all(0);
    rasterizer_set_shader_stage_config(0);

    if (picked_surfaces_valid) {
        structure_pass(0, (structure_material_callback)function_do_nothing, 0, 0);
        structure_pass(0, (structure_material_callback)function_do_nothing, 0, 0);
    }
    lens_flare_render_all();
    first_person_weapon_update_screen_effects();
    rasterizer_screen_flash_render();
    widget_draw_fullscreen_region(local_player_index);
    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
    }
}

}  // namespace halo::render::frame

namespace halo::render::window_structure {

/**
 * Structure pass callback that binds the lightmap bitmap for the fixed function path. Does nothing unless the device and debug toggles allow lightmaps; a null bitmap marks the lightmap as missing.
 *
 * @address 0x00511f90
 */
void lightmap_begin_0x511f90(void *bitmap_data)
{
    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f7 == 0 ||
        rasterizer_device_version < 0xffff0104) {
        return;
    }
    if (bitmap_data != 0) {
        rasterizer_lightmap_bitmap = bitmap_data;
        rasterizer_lightmap_bitmap_missing = 0;
    } else {
        rasterizer_lightmap_bitmap = 0;
        rasterizer_lightmap_bitmap_missing = 1;
    }
}

/**
 * Structure pass callback that binds the lightmap bitmap for the shader environment path.
 *
 * @address 0x00512010
 */
void lightmap_begin_0x512010(void *bitmap_data)
{
    rasterizer_shader_environment_set_lightmap(bitmap_data);
}

/**
 * Structure pass material callback that forwards its arguments to the routine stored at 0x007c0494.
 *
 * @address 0x00511f40
 */
void material_0x511f40(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    ((void (*)(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra))unknown_007c0494)(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces through the object shadow structure routine.
 *
 * @address 0x00511f50
 */
void material_0x511f50(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_object_shadow_structure_draw(material_extra, render_context, first_surface, surface_count);
}

/**
 * Structure pass material callback that forwards its arguments to the routine stored at 0x007c0490.
 *
 * @address 0x00511f70
 */
void material_0x511f70(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    ((void (*)(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra))unknown_007c0490)(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the projected light shader environment.
 *
 * @address 0x00511f80
 */
void material_0x511f80(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_projected_light_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the lightmap specular shader environment.
 *
 * @address 0x00511fe0
 */
void material_0x511fe0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_lightmap_specular_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the shader environment technique.
 *
 * @address 0x00512020
 */
void material_0x512020(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_technique_draw(material_extra, shader_data, render_context, first_surface, surface_count);
}

/**
 * Structure pass material callback that draws the surfaces with the dynamic mirror shader environment.
 *
 * @address 0x00512040
 */
void material_0x512040(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_dynamic_mirror_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the reflection shader environment.
 *
 * @address 0x00512070
 */
void material_0x512070(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_reflection_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}

/**
 * Structure pass material callback that draws the surfaces as water ripples.
 *
 * @address 0x005120c0
 */
void material_0x5120c0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    rasterizer_water_ripple_draw(material_extra, shader_data, render_context, first_surface, surface_count);
}

/**
 * Structure pass callback for transparent shaders; submits the surfaces of one shader permutation with their bitmap as a transparent geometry group.
 *
 * @address 0x00512080
 */
void transparent_0x512080(void *shader_data, int16_t shader_permutation, void *bitmap, int32_t render_context,
    int32_t surface_offset, int16_t surface_count, void *material_extra, void *rendered_vertices,
    void *lightmap_vertices, void *coplanar_vector, void *lightmap_vertices_offset, int32_t zero)
{
    rasterizer_transparent_geometry_group_new(shader_data, shader_permutation, (uint32_t)bitmap, (uint32_t)render_context,
        (uint32_t)surface_offset, (uint32_t)(uint16_t)surface_count, (uint32_t)material_extra, lightmap_vertices,
        (uint32_t)lightmap_vertices_offset, (uint32_t)zero, rendered_vertices);
}

}  // namespace halo::render::window_structure
