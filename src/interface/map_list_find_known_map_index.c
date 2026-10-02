// map_list_find_known_map_index  (Ghidra: map_list_find_known_map_index, already named)
// address 0x494ff0, size 195 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/interface_functions.md "Looks up whether a given map path matches one of
// the engine's known/built-in multiplayer maps, returning its index or -1 for a custom map.";
// types/interface.h map_list_entry (path stored lowercased by map_list_add_entry).
// register convention: map path in EAX (in_EAX). // blam-cc: EAX -> map_path
// UNSURE: Ghidra's decompile repeatedly stores the loop cursor back into DAT_00712dcc (map_list)
// while lowercasing the local path copy, then reads it back unchanged on the next iteration --
// map_list is never actually mutated by that loop (the value written is always the value just
// read), so this is a dead self-store artifact of register spilling and is dropped here.
// UNSURE: the byte-pair-unrolled manual comparison loop against each map_list_entry::path is
// reproduced as an ordinary strcmp, which both entries being already-lowercased NUL-terminated
// strings makes exactly equivalent.
// UNSURE: the lowercase loop's condition also repeatedly writes `local_104[0] = *pbVar3` (the
// next character to process). Read literally this would shift/corrupt the buffer, which cannot
// be right for shipped map-matching code; it is far more consistent with a single scalar
// loop-condition variable (`c = *p`) that the compiler happened to place at the same stack
// address Ghidra attributes to local_104[0] (a common stack-slot-reuse artifact), so it is
// rewritten as the plain "lowercase each byte in place, stop at NUL" loop that reading gives.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern map_list_entry *map_list; // 0x00712dcc
extern int32_t map_list_count;   // 0x00712dd0


// blam-cc: EAX -> map_path
// Lowercases a local copy of `map_path` and linear-scans map_list for an entry whose path
// matches it exactly, returning that entry's index, or -1 if none match (or the list is empty).
int32_t map_list_find_known_map_index(char *map_path)
{
    char local_path[0x103];
    char *cursor;
    int32_t index;

    strncpy(local_path, map_path, 0x103);
    for (cursor = local_path; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((uint8_t)*cursor);
    }

    if (map_list_count < 1) {
        return -1;
    }

    for (index = 0; index < map_list_count; index++) {
        if (strcmp(map_list[index].path, local_path) == 0) {
            return index;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x494ff0):

int map_list_find_known_map_index(void)

{
  byte bVar1;
  char *in_EAX;
  int iVar2;
  byte *pbVar3;
  int iVar4;
  undefined4 *puVar5;
  byte *pbVar6;
  bool bVar7;
  byte local_104 [259];
  undefined1 local_1;

  _strncpy((char *)local_104,in_EAX,0x103);
  local_1 = 0;
  pbVar6 = local_104;
  puVar5 = DAT_00712dcc;
  while (local_104[0] != 0) {
    DAT_00712dcc = puVar5;
    iVar2 = _tolower((uint)*pbVar6);
    *pbVar6 = (byte)iVar2;
    pbVar3 = pbVar6 + 1;
    pbVar6 = pbVar6 + 1;
    puVar5 = DAT_00712dcc;
    local_104[0] = *pbVar3;
  }
  DAT_00712dcc = puVar5;
  iVar2 = 0;
  if (DAT_00712dd0 < 1) {
    return -1;
  }
  do {
    pbVar3 = (byte *)*puVar5;
    pbVar6 = local_104;
    do {
      bVar1 = *pbVar3;
      bVar7 = bVar1 < *pbVar6;
      if (bVar1 != *pbVar6) {
LAB_0049507b:
        iVar4 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
        goto LAB_00495080;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar3[1];
      bVar7 = bVar1 < pbVar6[1];
      if (bVar1 != pbVar6[1]) goto LAB_0049507b;
      pbVar3 = pbVar3 + 2;
      pbVar6 = pbVar6 + 2;
    } while (bVar1 != 0);
    iVar4 = 0;
LAB_00495080:
    if (iVar4 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
    puVar5 = puVar5 + 3;
    if (DAT_00712dd0 <= iVar2) {
      return -1;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
