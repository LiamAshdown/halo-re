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
#include "camera.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/render/layout.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/vars.hpp"

static auto &render_frustum_global = halo::link::ref<render_frustum>(halo::render::vars().render_frustum_global);
static auto &object_render_state_cache = halo::link::ref<data_array *>(halo::game::vars().object_render_state_cache);
static auto &render_uncached_object_lighting = halo::link::ref<render_lighting>(halo::render::vars().render_uncached_object_lighting);
static auto &render_window_count = halo::link::ref<int32_t>(halo::render::vars().render_window_count);
static auto &render_frame_index = halo::link::ref<int32_t>(halo::render::vars().render_frame_index);
static auto &render_lighting_smoothing_enabled = halo::link::ref<uint8_t>(halo::render::vars().render_lighting_smoothing_enabled);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &render_fog_state = halo::link::ref<render_fog>(halo::render::vars().render_fog_state);
static auto &render_camera_global = halo::link::ref<render_camera>(halo::render::vars().render_camera_global);
static auto &render_debug_objects = halo::link::ref<uint8_t>(halo::render::vars().render_debug_objects);
static auto &rasterizer_window = halo::link::ref<rasterizer_window_parameters>(halo::rasterizer::vars().rasterizer_window);
static auto &rasterizer_caps_flag_689 = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_caps_flag_689);
static auto &rasterizer_object_shadow_window_restored = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_object_shadow_window_restored);
static auto &rendered_object_count = halo::link::ref<int16_t>(halo::render::vars().rendered_object_count);
static auto &rendered_objects = halo::link::ref<datum_index [halo::render::k_maximum_rendered_objects]>(halo::render::vars().rendered_objects);
static auto &rasterizer_render_states_dirty = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_states_dirty);
static auto &console_debug_toggle_6893ee = halo::link::ref<uint8_t>(halo::render::vars().console_debug_toggle_6893ee);
static auto &rendered_objects_full_warning = halo::link::ref<uint8_t>(halo::render::vars().rendered_objects_full_warning);

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

/**
 * Applies either the fake object "blob" shadow effect (data->shadow_pass != 0) or ordinary cached
 * ambient lighting plus a fog-plane test (data->shadow_pass == 0) to one object, then always hands
 * it to render_object_list.
 *
 * @address 0x0050eba0
 */
void halo::render::ObjectRenderData::draw()
{
    object_render_data *data = self;
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)data->object_index];
    object *obj = header->data;

    if (data->shadow_pass != 0) {
        real level_of_detail_pixels;
        real luminance_deficit;

        if (halo::render::render_object_is_camera_unit(data->object_index)) {
            return;
        }
        if ((obj->flags & _object_definition_flag0_bit) != 0) {
            return;
        }
        if ((obj->flags & _object_no_collision_bit) != 0 && obj->first_child_object == k_datum_index_none) {
            return;
        }

        level_of_detail_pixels = halo::render::object_compute_level_of_detail_pixels(data->object_index);
        {
            render_lighting *lighting = halo::render::object_get_cached_render_lighting(data->object_index, level_of_detail_pixels);
            data->lighting = (uint32_t)(uintptr_t)lighting;
            level_of_detail_pixels = halo::render::object_compute_level_of_detail_pixels(data->object_index);
            luminance_deficit = 1.0f - (lighting->shadow_color.red * 0.299f + lighting->shadow_color.green * 0.587f +
                                        lighting->shadow_color.blue * 0.114f);
        }

        if (level_of_detail_pixels <= 30.0f) {
            return;
        }
        if (luminance_deficit <= 0.19f) {
            return;
        }

        {
            float distance_fade = (level_of_detail_pixels - 30.0f) * 0.06666667f;
            float darkness_fade = (luminance_deficit - 0.19f) * 9.090908f;

            if (distance_fade < 0.0f) {
                distance_fade = 0.0f;
            } else if (distance_fade > 1.0f) {
                distance_fade = 1.0f;
            }
            if (darkness_fade < 0.0f) {
                darkness_fade = 0.0f;
            } else if (darkness_fade > 1.0f) {
                darkness_fade = 1.0f;
            }

            if (!halo::render::render_object_shadow_begin(data, darkness_fade * distance_fade)) {
                return;
            }
        }

        halo::render::render_object_list(data, 0, data->object_index);
        halo::render::render_object_shadow_end(data);
        return;
    }

    {
        uint8_t sample_full_lighting;
        Object *definition;

        if ((obj->flags & _object_no_collision_bit) != 0 && obj->first_child_object == k_datum_index_none) {
            datum_index first_widget = obj->first_widget;
            if (!halo::objects::widget_list_has_flag(first_widget)) {
                if (first_widget == k_datum_index_none) {
                    return;
                }
                sample_full_lighting = 0;
                goto sampled;
            }
        }
        sample_full_lighting = 1;

    sampled:
        definition = (Object *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;

        if (sample_full_lighting) {
            real level_of_detail_pixels = halo::render::object_compute_level_of_detail_pixels(data->object_index);
            render_lighting *lighting = halo::render::object_get_cached_render_lighting(data->object_index, level_of_detail_pixels);
            data->lighting = (uint32_t)(uintptr_t)lighting;
        } else {
            data->lighting = 0;
        }

        if (render_fog_state.planar_mode != _structure_fog_plane_bounded ||
            definition->bounding_radius <
                (render_fog_state.plane.normal.i * obj->bounding_center.x +
                 render_fog_state.plane.normal.k * obj->bounding_center.z +
                 render_fog_state.plane.normal.j * obj->bounding_center.y) -
                    render_fog_state.plane.d) {
            data->outside_fog_plane = 1;
        } else {
            data->outside_fog_plane = 0;
        }

        render_model_effect top_level_effect = {0};
        top_level_effect.type = _render_model_effect_none;
        halo::render::render_object_list(data, &top_level_effect, data->object_index);
    }
}

