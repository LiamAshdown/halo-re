// distance_falloff_fraction  (Ghidra: FUN_00459360; renamed per symbols/review_queue.txt)
// address 0x459360, size 78 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x459360..0x4593ad)
// evidence: symbols/review_queue.txt 0x459360 "returns 0 if param_1>=param_2, 1 if param_1 is
//   below half of param_2, else linearly interpolates between half and full range"; used by
//   camera_observer_target_score (0x459b10) to turn a raw distance or angle into a 0..1 weight
//   against a caller-supplied maximum.
// register convention: both operands are the recognized stack parameters (param_1, param_2);
//   no register arguments.
//   // blam-cc: stack -> value, max_range

#include "tags.h"
#include "math.h"

// Returns 1.0 while `value` is at or below half of `max_range`, 0.0 once it reaches or exceeds
// `max_range`, and linearly interpolates between those two bounds in between. Used to turn a
// raw distance or angle into a falloff weight for the camera-observer target scoring.
real distance_falloff_fraction(real value, real max_range)
{
    real half_range;

    half_range = max_range * 0.5f;
    if (value >= max_range) {
        return 0.0f;
    }
    if (value <= half_range) {
        return 1.0f;
    }
    return (max_range - value) / (max_range - half_range);
}

#if 0
Original Ghidra decompilation (0x459360), from tools/pack.py 0x459360:

float10 FUN_00459360(float param_1,float param_2)

{
  float fVar1;

  fVar1 = param_2 * 0.5;
  if (param_2 <= param_1) {
    return (float10)0.0;
  }
  if (param_1 < fVar1 != (param_1 == fVar1)) {
    return (float10)1.0;
  }
  return ((float10)param_2 - (float10)param_1) / ((float10)param_2 - (float10)fVar1);
}
#endif
