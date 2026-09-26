// players_initialize  (Ghidra: players_initialize, already named)
// address 0x4735b0, size 181 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Allocates and CRC-registers the players and teams
//   datum arrays and their associated globals blocks"); types/game.h header comment, which
//   derives the two data_array element sizes (k_player_size 0x200, k_team_size 0x40) and the
//   two raw allocation sizes (k_player_globals_size 0x98, k_player_control_globals_size 0x50)
//   straight from objdump -d -M intel --start-address=0x4735b0 --stop-address=0x473670
//   bin/halo.exe -- Ghidra's own decompile drops the "mov ebx,<size>" ahead of every
//   game_state_new call, the same way it drops it ahead of data_new. The disassembly also
//   confirms the three field writes into player_globals (local_players[0], unknown_00,
//   local_player_count) and that player_control_globals gets no field writes here (its fields are
//   filled in by the functions that read them, e.g. game_engine_reset_player_look_state).
// register convention: no arguments.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t game_state_cursor; // 0x006e2dcc
extern uint8_t *game_state_base;   // 0x006e2dc8
extern uint32_t game_state_crc;   // 0x006e2dd4

extern data_array *player_data;                            // 0x0087a480, "players", 16 x 0x200
extern data_array *team_data;                              // 0x0087a47c, "teams", 16 x 0x40
extern player_globals *local_player_globals;                // 0x0087a478, 0x98 of game state
extern player_control_globals *player_control_globals_ptr;  // 0x006b145c, 0x50 of game state

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
    // 0x5380d0, blam-cc: EBX -> element_size, then the stack pair (name, maximum_count).
    // Builds a full data_array header (name, maximum_count, element size, 'd@t@' signature,
    // data pointer) out of the game-state arena, exactly like data_new but bump-allocating
    // from game_state_base/game_state_cursor instead of the heap.
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module

// Allocates the 16-entry "players" and "teams" data arrays, then bump-allocates
// player_globals (0x98 bytes) and player_control_globals (0x50 bytes) out of the game-state
// arena, CRC-registering each allocation's size. Only player_globals gets explicit field
// writes here: local_players[0] and unknown_00 are seeded to the datum-index wildcard and
// local_player_count is cleared.
void players_initialize(void)
{
    uint32_t size;

    player_data = (data_array *)game_state_new("players", k_maximum_players, k_player_size);
    team_data = (data_array *)game_state_new("teams", k_maximum_teams, k_team_size);

    local_player_globals = (player_globals *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + k_player_globals_size;
    size = k_player_globals_size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    local_player_globals->local_players[0] = (datum_index)-1;
    local_player_globals->unknown_00 = (datum_index)-1;
    local_player_globals->local_player_count = 0;

    player_control_globals_ptr = (player_control_globals *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + k_player_control_globals_size;
    size = k_player_control_globals_size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
}

#if 0
Original Ghidra decompilation (0x4735b0), from tools/pack.py 0x4735b0:

void players_initialize(void)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 local_4;

  DAT_0087a480 = game_state_new("players",0x10);
  DAT_0087a47c = game_state_new("teams",0x10);
  puVar1 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 0x98;
  local_4 = 0x98;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  puVar1[1] = 0xffffffff;
  *puVar1 = 0xffffffff;
  *(undefined2 *)(puVar1 + 3) = 0;
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x50;
  local_4 = 0x50;
  DAT_0087a478 = puVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b145c = iVar2;
  return;
}
#endif