/**
 * Recursively walks an object and its sibling/child chain, drawing each one's model (skipping the
 * local player's own camera unit unless viewing through a mirror), building a render_model_effect
 * per object (inherited from the parent, reset below a self-occlusion parent, and re-derived for
 * active camouflage or self-occlusion on the object itself) and notifying its widget list.
 *
 * @address 0x0050ee20
 */
void halo::render::ObjectRenderData::list(render_model_effect *parent_effect, datum_index object_index)
{
    object_render_data *data = self;
    while (object_index != k_datum_index_none) {
        object *obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index].data;
        render_model_effect effect;

        if (halo::render::render_object_is_camera_unit(object_index) && !render_camera_global.mirrored) {
            goto next_sibling;
        }

        if (data->shadow_pass == 0) {
            effect = *parent_effect;
            if (parent_effect->type == _render_model_effect_self_occlusion) {
                effect.type = _render_model_effect_none;
                effect.modifier_shader = 0;
                effect.function_values = 0;
                effect.change_colors = 0;
            }
        }

        if ((obj->flags & _object_no_collision_bit) == 0) {
            Object *tag_data = (Object *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
            real lod = halo::render::object_compute_level_of_detail_pixels(object_index);

            if (data->shadow_pass == 0) {
                if (*(uint32_t *)&tag_data->modifier_shader.tag_id != k_dword_none) {
                    Shader *shader_data =
                        (Shader *)halo::cache::globals().tag_instances[tag_data->modifier_shader.tag_id.index].data;

                    effect.modifier_shader = (uint32_t)(uintptr_t)shader_data;
                    if (shader_type_is_transparent(shader_data->shader_type)) {
                        effect.change_colors = (uint32_t)(uintptr_t)obj->change_colors;
                        effect.function_values = (uint32_t)(uintptr_t)obj->function_out_values;
                    } else {
                        effect.modifier_shader = 0;
                    }
                }

                if (objects::object_mask_has_type(objects::object_mask::unit, obj->type)) {
                    object *unit_object =
                        ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index].data;
                    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

                    if (unit->active_camouflage_power > 0.0f) {
                        effect.centroid = obj->bounding_center;
                        effect.type = _render_model_effect_active_camouflage;
                        effect.object_index = object_index;
                        effect.unit_37c = unit->active_camouflage_power;
                        effect.unit_380 = unit->super_active_camouflage_power;
                    }
                }
                if (test_flag(tag_data->flags, tags::object_tag_flag::transparent_self_occlusion)) {
                    effect.centroid = obj->bounding_center;
                    effect.type = _render_model_effect_self_occlusion;
                    effect.object_index = object_index;
                }

                halo::models::render_model(tag_data->model.tag_id,
                             (uint8_t *)obj + obj->nodes.offset, lod,
                             obj->region_permutations, obj->change_colors, obj->function_out_values,
                             (render_lighting *)(uintptr_t)data->lighting, &obj->bounding_center,
                             obj->bounding_radius, &effect, object_index,
                             obj->forced_shader_permutation,
                             (data->outside_fog_plane != 0) ? 4u : 0u);

                if (render_debug_objects != 0) {
                    halo::objects::object_type_definitions_notify_0x5c(object_index);
                }
            } else {
                halo::models::render_model(tag_data->model.tag_id,
                             (uint8_t *)obj + obj->nodes.offset, lod * 0.3f,
                             obj->region_permutations, obj->change_colors, obj->function_out_values,
                             (render_lighting *)(uintptr_t)data->lighting, &obj->bounding_center,
                             obj->bounding_radius, 0, object_index,
                             obj->forced_shader_permutation, 2);
            }
        }

        if (data->shadow_pass == 0 && obj->first_widget != k_datum_index_none) {
            render_animation animation;

            animation.change_colors = (uint32_t)(uintptr_t)obj->change_colors;
            animation.function_values = (uint32_t)(uintptr_t)obj->function_out_values;
            halo::objects::widget_list_notify(object_index, (uint32_t)((render_lighting *)(uintptr_t)data->lighting),
                               &animation);
        }

        if (obj->first_child_object != k_datum_index_none) {
            halo::render::render_object_list(data, data->shadow_pass != 0 ? 0 : &effect,
                               obj->first_child_object);
        }

    next_sibling:
        object_index = obj->next_object;
    }
}

