// color_interpolate  (Ghidra: color_interpolate, already named)
// address 0x43f6a0, size 303 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Blends two RGB colors by a factor param_3, either
// linearly in RGB space or by converting to HSV and interpolating hue, depending on a mode flag
// in param_2."); types/bitmaps.h color_interpolation_flags and real_hsv_color; objdump of
// bin/halo.exe at 0x43f6a0..0x43f78f (Ghidra elided every register argument to the two
// color_rgb_to_hsv calls and to color_hsv_to_rgb, so the HSV branch below is reconstructed from
// the raw x87/stack sequence rather than the decompile): color_rgb_to_hsv is called once for
// each input color (ECX -> [esp+0x18] for the (1-t)-weighted color, then ECX -> [esp+0x24] for
// the t-weighted color); the long-hue-path bit is compared against `fabsf(h0 - h1) > 0.5`
// (constant at 0x672cf0) to decide which of the two hues gets +1.0 before the blend; the
// blended real_hsv_color is built at [esp+0xc/0x10/0x14] and passed to color_hsv_to_rgb.
// register convention: EAX = ColorRGB *color1 (weighted by t), ECX = ColorRGB *color0 (weighted
// by 1-t), stack -> dest, flags, t.
//   // blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"

extern float fabsf(float x); // x87 FABS

extern real_hsv_color *color_rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv); // 0x43f330, this batch
extern ColorRGB *color_hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color); // 0x43f460, this batch

// blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest,
    color_interpolation_flags flags, float t)
{
    float one_minus_t = 1.0f - t;

    if ((flags & _color_interpolation_hsv_bit) == 0) {
        dest->red   = t * color1->red   + one_minus_t * color0->red;
        dest->green = t * color1->green + one_minus_t * color0->green;
        dest->blue  = t * color1->blue  + one_minus_t * color0->blue;
        return dest;
    }

    {
        real_hsv_color hsv0, hsv1, hsv;
        uint8_t hue_far_apart;
        uint8_t take_long_path;

        color_rgb_to_hsv(color0, &hsv0);
        color_rgb_to_hsv(color1, &hsv1);

        hue_far_apart = fabsf(hsv0.hue - hsv1.hue) > 0.5f;
        take_long_path = (uint8_t)((flags >> 1) & 1); // _color_interpolation_long_hue_path_bit

        if (hue_far_apart != take_long_path) {
            // Send whichever hue is smaller a full turn around the wheel, so the linear blend
            // below takes the requested side (short or long way) of the color wheel.
            if (hsv0.hue < hsv1.hue) {
                hsv0.hue = hsv0.hue + 1.0f;
            } else {
                hsv1.hue = hsv1.hue + 1.0f;
            }
        }

        hsv.hue = one_minus_t * hsv0.hue + t * hsv1.hue;
        if (hsv.hue > 1.0f) {
            hsv.hue = hsv.hue - 1.0f;
        }
        hsv.saturation = one_minus_t * hsv0.saturation + t * hsv1.saturation;
        hsv.value      = one_minus_t * hsv0.value      + t * hsv1.value;

        color_hsv_to_rgb(&hsv, dest);
    }
    return dest;
}

#if 0
Original Ghidra decompilation (0x43f6a0):

float * color_interpolate(float *param_1,uint param_2,float param_3)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;

  fVar1 = 1.0 - param_3;
  if ((param_2 & 1) == 0) {
    *param_1 = param_3 * *in_EAX + fVar1 * *in_ECX;
    param_1[1] = param_3 * in_EAX[1] + fVar1 * in_ECX[1];
    param_1[2] = param_3 * in_EAX[2] + fVar1 * in_ECX[2];
    return param_1;
  }
  color_rgb_to_hsv();
  color_rgb_to_hsv();
  color_hsv_to_rgb();
  return param_1;
}

