// lens_flare_get_visibility_byte  (Ghidra: FUN_005134f0, unnamed; named per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x5134f0, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: reads exactly lens_flare_instance.visibility_high/visibility_low/window_flags
//   (offsets 0x1e/0x20/0x22, confirmed against types/rasterizer.h) and resolves them into a
//   pointer inside lens_flare_marker_visibility (BSP flares, visibility_high < 0) or
//   lens_flare_object_visibility (object flares, visibility_high >= 0, stride 0x0a, +2 == the
//   visibility[8] array) -- the same two tables and stride the type header documents.
// register convention: flare record in in_ECX. // blam-cc: ECX -> flare

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t lens_flare_marker_visibility[0x10008]; // 0x006be810
extern lens_flare_object_visibility lens_flare_object_visibility_table[k_lens_flare_object_visibility_slots]; // 0x006bc510

// blam-cc: ECX -> flare
// Resolves a lens flare instance's smoothed visibility byte: for a BSP marker flare
// (visibility_high < 0) it indexes lens_flare_marker_visibility by the packed marker offset in
// visibility_high/visibility_low plus the window bits of window_flags; for an object flare it
// indexes lens_flare_object_visibility_bytes by object index (stride 0x0a, +2 for the per
// window visibility array) plus the same window bits.
uint8_t *lens_flare_get_visibility_byte(lens_flare_instance *flare)
{
    if (flare->visibility_high < 0) {
        return lens_flare_marker_visibility +
               (flare->window_flags & 0xffffff7f) +
               (((uint32_t)(uint16_t)flare->visibility_high & 0x7fff) << 0x10 |
                (uint32_t)(int32_t)flare->visibility_low);
    }
    return (uint8_t *)lens_flare_object_visibility_table +
           (flare->window_flags & 0xffffff7f) + (int32_t)flare->visibility_high * 10 + 2 +
           (int32_t)flare->visibility_low;
}

#if 0
Original Ghidra decompilation (0x5134f0):

int FUN_005134f0(void)

{
  ushort uVar1;
  int in_ECX;

  uVar1 = *(ushort *)(in_ECX + 0x1e);
  if ((short)uVar1 < 0) {
    return (int)&DAT_006be810 +
           (*(byte *)(in_ECX + 0x22) & 0xffffff7f) +
           ((uVar1 & 0x7fff) << 0x10 | (int)*(short *)(in_ECX + 0x20));
  }
  return (*(byte *)(in_ECX + 0x22) & 0xffffff7f) + (short)uVar1 * 10 + 0x6bc512 +
         (int)*(short *)(in_ECX + 0x20);
}
#endif
