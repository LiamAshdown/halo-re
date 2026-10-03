/**
 * @file src/bitmaps/dxt_decoder.cpp
 * Software decoders for single texels of DXT1, DXT3 and DXT5 blocks.
 * The original author notes and decompiles are in docs/original/bitmaps/.
 */

#include "halo/bitmaps/bitmaps.hpp"

namespace halo::bitmaps {

static uint8_t dxt1_blend_two_thirds(uint8_t near_channel, uint8_t far_channel)
{
    return (uint8_t)(((uint32_t)near_channel * 2 + far_channel + 1) / 3);
}

void dxt_decoder::decode_dxt1_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y)
{
    ColorARGBInt palette[4];
    uint32_t selector;
    int32_t i;

    if (block == 0) {
        for (i = 0; i < k_dxt_block_texel_count; i++) {
            out[i].blue = 0;
            out[i].green = 0;
            out[i].red = 0;
            out[i].alpha = 0;
        }
        return;
    }

    color_codec::unpack_565_to_rgb888(&block->color0, &palette[0]);
    color_codec::unpack_565_to_rgb888(&block->color1, &palette[1]);
    palette[0].alpha = 0xff;
    palette[1].alpha = 0xff;

    if (block->color1 < block->color0) {
        palette[2].blue = dxt1_blend_two_thirds(palette[0].blue, palette[1].blue);
        palette[2].green = dxt1_blend_two_thirds(palette[0].green, palette[1].green);
        palette[2].red = dxt1_blend_two_thirds(palette[0].red, palette[1].red);
        palette[2].alpha = 0xff;
        palette[3].blue = dxt1_blend_two_thirds(palette[1].blue, palette[0].blue);
        palette[3].green = dxt1_blend_two_thirds(palette[1].green, palette[0].green);
        palette[3].red = dxt1_blend_two_thirds(palette[1].red, palette[0].red);
        palette[3].alpha = 0xff;
    } else {
        palette[2].blue = (uint8_t)(((uint32_t)palette[1].blue + palette[0].blue) / 2);
        palette[2].green = (uint8_t)(((uint32_t)palette[1].green + palette[0].green) / 2);
        palette[2].red = (uint8_t)(((uint32_t)palette[1].red + palette[0].red) / 2);
        palette[2].alpha = 0xff;
        palette[3].blue = 0;
        palette[3].green = 0;
        palette[3].red = 0;
        palette[3].alpha = 0;
    }

    selector = (block->indices >> (((x + y * 4) * 2) & 0x1f)) & 3;
    *out = palette[selector];
}

void dxt_decoder::decode_dxt3_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block)
{
    uint8_t raw;

    dxt_decoder::decode_dxt1_texel(texel_out, &block->color, x, y);

    raw = (uint8_t)(block->alpha_rows[y] >> ((x & 7) << 2));
    texel_out->alpha = (uint8_t)((raw << 4) | (raw & 0xf));
}

void dxt_decoder::decode_dxt5_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y)
{
    uint8_t alpha0, alpha1;
    uint8_t palette[8];
    uint32_t group;
    int32_t local_texel;

    dxt_decoder::decode_dxt1_texel(texel_out, &block->color, x, y);

    alpha0 = block->alpha0;
    alpha1 = block->alpha1;
    palette[0] = alpha0;
    palette[1] = alpha1;
    if (alpha1 < alpha0) {
        palette[2] = (uint8_t)((alpha1 + alpha0 * 6) / 7);
        palette[3] = (uint8_t)((alpha0 * 5 + alpha1 * 2) / 7);
        palette[4] = (uint8_t)((alpha1 * 3 + alpha0 * 4) / 7);
        palette[5] = (uint8_t)((alpha0 * 3 + alpha1 * 4) / 7);
        palette[6] = (uint8_t)((alpha1 * 5 + alpha0 * 2) / 7);
        palette[7] = (uint8_t)((alpha0 + alpha1 * 6) / 7);
    } else {
        palette[2] = (uint8_t)((alpha1 + alpha0 * 4) / 5);
        palette[3] = (uint8_t)((alpha0 * 3 + alpha1 * 2) / 5);
        palette[4] = (uint8_t)((alpha1 * 3 + alpha0 * 2) / 5);
        palette[5] = (uint8_t)((alpha0 + alpha1 * 4) / 5);
        palette[6] = 0;
        palette[7] = 0xff;
    }

    if (y < 2) {
        group = (uint32_t)block->alpha_indices[0] | ((uint32_t)block->alpha_indices[1] << 8) |
                ((uint32_t)block->alpha_indices[2] << 16);
        local_texel = x + y * 4;
    } else {
        group = (uint32_t)block->alpha_indices[3] | ((uint32_t)block->alpha_indices[4] << 8) |
                ((uint32_t)block->alpha_indices[5] << 16);
        local_texel = x + y * 4 - 8;
    }
    texel_out->alpha = palette[(group >> (local_texel * 3)) & 7];
}

}  // namespace halo::bitmaps
