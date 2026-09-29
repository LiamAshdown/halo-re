// game_engine_koth_player_tick  (Ghidra: FUN_0046ab00; named per its summary)
// address 0x46ab00, size 424 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Per-tick handler for a King-of-the-Hill player: if
//   the player is validly standing in the hill and the game is not already decided, credits
//   hill time to their team, fires countdown/warning sound cues..."); types/game.h player::unit
//   (0x34), player::team (0x20), player::objective_time (0xc4, though this function narrows it
//   to a 16-bit increment -- see UNSURE), player::engine_message/engine_message_subject (0x74/0x78); game_variant::
//   score_limit aliased at 0x006f1ce0 (variant + 0x58), multiplied by 0x708 (1800 ticks == 60 s
//   at 30 Hz) giving a per-minute-configured time limit in ticks; game_time_globals::game_time.
//   0x006b0f40 sits in the 0x10 bytes directly before king_starting_location_count (0x006b0f50,
//   this batch), sized for k_maximum_players (16) indexed by player, not team; 0x006b0f00
//   (0x40 bytes before it) and 0x006b0ec0 are each sized for k_maximum_teams (16), indexed by
//   player::team.
// register convention: player index in the stack parameter (Ghidra's own param_1).
// UNSURE: player + 0xc4 is written here as a 16-bit increment, narrower than types/game.h's
//   int32_t objective_time; kept literal rather than reconciled.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"

extern data_array *player_data;                  // 0x0087a480
extern uint8_t king_hill_player_in_hill[16];     // 0x006b0f40
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10
extern int16_t network_game_mode;                // 0x00719720
extern game_time_globals *game_time;             // 0x006f1d6c
extern int32_t king_bucket_last_credit_tick[16]; // 0x006b0f00
extern int32_t king_bucket_credit_ticks[16];     // 0x006b0ec0
extern game_variant game_engine_variant;         // 0x006f1c88 (score_limit aliased 0x006f1ce0)

extern uint8_t game_engine_koth_player_in_hill_bounds(uint32_t player_index); // 0x46aa60, this batch
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern uint8_t game_engine_get_teams_enabled(void); // 0x462bf0; returns a bool in AL
extern void game_engine_begin_end_game_sequence(void); // 0x45fd90

// Clears the player's per-life hill markers, and, if their unit is valid, no engine has already
// decided the game, and they are standing in the hill bounds, marks them as currently crediting,
// bumps their per-round hill-time tally (a narrow 16-bit increment of player::objective_time),
// and -- once per game tick while hosting -- advances their team's hill-time bucket toward the
// variant's score limit (in minutes, scaled to ticks), firing warning sounds at fixed
// checkpoints and ending the game once the bucket reaches the limit. Finally re-marks the player
// as actively crediting (0x74 = 0x22, 0x78 = player_index).
void game_engine_koth_player_tick(uint32_t player_index)
{
    uint32_t idx = player_index & 0xffff;
    player *p = (player *)((uint8_t *)player_data->data + idx * sizeof(player));

    *(uint32_t *)&((struct player *)p)->engine_message = 0xffffffff;
    *(uint32_t *)&((struct player *)p)->engine_message_subject = 0xffffffff;
    king_hill_player_in_hill[idx] = 0;

    if (p->unit != (datum_index)0xffffffff &&
        (current_game_engine == 0 || game_engine_state_value == 0) &&
        game_engine_koth_player_in_hill_bounds(player_index) != 0) {
        uint8_t hosting = (network_game_mode == 2);

        king_hill_player_in_hill[idx] = 1;
        if (hosting) {
            *(int16_t *)&((struct player *)p)->objective_time += 1;
        }

        if (king_bucket_last_credit_tick[p->team] < game_time->game_time && network_game_mode == 2) {
            int32_t limit_ticks = game_engine_variant.score_limit * 0x708;
            int32_t bucket;

            king_bucket_credit_ticks[p->team]++;
            king_bucket_last_credit_tick[p->team] = game_time->game_time;
            bucket = king_bucket_credit_ticks[p->team];

            // CORRECTED by review (objdump 0x46abd0..0x46ac76): Ghidra drops the announcer
            // index these three sites compute into ESI, leaving only the literal 1 that is
            // really multiplayer_sound_request::broadcast. Per this module's established
            // single-parameter convention for game_engine_queue_multiplayer_sound, the ESI
            // value is what belongs in that parameter:
            //   0x46abf5: teams ? 5 + 2 * (team != 0) : 3
            //   0x46ac27: teams ? 4 + 2 * (team != 0) : 2
            //   0x46ac6c: the constant 0x2a
            if (limit_ticks - bucket == 900) {
                game_engine_queue_multiplayer_sound(game_engine_get_teams_enabled() != 0
                    ? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1);
            }
            if (limit_ticks - bucket == 0x708) {
                game_engine_queue_multiplayer_sound(game_engine_get_teams_enabled() != 0
                    ? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1);
            }
            bucket = king_bucket_credit_ticks[p->team];
            if (bucket > 0 && bucket % 0x96 == 0 && bucket < limit_ticks) {
                // 0x46ac66 loads EDI (the recipient) from the first argument.
                game_engine_queue_multiplayer_sound(0x2a, player_index, 1);
            }
            if (limit_ticks <= king_bucket_credit_ticks[p->team]) {
                game_engine_begin_end_game_sequence();
            }
        }

        *(uint32_t *)&((struct player *)p)->engine_message = 0x22;
        *(uint32_t *)&((struct player *)p)->engine_message_subject = player_index;
    }
}

