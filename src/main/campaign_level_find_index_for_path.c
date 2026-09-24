// campaign_level_find_index_for_path  (Ghidra: FUN_004c8b90; still unnamed -> renamed)
// address 0x4c8b90, size 428 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/main_types_notes.md: "FUN_004c8b90: the summary says nine entries; the
// table has ten (a10 .. d40) and the function returns 0..9 or -1", and "campaign_level_short_
// names[10][4] (.rdata 0x00669a38, d40 first, a10 last; FUN_004c8b90 tests them in reverse order
// with strstr)". The ten literals at 0x00669a38..0x00669a5c match k_main_campaign_level_count
// (types/main.h). Testing starts at 0x00669a5c ("a10", the LAST table entry) and returns 0, then
// walks backward to 0x00669a38 ("d40", the FIRST table entry) returning 9 -- i.e. the return
// value is the campaign order index into campaign_level_paths[10] (main.h: "levels a10 .. d40
// scenario paths", ascending), not the table's own storage order.
// register convention: destination buffer as the recognized parameter (param_1 in Ghidra's own
// phase 4 review (disassembly 0x4c8b90..0x4c8d3c: a10 -> 0 .. d40 -> 9 from the 0x00669a38 table read backwards; no drift.
// signature, i.e. an ordinary cdecl stack/register parameter -- Ghidra fully recognized it).
// UNSURE: FUN_00625430 is declared strcmp-shaped (returns 0 on inequality) by unrelated callers
// in other modules (src/game/cheat_spawn_warthog.c), but main.h's own analysis of THIS function
// specifically identifies it as strstr (substring search, non-NULL on a match); that reading is
// used here since it is the only one under which "if (result != 0) return <index>" makes sense
// for a short level-code lookup inside a full map path.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>
#include <ctype.h>

extern char campaign_level_short_names[k_main_campaign_level_count][4]; // .rdata 0x00669a38,
    // d40 first, a10 last (types/main.h)

// Lowercases a copy of `path` and checks it against each of the ten campaign level short codes
// ("a10".."d40"), starting from the last table entry ("a10") and working backward to the first
// ("d40"), returning the matching campaign order index (0 for "a10" through 9 for "d40"), or -1
// if none of them appear in the path.
int campaign_level_find_index_for_path(char *path)
{
    char lower_path[128];
    char *c;
    int i;

    strncpy(lower_path, path, 0x7f);
    lower_path[0x7f] = 0;
    for (c = lower_path; *c != 0; c++) {
        *c = (char)tolower((uint8_t)*c);
    }

    for (i = k_main_campaign_level_count - 1; i >= 0; i--) {
        if (strstr(lower_path, campaign_level_short_names[i]) != 0) {
            return (k_main_campaign_level_count - 1) - i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4c8b90):

int FUN_004c8b90(char *param_1)

{
  byte *pbVar1;
  byte bVar2;
  int iVar3;
  byte *pbVar4;
  undefined4 *puVar5;
  byte local_88;
  undefined4 local_87;
  undefined1 local_9;

  local_88 = 0;
  puVar5 = &local_87;
  for (iVar3 = 0x1f; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *(undefined2 *)puVar5 = 0;
  *(undefined1 *)((int)puVar5 + 2) = 0;
  _strncpy((char *)&local_88,param_1,0x7f);
  local_9 = 0;
  pbVar4 = &local_88;
  bVar2 = local_88;
  while (bVar2 != 0) {
    iVar3 = _tolower((uint)*pbVar4);
    *pbVar4 = (byte)iVar3;
    pbVar1 = pbVar4 + 1;
    pbVar4 = pbVar4 + 1;
    bVar2 = *pbVar1;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a5c);
  if (iVar3 != 0) {
    return 0;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a58);
  if (iVar3 != 0) {
    return 1;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a54);
  if (iVar3 != 0) {
    return 2;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a50);
  if (iVar3 != 0) {
    return 3;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a4c);
  if (iVar3 != 0) {
    return 4;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a48);
  if (iVar3 != 0) {
    return 5;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a44);
  if (iVar3 != 0) {
    return 6;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a40);
  if (iVar3 != 0) {
    return 7;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a3c);
  if (iVar3 != 0) {
    return 8;
  }
  iVar3 = FUN_00625430(&local_88,&DAT_00669a38);
  return (-(uint)(iVar3 != 0) & 10) - 1;
}
#endif
