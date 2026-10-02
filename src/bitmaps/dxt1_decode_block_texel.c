// dxt1_decode_block_texel  (Ghidra: dxt1_decode_block_texel, already named)
// address 0x43ffe0, size 357 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: unpacks a dxt_color_block's two endpoint colors with color_565_unpack_to_rgb888,
//   builds the 2 remaining palette entries either as opaque 1/3-2/3 blends (color0 > color1,
//   unsigned) or as an opaque average plus a transparent black (color0 <= color1), then selects
//   one of the 4 by the block's 2-bit-per-texel index at (x, y); out/phase4/
//   bitmaps_types_notes.md dxt_color_block section and out/phase4/bitmaps_functions.md: "Decodes
//   a single texel's color from a DXT1-compressed 4x4 block, handling both the opaque and
//   1-bit-alpha (3-color) block modes." The 1/3-2/3 blend below is `(near*2 + far + 1) / 3`
//   rather than Ghidra's `((near*2 + far + 1) * 0x55555556) >> 32` fixed-point trick; the two
//   were checked to agree for every input in [0, 765] (2*255 + 255).
//   dxt3_decode_alpha_texel (0x440150) and dxt5_decode_alpha_texel (0x440190), both in this
//   module, call this for the color channels of their own texels.
// register convention: EAX ColorARGBInt out, stack (block, x, y), per bitmaps_types_notes.md
//   ("0x43ffe0: EAX ColorARGBInt out, stack (block, x, y)").
//   // blam-cc: EAX -> out, stack -> block, x, y
// NOTE: src/rasterizer/rasterizer_bitmap_sample_texel.c declares this function with a different,
//   explicitly low-confidence signature (`uint32_t dxt1_decode_block_texel(void *block, uint32_t x,
//   uint32_t y)`, i.e. returning the texel instead of writing it through an EAX out-pointer).
//   bitmaps_types_notes.md's register table (built from objdump, not just the decompile) is
//   followed here instead; that rasterizer-side declaration is out of this module and was left
//   unchanged, but is worth reconciling.
// A null block clears k_dxt_block_texel_count (16) consecutive ColorARGBInt entries starting at
// *out* instead of decoding a single texel -- this is not a guess, it is exactly what Ghidra's
// `if (param_1 == 0) { for (16) *in_EAX++ = 0; }` branch does.

#include "tags.h"
#include "bitmaps.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void color_565_unpack_to_rgb888(uint16_t *packed, ColorARGBInt *out); // 0x43ff80, this module

// (2*near + far + 1) / 3, i.e. a color 2/3 of the way from far to near. See the file header for
// the equivalence to Ghidra's fixed-point multiply.
static uint8_t dxt1_blend_two_thirds(uint8_t near_channel, uint8_t far_channel)
{
    return (uint8_t)(((uint32_t)near_channel * 2 + far_channel + 1) / 3);
}

// blam-cc: EAX -> out, stack -> block, x, y
// Decodes the color of texel (x, y) (each in [0, 3]) of a DXT1 block into *out. If block is
// NULL, clears k_dxt_block_texel_count consecutive ColorARGBInt entries starting at out instead
// (a whole decoded 4x4 block, used by callers as a fast path for a missing/transparent block).
void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y)
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

    color_565_unpack_to_rgb888(&block->color0, &palette[0]);
    color_565_unpack_to_rgb888(&block->color1, &palette[1]);
    palette[0].alpha = 0xff;
    palette[1].alpha = 0xff;

    if (block->color1 < block->color0) {
        // opaque 4-color mode: color2/color3 are 2/3-1/3 blends of color0/color1
        palette[2].blue = dxt1_blend_two_thirds(palette[0].blue, palette[1].blue);
        palette[2].green = dxt1_blend_two_thirds(palette[0].green, palette[1].green);
        palette[2].red = dxt1_blend_two_thirds(palette[0].red, palette[1].red);
        palette[2].alpha = 0xff;
        palette[3].blue = dxt1_blend_two_thirds(palette[1].blue, palette[0].blue);
        palette[3].green = dxt1_blend_two_thirds(palette[1].green, palette[0].green);
        palette[3].red = dxt1_blend_two_thirds(palette[1].red, palette[0].red);
        palette[3].alpha = 0xff;
    } else {
        // 3-color + transparent mode: color2 is the average, color3 is transparent black
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

#if 0
Original Ghidra decompilation (0x43ffe0):

void dxt1_decode_block_texel(ushort *param_1,char param_2,char param_3)

{
  undefined4 *in_EAX;
  int iVar1;
  byte local_10 [16];

  if (param_1 == (ushort *)0x0) {
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *in_EAX = 0;
      in_EAX = in_EAX + 1;
    }
    return;
  }
  color_565_unpack_to_rgb888(local_10);
  color_565_unpack_to_rgb888(local_10 + 4);
  local_10[0xb] = 0xff;
  local_10[7] = 0xff;
  local_10[3] = 0xff;
  if (param_1[1] < *param_1) {
    local_10[8] = (char)((ulonglong)
                         ((longlong)(int)(local_10[4] + 1 + (uint)local_10[0] * 2) * 0x55555556) >>
                        0x20);
    local_10[0xc] =
         (char)((ulonglong)((longlong)(int)(local_10[0] + 1 + (uint)local_10[4] * 2) * 0x55555556)
               >> 0x20);
    local_10[9] = (char)((ulonglong)
                         ((longlong)(int)(local_10[5] + 1 + (uint)local_10[1] * 2) * 0x55555556) >>
                        0x20);
    local_10[0xd] =
         (char)((ulonglong)((longlong)(int)(local_10[1] + 1 + (uint)local_10[5] * 2) * 0x55555556)
               >> 0x20);
    local_10[10] = (char)((ulonglong)
                          ((longlong)(int)(local_10[6] + 1 + (uint)local_10[2] * 2) * 0x55555556) >>
                         0x20);
    local_10[0xe] =
         (char)((ulonglong)((longlong)(int)(local_10[2] + 1 + (uint)local_10[6] * 2) * 0x55555556)
               >> 0x20);
    local_10[0xf] = 0xff;
  }
  else {
    iVar1 = 0;
    do {
      local_10[iVar1 + 8] = (byte)(((uint)local_10[iVar1 + 4] + (uint)local_10[iVar1]) / 2);
      local_10[iVar1 + 0xc] = 0;
      iVar1 = iVar1 + 1;
    } while (iVar1 < 3);
    local_10[0xf] = 0;
  }
  *in_EAX = *(undefined4 *)
             (local_10 +
             (*(uint *)(param_1 + 2) >> ((param_2 + param_3 * '\x04') * '\x02' & 0x1fU) & 3) * 4);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
