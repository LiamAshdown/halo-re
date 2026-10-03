/**
 * @file include/halo/structures/structure_lighting.hpp
 * Lightmap and base map sampling for object lighting.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Samples bsp lightmaps and base maps to light objects and builds lightmap uv rectangles.
 */
struct bsp_lighting {
    /**
     * Samples the lightmap bitmap at the barycentric position inside a triangle and writes the blended colour.
     *
     * @address 0x4f0730
     */
    static void lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);

    /**
     * Samples the material's base map at the barycentric position inside a triangle and writes the blended colour.
     *
     * @address 0x4f0900
     */
    static void material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);

    /**
     * Builds the render lighting for a point from the bsp surface found along one of the probe directions: lightmap
     * colour, base map colour, shading normal, lightmap normal and intensity. Returns whether a surface was found.
     *
     * @address 0x4f2550
     */
    static uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting);

    /**
     * Builds the sprite rectangle and extent of a decal from its bitmap sequence and sprite index, scaled by the given
     * scale.
     *
     * @address 0x44db30
     */
    static void lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent, real *out_sprite_rect, const Decal *decal_definition);

};

}  // namespace halo::structures
