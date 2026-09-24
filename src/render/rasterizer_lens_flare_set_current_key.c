// rasterizer_lens_flare_set_current_key  (Ghidra: FUN_005120f0; new name, evidence below)
// address 0x5120f0, size 42 bytes
// name confidence: 0.45   rewrite confidence: 0.7
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
uint32_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index,
                                                int16_t bitmap_tag_index, int16_t bitmap_index)
{
    lens_flare_current_key.bitmap_tag_index = bitmap_tag_index;
    if (second_bitmap_tag_index == -1) {
        second_bitmap_tag_index = *(int32_t *)&rasterizer_globals_data->glow.tag_id;
    }
    lens_flare_current_key.second_bitmap_tag_index = second_bitmap_tag_index;
    lens_flare_current_key.bitmap_index = bitmap_index;
    return (uint32_t)second_bitmap_tag_index & 0xffffff00;
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
