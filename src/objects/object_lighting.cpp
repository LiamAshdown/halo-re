#include "halo/objects/object_lighting.hpp"
#include "halo/bitmaps/api.hpp"
#include "structures.h"
#include "rasterizer.h"
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"

extern "C" {
extern real_vector3d *default_axis_b;
extern ScenarioStructureBSP *global_structure_bsp;
extern data_array *light_data;
extern int32_t light_frame_counter;
extern uint8_t light_render_unknown_7c0;
extern real_vector3d *object_ambient_lightmap_default;
extern void object_color_clamp_to_intensity(float intensity, ColorRGB *color);
extern data_array *object_data;
extern int16_t object_get_root_parent_placement(uint32_t object_index, object_placement_cursor *out_cursor);
extern void object_light_clear_dirty_flag(uint32_t light_index);
extern void object_light_recompute_transform(uint32_t light_index);
extern float object_lighting_ambient_bias;
extern float object_lighting_ambient_scale;
extern float object_lighting_base_light_scale;
extern real_vector3d object_lightmap_probe_direction;
extern void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index, real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities, uint32_t out_falloffs, int16_t *count, int16_t max_count);
extern real object_sum_attached_light_luminance(uint32_t object_index);
extern double pow(double x, double y);
extern double sqrt(double x);
}

/**
 * Returns the summed luminance of the lights attached to an object.
 *
 * Original register convention: stack -> object_index.
 *
 * @address 0x004f1b30
 */
real halo::objects::ObjectLighting::sum_attached_light_luminance()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
    int32_t attachment_count = definition->attachments.count;
    real total = 0.0f;
    int32_t i;

    for (i = 0; i < attachment_count; i++) {
        if (obj->attachment_types[i] == _object_attachment_type_light &&
            obj->attachment_handles[i] != (datum_index)0xffffffff) {
            light *l = &((light *)light_data->data)[obj->attachment_handles[i] & 0xffff];

            total = (*(float *)((uint8_t *)l + 0x1c) * 0.114f +
                     *(float *)((uint8_t *)l + 0x18) * 0.587f +
                     *(float *)((uint8_t *)l + 0x14) * 0.299f) + total;
        }
    }

    if (obj->first_child_object != (datum_index)0xffffffff) {
        total = object_sum_attached_light_luminance(obj->first_child_object) + total;
    }
    if (obj->next_object != (datum_index)0xffffffff) {
        total = object_sum_attached_light_luminance(obj->next_object) + total;
    }

    return total;
}

/**
 * Samples the total lighting colour at a point in a BSP location.
 *
 * Original register convention: plain cdecl. // blam-cc: stack -> (point, location, color).
 *
 * @address 0x004f1c20
 */
void halo::objects::ObjectLighting::sample_total_lighting_at_point(real_point3d *point, bsp_leaf_reference *location,
    real_vector3d *color)
{
    real_point3d contact;
    int16_t lightmap_index;
    int16_t material_index;
    int32_t surface_index;
    float weight_1;
    float weight_2;
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;

    *color = *default_axis_b;

    if (halo::structures::structure_bsp_resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2,
            &object_lightmap_probe_direction, &material_index, &surface_index, &weight_1)) {
        bsp = global_structure_bsp;
        lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
        material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;

        if (*(int32_t *)&bsp->lightmaps_bitmap.tag_id != -1 && (int16_t)lightmap->bitmap != -1) {
            BitmapData *bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
                (int16_t)lightmap->bitmap);
            uint16_t *triangle =
                (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);

            if (halo::cache::texture_cache_get(bitmap, 0, 0) != 0) {
                halo::structures::bsp_lightmap_sample_vertex_color(bitmap, weight_1, weight_2, (ColorRGB *)color, material, triangle);
            }
        }
    }

    if (location->cluster_index != -1) {
        uint32_t indices[2];
        float scores[2];
        float weights[2];
        int16_t count = 0;
        int32_t i;

        light_frame_counter++;
        light_render_unknown_7c0 = 1;
        object_lights_gather_nearest(location->cluster_index, 0xffffffff, point, 0.0f, indices, scores,
            (uint32_t)(uintptr_t)weights, &count, 2);
        light_render_unknown_7c0 = 0;

        for (i = 0; i < count; i++) {
            uint8_t *entry = (uint8_t *)light_data->data + (indices[i] & 0xffff) * 0x7c;

            if (*(entry + 2) & _light_always_visible_bit) {
                color->i += *(float *)(entry + 0x14) * weights[i];
                color->j += *(float *)(entry + 0x18) * weights[i];
                color->k += *(float *)(entry + 0x1c) * weights[i];
            }
        }
    }

    if (color->i < 0.0f) {
        color->i = 0.0f;
    } else if (color->i > 1.0f) {
        color->i = 1.0f;
    }
    if (color->j < 0.0f) {
        color->j = 0.0f;
    } else if (color->j > 1.0f) {
        color->j = 1.0f;
    }
    if (color->k < 0.0f) {
        color->k = 0.0f;
    } else if (color->k > 1.0f) {
        color->k = 1.0f;
    }
}

