// ui_network_host_setup_defaults_init  (Ghidra: FUN_004a2ad0, renamed)
// renamed from FUN_004a2ad0 in the naming pass
// address 0x4a2ad0, size 375 bytes, callers=0 in this build
// name confidence: 0.2   rewrite confidence: 0.75
// evidence: functions.md: "Initializes default profile name fields and a resolution/quality-like
// list selection, enabling widgets based on network-capability globals." Rewritten in the
// phase-4 review from objdump -d 0x4a2ad0..0x4a2c46. The two locals Ghidra showed as never
// written are bytes of the 0x2000 byte profile copy on the stack (profile + 0xfc0, a 0..4 list
// choice, and profile + 0xebf, an index clamped to that choice's row count); the copy comes
// from saved profile slot 0 or, without a current profile, from the default builder 0x53a150
// (ESI out). The wide strings copied out are profile + 0xd8c (to 0x00719170) and + 0xeac (to
// 0x007191f0). The second row (not the first) is hidden while 0x00719010 is set, and the return
// is the byte 1 (AL; EAX held 1.0f, which Ghidra printed as 0x3f800001).
// UNSURE: the meaning of the two tables at 0x0065bf74 / 0x0065bfb4 and of 0x00692b04 /
// 0x00719204 / 0x00699584 (a game setup choice, its sub index and the value picked from the
// second table).
// register convention: cdecl, the one stack parameter (widget).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

extern uint8_t save_in_progress_00719010;           // 0x00719010
extern int32_t quality_selection_00692b04;          // 0x00692b04
extern int32_t resolution_selection_00719204;       // 0x00719204
extern int32_t saved_player_profile_slots_handle;               // 0x00714dd4
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h
extern uint16_t network_host_name_00719170[0x40];   // 0x00719170
extern uint16_t network_host_subname_007191f0[9];   // 0x007191f0
extern int32_t resolution_row_count_table_0065bfb4[5]; // 0x0065bfb4
extern int32_t resolution_index_table_0065bf74[];   // 0x0065bf74
extern int32_t sv_maxplayers_value;  // 0x00699584
extern uint8_t network_game_info_packet_flag;    // 0x006894a2

extern void player_profile_set_default_server_options(uint8_t *out_profile);     // 0x53a150, blam-cc: ESI out_profile; builds the default profile

uint8_t ui_network_host_setup_defaults_init(widget_instance *widget)
{
    uint8_t profile[0x1ffc];
    int32_t choice;
    int32_t last_row;
    int32_t index;
    widget_instance *row1;
    widget_instance *row2;
    widget_instance *control;

    quality_selection_00692b04 = 4;
    if (save_in_progress_00719010 != 0) {
        resolution_selection_00719204 = 0;
    }
    if (saved_player_profile_slots_handle != -1) {
        memcpy(profile, &profile_globals_block[0].profile, sizeof(profile));
    } else {
        player_profile_set_default_server_options(profile);
    }

    wcslen((const uint16_t *)(profile + 0xd8c));
    wcscpy(network_host_name_00719170, (const uint16_t *)(profile + 0xd8c));
    wcslen((const uint16_t *)(profile + 0xeac));
    wcscpy(network_host_subname_007191f0, (const uint16_t *)(profile + 0xeac));

    choice = (profile[0xfc0] > 4) ? 4 : profile[0xfc0];
    last_row = resolution_row_count_table_0065bfb4[choice] - 1;
    if ((int32_t)profile[0xebf] > last_row) {
        profile[0xebf] = (uint8_t)last_row;
    }
    quality_selection_00692b04 = choice;
    index = profile[0xebf];
    if (index < 0) {
        index = 0;
    } else if (index > last_row) {
        index = last_row;
    }
    resolution_selection_00719204 = index;
    sv_maxplayers_value = resolution_index_table_0065bf74[index];

    row1 = widget->first_child->next_sibling->next_sibling;
    control = row1->first_child->next_sibling;
    if (network_game_info_packet_flag != 0) {
        row1->scale = 1.0f;
        row1->hidden = 0;
        control->selection_index = (int16_t)choice;
    } else {
        row1->scale = 0.333f;
        row1->hidden = 1;
        control->selection_index = 4;
    }

    row2 = row1->next_sibling;
    control = row2->first_child->next_sibling;
    control->selection_index = (int16_t)index;
    control->item_count = (uint16_t)resolution_row_count_table_0065bfb4[choice];
    if (save_in_progress_00719010 != 0) {
        row2->scale = 0.333f;
        row2->hidden = 1;
    } else {
        row2->scale = 1.0f;
        row2->hidden = 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a2ad0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x004a2ba7) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004a2ad0(int param_1)

{
  undefined2 uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  uint uVar8;
  bool bVar9;
  undefined4 local_2008 [867];
  wchar_t local_127c [144];
  wchar_t local_115c [9];
  byte local_1149;
  byte local_1048;
  undefined4 uStack_c;

  uStack_c = 0x4a2ae0;
  DAT_00692b04 = 4;
  if (DAT_00719010 != '\0') {
    _DAT_00719204 = 0;
  }
  if (DAT_00714dd4 == -1) {
    FUN_0053a150();
  }
  else {
    puVar6 = &DAT_00712dd8;
    puVar7 = local_2008;
    for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
  }
  FUN_00625b7a(local_127c);
  _wcscpy((wchar_t *)&DAT_00719170,local_127c);
  FUN_00625b7a(local_115c);
  _wcscpy((wchar_t *)&DAT_007191f0,local_115c);
  uVar8 = 4;
  if (local_1048 < 5) {
    uVar8 = (uint)local_1048;
  }
  uVar5 = (uint)local_1149;
  uVar3 = *(int *)(&DAT_0065bfb4 + uVar8 * 4) - 1;
  if ((int)uVar3 < (int)uVar5) {
    uVar5 = uVar3 & 0xff;
  }
  if ((int)uVar3 < (int)uVar5) {
    uVar5 = uVar3;
  }
  DAT_00699584 = (&DAT_0065bf74)[uVar5];
  iVar4 = *(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x2c) + 0x2c);
  iVar2 = *(int *)(*(int *)(iVar4 + 0x34) + 0x2c);
  DAT_00692b04 = uVar8;
  _DAT_00719204 = uVar5;
  if (DAT_006894a2 == '\0') {
    *(undefined4 *)(iVar4 + 0x24) = 0x3eaa7efa;
    *(undefined1 *)(iVar4 + 0x12) = 1;
    *(undefined2 *)(iVar2 + 0x40) = 4;
  }
  else {
    *(undefined4 *)(iVar4 + 0x24) = 0x3f800000;
    *(undefined1 *)(iVar4 + 0x12) = 0;
    *(short *)(iVar2 + 0x40) = (short)uVar8;
  }
  iVar4 = *(int *)(iVar4 + 0x2c);
  iVar2 = *(int *)(*(int *)(iVar4 + 0x34) + 0x2c);
  uVar1 = *(undefined2 *)(&DAT_0065bfb4 + uVar8 * 4);
  *(short *)(iVar2 + 0x40) = (short)uVar5;
  bVar9 = DAT_00719010 != '\0';
  *(undefined2 *)(iVar2 + 0x48) = uVar1;
  if (bVar9) {
    *(undefined4 *)(iVar4 + 0x24) = 0x3eaa7efa;
    *(undefined1 *)(iVar4 + 0x12) = 1;
    return 0x3f800001;
  }
  *(undefined4 *)(iVar4 + 0x24) = 0x3f800000;
  *(undefined1 *)(iVar4 + 0x12) = 0;
  return 0x3f800001;
}
#endif