/**
 * stack -> (radius, out_radius); always returns 1
 * Builds the shadow projection of the object in data: a basis whose up axis is the cached
 * lighting's shadow vector, centred on the object, and a shadow colour that fades from white
 * toward lighting.shadow_color by `fade` (reduced by the unit camouflage amount), then hands both
 * to the rasterizer, which writes the shadow radius into data->shadow_radius.
 *
 * @address 0x0050f830
 */
uint8_t halo::render::ObjectRenderData::shadow_begin(float fade)
{
    object_render_data *data = self;
    object *o = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)data->object_index].data;
    render_lighting *lighting = (render_lighting *)data->lighting;
    real_point3d center = o->bounding_center;
    float radius = o->bounding_radius;
    real_vector3d forward;
    ColorRGB color;
    float t;

    halo::math::vector3d_build_perpendicular(forward, lighting->shadow_vector);
    halo::math::vector3d_normalize_with_length(forward);
    halo::math::matrix4x3_from_forward_up(lighting->shadow_vector, forward, data->shadow_matrix);
    data->shadow_matrix.position = center;

    color = lighting->shadow_color;

    o = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)data->object_index].data;
    if (((1 << (uint8_t)o->type) & 3) != 0) {
        unit_data *unit = (unit_data *)((uint8_t *)o + k_unit_data_offset);
        if (unit->active_camouflage_power > 0.0f) {
            t = (1.0f - unit->active_camouflage_power) * fade;
        } else {
            t = fade;
        }
    } else {
        t = fade;
    }

    color.red = color.red * t + (1.0f - t);
    color.green = color.green * t + (1.0f - t);
    color.blue = color.blue * t + (1.0f - t);

    return halo::rasterizer::rasterizer_object_shadow_begin(&data->shadow_matrix, &color, radius, &data->shadow_radius);
}

/**
 * Draws the fake shadow of the object in data onto the BSP surfaces inside its shadow volume
 * (a box around the shadow basis, 4 radii deep along the light) and restores the window render
 * target the first time a shadow is finished in the main window.
 *
 * @address 0x0050f980
 */
