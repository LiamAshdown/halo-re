// player_profile_apply_video_options  (Ghidra: FUN_00495580, unnamed)
// address 0x495580, size 584 bytes
// name confidence: 0.4   rewrite confidence: 0.85
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
// REWRITTEN (first-boot track, objdump 0x495580..0x4957c7): the old version called the display-mode check with
//   no mode, dropped the desktop-size fallback, wrote 32-bit values over the 16-bit globals at 0x00689450 and
//   0x0068944c (clobbering the present mode word next to them) and read the tick flag as a byte. Now:
//   - the mode compared is built from the settings: width +0xa68, height +0xa6a, refresh +0xa6c (int16 each),
//     vsync = +0xa6f != 0; when no device exists yet and it does not fit the desktop (unsigned compares against
//     GetWindowRect(GetDesktopWindow())), 800x600 is used if the desktop is taller than 600, else 640x480
//   - a differing mode (and no reset already pending) builds present parameters from it, resets the device, reads
//     the display mode back (device vtable +0x20, GetDisplayMode(0, 0x007c11f0)) and resizes the window; the
//     result is always 1 (the local only ever holds 0 or 1 and the return is `>= 0`)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "interface.h"

extern int32_t profile_write_back_enabled;   // 0x007196f4
extern int16_t rasterizer_present_mode;      // 0x0068944e
extern int16_t unknown_00689450;             // 0x00689450, always set to 2 here
extern int32_t game_time_force_single_tick;  // 0x007196d8
extern uint8_t unknown_006894ba;             // 0x006894ba
extern uint8_t rasterizer_device_valid;      // 0x0071d16c
extern void *rasterizer_device;              // 0x0071d174
extern uint8_t rasterizer_needs_reset;       // 0x0071d16d
extern uint8_t present_parameters_flags[];   // 0x007c11f0, the D3DDISPLAYMODE GetDisplayMode fills
extern uint32_t video_triple_buffer_unsupported; // 0x00722b6c
extern uint8_t unknown_006893f7;             // 0x006893f7
extern uint8_t unknown_006893f6;             // 0x006893f6
extern uint8_t unknown_006893fa;             // 0x006893fa
extern uint32_t rasterizer_device_version;   // 0x007c118c
extern uint8_t unknown_006893f2;             // 0x006893f2
extern uint32_t rasterizer_capability_007c10e4; // 0x007c10e4
extern int16_t light_count_enabled;          // 0x0068944c, always set to 2 here
extern uint8_t unknown_006893ff;             // 0x006893ff
extern uint8_t unknown_00689404;             // 0x00689404
extern uint8_t decals_for_all_responses;     // 0x006893f5
extern uint8_t particle_spawn_debug_mode;    // 0x0069c565
extern uint8_t particle_systems_enabled;     // 0x0069c566
extern int32_t rasterizer_gamma;             // 0x0071d1e0
extern struct cache *texture_cache;          // 0x006ac540

extern void *GetDesktopWindow(void);
extern int32_t GetWindowRect(void *window, win32_rect *rect);
extern uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested); // 0x515d10, blam-cc: EDI -> requested
extern void rasterizer_build_present_parameters(void *dest, rasterizer_display_mode *source); // 0x515fc0,
    // blam-cc: EAX -> source, stack -> dest
extern uint8_t rasterizer_device_reset(void *present_parameters); // 0x515d90, stack -> present_parameters
extern void rasterizer_resize_game_window(int32_t height, int32_t width); // 0x515b20, blam-cc: EAX -> height, ECX -> width
extern void chimera__gamma(void); // 0x5227a0
extern void cache_flush(struct cache *self); // 0x4d17f0, blam-cc: ESI -> self

typedef int32_t (__stdcall *d3d_get_display_mode_fn)(void *device, uint32_t swap_chain, void *mode);

// blam-cc: EAX -> settings
// Applies the profile's video options (settings is the profile's settings block): the present mode from +0xa74,
// the display mode from +0xa68..+0xa6f (resetting the device when it differs), and the detail toggles and gamma
// from +0xa70..+0xa76. Flushes the texture cache when the present mode changed.
uint8_t player_profile_apply_video_options(uint8_t *settings)
{
    rasterizer_display_mode mode;
    win32_rect desktop;
    uint8_t present_parameters[0x38];
    uint8_t mode_changed = 0;
    int32_t reset = 0;
    int16_t new_mode;
    uint8_t value;

    if (profile_write_back_enabled != 0) {
        settings[0xa70] = 0;
        settings[0xa71] = 0;
        settings[0xa72] = 0;
        settings[0xa74] = 0;
        settings[0xa73] = 0;
    }
    switch (settings[0xa74]) {
    case 0: new_mode = 2; break;
    case 1: new_mode = 1; break;
    case 2: new_mode = 0; break;
    default: new_mode = -1; break;
    }
    if (new_mode >= 0) {
        mode_changed = rasterizer_present_mode != new_mode;
        rasterizer_present_mode = new_mode;
    }
    unknown_00689450 = 2;

    mode.width = *(int16_t *)(settings + 0xa68);
    mode.height = *(int16_t *)(settings + 0xa6a);
    mode.refresh_rate = *(int16_t *)(settings + 0xa6c);
    mode.vsync = settings[0xa6f] != 0;
    unknown_006894ba = game_time_force_single_tick != 0 ? 0 : settings[0xa6f] == 2;

    if (rasterizer_device_valid == 0 || rasterizer_device == 0) {
        GetWindowRect(GetDesktopWindow(), &desktop);
        if ((uint32_t)mode.height >= (uint32_t)desktop.bottom || (uint32_t)mode.width >= (uint32_t)desktop.right) {
            if (desktop.bottom > 600) {
                mode.width = 800;
                mode.height = 600;
            } else {
                mode.width = 640;
                mode.height = 480;
            }
        }
    }
    if (rasterizer_needs_reset == 0 && rasterizer_display_mode_differs(&mode)) {
        void **vtable;

        rasterizer_build_present_parameters(present_parameters, &mode);
        rasterizer_device_reset(present_parameters);
        vtable = *(void ***)rasterizer_device;
        ((d3d_get_display_mode_fn)vtable[0x20 / 4])(rasterizer_device, 0, present_parameters_flags);
        reset = 1;
        rasterizer_resize_game_window(mode.height, mode.width);
        rasterizer_needs_reset = 0;
    }

    value = video_triple_buffer_unsupported != 0 ? 0 : settings[0xa70];
    unknown_006893f7 = value;
    unknown_006893f6 = value;
    unknown_006893fa = value;
    unknown_006893f2 = rasterizer_device_version < 0xffff0101u ? 0 : settings[0xa71];
    light_count_enabled = 2;
    unknown_006893ff = 1;
    unknown_00689404 = 1;
    decals_for_all_responses = (rasterizer_capability_007c10e4 & 0x6000000u) != 0 ? settings[0xa72] : 0;
    particle_spawn_debug_mode = settings[0xa73];
    particle_systems_enabled = settings[0xa73];
    rasterizer_gamma = settings[0xa76];
    chimera__gamma();

    if (mode_changed) {
        texture_cache->age = texture_cache->age + 1;
        cache_flush(texture_cache);
    }
    return reset >= 0;
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
