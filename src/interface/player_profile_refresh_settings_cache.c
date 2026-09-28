// player_profile_refresh_settings_cache  (Ghidra: FUN_00496060, unnamed)
// address 0x496060, size 878 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Converts a saved profile's raw settings
// (difficulty, name, 0-9 sensitivity/acceleration slider indic[es]) ..."; every source field
// this function reads lives at a fixed byte offset inside profile_globals_block (0x00712dd8,
// per player_profile_subsystem_initialize.c), stride 0x2004 per slot.
// register convention: player index in BX (unaff_BX). // blam-cc: BX -> player_index
// The destination row is player_control_settings (types/interface.h), 0x85c bytes: the final
// copy is rep movsd with ecx 0x217 dwords to 0x710328 + slot * 0x85c (imul at 0x4963ba).
// Destination offsets below are the stack locals' own frame positions relative to
// local_8e0[0x1e], not their assignment order; note the 0xda byte first range (0x36 dwords plus
// one trailing halfword) and the two zero bytes at 0x80e.
// UNSURE: the fields are named only by the table they are remapped through; which control each
// one drives is not attested in this function.
// reconciled: R19 player_control_settings unknown ranges named from the input.h field map (keyboard, mouse_button/mouse_axis, gamepad_button, gamepad_action_button, gamepad_axis, gamepad_pov, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y, gamepad_rate_80/40, look_inverted/_driving); same offsets and widths

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern uint8_t profile_globals_block[];         // 0x00712dd8, stride 0x2004 per slot
extern int16_t profile_slot_id[];               // 0x00714dde, indexed by player index
extern player_control_settings input_globals[]; // 0x00710328

// Remap tables, built on the stack in the original (local_8e0[0x00..0x1d]).
static const float k_table_80[10] = { // local_8e0[0x00..0x09]
    80.0f, 100.0f, 120.0f, 140.0f, 160.0f, 180.0f, 200.0f, 220.0f, 240.0f, 260.0f
};
static const float k_table_40[10] = { // local_8e0[0x0a..0x13]
    40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f, 110.0f, 120.0f, 130.0f
};
static const float k_table_01[10] = { // local_8e0[0x14..0x1d], 0x3dcccccd is 0.1f
    0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 3.0f, 4.0f
};

static uint32_t slider_index(uint8_t value)
{
    return (value < 10) ? value : 9;
}

// blam-cc: BX -> player_index
// Rebuilds player_index's row of input_globals from its raw profile record.
// The destination row is profile_slot_id[player_index] if one is assigned, else player_index.
void player_profile_refresh_settings_cache(int16_t player_index)
{
    player_control_settings settings;
    uint8_t *profile = profile_globals_block + (int32_t)player_index * 0x2004;
    int32_t slider;
    int32_t dest_slot;
    int32_t i;

    memset(&settings, 0, sizeof(settings));

    slider = (int32_t)profile[0x12e] - 1;
    if (slider < 0) {
        slider = 0;
    } else if (slider > 9) {
        slider = 9;
    }
    settings.look_rate_80 = k_table_80[slider];
    settings.look_rate_40 = k_table_40[slider];

    memcpy(settings.keyboard, profile + 0x134, sizeof(settings.keyboard)); // 0x36 dwords + 1 halfword
    memcpy(settings.mouse_button, profile + 0x20e, sizeof(settings.mouse_button) + sizeof(settings.mouse_axis)); // one 7-dword copy
    memcpy(settings.gamepad_button, profile + 0x22a, sizeof(settings.gamepad_button));
    memcpy(settings.gamepad_action_button, profile + 0x32a, sizeof(settings.gamepad_action_button));
    memcpy(settings.gamepad_axis, profile + 0x33a, sizeof(settings.gamepad_axis));
    memcpy(settings.gamepad_pov, profile + 0x53a, sizeof(settings.gamepad_pov));
    memcpy(&settings.forward_rate, profile + 0x93c, 6 * sizeof(float)); // forward_rate .. mouse_strafe_scale

    settings.mouse_look_x_sensitivity = k_table_01[slider_index(profile[0x954])];
    settings.mouse_look_y_sensitivity = k_table_01[slider_index(profile[0x955])];

    memcpy(&settings.gamepad_axis_scale_x, profile + 0x960, 2 * sizeof(float)); // x, y

    for (i = 0; i < 4; i++) {
        settings.gamepad_rate_80[i] = k_table_80[slider_index(profile[0x956 + i])];
        settings.gamepad_rate_40[i] = k_table_40[slider_index(profile[0x95a + i])];
    }

    settings.look_inverted = profile[0x12f];
    settings.look_inverted_driving = profile[0x131];

    dest_slot = profile_slot_id[player_index];
    if (profile_slot_id[player_index] == -1) {
        dest_slot = player_index;
    }
    input_globals[dest_slot] = settings;
}

