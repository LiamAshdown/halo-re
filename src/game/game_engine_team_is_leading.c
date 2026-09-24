// game_engine_team_is_leading  (Ghidra: FUN_00470720; renamed, no established name)
// address 0x470720, size 109 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Determines whether one side is currently leading over
// the other, using score totals with a count-based tiebreaker"); confirmed by objdump
// (--start-address=0x470720 --stop-address=0x47078d) that Ghidra's four EBP-style locals are, in
// increasing memory order, out_count[0], out_count[1], out_score[0], out_score[1] -- the exact
// same pair game_engine_gather_team_score_totals.c fills.
// register convention: single stack parameter (`filter_value`, forwarded unchanged); no register
// arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc

extern void game_engine_gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2],
    int32_t filter_value); // this batch, 0x470690

// Returns false if no multiplayer engine with teams is loaded. Otherwise gathers the two team
// score/count totals; if the two sides' match counts differ, the side with more matches is
// "leading". If the counts are tied, refreshes both team scores via get_team_score(0)/(1) (return
// values discarded, matching Ghidra) and falls back to comparing the two side's scores.
uint8_t game_engine_team_is_leading(int32_t filter_value)
{
    uint32_t out_count[2];
    uint32_t out_score[2];

    if (current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        return 0;
    }

    game_engine_gather_team_score_totals(out_count, out_score, filter_value);
    if (out_count[0] != out_count[1]) {
        return out_count[1] <= out_count[0];
    }

    ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(0);
    ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(1);
    return out_score[1] < out_score[0];
}

#if 0
Original Ghidra decompilation (0x470720), from tools/pack.py 0x470720:

bool FUN_00470720(undefined4 param_1)

{
  bool bVar1;
  int local_10;
  int local_c;
  int local_8;
  int iStack_4;

  bVar1 = false;
  if ((DAT_006f1d20 != 0) && (DAT_006f1cbc != '\0')) {
    FUN_00470690(&local_8,param_1);
    if (local_10 == local_c) {
      (**(code **)(DAT_006f1d20 + 0x50))(0);
      (**(code **)(DAT_006f1d20 + 0x50))(1);
      return iStack_4 < local_8;
    }
    bVar1 = local_c <= local_10;
  }
  return bVar1;
}
#endif
