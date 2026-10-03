// game_engine_get_teams_enabled  (Ghidra: game_engine_get_teams_enabled, already named)
// address 0x462bf0, size 18 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Returns whether the active game engine has team play
// enabled"); types/game.h game_variant::teams (+0x34, aliased 0x006f1cbc,
// "game_engine_get_teams_enabled" named directly in the header comment).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (teams aliased 0x006f1cbc)

uint8_t game_engine_get_teams_enabled(void)
{
    if (current_game_engine != 0) {
        return game_engine_variant.teams;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x462bf0), from tools/pack.py 0x462bf0:

undefined1 game_engine_get_teams_enabled(void)

{
  undefined1 uVar1;

  uVar1 = 0;
  if (DAT_006f1d20 != 0) {
    uVar1 = DAT_006f1cbc;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
