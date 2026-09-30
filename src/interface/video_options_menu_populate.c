// video_options_menu_populate  (Ghidra: FUN_004baec0, still unnamed there; named here)
// address 0x4baec0, size 975 bytes
// name confidence: 0.4 (chosen)   rewrite confidence: 0.75
// evidence: out/phase4/interface_functions.md "Populates the video-options menu's widgets
// (resolution, refresh rate, gamma, and several quality/toggle controls) from a video
// settings structure, disabling controls the current hardware/driver does not support.";
// types/interface.h video_resolution / video_resolutions / video_resolution_count /
// video_refresh_rate_find_index (this module).
// register convention: a widget/context pointer and a video-settings-record pointer in the
// two recovered stack parameters (param_1, param_2).
// Checked against objdump 0x4baec0..0x4bb28f in the phase-4 review: the fallback refresh
// index is looked up for 60 Hz (EDI 0x3c), and the refresh spinner item count is the count of
// resolution entry 0; both fixed. The spinner found under the row after the last one is never
// written (the gamma byte +0xa76 goes to 0x00695464 / 0x0071d1e0, clamped 1..0xfe, and the
// function tail-jumps to 0x5227a0). The layout notes below still apply. The widget-tree walk (+0x2c/+0x30/+0x34/+0x40/+0x44/+0x48,
// and the "find the next sibling with kind==2" loops at +0xe) and the video-settings record
// offsets (+0xa68.. +0xa76) have no header type in this codebase.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_rasterizer.h"
#include "fn_shell.h"
#include "fn_interface.h"

extern uint8_t video_force_mode_flag;    // 0x0071d170, UNSURE name (distinct byte from 0x0071d16c below)
extern int32_t os_platform;     // 0x00721ef0, UNSURE name, set by os_platform_identify
extern int32_t os_platform_refresh_default; // 0x007c11f8, UNSURE name
extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_resolution_count;            // 0x007196cc
extern uint8_t rasterizer_fullscreen; // 0x0071d16c, UNSURE name (see video_display_modes_enumerate.c's video_force_mode_flag; kept distinct here since both addresses appear together)
extern uint32_t rasterizer_device;   // 0x0071d174
extern uint32_t rasterizer_device_version; // 0x007c118c, UNSURE name
extern uint32_t config_disable_specular; // 0x00722b6c, UNSURE name
extern uint32_t rasterizer_capability_007c10e4; // 0x007c10e4, UNSURE name
extern int32_t video_gamma_setting; // 0x00695464, UNSURE name
extern int32_t rasterizer_gamma_exponent;      // 0x0071d1e0, UNSURE name

extern void video_resolution_list_build(void); // 0x4bad40, this module


void video_options_menu_populate(uint8_t *context, uint8_t *settings)
{
    int32_t target_refresh;
    uint8_t *resolution_field;
    uint8_t *resolution_field2;
    uint8_t *refresh_field;
    int32_t resolution_index;
    uint32_t refresh_index;
    int i;
    uint8_t *node;

    if (video_force_mode_flag == 0) {
        target_refresh = *(int16_t *)(settings + 0xa6c);
    } else {
        if (os_platform == 0) {
            os_platform_identify();
        }
        target_refresh = os_platform_refresh_default;
        if (os_platform < 3) {
            target_refresh = 0x3c;
        }
    }

    resolution_field = *(uint8_t **)(*(uint8_t **)(context + 0x34) + 0x2c);
    resolution_field2 = *(uint8_t **)(*(uint8_t **)(*(uint8_t **)(context + 0x34) + 0x34) + 0x2c);
    refresh_field = *(uint8_t **)(*(uint8_t **)(resolution_field + 0x34) + 0x2c);

    video_resolution_list_build();

    resolution_index = -1;
    for (i = 0; i < video_resolution_count; i++) {
        if (video_resolutions[i].width == *(int16_t *)(settings + 0xa68) &&
            video_resolutions[i].height == *(int16_t *)(settings + 0xa6a)) {
            resolution_index = i;
            break;
        }
    }
    if (resolution_index == -1) {
        for (i = 0; i < video_resolution_count; i++) {
            if (video_resolutions[i].width == 0x280 && video_resolutions[i].height == 0x1e0) {
                resolution_index = i;
                break;
            }
        }
    }

    refresh_index = 0xffffffffu;
    if (resolution_index >= 0 && resolution_index < video_resolution_count) {
        for (i = 0; i < (int32_t)video_resolutions[resolution_index].refresh_rate_count; i++) {
            if (video_resolutions[resolution_index].refresh_rates[i] == target_refresh) {
                refresh_index = (uint32_t)i;
                break;
            }
        }
    }
    if (refresh_index == 0xffffffffu) {
        refresh_index = video_refresh_rate_find_index(resolution_index, 0x3c); // 60 Hz, not target_refresh
    }

    *(video_resolution **)(resolution_field2 + 0x44) = video_resolutions;
    *(int16_t *)(resolution_field2 + 0x40) = (int16_t)resolution_index;
    *(int16_t *)(resolution_field2 + 0x48) = (int16_t)video_resolution_count;

    *(video_resolution **)(refresh_field + 0x44) = video_resolutions;
    *(int16_t *)(refresh_field + 0x48) = (int16_t)video_resolutions[0].refresh_rate_count; // entry 0, not the selected one (0x006b66b8)
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        *(int16_t *)(refresh_field + 0x40) = 0;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 1;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3eaa7efa;
    } else if (video_force_mode_flag == 0 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        *(int16_t *)(refresh_field + 0x40) = (int16_t)refresh_index;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 0;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3f800000;
    } else {
        *(int16_t *)(refresh_field + 0x40) = (int16_t)refresh_index;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 1;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3eaa7efa;
    }

    // remaining quality/toggle rows: each block walks to the next sibling whose kind==2, then
    // stamps a widget value/enabled-state from `settings`; the exact toggle each block controls
    // is not independently named here (see header UNSURE note)
    {
        uint8_t *base = *(uint8_t **)(resolution_field + 0x2c);

        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(uint8_t *)(settings + 0xa6f) < 3) ? *(uint8_t *)(settings + 0xa6f) : 2;

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa70) != 0) ? 1 : 0;
        if (rasterizer_device_version < 0xffff0101u || config_disable_specular != 0) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa71) != 0) ? 1 : 0;
        if (rasterizer_device_version < 0xffff0101u) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa72) != 0) ? 1 : 0;
        if ((rasterizer_capability_007c10e4 & 0x6000000) == 0) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(uint8_t *)(settings + 0xa73) < 3) ? *(uint8_t *)(settings + 0xa73) : 2;

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(uint8_t *)(settings + 0xa74) < 3) ? *(uint8_t *)(settings + 0xa74) : 2;

        base = *(uint8_t **)(base + 0x2c);
        (void)base;
    }

    {
        uint8_t gamma = *(uint8_t *)(settings + 0xa76);
        if (gamma == 0) {
            video_gamma_setting = 1;
            rasterizer_gamma_exponent = 1;
        } else if (gamma == 0xff) {
            video_gamma_setting = 0xfe;
            rasterizer_gamma_exponent = 0xfe;
        } else {
            video_gamma_setting = gamma;
            rasterizer_gamma_exponent = gamma;
        }
        chimera__gamma();
    }
}