namespace {
static void *object_lightmap_texture_ready(BitmapData *bitmap, uint8_t wait_for_textures)
{
    void *texture = 0;

    if (wait_for_textures) {
        texture = halo::cache::texture_cache_get(bitmap, 1, 1);
    }
    if (texture == 0) {
        texture = halo::cache::texture_cache_get(bitmap, 0, 0);
    }
    return texture;
}
}

/**
 * Samples the lightmap colour and base map colour at a point.
 *
 * Original register convention: plain cdecl, four stack parameters. // blam-cc: stack -> (point, lightmap_color,
 * base_map_color, wait_for_textures).
 *
 * @address 0x004f1e60
 */
void halo::objects::ObjectLighting::sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures)
{
    real_point3d contact;
    int16_t lightmap_index;
    int16_t material_index;
    int32_t surface_index;
    float weight_1;
    float weight_2;
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    uint8_t *shader;
    uint8_t *base_map_tag;
    datum_index base_map;
    BitmapData *lightmap_bitmap;
    BitmapData *base_map_bitmap;
    uint16_t *triangle = 0;

    *lightmap_color = *object_ambient_lightmap_default;
    *base_map_color = *object_ambient_lightmap_default;

    if (!halo::structures::structure_bsp_resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2,
            &object_lightmap_probe_direction, &material_index, &surface_index, &weight_1)) {
        return;
    }

    bsp = global_structure_bsp;
    lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
    material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;
    shader = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&material->shader.tag_id & 0xffff].data;

    if (*(int16_t *)&((struct Shader *)shader)->shader_type != 3 ||
        *(int32_t *)&bsp->lightmaps_bitmap.tag_id == -1 ||
        *(int32_t *)(shader + 0x94) == -1 ||
        (int16_t)lightmap->bitmap == -1) {
        return;
    }

    lightmap_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
        (int16_t)lightmap->bitmap);
    base_map = *(datum_index *)(shader + 0x94);
    base_map_tag = (uint8_t *)halo::cache::globals().tag_instances[base_map & 0xffff].data;
    base_map_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(base_map,
        (int16_t)((int32_t)(int16_t)material->shader_permutation % *(int32_t *)(base_map_tag + 0x60)));

    if (lightmap_bitmap != 0 && object_lightmap_texture_ready(lightmap_bitmap, wait_for_textures) != 0) {
        triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
        halo::structures::bsp_lightmap_sample_vertex_color(lightmap_bitmap, weight_1, weight_2, (ColorRGB *)lightmap_color,
            material, triangle);
        lightmap_color->i += 0.1f;
        if (!(lightmap_color->i <= 1.0f)) {
            lightmap_color->i = 1.0f;
        }
        lightmap_color->j += 0.1f;
        if (!(lightmap_color->j <= 1.0f)) {
            lightmap_color->j = 1.0f;
        }
        lightmap_color->k += 0.1f;
        if (!(lightmap_color->k <= 1.0f)) {
            lightmap_color->k = 1.0f;
        }
    }

    if (base_map_bitmap != 0 && object_lightmap_texture_ready(base_map_bitmap, wait_for_textures) != 0) {
        if (triangle == 0) {
            triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
        }
        halo::structures::bsp_material_sample_base_map_color(base_map_bitmap, weight_1, weight_2, (ColorRGB *)base_map_color,
            material, triangle);
    }
}

namespace {
static int object_ambient_sample_slot_is_averaged(int index)
{
    return index != 3 && (index < 0x10 || index > 0x12);
}
}

/**
 * Samples the ambient lighting around an object into the sample array.
 *
 * Original register convention: EAX -> object_index, stack -> sample.
 *
 * @address 0x004f20b0
 */
