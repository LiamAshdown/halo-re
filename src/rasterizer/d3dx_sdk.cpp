/**
 * @file src/rasterizer/d3dx_sdk.cpp
 * The D3DX calls that need the DirectX SDK headers (halo::rasterizer::d3dx, include/halo/rasterizer/d3dx.hpp). With
 * HALO_D3D9 off (no DirectX SDK: the OpenGL-only and browser builds) they are done here without D3DX: there is no
 * effect pool, FVF codes are worked out from the declaration, there is no loading-screen resource, and the thread
 * locale is left alone.
 */

#include "halo/rasterizer/d3dx.hpp"

#if HALO_D3D9

#include "win32.h"
#include "d3d.h"

namespace halo::rasterizer::d3dx {

int32_t create_effect_pool(void **out_pool)
{
    return D3DXCreateEffectPool(reinterpret_cast<LPD3DXEFFECTPOOL *>(out_pool));
}

int32_t fvf_from_declarator(const void *elements, uint32_t *out_fvf)
{
    return D3DXFVFFromDeclarator(static_cast<const D3DVERTEXELEMENT9 *>(elements), reinterpret_cast<DWORD *>(out_fvf));
}

int32_t load_surface_from_resource(void *surface, void *module, uint32_t resource_id)
{
    return D3DXLoadSurfaceFromResourceA(static_cast<LPDIRECT3DSURFACE9>(surface), nullptr, nullptr, static_cast<HMODULE>(module),
        MAKEINTRESOURCEA(resource_id), nullptr, k_default, 0, nullptr);
}

uint32_t locale_begin(uint32_t locale)
{
    uint32_t saved = GetThreadLocale();

    SetThreadLocale(locale);
    return saved;
}

void locale_end(uint32_t saved)
{
    SetThreadLocale(saved);
}

}  // namespace halo::rasterizer::d3dx

#else

namespace halo::rasterizer::d3dx {

namespace {

constexpr int32_t k_invalid_call = static_cast<int32_t>(0x8876086c);  // D3DERR_INVALIDCALL

struct vertex_element {
    uint16_t stream;
    uint16_t offset;
    uint8_t type;  // D3DDECLTYPE
    uint8_t method;
    uint8_t usage;  // D3DDECLUSAGE
    uint8_t usage_index;
};

}  // namespace

int32_t create_effect_pool(void **out_pool)
{
    *out_pool = nullptr;  // the OpenGL effects share nothing through a pool
    return 0;
}

int32_t fvf_from_declarator(const void *elements, uint32_t *out_fvf)
{
    const vertex_element *e = static_cast<const vertex_element *>(elements);
    uint32_t fvf = 0;
    uint32_t texture_count = 0;

    for (; e->stream != 0xff; e++) {
        if (e->stream != 0 || e->method != 0) {
            return k_invalid_call;
        }
        switch (e->usage) {
        case 0:  // POSITION: FLOAT3 -> XYZ, FLOAT4 -> XYZW
            fvf |= e->type == 2 ? 0x002u : e->type == 3 ? 0x4002u : 0u;
            break;
        case 9:  // POSITIONT FLOAT4 -> XYZRHW
            fvf |= 0x004;
            break;
        case 1:  // BLENDWEIGHT FLOAT1..4 after XYZ -> XYZB1..4
            if ((fvf & 0x400e) != 0x002 || e->type > 3) {
                return k_invalid_call;
            }
            fvf = (fvf & ~0x400eu) | (0x006u + 2u * e->type);
            break;
        case 3:  // NORMAL
            fvf |= 0x010;
            break;
        case 4:  // PSIZE
            fvf |= 0x020;
            break;
        case 10:  // COLOR 0 DIFFUSE, 1 SPECULAR
            fvf |= e->usage_index == 0 ? 0x040u : 0x080u;
            break;
        case 5: {  // TEXCOORD n: FLOAT2 format 0, FLOAT3 1, FLOAT4 2, FLOAT1 3
            static const uint32_t formats[4] = {3, 0, 1, 2};

            if (e->usage_index != texture_count || e->type > 3) {
                return k_invalid_call;
            }
            fvf |= formats[e->type] << (16 + 2 * texture_count);
            texture_count++;
            break;
        }
        default:
            return k_invalid_call;
        }
    }
    *out_fvf = fvf | (texture_count << 8);
    return 0;
}

int32_t load_surface_from_resource(void *surface, void *module, uint32_t resource_id)
{
    (void)surface;
    (void)module;
    (void)resource_id;
    return k_invalid_call;  // no executable resources: the loading screen falls back to a cleared frame
}

uint32_t locale_begin(uint32_t locale)
{
    return locale;
}

void locale_end(uint32_t saved)
{
    (void)saved;
}

}  // namespace halo::rasterizer::d3dx

#endif
