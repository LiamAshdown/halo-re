// object_build_effect_parameter_block  (FUN_004f2ff0; really builds an object's render_lighting from a lightmap sample)
// address 0x4f2ff0, size 1049 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x4f2ff0..0x4f3408. The draft took its seven arguments in a different order from its one
//   caller (object_lighting_sample_point 0x4f2550), so lightmap directions landed in the light colour slots -- the
//   dark blue/purple tint on every object. Registers: ECX the lightmap colour, EAX the lightmap normal, EDX the
//   base map colour, ESI the render_lighting out; stack flags, the shading normal, the lightmap intensity.
//   - ambient = 0.4 * lightmap colour + 0.03 (globals 0x689478 / 0x689474); two distant lights:
//     0: the lightmap colour from the negated lightmap normal; 1: base map colour x luminance (x 0x68947c) from the
//        shading normal;
//   - reflection tint: alpha clamp01(luminance * 1.5 + 0.25), rgb clamp01(base * 3 + 0.5) * clamp01(lightmap * 2 + 0.25);
//   - shadow: the light-0 direction's xy scaled by intensity^0.25, with z = -sqrt(1 - h^2) (h < 0.707) or the xy
//     rescaled to 0.707 and z = -0.7071; shadow colour clamp(1 - 1.3 * light-0 colour + (1 - intensity) / 2, 0.03, 1);
//   - flag 4 (brighter than it should be) clamps ambient 0.2, light 0 0.3, light 1 0.2, tint 0.5 (0x4f3410) and the
//     tint alpha becomes 1.
// blam-cc: stack -> flags, shading_normal, intensity; ECX -> lightmap_color, EAX -> lightmap_normal,
//   EDX -> base_map_color, ESI -> lighting

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_objects.h"

extern double pow(double x, double y);
extern double sqrt(double x);

extern float object_lighting_ambient_bias;      // 0x00689474 (0.03)
extern float object_lighting_ambient_scale;     // 0x00689478 (0.4)
extern float object_lighting_base_light_scale;  // 0x0068947c (1.0)

extern void object_color_clamp_to_intensity(float intensity, ColorRGB *color); // 0x4f3410, stack, ECX