-- objdump of 0x43f6a0..0x43f78f (bin/halo.exe), the HSV branch Ghidra collapsed above:

  43f6a0: sub    esp,0x24
  43f6a3: fld    DWORD PTR ds:0x672ac4        ; 1.0
  43f6a9: push   ebx
  43f6aa: mov    ebx,DWORD PTR [esp+0x30]     ; flags
  43f6ae: fsub   DWORD PTR [esp+0x34]         ; 1.0 - t
  43f6b2: test   bl,0x1
  43f6b5: push   esi
  43f6b6: mov    esi,DWORD PTR [esp+0x30]     ; dest
  43f6ba: fstp   DWORD PTR [esp+0x34]         ; store (1-t) over the flags stack slot (scratch)
  43f6be: push   edi
  43f6bf: mov    edi,eax                      ; edi = color1 (the EAX-in color)
  43f6c1: je     0x43f790                     ; RGB path
  43f6c7: lea    edx,[esp+0x18]
  43f6cb: call   0x43f330                     ; color_rgb_to_hsv(ecx=color0, edx=&hsv0)
  43f6d0: lea    edx,[esp+0x24]
  43f6d4: mov    ecx,edi
  43f6d6: call   0x43f330                     ; color_rgb_to_hsv(ecx=color1, edx=&hsv1)
  43f6db: fld    DWORD PTR [esp+0x18]         ; h0
  43f6df: fld    DWORD PTR [esp+0x24]         ; h1
  43f6e3: fld    st(1)
  43f6e5: fsub   st,st(1)                     ; h0 - h1
  43f6e7: fabs
  43f6e9: fcomp  QWORD PTR ds:0x672cf0        ; vs 0.5
  43f6ef: fnstsw ax
  43f6f1: test   ah,0x41
  43f6f4: jne    0x43f6fd
  43f6f6: mov    eax,0x1                      ; eax = far_apart (|h0-h1| > 0.5)
  43f6fb: jmp    0x43f6ff
  43f6fd: xor    eax,eax
  43f6ff: shr    ebx,1
  43f701: and    ebx,0x1                      ; ebx = long_hue_path bit
  43f704: cmp    eax,ebx
  43f706: je     0x43f725                     ; equal -> no hue adjustment
  43f708: fld    st(1)
  43f70a: fcomp  st(1)                        ; h0 vs h1
  43f70c: fnstsw ax
  43f70e: test   ah,0x5
  43f711: jp     0x43f71f                     ; h0 >= h1 (or unordered) -> add to h1
  43f713: fxch   st(1)
  43f715: fadd   DWORD PTR ds:0x672ac4        ; h0 += 1.0
  43f71b: fxch   st(1)
  43f71d: jmp    0x43f725
  43f71f: fadd   DWORD PTR ds:0x672ac4        ; h1 += 1.0
  43f725: fxch   st(1)
  43f727: fmul   DWORD PTR [esp+0x38]         ; * (1-t)
  43f72b: fxch   st(1)
  43f72d: fmul   DWORD PTR [esp+0x3c]         ; * t
  43f731: faddp  st(1),st                     ; blended hue
  43f733: fst    DWORD PTR [esp+0xc]
  43f737: fcomp  DWORD PTR ds:0x672ac4        ; vs 1.0
  43f73d: fnstsw ax
  43f73f: test   ah,0x41
  43f742: jne    0x43f752                     ; hue <= 1.0 -> skip wrap
  43f744: fld    DWORD PTR [esp+0xc]
  43f748: fsub   DWORD PTR ds:0x672ac4        ; hue -= 1.0
  43f74e: fstp   DWORD PTR [esp+0xc]
  43f752: fld    DWORD PTR [esp+0x1c]         ; s0
  43f756: lea    edi,[esp+0xc]                ; &blended hsv, for color_hsv_to_rgb
  43f75a: fmul   DWORD PTR [esp+0x38]
  43f75e: fld    DWORD PTR [esp+0x28]         ; s1
  43f762: fmul   DWORD PTR [esp+0x3c]
  43f766: faddp  st(1),st
  43f768: fstp   DWORD PTR [esp+0x10]         ; blended saturation
  43f76c: fld    DWORD PTR [esp+0x20]         ; v0
  43f770: fmul   DWORD PTR [esp+0x38]
  43f774: fld    DWORD PTR [esp+0x2c]         ; v1
  43f778: fmul   DWORD PTR [esp+0x3c]
  43f77c: faddp  st(1),st
  43f77e: fstp   DWORD PTR [esp+0x14]         ; blended value
  43f782: call   0x43f460                     ; color_hsv_to_rgb(edi=&hsv, esi=dest)
  43f787: pop    edi
  43f788: mov    eax,esi
  43f78a: pop    esi
  43f78b: pop    ebx
  43f78c: add    esp,0x24
  43f78f: ret
#endif
