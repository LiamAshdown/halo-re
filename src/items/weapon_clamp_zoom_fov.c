// weapon_clamp_zoom_fov  (Ghidra: FUN_004c2e50; named per types/items.h,
// "k_weapon_zoom_fov_maximum ... the upper clamp in 0x4c2e50")
// address 0x4c2e50, size 71 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/items.h k_weapon_zoom_fov_maximum (0x00672ea0, 3.1101768) /
//   k_weapon_zoom_fov_minimum (0x00672ea4, 0.031415928); calls weapon_get_zoom_magnification
//   (0x4c2d70).
// register convention: item index in EAX (unused by this function's own body but forwarded to
// weapon_get_zoom_magnification), zoom level in DX (unaff_DX, likewise forwarded), base FOV as
// a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, DX -> zoom_level, stack -> base_fov
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
