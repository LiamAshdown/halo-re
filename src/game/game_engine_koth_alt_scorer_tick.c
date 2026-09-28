// game_engine_koth_alt_scorer_tick  (Ghidra: FUN_0046c230; named per its summary)
// address 0x46c230, size 231 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Alternate per-tick King-of-the-Hill scorer that
//   increments per-player and per-team hill-occupancy counters against a fixed target and
//   triggers the end-of-game sequence once that target is met"); types/game.h player::team
//   (0x20), player_data (0x0087a480); game_engine_begin_end_game_sequence already committed.
// register convention: player index in in_EAX.
//   // blam-cc: EAX -> player_index
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;  // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t king_alt_player_score[]; // 0x006b118c, UNSURE exact size (indexed by player index)
extern int32_t king_alt_team_score[16]; // 0x006b114c (indexed by player::team)
extern int32_t king_alt_score_target;   // 0x006b1148

extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_begin_end_game_sequence(void); // 0x45fd90

// blam-cc: EAX -> player_index
// While hosting, increments the player's own and their team's alternate hill-score counters,
// firing warning sounds at fixed checkpoints below the target. Regardless of hosting, ends the
// game once the player's team reaches the target.
void game_engine_koth_alt_scorer_tick(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (network_game_mode == 2) {
        king_alt_player_score[player_index & 0xffff]++;
        king_alt_team_score[p->team]++;
        if (king_alt_score_target - king_alt_team_score[p->team] == 900) {
            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1); // 0x46c27d..0x46c2a8
        }
        if (king_alt_score_target - king_alt_team_score[p->team] == 0x708) {
            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0
                ? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1); // 0x46c2c8..0x46c2f5
        }
    }

    if (king_alt_team_score[p->team] < king_alt_score_target) {
        return;
    }
    game_engine_begin_end_game_sequence();
}

#if 0
Original Ghidra decompilation (0x46c230), from tools/pack.py 0x46c230:

void FUN_0046c230(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if (DAT_00719720 == 2) {
    *(int *)(&DAT_006b118c + (in_EAX & 0xffff) * 4) =
         *(int *)(&DAT_006b118c + (in_EAX & 0xffff) * 4) + 1;
    (&DAT_006b114c)[*(int *)(iVar1 + 0x20)] = (&DAT_006b114c)[*(int *)(iVar1 + 0x20)] + 1;
    if (DAT_006b1148 - (&DAT_006b114c)[*(int *)(iVar1 + 0x20)] == 900) {
      game_engine_queue_multiplayer_sound(1);
    }
    if (DAT_006b1148 - (&DAT_006b114c)[*(int *)(iVar1 + 0x20)] == 0x708) {
      game_engine_queue_multiplayer_sound(1);
    }
  }
  if ((int)(&DAT_006b114c)[*(int *)(iVar1 + 0x20)] < DAT_006b1148) {
    return;
  }
  game_engine_begin_end_game_sequence();
  return;
}
#endif
