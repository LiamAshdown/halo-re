// network_banlist_print  (Ghidra: FUN_004e34e0; renamed -- console command that lists bans)
// address 0x4e34e0, size 210 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md ("Console command that lists the currently
// banned players with their ban counts"); the literal header string "[Num Bans Name]" and row
// format "[%-4d %2d %*s]" (index, ban count, 12-wide name); types/networking.h ban_list_entry.
// register convention: __cdecl, no arguments.
// UNSURE: the destination line buffer is built one byte past its start in the original
// decompile (acStack_101[1] = 0, then every append walks from acStack_101 to find that NUL and
// the whole buffer is printed from +1 on), which only matters because byte 0 is left
// uninitialized and never printed; this rewrite uses an ordinary NUL-terminated-at-0 buffer,
// which produces byte-for-byte identical console output.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
#include <string.h>

extern growable_array ban_list; // 0x006b859c, element_size 0x38; see network_banlist_save.c

extern void *console_color_00685214; // 0x00685214, a ColorARGB * the original loads into EAX
extern void *actor_mode_default_look_weights; // 0x00686af8, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Prints "[Num Bans Name]" followed by rows of up to three "[index ban_count name]" entries
// each, until every ban-list entry has been listed.
void network_banlist_print(void)
{
    ban_list_entry *entries = (ban_list_entry *)ban_list.data;
    int32_t i = 0;
    char entry_text[63];
    char line[257];

    chimera__console_out((ColorARGB *)console_color_00685214, "[Num Bans Name]");
    if (0 < ban_list.count) {
        do {
            int32_t in_line = 0;
            line[0] = 0;
            do {
                if (ban_list.count <= i) {
                    break;
                }
                sprintf(entry_text, "[%-4d %2d %*s]", i, entries[i].ban_count, 0xc, entries[i].name);
                strcat(line, entry_text);
                in_line = in_line + 1;
                i = i + 1;
            } while (in_line < 3);
            chimera__console_out((ColorARGB *)actor_mode_default_look_weights, line);
        } while (i < ban_list.count);
    }
}

#if 0
Original Ghidra decompilation (0x4e34e0), from tools/pack.py 0x4e34e0:

void FUN_004e34e0(void)

{
  char cVar1;
  char *pcVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  char *pcVar7;
  int local_144;
  char local_140 [63];
  char acStack_101 [257];

  iVar6 = 0;
  chimera__console_out("[Num Bans Name]");
  if (0 < DAT_006b85a0) {
    do {
      acStack_101[1] = 0;
      iVar5 = iVar6 * 0x38;
      local_144 = 0;
      do {
        if (DAT_006b85a0 <= iVar6) break;
        _sprintf(local_140,"[%-4d %2d %*s]",iVar6,(int)*(short *)(DAT_006b85a4 + 0x2e + iVar5),0xc,
                 DAT_006b85a4 + iVar5);
        pcVar2 = local_140;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        uVar3 = (int)pcVar2 - (int)local_140;
        pcVar2 = acStack_101;
        do {
          pcVar7 = pcVar2 + 1;
          pcVar2 = pcVar2 + 1;
        } while (*pcVar7 != '\0');
        pcVar7 = local_140;
        for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
          *(undefined4 *)pcVar2 = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          pcVar2 = pcVar2 + 4;
        }
        local_144 = local_144 + 1;
        iVar6 = iVar6 + 1;
        iVar5 = iVar5 + 0x38;
        for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
          *pcVar2 = *pcVar7;
          pcVar7 = pcVar7 + 1;
          pcVar2 = pcVar2 + 1;
        }
      } while (local_144 < 3);
      chimera__console_out(acStack_101 + 1);
    } while (iVar6 < DAT_006b85a0);
  }
  return;
}
#endif
