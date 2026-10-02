// game_state_read_persistent_storage  (Ghidra: game_state_read_persistent_storage, already named)
// address 0x539330, size 148 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md; straightforward: waits for any in-progress
// write, then reads game_state_size bytes from the already-open persistent storage handle into
// game_state_snapshot_source (== map_memory), raising a fatal error dialog on failure.
// Phase 4 review: matched objdump 0x539330..0x5393c5; the result is a bool in AL, now uint8_t.
// register convention: no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t game_state_write_in_progress; // 0x006e3000
extern void *game_state_persistent_storage; // 0x006e2df8
extern uint32_t game_state_size; // 0x006e2df0
extern uint8_t *game_state_snapshot_source; // 0x006e2dec

extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

uint8_t game_state_read_persistent_storage(void)
{
    uint32_t bytes_read;

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    if (SetFilePointer(game_state_persistent_storage, 0, 0, 0) != 0xffffffff &&
        ReadFile(game_state_persistent_storage, game_state_snapshot_source, game_state_size, (LPDWORD)&bytes_read, 0) != 0 &&
        bytes_read == game_state_size) {
        return 1;
    }
    shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
    return 0;
}

#if 0
Original Ghidra decompilation (0x539330):

undefined4 game_state_read_persistent_storage(void)

{
  DWORD DVar1;
  BOOL BVar2;
  DWORD local_4;

  if (DAT_006e3000 != '\0') {
    while (DAT_006e3000 != '\0') {
      Sleep(0);
    }
  }
  DVar1 = SetFilePointer(DAT_006e2df8,0,(PLONG)0x0,0);
  if (((DVar1 != 0xffffffff) &&
      (BVar2 = ReadFile(DAT_006e2df8,DAT_006e2dec,DAT_006e2df0,&local_4,(LPOVERLAPPED)0x0),
      BVar2 != 0)) && (local_4 == DAT_006e2df0)) {
    return 1;
  }
  shell_display_fatal_error_dialog(0x8b,0x8c,1);
  return 0;
}
#endif
