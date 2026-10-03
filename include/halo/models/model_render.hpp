/**
 * @file include/halo/models/model_render.hpp
 * Model view operations: bind pose and part rendering, and the top-level model render call.
 */
#pragma once

#include "halo/models/models_types.hpp"

namespace halo::models {

/**
 * Non-owning view of a GBXModel tag instance.
 */
class model_view {
public:
    explicit model_view(GBXModel *p) : self(p) {}

    /**
     * Copies the model's node default orientations (bind pose) into the output array.
     *
     * @address 0x4d7610
     */
    void get_default_transforms(real_orientation *out);

    /**
     * Walks every region, permutation and part of the model three times (opaque, model decal, transparent), skipping the
     * last two in immediate mode, and hands each visible part to the rasterizer draw call for its shader type. Transparent
     * parts are chained into part groups.
     *
     * @address 0x4d72a0
     */
    void render_parts(uint8_t *region_permutations, rasterizer_node_matrices *node_matrices, model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags);

private:
    GBXModel *self;
};

/**
 * Renders a model tag with the given node matrices, region permutations, change colours, lighting, bounding sphere and
 * optional effect. Models whose on-screen size in pixels is below the lowest level of detail cutoff are skipped.
 *
 * @address 0x4d6fc0
 */
void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting, real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags);

}  // namespace halo::models
