// game_engine_ctf_player_flag_tick  (Ghidra: FUN_004697e0; named per its summary)
// address 0x4697e0, size 374 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Per-tick per-player handling of objective (flag)
//   pickup/carry/drop bookkeeping and associated event notifications"); types/game.h
//   game_variant::ctf_option_7c/7e (aliased 0x006f1d04 / 0x006f1d06), player::team (0x20),
//   player_data (0x0087a480); game_engine_is_inactive, game_engine_broadcast_kill_feed_by_
//   relationship (0x460c10) and game_engine_ctf_notify_flag_carried_throttled /
//   game_engine_ctf_reset_team_return_credit (this batch) already established.
// UNSURE: player + 0xc6 and player + 0xc4 are read/incremented here as raw counters that do not
//   line up with any named field in types/game.h's player struct (the nearest named neighbours
//   are objective_time at 0xc4 and unknown_c8 at 0xc8); kept as raw offsets. extraout_EDX (the
//   team index used to clear the return-credit arrays on the "still on own pad" path) is not
//   independently confirmed here and is modeled as the flag's own team_index field.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include "fn_game.h"

extern data_array *object_data; // 0x008603b0
extern data_array *player_data;    // 0x0087a480
extern int16_t network_game_mode;  // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_option_7c/7e aliased 0x006f1d04/06)
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10
extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4
extern int32_t ctf_team_return_credit_ticks[2];  // 0x006b0ea8


extern void game_engine_ctf_notify_flag_carried_throttled(void); // 0x4689e0, this batch


// While `player_index` is valid and hosting: if the player is standing back on their own flag's
// pad (team matches), either finishes returning the flag (ctf_option_7e clear, resetting the
// return credit and, if it had been active and the game is no longer active, crediting the
// return and broadcasting it) or, while it is still resetting, throttled-notifies that it is
// being returned. Otherwise (a different team's pad), starts the return-credit countdown for
// the flag's own team when nothing else claims it and it is not currently a normal engine round.
// Returns true unless the check was skipped because player_index/network mode did not apply.
// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; flag_handle arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> flag_handle, player_index
uint8_t game_engine_ctf_player_flag_tick(uint32_t flag_handle, uint32_t player_index)
{
    object *flag_obj = ((object_header *)object_data->data)[flag_handle & 0xffff].data;
    int16_t team = ((struct object *)flag_obj)->owner_team; // UNSURE: name_index/team_index

    if (player_index != 0xffffffff && network_game_mode == 2) {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

        if ((int32_t)team == p->team) {
            if (game_engine_variant.ctf_option_7e == 0) {
                if ((*(uint8_t *)((uint8_t *)flag_obj + 0x22c) & 0x40) != 0) {
                    if (game_engine_is_inactive() != 0) {
                        ctf_team_return_credit_active[team] = 0; // UNSURE: extraout_EDX modeled as `team`
                        ctf_team_return_credit_ticks[team] = 0;
                        *(int16_t *)((uint8_t *)p + 0xc6) += 1; // UNSURE: unnamed player field
                        game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index, 1); // BL = 1 at 0x469886
                        game_engine_queue_multiplayer_sound(p->team != 0 ? 9 : 0xc, 0xffffffff, 1); // 0x46988d..0x46989f
                    }
                }
                game_engine_ctf_reset_team_return_credit(flag_handle); // FIXED 2026-09-28: 0x4698a7 loads EAX from the first argument
                return 0;
            }
            if ((*(uint8_t *)((uint8_t *)flag_obj + 0x22c) & 0x40) != 0) {
                game_engine_ctf_notify_flag_carried_throttled();
            }
            return 0;
        }

        if ((*(uint8_t *)((uint8_t *)flag_obj + 0x22c) & 0x40) == 0 &&
            (current_game_engine == 0 || game_engine_state_value == 0)) {
            *(int16_t *)&((struct player *)p)->objective_time += 1; // UNSURE: unnamed player field
            if (game_engine_variant.ctf_option_7c == 0) {
                game_engine_queue_multiplayer_sound(p->team != 0 ? 8 : 0xb, 0xffffffff, 1); // 0x4698fe..0x46990d
                ctf_team_return_credit_active[team] = 1;
                ctf_team_return_credit_ticks[team] = 0;
                game_engine_broadcast_kill_feed_by_relationship(player_index, 0xffffffff, 0x29, 0x26, player_index, 1); // BL = 1 at 0x469928
            }
        }
        *(uint8_t *)((uint8_t *)flag_obj + 0x22c) |= 0x40;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4697e0), from tools/pack.py 0x4697e0:

undefined1 FUN_004697e0(uint param_1,uint param_2)

{
  short *psVar1;
  short sVar2;
  int iVar3;
  char cVar4;
  int extraout_EDX;
  int iVar5;
  int iVar6;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((param_2 != 0xffffffff) && (DAT_00719720 == 2)) {
    iVar5 = (param_2 & 0xffff) * 0x200;
    iVar6 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((int)*(short *)(iVar3 + 0xb8) == *(int *)(iVar5 + 0x20 + *(int *)(DAT_0087a480 + 0x34))) {
      if (DAT_006f1d06 == '\0') {
        if ((*(byte *)(iVar3 + 0x22c) & 0x40) != 0) {
          cVar4 = game_engine_is_inactive();
          if (cVar4 != '\0') {
            (&DAT_006b0ea4)[extraout_EDX] = 0;
            (&DAT_006b0ea8)[extraout_EDX] = 0;
            psVar1 = (short *)(iVar6 + 0xc6);
            *psVar1 = *psVar1 + 1;
            FUN_00460c10(param_2,0x25,0x2a,0x28,param_2);
            game_engine_queue_multiplayer_sound(1);
          }
        }
        FUN_00468840();
        return 0;
      }
      if ((*(byte *)(iVar3 + 0x22c) & 0x40) != 0) {
        FUN_004689e0();
      }
      return 0;
    }
    if (((*(byte *)(iVar3 + 0x22c) & 0x40) == 0) &&
       (((DAT_006f1d20 == 0 || (DAT_0087aa10 == 0)) &&
        (psVar1 = (short *)(iVar6 + 0xc4), *psVar1 = *psVar1 + 1, DAT_006f1d04 == '\0')))) {
      game_engine_queue_multiplayer_sound(1);
      sVar2 = *(short *)(iVar3 + 0xb8);
      (&DAT_006b0ea4)[sVar2] = 1;
      (&DAT_006b0ea8)[sVar2] = 0;
      FUN_00460c10(param_2,0xffffffff,0x29,0x26,param_2);
    }
    *(uint *)(iVar3 + 0x22c) = *(uint *)(iVar3 + 0x22c) | 0x40;
  }
  return 1;
}
#endif
