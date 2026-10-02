// saved_game_index_open_for_write  (Ghidra: FUN_0053daa0, renamed)
// address 0x53daa0, size 148 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Creates/opens the on-disk save-game
// index file for writing at the start of an index rebuild, resetting the in-memory entry count."
// The path component argument to path_append_component is register-dropped by Ghidra, but
// saved_game_root_path (0x006e3108) is the only global this function's own referenced-globals list
// includes that fits (this module's fixed "index of everything" file, matching
// saved_game_list_rebuild_index.c's use of the same file for its own index writes right after
// this call).
// register convention: no arguments.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char saved_game_root_path[0x100]; // 0x006e3108
extern file_reference_record savegame_index_file; // 0x00721330
extern int16_t savegame_index_write_count; // 0x00721444
extern uint8_t saved_game_index_file_open; // 0x00721448

extern void path_remove_last_component(char *path); // 0x555f80, this module
extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern uint8_t file_reference_create(file_reference_record *ref); // 0x5555b0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module

// blam-cc: no arguments
// Builds savegame_index_file as a fresh, absolute file_reference_record naming saved_game_root_path,
// creates and opens it for writing. On success, resets savegame_index_write_count to 0, marks
// the index open, and returns 1. On failure, sets savegame_index_write_count to -1 and returns
// whatever saved_game_index_file_open already was.
uint8_t saved_game_index_open_for_write(void)
{
    uint8_t created;
    uint8_t opened;

    memset(&savegame_index_file, 0, sizeof(savegame_index_file));
    savegame_index_file.signature = k_file_reference_signature;
    savegame_index_file.location = _file_location_absolute;
    if ((savegame_index_file.flags & _file_reference_is_file_bit) != 0) {
        path_remove_last_component(savegame_index_file.path); // unreachable: flags was just zeroed above
    }
    path_append_component(savegame_index_file.path, saved_game_root_path);
    savegame_index_file.flags |= _file_reference_is_file_bit;

    created = file_reference_create(&savegame_index_file);
    if (created != 0) {
        opened = file_reference_open(&savegame_index_file, 2);
        if (opened != 0) {
            savegame_index_write_count = 0;
            saved_game_index_file_open = 1;
            return 1;
        }
    }
    savegame_index_write_count = -1;
    return saved_game_index_file_open;
}

#if 0
Original Ghidra decompilation (0x53daa0):

undefined1 FUN_0053daa0(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;

  puVar3 = &DAT_00721330;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  DAT_00721330 = 0x66696c6f;
  DAT_00721334._2_2_ = 2;
  if (((byte)DAT_00721334 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  DAT_00721334._0_1_ = (byte)DAT_00721334 | 1;
  cVar1 = file_reference_create();
  if (cVar1 != '\0') {
    cVar1 = file_reference_open(2);
    if (cVar1 != '\0') {
      DAT_00721444 = 0;
      DAT_00721448 = 1;
      return 1;
    }
  }
  DAT_00721444 = 0xffff;
  return DAT_00721448;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
