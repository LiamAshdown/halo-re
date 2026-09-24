// color_hsv_to_rgb  (Ghidra: color_hsv_to_rgb, already named)
// address 0x43f460, size 296 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/bitmaps_functions.md ("Converts an HSV color back to an RGB float
// triplet, degenerating to greyscale when saturation is 0."); types/bitmaps.h real_hsv_color.
// The sextant is picked with __ftol (x87 truncation) plus a defensive floor: `sector =
// (int)(hue*6); if ((float)sector > hue*6) sector -= 1;`, reproduced here at plain float
// precision instead of the original's 80-bit x87 intermediate.
// register convention: EDI = real_hsv_color *hsv (in), ESI = ColorRGB *color (out).
//   // blam-cc: EDI -> hsv, ESI -> color
// Verified against objdump (phase 4 review): the jump table at 0x43f588 maps sectors 0..5 to the
// six (r, g, b) assignments below; any other sector (unsigned ja) leaves *color unwritten.
// Grey path taken iff saturation == 0 (fucompp; test ah,0x44; jp), so NaN takes the color path.

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"

// blam-cc: EDI -> hsv, ESI -> color; returns color in EAX (mov eax,esi at every exit)
ColorRGB *color_hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color)
{
    if (hsv->saturation != 0.0f) {
        float hue6 = hsv->hue * 6.0f;
        int32_t sector = (int32_t)hue6;
        float frac;
        float p, q, t;

        // floor(hue6): __ftol truncation (0x6391b4), then -1 when (float)sector > hue6
        // (fcomp; test ah,0x41; je at 0x43f4a1..0x43f4ab).
        if ((float)sector > hue6) {
            sector -= 1;
        }
        frac = hue6 - (float)sector;

        p = (1.0f - hsv->saturation) * hsv->value;
        q = (1.0f - frac * hsv->saturation) * hsv->value;
        t = (1.0f - (1.0f - frac) * hsv->saturation) * hsv->value;

        switch (sector) {
        case 0: color->red = hsv->value; color->green = t;         color->blue = p;         break;
        case 1: color->red = q;          color->green = hsv->value; color->blue = p;         break;
        case 2: color->red = p;          color->green = hsv->value; color->blue = t;         break;
        case 3: color->red = p;          color->green = q;          color->blue = hsv->value; break;
        case 4: color->red = t;          color->green = p;          color->blue = hsv->value; break;
        case 5: color->red = hsv->value; color->green = p;          color->blue = q;         break;
        default: break; // sector > 5 unsigned (ja 0x43f580): *color is left untouched, as in the original
        }
    } else {
        color->red = hsv->value;
        color->green = hsv->value;
        color->blue = hsv->value;
    }
    return color;
}

#if 0
Original Ghidra decompilation (0x43f460):

void color_hsv_to_rgb(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  float *unaff_ESI;
  int unaff_EDI;
  float10 extraout_ST0;

  if (*(float *)(unaff_EDI + 4) != 0.0) {
    iVar4 = __ftol();
    iVar5 = iVar4 + -1;
    if ((float10)iVar4 <= extraout_ST0) {
      iVar5 = iVar4;
    }
    fVar1 = (1.0 - *(float *)(unaff_EDI + 4)) * *(float *)(unaff_EDI + 8);
    fVar2 = (1.0 - (float)(extraout_ST0 - (float10)iVar5) * *(float *)(unaff_EDI + 4)) *
            *(float *)(unaff_EDI + 8);
    fVar3 = (1.0 - (1.0 - (float)(extraout_ST0 - (float10)iVar5)) * *(float *)(unaff_EDI + 4)) *
            *(float *)(unaff_EDI + 8);
    switch(iVar5) {
    case 0:
      fVar2 = *(float *)(unaff_EDI + 8);
      unaff_ESI[2] = fVar1;
      *unaff_ESI = fVar2;
      unaff_ESI[1] = fVar3;
      return;
    case 1:
      *unaff_ESI = fVar2;
      fVar2 = *(float *)(unaff_EDI + 8);
      unaff_ESI[2] = fVar1;
      unaff_ESI[1] = fVar2;
      return;
    case 2:
      *unaff_ESI = fVar1;
      unaff_ESI[1] = *(float *)(unaff_EDI + 8);
      unaff_ESI[2] = fVar3;
      return;
    case 3:
      *unaff_ESI = fVar1;
      unaff_ESI[1] = fVar2;
      unaff_ESI[2] = *(float *)(unaff_EDI + 8);
      return;
    case 4:
      unaff_ESI[1] = fVar1;
      *unaff_ESI = fVar3;
      unaff_ESI[2] = *(float *)(unaff_EDI + 8);
      return;
    case 5:
      fVar3 = *(float *)(unaff_EDI + 8);
      unaff_ESI[1] = fVar1;
      *unaff_ESI = fVar3;
      unaff_ESI[2] = fVar2;
      return;
    default:
      return;
    }
  }
  fVar1 = *(float *)(unaff_EDI + 8);
  unaff_ESI[2] = fVar1;
  unaff_ESI[1] = fVar1;
  *unaff_ESI = fVar1;
  return;
}
#endif
