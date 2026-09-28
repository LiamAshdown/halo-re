// text_font_system_initialize  (Ghidra: text_font_system_initialize, already named)
// address 0x514820, size 143 bytes
// name confidence: 0.55  rewrite confidence: 0.6
// evidence: allocates and fills a 0x30 byte BitmapData exactly matching the type header's font
//   atlas note ("bitmap class 'bitm', depth 1, format 9, flags 0x41", pixel pointer held at
//   +0x2c rather than the tag-loaded BitmapData's usual +0x24 pointer field) and zeroes/inits
//   font_glyph_cache (0x006d8828, size 0x1010 == 0x404 dwords, matches types/rasterizer.h).
// register convention: none -- __cdecl, no parameters.
// phase 4 review: both helpers take the atlas in a register (EAX and ESI), checked against
//   0x51484d/0x51488e. Earlier note, superseded: bitmap_data_calculate_pixel_data_size and FUN_00523fa0 are both called with no visible
//   arguments; the former almost certainly takes the just-built BitmapData, the latter (outside
//   this session's range) is presumably the texture-create step that turns it into a hardware
//   texture.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

// blam-cc: EAX -> bitmap
extern uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap); // 0x43fb70
// blam-cc: ESI -> bitmap
extern uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap); // 0x523fa0
extern font_glyph_cache g_font_glyph_cache; // 0x006d8828

// Allocates and fills the 512x512 font atlas BitmapData (a4r4g4b4, one mip level), clears
// font_glyph_cache, uploads the atlas as a hardware texture, and marks the cache initialized.
// Returns a nonzero low byte on success (0 on any allocation/upload failure).
int32_t __cdecl text_font_system_initialize(void)
{
    uint8_t *atlas;
    uint32_t pixel_data_size;
    void *pixels;
    uint32_t result;

    atlas = (uint8_t *)GlobalAlloc(0, 0x30);
    result = 0;
    if (atlas != (uint8_t *)0) {
        int i;
        for (i = 0; i < 0x30; i++) {
            atlas[i] = 0;
        }

        *(uint16_t *)(atlas + 0x04) = 0x200;            // width
        *(uint16_t *)(atlas + 0x06) = 0x200;            // height
        *(uint32_t *)(atlas + 0x00) = 0x6269746d;       // bitmap_class 'bitm'
        *(uint16_t *)(atlas + 0x08) = 1;                // depth
        *(uint16_t *)(atlas + 0x0a) = 0;                // type (2d_texture)
        *(uint16_t *)(atlas + 0x0c) = 9;                // format (a4r4g4b4)
        *(uint16_t *)(atlas + 0x14) = 0;                // mipmap_count
        *(uint16_t *)(atlas + 0x0e) = 0x41;             // flags

        pixel_data_size = bitmap_data_calculate_pixel_data_size((BitmapData *)atlas); // EAX = atlas (0x51484d)
        pixels = GlobalAlloc(0, pixel_data_size);
        *(uint32_t *)(atlas + 0x2c) = (uint32_t)pixels;  // BitmapData +0x2c: the pixel pointer of a runtime bitmap

        {
            uint8_t *cache = (uint8_t *)&g_font_glyph_cache;
            for (i = 0; i < 0x1010; i++) {
                cache[i] = 0;
            }
        }

        result = rasterizer_bitmap_create_hardware_texture((BitmapData *)atlas); // ESI = atlas (0x51488e)
        if ((uint8_t)result != 0) {
            g_font_glyph_cache.atlas = (uint32_t)atlas;
            g_font_glyph_cache.initialized = 1;
            return (int32_t)((result & 0xffffff00) | 1);
        }
    }
    return (int32_t)(result & 0xffffff00);
}

#if 0
Original Ghidra decompilation (0x514820):

int __cdecl text_font_system_initialize(void)

{
  undefined4 *puVar1;
  SIZE_T dwBytes;
  HGLOBAL pvVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;

  puVar1 = GlobalAlloc(0,0x30);
  uVar3 = 0;
  if (puVar1 != (undefined4 *)0x0) {
    puVar5 = puVar1;
    for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    *(undefined2 *)(puVar1 + 1) = 0x200;
    *(undefined2 *)((int)puVar1 + 6) = 0x200;
    *puVar1 = 0x6269746d;
    *(undefined2 *)(puVar1 + 2) = 1;
    *(undefined2 *)((int)puVar1 + 10) = 0;
    *(undefined2 *)(puVar1 + 3) = 9;
    *(undefined2 *)(puVar1 + 5) = 0;
    *(undefined2 *)((int)puVar1 + 0xe) = 0x41;
    dwBytes = bitmap_data_calculate_pixel_data_size();
    pvVar2 = GlobalAlloc(0,dwBytes);
    puVar1[0xb] = pvVar2;
    puVar5 = (undefined4 *)&DAT_006d8828;
    for (iVar4 = 0x404; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    uVar3 = FUN_00523fa0();
    if ((char)uVar3 != '\0') {
      DAT_006d8834 = puVar1;
      DAT_006d8828 = 1;
      return CONCAT31((int3)(uVar3 >> 8),1);
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
