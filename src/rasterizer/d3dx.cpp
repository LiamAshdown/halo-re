/**
 * @file src/rasterizer/d3dx.cpp
 * Forwards halo::rasterizer::d3dx to the D3DX entry points under their link names.
 */

#include "halo/rasterizer/d3dx.hpp"
#include "halo/rasterizer/gl_device.hpp"
#include "link/d3dx.hpp"
#include "halo/rasterizer/api.hpp"

namespace halo::rasterizer::d3dx {

uint32_t fvf_vertex_size(uint32_t fvf)
{
    return ::D3DXGetFVFVertexSize(fvf);
}

int32_t create_effect(void *device, const void *data, uint32_t size, const void *defines, void *include, uint32_t flags, void *pool, void *out_effect, void **out_error_buffer)
{
    if (gl_renderer_requested()) {
        if (out_error_buffer != nullptr) {
            *out_error_buffer = nullptr;
        }
        return gl_create_effect(out_effect);
    }
    return ::D3DXCreateEffect(device, data, size, defines, include, flags, pool, out_effect, out_error_buffer);
}

}  // namespace halo::rasterizer::d3dx