#if 0
Original Ghidra decompilation (0x4baec0):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004baec0(int param_1,int param_2)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  ushort uVar9;
  int *piVar10;
  uint uVar11;
  undefined2 uVar12;
  int iVar13;
  bool bVar14;
  
  if (DAT_0071d170 == '\0') {
    iVar5 = (int)*(short *)(param_2 + 0xa6c);
  }
  else {
    if (DAT_00721ef0 == 0) {
      os_platform_identify();
    }
    iVar5 = DAT_007c11f8;
    if (DAT_00721ef0 < 3) {
      iVar5 = 0x3c;
    }
  }
  iVar2 = *(int *)(*(int *)(param_1 + 0x34) + 0x2c);
  iVar3 = *(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x2c);
  iVar4 = *(int *)(*(int *)(iVar2 + 0x34) + 0x2c);
  video_resolution_list_build();
  iVar6 = 0;
  iVar13 = -1;
  if (0 < DAT_007196cc) {
    piVar10 = (int *)&DAT_006b6694;
    do {
      if ((piVar10[-1] == (int)*(short *)(param_2 + 0xa68)) &&
         (iVar13 = iVar6, *piVar10 == (int)*(short *)(param_2 + 0xa6a))) break;
      iVar6 = iVar6 + 1;
      piVar10 = piVar10 + 0x13;
      iVar13 = -1;
    } while (iVar6 < DAT_007196cc);
  }
  iVar6 = iVar13;
  if ((iVar13 == -1) && (iVar7 = 0, 0 < DAT_007196cc)) {
    piVar10 = (int *)&DAT_006b6694;
    do {
      if ((piVar10[-1] == 0x280) && (iVar6 = iVar7, *piVar10 == 0x1e0)) break;
      iVar7 = iVar7 + 1;
      piVar10 = piVar10 + 0x13;
      iVar6 = iVar13;
    } while (iVar7 < DAT_007196cc);
  }
  uVar12 = (undefined2)DAT_007196cc;
  if ((-1 < iVar6) && (iVar6 < DAT_007196cc)) {
    uVar11 = 0;
    uVar8 = 0xffffffff;
    if ((&DAT_006b66b8)[iVar6 * 0x13] != 0) {
      piVar10 = (int *)(&DAT_006b66bc + iVar6 * 0x26);
      do {
        uVar8 = uVar11;
        if (*piVar10 == iVar5) goto LAB_004bafd4;
        uVar11 = uVar11 + 1;
        piVar10 = piVar10 + 1;
      } while (uVar11 < (uint)(&DAT_006b66b8)[iVar6 * 0x13]);
      uVar8 = 0xffffffff;
    }
LAB_004bafd4:
    if (uVar8 != 0xffffffff) goto LAB_004bafe7;
  }
  uVar8 = video_refresh_rate_find_index();
