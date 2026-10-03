#include "halo/tags/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/light_volume.hpp"
#include "halo/core/lcg.hpp"
#include "halo/bitmaps/api.hpp"
#include "bitmaps.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/effects/api.hpp"

static auto &camera_forward_x = halo::link::ref<float>(halo::effects::vars().camera_forward_x);
static auto &camera_forward_y = halo::link::ref<float>(halo::objects::vars().camera_forward_y);
static auto &camera_forward_z = halo::link::ref<float>(halo::objects::vars().camera_forward_z);
static auto &camera_position_y = halo::link::ref<float>(halo::ui::vars().camera_position_y);
static auto &camera_position_z = halo::link::ref<float>(halo::effects::vars().camera_position_z);
static auto &global_white_color = halo::link::ref<real_vector3d *>(halo::effects::vars().global_white_color);
static auto &light_volume_instances = halo::link::ref<data_array *>(halo::objects::vars().light_volume_instances);
static auto &lightning_instances = halo::link::ref<data_array *>(halo::objects::vars().lightning_instances);
static auto &render_camera_global = halo::link::ref<float>(halo::render::vars().render_camera_global);
static auto &shared_constant_vector_696704 = halo::link::ref<real_vector3d *>(halo::objects::vars().shared_constant_vector_696704);

/**
 * Creates the light volume data array.
 *
 * Original register convention: none.
 *
 * @address 0x004fe680
 */
void halo::objects::LightVolumeSystem::initialize()
{
    light_volume_instances = halo::saved_games::game_state_new((char *)"light volumes", 0x100, 8);
}

/**
 * Disposes the light volume data array.
 *
 * Original register convention: none.
 *
 * @address 0x004fe6a0
 */
void halo::objects::LightVolumeSystem::dispose()
{
    if (light_volume_instances != 0) {
        light_volume_instances->valid = 1;
        halo::memory::data_delete_all(light_volume_instances);
    }
}

/**
 * Clears the disposing flag of the light volume data array after a dispose pass.
 *
 * Original register convention: none.
 *
 * @address 0x004fe6c0
 */
void halo::objects::LightVolumeSystem::clear_disposing_flag()
{
    if (light_volume_instances != 0) {
        light_volume_instances->valid = 0;
    }
}

namespace {
static void *datum_try_get(data_array *array, datum_index index)
{
    int16_t absolute = (int16_t)index;
    int16_t salt;

    if (absolute < 0 || absolute >= array->last_index) {
        return 0;
    }
    salt = *(int16_t *)((uint8_t *)array->data + absolute * array->size);
    if (salt == 0 || ((int16_t)(index >> 16) != 0 && (int16_t)(index >> 16) != salt)) {
        return 0;
    }
    return (uint8_t *)array->data + absolute * array->size;
}
}

/**
 * Allocates a light volume datum for a LightVolume tag and returns its handle.
 *
 * Original register convention: stack -> definition_tag (cdecl).
 *
 * @address 0x004fe6d0
 */
datum_index halo::objects::LightVolumeSystem::create(datum_index definition_tag)
{
    datum_index index = halo::memory::datum_new(light_volume_instances);

    if (index != k_datum_index_none) {
        static_cast<effect_widget_instance *>(datum_try_get(light_volume_instances, index))->definition_tag = definition_tag;
    }
    return index;
}

/**
 * Frees a light volume datum.
 *
 * Original register convention: stack -> light_volume_index (cdecl).
 *
 * @address 0x004fe720
 */
void halo::objects::LightVolumeSystem::destroy(datum_index light_volume_index)
{
    if (light_volume_index != k_datum_index_none) {
        halo::memory::datum_delete(light_volume_instances, light_volume_index);
    }
}

/**
 * Widget render hook for a light volume.
 *
 * Original register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is never read
 * anywhere in the body. Follows the same handle-validation shape as glow_render_dispatch.c's param_2.
 *
 * @address 0x004fe900
 */
