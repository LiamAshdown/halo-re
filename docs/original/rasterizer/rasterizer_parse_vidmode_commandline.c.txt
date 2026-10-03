// rasterizer_parse_vidmode_commandline  (Ghidra: rasterizer_parse_vidmode_commandline, already
// named)
// address 0x5168c0, size 252 bytes
// name confidence: 0.65  rewrite confidence: 0.6
// evidence: parses "-vidmode %d,%d,%d" (width,height,refresh) and "-refresh %d" from the command
//   line; command_line_check_flag's real signature (EDI out_value) is established elsewhere in
//   the tree (src/interface/network_autojoin_from_command_line.c) and used here instead of the
//   uninitialized-looking `local_10` Ghidra shows.
// register convention: width destination in unaff_ESI, height/refresh destinations as the two
//   recognized stack parameters (param_1 = height, param_2 = refresh).
//   // blam-cc: unaff_ESI -> width_out, stack -> (height_out, refresh_out)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t unknown_0071d1b0; // 0x0071d1b0 UNSURE: "already parsed once" latch
extern uint8_t rasterizer_needs_reset; // 0x0071d16d UNSURE: "explicit vidmode requested" flag
extern uint8_t video_force_mode_flag; // 0x0071d170 UNSURE, shared with rasterizer_build_present_parameters.c

extern uint8_t command_line_check_flag(const char *flag, const char **out_value); // 0x542760

// blam-cc: unaff_ESI -> height_out, stack -> (width_out, refresh_out)
// Parses -vidmode width,height[,refresh] and -refresh refresh from the command line into
// *width_out/*height_out/*refresh_out (any of which may be NULL). Returns 1 if either switch was
// present.
uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out)
{
    uint8_t found = 0;
    const char *value;
    int32_t width = 800;
    int32_t height = 600;
    long refresh = 0x3c;

    if (command_line_check_flag("-vidmode", &value) != 0 && value != (const char *)0) {
        int32_t parsed = sscanf(value, "%d,%d,%d", &width, &height, &refresh);
        if (parsed == 2 || parsed == 3) {
            if (parsed == 3 && refresh_out != (long *)0) {
                *refresh_out = refresh;
            }
            if (width_out != (int32_t *)0) {
                *width_out = width;
            }
            if (height_out != (int32_t *)0) {
                *height_out = height;
            }
            if (unknown_0071d1b0 == 0) {
                rasterizer_needs_reset = 1;
            }
            found = 1;
        }
    }

    if (command_line_check_flag("-refresh", &value) != 0) {
        refresh = (value == (const char *)0) ? 0 : atol(value);
        if (refresh_out != (long *)0) {
            *refresh_out = refresh;
        }
        found = 1;
    }

    if (unknown_0071d1b0 == 0) {
        if (refresh == 0) {
            video_force_mode_flag = 1;
        }
        unknown_0071d1b0 = 1;
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x5168c0):

undefined1 rasterizer_parse_vidmode_commandline(undefined4 *param_1,long *param_2)

{
  char cVar1;
  int iVar2;
  undefined1 uVar3;
  undefined4 *unaff_ESI;
  char *local_10;
  long local_c;
  undefined4 local_8;
  undefined4 local_4;

  uVar3 = 0;
  local_8 = 800;
  local_4 = 600;
  local_c = 0x3c;
  cVar1 = command_line_check_flag("-vidmode");
  if ((cVar1 != '\0') && (local_10 != (char *)0x0)) {
    iVar2 = _sscanf(local_10,"%d,%d,%d",&local_8,&local_4,&local_c);
    if (iVar2 != 2) {
      if (iVar2 != 3) goto LAB_0051695b;
      if (param_2 != (long *)0x0) {
        *param_2 = local_c;
      }
    }
    if (unaff_ESI != (undefined4 *)0x0) {
      *unaff_ESI = local_8;
    }
    if (param_1 != (undefined4 *)0x0) {
      *param_1 = local_4;
    }
    if (DAT_0071d1b0 == '\0') {
      DAT_0071d16d = 1;
    }
    uVar3 = 1;
  }
LAB_0051695b:
  cVar1 = command_line_check_flag("-refresh");
  if (cVar1 != '\0') {
    if (local_10 == (char *)0x0) {
      local_c = 0;
    }
    else {
      local_c = _atol(local_10);
    }
    if (param_2 != (long *)0x0) {
      *param_2 = local_c;
    }
    uVar3 = 1;
  }
  if (DAT_0071d1b0 == '\0') {
    if (local_c == 0) {
      DAT_0071d170 = 1;
    }
    DAT_0071d1b0 = '\x01';
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
