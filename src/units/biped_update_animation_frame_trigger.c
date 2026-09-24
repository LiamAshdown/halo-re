// biped_update_animation_frame_trigger  (Ghidra: biped_update_animation_frame_trigger, renamed)
// address 0x55eaa0, size 232 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: biped_data.unknown_4d0/unknown_4d1/unknown_508 match types/units.h ("frame counter
//   0x55eb90 advances" / "the frame count it is compared against, loaded by 0x55eaa0" /
//   "0x55eaa0 stores a 0/1 comparison result here").
// register convention: a timing-table pointer in ECX, the biped's object base pointer directly
//   in ESI (not an index), threshold in param_1.
//   // blam-cc: ECX -> timing_table, ESI -> object_base, stack -> threshold
// UNSURE: timing_table's own layout (offsets 0x3dc/0x3e0/0x3e4, seconds after dividing by 30) is
//   not identified against any documented struct -- kept as a raw pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern int32_t __ftol(); // 0x6391b4, MSVC 7.1 CRT float-to-int truncation; the double is on the x87 stack

// Compares a unit's animation timing-table entries against threshold and, if the gap between
// them is positive, updates the biped's frame-tracking state (unknown_4d0/unknown_4d1) and a
// comparison flag (unknown_508).
void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base)
{
    biped_data *biped = (biped_data *)((uint8_t *)object_base + k_unit_object_size);
    float t0 = *(float *)(timing_table + 0x3dc) * 0.033333335f;
    float t1 = *(float *)(timing_table + 0x3e0) * 0.033333335f;
    float gap;

    if (t0 > threshold) {
        return;
    }
    if (t1 > threshold) {
        gap = t1 - t0;
    } else {
        gap = *(float *)(timing_table + 0x3e4) * 0.033333335f - t1;
    }
    if (gap > 0.0f) {
        biped->unknown_508 = (t1 <= threshold);
        biped->unknown_4d0 = 0;
        biped->unknown_4d1 = (int8_t)__ftol(gap);
    }
}

#if 0
Original Ghidra decompilation (0x55eaa0):

void FUN_0055eaa0(float param_1)

{
  float fVar1;
  float fVar2;
  undefined1 uVar3;
  int in_ECX;
  int unaff_ESI;

  fVar1 = *(float *)(in_ECX + 0x3dc) * 0.033333335;
  fVar2 = *(float *)(in_ECX + 0x3e0) * 0.033333335;
  if (fVar1 <= param_1) {
    if (fVar2 <= param_1) {
      fVar1 = *(float *)(in_ECX + 0x3e4) * 0.033333335 - fVar2;
    }
    else {
      fVar1 = fVar2 - fVar1;
    }
    if (0.0 < fVar1) {
      *(ushort *)(unaff_ESI + 0x508) = (ushort)(fVar2 <= param_1);
      *(undefined1 *)(unaff_ESI + 0x4d0) = 0;
      uVar3 = __ftol();
      *(undefined1 *)(unaff_ESI + 0x4d1) = uVar3;
      return;
    }
  }
  return;
}
#endif
