// server_browser_list_row_gather  (Ghidra: FUN_004b69c0, still unnamed -> renamed)
// address 0x4b69c0, size 533 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md summary ("gathers one server's queried info
// fields (name, map, player counts, ping range) and hands them to the row-population routine
// for display"); the eight literal keys (password, dedicated, hostname, mapname, gametype,
// game_classic, numplayers, maxplayers) appear in exactly the order the eight accessor calls
// run; server_browser_list_row_populate (0x4b67e0) is this module's own rewrite, and its
// unused_param1/unused_param3 slots are exactly where this function's "hostname"/"gametype"
// string results land (evaluated for a side effect that was not identified, then discarded).
// register convention: row widget as a real stack parameter (param_1); a flag byte in AL
// (in_AL) and the GameSpy entry pointer in ECX (in_ECX).
// UNSURE: the two "password"/"dedicated" bool-accessor results are never assigned to a local
// in Ghidra's own output, yet server_browser_list_row_populate needs two boolean flags (its own
// CL/DL registers) that are equally invisible at this call site -- reconstructed as a direct
// feed from these two calls into that callee's flag1/flag2, which is a plausible but unproven
// register short-circuit.
// UNSURE: the "mapname" accessor's result is likewise never assigned; reconstructed as an EAX
// pass-through into map_list_get_friendly_level_name, matching
// server_list_compare_by_mapname.c's identical pattern for the same two callees.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <wchar.h>
#include <string.h>

extern int32_t SBServerGetBoolValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy bool accessor // foreign, GameSpy library, bool accessor
extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library, string accessor
extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library, int accessor
extern int32_t SBServerGetPing(void *entry); // foreign, GameSpy library, ping accessor, no key
extern void map_list_get_friendly_level_name(const char *map_name, wchar_t out_buffer[0x20]); // foreign, outside this session's range

extern wchar_t empty_string[]; // see server_browser_open.c UNSURE

// blam-cc: row widget as param_1 (real parameter); flag byte in AL (in_AL); GameSpy entry
// pointer in ECX (in_ECX)
void server_browser_list_row_gather(network_ui_widget *row, uint8_t flag, void *entry)
{
    char *hostname;
    char *mapname;
    char *gametype;
    int32_t is_classic;
    int32_t is_password;
    int32_t is_dedicated;
    int32_t probe;
    int32_t count_a;
    int32_t count_b;
    int32_t ping;
    wchar_t friendly_map[0x20];

    if (flag == 0) {
        row->highlight_flag = (row->parent->selected_child == row);
    } else {
        row->highlight_flag = 2;
    }
    if (entry != 0) {
        memset(friendly_map, 0, sizeof(friendly_map));
        is_password = SBServerGetBoolValue(entry, "password", 0);
        is_dedicated = SBServerGetBoolValue(entry, "dedicated", 0);
        hostname = SBServerGetStringValue(entry, "hostname", "");
        mapname = SBServerGetStringValue(entry, "mapname", "");
        gametype = SBServerGetStringValue(entry, "gametype", "");
        is_classic = SBServerGetBoolValue(entry, "game_classic", 0);

        probe = SBServerGetIntValue(entry, "numplayers", 0);
        count_b = 0x10;
        if (probe < -1) {
            count_a = -1;
        } else {
            probe = SBServerGetIntValue(entry, "numplayers", 0);
            if (probe < 0x11) {
                count_a = SBServerGetIntValue(entry, "numplayers", 0);
            } else {
                count_a = 0x10;
            }
        }
        probe = SBServerGetIntValue(entry, "maxplayers", 0);
        if (probe < -1) {
            count_b = -1;
        } else {
            probe = SBServerGetIntValue(entry, "maxplayers", 0);
            if (probe < 0x11) {
                count_b = SBServerGetIntValue(entry, "maxplayers", 0);
            }
        }
        probe = SBServerGetPing(entry);
        if (probe < 0) {
            ping = 0;
        } else {
            probe = SBServerGetPing(entry);
            if (probe < 10000) {
                ping = SBServerGetPing(entry);
            } else {
                ping = 9999;
            }
        }

        map_list_get_friendly_level_name(mapname, friendly_map);
        server_browser_list_row_populate(row, (uint8_t)is_password, (uint8_t)is_dedicated,
                                          hostname, friendly_map, gametype,
                                          (uint8_t)(is_classic == 1), count_a, count_b, ping);
        row->hidden = 0; // *(byte*)(param_1+0x12)=0
        return;
    }
    server_browser_list_row_populate(row, 0, 0, 0, empty_string, 0, 0, 0xffffffff, 0xffffffff, 0xffffffff);
    row->hidden = 1; // *(byte*)(param_1+0x12)=1
}

#if 0
Original Ghidra decompilation (0x4b69c0):

void FUN_004b69c0(int param_1)

{
  char in_AL;
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int in_ECX;
  int iVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  undefined4 local_54;
  undefined2 local_40;
  undefined4 local_3e [15];

  if (in_AL == '\0') {
    *(ushort *)(param_1 + 0x58) = (ushort)(*(int *)(*(int *)(param_1 + 0x30) + 0x38) == param_1);
  }
  else {
    *(undefined2 *)(param_1 + 0x58) = 2;
  }
  if (in_ECX != 0) {
    local_40 = 0;
    puVar6 = local_3e;
    for (iVar4 = 0xf; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    *(undefined2 *)puVar6 = 0;
    FUN_006174d0();
    FUN_006174d0();
    uVar1 = FUN_00617490();
    FUN_00617490();
    uVar2 = FUN_00617490();
    iVar4 = FUN_006174d0();
    iVar3 = FUN_00617c10();
    uVar5 = 0x10;
    if (iVar3 < -1) {
      local_54 = 0xffffffff;
    }
    else {
      iVar3 = FUN_00617c10();
      if (iVar3 < 0x11) {
        local_54 = FUN_00617c10();
      }
      else {
        local_54 = 0x10;
      }
    }
    iVar3 = FUN_00617c10();
    if (iVar3 < -1) {
      uVar5 = 0xffffffff;
    }
    else {
      iVar3 = FUN_00617c10();
      if (iVar3 < 0x11) {
        uVar5 = FUN_00617c10();
      }
    }
    iVar3 = FUN_00617aa0();
    if (iVar3 < 0) {
      uVar7 = 0;
    }
    else {
      iVar3 = FUN_00617aa0();
      if (iVar3 < 10000) {
        uVar7 = FUN_00617aa0();
      }
      else {
        uVar7 = 9999;
      }
    }
    map_list_get_friendly_level_name(&local_40);
    FUN_004b67e0(uVar1,&local_40,uVar2,'\x01' - (iVar4 != 1),local_54,uVar5,uVar7);
    *(undefined1 *)(param_1 + 0x12) = 0;
    return;
  }
  FUN_004b67e0(&DAT_0065512c,&DAT_00660c34,&DAT_0065512c,0,0xffffffff,0xffffffff,0xffffffff);
  *(undefined1 *)(param_1 + 0x12) = 1;
  return;
}
#endif
