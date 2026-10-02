// game_state_create_persistent_storage_file  (Ghidra: chimera__multiple_instance_2, renamed)
// address 0x538690, size 105 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// "Misattributed / misnamed functions" entry: "stale Chimera label; it creates and pre-sizes
// savegame.bin (game_state_create_persistent_storage_file)." Opens
// game_state_persistent_storage_path (built by game_state_allocate_buffer) and pre-sizes it to
// k_game_state_file_size, matching the same pattern as game_state_open_persistent_storage.
// register convention: __cdecl (Ghidra-recognized), no parameters.

#include "win32.h"
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
extern char game_state_persistent_storage_path[0x100]; // 0x006e2dfc
extern void *game_state_persistent_storage; // 0x006e2df8
extern uint8_t game_state_persistent_storage_created; // 0x006e2df4

extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

void game_state_create_persistent_storage_file(void)
{
    game_state_persistent_storage = CreateFileA(game_state_persistent_storage_path, 0xc0000000, 0, 0,
        4 /* OPEN_ALWAYS */, 0x8000000 /* FILE_FLAG_SEQUENTIAL_SCAN */, 0);
    if (game_state_persistent_storage != (void *)0xffffffff &&
        SetFilePointer(game_state_persistent_storage, k_game_state_file_size, 0, 0) != 0xffffffff &&
        SetEndOfFile(game_state_persistent_storage) != 0) {
        game_state_persistent_storage_created = 1;
        return;
    }
    shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
}

#if 0
Original Ghidra decompilation (0x538690):

void __cdecl chimera__multiple_instance_2(void)

{
  DWORD DVar1;
  BOOL BVar2;

  DAT_006e2df8 = CreateFileA(&DAT_006e2dfc,0xc0000000,0,(LPSECURITY_ATTRIBUTES)0x0,4,0x8000000,
                             (HANDLE)0x0);
  if (DAT_006e2df8 != (HANDLE)0xffffffff) {
    DVar1 = SetFilePointer(DAT_006e2df8,0x480000,(PLONG)0x0,0);
    if (DVar1 != 0xffffffff) {
      BVar2 = SetEndOfFile(DAT_006e2df8);
      if (BVar2 != 0) {
        DAT_006e2df4 = 1;
        return;
      }
    }
  }
  shell_display_fatal_error_dialog(0x8b,0x8c,1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
