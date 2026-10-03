// chimera__registry_check_3  (Ghidra: already named; Chimera name, hint only)
// address 0x5226c0, size 210 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: out/phase4/rasterizer_types_notes.md ("Restores the display gamma ramp saved earlier
// and resets the gamma registry marker, mirroring chimera__registry_check_4's setup"). Clean
// Win32/D3D9 usage; vtable+0x54 (index 21) is IDirect3DDevice9::SetGammaRamp(iSwapChain, Flags,
// pRamp), confirmed by the 3-argument call shape.
// register convention: no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"



#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t rasterizer_gamma_disabled;             // 0x0071d1e8
extern int32_t rasterizer_gamma_captured;             // 0x0071d1ec
extern uint8_t rasterizer_gamma_high_bit_17;          // 0x006e1718
extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern void *rasterizer_device;                       // 0x0071d174
extern d3d_gamma_ramp rasterizer_desktop_gamma_ramp;  // 0x006e0b18
extern HWND rasterizer_window_handle;                 // 0x007461c8

typedef int32_t (__stdcall *d3d_set_gamma_ramp_fn)(void *self, uint32_t swap_chain, uint32_t flags, const void *ramp);

#define k_HKEY_CURRENT_USER ((HKEY)0x80000001)

void chimera__registry_check_3(void)
{
    HDC dc;
    uint8_t zero_value[4] = {0, 0, 0, 0};
    HKEY key;
    void **vtable;
    d3d_set_gamma_ramp_fn set_gamma_ramp;

    if (rasterizer_gamma_disabled == 0 && rasterizer_gamma_captured != 0) {
        if (rasterizer_gamma_high_bit_17 == 1 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
            vtable = *(void ***)rasterizer_device;
            set_gamma_ramp = (d3d_set_gamma_ramp_fn)vtable[0x15]; // +0x54
            set_gamma_ramp(rasterizer_device, 0, 0, &rasterizer_desktop_gamma_ramp);
            return;
        }

        dc = GetDC(rasterizer_window_handle);
        SetDeviceGammaRamp(dc, &rasterizer_desktop_gamma_ramp);
        ReleaseDC(rasterizer_window_handle, dc);

        RegCreateKeyExA(k_HKEY_CURRENT_USER, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                         (LPSTR)0, 0, 0x20006, (LPSECURITY_ATTRIBUTES)0, (PHKEY)&key, (LPDWORD)0);
        RegSetValueExA(key, "gamma", 0, 4, zero_value, 4);
        RegCloseKey(key);
    }
}

#if 0
Original Ghidra decompilation (0x5226c0):

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void chimera__registry_check_3(void)

{
  HDC hdc;
  BYTE local_8 [4];
  HKEY local_4;

  local_8[0] = '\0';
  local_8[1] = '\0';
  local_8[2] = '\0';
  local_8[3] = '\0';
  if ((DAT_0071d1e8 == '\0') && (DAT_0071d1ec != 0)) {
    if ((DAT_006e1718 == '\x01') && ((DAT_0071d16c != '\0' && (DAT_0071d174 != (int *)0x0)))) {
      (**(code **)(*DAT_0071d174 + 0x54))(DAT_0071d174,0,0,&DAT_006e0b18);
      return;
    }
    hdc = GetDC(DAT_007461c8);
    SetDeviceGammaRamp(hdc,&DAT_006e0b18);
    ReleaseDC(DAT_007461c8,hdc);
    local_8[0] = '\0';
    local_8[1] = '\0';
    local_8[2] = '\0';
    local_8[3] = '\0';
    RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                    0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_4,(LPDWORD)0x0);
    RegSetValueExA(local_4,"gamma",0,4,local_8,4);
    RegCloseKey(local_4);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
