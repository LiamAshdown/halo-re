// color_interpolate_argb_with_tint  (Ghidra: FUN_0043f7d0, renamed)
// address 0x43f7d0, size 166 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_types_notes.md ("combines two ColorARGB colors (alpha at +0,
// RGB at +4) with an optional tint: a better name is color_interpolate_argb_with_tint");
// out/phase4/bitmaps_functions.md ("Combines two colors into a destination, either by straight
// multiply or by a threshold-weighted interpolation, used for layered color blending (e.g.
// detail/base color combination)."). Ghidra elided every register argument here too (the call
// to color_interpolate() shows no operands), so the reconstruction below follows the objdump at
// 0x43f7d0..0x43f875: it calls color_interpolate(EAX = &color1_argb->red, ECX =
// &color0_argb->red, dest = ESI, flags = EDX, t = the stack float), reinterpreting each
// ColorARGB's RGB tail as a ColorRGB the same way types/bitmaps.h documents for this function.
// register convention: EDX = flags, EBX = ColorARGB *color1 (weighted by t), ESI = ColorRGB
// *dest, EDI = ColorRGB *tint (optional, may be 0), stack -> ColorARGB *color0 (weighted by
// 1-t), t.
//   // blam-cc: EDX -> flags, EBX -> color1, ESI -> dest, EDI -> tint, stack -> color0, t

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest,
    color_interpolation_flags flags, float t); // 0x43f6a0, this batch

// blam-cc: EDX -> flags, EBX -> color1, ESI -> dest, EDI -> tint, stack -> color0, t
ColorRGB *color_interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1,
    ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t)
{
    color_interpolate((ColorRGB *)&color1->red, (ColorRGB *)&color0->red, dest, flags, t);

    if (tint != 0) {
        if (color0->alpha <= 0.0001f && color1->alpha <= 0.0001f) {
            dest->red   = dest->red * tint->red;
            dest->green = tint->green * dest->green;
            dest->blue  = tint->blue * dest->blue;
        } else {
            float weight = t * color1->alpha + (1.0f - t) * color0->alpha;
            float inverse_weight = 1.0f - weight;
            dest->red   = weight * dest->red   + inverse_weight * tint->red;
            dest->green = weight * dest->green + inverse_weight * tint->green;
            dest->blue  = weight * dest->blue  + inverse_weight * tint->blue;
        }
    }
    return dest;
}

#if 0
Original Ghidra decompilation (0x43f7d0):

void FUN_0043f7d0(float *param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;

  color_interpolate();
  if (unaff_EDI != (float *)0x0) {
    if ((*param_1 <= 0.0001) && (*unaff_EBX <= 0.0001)) {
      *unaff_ESI = *unaff_ESI * *unaff_EDI;
      unaff_ESI[1] = unaff_EDI[1] * unaff_ESI[1];
      unaff_ESI[2] = unaff_EDI[2] * unaff_ESI[2];
      return;
    }
    fVar1 = param_2 * *unaff_EBX + (1.0 - param_2) * *param_1;
    fVar2 = 1.0 - fVar1;
    *unaff_ESI = fVar2 * *unaff_EDI + fVar1 * *unaff_ESI;
    unaff_ESI[1] = fVar1 * unaff_ESI[1] + fVar2 * unaff_EDI[1];
    unaff_ESI[2] = fVar1 * unaff_ESI[2] + fVar2 * unaff_EDI[2];
  }
  return;
}

-- objdump of 0x43f7d0..0x43f875 (bin/halo.exe), the call and register setup Ghidra elided:

  43f7d0: mov    eax,DWORD PTR [esp+0x8]      ; t (2nd stack arg, at function entry)
  43f7d4: push   ebp
  43f7d5: mov    ebp,DWORD PTR [esp+0x8]      ; ebp = color0 (1st stack arg)
  43f7d9: push   eax                          ; push t
  43f7da: push   edx                          ; push flags
  43f7db: lea    eax,[ebx+0x4]                ; &color1->red
  43f7de: lea    ecx,[ebp+0x4]                ; &color0->red
  43f7e1: push   esi                          ; push dest
  43f7e2: call   0x43f6a0                     ; color_interpolate(eax, ecx, dest, flags, t)
  43f7e7: add    esp,0xc
  43f7ea: test   edi,edi
  43f7ec: je     0x43f872                     ; tint == 0 -> just return dest
  43f7f2: fld    DWORD PTR [ebp+0x0]          ; color0->alpha
  43f7f5: fcomp  DWORD PTR ds:0x672bbc        ; vs 0.0001
  43f800: je     0x43f82d                     ; color0->alpha > 0.0001 -> weighted path
  43f802: fld    DWORD PTR [ebx]              ; color1->alpha
  43f804: fcomp  DWORD PTR ds:0x672bbc
  43f80f: je     0x43f82d                     ; color1->alpha > 0.0001 -> weighted path
  43f811: fld    DWORD PTR [esi]              ; both alphas <= 0.0001: dest *= tint
  43f813: mov    eax,esi
  43f815: fmul   DWORD PTR [edi]
  43f817: pop    ebp
  43f818: fstp   DWORD PTR [esi]
  43f81a: fld    DWORD PTR [edi+0x4]
  43f81d: fmul   DWORD PTR [esi+0x4]
  43f820: fstp   DWORD PTR [esi+0x4]
  43f823: fld    DWORD PTR [edi+0x8]
  43f826: fmul   DWORD PTR [esi+0x8]
  43f829: fstp   DWORD PTR [esi+0x8]
  43f82c: ret
  43f82d: fld    DWORD PTR ds:0x672ac4        ; 1.0
  43f833: fsub   DWORD PTR [esp+0xc]          ; 1.0 - t
  43f837: fmul   DWORD PTR [ebp+0x0]          ; * color0->alpha
  43f83a: fld    DWORD PTR [esp+0xc]          ; t
  43f83e: fmul   DWORD PTR [ebx]              ; * color1->alpha
  43f840: faddp  st(1),st                     ; weight
  43f842: fld    DWORD PTR ds:0x672ac4        ; 1.0
  43f848: fsub   st,st(1)                     ; inverse_weight = 1.0 - weight
  43f84a..43f86f: dest->{red,green,blue} = weight*dest->{..} + inverse_weight*tint->{..}
  43f872: mov    eax,esi
  43f874: pop    ebp
  43f875: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
