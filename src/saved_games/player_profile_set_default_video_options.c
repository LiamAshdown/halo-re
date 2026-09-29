// player_profile_set_default_video_options  (Ghidra: player_profile_set_default_video_options, already named)
// address 0x53b000, size 563 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x53b000..0x53b232 fixes a
// real Ghidra decompiler error: every "extraout_CL" the pseudocode shows (0xa74/0xa73/0xa6f) is
// simply the literal 2 loaded into CL at function entry (0x53b010) and never touched again --
// Ghidra lost track of it across the rasterizer_decal_zbias_active call and invented a fake
// "extra output", not a real secondary return value. Field offsets/values otherwise match
// out/phase4/saved_games_types_notes.md's video block (0xa68..0xb78) note.
// register convention: __cdecl (Ghidra-recognized); profile and allow_display_query are the
// recognized stack parameters.
// Phase 4 review (objdump 0x53b109..0x53b1c1): rasterizer_parse_vidmode_commandline takes
// width_out in ESI plus height_out / refresh_out on the stack (the same prototype as
// src/rasterizer/rasterizer_initialize_direct3d.c); width_out reuses the dead profile argument
// slot. display_mode_get_current fills a rasterizer_display_mode (types/rasterizer.h) through
// EDI: the words at +0x0 / +0x4 / +0x8 of three int32 fields and the vsync byte at +0xc. The
// earlier rewrite modeled that block as four int16s, which read height from +0x2 (the high
// word of width) and the flag from +0x6; fixed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "rasterizer.h"
#include "saved_games.h"

extern int32_t rasterizer_gamma_exponent; // 0x0071d1e0, only its low byte is read here (mov al,ds:0x71d1e0)
extern uint32_t safe_mode; // 0x007196f4, read as a dword (src/math/math_initialize.c name); nonzero selects the low-end defaults
extern uint32_t rasterizer_device_version; // rasterizer capability dword
extern uint32_t cpu_speed; // machine class threshold (> 1000)
extern uint32_t physical_memory; // machine class threshold (> 0x80)
extern uint32_t video_memory; // machine class threshold (> 0x2000000)
extern uint32_t config_disable_specular; // UNSURE
extern uint8_t width640; // UNSURE: "no display query" flag
extern uint8_t unknown_006894ba; // UNSURE

extern uint8_t rasterizer_decal_zbias_active(void); // 0x5195d0, not in this module
extern uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out); // 0x5168c0, ESI width_out
extern void display_mode_get_current(rasterizer_display_mode *out); // 0x515ca0, EDI out

