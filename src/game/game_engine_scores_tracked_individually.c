// game_engine_scores_tracked_individually  (Ghidra: FUN_004635e0; renamed per its summary)
// address 0x4635e0, size 56 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Returns whether player scores should be tracked/
// displayed individually rather than by team, based on the team-mode flag and an option bit");
// types/game.h game_variant::unknown_3c (+0x3c, aliased 0x006f1cc4), game_variant::
// game_engine_index (+0x30, aliased 0x006f1cb8), game_variant::flags (+0x38, aliased
// 0x006f1cc0), game_variant::ctf_option_7e (+0x7e, aliased 0x006f1d06).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88

uint8_t game_engine_scores_tracked_individually(void)
{
    uint8_t result = 1;

    if (current_game_engine != 0) {
        uint8_t no_team_mode = (game_engine_variant.objective_indicator == 0);
        if (game_engine_variant.game_engine_index == _game_engine_slayer &&
            game_engine_variant.engine.slayer.kill_in_order == 0) {
            no_team_mode = 0;
        }
        result = ((uint8_t)game_engine_variant.flags & 1) | no_team_mode;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4635e0), from tools/pack.py 0x4635e0:

byte FUN_004635e0(void)

{
  byte bVar1;
  bool bVar2;

  bVar1 = 1;
  if (DAT_006f1d20 != 0) {
    bVar2 = DAT_006f1cc4 == 0;
    if ((DAT_006f1cb8 == 2) && (DAT_006f1d06 == '\0')) {
      bVar2 = false;
    }
    bVar1 = (byte)DAT_006f1cc0 & 1 | bVar2;
  }
  return bVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
