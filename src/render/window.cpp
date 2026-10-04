#include "halo/rasterizer/render_device.hpp"
#include "halo/rasterizer/globals.hpp"
#include "crt.h"
#include "halo/models/api.hpp"
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
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/render/layout.hpp"
#include "halo/scenario/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/models/models.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/units/api.hpp"

static auto &sky_animation_times = halo::link::ref<float [9]>(halo::render::vars().sky_animation_times);
static auto &render_camera_global = halo::link::ref<render_camera>(halo::render::vars().render_camera_global);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &k_render_identity_matrix_ptr = halo::link::ref<real_matrix4x3 *>(halo::effects::vars().k_render_identity_matrix_ptr);
static auto &global_white_color = halo::link::ref<ColorRGB *>(halo::effects::vars().global_white_color);
static auto &rasterizer_render_states_dirty = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_states_dirty);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &render_window_count = halo::link::ref<int32_t>(halo::render::vars().render_window_count);
static auto &render_window_index = halo::link::ref<int16_t>(halo::render::vars().render_window_index);
static auto &render_frustum_global = halo::link::ref<render_frustum>(halo::render::vars().render_frustum_global);
static auto &render_fog_state = halo::link::ref<render_fog>(halo::render::vars().render_fog_state);
static auto &decals_for_all_responses = halo::link::ref<uint8_t>(halo::effects::vars().decals_for_all_responses);
static auto &rasterizer_active_environment_effect = halo::link::ref<uint32_t>(halo::rasterizer::vars().rasterizer_active_environment_effect);
static auto &transparent_geometry_group_last_drawn_key = halo::link::ref<int32_t>(halo::rasterizer::vars().transparent_geometry_group_last_drawn_key);
static auto &rasterizer_secondary_groups_drawn = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_secondary_groups_drawn);
static auto &rasterizer_decal_layer = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_decal_layer);
static auto &rasterizer_lightmap_bitmap = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_lightmap_bitmap);
static auto &rasterizer_lightmap_bitmap_missing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_lightmap_bitmap_missing);

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
    halo::rasterizer::render_device().set_render_state(state, value);
}

/**
 * Draws the decals of every visible cluster of the current window.
 */
static void draw_visible_cluster_decals(void)
{
    int16_t i;

    for (i = 0; i < halo::structures::globals().visible_cluster_count; i++) {
        halo::rasterizer::rasterizer_decals_draw_cluster((int16_t)(uint16_t)halo::structures::globals().visible_clusters[i].cluster_index);
    }
}

/**
 * The fog and depth bias reset that closes the stage 1 and stage 4 decal passes.
 */
