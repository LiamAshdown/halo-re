/**
 * @file include/halo/rasterizer/d3dx.hpp
 * The D3DX entry points the rasterizer calls, as halo::rasterizer::d3dx functions (src/rasterizer/d3dx.cpp and, for the
 * ones that need the DirectX SDK headers, src/rasterizer/d3dx_sdk.cpp). Builds without Direct3D 9 (HALO_D3D9 0) do the
 * work without D3DX.
 */
#pragma once

#include <stdint.h>

namespace halo::rasterizer::d3dx {

/** D3DX_DEFAULT: the library chooses the filter. */
inline constexpr uint32_t k_default = ~0u;

/** Size in bytes of one vertex with the given FVF code. */
uint32_t fvf_vertex_size(uint32_t fvf);

/** Creates an effect from compiled effect data through the D3DX version the original code was linked against. */
int32_t create_effect(void *device, const void *data, uint32_t size, const void *defines, void *include, uint32_t flags, void *pool, void *out_effect, void **out_error_buffer);

/** D3DXCreateEffectPool. */
int32_t create_effect_pool(void **out_pool);

/** D3DXFVFFromDeclarator: the FVF code of a vertex declaration (D3DVERTEXELEMENT9 entries up to D3DDECL_END). */
int32_t fvf_from_declarator(const void *elements, uint32_t *out_fvf);

/** D3DXLoadSurfaceFromResourceA with default filtering: a bitmap resource of module into surface. */
int32_t load_surface_from_resource(void *surface, void *module, uint32_t resource_id);

/** Sets the thread locale the effect compiler parses numbers with (SetThreadLocale); returns the one to restore. */
uint32_t locale_begin(uint32_t locale);
void locale_end(uint32_t saved);

}  // namespace halo::rasterizer::d3dx
