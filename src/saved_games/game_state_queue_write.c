// game_state_queue_write  (Ghidra: game_state_queue_write, already named)
// address 0x538700, size 105 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/saved_games_functions.md; the dword/byte copy loop is an inlined memcpy
// of game_state_size bytes from game_state_snapshot_source to game_state_write_buffer; the
// __cdecl signature and final_flag parameter are Ghidra-recognized.
// register convention: __cdecl (Ghidra-recognized); final_flag is the recognized parameter.
// Returns a bool in AL only (mov al,1 at 0x538763); the caller tests AL (0x5381d7).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern uint8_t game_state_write_in_progress; // 0x006e3000
extern uint32_t game_state_size; // 0x006e2df0
extern uint8_t *game_state_snapshot_source; // 0x006e2dec
extern uint8_t *game_state_write_buffer; // 0x006e2de4
extern void *game_state_write_event; // 0x006e2ffc
extern uint8_t game_state_write_is_checkpoint; // 0x006e3001

extern void *memcpy(void *dest, const void *src, uint32_t count);

uint8_t game_state_queue_write(uint8_t final_flag)
{
    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    memcpy(game_state_write_buffer, game_state_snapshot_source, game_state_size);

    game_state_write_is_checkpoint = final_flag;
    SetEvent(game_state_write_event);
    return 1;
}

#if 0
Original Ghidra decompilation (0x538700):

int __cdecl game_state_queue_write(char final_flag)

{
  BOOL BVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;

  if (DAT_006e3000 != '\0') {
    while (DAT_006e3000 != '\0') {
      Sleep(0);
    }
  }
  uVar3 = DAT_006e2df0;
  puVar4 = DAT_006e2dec;
  puVar5 = DAT_006e2de4;
  for (uVar2 = DAT_006e2df0 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
    puVar4 = (undefined4 *)((int)puVar4 + 1);
    puVar5 = (undefined4 *)((int)puVar5 + 1);
  }
  DAT_006e3001 = final_flag;
  BVar1 = SetEvent(DAT_006e2ffc);
  return CONCAT31((int3)((uint)BVar1 >> 8),1);
}
#endif
