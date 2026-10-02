// game_state_load_core  (Ghidra: game_state_load_core, already named -- CEA/PDB match)
// address 0x538390, size 146 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; CEA/PDB string match on "couldn't open '%s'" /
// "loaded '%s'". Disassembly at 0x538390 confirms EAX (register-passed) is a name/path string
// forwarded unchanged to game_state_read_profile_header, saved_game_verify_version_and_checksum,
// game_state_read_profile_file and both console_print_error_va calls (as the dropped %s
// argument -- Ghidra elided it at every one of these call sites).
// register convention: name in EAX; no recognized stack parameters.

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
extern uint8_t *game_state_base; // 0x006e2dc8
extern game_state_proc game_state_revert_proc; // 0x0069e7b0

extern uint8_t game_state_read_profile_header(char *name, int32_t size, void *buffer); // 0x539460
extern uint8_t saved_game_verify_version_and_checksum(game_state_header *header, uint8_t report_error); // 0x538430
extern void game_state_read_profile_file(char *name, int32_t size, void *buffer); // 0x5394e0
extern void game_state_dispatch_load_callbacks(void); // 0x537f70
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first

// blam-cc: name in EAX
// Reads and validates the profile file "<core directory>\<name>" as a game state header; on a
// good header (version, checksum, scenario and player-count match), reverts to it, reads the
// full body into the game-state arena and dispatches the load callbacks, logging success. On
// any failure, logs that the file couldn't be opened.
void game_state_load_core(char *name)
{
    game_state_header header;

    if (game_state_read_profile_header(name, k_game_state_header_size, &header) != 0 &&
        saved_game_verify_version_and_checksum(&header, 1) != 0) {
        game_state_revert_proc();
        game_state_read_profile_file(name, k_game_state_size, game_state_base);
        console_print_error_va(0, "loaded '%s'", name);
        game_state_dispatch_load_callbacks();
        return;
    }
    console_print_error_va(0, "couldn't open '%s'", name);
}

#if 0
Original Ghidra decompilation (0x538390):

void __cdecl game_state_load_core(void)

{
  char cVar1;
  uint uVar2;
  undefined **ppuVar3;
  int iVar4;
  int local_14c [83];

  cVar1 = game_state_read_profile_header(local_14c);
  if (cVar1 != '\0') {
    uVar2 = saved_game_verify_version_and_checksum(local_14c,'\x01');
    if ((char)uVar2 != '\0') {
      (*(code *)PTR_chimera__revert_0069e7b0)();
      game_state_read_profile_file(DAT_006e2dc8);
      console_print_error_va("loaded \'%s\'");
      ppuVar3 = &PTR_FUN_0069e7b4;
      iVar4 = 0xd;
      do {
        (*(code *)*ppuVar3)();
        ppuVar3 = ppuVar3 + 1;
        iVar4 = iVar4 + -1;
      } while (iVar4 != 0);
      return;
    }
  }
  console_print_error_va("couldn\'t open \'%s\'");
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
