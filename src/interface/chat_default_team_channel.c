// chat_default_team_channel  (Ghidra: FUN_004ab1e0, renamed)
// address 0x4ab1e0, size 87 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.3
// evidence: phase-4 summary "Returns the team index of the first active player found, used as
// the default chat channel"; types/game.h player::local_player_index (offset 0x02, "-1 unless
// this player is driven locally") and player::team_index_desired (offset 0x67).
// UNSURE: no iterator setup is visible in the decompilation (Ghidra shows the loop starting
// cold at `data_iterator_next()`); player_data is used here as the scanned array since the two
// fields read (local_player_index, team_index_desired) both match the player struct exactly.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern data_array *player_data; // 0x0087a480, UNSURE: iterated array, see header note
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// Scans player_data for the first locally-driven player and returns its desired team index, or
// -1 if none is found.
int32_t chat_default_team_channel(void)
{
    data_iterator iterator;
    player *entry;

    iterator.data = player_data; // UNSURE: guessed iterated array, see header note
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;

    entry = (player *)data_iterator_next(&iterator);
    while (entry != 0) {
        if (entry->local_player_index != -1) {
            return entry->team_index_desired;
        }
        entry = (player *)data_iterator_next(&iterator);
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4ab1e0):

int FUN_004ab1e0(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return -1;
    }
    if (*(short *)(iVar1 + 2) != -1) break;
    iVar1 = data_iterator_next();
  }
  return (int)*(char *)(iVar1 + 0x67);
}
#endif
