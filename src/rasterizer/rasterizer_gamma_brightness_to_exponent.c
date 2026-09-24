// rasterizer_gamma_brightness_to_exponent  (Ghidra: FUN_00522890)
// address 0x522890, size 158 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase2/results/rasterizer_01.json ("Converts a stored brightness setting into
// the exponent used to build the display gamma ramp."); called by chimera__registry_check_4
// (0x522520), consistent with rasterizer_gamma_exponent (0x0071d1e0) per types/rasterizer.h.
// The x87 sequence (log2/f2xm1/fscale) is the standard idiom for computing `pow(x, y)` via
// `2^(y*log2(x))`; the constant `0.5019607843137255` (== 128.5/256) and the final clamp to
// [1, 254] both suggest brightness is calibrated so that the ramp's midpoint input maps to
// exponent 1.0.
// register convention: EAX points to a settings record whose ushort at +0x100 is the raw
// brightness slider value.
// Phase 4 review against the raw code: the x87 sequence is x^(ln 0.5 / ln (128/255)) with
//   x = brightness / 65535 (1.5259021896696422e-05), computed as 2^(log2e * ln x * ratio) with
//   f2xm1 / fscale, then scaled by 255.0 (0x672cf8) before __ftol; the earlier guess of a 256
//   scale and a pow(2, log2 x * ratio) form was right in shape but not in constant.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0


extern int32_t rasterizer_gamma_exponent;             // 0x0071d1e0
extern int32_t rasterizer_gamma_exponent_clamped;     // 0x0071d1e4, UNSURE owner

extern double log(double x); // inline fldln2 / fyl2x
extern double exp(double x); // inline fldl2e / f2xm1 / fscale

// blam-cc: EAX = settings
void rasterizer_gamma_brightness_to_exponent(rasterizer_gamma_settings *settings)
{
    uint16_t brightness;
    double brightness_norm;
    double ratio;
    double exponent;
    double scaled;

    brightness = *(uint16_t *)(settings + 0x100);
    brightness_norm = (double)brightness * 1.5259021896696422e-05; // 1/65535
    ratio = log(0.5) / log(0.5019607843137255);                    // fldln2 / fyl2x, 128/255
    exponent = log(brightness_norm) * ratio;
    scaled = exp(exponent) * 255.0;                                 // fldl2e, f2xm1, fscale

    rasterizer_gamma_exponent = (int32_t)scaled;
    if (0xffff0100 < rasterizer_caps.pixel_shader_version) {
        rasterizer_gamma_exponent = rasterizer_gamma_exponent + 10;
    }
    if (rasterizer_gamma_exponent < 1) {
        rasterizer_gamma_exponent = 1;
        rasterizer_gamma_exponent_clamped = 1;
        return;
    }
    if (0xfe < rasterizer_gamma_exponent) {
        rasterizer_gamma_exponent = 0xfe;
    }
    rasterizer_gamma_exponent_clamped = rasterizer_gamma_exponent;
}

#if 0
Original Ghidra decompilation (0x522890):

void FUN_00522890(void)

{
  float10 fVar1;
  int in_EAX;
  float10 fVar2;
  float10 fVar3;

  fVar2 = (float10)log2((float10)*(ushort *)(in_EAX + 0x100) * (float10)1.5259021896696422e-05);
  fVar3 = (float10)log2((float10)0.5);
  fVar1 = (float10)log2((float10)0.5019607843137255);
  fVar3 = (float10)1.4426950408889634 *
          (((float10)0.6931471805599453 * fVar3) / ((float10)0.6931471805599453 * fVar1)) *
          (float10)0.6931471805599453 * fVar2;
  fVar2 = ROUND(fVar3);
  fVar3 = (float10)f2xm1(fVar3 - fVar2);
  fscale((float10)1 + fVar3,fVar2);
  _DAT_0071d1e0 = __ftol(*(ushort *)(in_EAX + 0x100));
  if (0xffff0100 < DAT_007c118c) {
    _DAT_0071d1e0 = _DAT_0071d1e0 + 10;
  }
  if (_DAT_0071d1e0 < 1) {
    _DAT_0071d1e0 = 1;
    _DAT_0071d1e4 = 1;
    return;
  }
  if (0xfe < _DAT_0071d1e0) {
    _DAT_0071d1e0 = 0xfe;
  }
  _DAT_0071d1e4 = _DAT_0071d1e0;
  return;
}
#endif
