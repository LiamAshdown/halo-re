// chimera__gamma  (Ghidra: already named; Chimera name, hint only)
// address 0x5227a0, size 233 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: raw disassembly (phase 4 review). Builds rasterizer_game_gamma_ramp (0x006e1118):
//   exponent = ln(gamma / 255) / ln(0.5) computed inline (fldln2 / fyl2x), then for each of the
//   256 entries pow(i / 255, exponent) * 65535 truncated by __ftol into all three channels;
//   0x6283c0 is the CRT _CIpow (base and exponent on the x87 stack), not an opaque call.
//   Applies it with IDirect3DDevice9::SetGammaRamp (+0x54) when 0x006e1718 is 1, the device is
//   fullscreen and exists, otherwise with GetDC / SetDeviceGammaRamp / ReleaseDC.
//   Phase 4 fix: the early return needs both 0x0071d1e8 and 0x0071d1ec set (the earlier file
//   returned when either test failed).
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

typedef void *HDC;
typedef void *HWND;

extern HDC __stdcall GetDC(HWND wnd);
extern long __stdcall ReleaseDC(HWND wnd, HDC dc);
extern long __stdcall SetDeviceGammaRamp(HDC dc, void *ramp);
extern double log(double x);                    // inline fldln2 / fyl2x
extern double pow(double base, double exponent); // 0x6283c0 CRT _CIpow, x87 operands

extern uint8_t rasterizer_gamma_disabled;    // 0x0071d1e8
extern int32_t rasterizer_gamma_captured;    // 0x0071d1ec
extern int32_t rasterizer_gamma_exponent;    // 0x0071d1e0
extern d3d_gamma_ramp rasterizer_game_gamma_ramp; // 0x006e1118
extern uint8_t rasterizer_gamma_high_bit_17; // 0x006e1718
extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern void *rasterizer_device;              // 0x0071d174
extern HWND rasterizer_window_handle;        // 0x007461c8

typedef int32_t (__stdcall *d3d_set_gamma_ramp_fn)(void *self, uint32_t swap_chain, uint32_t flags, const void *ramp);

void chimera__gamma(void)
{
    uint32_t i;
    int16_t ramp_value;
    float exponent;
    void **vtable;
    d3d_set_gamma_ramp_fn set_gamma_ramp;
    HDC dc;

    if (rasterizer_gamma_disabled != 0 && rasterizer_gamma_captured != 0) {
        return;
    }

    exponent = (float)(log((double)rasterizer_gamma_exponent * 0.003921568859368563) / log(0.5));
    for (i = 0; i < 0x100; i++) {
        ramp_value = (int16_t)(int32_t)(pow((double)i * 0.003921568859368563, exponent) * 65535.0);
        rasterizer_game_gamma_ramp.red[i] = ramp_value;
        rasterizer_game_gamma_ramp.green[i] = ramp_value;
        rasterizer_game_gamma_ramp.blue[i] = ramp_value;
    }

    if (rasterizer_gamma_high_bit_17 == 1 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        vtable = *(void ***)rasterizer_device;
        set_gamma_ramp = (d3d_set_gamma_ramp_fn)vtable[0x15]; // +0x54
        set_gamma_ramp(rasterizer_device, 0, 0, &rasterizer_game_gamma_ramp);
        return;
    }

    dc = GetDC(rasterizer_window_handle);
    SetDeviceGammaRamp(dc, &rasterizer_game_gamma_ramp);
    ReleaseDC(rasterizer_window_handle, dc);
}

#if 0
Original Ghidra decompilation (0x5227a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void chimera__gamma(void)

{
  undefined2 uVar1;
  HDC hdc;
  uint uVar2;

  if ((DAT_0071d1e8 == '\0') || (DAT_0071d1ec == 0)) {
    uVar2 = 0;
    log2((float10)_DAT_0071d1e0 * (float10)0.003921569);
    log2((float10)0.5);
    do {
      FUN_006283c0();
      uVar1 = __ftol();
      (&DAT_006e1518)[uVar2] = uVar1;
      (&DAT_006e1318)[uVar2] = uVar1;
      (&DAT_006e1118)[uVar2] = uVar1;
      uVar2 = uVar2 + 1;
    } while (uVar2 < 0x100);
    if (((DAT_006e1718 == '\x01') && (DAT_0071d16c != '\0')) && (DAT_0071d174 != (int *)0x0)) {
      (**(code **)(*DAT_0071d174 + 0x54))(DAT_0071d174,0,0,&DAT_006e1118);
      return;
    }
    hdc = GetDC(DAT_007461c8);
    SetDeviceGammaRamp(hdc,&DAT_006e1118);
    ReleaseDC(DAT_007461c8,hdc);
  }
  return;
}
#endif