LAB_004bafe7:
  *(undefined2 **)(iVar3 + 0x44) = &DAT_006b6690;
  *(short *)(iVar3 + 0x40) = (short)iVar6;
  *(undefined2 *)(iVar3 + 0x48) = uVar12;
  uVar12 = (undefined2)DAT_006b66b8;
  *(undefined2 **)(iVar4 + 0x44) = &DAT_006b6690;
  bVar14 = DAT_00721ef0 == 0;
  *(undefined2 *)(iVar4 + 0x48) = uVar12;
  if (bVar14) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    *(undefined2 *)(iVar4 + 0x40) = 0;
    *(undefined1 *)(*(int *)(iVar4 + 0x30) + 0x12) = 1;
    *(undefined4 *)(*(int *)(iVar4 + 0x30) + 0x24) = 0x3eaa7efa;
  }
  else if (((DAT_0071d170 == '\0') && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
    *(short *)(iVar4 + 0x40) = (short)uVar8;
    *(undefined1 *)(*(int *)(iVar4 + 0x30) + 0x12) = 0;
    *(undefined4 *)(*(int *)(iVar4 + 0x30) + 0x24) = 0x3f800000;
  }
  else {
    *(short *)(iVar4 + 0x40) = (short)uVar8;
    *(undefined1 *)(*(int *)(iVar4 + 0x30) + 0x12) = 1;
    *(undefined4 *)(*(int *)(iVar4 + 0x30) + 0x24) = 0x3eaa7efa;
  }
  iVar5 = *(int *)(iVar2 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(byte *)(param_2 + 0xa6f) < 3) {
    uVar9 = (ushort)*(byte *)(param_2 + 0xa6f);
  }
  else {
    uVar9 = 2;
  }
  *(ushort *)(iVar2 + 0x40) = uVar9;
  iVar5 = *(int *)(iVar5 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  *(ushort *)(iVar2 + 0x40) = (ushort)(*(char *)(param_2 + 0xa70) != '\0');
  if ((DAT_007c118c < 0xffff0101) || (DAT_00722b6c != 0)) {
    *(undefined2 *)(iVar2 + 0x40) = 0;
    *(undefined1 *)(iVar5 + 0x12) = 1;
    *(undefined4 *)(iVar5 + 0x24) = 0x3eaa7efa;
  }
  else {
    *(undefined1 *)(iVar5 + 0x12) = 0;
    *(undefined4 *)(iVar5 + 0x24) = 0x3f800000;
  }
  iVar5 = *(int *)(iVar5 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  *(ushort *)(iVar2 + 0x40) = (ushort)(*(char *)(param_2 + 0xa71) != '\0');
  if (DAT_007c118c < 0xffff0101) {
    *(undefined2 *)(iVar2 + 0x40) = 0;
    *(undefined1 *)(iVar5 + 0x12) = 1;
    *(undefined4 *)(iVar5 + 0x24) = 0x3eaa7efa;
  }
  else {
    *(undefined1 *)(iVar5 + 0x12) = 0;
    *(undefined4 *)(iVar5 + 0x24) = 0x3f800000;
  }
  iVar5 = *(int *)(iVar5 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  *(ushort *)(iVar2 + 0x40) = (ushort)(*(char *)(param_2 + 0xa72) != '\0');
  if ((_DAT_007c10e4 & 0x6000000) == 0) {
    *(undefined2 *)(iVar2 + 0x40) = 0;
    *(undefined1 *)(iVar5 + 0x12) = 1;
    *(undefined4 *)(iVar5 + 0x24) = 0x3eaa7efa;
  }
  else {
    *(undefined1 *)(iVar5 + 0x12) = 0;
    *(undefined4 *)(iVar5 + 0x24) = 0x3f800000;
  }
  iVar5 = *(int *)(iVar5 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  uVar9 = 2;
  if (*(byte *)(param_2 + 0xa73) < 3) {
    uVar9 = (ushort)*(byte *)(param_2 + 0xa73);
  }
  *(ushort *)(iVar2 + 0x40) = uVar9;
  iVar5 = *(int *)(iVar5 + 0x2c);
  for (iVar2 = *(int *)(iVar5 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  uVar9 = 2;
  if (*(byte *)(param_2 + 0xa74) < 3) {
    uVar9 = (ushort)*(byte *)(param_2 + 0xa74);
  }
  *(ushort *)(iVar2 + 0x40) = uVar9;
  for (iVar5 = *(int *)(*(int *)(iVar5 + 0x2c) + 0x34);
      (iVar5 != 0 && (*(short *)(iVar5 + 0xe) != 2)); iVar5 = *(int *)(iVar5 + 0x2c)) {
  }
  bVar1 = *(byte *)(param_2 + 0xa76);
  if (bVar1 != 0) {
    if (bVar1 != 0xff) {
      _DAT_00695464 = (uint)bVar1;
      _DAT_0071d1e0 = _DAT_00695464;
      chimera__gamma();
      return;
    }
    _DAT_00695464 = 0xfe;
    _DAT_0071d1e0 = 0xfe;
    chimera__gamma();
    return;
  }
  _DAT_00695464 = 1;
  _DAT_0071d1e0 = 1;
  chimera__gamma();
  return;
}
#endif
