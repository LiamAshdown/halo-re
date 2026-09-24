// game_engine_get_player_scoreboard_entry  (Ghidra: FUN_0045cee0; named for what it does)
// address 0x45cee0, size 78 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Looks up and copies out a single player's scoreboard
// entry from the sorted scoreboard list"); types/game.h scoreboard_entry (0x1c bytes, up to 16
// via game_engine_build_sorted_player_list).
// register convention: player handle in EAX (in_EAX); output scoreboard_entry * in EBX
// (unaff_EBX).
//   // blam-cc: EAX -> player, EBX -> out
// UNSURE: game_engine_build_sorted_player_list is called here with only one visible argument
// (the entries array); its own decompile takes a second `mode` parameter used to pick the
// comparator. Preserved as mode 0 (free-for-all ordering), matching the only other call site in
// this batch (game_engine_get_scoreboard_place / select_players_to_display) that passes it
// explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat,
    scoreboard_entry entries[16], int32_t mode); // 0x45cc90; invert_low_stat travels in AL

// blam-cc: EAX -> player, EBX -> out
// Rebuilds the sorted scoreboard list and copies the entry belonging to `player` into `*out`.
// If `player` isn't found in the list (shouldn't normally happen), copies whichever entry the
// scan stopped on, exactly as the original does.
void game_engine_get_player_scoreboard_entry(datum_index player, scoreboard_entry *out)
{
    scoreboard_entry entries[16];
    int i;

    game_engine_build_sorted_player_list(0, entries, 0); // xor al,al / xor esi,esi at 0x45cee2

    i = 0;
    if (entries[0].player != player) {
        do {
            i = i + 1;
        } while (entries[i].player != player);
    }

    *out = entries[i];
}

#if 0
Original Ghidra decompilation (0x45cee0), from tools/pack.py 0x45cee0:

void FUN_0045cee0(void)

{
  int *piVar1;
  int in_EAX;
  int *piVar2;
  int iVar3;
  int *unaff_EBX;
  int iVar4;
  int local_1c0 [112];

  iVar4 = 0;
  game_engine_build_sorted_player_list(local_1c0,0);
  if (local_1c0[0] != in_EAX) {
    piVar2 = local_1c0;
    do {
      piVar1 = piVar2 + 7;
      piVar2 = piVar2 + 7;
      iVar4 = iVar4 + 1;
    } while (*piVar1 != in_EAX);
  }
  piVar2 = local_1c0 + iVar4 * 7;
  for (iVar3 = 7; iVar3 != 0; iVar3 = iVar3 + -1) {
    *unaff_EBX = *piVar2;
    piVar2 = piVar2 + 1;
    unaff_EBX = unaff_EBX + 1;
  }
  return;
}
#endif
