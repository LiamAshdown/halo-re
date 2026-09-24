// player_profile_apply_video_options  (Ghidra: FUN_00495580, unnamed)
// address 0x495580, size 584 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/interface_functions.md "Applies a set of video/display options
// (vsync-like mode, gamma, misc toggles) from the current profile."; src/cache/cache_file_unload.c
// and src/cache/texture_cache_new.c's confirmed texture_cache (0x006ac540, cache::age at +0x30);
// src/cache/cache_file_unload.c's cache_flush(cache *self) [ESI] convention.
// register convention: profile settings block in EAX (in_EAX). // blam-cc: EAX -> settings
// UNSURE: the rasterizer device-reset path (rasterizer_display_mode_differs, rasterizer_build_present_parameters,
// rasterizer_device_reset, the vtable call at *DAT_0071d174+0x20, rasterizer_resize_game_window)
// and the great majority of the DAT_0068... / DAT_0069c5.. / DAT_007c1... globals belong to the
// rasterizer module, well outside this batch; every name/signature below is inferred solely from
// this call site. The settings offsets (0xa6f..0xa76) are read from a caller-owned block whose
// true type this batch does not recover (see interface_types_notes.md's player_control_settings
// entry); kept as raw byte offsets into an opaque buffer.
// UNSURE: the return value's upper 3 bytes are `extraout_EAX`-derived decompiler noise from a
// register that survives an untaken/taken call path; only the low byte (a success flag, always 1
// on every path Ghidra shows: local_5c is never negative) is semantically real, so this is
// rewritten to return a plain int32_t 1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t profile_write_back_enabled; // 0x007196f4 (per player_profile_subsystem_initialize.c)
extern int16_t rasterizer_present_mode;    // 0x0068944e, UNSURE: vsync/present mode cache
extern int32_t unknown_00689450;           // 0x00689450, UNSURE: always forced to 2
extern uint8_t unknown_007196d8;           // 0x007196d8, UNSURE
extern uint8_t unknown_006894ba;           // 0x006894ba, UNSURE
extern uint8_t rasterizer_device_valid; // 0x0071d16c, UNSURE
extern void **rasterizer_device;           // 0x0071d174, UNSURE: device object, vtable slot +0x20
extern uint8_t rasterizer_needs_reset;     // 0x0071d16d, UNSURE
extern uint32_t present_parameters_flags;  // 0x007c11f0, UNSURE
extern uint32_t video_triple_buffer_unsupported; // 0x00722b6c, UNSURE
extern uint8_t unknown_006893f6;           // 0x006893f6, UNSURE
extern uint32_t rasterizer_device_version; // 0x007c118c, UNSURE
extern uint8_t unknown_006893f2;           // 0x006893f2, UNSURE
extern int32_t unknown_0068944c;           // 0x0068944c, UNSURE: always forced to 2
extern uint8_t unknown_006893ff;           // 0x006893ff, UNSURE: always forced to 1
extern uint8_t unknown_00689404;           // 0x00689404, UNSURE: always forced to 1
extern uint32_t rasterizer_capability_007c10e4; // 0x007c10e4, UNSURE
extern uint8_t unknown_006893f5;           // 0x006893f5, UNSURE
extern int32_t rasterizer_gamma; // 0x0071d1e0, UNSURE
extern uint8_t unknown_0069c565;           // 0x0069c565, UNSURE
extern uint8_t unknown_006893f7;           // 0x006893f7, UNSURE
extern uint8_t unknown_006893fa;           // 0x006893fa, UNSURE
extern uint8_t unknown_0069c566;           // 0x0069c566, UNSURE
extern struct cache *texture_cache;        // 0x006ac540

extern void *GetDesktopWindow(void);
extern int32_t GetWindowRect(void *window, win32_rect *rect);
extern uint8_t rasterizer_display_mode_differs(void);                              // 0x515d10, UNSURE
extern void rasterizer_build_present_parameters(void *params);  // 0x515fc0, UNSURE
extern void rasterizer_device_reset(void *params);               // 0x515d90, UNSURE
extern void rasterizer_resize_game_window(void);                 // 0x515b20
extern void chimera__gamma(void);                                // 0x5227a0
extern void cache_flush(struct cache *self); // 0x4d17f0, blam-cc: ESI -> self