void halo::objects::LightVolumeSystem::render(uint32_t object_index, datum_index light_volume_handle, uint32_t unused,
    uint8_t *function_context)
{
    if (object_index == k_datum_index_none || light_volume_handle == k_datum_index_none) {
        return;
    }

    effect_widget_instance *instance = static_cast<effect_widget_instance *>(datum_try_get(light_volume_instances, light_volume_handle));

    {
        LightVolume *tag = halo::objects::tag_as<LightVolume>(instance->definition_tag);

        if (tag->count > 0 && tag->frames.count > 0 &&
            (tag->brightness_scale_source == 0 || function_context == 0 ||
             *(float *)(*(int32_t *)(function_context + 4) - 4 + tag->brightness_scale_source * 4) > 0.0f)) {
            object_marker marker;

            halo::objects::object_get_node_local_transform(object_index, tag->attachment_marker.string, &marker, 1);

            if (tag->far_fade_distance == 0.0f ||
                camera_forward_y * (marker.node_transform.position.y - camera_position_y) +
                camera_forward_x * (marker.node_transform.position.x - render_camera_global) +
                camera_forward_z * (marker.node_transform.position.z - camera_position_z) <
                tag->far_fade_distance) {

                halo::rasterizer::rasterizer_lens_flare_occlusion_sample_add((void *)halo::objects::light_volume_render_procedure,
                    &marker.node_transform.position, object_index, light_volume_handle);
            }
        }
    }
}

namespace {
static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}
}

/**
 * Builds and submits the sprite strip for a light volume, interpolating colours along its length.
 *
 * Original register convention: stack -> object_index, light_volume_handle.
 *
 * @address 0x004fea80
 */
void halo::objects::LightVolumeSystem::render_procedure(uint32_t object_index, datum_index light_volume_handle)
{
    effect_widget_instance *instance;
    LightVolume *tag;
    LightVolumeFrame *frame;
    object_marker marker;
    real_vector3d *forward;
    real_point3d *origin;
    float facing;
    float fade = 1.0f;
    float function_value = 1.0f;
    float brightness;

    if (object_index == k_datum_index_none || light_volume_handle == k_datum_index_none) {
        return;
    }
    instance = static_cast<effect_widget_instance *>(datum_try_get(light_volume_instances, light_volume_handle));
    tag = halo::objects::tag_as<LightVolume>(instance->definition_tag);
    if (tag->count <= 0 || tag->frames.count <= 0) {
        return;
    }
    frame = halo::objects::object_attachment_get_blended_marker(object_index, tag);
    halo::objects::object_get_node_local_transform(object_index, tag->attachment_marker.string, &marker, 1);
    forward = &marker.node_transform.forward;
    origin = &marker.node_transform.position;

    facing = forward->k * camera_forward_z + forward->j * camera_forward_y + forward->i * camera_forward_x;
    if (facing < 0.0f) {
        facing = -facing;
    }
    if (tag->far_fade_distance > 0.0f) {
        float distance = (origin->z - camera_position_z) * camera_forward_z +
            (origin->x - render_camera_global) * camera_forward_x + camera_forward_y * (origin->y - camera_position_y);

        fade = clamp01((distance - tag->far_fade_distance) / (tag->near_fade_distance - tag->far_fade_distance));
    }
    brightness = clamp01((1.0f - facing) * tag->perpendicular_brightness_scale + facing * tag->parallel_brightness_scale) * fade;
    if (halo::objects::object_function_get_value(object_index, (int16_t)(tag->brightness_scale_source - 1), &function_value)) {
        brightness *= function_value;
    }
    if (brightness <= 0.0f) {
        return;
    }
    if (frame->tint_color_hither.alpha <= 0.0f && frame->tint_color_yon.alpha <= 0.0f) {
        return;
    }
    if (frame->radius_hither <= 0.0f && frame->radius_yon <= 0.0f) {
        return;
    }

    halo::rasterizer::rasterizer_lens_flare_batching_select_mode(5, 1);
    if (halo::render::rasterizer_lens_flare_set_current_key(halo::objects::tag_handle(tag->map), 0, (int16_t)tag->sequence_index) == 0 &&
        tag->count > 0) {
        int16_t count = tag->count;
        float last = (float)(count - 1);
        int32_t i;

        for (i = 0; i < count; i++) {
            float t = halo::objects::curve_apply_exponent((float)i / last, frame->offset_exponent);
            float radius_t = halo::objects::curve_apply_exponent(t, frame->radius_exponent);
            float radius = (1.0f - radius_t) * frame->radius_hither + radius_t * frame->radius_yon;
            float alpha_t = halo::objects::curve_apply_exponent(t, frame->brightness_exponent);
            float along = t * frame->length + frame->offset_from_marker;
            real_point3d point;
            ColorARGB color;
            float color_t;

            point.x = forward->i * along + origin->x;
            point.y = forward->j * along + origin->y;
            point.z = forward->k * along + origin->z;
            color_t = halo::objects::curve_apply_exponent(t, frame->tint_color_exponent);
            halo::bitmaps::color_interpolate((ColorRGB *)(reinterpret_cast<uint8_t *>(frame) + 0x7c), (ColorRGB *)(reinterpret_cast<uint8_t *>(frame) + 0x6c), (ColorRGB *)&color.red,
                (color_interpolation_flags)(tag->flags & 3), color_t);
            color.alpha = ((1.0f - alpha_t) * frame->tint_color_hither.alpha + alpha_t * frame->tint_color_yon.alpha) *
                brightness;
            halo::rasterizer::rasterizer_lens_flare_quad_add(0, halo::interface::color_pack_argb_from_real(&color), &point, radius, 0.0f);
        }
    }
    halo::render::rasterizer_effect_slot_release_active();
}