void halo::render::ObjectRenderData::shadow_end()
{
    object_render_data *data = self;
    real_vector3d *forward = &data->shadow_matrix.forward;
    real_vector3d *left = &data->shadow_matrix.left;
    real_vector3d *up = &data->shadow_matrix.up;
    real_point3d *position = &data->shadow_matrix.position;
    real_plane3d planes[6];
    real_rectangle3d box;
    float d;
    float sx;
    float sy;
    float sz;
    float lower;
    float upper;

    d = up->k * position->z + up->j * position->y + up->i * position->x;
    planes[0].normal = *up;
    planes[0].d = d - data->shadow_radius * 0.5f;
    planes[1].normal.i = -up->i;
    planes[1].normal.j = -up->j;
    planes[1].normal.k = -up->k;
    planes[1].d = -d - data->shadow_radius * 4.0f;

    d = forward->k * position->z + forward->j * position->y + forward->i * position->x;
    planes[2].normal = *forward;
    planes[2].d = d - data->shadow_radius;
    planes[3].normal.i = -forward->i;
    planes[3].normal.j = -forward->j;
    planes[3].normal.k = -forward->k;
    planes[3].d = -d - data->shadow_radius;

    d = position->z * left->k + position->y * left->j + position->x * left->i;
    planes[4].normal = *left;
    planes[4].d = d - data->shadow_radius;
    planes[5].normal.i = -left->i;
    planes[5].normal.j = -left->j;
    planes[5].normal.k = -left->k;
    planes[5].d = -d - data->shadow_radius;

    sx = (left->i < 0.0f ? -left->i : left->i) + (forward->i < 0.0f ? -forward->i : forward->i);
    sy = (left->j < 0.0f ? -left->j : left->j) + (forward->j < 0.0f ? -forward->j : forward->j);
    sz = (forward->k < 0.0f ? -forward->k : forward->k) + (left->k < 0.0f ? -left->k : left->k);

    lower = up->i * (up->i > 0.0f ? -0.5f : 4.0f) + -sx;
    upper = up->i * (up->i > 0.0f ? 4.0f : -0.5f) + sx;
    box.x.lower = lower * data->shadow_radius + position->x;
    box.x.upper = upper * data->shadow_radius + position->x;
    lower = up->j * (up->j > 0.0f ? -0.5f : 4.0f) + -sy;
    upper = up->j * (up->j > 0.0f ? 4.0f : -0.5f) + sy;
    box.y.lower = lower * data->shadow_radius + position->y;
    box.y.upper = upper * data->shadow_radius + position->y;
    lower = up->k * 4.0f + -sz;
    upper = sz - up->k * 0.5f;
    box.z.lower = lower * data->shadow_radius + position->z;
    box.z.upper = upper * data->shadow_radius + position->z;

    halo::structures::structure_debug_draw_surfaces_simple(position, data->shadow_radius * 4.0f, &box, planes, 6);

    if (rasterizer_window.type == 1 && rasterizer_caps_flag_689 == 0 && halo::rasterizer::fields::object_shadows_enabled != 0 &&
        rasterizer_object_shadow_window_restored == 0) {
        halo::rasterizer::rasterizer_render_target_set_active(1, 0, 0);
        rasterizer_object_shadow_window_restored = 1;
    }
}

/**
 * Applies the fake object "blob" shadow effect to every object in the current shadow candidate
 * list (rebuilt by render_objects_collect, 0x50eac0), reusing the single caller-supplied
 * object_render_data block for each one.
 *
 * @address 0x0050eb70
 */
void halo::render::ObjectRenderData::shadows()
{
    object_render_data *data = self;
    int16_t i;

    for (i = 0; i < rendered_object_count; i++) {
        data->object_index = rendered_objects[i];
        halo::render::render_object(data);
    }
}

namespace halo::render::object_cache {

/**
 * a variable cannot share the render_frustum typedef's
 * own name in C; matches src/render/render_nonplayer_frame.c)
 * Projects an object's bounding sphere to an approximate on-screen pixel radius, used by
 * object_render_state_refresh (0x50f270) to pick the cached-lighting refresh interval. Objects
 * flagged with the (unnamed) 0x400000 bit are exempt from the detail falloff while
 * cinematic_globals_ptr->in_progress is set, and always report the maximum size.
 *
 * @address 0x0050f740
 */
real compute_level_of_detail_pixels(datum_index object_index)
{
    object *obj;
    real radius;
    real depth;

    obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index].data;

    if (halo::cutscene::globals().cinematic_globals->in_progress != 0 && test_flag(obj->flags, objects::object_flag::unknown_400000)) {
        return k_maximum_level_of_detail_pixels;
    }

    radius = obj->bounding_radius;
    if ((int16_t)halo::rasterizer::fields::object_lod_quality == 1) {
        radius = radius * 0.5f;
    } else if ((int16_t)halo::rasterizer::fields::object_lod_quality == 0) {
        radius = radius * 0.25f;
    }

    depth = render_frustum_global.world_to_view.left.k * obj->bounding_center.y +
            render_frustum_global.world_to_view.forward.k * obj->bounding_center.x +
            render_frustum_global.world_to_view.up.k * obj->bounding_center.z +
            render_frustum_global.world_to_view.position.z;
    if (depth < 0.0f) {
        depth = -depth;
    }
    if (depth <= 0.1f) {
        depth = 0.1f;
    }

    radius = (render_frustum_global.projection_world_to_screen.j / depth) * radius;
    return radius + radius;
}

/**
 * Returns a pointer to the (cached, or freshly sampled and uncached) ambient lighting values for
 * an object: if the object render-state cache has (or can make) room, returns a pointer straight
 * into that cached entry's lighting field; otherwise falls back to sampling directly into the
 * shared render_uncached_object_lighting scratch global.
 *
 * @address 0x0050ea00
 */
