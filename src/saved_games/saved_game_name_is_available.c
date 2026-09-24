// saved_game_name_is_available  (Ghidra: FUN_0053d1e0, renamed; earlier phase-4 name
// saved_game_slot_exists_for_id)
// address 0x53d1e0, size 57 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: objdump 0x53d1e0..0x53d218: EAX (register, no stack arguments) is a wide save-game
// name, tested for non-null and non-empty, then passed unchanged as EAX to XCreateSaveGame mode 3
// (open existing). src/game/XCreateSaveGame.c returns 0 in mode 3 only when the slot directory
// and file already exist, so this returns 1 exactly when the name is NOT in use. That matches
// both users: saved_game_allocate_new_slot stops at the first number whose mode-3 query is
// nonzero (a free name), and src/interface/virtual_keyboard_process_input.c jumps to
// name_taken when this returns 0. Renamed in the phase-4 review (the old name read the result
// backwards).
// register convention: save-game name in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char savegames_directory[0x100]; // 0x00721549

extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size); // 0x551710, blam-cc: EAX save_game_name (src/game/XCreateSaveGame.c: validity_token)

// blam-cc: save-game name in EAX
// Returns 1 if name is non-null, non-empty and XCreateSaveGame's open-existing query (mode 3)
// against savegames_directory fails, i.e. no saved game of that name exists yet; 0 otherwise.
uint8_t saved_game_name_is_available(const uint16_t *name)
{
    char scratch[0x100];
    uint32_t result;

    if (name != 0 && *name != 0) {
        result = XCreateSaveGame(name, savegames_directory, 3, scratch, 0x100);
        if (result != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53d1e0):

undefined4 FUN_0053d1e0(void)

{
  short *in_EAX;
  int iVar1;
  undefined1 local_100 [256];

  if (((in_EAX != (short *)0x0) && (*in_EAX != 0)) &&
     (iVar1 = XCreateSaveGame(&DAT_00721549,3,local_100,0x100), iVar1 != 0)) {
    return 1;
  }
  return 0;
}
#endif