/**
 * Creates the lightning data array.
 *
 * Original register convention: none.
 *
 * @address 0x004fee80
 */
void halo::objects::LightningSystem::initialize()
{
    lightning_instances = halo::saved_games::game_state_new((char *)"lightnings", 0x100, 8);
}

/**
 * Disposes the lightning data array.
 *
 * Original register convention: none.
 *
 * @address 0x004feea0
 */
void halo::objects::LightningSystem::dispose()
{
    if (lightning_instances != 0) {
        lightning_instances->valid = 1;
        halo::memory::data_delete_all(lightning_instances);
    }
}

/**
 * Clears the disposing flag of the lightning data array after a dispose pass.
 *
 * Original register convention: none.
 *
 * @address 0x004feec0
 */
void halo::objects::LightningSystem::clear_disposing_flag()
{
    if (lightning_instances != 0) {
        lightning_instances->valid = 0;
    }
}

/**
 * Allocates a lightning datum for a Lightning tag and returns its handle.
 *
 * Original register convention: stack -> definition_tag (cdecl).
 *
 * @address 0x004feed0
 */
datum_index halo::objects::LightningSystem::create(datum_index definition_tag)
{
    datum_index index = halo::memory::datum_new(lightning_instances);

    if (index != k_datum_index_none) {
        static_cast<effect_widget_instance *>(datum_try_get(lightning_instances, index))->definition_tag = definition_tag;
    }
    return index;
}

/**
 * Frees a lightning datum.
 *
 * Original register convention: stack -> lightning_index (cdecl).
 *
 * @address 0x004fef20
 */
void halo::objects::LightningSystem::destroy(datum_index lightning_index)
{
    if (lightning_index != k_datum_index_none) {
        halo::memory::datum_delete(lightning_instances, lightning_index);
    }
}

namespace {
static uint32_t (*const color_pack_argb_from_real__as_lightning_render)(float *argb) = reinterpret_cast<uint32_t (*)(float *argb)>(&halo::interface::color_pack_argb_from_real);
static float glow_random_unit_for_lightning(void)
{
    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
    return (float)(halo::math::globals().effect_random_seed >> halo::k_random_high_shift) * halo::k_unit_word_scale;
}
}

/**
 * Builds and submits the jittered lightning segments for a lightning widget.
 *
 * Original register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is never read. No
 * implicit register inputs.
 *
 * @address 0x004ff010
 */