static void reset_decal_fog_and_depth_bias(void)
{
    set_render_state((uint32_t)halo::d3d9::render_state::fog_enable, 0);
    if (halo::test_flag(halo::rasterizer::globals().caps.raster_caps, halo::d3d9::raster_cap::depth_bias)) {
        set_render_state((uint32_t)halo::d3d9::render_state::depth_bias, 0);
    }
    if (halo::test_flag(halo::rasterizer::globals().caps.raster_caps, halo::d3d9::raster_cap::slope_scale_depth_bias)) {
        set_render_state((uint32_t)halo::d3d9::render_state::slope_scale_depth_bias, 0);
    }
    if (rasterizer_decal_layer == 3) {
        halo::rasterizer::rasterizer_set_shader_stage_config(2);
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
    halo::structures::structure_leaf_faces_for_each(halo::structures::globals().picked_surfaces_geometry, lightmap_begin, material_cb,
                                  lightmap_end, transparent_material_cb, halo::structures::globals().visible_surface_indices,
                                  halo::structures::globals().visible_surface_count);
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

    if (!halo::structures::globals().render_cluster_has_sky) {
        return;
    }
    sky_tag = k_dword_none;
    if (halo::structures::globals().render_cluster_sky_index >= 0 &&
        (int32_t)halo::structures::globals().render_cluster_sky_index < (int32_t)halo::scenario::globals().scenario->skies.count) {
        sky_tag = tag_id_of(((ScenarioSky *)halo::scenario::globals().scenario->skies.pointer)[halo::structures::globals().render_cluster_sky_index].sky.tag_id);
    }
    sky = 0;
    if (sky_tag != k_dword_none) {
        sky = (Sky *)halo::cache::globals().tag_instances[(uint16_t)sky_tag].data;
    }
    model = (GBXModel *)halo::cache::globals().tag_instances[sky->model.tag_id.index].data;
    halo::models::model_view(model).get_default_transforms(reinterpret_cast<real_orientation *>(nodes));

    if (tag_id_of(sky->animation_graph.tag_id) != k_dword_none) {
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
                    float time = (float)halo::libm::fmod(halo::render::globals().time_since_frame / entry->period +
                                             sky_animation_times[i], 1.0);

                    sky_animation_times[i] = time;
                    halo::models::animation_view(animation).overlay_interpolated_frame_orientations((float)(int32_t)(int16_t)animation->frame_count * time, reinterpret_cast<real_orientation *>(nodes));
                }
            }
        }
    }

    halo::models::model_skeleton::build_matrices(global_zero_vector3d_pointer, halo::math::globals().global_forward3d_pointer, model, matrices, reinterpret_cast<real_orientation *>(nodes), halo::math::globals().global_up3d_pointer);

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

        if (tag_id_of(light->lens_flare.tag_id) == k_dword_none) {
            continue;
        }
        if (light->lens_flare_marker_name.string[0] == '\0') {
            real cos_pitch = (real)halo::libm::cos(light->direction.pitch);

            direction.i = (real)halo::libm::cos(light->direction.yaw) * cos_pitch;
            direction.j = (real)halo::libm::sin(light->direction.yaw) * cos_pitch;
            direction.k = (real)halo::libm::sin(light->direction.pitch);
        } else {
            object_marker marker;

            if (halo::models::model_markers::get_by_name(tag_id_of(sky->model.tag_id), light->lens_flare_marker_name.string, 0, 0, matrices, 0, &marker, 1) == 0) {
                continue;
            }
            direction.i = marker.node_transform.position.x - render_camera_global.position.x;
            direction.j = marker.node_transform.position.y - render_camera_global.position.y;
            direction.k = marker.node_transform.position.z - render_camera_global.position.z;
            length = (real)halo::libm::sqrt(direction.k * direction.k + direction.j * direction.j +
                                direction.i * direction.i);
            if (halo::libm::fabs(length) >= 0.0001) {
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
        length = (real)halo::libm::sqrt(up.k * up.k + up.j * up.j + up.i * up.i);
        if (halo::libm::fabs(length) >= 0.0001) {
            real inverse = 1.0f / length;

            up.i = up.i * inverse;
            up.j = up.j * inverse;
            up.k = up.k * inverse;
        }
        halo::objects::light_transient_add(tag_id_of(light->lens_flare.tag_id), (real_vector3d *)global_white_color, &position,
                            (uint32_t)&toward_camera, (uint32_t)&up, 1.0f);
    }

    sky_transform = *k_render_identity_matrix_ptr;
    sky_transform.position.x = render_camera_global.position.x * 0.99902344f;
    sky_transform.position.y = render_camera_global.position.y * 0.99902344f;
    sky_transform.position.z = render_camera_global.position.z * 0.99902344f;
    sky_transform.scale = 0.0009765625f;
    for (i = 0; (int32_t)i < (int32_t)model->nodes.count; i++) {
        halo::math::globals().matrix4x3_multiply_procedure(&sky_transform, &matrices[i], &matrices[i]);
    }

    if (halo::rasterizer::fields::models_enabled) {
        rasterizer_render_states_dirty = 1;
        halo::rasterizer::fields::sky_pass_active = 1;
    }
    {
        uint8_t *raw = (uint8_t *)&lighting;
        uint32_t k;

        for (k = 0; k < sizeof(lighting); k++) {
            raw[k] = 0;
        }
    }
    lighting.ambient_color = *global_white_color;
    halo::models::render_model(sky->model.tag_id, matrices, 0.0f, 0, 0, function_values, &lighting,
                 &render_camera_global.position, 0.0f, 0, 0, 0, 1);

    if (console_debug_toggle_6893ec && rasterizer_device_version < d3d9::k_pixel_shader_version_1_1) {
        halo::rasterizer::render_device().set_render_state((uint32_t)d3d9::render_state::lighting, 0);
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
    halo::interface::globals().current_local_player_index = local_player_index;
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

    halo::structures::structure_bsp_cluster_visibility_update();
    halo::effects::player_effect_build_screen_flash((uint32_t *)&parameters.screen_flash, local_player_index);
    halo::rasterizer::rasterizer_begin_frame(&parameters);
    halo::interface::first_person_weapon_update_zoom_static_tint(1);
    halo::render::billboard_system_frame_init();
    halo::render::render_sky();
    halo::interface::first_person_weapon_update_active_state();
    halo::objects::object_lights_update_all();
    halo::render::render_objects();
    halo::structures::structure_picked_polygon_refresh();
    halo::structures::structure_picked_polygon_draw();
    halo::rasterizer::lens_flare_update_samples();
    if (halo::rasterizer::fields::object_shadow_pass_enabled) {
        shadow_data.object_index = k_dword_none;
        shadow_data.unknown_44 = -1;
        shadow_data.lighting = 0;
        shadow_data.shadow_pass = 1;
        shadow_data.outside_fog_plane = 1;
        shadow_data.shadow_radius = 0.0f;
        halo::render::render_object_shadows(&shadow_data);
    }
    halo::objects::lights_apply_spot_falloff();

    if (console_debug_toggle_6893e4 == 0 && halo::effects::globals().decals_for_all_responses) {
        halo::rasterizer::rasterizer_decal_pass_begin(2);
        draw_visible_cluster_decals();
        halo::rasterizer::rasterizer_end_decal_pass();
    }
    halo::rasterizer::rasterizer_decal_pass_begin(3);
    draw_visible_cluster_decals();
    halo::rasterizer::rasterizer_end_decal_pass();

    if (halo::structures::globals().picked_surfaces_valid) {
        halo::rasterizer::rasterizer_fog_screen_overlay_set_states();
        structure_pass(0, (structure_material_callback)halo::render::render_window_structure_material_0x511f70,
                       0, 0);
        halo::rasterizer::rasterizer_force_bilinear_filtering();
    }

    if (console_debug_toggle_6893e4 == 0 && halo::effects::globals().decals_for_all_responses) {
        halo::rasterizer::rasterizer_decal_pass_begin(0);
        draw_visible_cluster_decals();
        halo::rasterizer::rasterizer_end_decal_pass();
        halo::rasterizer::rasterizer_decal_pass_begin(1);
        draw_visible_cluster_decals();
        reset_decal_fog_and_depth_bias();
    }
    halo::objects::lights_apply_spot_falloff_specular();

    if (halo::structures::globals().picked_surfaces_valid) {
        saved_69c67c = halo::render::globals().force_flag;
        if (*(int32_t *)&halo::scenario::globals().structure_bsp->lightmaps_bitmap.tag_id == -1 && saved_69c67c == 0) {
            halo::render::globals().force_flag = 1;
        }
        halo::rasterizer::rasterizer_dynamic_light_technique_ps2_set_states();
        structure_pass(halo::render::render_window_structure_lightmap_begin_0x511f90,
                       (structure_material_callback)halo::render::render_window_structure_material_0x511fe0,
                       (structure_lightmap_end_callback)halo::cseries::function_do_nothing, 0);
        halo::render::globals().force_flag = saved_69c67c;
        if (halo::structures::globals().picked_surfaces_valid) {
            halo::rasterizer::rasterizer_shader_environment_technique_multipurpose_set_states();
            structure_pass(halo::render::render_window_structure_lightmap_begin_0x512010,
                           (structure_material_callback)halo::render::render_window_structure_material_0x512020,
                           (structure_lightmap_end_callback)halo::cseries::function_do_nothing, 0);
            rasterizer_active_environment_effect = 0;
            if (halo::structures::globals().picked_surfaces_valid) {
                halo::rasterizer::rasterizer_shader_environment_technique_self_illumination_set_states();
                structure_pass(0, (structure_material_callback)halo::render::render_window_structure_material_0x512040,
                               0, 0);
                if (halo::structures::globals().picked_surfaces_valid) {
                    halo::rasterizer::rasterizer_shader_decal_pass_set_states();
                    structure_pass(0,
                        (structure_material_callback)halo::render::render_window_structure_material_0x512070, 0, 0);
                    if (halo::structures::globals().picked_surfaces_valid) {
                        transparent_geometry_group_last_drawn_key = 0;
                        rasterizer_secondary_groups_drawn = 0;
                        structure_pass(0, 0, 0,
                            (structure_transparent_material_callback)
                                halo::render::render_window_structure_transparent_0x512080);
                        if (halo::structures::globals().picked_surfaces_valid) {
                            halo::rasterizer::rasterizer_water_fade_compute_and_set_states();
                            structure_pass(0,
                                (structure_material_callback)halo::render::render_window_structure_material_0x5120c0,
                                0, 0);
                        }
                    }
                }
            }
        }
    }

    if (halo::game::globals().current_engine != 0 && halo::game::globals().current_engine->post_rasterize != 0) {
        ((void (*)(void))halo::game::globals().current_engine->post_rasterize)();
    }
    halo::effects::weather_update_local_player();
    halo::render::render_particles();
    halo::effects::particle_systems_render();
    halo::render::render_contrails(0xfffffff3);
    halo::rasterizer::transparent_geometry_group_draw_all(1);

    halo::rasterizer::rasterizer_decal_pass_begin(4);
    draw_visible_cluster_decals();
    reset_decal_fog_and_depth_bias();
    halo::structures::detail_objects_update_render_list();
    halo::rasterizer::transparent_geometry_group_draw_all(0);
    halo::rasterizer::rasterizer_set_shader_stage_config(0);

    if (halo::structures::globals().picked_surfaces_valid) {
        structure_pass(0, (structure_material_callback)halo::cseries::function_do_nothing, 0, 0);
        structure_pass(0, (structure_material_callback)halo::cseries::function_do_nothing, 0, 0);
    }
    halo::rasterizer::lens_flare_render_all();
    halo::interface::first_person_weapon_update_screen_effects();
    halo::rasterizer::rasterizer_screen_flash_render();
    halo::interface::widget_draw_fullscreen_region(local_player_index);
    if (halo::rasterizer::globals().window.window_index == -1) {
        halo::render::rasterizer_frame_statistics_sample(&halo::rasterizer::globals().frame_statistics, 0);
        halo::render::rasterizer_frame_statistics_draw();
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
    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::specular_lightmap_enabled == 0 ||
        rasterizer_device_version < k_device_version_lightmap_pass) {
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
    halo::rasterizer::rasterizer_shader_environment_set_lightmap((BitmapData *)bitmap_data);
}

/**
 * Structure pass material callback that forwards its arguments to the routine stored at 0x007c0494.
 *
 * @address 0x00511f40
 */
void material_0x511f40(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::fields::light_cone_draw(static_cast<const ShaderEnvironment *>(shader_data), shader_permutation, render_context, first_surface, surface_count, static_cast<rasterizer_vertex_buffer *>(material_extra));
}

/**
 * Structure pass material callback that draws the surfaces through the object shadow structure routine.
 *
 * @address 0x00511f50
 */
void material_0x511f50(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_object_shadow_structure_draw((rasterizer_vertex_buffer *)material_extra, render_context, first_surface, surface_count);
}

/**
 * Structure pass material callback that forwards its arguments to the routine stored at 0x007c0490.
 *
 * @address 0x00511f70
 */
void material_0x511f70(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::fields::environment_lightmap_draw(static_cast<const ShaderEnvironment *>(shader_data), shader_permutation, render_context, first_surface, surface_count, static_cast<rasterizer_vertex_buffer *>(material_extra));
}

/**
 * Structure pass material callback that draws the surfaces with the projected light shader environment.
 *
 * @address 0x00511f80
 */
void material_0x511f80(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_shader_environment_projected_light_draw((const ShaderEnvironment *)shader_data, shader_permutation, render_context, first_surface, surface_count, (rasterizer_vertex_buffer *)material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the lightmap specular shader environment.
 *
 * @address 0x00511fe0
 */
void material_0x511fe0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_shader_environment_lightmap_specular_draw((const ShaderEnvironment *)shader_data, shader_permutation, render_context, first_surface, surface_count, (rasterizer_vertex_buffer *)material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the shader environment technique.
 *
 * @address 0x00512020
 */
void material_0x512020(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_shader_environment_technique_draw((rasterizer_vertex_buffer *)material_extra, (const ShaderEnvironment *)shader_data, render_context, first_surface, surface_count);
}

/**
 * Structure pass material callback that draws the surfaces with the dynamic mirror shader environment.
 *
 * @address 0x00512040
 */
void material_0x512040(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_shader_environment_dynamic_mirror_draw((const ShaderEnvironment *)shader_data, shader_permutation, render_context, first_surface, surface_count, (rasterizer_vertex_buffer *)material_extra);
}

/**
 * Structure pass material callback that draws the surfaces with the reflection shader environment.
 *
 * @address 0x00512070
 */
void material_0x512070(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_shader_environment_reflection_draw((const ShaderEnvironment *)shader_data, shader_permutation, render_context, first_surface, surface_count, (rasterizer_vertex_buffer *)material_extra);
}

/**
 * Structure pass material callback that draws the surfaces as water ripples.
 *
 * @address 0x005120c0
 */
void material_0x5120c0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count, void *material_extra)
{
    halo::rasterizer::rasterizer_water_ripple_draw((rasterizer_vertex_buffer *)material_extra, (const Shader *)shader_data, render_context, first_surface, surface_count);
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
    halo::rasterizer::rasterizer_transparent_geometry_group_new((Shader *)shader_data, shader_permutation, static_cast<BitmapData *>(bitmap), (uint32_t)render_context,
        (uint32_t)surface_offset, (uint32_t)(uint16_t)surface_count, static_cast<rasterizer_vertex_buffer *>(material_extra), (ColorARGB *)lightmap_vertices,
        static_cast<render_lighting *>(lightmap_vertices_offset), (uint32_t)zero, (real_point3d *)rendered_vertices);
}

}  // namespace halo::render::window_structure
