// object_build_effect_parameter_block
// address 0x4f2ff0, size 1049 bytes
// name confidence: 0.3 (still FUN_004f2ff0 in Ghidra; functions.md's summary: "Builds a
//   per-effect parameter block (color, randomized direction, saturation thresholds) from an
//   object's base/damage colors and normal vector, likely feeding a decal or particle-effect
//   spawn")
// rewrite confidence: 0.2
// evidence: callee object_color_clamp_to_intensity (0x4f3410, this batch, matched onto the four
//   FUN_004f3410 calls at the end, though the ColorRGB* each call targets is not visible here
//   and is not guessed at). Nothing else in this function is covered by types/objects.h -- the
//   four hidden pointers (in_EAX/in_ECX/in_EDX/unaff_ESI) and the output block's true type
//   belong to whatever decal/particle/effect system consumes this block, entirely outside this
//   module's recovered types.
// register convention: flags byte in EAX (param_1), a 3-float vector in ECX (in_ECX, used as a
//   base color), a 3-float vector in EDX (in_EDX, used as a damage color), a 3-float vector in
//   EAX... -- Ghidra could not resolve which physical registers carry in_EAX/in_ECX/in_EDX/
//   unaff_ESI at all; they are listed here in the order Ghidra printed them, not a verified
//   register assignment. param_2 (a 3-float vector) and param_3 (a scalar) are the two visible
//   stack/register parameters.
// UNSURE: essentially this entire function. It is transliterated close to line-for-line from
//   the decompilation, with the output block kept as a flat float array (matching the indices
//   Ghidra itself used) and the four hidden input vectors kept as real_vector3d* rather than
//   guessing a named struct. The DAT_00689474/78/7c constants are preserved as opaque named
//   externs (a small "effect blend constants" table) since their exact values/meaning were not
//   examined.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern float effect_blend_constant_474; // 0x00689474, UNSURE: unexamined
extern float effect_blend_constant_478; // 0x00689478, UNSURE: unexamined
extern float effect_blend_constant_47c; // 0x0068947c, UNSURE: unexamined

extern double sqrt(double x); // see object_lights_gather_nearest.c
// 0x6283c0 is the MSVC x87 pow intrinsic (_CIpow): it checks MXCSR/the FPU control word and
// tail-jumps to 0x6328a0, taking both operands off the x87 stack rather than the C stack. The
// call site at 0x4f327e pushes a float from the frame and then the double constant at
// 0x00672d58, which the image holds as exactly 0.25 -- so this is pow(x, 0.25), a fourth root,
// NOT the random-number source an earlier draft guessed at.
extern double pow(double x, double y); // CRT pow (0x6283c0: _CIpow, SSE2-dispatched)
extern void object_color_clamp_to_intensity(float intensity, ColorRGB *color); // 0x4f3410, this
    // batch; UNSURE: called with no visible color pointer at this call site, see file header

static float clamp01(float v)
{
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}

void object_build_effect_parameter_block(uint8_t param_1, real_vector3d *param_2, float param_3,
    real_vector3d *hidden_eax, real_vector3d *base_color, real_vector3d *damage_color,
    float *output)
    // blam-cc: EAX -> param_1, stack -> param_2, param_3; UNSURE: hidden_eax/base_color/
    //          damage_color/output are Ghidra's in_EAX/in_ECX/in_EDX/unaff_ESI, register
    //          assignment not verified (see file header)
{
    float luminance = base_color->j * 0.587f + base_color->k * 0.114f + base_color->i * 0.299f;
    float t1, t2, t3, t4;
    float len;
    double gamma; // pow(x, 0.25)

    output[0] = effect_blend_constant_478 * base_color->i + effect_blend_constant_474;
    output[1] = effect_blend_constant_478 * base_color->j + effect_blend_constant_474;
    t1 = effect_blend_constant_478 * base_color->k + effect_blend_constant_474;
    *(int16_t *)(output + 3) = 2;
    output[2] = t1;
    output[4] = base_color->i;
    output[5] = base_color->j;
    output[6] = base_color->k;
    output[7] = -hidden_eax->i;
    output[8] = -hidden_eax->j;
    output[9] = -hidden_eax->k;
    output[10] = effect_blend_constant_47c * damage_color->i * luminance;
    output[11] = effect_blend_constant_47c * damage_color->j * luminance;
    output[12] = effect_blend_constant_47c * luminance * damage_color->k;
    output[13] = param_2->i;
    output[14] = param_2->j;
    luminance = luminance * 1.5f + 0.25f;
    output[15] = param_2->k;
    output[0x13] = clamp01(luminance);

    t1 = clamp01(base_color->i * 3.0f + 0.5f);
    output[0x14] = t1;
    t2 = clamp01(base_color->j * 3.0f + 0.5f);
    output[0x15] = t2;
    t3 = clamp01(base_color->k * 3.0f + 0.5f);
    output[0x16] = t3;

    t4 = clamp01(base_color->i + base_color->i + 0.25f);
    output[0x14] = t4 * t1;
    t1 = clamp01(base_color->j + base_color->j + 0.25f);
    output[0x15] = t1 * t2;
    t1 = clamp01(base_color->k + base_color->k + 0.25f);
    output[0x16] = t1 * t3;

    gamma = pow(luminance, 0.25); // UNSURE: the base is the float at [esp+0x1c];
                                          // the exponent 0.25 is read from the image
    output[0x17] = (float)(gamma * output[7]);
    output[0x18] = (float)(gamma * output[8]);
    len = (float)sqrt((double)(output[0x18] * output[0x18] + output[0x17] * output[0x17]));
    if (len >= 0.707f) {
        output[0x19] = -0.707f;
        output[0x17] = (float)(gamma * output[7]) * (0.707f / len);
        output[0x18] = (float)(gamma * output[8]) * (0.707f / len);
    } else {
        output[0x19] = -(float)sqrt((double)(1.0f - len * len));
    }

    t1 = (1.0f - param_3) * 0.5f;
    t2 = (1.0f - output[4] * 1.3f) + t1;
    t3 = effect_blend_constant_474;
    if (effect_blend_constant_474 <= t2) {
        t3 = t2;
        if (t2 > 1.0f) {
            t3 = 1.0f;
        }
    }
    output[0x1a] = t3;

    t2 = (1.0f - output[5] * 1.3f) + t1;
    t3 = effect_blend_constant_474;
    if (effect_blend_constant_474 <= t2) {
        t3 = t2;
        if (t2 > 1.0f) {
            t3 = 1.0f;
        }
    }
    output[0x1b] = t3;

    t1 = (1.0f - output[6] * 1.3f) + t1;
    t2 = effect_blend_constant_474;
    if (effect_blend_constant_474 <= t1) {
        t2 = t1;
        if (t1 > 1.0f) {
            t2 = 1.0f;
        }
    }
    output[0x1c] = t2;

    if ((param_1 & 4) != 0) {
        // UNSURE: none of the four ColorRGB* targets are visible at these call sites. The
        // guesses below pick the four 3-float regions this function itself produced, in roughly
        // the order they were computed, purely because the count (4) and shapes line up; there
        // is no direct evidence tying a given call to a given region.
        object_color_clamp_to_intensity(0.2f, (ColorRGB *)(output + 0));
        object_color_clamp_to_intensity(0.3f, (ColorRGB *)(output + 4));
        object_color_clamp_to_intensity(0.2f, (ColorRGB *)(output + 10));
        object_color_clamp_to_intensity(0.5f, (ColorRGB *)(output + 0x1a));
        output[0x13] = 1.0f;
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
