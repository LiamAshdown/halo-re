// biped_update_animation_frame_trigger  (Ghidra: biped_update_animation_frame_trigger, renamed)
// address 0x55eaa0, size 232 bytes
// name confidence: 0.3   rewrite confidence: 0.9
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

// REWRITTEN from objdump 0x55eaa0..0x55eb87. Stack: threshold (seconds); ECX: the Biped tag; ESI: the biped object.
//   With t0 / t1 / t2 = tag +0x3dc / +0x3e0 / +0x3e4 in ticks / 30: nothing before t0. Before t1 the fraction is
//   (threshold - t0) / (t1 - t0) of tag +0x3d4 (phase 0); from t1 on it is threshold / (t2 - t1) of tag +0x3d8
//   (phase 1 -- the binary does not subtract t1 there). With a positive span: +0x508 = phase, +0x4d0 = 0 and
//   +0x4d1 = trunc(value * 30 * clamp(fraction, 0, 1)). The draft stored trunc(span) and ignored +0x3d4 / +0x3d8.
// blam-cc: stack -> threshold, ECX -> timing_table (Biped tag), ESI -> object_base
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base)
{
    uint8_t *biped = (uint8_t *)object_base;
    float t0 = *(float *)(timing_table + 0x3dc) * 0.033333335f;
    float t1 = *(float *)(timing_table + 0x3e0) * 0.033333335f;
    float numerator;
    float span;
    float value;
    float fraction;
    int16_t phase;

    if (threshold < t0) {
        return;
    }
    if (!(threshold >= t1)) {
        numerator = threshold - t0;
        span = t1 - t0;
        value = *(float *)(timing_table + 0x3d4);
        phase = 0;
    } else {
        numerator = threshold;
        span = *(float *)(timing_table + 0x3e4) * 0.033333335f - t1;
        value = *(float *)(timing_table + 0x3d8);
        phase = 1;
    }
    value = value * 30.0f;
    if (!(span > 0.0f)) {
        return;
    }
    fraction = numerator / span;
    if (!(fraction >= 0.0f)) {
        fraction = 0.0f;
    } else if (!(fraction <= 1.0f)) {
        fraction = 1.0f;
    }
    *(int16_t *)(biped + 0x508) = phase;
    biped[0x4d0] = 0;
    biped[0x4d1] = (uint8_t)(int32_t)(value * fraction); // __ftol
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
