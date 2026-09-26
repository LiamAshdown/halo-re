// map_list_matching_substring  (Ghidra: map_list_matching_substring, already named)
// address 0x4e4470, size 393 bytes (0x4e4470..0x4e45f8, single `ret`; Ghidra's metadata said 113 and
//   catalogued the rest as the bogus function 0x4e44e1 "client_machine_cleanup__hook_remove_player",
//   which is the middle of the tolower loop below -- re-checked against objdump by orphan pass 4)
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md ("lists installed map names whose lowercased
// name contains an optional substring argument, two per output line"); the literal
// "Maps matching substring \"%s\" :" format string.
// register convention: both parameters arrive on the stack (Ghidra recognizes both, and
// disassembly confirms argument_count is read from a genuine stack slot three pushes deep) --
// the documented exception among this batch's console commands (see sv_maxplayers_evaluate.c:
// "the two *_matching_substring handlers, which pass both on the stack").
//   // blam-cc: stack -> argument_count, stack -> arguments
// UNSURE: the map table's element type (name at +0x00, a validity byte at +0x08) is not
// otherwise attested in this batch, so no struct is declared for it; FUN_00625430 (foreign) is
// read as a case-insensitive substring test from its two string-shaped arguments.
// reconciled: R81 0x00712dcc/0x00712dd0 -> interface.h map_list_entry *map_list / int32_t map_list_count (network_map_list_entry dropped: name -> path, valid -> cache_file_exists)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "interface.h"
#include "networking.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// The element type is types/interface.h map_list_entry (0x0c), the table interface.h calls map_list.

extern int32_t map_list_count; // 0x00712dd0
extern map_list_entry *map_list; // 0x00712dcc, stride 0xc

extern int32_t FUN_00625430(char *map_name, char *filter); // foreign, UNSURE: case-insensitive substring test
extern void *console_color_00685214; // 0x00685214, a ColorARGB * the original loads into EAX
extern void *console_color_00686af8; // 0x00686af8, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: prints every installed map name (lowercased filter substring optional),
// two per output line.
void map_list_matching_substring(uint32_t argument_count, char **arguments) // blam-cc: stack -> argument_count, stack -> arguments
{
    char filter[64];
    int32_t i;

    filter[0] = 0;
    if (0 < (int32_t)argument_count) {
        char *p;
        strncpy(filter, arguments[0], 0x3f);
        filter[0x3f] = 0;
        for (p = filter; *p != 0; p = p + 1) {
            *p = (char)tolower((uint8_t)*p);
        }
    }
    chimera__console_out((ColorARGB *)console_color_00685214, "Maps matching substring \"%s\" :", filter);
    i = 0;
    while (i < map_list_count) {
        char line[256];
        int32_t on_line = 0;

        line[0] = 0;
        while (i < map_list_count && on_line < 2) {
            map_list_entry *entry = &map_list[i];
            if (entry->cache_file_exists != 0 && entry->path != 0 &&
                (filter[0] == 0 || FUN_00625430(entry->path, filter) != 0)) {
                char formatted[64];
                sprintf(formatted, "%-36s ", entry->path);
                strcat(line, formatted);
                on_line = on_line + 1;
            }
            i = i + 1;
        }
        if (line[0] != 0) {
            chimera__console_out((ColorARGB *)console_color_00686af8, line);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e4470), from tools/pack.py 0x4e4470:

void map_list_matching_substring(int param_1,undefined4 *param_2)

{
  byte *pbVar1;
  byte bVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char *pcVar8;
  uint uVar9;
  int iVar10;
  uint uVar11;
  int iVar12;
  byte *pbVar13;
  char *pcVar14;
  undefined4 *puVar15;
  int local_188;
  byte local_180;
  undefined4 local_17f;
  undefined1 local_141;
  char acStack_140 [63];
  char cStack_101;
  char acStack_100 [256];

  iVar5 = DAT_00712dd0;
  local_180 = 0;
  puVar15 = &local_17f;
  for (iVar10 = 0xf; iVar10 != 0; iVar10 = iVar10 + -1) {
    *puVar15 = 0;
    puVar15 = puVar15 + 1;
  }
  *(undefined2 *)puVar15 = 0;
  *(undefined1 *)((int)puVar15 + 2) = 0;
  iVar10 = 0;
  local_188 = 0;
  if (0 < param_1) {
    _strncpy((char *)&local_180,(char *)*param_2,0x3f);
    local_141 = 0;
    pbVar13 = &local_180;
    bVar2 = local_180;
    while (bVar2 != 0) {
      iVar6 = _tolower((uint)*pbVar13);
      *pbVar13 = (byte)iVar6;
      pbVar1 = pbVar13 + 1;
      pbVar13 = pbVar13 + 1;
      bVar2 = *pbVar1;
    }
  }
  chimera__console_out("Maps matching substring \"%s\" :",&local_180);
  if (0 < iVar5) {
    do {
      iVar6 = 0;
      acStack_100[0] = '\0';
      iVar12 = iVar10 * 0xc;
      do {
        if (iVar5 <= iVar10) break;
        if ((((-1 < iVar12) && (iVar10 < DAT_00712dd0)) &&
            (*(char *)(DAT_00712dcc + 8 + iVar12) != '\0')) &&
           ((iVar4 = *(int *)(DAT_00712dcc + iVar12), iVar4 != 0 &&
            ((local_180 == 0 || (iVar7 = FUN_00625430(iVar4,&local_180), iVar7 != 0)))))) {
          _sprintf(acStack_140,"%-36s ",iVar4);
          pcVar8 = acStack_140;
          do {
            cVar3 = *pcVar8;
            pcVar8 = pcVar8 + 1;
          } while (cVar3 != '\0');
          uVar9 = (int)pcVar8 - (int)acStack_140;
          pcVar8 = &cStack_101;
          do {
            pcVar14 = pcVar8 + 1;
            pcVar8 = pcVar8 + 1;
          } while (*pcVar14 != '\0');
          pcVar14 = acStack_140;
          for (uVar11 = uVar9 >> 2; uVar11 != 0; uVar11 = uVar11 - 1) {
            *(undefined4 *)pcVar8 = *(undefined4 *)pcVar14;
            pcVar14 = pcVar14 + 4;
            pcVar8 = pcVar8 + 4;
          }
          for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
            *pcVar8 = *pcVar14;
            pcVar14 = pcVar14 + 1;
            pcVar8 = pcVar8 + 1;
          }
          iVar6 = iVar6 + 1;
          iVar10 = local_188;
        }
        iVar10 = iVar10 + 1;
        iVar12 = iVar12 + 0xc;
        local_188 = iVar10;
      } while (iVar6 < 2);
      if (acStack_100[0] != '\0') {
        chimera__console_out(acStack_100);
      }
    } while (iVar10 < iVar5);
  }
  return;
}
#endif
