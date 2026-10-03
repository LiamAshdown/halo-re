/**
 * @file include/halo/bitmaps/dxt_decoder.hpp
 * Software decoders for single texels of DXT1, DXT3 and DXT5 blocks.
 */
#pragma once

#include "halo/bitmaps/bitmaps_types.hpp"

namespace halo::bitmaps {

/**
 * Stateless decoders for individual texels of the Direct3D DXT1/DXT3/DXT5 block formats.
 */
struct dxt_decoder {
    /**
     * Decodes the colour of texel (x, y) of a DXT1 block, each coordinate in [0, 3]. A NULL block clears sixteen
     * consecutive texels starting at out instead.
     *
     * @address 0x43ffe0
     */
    static void decode_dxt1_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y);

    /**
     * Decodes texel (x, y) of a DXT3 block: colour from its embedded DXT1 block, alpha from the explicit 4-bit nibble
     * widened to 8 bits.
     *
     * @address 0x440150
     */
    static void decode_dxt3_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block);

    /**
     * Decodes texel (x, y) of a DXT5 block: colour from its embedded DXT1 block, alpha from the 8-entry interpolated alpha
     * palette.
     *
     * @address 0x440190
     */
    static void decode_dxt5_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y);

};

}  // namespace halo::bitmaps
