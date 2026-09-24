// players_dispose  (Ghidra: players_dispose, already named)
// address 0x473670, size 95 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Resets the players and teams datum arrays and
//   clears the local-player mapping table"); types/game.h player_globals field list, which
//   is derived from exactly this function ("players_dispose zeroes all 0x26 dwords and then
//   re-seeds +0x00/+0x04/+0x08 = -1, +0x0e = 0, +0x10 = 0, +0x11 = 0, +0x12 = -1, +0x14 = 0");
//   src/memory/data_delete_all.c and its established `array->valid = 1; data_delete_all(array)`
//   call shape (matched by src/objects/objects_reset.c, src/objects/antennas_dispose.c, etc.).
// register convention: no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

extern data_array *player_data;               // 0x0087a480, "players"
extern data_array *team_data;                 // 0x0087a47c, "teams"
extern player_globals *local_player_globals;  // 0x0087a478
extern datum_index machine_to_player[16];     // 0x006b1460

extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array (memory module)

// Zeroes player_globals, re-seeds its datum-index fields to the wildcard and its two counters
// to zero, marks both the "players" and "teams" data arrays valid and deletes every live datum
// out of them, then clears the whole machine-to-player mapping table back to the wildcard.
void players_dispose(void)
{
    int32_t i;

    memset(local_player_globals, 0, sizeof(player_globals));
    local_player_globals->local_players[0] = (datum_index)-1;
    local_player_globals->local_player_units[0] = (datum_index)-1;
    local_player_globals->unknown_00 = (datum_index)-1;
    local_player_globals->unknown_11 = 0;
    local_player_globals->respawn_stagger = 0;
    local_player_globals->no_player_has_a_unit = 0;
    local_player_globals->unknown_12 = -1;
    local_player_globals->mode = 0;

    player_data->valid = 1;
    data_delete_all(player_data);
    team_data->valid = 1;
    data_delete_all(team_data);

    for (i = 0; i < 16; i = i + 1) {
        machine_to_player[i] = (datum_index)-1;
    }
}

#if 0
Original Ghidra decompilation (0x473670), from tools/pack.py 0x473670:

void players_dispose(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;

  iVar2 = DAT_0087a480;
  puVar4 = DAT_0087a478;
  puVar3 = DAT_0087a478;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar4[1] = 0xffffffff;
  puVar4[2] = 0xffffffff;
  *puVar4 = 0xffffffff;
  *(undefined1 *)((int)puVar4 + 0x11) = 0;
  *(undefined2 *)((int)puVar4 + 0xe) = 0;
  *(undefined1 *)(puVar4 + 4) = 0;
  *(undefined2 *)((int)puVar4 + 0x12) = 0xffff;
  *(undefined2 *)(puVar4 + 5) = 0;
  *(undefined1 *)(iVar2 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087a47c + 0x24) = 1;
  data_delete_all();
  puVar4 = &DAT_006b1460;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0xffffffff;
    puVar4 = puVar4 + 1;
  }
  return;
}
#endif
