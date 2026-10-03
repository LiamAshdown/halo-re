#include "halo/objects/light_volume.hpp"
#include "bitmaps.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern void antenna_tip_jitter(real_vector3d *amplitude, real_point3d *position, real_matrix4x3 *m);
extern float camera_forward_x;
extern float camera_forward_y;
extern float camera_forward_z;
extern float camera_position_y;
extern float camera_position_z;
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t);
extern uint32_t color_pack_argb_from_real(ColorARGB *color);
extern float curve_apply_exponent(float value, float exponent);
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern real_vector3d *global_white_color;
extern data_array *light_volume_instances;
extern void light_volume_render_procedure(uint32_t object_index, datum_index light_volume_handle);
extern data_array *lightning_instances;
extern uint8_t *object_attachment_get_blended_marker(uint32_t object_index, uint8_t *instance);
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern void *rasterizer_dynamic_vertex_cache_lock(void);
extern int32_t rasterizer_dynamic_vertex_cache_reserve(void);
extern void rasterizer_effect_slot_release_active(void);
extern void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags);
extern void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1, uint32_t id_2);
extern void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position, float radius, float rotation_degrees);
extern uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index, int16_t bitmap_index);
extern void rasterizer_transparent_object_append(uint32_t a, int32_t b, int32_t c, int32_t d, uint32_t e);
extern int16_t rasterizer_vertex_buffer_lock_state;
extern float render_camera_global;
extern real_vector3d *shared_constant_vector_696704;
}

/**
 * Calls halo::cache::texture_cache_get with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static int32_t texture_cache_get_unresolved(uint32_t a, uint32_t b)
{
    using call_t = int32_t (*)(uint32_t a, uint32_t b);
    return reinterpret_cast<call_t>(&halo::cache::texture_cache_get)(a, b);
}

/**
 * Creates the light volume data array.
 *
 * Original register convention: none.
 *
 * @address 0x004fe680
 */
