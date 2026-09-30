// game_engine_get_scoreboard_place  (Ghidra: FUN_0045d440; named for what it does)
// address 0x45d440, size 84 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Computes a player's rank/placement number within the
// sorted scoreboard list, accounting for tied entries"); types/game.h scoreboard_entry.
// register convention: player handle in EDI (unaff_EDI).
//   // blam-cc: EDI -> player, EAX -> mode, stack -> invert_low_stat
// UNSURE: the tie test below reads scoreboard_entry::unknown_04 (dword offset 0x04) between
// consecutive entries, not the `place` field's own tie bit that game_engine_build_sorted_player_
// list (0x45cc90, not in this batch) sets. The pointer arithmetic pins this exactly (local_1c0[8]
// aligns to entries[1].unknown_04, and local_1c0[8-1] to entries[1].player, which the loop does
// test against the target handle) -- it is not a misreading of the offsets. 0x45cc90's own
// decompile is itself corrupted by a Ghidra jump-table recovery failure ("Could not recover
// jumptable... treating indirect jump as call") in exactly the branch that would write this
// field, so unknown_04 most likely holds a rank/tier value that function computes on a path
// Ghidra could not decompile. Transcribed here exactly as this function reads it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat,
    scoreboard_entry entries[16], int32_t mode); // 0x45cc90; invert_low_stat travels in AL

// blam-cc: EDI -> player, EAX -> mode, stack -> invert_low_stat
// Returns `player`'s 0-based on-screen rank in the sorted scoreboard list: entries sharing the
// same unknown_04 group value as the previous entry share the previous entry's rank.
int32_t game_engine_get_scoreboard_place(datum_index player, int32_t mode, uint8_t invert_low_stat)
{
    scoreboard_entry entries[16];
    int count;
    int place;
    int i;

    count = game_engine_build_sorted_player_list(invert_low_stat, entries, mode);
    place = 0;

    if (entries[0].player != player && 1 < count) {
        for (i = 1; i < count; i++) {
            if (entries[i - 1].single_sort_key != entries[i].single_sort_key) {
                place = place + 1;
            }
            if (entries[i].player == player) {
                return place;
            }
        }
    }

    return place;
}

#if 0
Original Ghidra decompilation (0x45d440), from tools/pack.py 0x45d440:

int FUN_0045d440(void)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int unaff_EDI;
  int local_1c0 [8];
  int local_1a0 [104];

  iVar1 = game_engine_build_sorted_player_list(local_1c0);
  iVar2 = 0;
  if ((local_1c0[0] != unaff_EDI) && (iVar4 = 1, 1 < iVar1)) {
    piVar3 = local_1a0;
    do {
      if (piVar3[-7] != *piVar3) {
        iVar2 = iVar2 + 1;
      }
      if (piVar3[-1] == unaff_EDI) {
        return iVar2;
      }
      iVar4 = iVar4 + 1;
      piVar3 = piVar3 + 7;
    } while (iVar4 < iVar1);
  }
  return iVar2;
}
#endif
