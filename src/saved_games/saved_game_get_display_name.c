// saved_game_get_display_name  (Ghidra: FUN_0053c600, renamed)
// address 0x53c600, size 93 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md summary "Given a packed save-game handle,
// looks up its index record and returns a pointer to its cached wide display name." Ghidra's
// own "int in_EAX" shows the handle is register-passed in EAX; savegame_index_read_slot (FUN_0053e0e0) (outside this
// batch, also used by saved_game_delete_by_handle / saved_game_open_file_by_handle) resolves a
// handle's index-file slot number to its saved_game_index_entry.
// register convention: saved-game handle in EAX (matches out/phase4/saved_games_types_notes.md
// "saved-game handle in EAX for 0x53c600 / 0x53c9f0 / 0x53ce80 / 0x53d080").

#include "crt.h"
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
extern uint16_t saved_game_display_name_buffer[0x80]; // 0x006e3008

extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry); // 0x53e0e0, FUN_0053e0e0 (src/game)

// blam-cc: saved-game handle in EAX
// Resolves handle to its index-file slot (bits 16..27) and, if it is in range and the index
// entry is found, copies its display name into the shared saved_game_display_name_buffer.
// Always returns a pointer to that shared buffer (stale contents if the lookup failed).
uint16_t *saved_game_get_display_name(int32_t handle)
{
    saved_game_index_entry entry;
    uint32_t slot_index;
    uint8_t found;

    slot_index = ((uint32_t)handle >> 16) & 0xfff;
    saved_game_display_name_buffer[0] = 0;
    if (slot_index < 999) {
        found = savegame_index_read_slot(slot_index, &entry);
        if (found != 0) {
            wcsncpy((wchar_t *)saved_game_display_name_buffer, (const wchar_t *)entry.display_name, 0x7f);
            saved_game_display_name_buffer[0x7f] = 0;
        }
    }
    return saved_game_display_name_buffer;
}

#if 0
Original Ghidra decompilation (0x53c600):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined * FUN_0053c600(void)

{
  char cVar1;
  int in_EAX;
  uint uVar2;
  undefined1 local_208 [256];
  wchar_t local_108 [132];

  uVar2 = in_EAX >> 0x10 & 0xfff;
  _DAT_006e3008 = 0;
  if ((uVar2 < 999) && (cVar1 = FUN_0053e0e0(uVar2,local_208), cVar1 != '\0')) {
    _wcsncpy((wchar_t *)&DAT_006e3008,local_108,0x7f);
    _DAT_006e3106 = 0;
  }
  return &DAT_006e3008;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
