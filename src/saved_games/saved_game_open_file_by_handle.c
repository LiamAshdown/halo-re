// saved_game_open_file_by_handle  (Ghidra: saved_game_open_file_by_handle, already named)
// address 0x53c9f0, size 136 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA. Ghidra's own "int in_EAX" shows the handle is
// register-passed in EAX, matching out/phase4/saved_games_types_notes.md "saved-game handle in
// EAX for 0x53c600 / 0x53c9f0 / 0x53ce80 / 0x53d080". The out_ref parameter is fully resolved by
// Ghidra as a normal stack argument.
// register convention: saved-game handle in EAX; out_ref as the one stack argument.

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
extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry); // 0x53e0e0, FUN_0053e0e0 (src/game)
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern void path_remove_last_component(char *path); // 0x555f80, this module

// blam-cc: saved-game handle in EAX; out_ref as the one stack argument
// Resolves handle to its index-file slot and, if found, builds a file-reference in out_ref
// naming its path (absolute, is-file) and opens it for reading and writing. Returns 1 on
// success, 0 if the slot couldn't be resolved or the open failed.
uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref)
{
    saved_game_index_entry entry;
    uint8_t found;
    uint8_t opened;

    found = savegame_index_read_slot(((uint32_t)handle >> 16) & 0xfff, &entry);
    if (found != 0) {
        memset(out_ref, 0, sizeof(*out_ref));
        out_ref->signature = k_file_reference_signature;
        out_ref->location = _file_location_absolute;
        if ((out_ref->flags & _file_reference_is_file_bit) != 0) {
            path_remove_last_component(out_ref->path);
        }
        path_append_component(out_ref->path, entry.path);
        out_ref->flags = out_ref->flags | _file_reference_is_file_bit;
        opened = file_reference_open(out_ref, 3);
        if (opened != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53c9f0):

undefined4 saved_game_open_file_by_handle(undefined4 *param_1)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  undefined4 *puVar3;
  undefined1 local_208 [520];

  cVar1 = FUN_0053e0e0(in_EAX >> 0x10 & 0xfff,local_208);
  if (cVar1 != '\0') {
    puVar3 = param_1;
    for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    *param_1 = 0x66696c6f;
    *(undefined2 *)((int)param_1 + 6) = 2;
    if ((*(byte *)(param_1 + 1) & 1) != 0) {
      path_remove_last_component();
    }
    path_append_component();
    *(byte *)(param_1 + 1) = *(byte *)(param_1 + 1) | 1;
    cVar1 = file_reference_open(3);
    if (cVar1 != '\0') {
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
