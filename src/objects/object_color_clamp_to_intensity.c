// object_color_clamp_to_intensity
// address 0x4f3410, size 172 bytes
// name confidence: 0.4 (still FUN_004f3410 in Ghidra; functions.md's summary: "Rescales an
//   in-place RGB color vector (in_ECX) so its brightest channel does not exceed an intensity
//   target derived from param_1")
// rewrite confidence: 0.65
// evidence: types/tags.h ColorRGB (red, green, blue).
// register convention: intensity target in EAX (param_1), color vector in ECX (in_ECX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

void object_color_clamp_to_intensity(float intensity, ColorRGB *color) // blam-cc: EAX -> intensity, ECX -> color
{
    float max_channel = color->green <= color->blue ? color->blue : color->green;
    float scale;

    max_channel = color->red <= max_channel ? max_channel : color->red;

    scale = intensity + 1.0f;
    if (max_channel == 0.0f) {
        scale = 1.0f;
    } else {
        float headroom = max_channel * (intensity + 1.0f);
        if (headroom <= 1.0f) {
            if (intensity <= headroom) {
                goto apply;
            }
        } else {
            intensity = 1.0f;
        }
        scale = intensity / max_channel;
    }
apply:
    color->red *= scale;
    color->green *= scale;
    color->blue *= scale;
}

#if 0
Original Ghidra decompilation (0x4f3410):

void FUN_004f3410(float param_1)

{
  float fVar1;
  float *in_ECX;
  float local_4;

  if (in_ECX[1] <= in_ECX[2]) {
    fVar1 = in_ECX[2];
  }
  else {
    fVar1 = in_ECX[1];
  }
  if (*in_ECX <= fVar1) {
    if (in_ECX[1] <= in_ECX[2]) {
      local_4 = in_ECX[2];
    }
    else {
      local_4 = in_ECX[1];
    }
  }
  else {
    local_4 = *in_ECX;
  }
  fVar1 = param_1 + 1.0;
  if (local_4 == 0.0) {
    fVar1 = 1.0;
  }
  else {
    if (local_4 * fVar1 <= 1.0) {
      if (param_1 <= local_4 * fVar1) goto LAB_004f34a6;
    }
    else {
      param_1 = 1.0;
    }
    fVar1 = param_1 / local_4;
  }
LAB_004f34a6:
  *in_ECX = fVar1 * *in_ECX;
  in_ECX[1] = fVar1 * in_ECX[1];
  in_ECX[2] = fVar1 * in_ECX[2];
  return;
}
#endif
