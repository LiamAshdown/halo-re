// color_565_unpack_to_rgb888  (Ghidra: color_565_unpack_to_rgb888, already named)
// address 0x43ff80, size 85 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: unpacks a 16-bit r5g6b5 word into a 4-byte {blue, green, red, alpha} result
//   (ColorARGBInt, types/tags.h) with alpha forced to 0; out/phase4/bitmaps_functions.md:
//   "Unpacks a packed 16-bit RGB565 color into 8-bit-per-channel RGB bytes." Ghidra's bit
//   shuffling (CONCAT11/CONCAT12 of shifted-and-masked bytes) was checked, input by input over
//   all 65536 possible r5g6b5 values, to compute exactly the textbook 5/6-bit-to-8-bit expansion
//   used below (channel8 = (channelN << (8-N)) | (channelN >> (2*N-8))), in {blue, green, red}
//   byte order with alpha left 0.
// register convention: EAX pointer to the r5g6b5 word, stack ColorARGBInt out, per
//   bitmaps_types_notes.md ("0x43ff80: EAX pointer to the r5g6b5 word, stack ColorARGBInt out").
//   // blam-cc: EAX -> packed, stack -> out

#include "tags.h"
#include "bitmaps.h"

// blam-cc: EAX -> packed, stack -> out
// Unpacks the r5g6b5 color at *packed into out as 8-bit-per-channel blue/green/red, with alpha
// forced to 0.
void color_565_unpack_to_rgb888(uint16_t *packed, ColorARGBInt *out)
{
    uint16_t value;
    uint8_t r5, g6, b5;

    value = *packed;
    r5 = (uint8_t)((value >> 11) & 0x1f);
    g6 = (uint8_t)((value >> 5) & 0x3f);
    b5 = (uint8_t)(value & 0x1f);

    out->blue = (uint8_t)((b5 << 3) | (b5 >> 2));
    out->green = (uint8_t)((g6 << 2) | (g6 >> 4));
    out->red = (uint8_t)((r5 << 3) | (r5 >> 2));
    out->alpha = 0;
}

#if 0
Original Ghidra decompilation (0x43ff80):

void color_565_unpack_to_rgb888(uint *param_1)

{
  ushort uVar1;
  byte bVar2;
  byte bVar3;
  ushort *in_EAX;
  byte bVar4;
  undefined4 local_4;

  uVar1 = *in_EAX;
  local_4._0_1_ = (byte)(uVar1 >> 5);
  bVar4 = (byte)local_4 << 2;
  bVar2 = (byte)local_4 & 0x3f;
  bVar3 = (byte)(uVar1 >> 8);
  local_4 = (uint)CONCAT12(bVar3 & 0xf8 | bVar3 >> 5,
                           CONCAT11(bVar4 | bVar2 >> 4,(byte)uVar1 << 3 | ((byte)uVar1 & 0x1f) >> 2)
                          );
  *param_1 = local_4;
  return;
}
#endif
