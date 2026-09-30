// saved_game_delete_by_handle  (Ghidra: saved_game_delete_by_handle, already named)
// address 0x53c960, size 143 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x53c960..0x53c9da: handle
// is in EDI (register, no stack arguments); savegame_index_read_slot (FUN_0053e0e0) takes (slot index, out entry) as plain
// stack args; XDeleteSaveGame is called with EAX = the found entry's wide display name
// (+0x100: lea eax,[esp+0x108] at 0x53c9b2; phase-4 review, was entry.path) and ECX = the fixed
// root directory (register arguments, not stack -- corrected from the earlier guess in
// saved_game_create_slot.c, which is fixed alongside this file); savegame_index_remove_slot (FUN_0053e4a0) takes the slot
// index as a single stack argument.
// register convention: saved-game handle in EDI. No stack arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_game.h"
#include "fn_saved_games.h"

extern uint8_t savegame_index_dirty; // 0x00721447
extern char savegames_directory[0x100]; // 0x00721549
extern int32_t cached_saved_game_something; // 0x00692af8, UNSURE name/purpose (unrelated cache reset)

extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry); // 0x53e0e0, FUN_0053e0e0 (src/game)
extern uint8_t savegame_index_remove_slot(int32_t slot_index); // 0x53e4a0, FUN_0053e4a0 (src/game): removes the in-memory index entry


// blam-cc: saved-game handle in EDI
// Deletes the saved-game entry identified by handle, both from disk (via XDeleteSaveGame,
// unless the handle's builtin bit is set) and from the in-memory index. Returns 1 on success, 0
// if the index is dirty, the handle is out of range, the entry can't be found, the disk delete
// fails, or the in-memory removal fails.
uint8_t saved_game_delete_by_handle(int32_t handle)
{
    uint8_t result;
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    int32_t delete_result;
    uint8_t removed;

    result = 0;
    if (savegame_index_dirty == 0) {
        slot_index = ((uint32_t)handle >> 16) & 0xfff;
        result = 0;
        if ((handle & 0xf) < 2 && slot_index < 999) {
            found = savegame_index_read_slot((int32_t)slot_index, &entry);
            if (found != 0) {
                if ((handle & 0x40000000) == 0) {
                    result = 1;
                    delete_result = XDeleteSaveGame(entry.display_name, savegames_directory);
                    if (delete_result != 0) {
                        result = 0;
                    }
                }
                removed = savegame_index_remove_slot((int32_t)slot_index);
                if (removed == 0) {
                    result = 0;
                }
            }
        }
    }
    cached_saved_game_something = -1;
    return result;
}

#if 0
Original Ghidra decompilation (0x53c960):

undefined1 saved_game_delete_by_handle(void)

{
  char cVar1;
  int iVar2;
  undefined1 uVar3;
  undefined1 uVar4;
  uint uVar5;
  uint unaff_EDI;
  undefined1 local_208 [520];

  uVar3 = 0;
  uVar4 = 0;
  if (DAT_00721447 == '\0') {
    uVar5 = (int)unaff_EDI >> 0x10 & 0xfff;
    uVar4 = uVar3;
    if ((((unaff_EDI & 0xf) < 2) && (uVar5 < 999)) &&
       (cVar1 = FUN_0053e0e0(uVar5,local_208), cVar1 != '\0')) {
      if ((unaff_EDI & 0x40000000) == 0) {
        uVar4 = 1;
        iVar2 = XDeleteSaveGame();
        if (iVar2 != 0) {
          uVar4 = 0;
        }
      }
      cVar1 = FUN_0053e4a0(uVar5);
      if (cVar1 == '\0') {
        uVar4 = 0;
      }
    }
  }
  DAT_00692af8 = 0xffffffff;
  return uVar4;
}
#endif
