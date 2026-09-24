// color_rgb_to_hsv  (Ghidra: color_rgb_to_hsv, already named)
// address 0x43f330, size 290 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Converts an RGB float triplet to HSV (hue in
// [0,1), saturation, value)."); types/bitmaps.h real_hsv_color (hue *6 sextant convention,
// written here). The two redundant re-computations of max(g,b)/min(g,b) that Ghidra shows for
// the already-taken branches evaluate to the exact same value as the first computation in every
// case (they are the same sub-expression), so this rewrite keeps a single max()/min() each,
// which is semantically identical for every input, not a behavioural change.
// register convention: ECX = ColorRGB *color (in), EDX = real_hsv_color *hsv (out).
//   // blam-cc: ECX -> color, EDX -> hsv

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"

// blam-cc: ECX -> color, EDX -> hsv; returns hsv in EAX (mov eax,edx at both exits)
real_hsv_color *color_rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv)
{
    float max, min, delta;
    float hue;

    max = color->red;
    if (max < color->green) max = color->green;
    if (max < color->blue) max = color->blue;

    min = color->red;
    if (min > color->green) min = color->green;
    if (min > color->blue) min = color->blue;

    delta = max - min;
    hsv->value = max;
    hsv->saturation = (max == 0.0f) ? 0.0f : delta / max;

    if (hsv->saturation == 0.0f) {
        hsv->hue = 0.0f;
        return hsv;
    }

    if (color->red == max) {
        hue = (color->green - color->blue) / delta;
    } else if (color->green == max) {
        hue = (color->blue - color->red) / delta + 2.0f;
    } else {
        hue = (color->red - color->green) / delta + 4.0f;
    }

    hue = hue * 0.16666667f; // /6, picks the sextant fraction
    if (hue < 0.0f) {
        hue = hue + 1.0f;
    }
    hsv->hue = hue;
    return hsv;
}

#if 0
Original Ghidra decompilation (0x43f330):

void color_rgb_to_hsv(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_ECX;
  float *in_EDX;

  if (in_ECX[1] <= in_ECX[2]) {
    fVar1 = in_ECX[2];
  }
  else {
    fVar1 = in_ECX[1];
  }
  if (*in_ECX <= fVar1) {
    if (in_ECX[1] <= in_ECX[2]) {
      fVar1 = in_ECX[2];
    }
    else {
      fVar1 = in_ECX[1];
    }
  }
  else {
    fVar1 = *in_ECX;
  }
  if (in_ECX[1] <= in_ECX[2]) {
    fVar2 = in_ECX[1];
  }
  else {
    fVar2 = in_ECX[2];
  }
  if (*in_ECX <= fVar2) {
    fVar2 = *in_ECX;
  }
  else if (in_ECX[1] <= in_ECX[2]) {
    fVar2 = in_ECX[1];
  }
  else {
    fVar2 = in_ECX[2];
  }
  fVar2 = fVar1 - fVar2;
  in_EDX[2] = fVar1;
  if (fVar1 == 0.0) {
    fVar3 = 0.0;
  }
  else {
    fVar3 = fVar2 / fVar1;
  }
  in_EDX[1] = fVar3;
  if (fVar3 == 0.0) {
    *in_EDX = 0.0;
  }
  else {
    if (*in_ECX == fVar1) {
      fVar2 = (in_ECX[1] - in_ECX[2]) / fVar2;
    }
    else if (in_ECX[1] == fVar1) {
      fVar2 = (in_ECX[2] - *in_ECX) / fVar2 + 2.0;
    }
    else {
      fVar2 = (*in_ECX - in_ECX[1]) / fVar2 + 4.0;
    }
    *in_EDX = fVar2;
    fVar1 = *in_EDX * 0.16666667;
    *in_EDX = fVar1;
    if (fVar1 < 0.0) {
      *in_EDX = fVar1 + 1.0;
      return;
    }
  }
  return;
}
#endif
