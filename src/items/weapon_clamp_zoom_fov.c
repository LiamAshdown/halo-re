// weapon_clamp_zoom_fov  (Ghidra: FUN_004c2e50; named per types/items.h,
// "k_weapon_zoom_fov_maximum ... the upper clamp in 0x4c2e50")
// address 0x4c2e50, size 71 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/items.h k_weapon_zoom_fov_maximum (0x00672ea0, 3.1101768) /
//   k_weapon_zoom_fov_minimum (0x00672ea4, 0.031415928); calls weapon_get_zoom_magnification
//   (0x4c2d70).
// zoom level in DX (unaff_DX, forwarded to weapon_get_zoom_magnification); item index and base FOV
// on the stack.
// FIXED (objdump 0x4c2e51): item_index is the FIRST STACK argument ([esp+8] after push ecx), moved into EAX
// only for weapon_get_zoom_magnification; EAX is not an input. The only caller (0x471ffb) pushes fov, then item.
// blam-cc: DX -> zoom_level, stack -> item_index, base_fov
// UNSURE: `param_1` in the original is never read; it is kept here only as the item_index that
// flows through to weapon_get_zoom_magnification.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "items.h"

extern real k_weapon_zoom_fov_maximum; // 0x00672ea0
extern real k_weapon_zoom_fov_minimum; // 0x00672ea4
extern real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level); // 0x4c2d70

// Converts a magnification factor for one zoom level into a target field of view, falling back
// to the unclamped base FOV when the magnification is trivial (1.0) or the resulting FOV would
// fall outside the tag-independent [min, max] zoom FOV range.
real weapon_clamp_zoom_fov(datum_index item_index, int16_t zoom_level, real base_fov)
{
    real magnification;
    real fov;

    magnification = weapon_get_zoom_magnification(item_index, zoom_level);
    if (magnification == 1.0f) {
        return base_fov;
    }
    fov = base_fov / magnification;
    if (fov <= k_weapon_zoom_fov_minimum || fov >= k_weapon_zoom_fov_maximum) {
        return base_fov;
    }
    return fov;
}

#if 0
Original Ghidra decompilation (0x4c2e50):

float10 FUN_004c2e50(undefined4 param_1,float param_2)

{
  float10 fVar1;

  fVar1 = (float10)FUN_004c2d70();
  if (((fVar1 == (float10)1.0) || (fVar1 = (float10)param_2 / fVar1, fVar1 <= (float10)0.03141593))
     || ((float10)3.1101768 <= fVar1)) {
    fVar1 = (float10)param_2;
  }
  return fVar1;
}
#endif