void halo::objects::LightningSystem::render(uint32_t object_index, datum_index lightning_handle, uint32_t unused,
    int32_t *function_context)
{
    Lightning *tag;

    if (object_index == k_datum_index_none || lightning_handle == k_datum_index_none) {
        return;
    }

    {
        effect_widget_instance *instance = static_cast<effect_widget_instance *>(datum_try_get(lightning_instances, lightning_handle));

        tag = halo::objects::tag_as<Lightning>(instance->definition_tag);
    }

    if (tag->markers.count <= 0) {
        return;
    }

    {
        object_marker root_marker;
        int16_t markers_ok = (int16_t)halo::objects::object_get_node_local_transform(
            object_index, halo::objects::block_elements<LightningMarker>(tag->markers)->attachment_marker.string, &root_marker, 1);
        if (markers_ok <= 0) {
            return;
        }
    }

    {
        uint32_t shader_something = *(uint32_t *)(
            halo::objects::tag_record_bytes(halo::objects::tag_handle(tag->bitmap)) + 100);
        int32_t device = (int32_t)(uintptr_t)halo::cache::texture_cache_get((BitmapData *)(uintptr_t)shader_something, 0, 1);

        int16_t shard;
        if (device == 0) {
            return;
        }
        for (shard = 0; shard < tag->count; shard++) {

            static float verts[32776];

            int32_t node_count = 0;
            float brightness_scale = 1.0f;
            int first_marker = 1;
            int32_t marker_index;

            if (function_context != 0 && function_context[1] != 0) {
                int16_t sel = tag->jitter_scale_source;
                if (sel > 0 && sel < 5) {
                    brightness_scale = *(float *)(function_context[1] - 4 + sel * 4);
                }
            }

            for (marker_index = 0; marker_index < tag->markers.count; marker_index++) {
                LightningMarker *marker_tag = &halo::objects::block_element<LightningMarker>(tag->markers, marker_index);

                if (first_marker) {
                    object_marker m;
                    real_point3d pos;
                    halo::objects::object_get_node_local_transform(object_index, marker_tag->attachment_marker.string, &m, 1);
                    pos = m.node_transform.position;
                    verts[1] = pos.y;
                    verts[2] = pos.z;
                    halo::objects::antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c)  ,
                                        &pos, &m.node_transform);
                    node_count = 0;
                    first_marker = 0;
                }

                if (!test_flag(marker_tag->flags, tags::lightning_marker_flag_tag_flag::not_connected_to_next_marker) && marker_index != tag->markers.count - 1) {
                    uint16_t octaves = marker_tag->octaves_to_next_marker;
                    LightningMarker *next_marker_tag = reinterpret_cast<LightningMarker *>(reinterpret_cast<uint8_t *>(marker_tag) + 0xe4);
                    object_marker m;
                    real_point3d pos;
                    int32_t base = node_count;
                    int32_t end = base + (1 << (octaves & 0x1f));

                    halo::objects::object_get_node_local_transform(object_index, next_marker_tag->attachment_marker.string, &m, 1);
                    pos = m.node_transform.position;

                    verts[end * 8 + 0] = pos.x;
                    verts[end * 8 + 1] = pos.y;
                    verts[end * 8 + 2] = pos.z;
                    halo::objects::antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c), &pos, &m.node_transform);
                    verts[end * 8 + 3] = next_marker_tag->thickness;
                    verts[end * 8 + 4] = next_marker_tag->tint.alpha;
                    verts[end * 8 + 5] = next_marker_tag->tint.red;
                    verts[end * 8 + 6] = next_marker_tag->tint.green;
                    verts[end * 8 + 7] = next_marker_tag->tint.blue;

                    {
                        real_vector3d axis;
                        axis.i = verts[end * 8 + 0] - verts[base * 8 + 0];
                        axis.j = verts[end * 8 + 1] - verts[base * 8 + 1];
                        axis.k = verts[end * 8 + 2] - verts[base * 8 + 2];

                        halo::math::vector3d_cross_product(axis, *((const real_vector3d *)&camera_forward_x), axis);
                        if (halo::math::vector3d_normalize_with_length(axis) == 0.0f) {
                            axis = *shared_constant_vector_696704;
                        }

                    }
                    node_count = node_count + (1 << (marker_tag->octaves_to_next_marker & 0x1f));
                } else {
                    halo::rasterizer::globals().vertex_buffer_lock_state = 0xc;
                    if (node_count > 2) {
                        int32_t n = node_count + 1;
                        int32_t frame = halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(0, 0);
                        if (frame != -1) {
                            float *out = (float *)halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(frame);
                            float t_bias = glow_random_unit_for_lightning();
                            float alpha_scale = 1.0f, color_scale_extra = 1.0f;
                            real_vector3d *color_scale = global_white_color;
                            int32_t i;
                            real_point3d bbmin = {0}, bbmax = {0};

                            if (function_context != 0) {
                                int32_t p1 = function_context[1];
                                int16_t s;
                                if (p1 != 0 && (s = tag->thickness_scale_source) > 0 && s < 5) {
                                    alpha_scale = *(float *)(p1 - 4 + s * 4);
                                }
                                if (function_context[0] != 0 && (s = tag->tint_modulation_source) > 0 && s < 5) {
                                    color_scale = (real_vector3d *)(function_context[0] - 0xc + s * 0xc);
                                }
                                if (p1 != 0 && (s = tag->brightness_scale_source) > 0 && s < 5) {
                                    color_scale_extra = *(float *)(p1 - 4 + s * 4);
                                }
                            }

                            for (i = 0; i < n; i++) {
                                float *v = &verts[i * 8];
                                float *prev = (i < 1) ? v : &verts[(i - 1) * 8];
                                float *next = (i >= n - 1) ? v : &verts[(i + 1) * 8];
                                float half_width = alpha_scale * v[3];
                                real_vector3d normal;
                                float argb[4];
                                uint32_t packed;
                                float *o = out + i * 12;

                                normal.i = (next[1] - prev[1]) * camera_forward_z - (next[2] - prev[2]) * camera_forward_y;
                                normal.j = (next[2] - prev[2]) * camera_forward_x - (next[0] - prev[0]) * camera_forward_z;
                                normal.k = (next[0] - prev[0]) * camera_forward_y - (next[1] - prev[1]) * camera_forward_x;
                                halo::math::vector3d_normalize(normal);

                                argb[0] = color_scale_extra * v[4];
                                argb[1] = v[5] * color_scale->i;
                                argb[2] = v[6] * color_scale->j;
                                argb[3] = v[7] * color_scale->k;
                                packed = color_pack_argb_from_real__as_lightning_render(argb);

                                o[3] = (float)packed;
                                o[5] = 0.0f;
                                o[0] = normal.i * half_width + v[0];
                                o[1] = normal.j * half_width + v[1];
                                o[2] = normal.k * half_width + v[2];
                                o[4] = (float)i * (1.0f / (float)n) + t_bias;
                                half_width = -half_width;
                                o[6] = normal.i * half_width + v[0];
                                o[7] = normal.j * half_width + v[1];
                                o[9] = (float)packed;
                                o[11] = 1.0f;
                                o[8] = normal.k * half_width + v[2];
                                o[10] = o[4];

                                if (i == 0) {
                                    bbmin.x = bbmax.x = v[0];
                                    bbmin.y = bbmax.y = v[1];
                                    bbmin.z = bbmax.z = v[2];
                                } else {
                                    if (v[0] <= bbmin.x) bbmin.x = v[0];
                                    if (v[1] <= bbmin.y) bbmin.y = v[1];
                                    if (v[2] <= bbmin.z) bbmin.z = v[2];
                                    if (bbmax.x < v[0]) bbmax.x = v[0];
                                    if (bbmax.y < v[1]) bbmax.y = v[1];
                                    if (bbmax.z < v[2]) bbmax.z = v[2];
                                }
                            }

                            halo::rasterizer::rasterizer_transparent_object_append(shader_something, n * -2, frame, n * 2 - 2, 0, 0, 0);
                        }
                        first_marker = 1;
                    }
                    halo::rasterizer::globals().vertex_buffer_lock_state = 0;
                }
            }
        }
    }
}
