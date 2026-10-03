// player_effect_clear_dead_players  (Ghidra: FUN_00456730, still unnamed there; named directly
//   by out/phase4/effects_types_notes.md: "player_effect_clear_dead_players 0x456730 (zeroes
//   0x3b dwords = 0xec)")
// address 0x456730, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/effects.h player_effect (size 0xec) and player_effect_globals.players[1];
//   types/game.h player_globals.local_players (+0x04) and player.unit (+0x34).
//   out/phase4/effects_types_notes.md's misattribution table places this whole address range
//   (0x456730..0x457d50) in a player_effect group rather than the "contrail" framing
//   functions.md guessed at phase 2.
// register convention: none -- no arguments.
// UNSURE: the original loops with a manual sentinel-driven while rather than a plain for over
//   k_maximum_local_player_effects; since that constant is 1, the loop can only ever process
//   local player 0, so it is rewritten here as the equivalent bounded for loop.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "game.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;                            // 0x0087a480
extern player_globals *local_player_globals;                // 0x0087a478
extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884

void player_effect_clear_dead_players(void)
{
    int32_t i;

    for (i = 0; i < k_maximum_local_player_effects; i++) {
        datum_index player_index = local_player_globals->local_players[i];
        uint8_t dead = 1;

        if (player_index != (datum_index)0xffffffff) {
            player *record = &((player *)player_data->data)[player_index & 0xffff];
            if (record->unit != (datum_index)0xffffffff) {
                dead = 0;
            }
        }

        if (dead) {
            memset(&player_effect_globals_pointer->players[i], 0, sizeof(player_effect));
        }
    }
}

#if 0
Original Ghidra decompilation (0x456730):

void FUN_00456730(void)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  undefined4 *puVar7;

  iVar4 = DAT_0087a480;
  iVar3 = DAT_0087a478;
  sVar5 = -1;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    sVar5 = 0;
  }
  while (sVar2 = sVar5, sVar2 != -1) {
    if ((((sVar2 == -1) || (0 < sVar2)) ||
        (uVar1 = *(uint *)(iVar3 + 4 + sVar2 * 4), uVar1 == 0xffffffff)) ||
       (*(int *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(iVar4 + 0x34)) == -1)) {
      puVar7 = (undefined4 *)(sVar2 * 0xec + DAT_006f1884);
      puVar7[0x39] = 0;
      for (iVar6 = 0x3b; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar7 = 0;
        puVar7 = puVar7 + 1;
      }
    }
    sVar5 = -1;
    if ((*(int *)(iVar3 + 4) != -1) && (sVar2 < 0)) {
      sVar5 = 0;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