// blam-cc: EAX -> settings
// Applies the profile's video/display options: the present/vsync mode (settings+0xa74, a
// tri-state that also detects whether the mode actually changed), a windowed/fullscreen-adjacent
// flag (settings+0xa6f), and a handful of capability-gated toggles and the gamma byte
// (settings+0xa70..0xa76) copied into the rasterizer's own globals. Resets the rasterizer device
// first if one is pending reset, and bumps/flushes the texture cache when the present mode
// changed.
int32_t player_profile_apply_video_options(uint8_t *settings)
{
    win32_rect desktop_rect;
    uint8_t present_parameters[56];
    uint8_t mode_changed;
    int16_t new_mode;

    mode_changed = 0;
    if (profile_write_back_enabled != 0) {
        settings[0xa70] = 0;
        settings[0xa71] = 0;
        settings[0xa72] = 0;
        settings[0xa74] = 0;
        settings[0xa73] = 0;
    }

    if (settings[0xa74] == 0) {
        new_mode = 2;
        mode_changed = (rasterizer_present_mode == new_mode);
        rasterizer_present_mode = new_mode;
        mode_changed = !mode_changed;
    } else if (settings[0xa74] == 1) {
        new_mode = 1;
        mode_changed = (rasterizer_present_mode == new_mode);
        rasterizer_present_mode = new_mode;
        mode_changed = !mode_changed;
    } else if (settings[0xa74] == 2) {
        mode_changed = (rasterizer_present_mode == 0);
        rasterizer_present_mode = 0;
        mode_changed = !mode_changed;
    }

    unknown_00689450 = 2;
    unknown_006894ba = (unknown_007196d8 == 0) && (settings[0xa6f] == 2);

    if (rasterizer_device_valid == 0 || rasterizer_device == (void **)0) {
        GetWindowRect(GetDesktopWindow(), &desktop_rect);
    }

    if (rasterizer_needs_reset == 0 && rasterizer_display_mode_differs() != 0) {
        rasterizer_build_present_parameters(present_parameters);
        rasterizer_device_reset(present_parameters);
        (*(void (**)(void **, int32_t, uint32_t *))((uint8_t *)*rasterizer_device + 0x20))(
            rasterizer_device, 0, &present_parameters_flags);
        rasterizer_resize_game_window();
        rasterizer_needs_reset = 0;
    }

    unknown_006893f6 = (video_triple_buffer_unsupported == 0) ? settings[0xa70] : 0;
    unknown_006893f2 = (rasterizer_device_version < 0xffff0101u) ? 0 : settings[0xa71];
    unknown_0068944c = 2;
    unknown_006893ff = 1;
    unknown_00689404 = 1;
    unknown_006893f5 = ((rasterizer_capability_007c10e4 & 0x6000000u) == 0) ? 0 : settings[0xa72];
    rasterizer_gamma = settings[0xa76];
    unknown_0069c565 = settings[0xa73];
    unknown_006893f7 = unknown_006893f6;
    unknown_006893fa = unknown_006893f6;
    unknown_0069c566 = unknown_0069c565;
    chimera__gamma();

    if (mode_changed) {
        texture_cache->age = texture_cache->age + 1;
        cache_flush(texture_cache);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x495580):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00495580(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  HWND hWnd;
  undefined4 extraout_EAX;
  undefined3 uVar3;
  undefined3 extraout_var;
  bool bVar4;
  tagRECT *lpRect;
  int local_5c;
  tagRECT local_48;
  undefined1 local_38 [56];

  bVar4 = false;
  if (DAT_007196f4 != 0) {
    *(undefined1 *)(in_EAX + 0xa70) = 0;
    *(undefined1 *)(in_EAX + 0xa71) = 0;
    *(undefined1 *)(in_EAX + 0xa72) = 0;
    *(undefined1 *)(in_EAX + 0xa74) = 0;
    *(undefined1 *)(in_EAX + 0xa73) = 0;
  }
  cVar1 = *(char *)(in_EAX + 0xa74);
  if (cVar1 == '\0') {
    sVar2 = 2;
LAB_004955e9:
    bVar4 = DAT_0068944e == sVar2;
    DAT_0068944e = sVar2;
  }
  else {
    if (cVar1 == '\x01') {
      sVar2 = 1;
      goto LAB_004955e9;
    }
    if (cVar1 != '\x02') goto LAB_004955fb;
    bVar4 = DAT_0068944e == 0;
    DAT_0068944e = 0;
  }
  bVar4 = !bVar4;
LAB_004955fb:
  DAT_00689450 = 2;
  DAT_006894ba = DAT_007196d8 == 0 && *(char *)(in_EAX + 0xa6f) == '\x02';
  if ((DAT_0071d16c == '\0') || (DAT_0071d174 == (int *)0x0)) {
    lpRect = &local_48;
    hWnd = GetDesktopWindow();
    GetWindowRect(hWnd,lpRect);
  }
  local_5c = 0;
  if ((DAT_0071d16d == '\0') && (cVar1 = FUN_00515d10(), cVar1 != '\0')) {
    rasterizer_build_present_parameters(local_38);
    rasterizer_device_reset(local_38);
    (**(code **)(*DAT_0071d174 + 0x20))(DAT_0071d174,0,&DAT_007c11f0);
    local_5c = 1;
    rasterizer_resize_game_window();
    DAT_0071d16d = '\0';
  }
  if (DAT_00722b6c == 0) {
    DAT_006893f6 = *(undefined1 *)(in_EAX + 0xa70);
  }
  else {
    DAT_006893f6 = 0;
  }
  if (DAT_007c118c < 0xffff0101) {
    DAT_006893f2 = 0;
  }
  else {
    DAT_006893f2 = *(undefined1 *)(in_EAX + 0xa71);
  }
  DAT_0068944c = 2;
  DAT_006893ff = 1;
  DAT_00689404 = 1;
  if ((_DAT_007c10e4 & 0x6000000) == 0) {
    DAT_006893f5 = 0;
  }
  else {
    DAT_006893f5 = *(undefined1 *)(in_EAX + 0xa72);
  }
  _DAT_0071d1e0 = (uint)*(byte *)(in_EAX + 0xa76);
  DAT_0069c565 = *(undefined1 *)(in_EAX + 0xa73);
  DAT_006893f7 = DAT_006893f6;
  DAT_006893fa = DAT_006893f6;
  DAT_0069c566 = DAT_0069c565;
  chimera__gamma();
  uVar3 = (undefined3)((uint)extraout_EAX >> 8);
  if (bVar4) {
    *(int *)(DAT_006ac540 + 0x30) = *(int *)(DAT_006ac540 + 0x30) + 1;
    cache_flush();
    uVar3 = extraout_var;
  }
  return CONCAT31(uVar3,-1 < local_5c);
}
#endif