static float clamp_range(float value, float low, float high)
{
    if (!(value >= low)) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

void object_build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal, float intensity,
    ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color, render_lighting *lighting)
{
    float luminance = lightmap_color->red * 0.299f + lightmap_color->blue * 0.114f + lightmap_color->green * 0.587f;
    float tint_red, tint_green;
    float shadow_scale;
    float x, y, h;
    float half_dark;

    lighting->ambient_color.red = object_lighting_ambient_scale * lightmap_color->red + object_lighting_ambient_bias;
    lighting->ambient_color.green = object_lighting_ambient_scale * lightmap_color->green + object_lighting_ambient_bias;
    lighting->ambient_color.blue = object_lighting_ambient_scale * lightmap_color->blue + object_lighting_ambient_bias;
    lighting->distant_light_count = 2;

    lighting->distant_lights[0].color = *lightmap_color;
    lighting->distant_lights[0].direction.i = -lightmap_normal->i;
    lighting->distant_lights[0].direction.j = -lightmap_normal->j;
    lighting->distant_lights[0].direction.k = -lightmap_normal->k;

    lighting->distant_lights[1].color.red = object_lighting_base_light_scale * base_map_color->red * luminance;
    lighting->distant_lights[1].color.green = object_lighting_base_light_scale * base_map_color->green * luminance;
    lighting->distant_lights[1].color.blue = object_lighting_base_light_scale * luminance * base_map_color->blue;
    lighting->distant_lights[1].direction = *shading_normal;

    lighting->reflection_tint.alpha = clamp_range(luminance * 1.5f + 0.25f, 0.0f, 1.0f);
    tint_red = clamp_range(base_map_color->red * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.red = tint_red;
    tint_green = clamp_range(base_map_color->green * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.green = tint_green;
    lighting->reflection_tint.blue = clamp_range(base_map_color->blue * 3.0f + 0.5f, 0.0f, 1.0f);
    lighting->reflection_tint.red = clamp_range(lightmap_color->red + lightmap_color->red + 0.25f, 0.0f, 1.0f) * tint_red;
    lighting->reflection_tint.green = clamp_range(lightmap_color->green + lightmap_color->green + 0.25f, 0.0f, 1.0f) * tint_green;
    lighting->reflection_tint.blue = clamp_range(lightmap_color->blue + lightmap_color->blue + 0.25f, 0.0f, 1.0f) *
        lighting->reflection_tint.blue;

    // 0x4f3274: _CIpow(intensity, 0.25)
    shadow_scale = (float)pow((double)intensity, 0.25);
    x = shadow_scale * lighting->distant_lights[0].direction.i;
    y = shadow_scale * lighting->distant_lights[0].direction.j;
    lighting->shadow_vector.i = x;
    lighting->shadow_vector.j = y;
    h = (float)sqrt((double)(x * x + y * y));
    if (h < 0.707f) {
        lighting->shadow_vector.k = -(float)sqrt((double)(1.0f - h * h));
    } else {
        float rescale = 0.707f / h;

        lighting->shadow_vector.k = -0.70710677f;
        lighting->shadow_vector.i = x * rescale;
        lighting->shadow_vector.j = y * rescale;
    }

    half_dark = (1.0f - intensity) * 0.5f;
    lighting->shadow_color.red = clamp_range(1.0f - lighting->distant_lights[0].color.red * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);
    lighting->shadow_color.green = clamp_range(1.0f - lighting->distant_lights[0].color.green * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);
    lighting->shadow_color.blue = clamp_range(1.0f - lighting->distant_lights[0].color.blue * 1.3f + half_dark,
        object_lighting_ambient_bias, 1.0f);

    if ((flags & 4) != 0) {
        object_color_clamp_to_intensity(0.2f, &lighting->ambient_color);
        object_color_clamp_to_intensity(0.3f, &lighting->distant_lights[0].color);
        object_color_clamp_to_intensity(0.2f, &lighting->distant_lights[1].color);
        object_color_clamp_to_intensity(0.5f, (ColorRGB *)&lighting->reflection_tint.red);
        lighting->reflection_tint.alpha = 1.0f;
    }
}

#if 0
Original Ghidra decompilation (0x4f2ff0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f2ff0(byte param_1,float *param_2,float param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  float10 fVar5;

  fVar1 = in_ECX[1] * 0.587 + in_ECX[2] * 0.114 + *in_ECX * 0.299;
  *unaff_ESI = _DAT_00689478 * *in_ECX + _DAT_00689474;
  unaff_ESI[1] = _DAT_00689478 * in_ECX[1] + _DAT_00689474;
  fVar2 = _DAT_00689478 * in_ECX[2] + _DAT_00689474;
  *(undefined2 *)(unaff_ESI + 3) = 2;
  unaff_ESI[2] = fVar2;
  unaff_ESI[4] = *in_ECX;
  unaff_ESI[5] = in_ECX[1];
  unaff_ESI[6] = in_ECX[2];
  unaff_ESI[7] = -*in_EAX;
  unaff_ESI[8] = -in_EAX[1];
  unaff_ESI[9] = -in_EAX[2];
  unaff_ESI[10] = _DAT_0068947c * *in_EDX * fVar1;
  unaff_ESI[0xb] = _DAT_0068947c * in_EDX[1] * fVar1;
  unaff_ESI[0xc] = _DAT_0068947c * fVar1 * in_EDX[2];
  unaff_ESI[0xd] = *param_2;
  unaff_ESI[0xe] = param_2[1];
  fVar1 = fVar1 * 1.5 + 0.25;
  unaff_ESI[0xf] = param_2[2];
  if (0.0 <= fVar1) {
    if (1.0 < fVar1) {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  unaff_ESI[0x13] = fVar1;
  fVar1 = *in_EDX * 3.0 + 0.5;
  if (0.0 <= fVar1) {
    if (1.0 < fVar1) {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  unaff_ESI[0x14] = fVar1;
  fVar2 = in_EDX[1] * 3.0 + 0.5;
  if (0.0 <= fVar2) {
    if (1.0 < fVar2) {
      fVar2 = 1.0;
    }
  }
  else {
    fVar2 = 0.0;
  }
  unaff_ESI[0x15] = fVar2;
  fVar3 = in_EDX[2] * 3.0 + 0.5;
  if (0.0 <= fVar3) {
    if (1.0 < fVar3) {
      fVar3 = 1.0;
    }
  }
  else {
    fVar3 = 0.0;
  }
  unaff_ESI[0x16] = fVar3;
  fVar4 = *in_ECX + *in_ECX + 0.25;
  if (0.0 <= fVar4) {
    if (1.0 < fVar4) {
      fVar4 = 1.0;
    }
  }
  else {
    fVar4 = 0.0;
  }
  unaff_ESI[0x14] = fVar4 * fVar1;
  fVar1 = in_ECX[1] + in_ECX[1] + 0.25;
  if (0.0 <= fVar1) {
    if (1.0 < fVar1) {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  unaff_ESI[0x15] = fVar1 * fVar2;
  fVar1 = in_ECX[2] + in_ECX[2] + 0.25;
  if (0.0 <= fVar1) {
    if (1.0 < fVar1) {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  unaff_ESI[0x16] = fVar1 * fVar3;
  fVar5 = (float10)FUN_006283c0();
  unaff_ESI[0x17] = (float)(fVar5 * (float10)unaff_ESI[7]);
  unaff_ESI[0x18] = (float)(fVar5 * (float10)unaff_ESI[8]);
  fVar1 = SQRT(unaff_ESI[0x18] * unaff_ESI[0x18] + unaff_ESI[0x17] * unaff_ESI[0x17]);
  if (0.707 <= fVar1) {
    unaff_ESI[0x19] = -0.707;
    unaff_ESI[0x17] = (float)(fVar5 * (float10)unaff_ESI[7]) * (0.707 / fVar1);
    unaff_ESI[0x18] = (float)(fVar5 * (float10)unaff_ESI[8]) * (0.707 / fVar1);
  }
  else {
    unaff_ESI[0x19] = -SQRT(1.0 - fVar1 * fVar1);
  }
  fVar1 = (1.0 - param_3) * 0.5;
  fVar2 = (1.0 - unaff_ESI[4] * 1.3) + fVar1;
  fVar3 = _DAT_00689474;
  if ((_DAT_00689474 <= fVar2) && (fVar3 = fVar2, 1.0 < fVar2)) {
    fVar3 = 1.0;
  }
  unaff_ESI[0x1a] = fVar3;
  fVar2 = (1.0 - unaff_ESI[5] * 1.3) + fVar1;
  fVar3 = _DAT_00689474;
  if ((_DAT_00689474 <= fVar2) && (fVar3 = fVar2, 1.0 < fVar2)) {
    fVar3 = 1.0;
  }
  unaff_ESI[0x1b] = fVar3;
  fVar1 = (1.0 - unaff_ESI[6] * 1.3) + fVar1;
  fVar2 = _DAT_00689474;
  if ((_DAT_00689474 <= fVar1) && (fVar2 = fVar1, 1.0 < fVar1)) {
    fVar2 = 1.0;
  }
  unaff_ESI[0x1c] = fVar2;
  if ((param_1 & 4) != 0) {
    FUN_004f3410(0x3e4ccccd);
    FUN_004f3410(0x3e99999a);
    FUN_004f3410(0x3e4ccccd);
    FUN_004f3410(0x3f000000);
    unaff_ESI[0x13] = 1.0;
  }
  return;
}
#endif