void halo::objects::LightVolumeSystem::initialize()
{
    light_volume_instances = game_state_new((char *)"light volumes", 0x100, 8);
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
        *(datum_index *)((uint8_t *)datum_try_get(light_volume_instances, index) + 4) = definition_tag;
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

namespace {
static uint8_t * &light_volume_instances__as_light_volume_render = reinterpret_cast<uint8_t * &>(light_volume_instances);
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
    uint8_t *instance;

    if (object_index == 0xffffffff || light_volume_handle == (datum_index)0xffffffff) {
        return;
    }

    {
        int16_t index = (int16_t)light_volume_handle;
        instance = 0;

        if (index >= 0 && index < *(int16_t *)(light_volume_instances__as_light_volume_render + 0x2e)) {
            int32_t off = *(int16_t *)(light_volume_instances__as_light_volume_render + 0x22) * index;
            int16_t identifier = *(int16_t *)(off + *(int32_t *)(light_volume_instances__as_light_volume_render + 0x34));
            int16_t salt = (int16_t)(light_volume_handle >> 16);

            off = off + *(int32_t *)(light_volume_instances__as_light_volume_render + 0x34);
            if (identifier != 0 && (salt == 0 || salt == identifier)) {
                instance = (uint8_t *)off;
            }
        }
    }

    {
        uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data;

        if (*(int16_t *)(tag + 0x6e) > 0 && *(int32_t *)(tag + 0x120) > 0 &&
            (*(int16_t *)(tag + 0x44) == 0 || function_context == 0 ||
             *(float *)(*(int32_t *)(function_context + 4) - 4 + *(int16_t *)(tag + 0x44) * 4) > 0.0f)) {
            object_marker marker;

            object_get_node_local_transform(object_index, (char *)tag, &marker, 1);

            if (*(float *)(tag + 0x38) == 0.0f ||
                camera_forward_y * (marker.node_transform.position.y - camera_position_y) +
                camera_forward_x * (marker.node_transform.position.x - render_camera_global) +
                camera_forward_z * (marker.node_transform.position.z - camera_position_z) <
                *(float *)(tag + 0x38)) {

                rasterizer_lens_flare_occlusion_sample_add((void *)light_volume_render_procedure,
                    &marker.node_transform.position, object_index, light_volume_handle);
            }
        }
    }
}

namespace {
static uint8_t * &light_volume_instances__as_light_volume_render_procedure = reinterpret_cast<uint8_t * &>(light_volume_instances);
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
    uint8_t *instance = 0;
    uint8_t *tag;
    uint8_t *frame;
    object_marker marker;
    real_vector3d *forward;
    real_point3d *origin;
    float facing;
    float fade = 1.0f;
    float function_value = 1.0f;
    float brightness;

    if (object_index == 0xffffffff || light_volume_handle == k_datum_index_none) {
        return;
    }
    {
        int16_t index = (int16_t)light_volume_handle;
        int16_t salt = (int16_t)(light_volume_handle >> 16);

        if (index >= 0 && index < *(int16_t *)(light_volume_instances__as_light_volume_render_procedure + 0x2e)) {
            uint8_t *slot = *(uint8_t **)(light_volume_instances__as_light_volume_render_procedure + 0x34) +
                *(int16_t *)(light_volume_instances__as_light_volume_render_procedure + 0x22) * index;

            if (*(int16_t *)slot != 0 && (salt == 0 || salt == *(int16_t *)slot)) {
                instance = slot;
            }
        }
    }
    tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data;
    if (*(int16_t *)(tag + 0x6e) <= 0 || *(int32_t *)(tag + 0x120) <= 0) {
        return;
    }
    frame = object_attachment_get_blended_marker(object_index, tag);
    object_get_node_local_transform(object_index, (char *)tag, &marker, 1);
    forward = &marker.node_transform.forward;
    origin = &marker.node_transform.position;

    facing = forward->k * camera_forward_z + forward->j * camera_forward_y + forward->i * camera_forward_x;
    if (facing < 0.0f) {
        facing = -facing;
    }
    if (*(float *)(tag + 0x38) > 0.0f) {
        float distance = (origin->z - camera_position_z) * camera_forward_z +
            (origin->x - render_camera_global) * camera_forward_x + camera_forward_y * (origin->y - camera_position_y);

        fade = clamp01((distance - *(float *)(tag + 0x38)) / (*(float *)(tag + 0x34) - *(float *)(tag + 0x38)));
    }
    brightness = clamp01((1.0f - facing) * *(float *)(tag + 0x3c) + facing * *(float *)(tag + 0x40)) * fade;
    if (object_function_get_value(object_index, (int16_t)(*(uint16_t *)(tag + 0x44) - 1), &function_value)) {
        brightness *= function_value;
    }
    if (brightness <= 0.0f) {
        return;
    }
    if (*(float *)(frame + 0x68) <= 0.0f && *(float *)(frame + 0x78) <= 0.0f) {
        return;
    }
    if (*(float *)(frame + 0x3c) <= 0.0f && *(float *)(frame + 0x40) <= 0.0f) {
        return;
    }

    rasterizer_lens_flare_batching_select_mode(5, 1);
    if (rasterizer_lens_flare_set_current_key(*(int32_t *)(tag + 0x68), 0, (int16_t)*(uint16_t *)(tag + 0x6c)) == 0 &&
        *(int16_t *)(tag + 0x6e) > 0) {
        int16_t count = *(int16_t *)(tag + 0x6e);
        float last = (float)(count - 1);
        int32_t i;

        for (i = 0; i < count; i++) {
            float t = curve_apply_exponent((float)i / last, *(float *)(frame + 0x14));
            float radius_t = curve_apply_exponent(t, *(float *)(frame + 0x44));
            float radius = (1.0f - radius_t) * *(float *)(frame + 0x3c) + radius_t * *(float *)(frame + 0x40);
            float alpha_t = curve_apply_exponent(t, *(float *)(frame + 0x8c));
            float along = t * *(float *)(frame + 0x18) + *(float *)(frame + 0x10);
            real_point3d point;
            ColorARGB color;
            float color_t;

            point.x = forward->i * along + origin->x;
            point.y = forward->j * along + origin->y;
            point.z = forward->k * along + origin->z;
            color_t = curve_apply_exponent(t, *(float *)(frame + 0x88));
            color_interpolate((ColorRGB *)(frame + 0x7c), (ColorRGB *)(frame + 0x6c), (ColorRGB *)&color.red,
                (color_interpolation_flags)(tag[0x22] & 3), color_t);
            color.alpha = ((1.0f - alpha_t) * *(float *)(frame + 0x68) + alpha_t * *(float *)(frame + 0x78)) *
                brightness;
            rasterizer_lens_flare_quad_add(0, color_pack_argb_from_real(&color), &point, radius, 0.0f);
        }
    }
    rasterizer_effect_slot_release_active();
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
    lightning_instances = game_state_new((char *)"lightnings", 0x100, 8);
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
        *(datum_index *)((uint8_t *)datum_try_get(lightning_instances, index) + 4) = definition_tag;
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
static uint8_t * &lightning_instances__as_lightning_render = reinterpret_cast<uint8_t * &>(lightning_instances);
static uint32_t (*const color_pack_argb_from_real__as_lightning_render)(float *argb) = reinterpret_cast<uint32_t (*)(float *argb)>(&color_pack_argb_from_real);
static float glow_random_unit_for_lightning(void)
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660dU + 0x3c6ef35fU;
    return (float)(halo::math::globals().effect_random_seed >> 16) * 1.5259022e-05f;
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
    uint8_t *tag;

    if (object_index == 0xffffffff || lightning_handle == (datum_index)0xffffffff) {
        return;
    }

    {
        int16_t index = (int16_t)lightning_handle;
        uint8_t *instance = 0;

        if (index >= 0 && index < *(int16_t *)(lightning_instances__as_lightning_render + 0x2e)) {
            int32_t off = *(int16_t *)(lightning_instances__as_lightning_render + 0x22) * index;
            int16_t identifier = *(int16_t *)(off + *(int32_t *)(lightning_instances__as_lightning_render + 0x34));
            int16_t salt = (int16_t)(lightning_handle >> 16);

            off = off + *(int32_t *)(lightning_instances__as_lightning_render + 0x34);
            if (identifier != 0 && (salt == 0 || salt == identifier)) {
                instance = (uint8_t *)off;
            }
        }
        tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data;
    }

    if (*(int32_t *)(tag + 0x98) <= 0) {
        return;
    }

    {
        object_marker root_marker;
        int16_t markers_ok = (int16_t)object_get_node_local_transform(
            object_index, (char *)*(uint32_t *)(tag + 0x9c), &root_marker, 1);
        if (markers_ok <= 0) {
            return;
        }
    }

    {
        uint32_t shader_something = *(uint32_t *)(
            (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)(tag + 0x40) & 0xffff].data + 100);
        int32_t device = texture_cache_get_unresolved(0, 1);

        int16_t shard;
        if (device == 0) {
            return;
        }
        for (shard = 0; shard < *(int16_t *)(tag + 2); shard++) {

            static float verts[32776];

            int32_t node_count = 0;
            float brightness_scale = 1.0f;
            int first_marker = 1;
            int32_t marker_index;

            if (function_context != 0 && function_context[1] != 0) {
                int16_t sel = *(int16_t *)(tag + 0x2c);
                if (sel > 0 && sel < 5) {
                    brightness_scale = *(float *)(function_context[1] - 4 + sel * 4);
                }
            }

            for (marker_index = 0; marker_index < *(int32_t *)(tag + 0x98); marker_index++) {
                uint8_t *marker_tag = *(uint8_t **)(tag + 0x9c) + marker_index * 0xe4;

                if (first_marker) {
                    object_marker m;
                    real_point3d pos;
                    object_get_node_local_transform(object_index, (char *)marker_tag, &m, 1);
                    pos = m.node_transform.position;
                    verts[1] = pos.y;
                    verts[2] = pos.z;
                    antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c)  ,
                                        &pos, &m.node_transform);
                    node_count = 0;
                    first_marker = 0;
                }

                if ((marker_tag[0x20] & 1) == 0 && marker_index != *(int32_t *)(tag + 0x98) - 1) {
                    uint16_t octaves = *(uint16_t *)(marker_tag + 0x24);
                    uint8_t *next_marker_tag = marker_tag + 0xe4;
                    object_marker m;
                    real_point3d pos;
                    int32_t base = node_count;
                    int32_t end = base + (1 << (octaves & 0x1f));

                    object_get_node_local_transform(object_index, (char *)next_marker_tag, &m, 1);
                    pos = m.node_transform.position;

                    verts[end * 8 + 0] = pos.x;
                    verts[end * 8 + 1] = pos.y;
                    verts[end * 8 + 2] = pos.z;
                    antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c), &pos, &m.node_transform);
                    verts[end * 8 + 3] = *(float *)(next_marker_tag + 0x84);
                    verts[end * 8 + 4] = *(float *)(next_marker_tag + 0x88);
                    verts[end * 8 + 5] = *(float *)(next_marker_tag + 0x8c);
                    verts[end * 8 + 6] = *(float *)(next_marker_tag + 0x90);
                    verts[end * 8 + 7] = *(float *)(next_marker_tag + 0x94);

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
                    node_count = node_count + (1 << (marker_tag[0x24] & 0x1f));
                } else {
                    rasterizer_vertex_buffer_lock_state = 0xc;
                    if (node_count > 2) {
                        int32_t n = node_count + 1;
                        int32_t frame = rasterizer_dynamic_vertex_cache_reserve();
                        if (frame != -1) {
                            float *out = (float *)rasterizer_dynamic_vertex_cache_lock();
                            float t_bias = glow_random_unit_for_lightning();
                            float alpha_scale = 1.0f, color_scale_extra = 1.0f;
                            real_vector3d *color_scale = global_white_color;
                            int32_t i;
                            real_point3d bbmin = {0}, bbmax = {0};

                            if (function_context != 0) {
                                int32_t p1 = function_context[1];
                                int16_t s;
                                if (p1 != 0 && (s = *(int16_t *)(tag + 0x2e)) > 0 && s < 5) {
                                    alpha_scale = *(float *)(p1 - 4 + s * 4);
                                }
                                if (function_context[0] != 0 && (s = *(int16_t *)(tag + 0x30)) > 0 && s < 5) {
                                    color_scale = (real_vector3d *)(function_context[0] - 0xc + s * 0xc);
                                }
                                if (p1 != 0 && (s = *(int16_t *)(tag + 0x32)) > 0 && s < 5) {
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

                            rasterizer_transparent_object_append(shader_something, n * -2, frame, n * 2 - 2, 0);
                        }
                        first_marker = 1;
                    }
                    rasterizer_vertex_buffer_lock_state = 0;
                }
            }
        }
    }
}