#if 0
Original Ghidra decompilation (0x496060):

void FUN_00496060(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  int iVar6;
  short unaff_BX;
  int iVar7;
  byte *pbVar8;
  undefined4 *puVar9;
  undefined4 local_8e0 [32];
  undefined4 local_860 [54];
  undefined4 local_786;
  undefined4 local_782;
  undefined4 local_77e;
  undefined4 local_77a;
  undefined4 local_776;
  undefined4 local_772;
  undefined4 local_76e;
  undefined4 local_76a [64];
  undefined4 local_66a;
  undefined4 local_666;
  undefined4 local_662;
  undefined4 local_65e;
  undefined4 local_65a [128];
  undefined4 local_45a [256];
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30 [4];
  undefined4 local_20 [4];
  undefined1 local_10;
  undefined1 local_f;

  iVar7 = (int)unaff_BX;
  puVar5 = local_8e0 + 0x1f;
  for (iVar2 = 0x216; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  iVar2 = iVar7 * 0x2004;
  iVar3 = (byte)(&DAT_00712f06)[iVar2] - 1;
  local_8e0[10] = 0x42200000;
  local_8e0[0xb] = 0x42480000;
  local_8e0[0xc] = 0x42700000;
  local_8e0[0xd] = 0x428c0000;
  local_8e0[0xe] = 0x42a00000;
  local_8e0[0xf] = 0x42b40000;
  local_8e0[0x10] = 0x42c80000;
  local_8e0[0x11] = 0x42dc0000;
  local_8e0[0x12] = 0x42f00000;
  local_8e0[0x13] = 0x43020000;
  local_8e0[0] = 0x42a00000;
  local_8e0[1] = 0x42c80000;
  local_8e0[2] = 0x42f00000;
  local_8e0[3] = 0x430c0000;
  local_8e0[4] = 0x43200000;
  local_8e0[5] = 0x43340000;
  local_8e0[6] = 0x43480000;
  local_8e0[7] = 0x435c0000;
  local_8e0[8] = 0x43700000;
  local_8e0[9] = 0x43820000;
  local_8e0[0x14] = 0x3dcccccd;
  local_8e0[0x15] = 0x3e800000;
  local_8e0[0x16] = 0x3f000000;
  local_8e0[0x17] = 0x3f400000;
  local_8e0[0x18] = 0x3f800000;
  local_8e0[0x19] = 0x3fa00000;
  local_8e0[0x1a] = 0x3fc00000;
  local_8e0[0x1b] = 0x40000000;
  local_8e0[0x1c] = 0x40400000;
  local_8e0[0x1d] = 0x40800000;
  if (iVar3 < 0) {
    iVar6 = 0;
  }
  else {
    iVar6 = 9;
    if (iVar3 < 10) {
      iVar6 = iVar3;
    }
  }
  if (iVar3 < 0) {
    iVar3 = 0;
  }
  else if (9 < iVar3) {
    iVar3 = 9;
  }
  local_8e0[0x1f] = local_8e0[(short)iVar6 + 10];
  local_8e0[0x1e] = local_8e0[(short)iVar3];
  puVar5 = (undefined4 *)(&DAT_00712f0c + iVar2);
  puVar9 = local_860;
  for (iVar3 = 0x36; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar9 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar9 = puVar9 + 1;
  }
  uVar1 = *(undefined4 *)(&DAT_00712fe6 + iVar2);
  *(undefined2 *)puVar9 = *(undefined2 *)puVar5;
  local_786 = uVar1;
  local_782 = *(undefined4 *)(&DAT_00712fea + iVar2);
  local_77e = *(undefined4 *)(&DAT_00712fee + iVar2);
  local_77a = *(undefined4 *)(&DAT_00712ff2 + iVar2);
  local_776 = *(undefined4 *)(&DAT_00712ff6 + iVar2);
  local_76e = *(undefined4 *)(&DAT_00712ffe + iVar2);
  local_772 = *(undefined4 *)(&DAT_00712ffa + iVar2);
  puVar5 = (undefined4 *)(&DAT_00713002 + iVar2);
  puVar9 = local_76a;
  for (iVar3 = 0x40; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar9 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar9 = puVar9 + 1;
  }
  local_66a = *(undefined4 *)(&DAT_00713102 + iVar2);
  local_666 = *(undefined4 *)(&DAT_00713106 + iVar2);
  local_662 = *(undefined4 *)(&DAT_0071310a + iVar2);
  local_65e = *(undefined4 *)(&DAT_0071310e + iVar2);
  uVar1 = *(undefined4 *)(&DAT_00713718 + iVar2);
  puVar5 = (undefined4 *)(&DAT_00713112 + iVar2);
  puVar9 = local_65a;
  for (iVar3 = 0x80; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar9 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar9 = puVar9 + 1;
  }
  puVar5 = (undefined4 *)(&DAT_00713312 + iVar2);
  puVar9 = local_45a;
  for (iVar3 = 0x100; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar9 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar9 = puVar9 + 1;
  }
  local_58 = *(undefined4 *)(&DAT_00713714 + iVar2);
  local_50 = *(undefined4 *)(&DAT_0071371c + iVar2);
  local_54 = uVar1;
  local_48 = *(undefined4 *)(&DAT_00713724 + iVar2);
  local_4c = *(undefined4 *)(&DAT_00713720 + iVar2);
  local_44 = *(undefined4 *)(&DAT_00713728 + iVar2);
  if ((byte)(&DAT_0071372c)[iVar2] < 10) {
    uVar4 = (uint)(byte)(&DAT_0071372c)[iVar2];
  }
  else {
    uVar4 = 9;
  }
  local_40 = local_8e0[uVar4 + 0x14];
  if ((byte)(&DAT_0071372d)[iVar2] < 10) {
    uVar4 = (uint)(byte)(&DAT_0071372d)[iVar2];
  }
  else {
    uVar4 = 9;
  }
  local_3c = local_8e0[uVar4 + 0x14];
  local_38 = *(undefined4 *)(&DAT_00713738 + iVar2);
  local_34 = *(undefined4 *)(&DAT_0071373c + iVar2);
  pbVar8 = &DAT_00713732 + iVar2;
  puVar5 = local_20;
  iVar3 = 4;
  do {
    if (pbVar8[-4] < 10) {
      uVar4 = (uint)pbVar8[-4];
    }
    else {
      uVar4 = 9;
    }
    puVar5[-4] = local_8e0[uVar4];
    if (*pbVar8 < 10) {
      uVar4 = (uint)*pbVar8;
    }
    else {
      uVar4 = 9;
    }
    *puVar5 = local_8e0[uVar4 + 10];
    pbVar8 = pbVar8 + 1;
    puVar5 = puVar5 + 1;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  local_10 = (&DAT_00712f07)[iVar2];
  local_f = (&DAT_00712f09)[iVar2];
  iVar3 = 0x217;
  iVar2 = (int)*(short *)(&DAT_00714dde + iVar7 * 2);
  if (*(short *)(&DAT_00714dde + iVar7 * 2) == -1) {
    iVar2 = iVar7;
  }
  puVar5 = local_8e0 + 0x1e;
  puVar9 = &DAT_00710328 + iVar2 * 0x217;
  for (; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar9 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar9 = puVar9 + 1;
  }
  return;
}
#endif
