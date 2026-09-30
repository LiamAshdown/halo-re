// video_display_modes_enumerate  (Ghidra: FUN_004baba0)
// address 0x4baba0, size 414 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4baba0..0x4bad3d in the phase-4 review. The first
// rewrite treated the enumerator as a get/count pair re-queried every pass, lost the EBX
// format argument, inverted the video memory filter and read the desktop rectangle from the
// wrong fields.
//   0x0071d178 is the IDirect3D9 interface: vtable +0x18 is GetAdapterModeCount(this,
// adapter, format) and +0x1c EnumAdapterModes(this, adapter, format, mode, D3DDISPLAYMODE*),
// adapter 0x0071d180, format in EBX (video_resolution_list_build passes 0x16,
// D3DFMT_X8R8G8B8). The desktop window rectangle is read once. Modes are walked from the
// last index down to 0; a failed EnumAdapterModes skips the mode. Filters, in order:
//   - when the maximum width (0x0069fe3c) is its default 0x1000, or the command line has
//     -vidmode (0x0066b174, compared with _stricmp 0x628d8b on arguments starting with -),
//     a width above 0x400 / 0x500 / 0x640 is dropped with at most 32 / 64 / 128 MB of video
//     memory (0x00722bb0);
//   - unless the byte 0x0071d16c is set and a device exists (0x0071d174), the mode must be
//     strictly smaller than the desktop (right, bottom of the window rectangle);
//   - width at most the maximum width, 640..4800 wide, 480..3600 high, refresh at most 120;
//   - 720x576, 720x480, 848x480 and 848x576 are dropped.
// Survivors go to video_resolution_add (EAX height; width, refresh rate on the stack).
// register convention: EBX the D3DFORMAT; no stack arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern d3d9_interface *rasterizer_direct3d;       // 0x0071d178
extern uint32_t d3d_adapter;                // 0x0071d180, UNSURE name
extern uint32_t config_maximum_resolution;            // 0x0069fe3c, 0x1000 by default; UNSURE name
extern int32_t shell_argc;           // 0x00721e94
extern char **shell_argv;            // 0x00721e90
extern uint32_t video_memory;         // 0x00722bb0, UNSURE name
extern uint8_t rasterizer_fullscreen; // 0x0071d16c, UNSURE name
extern void *rasterizer_device;                    // 0x0071d174

extern int _stricmp(const char *a, const char *b); // 0x628d8b


static uint8_t video_mode_memory_limit_applies(void)
{
    int32_t i;

    if (config_maximum_resolution == 0x1000) {
        return 1;
    }
    for (i = 0; i < shell_argc; i++) {
        const char *argument = shell_argv[i];

        if (argument[0] == '-' && _stricmp("-vidmode", argument) == 0) {
            return 1;
        }
    }
    return 0;
}

// blam-cc: EBX -> format
void video_display_modes_enumerate(uint32_t format)
{
    win32_rect desktop;
    d3d_display_mode mode;
    uint32_t index;

    if (rasterizer_direct3d == 0) {
        return;
    }
    GetWindowRect(GetDesktopWindow(), &desktop);
    index = rasterizer_direct3d->vtable->get_adapter_mode_count(rasterizer_direct3d, d3d_adapter, format);
    while (index != 0) {
        index--;
        if (rasterizer_direct3d->vtable->enum_adapter_modes(rasterizer_direct3d, d3d_adapter, format, index, &mode) < 0) {
            continue;
        }
        if (video_mode_memory_limit_applies()) {
            if (video_memory <= 0x2000000 && mode.width > 0x400) {
                continue;
            }
            if (video_memory <= 0x4000000 && mode.width > 0x500) {
                continue;
            }
            if (video_memory <= 0x8000000 && mode.width > 0x640) {
                continue;
            }
        }
        if (rasterizer_fullscreen == 0 || rasterizer_device == 0) {
            if (mode.width >= (uint32_t)desktop.right || mode.height >= (uint32_t)desktop.bottom) {
                continue;
            }
        }
        if (mode.width > config_maximum_resolution || mode.width < 0x280 || mode.height < 0x1e0 || mode.width > 0x12c0 ||
            mode.height > 0xe10 || mode.refresh_rate > 0x78) {
            continue;
        }
        if ((mode.width == 0x2d0 || mode.width == 0x350) && (mode.height == 0x240 || mode.height == 0x1e0)) {
            continue;
        }
        video_resolution_add((int32_t)mode.height, (int32_t)mode.width, (int32_t)mode.refresh_rate);
    }
}

#if 0
Original Ghidra decompilation (0x4baba0):

void video_display_modes_enumerate(void)

{
  char *_Str2;
  HWND hWnd;
  int iVar1;
  int iVar2;
  int iVar3;
  uint unaff_EBX;
  uint unaff_EDI;
  bool bVar4;
  uint uVar5;
  tagRECT *lpRect;
  uint uStack_14;
  tagRECT local_10;

  if (DAT_0071d178 != (int *)0x0) {
    lpRect = &local_10;
    hWnd = GetDesktopWindow();
    GetWindowRect(hWnd,lpRect);
    uVar5 = DAT_0071d180;
    iVar1 = (**(code **)(*DAT_0071d178 + 0x18))(DAT_0071d178);
joined_r0x004babda:
    if (iVar1 != 0) {
      iVar1 = iVar1 + -1;
      iVar2 = (**(code **)(*DAT_0071d178 + 0x1c))(DAT_0071d178,DAT_0071d180);
      if (-1 < iVar2) {
        if (DAT_0069fe3c == 0x1000) {
LAB_004bacd1:
          if (((DAT_00722bb0 < 0x2000001) && (0x400 < uVar5)) ||
             (((DAT_00722bb0 < 0x4000001 && (0x500 < uVar5)) ||
              ((DAT_00722bb0 < 0x8000001 && (0x640 < uVar5)))))) goto joined_r0x004babda;
        }
        else {
          iVar2 = 0;
          if (0 < DAT_00721e94) {
            do {
              _Str2 = *(char **)(DAT_00721e90 + iVar2 * 4);
              if ((*_Str2 == '-') && (iVar3 = __stricmp("-vidmode",_Str2), iVar3 == 0))
              goto LAB_004bacd1;
              iVar2 = iVar2 + 1;
            } while (iVar2 < DAT_00721e94);
          }
        }
        if ((((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) ||
            ((uVar5 < uStack_14 && (unaff_EBX < (uint)local_10.left)))) &&
           (((((uVar5 <= DAT_0069fe3c && (0x27f < uVar5)) && (0x1df < unaff_EBX)) &&
             ((uVar5 < 0x12c1 && (unaff_EBX < 0xe11)))) && (unaff_EDI < 0x79)))) {
          if (uVar5 == 0x2d0) {
            if (unaff_EBX == 0x240) goto joined_r0x004babda;
            bVar4 = unaff_EBX == 0x1e0;
LAB_004bad24:
            if (bVar4) goto joined_r0x004babda;
          }
          else if (uVar5 == 0x350) {
            if (unaff_EBX != 0x1e0) {
              bVar4 = unaff_EBX == 0x240;
              goto LAB_004bad24;
            }
            goto joined_r0x004babda;
          }
          video_resolution_add(uVar5,unaff_EDI);
        }
      }
      goto joined_r0x004babda;
    }
  }
  return;
}
#endif