#if 0
Original Ghidra decompilation (0x46ab00), from tools/pack.py 0x46ab00:

void FUN_0046ab00(uint param_1)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  bool bVar7;

  uVar6 = param_1 & 0xffff;
  iVar4 = *(int *)(DAT_0087a480 + 0x34) + uVar6 * 0x200;
  *(undefined4 *)(iVar4 + 0x74) = 0xffffffff;
  *(undefined4 *)(iVar4 + 0x78) = 0xffffffff;
  (&DAT_006b0f40)[uVar6] = 0;
  if ((*(int *)(iVar4 + 0x34) != -1) &&
     (((DAT_006f1d20 == 0 || (DAT_0087aa10 == 0)) &&
      (cVar2 = game_engine_koth_player_in_hill_bounds(), cVar2 != '\0')))) {
    bVar7 = DAT_00719720 == 2;
    (&DAT_006b0f40)[uVar6] = 1;
    if (bVar7) {
      *(short *)(iVar4 + 0xc4) = *(short *)(iVar4 + 0xc4) + 1;
    }
    iVar1 = DAT_006f1d6c;
    if ((*(int *)(&DAT_006b0f00 + *(int *)(iVar4 + 0x20) * 4) < *(int *)(DAT_006f1d6c + 0xc)) &&
       (DAT_00719720 == 2)) {
      (&DAT_006b0ec0)[*(int *)(iVar4 + 0x20)] = (&DAT_006b0ec0)[*(int *)(iVar4 + 0x20)] + 1;
      iVar5 = DAT_006f1ce0 * 0x708;
      *(undefined4 *)(&DAT_006b0f00 + *(int *)(iVar4 + 0x20) * 4) = *(undefined4 *)(iVar1 + 0xc);
      iVar1 = (&DAT_006b0ec0)[*(int *)(iVar4 + 0x20)];
      iVar3 = iVar5 - iVar1;
      if (iVar3 == 900) {
        game_engine_get_teams_enabled();
        game_engine_queue_multiplayer_sound(1);
      }
      if (iVar3 == 0x708) {
        game_engine_get_teams_enabled();
        game_engine_queue_multiplayer_sound(1);
      }
      iVar4 = (&DAT_006b0ec0)[*(int *)(iVar4 + 0x20)];
      if (((0 < iVar4) && (iVar4 % 0x96 == 0)) && (iVar4 < iVar5)) {
        game_engine_queue_multiplayer_sound(1);
      }
      if (iVar5 <= iVar1) {
        game_engine_begin_end_game_sequence();
      }
    }
    iVar4 = *(int *)(DAT_0087a480 + 0x34) + uVar6 * 0x200;
    *(undefined4 *)(iVar4 + 0x74) = 0x22;
    *(uint *)(iVar4 + 0x78) = param_1;
  }
  return;
}
#endif