render_lighting *get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels)
{
    datum_index cache_index = halo::render::object_get_cached_render_state(object_index, level_of_detail_pixels);

    if (cache_index != k_datum_index_none) {
        return &((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)cache_index].lighting;
    }

    halo::objects::object_sample_ambient_lighting(object_index, (float *)&render_uncached_object_lighting);
    halo::objects::object_gather_light_list(object_index, (uint8_t *)&render_uncached_object_lighting);
    return &render_uncached_object_lighting;
}

/**
 * 0x50f270, this module
 * Finds the cached render-state entry already associated with object_index (validating it still
 * belongs to that object), or allocates a new one, evicting the entry with the oldest
 * last_update_window when the cache is full. Either way refreshes the entry (a cheap step for a
 * reused entry, a full sample for a newly allocated or evicted one) and stamps the owning object's
 * cache-entry index before returning it.
 *
 * @address 0x0050f150
 */
datum_index get_cached_render_state(datum_index object_index, real level_of_detail_pixels)
{
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index];
    object *obj = header->data;
    datum_index cache_index = obj->cached_render_state_index;

    if (cache_index != k_datum_index_none &&
        ((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)cache_index].object_index ==
            object_index) {
        halo::render::object_render_state_refresh(cache_index, object_index, level_of_detail_pixels, 0);
        return cache_index;
    }

    cache_index = halo::memory::datum_new(object_render_state_cache);
    if (cache_index == k_datum_index_none) {
        float oldest_age = -3.4028235e+38f;
        datum_index candidate = halo::memory::datum_next(-1, object_render_state_cache);
        int32_t current_window = render_window_count;

        while (candidate != k_datum_index_none) {
            cached_object_render_state *entry =
                &((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)candidate];
            float age = (float)(current_window - entry->last_update_window);
            if (age < 0.0f) {
                age = 1000.0f;
            }
            if (oldest_age < age) {
                cache_index = candidate;
                oldest_age = age;
            }
            candidate = halo::memory::datum_next((int16_t)candidate, object_render_state_cache);
        }
        if (cache_index == k_datum_index_none) {
            return k_datum_index_none;
        }
    }

    halo::render::object_render_state_refresh(cache_index, object_index, level_of_detail_pixels, 1);
    obj->cached_render_state_index = cache_index;
    return cache_index;
}

/**
 * Refreshes one cached object render-state entry: a forced (full_sample) or overdue entry
 * resamples desired_lighting from the object's surroundings; the light list is regathered for a
 * full sample and whenever a window has been drawn since the last refresh; a full sample then
 * replaces the smoothed lighting outright. An entry that is not overdue only gets its point
 * light list copied (when a window has passed); an overdue one is stepped toward
 * desired_lighting (0.03 per call, 0.012 for the shadow vector) when
 * render_lighting_smoothing_enabled is set and the object is moving or is a type 7 object
 * (object_try_and_get mask 0x80), and overwritten with it otherwise.
 *
 * @address 0x0050f270
 */
