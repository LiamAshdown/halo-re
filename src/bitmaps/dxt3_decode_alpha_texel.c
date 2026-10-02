// dxt3_decode_alpha_texel  (Ghidra: dxt3_decode_alpha_texel, already named)
// address 0x440150, size 54 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: decodes the color channels by delegating to dxt1_decode_block_texel on the block's
//   embedded dxt_color_block (dxt3_block::color, +0x08), then decodes alpha as the explicit
//   4-bit nibble for (x, y) from dxt3_block::alpha_rows, expanded to 8 bits by nibble
//   replication ((nibble << 4) | nibble); out/phase4/bitmaps_types_notes.md dxt3_block section
//   and out/phase4/bitmaps_functions.md: "Decodes a single texel's color and 4-bit explicit
//   alpha from a DXT2/DXT3-compressed block."
// register convention: EDI texel out, stack block, BL x, SI y, per bitmaps_types_notes.md
//   ("0x440150: EDI texel out, stack block, BL x, SI y").
//   // blam-cc: EBX (BL) -> x, ESI (SI) -> y, EDI -> texel_out, stack -> block

#include "tags.h"
#include "bitmaps.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y); // 0x43ffe0, this module

// blam-cc: EBX (BL) -> x, ESI (SI) -> y, EDI -> texel_out, stack -> block
// Decodes texel (x, y) (each in [0, 3]) of a DXT3 block into *texel_out: color from the block's
// embedded DXT1 color block, alpha from the block's explicit 4-bit alpha nibble for row y,
// column x, expanded to 8 bits by nibble replication.
void dxt3_decode_alpha_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block)
{
    uint8_t raw;

    dxt1_decode_block_texel(texel_out, &block->color, x, y);

    raw = (uint8_t)(block->alpha_rows[y] >> ((x & 7) << 2));
    texel_out->alpha = (uint8_t)((raw << 4) | (raw & 0xf));
}

#if 0
Original Ghidra decompilation (0x440150):

void dxt3_decode_alpha_texel(int param_1)

{
  byte bVar1;
  byte unaff_BL;
  short unaff_SI;
  int unaff_EDI;

  dxt1_decode_block_texel(param_1 + 8);
  bVar1 = (byte)(*(ushort *)(param_1 + unaff_SI * 2) >> ((unaff_BL & 7) << 2));
  *(byte *)(unaff_EDI + 3) = bVar1 << 4 | bVar1 & 0xf;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
