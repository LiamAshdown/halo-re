// dxt5_decode_alpha_texel  (Ghidra: dxt5_decode_alpha_texel, already named)
// address 0x440190, size 440 bytes
// name confidence: 0.65   rewrite confidence: 0.85
// evidence: decodes the color channels by delegating to dxt1_decode_block_texel on the block's
//   embedded dxt_color_block (dxt5_block::color, +0x08), then decodes alpha from an 8-entry
//   palette built from alpha0/alpha1 (7-step interpolation when alpha0 > alpha1, else 5-step
//   plus fixed 0/255 entries), indexed by a 3-bit-per-texel selector packed across two 24-bit
//   little-endian groups (rows 0-1 at +0x02, rows 2-3 at +0x05); out/phase4/
//   bitmaps_types_notes.md dxt5_block section and out/phase4/bitmaps_functions.md: "Decodes a
//   single texel's color and interpolated alpha from a DXT5-compressed block." This is the
//   standard DXT5/BC3 alpha block layout.
// register convention: EBX block, stack (texel out, x, y), per bitmaps_types_notes.md
//   ("0x440190: EBX block, stack (texel out, x, y)").
//   // blam-cc: EBX -> block, stack -> texel_out, x, y

#include "tags.h"
#include "bitmaps.h"

extern void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y); // 0x43ffe0, this module

// blam-cc: EBX -> block, stack -> texel_out, x, y
// Decodes texel (x, y) (each in [0, 3]) of a DXT5 block into *texel_out: color from the block's
// embedded DXT1 color block, alpha from the block's 8-entry interpolated alpha palette.
void dxt5_decode_alpha_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y)
{
    uint8_t alpha0, alpha1;
    uint8_t palette[8];
    uint32_t group;
    int32_t local_texel;

    dxt1_decode_block_texel(texel_out, &block->color, x, y);

    alpha0 = block->alpha0;
    alpha1 = block->alpha1;
    palette[0] = alpha0;
    palette[1] = alpha1;
    if (alpha1 < alpha0) {
        // 7-step interpolation (no fully transparent/opaque fixed entries)
        palette[2] = (uint8_t)((alpha1 + alpha0 * 6) / 7);
        palette[3] = (uint8_t)((alpha0 * 5 + alpha1 * 2) / 7);
        palette[4] = (uint8_t)((alpha1 * 3 + alpha0 * 4) / 7);
        palette[5] = (uint8_t)((alpha0 * 3 + alpha1 * 4) / 7);
        palette[6] = (uint8_t)((alpha1 * 5 + alpha0 * 2) / 7);
        palette[7] = (uint8_t)((alpha0 + alpha1 * 6) / 7);
    } else {
        // 5-step interpolation plus fixed fully-transparent (0) and fully-opaque (255) entries
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

#if 0
Original Ghidra decompilation (0x440190):

void dxt5_decode_alpha_texel(int param_1,undefined4 param_2,undefined4 param_3)

{
  byte bVar1;
  byte bVar2;
  uint3 uVar3;
  char cVar4;
  uint uVar5;
  uint uVar6;
  byte *unaff_EBX;
  ushort local_10 [4];
  undefined2 local_8;
  undefined2 local_6;
  undefined2 local_4;
  undefined2 local_2;

  dxt1_decode_block_texel(unaff_EBX + 8,param_2,param_3);
  bVar1 = *unaff_EBX;
  bVar2 = unaff_EBX[1];
  uVar6 = (uint)bVar2;
  local_10[0] = (ushort)bVar1;
  local_10[1] = (ushort)bVar2;
  uVar5 = (uint)(ushort)bVar1;
  if ((ushort)bVar2 < (ushort)bVar1) {
    local_10[2] = (short)((uVar6 + uVar5 * 6) / 7);
    local_10[3] = (short)((uVar5 * 5 + uVar6 * 2) / 7);
    local_8 = (short)((uVar6 * 3 + uVar5 * 4) / 7);
    local_6 = (short)((uVar5 * 3 + uVar6 * 4) / 7);
    local_4 = (short)((uVar6 * 5 + uVar5 * 2) / 7);
    local_2 = (short)((uVar5 + uVar6 * 6) / 7);
  }
  else {
    local_10[2] = (short)((uVar6 + uVar5 * 4) / 5);
    local_10[3] = (short)((uVar5 * 3 + uVar6 * 2) / 5);
    local_8 = (short)((uVar6 * 3 + uVar5 * 2) / 5);
    local_6 = (short)((uVar5 + uVar6 * 4) / 5);
    local_4 = 0;
    local_2 = 0xff;
  }
  if ((short)param_3 < 2) {
    uVar3 = *(uint3 *)(unaff_EBX + 2);
    cVar4 = (char)param_2 + (char)param_3 * '\x04';
  }
  else {
    uVar3 = *(uint3 *)(unaff_EBX + 5);
    cVar4 = (char)param_2 + -8 + (char)param_3 * '\x04';
  }
  *(char *)(param_1 + 3) = (char)local_10[uVar3 >> (cVar4 * '\x03' & 0x1fU) & 7];
  return;
}
#endif