void render_state_refresh(datum_index cache_index, datum_index object_index, real level_of_detail_pixels,
    uint8_t full_sample)
{
    cached_object_render_state *entry =
        &((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)cache_index];
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index].data;
    int32_t windows_elapsed = render_window_count - entry->last_update_window;
    int32_t frames_since_update = render_frame_index - entry->last_update_frame;
    int32_t frames_since_sample = render_frame_index - entry->last_sample_frame;
    uint8_t overdue = 0;

    if (frames_since_sample < 0 || windows_elapsed < 0) {
        frames_since_sample = 1;
        windows_elapsed = 1;
    }

    if (test_flag(obj->flags, objects::object_flag::unknown_4000)) {
        int32_t staleness_threshold;

        if (level_of_detail_pixels > 400.0f) {
            staleness_threshold = 0;
        } else if (level_of_detail_pixels > 100.0f) {
            staleness_threshold = 3;
        } else {
            staleness_threshold = 10;
        }
        overdue = (uint8_t)(frames_since_sample > staleness_threshold);
        if (overdue && frames_since_update > 1) {
            full_sample = 1;
        }
    }

    if (full_sample || overdue) {
        entry->object_index = object_index;
        halo::objects::object_sample_ambient_lighting(object_index, (float *)(&entry->desired_lighting));
        entry->level_of_detail_pixels = level_of_detail_pixels;
        entry->last_sample_frame = render_frame_index;
    }

    if (full_sample || windows_elapsed > 0) {
        halo::objects::object_gather_light_list(object_index, (uint8_t *)(&entry->desired_lighting));
        if (full_sample) {
            entry->lighting = entry->desired_lighting;
            entry->last_update_window = render_window_count;
            entry->last_update_frame = render_frame_index;
            return;
        }
    }

    if (!overdue) {
        if (windows_elapsed > 0) {
            entry->lighting.point_light_count = entry->desired_lighting.point_light_count;
            entry->lighting.point_light_indices[0] = entry->desired_lighting.point_light_indices[0];
            entry->lighting.point_light_indices[1] = entry->desired_lighting.point_light_indices[1];
        }
        entry->last_update_window = render_window_count;
        entry->last_update_frame = render_frame_index;
        return;
    }

    if (render_lighting_smoothing_enabled == 0) {
        entry->lighting = entry->desired_lighting;
        entry->last_update_window = render_window_count;
        entry->last_update_frame = render_frame_index;
        return;
    }

    {
        real_vector3d root_velocity;

        halo::objects::object_get_root_object_velocities(object_index, &root_velocity, 0);
        if (root_velocity.i != 0.0f || root_velocity.j != 0.0f || root_velocity.k != 0.0f ||
            halo::objects::object_try_and_get(object_index, to_bits(objects::object_mask::device_machine)) != 0) {
            halo::render::render_lighting_step_vector3_toward(&entry->lighting.ambient_color.red,
                                                &entry->desired_lighting.ambient_color.red, 0.03f);
            halo::render::render_lighting_step_vector4_toward(&entry->lighting.reflection_tint.alpha,
                                                &entry->desired_lighting.reflection_tint.alpha, 0.03f);
            halo::render::render_lighting_step_vector3_toward(&entry->lighting.distant_lights[0].color.red,
                                                &entry->desired_lighting.distant_lights[0].color.red, 0.03f);
            halo::render::render_lighting_step_direction_toward(&entry->lighting.distant_lights[0].direction,
                                                  &entry->desired_lighting.distant_lights[0].direction, 0.03f);
            halo::render::render_lighting_step_vector3_toward(&entry->lighting.distant_lights[1].color.red,
                                                &entry->desired_lighting.distant_lights[1].color.red, 0.03f);
            halo::render::render_lighting_step_direction_toward(&entry->lighting.distant_lights[1].direction,
                                                  &entry->desired_lighting.distant_lights[1].direction, 0.03f);
            halo::render::render_lighting_step_direction_toward(&entry->lighting.shadow_vector,
                                                  &entry->desired_lighting.shadow_vector, 0.012f);
            halo::render::render_lighting_step_vector3_toward(&entry->lighting.shadow_color.red,
                                                &entry->desired_lighting.shadow_color.red, 0.03f);
        }
    }
    entry->lighting.point_light_count = entry->desired_lighting.point_light_count;
    entry->lighting.point_light_indices[0] = entry->desired_lighting.point_light_indices[0];
    entry->lighting.point_light_indices[1] = entry->desired_lighting.point_light_indices[1];
    entry->last_update_window = render_window_count;
    entry->last_update_frame = render_frame_index;
}

}  // namespace halo::render::object_cache

namespace halo::render::lighting {

/**
 * Disables fixed-function D3D lighting (D3DRS_LIGHTING = 0x89) when the debug toggle is set and
 * the device is older than version 0xffff0101.
 *
 * @address 0x00511ef0
 */
void disable_workaround(void)
{
    if (console_debug_toggle_6893ec != 0 && rasterizer_device_version < d3d9::k_pixel_shader_version_1_1) {
        d3d_set_render_state_fn set_render_state = d3d9::device_function<d3d_set_render_state_fn>(halo::rasterizer::globals().device, d3d9::device_method::set_render_state);
        set_render_state(halo::rasterizer::globals().device, (uint32_t)d3d9::render_state::lighting, 0);
    }
}

/**
 * Steps a 3-component direction toward a target by at most max_delta per component, then
 * renormalizes it. Used to smooth cached lighting directions (render_lighting distant light
 * directions and shadow_vector) frame to frame instead of snapping to the freshly sampled value.
 *
 * @address 0x0050f690
 */
void step_direction_toward(real_vector3d *current, real_vector3d *target, float max_delta)
{
    float delta;
    float step;
    int i;
    float *cur;
    float *tgt;

    cur = (float *)current;
    tgt = (float *)target;
    for (i = 0; i < 3; i++) {
        delta = tgt[i] - cur[i];
        step = -max_delta;
        if (-max_delta <= delta) {
            step = delta;
            if (max_delta < delta) {
                step = max_delta;
            }
        }
        cur[i] = cur[i] + step;
    }
    halo::math::vector3d_normalize_with_length(*current);
}

/**
 * Steps each of 3 floats in `current` toward the matching float in `target` by at most
 * `max_delta`, in place. Used to smooth cached lighting samples frame to frame instead of
 * snapping to the freshly sampled value.
 *
 * @address 0x0050f520
 */
void step_vector3_toward(float *current, float *target, float max_delta)
{
    float delta;
    float step;
    int i;

    for (i = 0; i < 3; i++) {
        delta = target[i] - current[i];
        step = -max_delta;
        if (-max_delta <= delta) {
            step = delta;
            if (max_delta < delta) {
                step = max_delta;
            }
        }
        current[i] = current[i] + step;
    }
}

/**
 * Steps each of 4 floats in `current` toward the matching float in `target` by at most
 * `max_delta`, in place. Used to smooth cached lighting samples (render_lighting.reflection_tint)
 * frame to frame instead of snapping to the freshly sampled value.
 *
 * @address 0x0050f5c0
 */
void step_vector4_toward(float *current, float *target, float max_delta)
{
    float delta;
    float step;
    int i;

    for (i = 0; i < 4; i++) {
        delta = target[i] - current[i];
        step = -max_delta;
        if (-max_delta <= delta) {
            step = delta;
            if (max_delta < delta) {
                step = max_delta;
            }
        }
        current[i] = current[i] + step;
    }
}

}  // namespace halo::render::lighting

