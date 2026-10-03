/**
 * @file include/halo/rasterizer/draw_procedures.hpp
 * Function pointer types of the draw routines the hardware code path selection installs, so the selected routine is
 * stored and called through its real signature instead of an untyped pointer.
 */
#pragma once

#include <cstdint>

struct Shader;
struct ShaderEnvironment;
struct rasterizer_index_buffer;
struct rasterizer_vertex_buffer;
struct transparent_geometry_group;

namespace halo::rasterizer {

/** Draws one part of an environment or model shader (the environment_draw / shader model draw routines). */
using part_draw_procedure = void (*)(Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                     int32_t dynamic_index_slot, int32_t primitive_count,
                                     rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot);

/** Draws the surfaces of one environment shader pass (lightmap, self illumination and light cone routines). */
using surface_draw_procedure = void (*)(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot,
                                        int32_t first_primitive, int32_t primitive_count,
                                        rasterizer_vertex_buffer *vertex_buffer);

/** Draws one transparent geometry group of a shader type. */
using group_draw_procedure = void (*)(transparent_geometry_group *group);

/** Draws one transparent geometry group of a shader type that has several variants (glass reflection kinds). */
using group_kind_draw_procedure = void (*)(transparent_geometry_group *group, int16_t kind);

/** The three routines a glass shader draws with, in the order the engine keeps them. */
struct glass_draw_procedures {
    group_draw_procedure diffuse;
    group_draw_procedure tint;
    group_kind_draw_procedure reflection;
};

}  // namespace halo::rasterizer