void halo::objects::ObjectLighting::sample_ambient_lighting(float *sample)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t flags = (int8_t)(obj->flags >> 8) < 0 ? 1 : 0;
    char center_ok;
    int16_t successes;
    uint16_t offset_index;
    int i;

    if ((*((uint8_t *)object_tag + 2) & 4) != 0) {
        flags |= 4;
    }

    center_ok = halo::structures::object_lighting_sample_point(flags, &obj->bounding_center, (render_lighting *)sample);

    if ((obj->flags & 0x4000) == 0) {
        float probe[29];

        if (center_ok == 0) {
            for (i = 0; i < 29; i++) {
                sample[i] = 0.0f;
            }
            *(int16_t *)(sample + 3) = 2;
            successes = 0;
        } else {
            successes = 1;
        }

        for (offset_index = 0; (int16_t)offset_index < 4; offset_index++) {
            real_point3d corner;
            char ok;

            corner.x = ((offset_index & 1) == 0 ? -0.70710677f : 0.70710677f) * obj->bounding_radius +
                       obj->bounding_center.x;
            corner.z = obj->bounding_center.z;
            corner.y = ((offset_index & 2) == 0 ? -0.70710677f : 0.70710677f) * obj->bounding_radius +
                       obj->bounding_center.y;

            ok = halo::structures::object_lighting_sample_point(flags, &corner, (render_lighting *)probe);
            if (ok != 0) {
                successes = successes + 1;
                for (i = 0; i < 29; i++) {
                    if (object_ambient_sample_slot_is_averaged(i)) {
                        sample[i] += probe[i];
                    }
                }
            }
        }

        if (successes > 1) {
            float scale = 1.0f / (float)(int)successes;

            sample[0] *= scale; sample[1] *= scale; sample[2] *= scale;
            for (i = 0x13; i <= 0x16; i++) sample[i] *= scale;
            for (i = 4; i <= 9; i++) sample[i] *= scale;
            halo::math::vector3d_normalize_with_length(*(real_vector3d *)(sample + 7));

            for (i = 0x0a; i <= 0x0f; i++) sample[i] *= scale;
            halo::math::vector3d_normalize_with_length(*(real_vector3d *)(sample + 0x0d));

            for (i = 0x1a; i <= 0x1c; i++) sample[i] *= scale;
            for (i = 0x17; i <= 0x19; i++) sample[i] *= scale;
            halo::math::vector3d_normalize_with_length(*(real_vector3d *)(sample + 0x17));
            return;
        }

        if (successes == 0) {

            for (i = 0; i < 29; i++) {
                sample[i] = probe[i];
            }
        }
    }
}

/**
 * Collects the lights that affect an object into the output list.
 *
 * @address 0x004f2430
 */
void halo::objects::ObjectLighting::gather_light_list(uint8_t *out)
{
    datum_index object_index = handle;
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    real_point3d center = *(real_point3d *)(object + 0xa0);
    float radius = *(float *)(object + 0xac);
    int16_t *count = (int16_t *)(out + 0x40);
    uint32_t cursor[2];
    float intensities[2];
    uint32_t falloffs[2];
    int16_t cluster;
    int16_t i;

    *count = 0;
    light_frame_counter = light_frame_counter + 1;
    light_render_unknown_7c0 = 1;
    cluster = object_get_root_parent_placement(object_index, (object_placement_cursor *)cursor);
    while (cluster != -1) {
        object_lights_gather_nearest(cluster, object_index, &center, radius, (uint32_t *)(out + 0x44), intensities,
                                     (uint32_t)falloffs, count, 2);
        if (cursor[1] == 0xffffffff) {
            cluster = -1;
        } else {
            data_array *references = *(data_array **)((uint8_t *)cursor[0] + 8);
            uint8_t *element = (uint8_t *)references->data + (cursor[1] & 0xffff) * 0xc;
            cursor[1] = *(uint32_t *)(element + 8);
            cluster = *(int16_t *)(element + 4);
        }
    }
    light_render_unknown_7c0 = 0;
    for (i = 0; i < *count; i++) {
        uint32_t *slot = (uint32_t *)(out + 0x44) + i;
        *slot = *(uint32_t *)((uint8_t *)light_data->data + (*slot & 0xffff) * 0x7c + 8);
    }
}

namespace {
static float clamp_range(float value, float low, float high)
{
    if (!(value >= low)) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}
}

/**
 * Fills a render_lighting record from a lightmap sample, shading normal and base map colour.
 *
 * Original register convention: stack -> flags, shading_normal, intensity; ECX -> lightmap_color, EAX ->
 * lightmap_normal, EDX -> base_map_color, ESI -> lighting.
 *
 * @address 0x004f2ff0
 */