namespace halo::render::frame {

/**
 * Returns true unconditionally for a normal camera (camera_get_type_for_player == 0). For any
 * other camera type, returns true only if the given local player has a unit, that unit's parent
 * object's Unit/Vehicle tag has a seat matching the unit's own vehicle_seat_index, and that seat
 * is a gunner seat.
 *
 * @address 0x0050fcd0
 */
int16_t local_player_gunner_seat_visible(int16_t local_player_index)
{
    datum_index player_index;
    object_header *unit_header;
    object *unit;
    unit_data *unit_ext;
    datum_index parent_index;
    object_header *parent_header;
    object *parent;
    tag_instance *vehicle_tag;
    Unit *vehicle;
    UnitSeat *seats;
    int16_t seat_index;

    if (halo::camera::camera_get_type_for_player(local_player_index) == 0) {
        return 1;
    }

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = k_datum_index_none;
    } else {
        player_index = halo::game::globals().local_player_globals->local_players[local_player_index];
    }

    player_index = ((player *)halo::game::globals().player_data->data)[(uint16_t)player_index].unit;
    if (player_index == k_datum_index_none) {
        return 0;
    }

    unit_header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)player_index];
    unit = unit_header->data;
    parent_index = unit->parent_object;
    if (parent_index == k_datum_index_none) {
        return 0;
    }
    unit_ext = (unit_data *)((uint8_t *)unit + k_unit_data_offset);
    seat_index = unit_ext->vehicle_seat_index;
    if (seat_index == -1) {
        return 0;
    }

    parent_header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)parent_index];
    parent = parent_header->data;
    vehicle_tag = &halo::cache::globals().tag_instances[(uint16_t)parent->definition_tag];
    vehicle = (Unit *)vehicle_tag->data;
    seats = (UnitSeat *)vehicle->seats.pointer;

    return (seats[seat_index].flags & 8) != 0;
}

}  // namespace halo::render::frame

namespace halo::render::object_pass {

/**
 * Frustum culling sphere of an object for render_objects_collect: its bounding centre and the
 * render bounding radius of its Object tag.
 *
 * @address 0x0050e8d0
 */
void _get_cull_sphere(datum_index object_index, real_point3d *center, float *radius)
{
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)object_index];
    object *o = header->data;
    Object *definition;

    *center = o->bounding_center;
    definition = (Object *)halo::cache::globals().tag_instances[(uint16_t)o->definition_tag].data;
    *radius = definition->render_bounding_radius;
}

/**
 * Tests whether `object` is the object the camera is currently effectively looking through: the
 * local player's own driven unit (when there is exactly one local player and the camera is in its
 * normal, non-cinematic mode), or, failing that, the director/theater camera's own tracked target
 * object (when that mode is active and set to mode 2). Used to exclude that object from work that
 * should not apply to whatever the viewer is currently inside of (e.g. the fake object shadow
 * candidate list built by render_objects_collect, 0x50eac0).
 *
 * @address 0x0050ea50
 */
