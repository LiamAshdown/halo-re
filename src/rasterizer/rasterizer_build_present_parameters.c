// rasterizer_build_present_parameters  (Ghidra: rasterizer_build_present_parameters, already
// named)
// address 0x515fc0, size 261 bytes
// name confidence: 0.55  rewrite confidence: 0.55
// evidence: when the optional display-mode source (in_EAX) is NULL, just copies the cached
//   rasterizer_present_parameters (0x007c04a0, 14 dwords) out; otherwise builds a fresh
//   d3d_present_parameters from it (format 0x16 X8R8G8B8, depth format 0x4b D24S8, hwnd
//   shell_window), matching every field this session's type header pins.
// register convention: destination d3d_present_parameters* is the recognized parameter, source
//   rasterizer_display_mode* (optional) in in_EAX. // blam-cc: EAX -> source(opt),
//   stack -> dest
// UNSURE: the exact boolean sense of rasterizer_fullscreen / video_force_mode_flag / the three debug
//   toggles that feed `flags` -- preserved as literal reads/assignments rather than renamed for
//   an assumed sense.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint32_t config_disable_buffering; // 0x00722b54 UNSURE: debug/flags toggle
extern uint8_t unknown_0071d18d;  // 0x0071d18d UNSURE: debug/flags toggle
extern uint32_t screenshots; // 0x007196e0 UNSURE: debug/flags toggle
extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern void *shell_window;       // 0x007461c4
extern int32_t os_platform;         // 0x00721ef0
extern uint8_t video_force_mode_flag;    // 0x0071d170 UNSURE (also read by rasterizer_display_mode_differs.c)
extern uint32_t game_time_force_single_tick;   // 0x007196d8 UNSURE: vsync override toggle
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0

extern void os_platform_identify(void); // 0x5427e0

// blam-cc: EAX -> source(opt), stack -> dest
// Reads or constructs a D3D present-parameters block. With no source display mode, copies the
// currently cached parameters out; with one, builds fresh parameters from its width/height/
// refresh_rate/vsync.
void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source)
{
    uint32_t *raw_dest = (uint32_t *)dest;
    int i;

    if (source == (rasterizer_display_mode *)0) {
        uint32_t *raw_src = (uint32_t *)&rasterizer_present_parameters;
        for (i = 0; i < 0xe; i++) {
            raw_dest[i] = raw_src[i];
        }
        return;
    }

    for (i = 0; i < 0xe; i++) {
        raw_dest[i] = 0;
    }

    dest->flags = (config_disable_buffering == 0 && unknown_0071d18d == 0 && screenshots == 0) ? 0 : 1;
    dest->enable_auto_depth_stencil = 1;
    dest->swap_effect = (rasterizer_fullscreen == 0) ? 3 : 1;
    dest->back_buffer_width = (uint32_t)source->width;
    dest->back_buffer_height = (uint32_t)source->height;
    dest->back_buffer_format = 0x16;
    dest->back_buffer_count = 1;
    dest->auto_depth_stencil_format = 0x4b;
    dest->device_window = (uint32_t)shell_window;

    if (rasterizer_fullscreen == 0) {
        dest->windowed = 1;
        dest->fullscreen_refresh_rate = 0;
    } else {
        if (os_platform == 0) {
            os_platform_identify();
        }
        dest->windowed = 0;
        if (video_force_mode_flag == 0 && source->refresh_rate != 0 && os_platform > 2) {
            dest->fullscreen_refresh_rate = (uint32_t)source->refresh_rate;
        } else {
            dest->fullscreen_refresh_rate = 0;
        }
    }

    if (source->vsync != 0 && game_time_force_single_tick == 0) {
        dest->presentation_interval = 1;
    } else {
        dest->presentation_interval = 0x80000000u;
    }
}

#if 0
Original Ghidra decompilation (0x515fc0):

void rasterizer_build_present_parameters(undefined4 *param_1)

{
  char cVar1;
  undefined4 *in_EAX;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  bool bVar5;
  bool bVar6;

  iVar3 = 0xe;
  puVar4 = param_1;
  if (in_EAX == (undefined4 *)0x0) {
    puVar4 = &DAT_007c04a0;
    for (; iVar3 != 0; iVar3 = iVar3 + -1) {
      *param_1 = *puVar4;
      puVar4 = puVar4 + 1;
      param_1 = param_1 + 1;
    }
    return;
  }
  for (; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  if (((DAT_00722b54 == 0) && (DAT_0071d18d == '\0')) && (DAT_007196e0 == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  param_1[0xb] = uVar2;
  bVar5 = DAT_0071d16c == '\0';
  bVar6 = DAT_0071d16c == '\0';
  param_1[9] = 1;
  param_1[6] = (uint)bVar5 * 2 + 1;
  *param_1 = *in_EAX;
  param_1[1] = in_EAX[1];
  param_1[2] = 0x16;
  param_1[3] = 1;
  param_1[10] = 0x4b;
  param_1[7] = DAT_007461c4;
  if (bVar6) {
    param_1[8] = 1;
    param_1[0xc] = 0;
    cVar1 = *(char *)(in_EAX + 3);
  }
  else {
    if (DAT_00721ef0 == 0) {
      os_platform_identify();
    }
    bVar5 = DAT_0071d170 == '\0';
    param_1[8] = 0;
    if (((bVar5) && (in_EAX[2] != 0)) && (2 < DAT_00721ef0)) {
      param_1[0xc] = in_EAX[2];
    }
    else {
      param_1[0xc] = 0;
    }
    cVar1 = *(char *)(in_EAX + 3);
  }
  if ((cVar1 != '\0') && (DAT_007196d8 == 0)) {
    param_1[0xd] = 1;
    return;
  }
  param_1[0xd] = 0x80000000;
  return;
}
#endif
