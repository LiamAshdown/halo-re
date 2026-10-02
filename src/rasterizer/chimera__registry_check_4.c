// chimera__registry_check_4  (Ghidra: already named; Chimera name, hint only)
// address 0x522520, size 399 bytes
// name confidence: 0.9   rewrite confidence: 0.8
// evidence: out/phase4/rasterizer_types_notes.md ("Checks the -nogamma command-line flag and the
// registry gamma setting, captures the current display gamma ramp (or a linear default), and
// persists the gamma registry flag"). Clean Win32 API usage (RegOpenKeyExA/RegQueryValueExA/
// RegCloseKey/GetDC/GetDeviceGammaRamp/ReleaseDC/RegCreateKeyExA/RegSetValueExA), no register
// ambiguity. `DAT_00721e90`/`DAT_00721e94` are the command-line argv/argc globals (used the same
// way `rasterizer_parse_vidmode_commandline` reads them, per types/cache.h); `DAT_007c10cc` is
// d3d_caps9.dev_caps (bit 0x11 tested, matching the header's note that its high word is used).
// register convention: no parameters.
// UNSURE: DAT_007196f4 (a second override flag alongside -nogamma) has no documented owner.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0


extern int __cdecl _stricmp(const char *a, const char *b);

extern int32_t shell_argc;                                   // 0x00721e94
extern char **shell_argv;                                    // 0x00721e90
extern int32_t safe_mode;                                    // 0x007196f4 UNSURE: safe mode switch
extern int32_t config_safe_mode;                             // 0x00722b60 nonzero: one vertex stream, fixed function path
extern uint8_t rasterizer_gamma_disabled;    // 0x0071d1e8
extern uint8_t rasterizer_gamma_high_bit_17; // 0x006e1718, UNSURE owner; bit 0x11 of dev_caps
extern d3d_gamma_ramp rasterizer_desktop_gamma_ramp; // 0x006e0b18
extern HWND rasterizer_window_handle;        // 0x007461c8
extern int32_t rasterizer_gamma_captured;    // 0x0071d1ec

extern void rasterizer_gamma_brightness_to_exponent(rasterizer_gamma_settings *settings); // 0x522890, blam-cc: EAX

#define k_HKEY_CURRENT_USER ((HKEY)0x80000001)

void chimera__registry_check_4(void)
{
    int32_t i;
    HKEY key;
    DWORD value_size;
    int32_t gamma_flag;
    HDC dc;

    if (0 < shell_argc) {
        for (i = 0; i < shell_argc; i++) {
            char *arg = shell_argv[i];
            if (*arg == '-' && _stricmp("-nogamma", arg) == 0) {
                rasterizer_gamma_disabled = 1;
                goto set_bit;
            }
        }
    }
    if (safe_mode != 0) {
        rasterizer_gamma_disabled = 1;
        goto set_bit;
    }
    rasterizer_gamma_disabled = 0;
    if (config_safe_mode != 0) {
        rasterizer_gamma_disabled = 1;
    }

set_bit:
    rasterizer_gamma_high_bit_17 = (uint8_t)((rasterizer_caps.caps2 >> 0x11) & 1);

    gamma_flag = 0;
    value_size = 4;
    RegOpenKeyExA(k_HKEY_CURRENT_USER, "Software\\Microsoft\\Microsoft Games\\Halo", 0, 0x20019, (PHKEY)&key);
    RegQueryValueExA(key, "gamma", (LPDWORD)0, (LPDWORD)0, (LPBYTE)&gamma_flag, &value_size);
    RegCloseKey(key);

    if (gamma_flag == 0) {
        dc = GetDC(rasterizer_window_handle);
        GetDeviceGammaRamp(dc, &rasterizer_desktop_gamma_ramp);
        ReleaseDC(rasterizer_window_handle, dc);
    } else {
        int32_t j;
        for (j = 0; j < 0x100; j++) {
            uint16_t value = (uint16_t)(j << 8);
            rasterizer_desktop_gamma_ramp.red[j] = value;
            rasterizer_desktop_gamma_ramp.green[j] = value;
            rasterizer_desktop_gamma_ramp.blue[j] = value;
        }
    }

    // 0x522652: mov eax,0x6e0b18 -- the ramp just read or built; its red[128] (+0x100) sets the exponent
    rasterizer_gamma_brightness_to_exponent((rasterizer_gamma_settings *)&rasterizer_desktop_gamma_ramp);

    gamma_flag = 1;
    RegCreateKeyExA(k_HKEY_CURRENT_USER, "Software\\Microsoft\\Microsoft Games\\Halo", 0, (LPSTR)0,
                     0, 0x20006, (LPSECURITY_ATTRIBUTES)0, (PHKEY)&key, (LPDWORD)0);
    RegSetValueExA(key, "gamma", 0, 4, (const BYTE *)&gamma_flag, 4);
    RegCloseKey(key);

    rasterizer_gamma_captured = 1;
}

#if 0
Original Ghidra decompilation (0x522520):

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void chimera__registry_check_4(void)

{
  char *_Str2;
  int iVar1;
  HDC hdc;
  undefined2 uVar2;
  int iVar3;
  int local_c;
  HKEY local_8;
  DWORD local_4;

  iVar3 = 0;
  local_c = 0;
  local_4 = 4;
  if (0 < DAT_00721e94) {
    do {
      _Str2 = *(char **)(DAT_00721e90 + iVar3 * 4);
      if ((*_Str2 == '-') && (iVar1 = __stricmp("-nogamma",_Str2), iVar1 == 0)) goto LAB_00522582;
      iVar3 = iVar3 + 1;
    } while (iVar3 < DAT_00721e94);
  }
  if ((DAT_007196f4 != 0) || (DAT_0071d1e8 = 0, DAT_00722b60 != 0)) {
LAB_00522582:
    DAT_0071d1e8 = 1;
  }
  DAT_006e1718 = (byte)(DAT_007c10cc >> 0x11) & 1;
  local_c = 0;
  RegOpenKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,&local_8)
  ;
  RegQueryValueExA(local_8,"gamma",(LPDWORD)0x0,(LPDWORD)0x0,(LPBYTE)&local_c,&local_4);
  RegCloseKey(local_8);
  if (local_c == 0) {
    hdc = GetDC(DAT_007461c8);
    GetDeviceGammaRamp(hdc,&DAT_006e0b18);
    ReleaseDC(DAT_007461c8,hdc);
  }
  else {
    iVar3 = 0;
    do {
      uVar2 = (undefined2)(iVar3 << 8);
      (&DAT_006e0b18)[iVar3] = uVar2;
      (&DAT_006e0d18)[iVar3] = uVar2;
      (&DAT_006e0f18)[iVar3] = uVar2;
      iVar3 = iVar3 + 1;
    } while (iVar3 < 0x100);
  }
  FUN_00522890();
  local_c = 1;
  RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                  0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_8,(LPDWORD)0x0);
  RegSetValueExA(local_8,"gamma",0,4,(BYTE *)&local_c,4);
  RegCloseKey(local_8);
  DAT_0071d1ec = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