// Fills a profile's default video settings. On a low-end machine (any of four capability
// thresholds unmet), hardcodes 640x480x60 with minimal capability flags. On a capable machine:
// honors a "-vidmode" command-line override if present, else queries the current display mode
// (if allow_display_query) or falls back to 800x600x60.
uint8_t player_profile_set_default_video_options(saved_player_profile *profile, uint8_t allow_display_query)
{
    uint8_t gamma;
    int32_t override_width;
    int32_t override_height;
    long override_refresh;
    rasterizer_display_mode mode;

    gamma = (uint8_t)rasterizer_gamma_exponent;
    profile->unknown_a6e = 2;
    profile->unknown_a75 = 2;
    profile->gamma = (int8_t)gamma;
    if (gamma == 0) {
        gamma = 1;
    } else if ((int8_t)gamma == -1) {
        gamma = (uint8_t)-2;
    }
    profile->gamma = (int8_t)gamma;

    if (safe_mode != 0 || rasterizer_device_version < 0xffff0101 ||
        cpu_speed < 0x3e9 || physical_memory < 0x81 || video_memory < 0x2000001) {
        profile->specular_enabled = 0;
        profile->shadows_enabled = 0;
        profile->decals_enabled = 0;
        profile->particles_enabled = safe_mode == 0;
        profile->texture_quality = 1;
        profile->screen_width = 0x280;
        profile->screen_height = 0x1e0;
        profile->refresh_rate = 0x3c;
        profile->vsync_mode = 2;
        return 1;
    }

    profile->specular_enabled = config_disable_specular == 0;
    profile->shadows_enabled = 1;
    profile->decals_enabled = rasterizer_decal_zbias_active() != 0;
    profile->texture_quality = 2;
    profile->particles_enabled = 2;
    profile->vsync_mode = 2;

    if (width640 != 0) {
        profile->screen_width = 0x280;
        profile->screen_height = 0x1e0;
        profile->refresh_rate = 0x3c;
        return 1;
    }

    override_width = -1;
    override_height = -1;
    override_refresh = -1;
    if (rasterizer_parse_vidmode_commandline(&override_width, &override_height, &override_refresh) == 0) {
        if (allow_display_query == 0) {
            profile->screen_width = 800;
            profile->screen_height = 600;
            profile->refresh_rate = 0x3c;
            return 1;
        }
        display_mode_get_current(&mode);
        profile->screen_height = (int16_t)mode.height;
        profile->screen_width = (int16_t)mode.width;
        profile->refresh_rate = (int16_t)mode.refresh_rate;
        profile->vsync_mode = 0;
        if (mode.vsync != 0) {
            profile->vsync_mode = (unknown_006894ba != 0) + 1;
            return 1;
        }
    } else {
        profile->refresh_rate = 0x3c;
        if (override_width != -1 && override_height != -1) {
            profile->screen_width = (int16_t)override_width;
            profile->screen_height = (int16_t)override_height;
        }
        if (override_refresh != -1) {
            profile->refresh_rate = (int16_t)override_refresh;
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x53b000):

/* WARNING: Removing unreachable block (ram,0x0053b134) */
/* WARNING: Removing unreachable block (ram,0x0053b13c) */

undefined4 player_profile_set_default_video_options(int param_1,char param_2)

{
  char cVar1;
  int iVar2;
  undefined1 extraout_CL;
  bool bVar3;
  undefined4 local_18;
  int local_14;
  undefined2 local_10;
  undefined2 local_c;
  undefined2 local_8;
  char local_4;

  cVar1 = DAT_0071d1e0;
  bVar3 = DAT_0071d1e0 == '\0';
  *(undefined1 *)(param_1 + 0xa6e) = 2;
  *(undefined1 *)(param_1 + 0xa75) = 2;
  *(char *)(param_1 + 0xa76) = cVar1;
  if (bVar3) {
    cVar1 = '\x01';
  }
  else if (cVar1 == -1) {
    cVar1 = -2;
  }
  *(char *)(param_1 + 0xa76) = cVar1;
  if ((((DAT_007196f4 != 0) || (DAT_007c118c < 0xffff0101)) || (DAT_00722bac < 0x3e9)) ||
     ((DAT_00722ba8 < 0x81 || (DAT_00722bb0 < 0x2000001)))) {
    bVar3 = DAT_007196f4 == 0;
    *(undefined1 *)(param_1 + 0xa70) = 0;
    *(undefined1 *)(param_1 + 0xa71) = 0;
    *(undefined1 *)(param_1 + 0xa72) = 0;
    *(bool *)(param_1 + 0xa73) = bVar3;
    *(undefined1 *)(param_1 + 0xa74) = 1;
    *(undefined2 *)(param_1 + 0xa68) = 0x280;
    *(undefined2 *)(param_1 + 0xa6a) = 0x1e0;
    *(undefined2 *)(param_1 + 0xa6c) = 0x3c;
    *(undefined1 *)(param_1 + 0xa6f) = 2;
    return 1;
  }
  local_18 = 0xffffffff;
  local_14 = -1;
  *(bool *)(param_1 + 0xa70) = DAT_00722b6c == 0;
  *(undefined1 *)(param_1 + 0xa71) = 1;
  iVar2 = rasterizer_decal_zbias_active();
  bVar3 = DAT_007196f0 == 0;
  *(bool *)(param_1 + 0xa72) = (char)iVar2 != '\0';
  *(undefined1 *)(param_1 + 0xa74) = extraout_CL;
  *(undefined1 *)(param_1 + 0xa73) = extraout_CL;
  *(undefined1 *)(param_1 + 0xa6f) = extraout_CL;
  if (bVar3) {
    cVar1 = rasterizer_parse_vidmode_commandline(&local_18,&local_14);
    if (cVar1 == '\0') {
      if (param_2 == '\0') {
        *(undefined2 *)(param_1 + 0xa68) = 800;
        *(undefined2 *)(param_1 + 0xa6a) = 600;
        *(undefined2 *)(param_1 + 0xa6c) = 0x3c;
        return 1;
      }
      display_mode_get_current();
      *(undefined2 *)(param_1 + 0xa6a) = local_c;
      *(undefined2 *)(param_1 + 0xa68) = local_10;
      *(undefined2 *)(param_1 + 0xa6c) = local_8;
      *(undefined1 *)(param_1 + 0xa6f) = 0;
      if (local_4 != '\0') {
        *(char *)(param_1 + 0xa6f) = (DAT_006894ba != '\0') + '\x01';
        return 1;
      }
    }
    else {
      *(undefined2 *)(param_1 + 0xa6c) = 0x3c;
      if (local_14 != -1) {
        *(short *)(param_1 + 0xa6c) = (short)local_14;
        return 1;
      }
    }
  }
  else {
    *(undefined2 *)(param_1 + 0xa68) = 0x280;
    *(undefined2 *)(param_1 + 0xa6a) = 0x1e0;
    *(undefined2 *)(param_1 + 0xa6c) = 0x3c;
  }
  return 1;
}
#endif