uint8_t _is_camera_unit(datum_index object)
{
    int16_t local_player_index = halo::interface::globals().current_local_player_index;
    datum_index local_unit = k_datum_index_none;

    if (local_player_index != -1 && local_player_index < 1) {
        datum_index local_player = halo::game::globals().local_player_globals->local_players[local_player_index];
        if (local_player != k_datum_index_none) {
            player *p = &((player *)halo::game::globals().player_data->data)[(uint16_t)local_player];
            local_unit = p->unit;
        }
    }

    if (local_unit == object && ((int16_t (*)(int16_t player_index))halo::camera::camera_get_type_for_player)(local_player_index) == 0) {
        return 1;
    }
    if (halo::camera::globals().camera_script.camera_control != 0 && halo::camera::globals().camera_script.mode == 2 && halo::camera::globals().camera_script.object == object) {
        return 1;
    }
    return 0;
}

/**
 * Per-frame object render driver: optionally forces D3D lighting off around the whole pass on
 * pre-0xffff0101 devices when the debug toggle is set, rebuilds the nearby-object candidate list,
 * then runs a two-iteration loop where exactly one iteration (selected by
 * console_debug_toggle_6893ee) calls render_object once per rendered object and the other calls
 * the foreign first_person_weapon_update_lighting instead.
 *
 * @address 0x0050e930
 */
void s(void)
{
    object_render_data data;
    uint8_t pass;
    uint8_t first_iteration;

    if (halo::rasterizer::fields::models_enabled != 0) {
        rasterizer_render_states_dirty = 1;
        halo::rasterizer::fields::sky_pass_active = 0;
        if (rasterizer_device_version < d3d9::k_pixel_shader_version_1_1) {
            d3d_set_render_state_fn set_render_state = d3d9::device_function<d3d_set_render_state_fn>(halo::rasterizer::globals().device, d3d9::device_method::set_render_state);
            set_render_state(halo::rasterizer::globals().device, (uint32_t)d3d9::render_state::lighting, 1);
        }
    }

    halo::render::render_objects_collect();

    data.shadow_pass = 0;
    pass = 0;
    do {
        if (pass == console_debug_toggle_6893ee) {
            int16_t i;
            for (i = 0; i < rendered_object_count; i++) {
                data.object_index = rendered_objects[i];
                halo::render::render_object(&data);
            }
        } else {
            halo::interface::first_person_weapon_update_lighting();
        }
        first_iteration = (pass == 0);
        pass = 1;
    } while (first_iteration);

    if (console_debug_toggle_6893ec != 0 && rasterizer_device_version < d3d9::k_pixel_shader_version_1_1) {
        d3d_set_render_state_fn set_render_state = d3d9::device_function<d3d_set_render_state_fn>(halo::rasterizer::globals().device, d3d9::device_method::set_render_state);
        set_render_state(halo::rasterizer::globals().device, (uint32_t)d3d9::render_state::lighting, 0);
    }
}

/**
 * Rebuilds rendered_objects: every object in a visible cluster whose render bounding sphere
 * passes the frustum test, collideable objects first, capped at 0x100 entries. Latches
 * rendered_objects_full_warning the first time the list fills up.
 *
 * @address 0x0050eac0
 */
void s_collect(void)
{
    int16_t count;

    halo::physics::globals().object_cluster_stamp++;
    halo::objects::globals().object_globals->collecting_in_clusters = 1;

    count = halo::structures::structure_bsp_collect_visible_objects((int32_t *)rendered_objects, k_maximum_rendered_objects,
        (structure_bsp_object_iterate_begin_fn)halo::objects::object_resolve_collideable_reference,
        (structure_bsp_object_iterate_next_fn)halo::objects::object_cluster_collideable_iterate_next,
        (structure_bsp_object_get_bounds_fn)halo::render::render_object_get_cull_sphere,
        (structure_bsp_object_predicate_fn)halo::objects::object_disconnect_from_map,
        (structure_bsp_object_accept_fn)halo::objects::object_cluster_stamp_mark_visited);
    rendered_object_count = count;

    rendered_object_count += halo::structures::structure_bsp_collect_visible_objects((int32_t *)(&rendered_objects[count]),
        (int16_t)(k_maximum_rendered_objects - rendered_object_count),
        (structure_bsp_object_iterate_begin_fn)halo::objects::object_cluster_noncollideable_iterate_begin,
        (structure_bsp_object_iterate_next_fn)halo::objects::object_cluster_noncollideable_iterate_next,
        (structure_bsp_object_get_bounds_fn)halo::render::render_object_get_cull_sphere,
        (structure_bsp_object_predicate_fn)halo::objects::object_disconnect_from_map,
        (structure_bsp_object_accept_fn)halo::objects::object_cluster_stamp_mark_visited);

    halo::objects::globals().object_globals->collecting_in_clusters = 0;

    if (rendered_object_count == k_maximum_rendered_objects && !rendered_objects_full_warning) {
        rendered_objects_full_warning = 1;
    }
}

}  // namespace halo::render::object_pass
