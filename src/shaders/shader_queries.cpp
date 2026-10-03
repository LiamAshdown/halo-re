/**
 * @file src/shaders/shader_queries.cpp
 * Questions the rasterizer asks of a shader tag: vertex shader permutation, decal flag, draw-before-water flag.
 * The original author notes and decompiles are in docs/original/shaders/.
 */

#include "halo/shaders/shaders.hpp"

namespace halo::shaders {

int16_t shader_view::vertex_shader_permutation()
{
    if (self == (Shader *)-1) {
        return _shader_vertex_permutation_default;
    }

    switch (self->shader_type - 1) {
    case 0: {
        shader_effect *effect = (shader_effect *)self;
        if (*(int32_t *)&effect->secondary_map.tag_id != -1) {
            return (int16_t)(effect->anchor + 1);
        }
        break;
    }
    case 3: {
        ShaderModel *model = (ShaderModel *)self;
        if (0.0f < model->translucency) {
            return _shader_vertex_permutation_model_translucent;
        }
        break;
    }
    case 4:
    case 5:
    case 6: {
        ShaderTransparentGeneric *transparent = (ShaderTransparentGeneric *)self;
        int16_t permutation = (int16_t)(transparent->first_map_type + 1);

        if (permutation == 1 &&
            (transparent->shader_transparent_generic_flags &
             _shader_transparent_first_map_is_in_screenspace_bit) == 0) {
            permutation = _shader_vertex_permutation_default;
        }
        if (self->shader_flags & _shader_transparent_lit_bit) {
            return _shader_vertex_permutation_transparent_lit;
        }
        return permutation;
    }
    }
    return _shader_vertex_permutation_default;
}

uint8_t shader_view::is_decal()
{
    if (self == (Shader *)0) {
        return 0;
    }

    switch (self->shader_type) {
    case shadertype_transparent_generic:
    case shadertype_transparent_chicago:
    case shadertype_transparent_chicago_extended: {
        ShaderTransparentGeneric *transparent = (ShaderTransparentGeneric *)self;
        return (transparent->shader_transparent_generic_flags >> 1) & 1;
    }
    case shadertype_transparent_glass: {
        ShaderTransparentGlass *glass = (ShaderTransparentGlass *)self;
        return (uint8_t)((glass->shader_transparent_glass_flags >> 1) & 1);
    }
    case shadertype_transparent_meter: {
        ShaderTransparentMeter *meter = (ShaderTransparentMeter *)self;
        return (uint8_t)(meter->meter_flags & 1);
    }
    default:
        return 0;
    }
}

uint8_t shader_view::draw_before_water()
{
    ShaderTransparentGeneric *transparent;

    if (self == (Shader *)0) {
        return 0;
    }
    if (self->shader_type != shadertype_transparent_generic &&
        self->shader_type != shadertype_transparent_chicago &&
        self->shader_type != shadertype_transparent_chicago_extended) {
        return 0;
    }

    transparent = (ShaderTransparentGeneric *)self;
    return (transparent->shader_transparent_generic_flags >> 4) & 1;
}

}  // namespace halo::shaders
