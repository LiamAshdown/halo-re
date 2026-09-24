// select_players_to_display  (Ghidra: select_players_to_display, already named)
// address 0x45d4a0, size 453 bytes
// name confidence: 0.85   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Filters and reorders the sorted scoreboard list down
// to just the local split-screen players for HUD display"); CEA-matched name (strings
// "player_count=%d, maxcount=%d", "found local player"); types/game.h scoreboard_entry (0x1c
// bytes), player (local_player_index +0x02).
// register convention: max display count in EBX (unaff_EBX).
//   // blam-cc: EBX -> max_count, stack -> out
// Algorithm (once max_count < total sorted count, i.e. the list doesn't fit): scan the entries
// past the visible [0, max_count) window for ones whose player is a local (split-screen) player,
// stash up to a handful of them, then for each one found, walk backward from the end of the
// visible window past any entry that is *already* a local player, evict the first non-local
// entry it finds by shifting everything after it down one slot, and drop the missing local
// player into the freed last slot. The final copy always truncates to at most max_count entries.
// UNSURE: `local_230`'s 21-dword size (3 scoreboard_entry slots) bounds how many *extra* local
// players beyond the visible window can be rescued this way; not derived from any named
// constant. debug_print_enabled/console_printf_verbose (a printf-style debug logger) are UNSURE identities.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

extern data_array *player_data; // 0x0087a480
extern uint8_t debug_print_enabled_flag; // 0x00689412, compared against 0x45 ('E'?); UNSURE identity

extern int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat,
    scoreboard_entry entries[16], int32_t mode); // 0x45cc90; invert_low_stat travels in AL
extern void console_printf_verbose(const char *format, ...); // 0x496a80, not in this batch; UNSURE exact identity
// memmove (0x006236f0 _memmove) comes from <string.h>.

// blam-cc: EBX -> max_count, stack -> out
// Writes at most `max_count` scoreboard_entry records to `out`, taken from the sorted scoreboard
// list, but guaranteed to include every local (split-screen) player even if their rank would
// otherwise put them past the cutoff.
int32_t select_players_to_display(int32_t mode, int32_t max_count, scoreboard_entry *out)
    // blam-cc: EAX -> mode, EBX -> max_count, stack -> out
{
    scoreboard_entry entries[16];
    int32_t total = game_engine_build_sorted_player_list(0, entries, mode); // xor al,al at 0x45d4b1
    uint8_t debug = (debug_print_enabled_flag == 0x45);

    if (debug) {
        console_printf_verbose("player_count=%d, maxcount=%d", total);
    }

    if (max_count < total) {
        scoreboard_entry rescued[3]; // matches Ghidra's local_230[21] == 3 * 0x1c
        int32_t rescued_count = 0;
        int32_t remaining_to_scan = 0;

        {
            scoreboard_entry *scan = &entries[max_count];
            int32_t remaining = total - max_count;

            do {
                remaining_to_scan = remaining;
                player *candidate = (player *)((uint8_t *)player_data->data +
                    (scan->player & 0xffff) * sizeof(player));

                if (candidate != 0 && candidate->local_player_index != -1) {
                    if (debug) {
                        console_printf_verbose("found local player");
                    }
                    rescued[rescued_count] = *scan;
                    rescued_count = rescued_count + 1;
                }
                scan = scan + 1;
                remaining = remaining_to_scan - 1;
                remaining_to_scan = rescued_count;
            } while (remaining != 0);
        }

        if (0 < remaining_to_scan) {
            scoreboard_entry *rescue_src = rescued;
            do {
                int32_t slot = max_count - 1;

                if (-1 < slot) {
                    scoreboard_entry *victim = &entries[slot];

                    while (((player *)((uint8_t *)player_data->data +
                                (victim->player & 0xffff) * sizeof(player)))->local_player_index != -1) {
                        slot = slot - 1;
                        victim = victim - 1;
                        if (slot < 0) {
                            goto next_rescue;
                        }
                    }

                    memmove(&entries[slot], &entries[slot + 1],
                        (size_t)((max_count - slot) * sizeof(scoreboard_entry) - sizeof(scoreboard_entry)));
                    entries[max_count - 1] = *rescue_src;
                }

            next_rescue:
                rescue_src = rescue_src + 1;
                remaining_to_scan = remaining_to_scan - 1;
            } while (remaining_to_scan != 0);
        }
    }

    if (max_count <= total) {
        total = max_count;
    }
    {
        int32_t i;
        for (i = 0; i < total; i++) {
            out[i] = entries[i];
        }
    }
}

