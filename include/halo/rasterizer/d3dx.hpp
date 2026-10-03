/**
 * @file include/halo/rasterizer/d3dx.hpp
 * The D3DX entry points the rasterizer calls, as halo::rasterizer::d3dx functions (defined in src/rasterizer/d3dx.cpp).
 */
#pragma once

#include <stdint.h>

namespace halo::rasterizer::d3dx {

/** Size in bytes of one vertex with the given FVF code. */
uint32_t fvf_vertex_size(uint32_t fvf);

/** Creates an effect from compiled effect data through the D3DX version the original code was linked against. */
int32_t create_effect(void *device, const void *data, uint32_t size, const void *defines, void *include, uint32_t flags, void *pool, void *out_effect, void **out_error_buffer);

}  // namespace halo::rasterizer::d3dx
