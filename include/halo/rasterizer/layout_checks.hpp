/**
 * @file include/halo/rasterizer/layout_checks.hpp
 * Compile-time checks that the typed Direct3D-facing records of types/rasterizer.h keep their retail layout: the
 * 32-bit handle and pointer members are pointers on this 32-bit target, so every size and offset below is the one the
 * original code addressed with byte offsets.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "rasterizer.h"

static_assert(sizeof(uintptr_t) == 4, "the rasterizer records mirror a 32-bit target");

static_assert(sizeof(rasterizer_vertex_buffer) == 0x14);
static_assert(offsetof(rasterizer_vertex_buffer, count) == 0x04);
static_assert(offsetof(rasterizer_vertex_buffer, data) == 0x0c);
static_assert(offsetof(rasterizer_vertex_buffer, hardware_buffer) == 0x10);

static_assert(sizeof(rasterizer_index_buffer) == 0x10);
static_assert(offsetof(rasterizer_index_buffer, data) == 0x08);
static_assert(offsetof(rasterizer_index_buffer, hardware_buffer) == 0x0c);

static_assert(sizeof(rasterizer_vertex_declaration) == 0x0c);
static_assert(offsetof(rasterizer_vertex_declaration, usage) == 0x08);

static_assert(sizeof(rasterizer_vertex_buffer_slot) == 0x14);
static_assert(offsetof(rasterizer_vertex_buffer_slot, vertex_type) == 0x04);

static_assert(sizeof(rasterizer_dynamic_vertex_slot) == 0x10);
static_assert(offsetof(rasterizer_dynamic_vertex_slot, locked_vertices) == 0x0c);

static_assert(sizeof(rasterizer_dynamic_index_slot) == 0x0c);
static_assert(offsetof(rasterizer_dynamic_index_slot, locked_indices) == 0x08);

static_assert(sizeof(rasterizer_effect_slot) == 0x20);
static_assert(offsetof(rasterizer_effect_slot, vertex_shader_index) == 0x04);
static_assert(offsetof(rasterizer_effect_slot, texture_handles) == 0x08);
static_assert(offsetof(rasterizer_effect_slot, constant_handles) == 0x18);

static_assert(sizeof(rasterizer_vertex_shader) == 0x08);

static_assert(sizeof(rasterizer_render_target) == 0x14);
static_assert(offsetof(rasterizer_render_target, surface) == 0x0c);
static_assert(offsetof(rasterizer_render_target, texture) == 0x10);

static_assert(sizeof(rasterizer_detail_object_draw) == 0x18);
static_assert(offsetof(rasterizer_detail_object_draw, z_reference) == 0x14);
static_assert(sizeof(rasterizer_detail_object_instance) == 0x06);
static_assert(sizeof(rasterizer_detail_object_batch) == 0x08);
static_assert(sizeof(rasterizer_detail_object_batches) == 0x08);
static_assert(sizeof(rasterizer_detail_object_vertex) == 0x14);

static_assert(sizeof(d3d_light9) == 0x68);
static_assert(offsetof(d3d_light9, position) == 0x34);
static_assert(offsetof(d3d_light9, range) == 0x4c);
static_assert(offsetof(d3d_light9, phi) == 0x64);

static_assert(sizeof(rasterizer_geometry_group_parameters) == 0x28);
static_assert(offsetof(rasterizer_geometry_group_parameters, shader) == 0x1c);
static_assert(offsetof(rasterizer_geometry_group_parameters, change_colors) == 0x20);
static_assert(offsetof(rasterizer_geometry_group_parameters, function_values) == 0x24);

static_assert(sizeof(transparent_geometry_group) == 0xa8);
static_assert(offsetof(transparent_geometry_group, shader) == 0x0c);
static_assert(offsetof(transparent_geometry_group, index_buffer) == 0x48);
static_assert(offsetof(transparent_geometry_group, callback) == 0x48);
static_assert(offsetof(transparent_geometry_group, vertex_buffer) == 0x58);
static_assert(offsetof(transparent_geometry_group, lightmap_bitmap) == 0x5c);
static_assert(offsetof(transparent_geometry_group, node_matrices) == 0x60);
static_assert(offsetof(transparent_geometry_group, node_part_indices) == 0x68);
static_assert(offsetof(transparent_geometry_group, lighting) == 0x70);
static_assert(offsetof(transparent_geometry_group, lighting_extra) == 0x74);