#if 0
Original Ghidra decompilation (0x45d4a0), from tools/pack.py 0x45d4a0:

void select_players_to_display(uint *param_1)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  uint uVar4;
  int unaff_EBX;
  uint *puVar5;
  uint *puVar6;
  bool bVar7;
  int local_240;
  int local_238;
  uint *local_234;
  uint local_230 [21];
  uint auStack_1dc [7];
  uint local_1c0 [7];
  undefined1 auStack_1a4 [420];

  iVar1 = game_engine_build_sorted_player_list(local_1c0);
  bVar7 = DAT_00689412 == 0x45;
  if (bVar7) {
    FUN_00496a80("player_count=%d, maxcount=%d",iVar1);
  }
  if (unaff_EBX < iVar1) {
    local_238 = 0;
    local_240 = 0;
    if (unaff_EBX < iVar1) {
      puVar5 = local_1c0 + unaff_EBX * 7;
      local_234 = local_230;
      iVar2 = iVar1 - unaff_EBX;
      do {
        local_240 = iVar2;
        iVar2 = (*puVar5 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
        if ((iVar2 != 0) && (*(short *)(iVar2 + 2) != -1)) {
          if (bVar7) {
            FUN_00496a80("found local player");
          }
          puVar3 = puVar5;
          puVar6 = local_234;
          for (iVar2 = 7; iVar2 != 0; iVar2 = iVar2 + -1) {
            *puVar6 = *puVar3;
            puVar3 = puVar3 + 1;
            puVar6 = puVar6 + 1;
          }
          local_238 = local_238 + 1;
          local_234 = local_234 + 7;
        }
        puVar5 = puVar5 + 7;
        iVar2 = local_240 + -1;
        local_240 = local_238;
      } while (iVar2 != 0);
    }
    if (0 < local_240) {
      puVar5 = local_230;
      do {
        iVar2 = unaff_EBX + -1;
        if (-1 < iVar2) {
          puVar3 = local_1c0 + iVar2 * 7;
LAB_0045d5c0:
          if (*(short *)((*puVar3 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1)
          goto code_r0x0045d5d3;
          _memmove(local_1c0 + iVar2 * 7,auStack_1a4 + iVar2 * 0x1c,
                   (unaff_EBX - iVar2) * 0x1c - 0x1c);
          puVar3 = puVar5;
          puVar6 = auStack_1dc + unaff_EBX * 7;
          for (iVar2 = 7; iVar2 != 0; iVar2 = iVar2 + -1) {
            *puVar6 = *puVar3;
            puVar3 = puVar3 + 1;
            puVar6 = puVar6 + 1;
          }
        }
LAB_0045d61b:
        puVar5 = puVar5 + 7;
        local_240 = local_240 + -1;
      } while (local_240 != 0);
    }
  }
  if (unaff_EBX <= iVar1) {
    iVar1 = unaff_EBX;
  }
  puVar5 = local_1c0;
  for (uVar4 = (uint)(iVar1 * 0x1c) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *param_1 = *puVar5;
    puVar5 = puVar5 + 1;
    param_1 = param_1 + 1;
  }
  for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(char *)param_1 = (char)*puVar5;
    puVar5 = (uint *)((int)puVar5 + 1);
    param_1 = (uint *)((int)param_1 + 1);
  }
  return;
code_r0x0045d5d3:
  iVar2 = iVar2 + -1;
  puVar3 = puVar3 + -7;
  if (iVar2 < 0) goto LAB_0045d61b;
  goto LAB_0045d5c0;
}
#endif
