// saved_game_get_directory_by_handle  (Ghidra: FUN_0053d080, renamed)
// address 0x53d080, size 159 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md summary "Resolves a save-game handle to its
// containing directory path (stripping the trailing file name)." out/phase4/
// saved_games_types_notes.md register-conventions list: "saved-game handle in EAX ... for
// 0x53c600 / 0x53c9f0 / 0x53ce80 / 0x53d080". The stack layout (a 0x100-byte path buffer
// immediately followed by a type word) matches saved_game_index_entry's path/type fields exactly
// (offset 0x200), so the local Ghidra types local_208/local_8 are read as that struct.
// Phase 4 review (objdump 0x53d0c0..0x53d0d7): type 0 loads EDI = 0x670c68 "blam.sav" and
// type 1 loads EDI = 0x670c54 "blam.lst" before the shared strncpy / strstr call, so the
// needle is picked by entry.type exactly as modeled (Ghidra had merged the two branches).
// register convention: saved-game handle in EAX; out_directory buffer in ESI. No stack
// arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry); // 0x53e0e0, FUN_0053e0e0 (src/game)
extern void _strncpy(char *dest, const char *source, uint32_t count); // CRT
extern char *strstr(char *haystack, const char *needle); // 0x625430, outside this batch (CRT-like substring search)

// blam-cc: saved-game handle in EAX; out_directory in ESI
// Resolves handle to its index-file slot and, if found and its type is a player profile or game
// variant, copies its path into out_directory and truncates it at the start of its "blam.sav" /
// "blam.lst" file name, leaving just the containing directory. Clears out_directory and returns 0
// on any failure (invalid handle, out-of-range slot, unresolved entry, unrecognised type, or the
// file name not found in the resolved path).
uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory)
{
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    const char *needle;
    char *name_start;

    out_directory[0] = '\0';
    if (handle == -1) {
        return 0;
    }
    slot_index = ((uint32_t)handle >> 16) & 0xfff;
    if (slot_index >= 999) {
        return 0;
    }
    found = savegame_index_read_slot((int32_t)slot_index, &entry);
    if (found == 0) {
        return 0;
    }
    if (entry.type != _saved_game_type_player_profile && entry.type != _saved_game_type_game_variant) {
        return 0;
    }
    _strncpy(out_directory, entry.path, 0xff);
    out_directory[0xff] = '\0';
    needle = (entry.type == _saved_game_type_player_profile) ? "blam.sav" : "blam.lst";
    name_start = strstr(out_directory, needle);
    if (name_start != 0) {
        *name_start = '\0';
        return 1;
    }
    out_directory[0] = '\0';
    return 0;
}

#if 0
Original Ghidra decompilation (0x53d080):

undefined4 FUN_0053d080(void)

{
  char cVar1;
  int in_EAX;
  uint uVar2;
  undefined1 *puVar3;
  char *unaff_ESI;
  char local_208 [512];
  short local_8;

  *unaff_ESI = '\0';
  if (((in_EAX != -1) && (uVar2 = in_EAX >> 0x10 & 0xfff, uVar2 < 999)) &&
     (cVar1 = FUN_0053e0e0(uVar2,local_208), cVar1 != '\0')) {
    if ((local_8 == 0) || (local_8 == 1)) {
      _strncpy(unaff_ESI,local_208,0xff);
      unaff_ESI[0xff] = '\0';
      puVar3 = (undefined1 *)FUN_00625430();
      if (puVar3 != (undefined1 *)0x0) {
        *puVar3 = 0;
        return 1;
      }
      *unaff_ESI = '\0';
    }
    return 0;
  }
  return 0;
}
#endif
