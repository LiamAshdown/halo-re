// rasterizer_lens_flare_set_current_key  (Ghidra: FUN_005120f0; new name, evidence below)
// address 0x5120f0, size 42 bytes
// name confidence: 0.45   rewrite confidence: 0.9 (REWRITTEN from objdump 0x5120f0..0x512119: key+0 = sign-extended CX, key+4 = EAX or the rasterizer globals glow bitmap (+0x6c) when -1, key+8 = sign-extended stack word; returns AL = 0 (xor al,al). FIXED: the draft returned EAX with tag bits, so its callers (which test only AL) skipped every lens flare and light volume)
// evidence: types/rasterizer.h documents 0x00746fb0 as lens_flare_current_key
//   (lens_flare_batch_key, size 0x10: bitmap_tag_index +0x00, second_bitmap_tag_index +0x04,
//   bitmap_index +0x08) "written by 0x5120f0". The -1 substitution reads
//   GlobalsRasterizerData.glow.tag_id (offset 0x6c: glow is the 7th TagDependency member,
//   0x60..0x70, tag_id at +0xc) through rasterizer_globals_data (0x0071d164).
// register convention: EAX = second_bitmap_tag_index (or -1 for the glow bitmap default),
//   ECX (low 16, CX) = bitmap_tag_index, stack = bitmap_index.
//   // blam-cc: EAX -> second_bitmap_tag_index, ECX -> bitmap_tag_index, stack -> bitmap_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern lens_flare_batch_key lens_flare_current_key;   // 0x00746fb0
extern GlobalsRasterizerData *rasterizer_globals_data; // 0x0071d164

// Latches the current lens flare batch key (bitmap_tag_index, second_bitmap_tag_index,
// bitmap_index), substituting the glow bitmap's tag id when no second bitmap was given, and
// returns that resolved second bitmap tag index with its low byte cleared.
uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index,
                                               int16_t bitmap_tag_index, int16_t bitmap_index)
{
    lens_flare_current_key.bitmap_tag_index = bitmap_tag_index;
    if (second_bitmap_tag_index == -1) {
        second_bitmap_tag_index = *(int32_t *)&rasterizer_globals_data->glow.tag_id;
    }
    lens_flare_current_key.second_bitmap_tag_index = second_bitmap_tag_index;
    lens_flare_current_key.bitmap_index = bitmap_index;
    // 0x512117: xor al,al -- the result is AL, always 0; both callers test only AL (0x5143f8, 0x4fed19).
    // The draft returned the whole EAX (the tag id with its low byte cleared), which every real bitmap
    // makes non-zero, so lens_flare_render_all skipped every flare and light volumes never drew.
    return 0;
}

#if 0
Original Ghidra decompilation (0x5120f0):

uint FUN_005120f0(short param_1)

{
  uint in_EAX;
  short in_CX;

  DAT_00746fb0 = (int)in_CX;
  if (in_EAX == 0xffffffff) {
    in_EAX = *(uint *)(DAT_0071d164 + 0x6c);
  }
  DAT_00746fb4 = in_EAX;
  DAT_00746fb8 = (int)param_1;
  return in_EAX & 0xffffff00;
}
#endif
