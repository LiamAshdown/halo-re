/**
 * @file src/rasterizer/d3dx.cpp
 * Forwards halo::rasterizer::d3dx to the D3DX entry points under their link names.
 */

#include "halo/rasterizer/d3dx.hpp"
#include "halo/rasterizer/gl_device.hpp"
#include "halo/rasterizer/api.hpp"
#if HALO_D3D9
#include "link/d3dx.hpp"
#endif

namespace halo::rasterizer::d3dx {

uint32_t fvf_vertex_size(uint32_t fvf)
{
#if HALO_D3D9
    return ::D3DXGetFVFVertexSize(fvf);
#else
    static const uint32_t position_size[8] = {0, 12, 16, 16, 20, 24, 28, 32};  // by (fvf & 0xe) / 2: XYZ, XYZRHW, XYZB1..5
    static const uint32_t coordinate_size[4] = {8, 12, 16, 4};
    uint32_t size = (fvf & 0x4000) != 0 ? 16 : position_size[(fvf & 0xe) / 2];  // XYZW
    uint32_t textures = (fvf >> 8) & 0xf;

    size += (fvf & 0x010) != 0 ? 12 : 0;  // NORMAL
    size += (fvf & 0x020) != 0 ? 4 : 0;   // PSIZE
    size += (fvf & 0x040) != 0 ? 4 : 0;   // DIFFUSE
    size += (fvf & 0x080) != 0 ? 4 : 0;   // SPECULAR
    for (uint32_t i = 0; i < textures; i++) {
        size += coordinate_size[(fvf >> (16 + 2 * i)) & 3];
    }
    return size;
#endif
}

int32_t create_effect(void *device, const void *data, uint32_t size, const void *defines, void *include, uint32_t flags, void *pool, void *out_effect, void **out_error_buffer)
{
    if (gl_renderer_requested()) {
        if (out_error_buffer != nullptr) {
            *out_error_buffer = nullptr;
        }
        return gl_create_effect(data, size, out_effect);
    }
#if HALO_D3D9
    return ::D3DXCreateEffect(device, data, size, defines, include, flags, pool, out_effect, out_error_buffer);
#else
    (void)device;
    (void)defines;
    (void)include;
    (void)flags;
    (void)pool;
    (void)out_error_buffer;
    return gl_create_effect(data, size, out_effect);
#endif
}

}  // namespace halo::rasterizer::d3dx
