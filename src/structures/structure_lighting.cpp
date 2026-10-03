/**
 * @file src/structures/structure_lighting.cpp
 * Lightmap and base map sampling for object lighting.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/math/api.hpp"

extern "C" {
extern int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias);
extern void color_rgb_int_to_real(ColorRGB *out, uint32_t packed);
extern ScenarioStructureBSP *global_structure_bsp;
extern tag_instance *tag_instances;
extern render_lighting object_lighting_default;
extern real_vector3d object_lightmap_probe_direction[1];
extern real_vector3d object_lighting_probe_sideways[4];
extern BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
extern void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex,
    real_vector3d *out);
extern void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex,
    real_vector3d *out);
extern void object_build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal, float intensity,
    ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color,
    render_lighting *lighting);
}

namespace halo::structures {

void bsp_lighting::lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    float u0, u1, u2, v0, v1, v2;
    float uv[2];
    int32_t packed;

    if (material->rendered_vertices_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        uint8_t *base = (uint8_t *)material->compressed_vertices.pointer;
        int32_t skip = material->rendered_vertices_count * 4;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e0 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[0] + skip;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e1 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[1] + skip;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e2 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[2] + skip;

        u0 = ((float)(int32_t)e0->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v0 = ((float)(int32_t)e0->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
        u1 = ((float)(int32_t)e1->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v1 = ((float)(int32_t)e1->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
        u2 = ((float)(int32_t)e2->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v2 = ((float)(int32_t)e2->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
    } else if (material->rendered_vertices_type == vertextype_structure_bsp_uncompressed_rendered_vertices ||
               material->rendered_vertices_type == 0xc) {
        uint8_t *base = (uint8_t *)material->uncompressed_vertices.pointer + material->rendered_vertices_count * 0x38;
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e0 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[0] * 0x14);
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e1 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[1] * 0x14);
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e2 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[2] * 0x14);

        u0 = e0->texture_coords.x; v0 = e0->texture_coords.y;
        u1 = e1->texture_coords.x; v1 = e1->texture_coords.y;
        u2 = e2->texture_coords.x; v2 = e2->texture_coords.y;
    } else {
        u0 = v0 = u1 = v1 = u2 = v2 = 0.0f;
    }

    uv[0] = (u1 - u0) * weight_1 + (u2 - u0) * weight_2 + u0;
    uv[1] = (v1 - v0) * weight_1 + (v2 - v0) * weight_2 + v0;

    packed = rasterizer_bitmap_sample_texel(bitmap, uv, 1.0f);
    color_rgb_int_to_real(out, (uint32_t)packed);
}

void bsp_lighting::material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    float u0, u1, u2, v0, v1, v2;
    float uv[2];
    int32_t packed;

    if (material->rendered_vertices_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedRenderedVertex *base =
            (ScenarioStructureBSPMaterialCompressedRenderedVertex *)material->compressed_vertices.pointer;

        u0 = base[triangle_vertex_indices[0]].texture_coords.x;
        v0 = base[triangle_vertex_indices[0]].texture_coords.y;
        u1 = base[triangle_vertex_indices[1]].texture_coords.x;
        v1 = base[triangle_vertex_indices[1]].texture_coords.y;
        u2 = base[triangle_vertex_indices[2]].texture_coords.x;
        v2 = base[triangle_vertex_indices[2]].texture_coords.y;
    } else if (material->rendered_vertices_type == vertextype_structure_bsp_uncompressed_rendered_vertices ||
               material->rendered_vertices_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedRenderedVertex *base =
            (ScenarioStructureBSPMaterialUncompressedRenderedVertex *)material->uncompressed_vertices.pointer;

        u0 = base[triangle_vertex_indices[0]].texture_coords.x;
        v0 = base[triangle_vertex_indices[0]].texture_coords.y;
        u1 = base[triangle_vertex_indices[1]].texture_coords.x;
        v1 = base[triangle_vertex_indices[1]].texture_coords.y;
        u2 = base[triangle_vertex_indices[2]].texture_coords.x;
        v2 = base[triangle_vertex_indices[2]].texture_coords.y;
    } else {
        u0 = v0 = u1 = v1 = u2 = v2 = 0.0f;
    }

    uv[0] = (u2 - u0) * weight_2 + (u1 - u0) * weight_1 + u0;
    uv[1] = (v2 - v0) * weight_2 + (v1 - v0) * weight_1 + v0;

    packed = rasterizer_bitmap_sample_texel(bitmap, uv, 0.3f);
    color_rgb_int_to_real(out, (uint32_t)packed);
}

uint8_t bsp_lighting::object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting)
{
    ScenarioStructureBSP *bsp = global_structure_bsp;
    real_vector3d *directions;
    int16_t direction_count;
    int16_t direction_index;
    real_point3d contact;
    int16_t lightmap_index;
    int16_t material_index;
    int32_t surface_index;
    float weight_1;
    float weight_2;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    uint8_t *shader;
    uint8_t *base_map_tag;
    uint16_t *triangle;
    BitmapData *lightmap_bitmap;
    BitmapData *base_map_bitmap;
    ColorRGB base_map_color;
    ColorRGB lightmap_color;
    real_vector3d normals[3];
    real_vector3d shading_normal;
    real_vector3d lightmap_normal;
    float lengths[3];
    int16_t vertex_type;
    int32_t i;

    if (bsp->default_ambient_color.red == 0.0f) {
        *lighting = object_lighting_default;
    } else {
        *lighting = *(render_lighting *)&bsp->default_ambient_color;
        lighting->distant_light_count = 2;
    }

    if (flags & 1) {
        directions = object_lighting_probe_sideways;
        direction_count = 4;
    } else {
        directions = object_lightmap_probe_direction;
        direction_count = 1;
    }

    for (direction_index = 0; direction_index < direction_count; direction_index++) {
        if (structure_bsp_query::resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2, &directions[direction_index], &material_index, &surface_index, &weight_1)) {
            break;
        }
    }
    if (direction_index >= direction_count) {
        return 0;
    }

    bsp = global_structure_bsp;
    lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
    material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;
    shader = (uint8_t *)tag_instances[*(uint32_t *)&material->shader.tag_id & 0xffff].data;

    if (*(int16_t *)&((struct Shader *)shader)->shader_type != 3 ||
        *(int32_t *)&bsp->lightmaps_bitmap.tag_id == -1 ||
        (int16_t)lightmap->bitmap == -1 ||
        *(int32_t *)(shader + 0x94) == -1) {
        return 0;
    }

    triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
    lightmap_bitmap = bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
        (int16_t)lightmap->bitmap);
    base_map_tag = (uint8_t *)tag_instances[*(uint32_t *)(shader + 0x94) & 0xffff].data;
    base_map_bitmap = bitmap_group_get_bitmap_data(*(datum_index *)(shader + 0x94),
        (int16_t)((int32_t)(int16_t)material->shader_permutation % *(int32_t *)(base_map_tag + 0x60)));
    if (lightmap_bitmap == 0 || base_map_bitmap == 0 ||
        texture_cache_get(lightmap_bitmap, 1, 1) == 0 || texture_cache_get(base_map_bitmap, 1, 1) == 0) {
        return 0;
    }

    bsp_lighting::material_sample_base_map_color(base_map_bitmap, weight_1, weight_2, &base_map_color, material, triangle);
    bsp_lighting::lightmap_sample_vertex_color(lightmap_bitmap, weight_1, weight_2, &lightmap_color, material, triangle);

    vertex_type = (int16_t)material->rendered_vertices_type;
    if (vertex_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedRenderedVertex *vertices =
            (ScenarioStructureBSPMaterialCompressedRenderedVertex *)(uintptr_t)material->compressed_vertices.pointer;
        for (i = 0; i < 3; i++) {
            bsp_compressed_rendered_vertex_unpack_normal(&vertices[triangle[i]], &normals[i]);
        }
    } else if (vertex_type == vertextype_structure_bsp_uncompressed_rendered_vertices || vertex_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedRenderedVertex *vertices =
            (ScenarioStructureBSPMaterialUncompressedRenderedVertex *)(uintptr_t)material->uncompressed_vertices.pointer;
        for (i = 0; i < 3; i++) {
            normals[i] = *(real_vector3d *)&vertices[triangle[i]].normal;
        }
    }
    halo::math::vector3d_barycentric_interpolate(shading_normal, normals[2], normals[1], normals[0], weight_1, weight_2);
    halo::math::vector3d_normalize_with_length(shading_normal);

    if (vertex_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedLightmapVertex *vertices =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)(uintptr_t)material->compressed_vertices.pointer +
            material->rendered_vertices_count * 4;
        for (i = 0; i < 3; i++) {
            bsp_compressed_lightmap_vertex_unpack_normal(&vertices[triangle[i]], &normals[i]);
        }
    } else if (vertex_type == vertextype_structure_bsp_uncompressed_rendered_vertices || vertex_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *vertices =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)((uint8_t *)(uintptr_t)material->uncompressed_vertices.pointer +
            material->rendered_vertices_count * 0x38);
        for (i = 0; i < 3; i++) {
            normals[i] = *(real_vector3d *)&vertices[triangle[i]].normal;
        }
    }
    lengths[0] = halo::math::vector3d_normalize_with_length(normals[0]);
    lengths[1] = halo::math::vector3d_normalize_with_length(normals[1]);
    lengths[2] = halo::math::vector3d_normalize_with_length(normals[2]);
    halo::math::vector3d_barycentric_interpolate(lightmap_normal, normals[2], normals[1], normals[0], weight_1, weight_2);
    halo::math::vector3d_normalize_with_length(lightmap_normal);

    object_build_effect_parameter_block(flags, &shading_normal,
        (lengths[1] - lengths[0]) * weight_1 + (lengths[2] - lengths[0]) * weight_2 + lengths[0],
        &lightmap_color, &lightmap_normal, &base_map_color, lighting);
    return 1;
}

void bsp_lighting::lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent, real *out_sprite_rect, const Decal *decal_definition)
{
    const Bitmap *bitmap =
        (const Bitmap *)tag_instances[*(const uint16_t *)&decal_definition->map.tag_id].data;
    const BitmapGroupSequence *sequence =
        &((const BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer)[sequence_index];
    const BitmapGroupSprite *sprite = &((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];
    const BitmapData *data = &((const BitmapData *)bitmap->bitmap_data.pointer)[(int16_t)sprite->bitmap_index];
    real aspect = 1.0f;
    real extent_scale;
    real scale_u;
    real scale_v;

    out_sprite_rect[0] = sprite->left;
    out_sprite_rect[1] = sprite->right;
    out_sprite_rect[2] = sprite->top;
    out_sprite_rect[3] = sprite->bottom;

    if ((decal_definition->flags & 0x100) != 0) {
        aspect = ((sprite->right - sprite->left) / (sprite->bottom - sprite->top)) *
            ((real)(int32_t)(int16_t)data->height / (real)(int32_t)(int16_t)data->width);
    }

    extent_scale = scale / decal_definition->maximum_sprite_extent;
    scale_u = (real)(int32_t)(int16_t)data->width * extent_scale;
    scale_v = (real)(int32_t)(int16_t)data->height * extent_scale * aspect;

    out_extent[0] = -sprite->registration_point.x * scale_u;
    out_extent[1] = ((sprite->right - sprite->registration_point.x) - sprite->left) * scale_u;
    out_extent[2] = -sprite->registration_point.y * scale_v;
    out_extent[3] = ((sprite->bottom - sprite->registration_point.y) - sprite->top) * scale_v;
}

}  // namespace halo::structures