void halo::objects::ObjectLighting::build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal,
    float intensity, ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color,
    render_lighting *lighting)
{
    float luminance = lightmap_color->red * 0.299f + lightmap_color->blue * 0.114f + lightmap_color->green * 0.587f;
    float tint_red, tint_green;
    float shadow_scale;
    float x, y, h;
    float half_dark;

    lighting->ambient_color.red = object_lighting_ambient_scale * lightmap_color->red + object_lighting_ambient_bias;
    lighting->ambient_color.green = object_lighting_ambient_scale * lightmap_color->green + object_lighting_ambient_bias;
    lighting->ambient_color.blue = object_lighting_ambient_scale * lightmap_color->blue + object_lighting_ambient_bias;
    lighting->distant_light_count = 2;

    lighting->distant_lights[0].color = *lightmap_color;
    lighting->distant_lights[0].direction.i = -lightmap_normal->i;
    lighting->distant_lights[0].direction.j = -lightmap_normal->j;
    lighting->distant_lights[0].direction.k = -lightmap_normal->k;

    lighting->distant_lights[1].color.red = object_lighting_base_light_scale * base_map_color->red * luminance;
    lighting->distant_lights[1].color.green = object_lighting_base_light_scale * base_map_color->green * luminance;
    lighting->distant_lights[1].color.blue = object_lighting_base_light_scale * luminance * base_map_color->blue;
    lighting->distant_lights[1].direction = *shading_normal;

    lighting->reflection_tint.alpha = clamp_range(luminance * 1.5f + 0.25f, 0.0f, 1.0f);
    tint_red = clamp_range(base_map_color->red * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.red = tint_red;
    tint_green = clamp_range(base_map_color->green * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.green = tint_green;
    lighting->reflection_tint.blue = clamp_range(base_map_color->blue * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.red = clamp_range(lightmap_color->red + lightmap_color->red + 0.25f, 0.0f, 1.0f) * tint_red;
    lighting->reflection_tint.green = clamp_range(lightmap_color->green + lightmap_color->green + 0.25f, 0.0f, 1.0f) * tint_green;
    lighting->reflection_tint.blue = clamp_range(lightmap_color->blue + lightmap_color->blue + 0.25f, 0.0f, 1.0f) *
        lighting->reflection_tint.blue;

    shadow_scale = (float)pow((double)intensity, 0.25);
    x = shadow_scale * lighting->distant_lights[0].direction.i;
    y = shadow_scale * lighting->distant_lights[0].direction.j;
    lighting->shadow_vector.i = x;
    lighting->shadow_vector.j = y;
    h = (float)sqrt((double)(x * x + y * y));
    if (h < 0.707f) {
        lighting->shadow_vector.k = -(float)sqrt((double)(1.0f - h * h));
    } else {
        float rescale = 0.707f / h;

        lighting->shadow_vector.k = -0.70710677f;
        lighting->shadow_vector.i = x * rescale;
        lighting->shadow_vector.j = y * rescale;
    }

    half_dark = (1.0f - intensity) * 0.5f;
    lighting->shadow_color.red = clamp_range(1.0f - lighting->distant_lights[0].color.red * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);
    lighting->shadow_color.green = clamp_range(1.0f - lighting->distant_lights[0].color.green * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);
    lighting->shadow_color.blue = clamp_range(1.0f - lighting->distant_lights[0].color.blue * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);

    if ((flags & 4) != 0) {
        object_color_clamp_to_intensity(0.2f, &lighting->ambient_color);
        object_color_clamp_to_intensity(0.3f, &lighting->distant_lights[0].color);
        object_color_clamp_to_intensity(0.2f, &lighting->distant_lights[1].color);
        object_color_clamp_to_intensity(0.5f, (ColorRGB *)&lighting->reflection_tint.red);
        lighting->reflection_tint.alpha = 1.0f;
    }
}

/**
 * Scales a colour so that it does not exceed the given intensity.
 *
 * @address 0x004f3410
 */
void halo::objects::ObjectLighting::color_clamp_to_intensity(float intensity, ColorRGB *color)
{
    float max_channel = color->green <= color->blue ? color->blue : color->green;
    float scale;

    max_channel = color->red <= max_channel ? max_channel : color->red;

    scale = intensity + 1.0f;
    if (max_channel == 0.0f) {
        scale = 1.0f;
    } else {
        float headroom = max_channel * (intensity + 1.0f);
        if (headroom <= 1.0f) {
            if (intensity <= headroom) {
                goto apply;
            }
        } else {
            intensity = 1.0f;
        }
        scale = intensity / max_channel;
    }
apply:
    color->red *= scale;
    color->green *= scale;
    color->blue *= scale;
}

/**
 * Visits each light attachment of an object, optionally registering it in the table and invoking the callback.
 *
 * @address 0x004f9a20
 */
void halo::objects::ObjectLighting::for_each_light_attachment(int32_t register_in_table, int32_t invoke_callback)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if ((obj->flags & 0x100) != 0) {
        Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
        int16_t i;

        for (i = 0; i < (int16_t)definition->attachments.count; i++) {
            if ((obj->attachment_types[i] == _object_attachment_type_light) &&
                (obj->attachment_handles[i] != k_datum_index_none)) {
                if (register_in_table != 0) {
                    object_light_clear_dirty_flag(obj->attachment_handles[i]);
                }
                if (invoke_callback != 0) {
                    object_light_recompute_transform(obj->attachment_handles[i]);
                }
            }
        }
    }
}
