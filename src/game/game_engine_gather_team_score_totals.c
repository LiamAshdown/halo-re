// game_engine_gather_team_score_totals  (Ghidra: FUN_00470690; renamed, no established name)
// address 0x470690, size 136 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Gathers two paired per-side totals (score-like values
// via a callback, and matching counts from a status table) used by the lead-change comparison
// helpers"); types/game.h game_engine_definition::get_team_score (+0x50, "called with 0 and 1").
// register convention: fully reconstructed against
//   objdump -d -M intel --start-address=0x470690 --stop-address=0x470718 bin/halo.exe
// since Ghidra shows the second out-parameter as `unaff_EDI`. `out_score` and `filter_value` are
// this function's own (correctly identified) stack parameters; `out_count` arrives in EDI.
//   // blam-cc: EDI -> out_count, stack -> out_score, filter_value
// UNSURE: the 16-entry, 0x20-stride table at network_server+0x1c8 and the per-entry predicate
// network_player_entry_validate(&entry[-0x1e]) are outside any header this module owns; modeled with raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc
extern uint8_t network_session_ptr[];                // 0x0071c2d4

extern uint8_t network_player_entry_validate(void *entry_minus_0x1e); // 0x4de9f0, not in this batch; UNSURE signature

// blam-cc: EDI -> out_count, stack -> out_score, filter_value
// Zeroes out_count[0..1] and out_score[0..1]. If a multiplayer engine is loaded and teams are
// enabled, scans the 16-entry status table at network_server+0x1c8, and for every entry that
// passes network_player_entry_validate, does not match `filter_value` at its own +1 byte, and whose category byte
// (offset 0) is 0 or 1, increments out_count[category]; then fills out_score[0] and out_score[1]
// from current_game_engine->get_team_score(0) and (1).
void game_engine_gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2], int32_t filter_value)
{
    out_score[0] = 0;
    out_score[1] = 0;
    out_count[0] = 0;
    out_count[1] = 0;

    if (current_game_engine != 0 && game_engine_teams_enabled_flag) {
        uint8_t *entry = network_session_ptr + 0x1c8;
        int32_t i;

        for (i = 0; i < 16; i++) {
            if (network_player_entry_validate(entry - 0x1e) != 0 && (int8_t)entry[1] != filter_value) {
                int8_t category = (int8_t)entry[0];

                if (category >= 0 && category < 2) {
                    out_count[category] = out_count[category] + 1;
                }
            }
            entry = entry + 0x20;
        }
        out_score[0] = ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        out_score[1] = ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(1);
    }
}

#if 0
Original Ghidra decompilation (0x470690), from tools/pack.py 0x470690:

void FUN_00470690(undefined4 *param_1,int param_2)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  char *pcVar4;
  undefined4 *unaff_EDI;
  bool bVar5;

  bVar5 = DAT_006f1d20 != 0;
  param_1[1] = 0;
  *param_1 = 0;
  unaff_EDI[1] = 0;
  *unaff_EDI = 0;
  if ((bVar5) && (DAT_006f1cbc != '\0')) {
    pcVar4 = (char *)(DAT_0071c2d4 + 0x1c8);
    iVar3 = 0x10;
    do {
      cVar1 = FUN_004de9f0();
      if ((((cVar1 != '\0') && (pcVar4[1] != param_2)) && (cVar1 = *pcVar4, -1 < cVar1)) &&
         (cVar1 < '\x02')) {
        unaff_EDI[cVar1] = unaff_EDI[cVar1] + 1;
      }
      pcVar4 = pcVar4 + 0x20;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    uVar2 = (**(code **)(DAT_006f1d20 + 0x50))(0);
    *param_1 = uVar2;
    uVar2 = (**(code **)(DAT_006f1d20 + 0x50))(1);
    param_1[1] = uVar2;
  }
  return;
}
#endif
