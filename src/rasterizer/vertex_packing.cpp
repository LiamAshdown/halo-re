/**
 * @file src/rasterizer/vertex_packing.cpp
 * Packed 11:11:10 normals and the compressed BSP vertex unpackers.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "internal/state.hpp"

extern "C" {

extern double floor(double x);

}  // extern "C"

namespace halo::rasterizer {

/**
 * 0x513400, blam-cc: EAX out, ECX packed Unpacks the 11:11:10 normal of a compressed BSP lightmap vertex.
 *
 * @address 0x5134c0
 */
void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex, real_vector3d *out)
{
    real_vector3d unpacked;

    *out = *vector3d_unpack_normal_11_11_10(&unpacked, vertex->normal);
}

/**
 * 0x513400, blam-cc: EAX out, ECX packed Unpacks the 11:11:10 normal of a compressed BSP rendered vertex.
 *
 * @address 0x513490
 */
void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex, real_vector3d *out)
{
    real_vector3d unpacked;

    *out = *vector3d_unpack_normal_11_11_10(&unpacked, vertex->normal);
}

static float clamp_unit(float v)
{
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

/**
 * Packs a unit-ish 3 component vector into an 11:11:10 bit signed encoding: each component is clamped to
 * [-1,1], scaled (1023.5 for the two 11 bit components, 511.5 for the 10 bit one), passed through floor and
 * rounded to an integer, then the three integers are packed low-to-high as (z << 22) | (y << 11) | x, each
 * masked to its field width.
 *
 * Registers: unaff_ESI -> direction
 *
 * @address 0x5132d0
 */
uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction)
{
    float cx, cy, cz;
    int32_t xi, yi, zi;

    cx = clamp_unit(direction->i);
    xi = (int32_t)(float)floor((double)(cx * 1023.5f));

    cy = clamp_unit(direction->j);
    yi = (int32_t)(float)floor((double)(cy * 1023.5f));

    cz = clamp_unit(direction->k);
    zi = (int32_t)(float)floor((double)(cz * 511.5f));

    return (uint32_t)(((zi << 0xb | (yi & 0x7ff)) << 0xb) | (xi & 0x7ff));
}

/**
 * Unpacks an 11:11:10 bit signed-normal-encoded direction vector (see vector3d_pack_normal_11_11_10) into
 * out->{i,j,k} and returns out.
 *
 * Registers: EAX -> out, ECX -> packed
 *
 * @address 0x513400
 */
real_vector3d * vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed)
{
    out->i = ((float)(int32_t)(packed << 0x15) * 9.536743e-07f + 1.0f) * 0.0004885198f;
    out->j = ((float)(int32_t)((packed >> 0xb) << 0x15) * 9.536743e-07f + 1.0f) * 0.0004885198f;
    out->k = ((float)(int32_t)(packed & 0xffc00000) * 4.7683716e-07f + 1.0f) * 0.0009775171f;
    return out;
}

}  // namespace halo::rasterizer
