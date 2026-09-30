// game_engine_player_ready_to_respawn  (Ghidra: game_engine_player_ready_to_respawn, already
// named)
// address 0x460f70, size 265 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Per-tick check that counts down a dead player's
// respawn timer, playing countdown cues and staggering respawns across frames"); types/game.h
// player::marked_for_deletion (+0xd5), player::deaths (+0xae), player::respawn_timer (+0x2c),
// player::local_player_index (+0x02), game_time_globals::game_time (+0x0c), game_engine_state;
// this batch's game_engine_player_is_eliminated (0x460f30) and
// game_engine_player_has_respawn_priority (0x460e40).
// register convention: a player index in EAX (in_EAX).
//   // blam-cc: EAX -> player_index
// UNSURE: this function calls FUN_00460f30/FUN_00460e40 with zero visible arguments in Ghidra's
// own rendering; both need the same player index this function itself receives in EAX, so it is
// passed explicitly here rather than modeled as some other unrecoverable register.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern game_engine_state game_engine_state_value;    // 0x0087aa10
extern game_time_globals *game_time;                 // 0x006f1d6c


extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_index); // 0x460e40, this batch


// blam-cc: EAX -> player_index
uint8_t game_engine_player_ready_to_respawn(uint32_t player_index)
{
    player *p;
    uint8_t ready;

    if (current_game_engine == 0) {
        return 0;
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    if (p->marked_for_deletion == 1) {
        return 0;
    }

    if (p->deaths != 0) {
        if (game_engine_player_is_eliminated(player_index) != 0) {
            return 0;
        }
        if (game_engine_player_has_respawn_priority(player_index) != 0) {
            return 0;
        }
        if (game_engine_state_value == _game_engine_state_post_game) {
            return 0;
        }
        if (game_engine_state_value == _game_engine_state_ended) {
            return 0;
        }
        if (0 < p->respawn_timer) {
            if (p->local_player_index != -1 &&
                (p->respawn_timer == 0x5a || p->respawn_timer == 0x3c ||
                 p->respawn_timer == 0x1e || p->respawn_timer == 1)) {
                game_engine_queue_multiplayer_sound(p->respawn_timer == 1 ? 0x1f : 0x1d, 0xffffffff, 0); // 0x461004..0x461034
            }
            p->respawn_timer = p->respawn_timer - 1;
            ready = (p->respawn_timer == 0);
            if (!ready) {
                return ready;
            }
            goto stagger_check;
        }
    }
    ready = 1;

stagger_check:
    if (game_time->game_time < 4) {
        return ready;
    }
    {
        uint32_t phase = (uint32_t)game_time->game_time & 0x8000001f;
        if ((int32_t)phase < 0) {
            phase = (phase - 1 | 0xffffffe0) + 1;
        }
        if (phase != (player_index & 0x1f)) {
            return 0;
        }
    }
    return ready;
}

#if 0
Original Ghidra decompilation (0x460f70), from tools/pack.py 0x460f70:

/* WARNING: Removing unreachable block (ram,0x0046106a) */

bool game_engine_player_ready_to_respawn(void)

{
  int *piVar1;
  char cVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  int iVar5;
  bool bVar6;
  
  if (DAT_006f1d20 == 0) {
    return false;
  }
  iVar4 = (in_EAX & 0xffff) * 0x200;
  iVar5 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
  if (*(char *)(iVar4 + 0xd5 + *(int *)(DAT_0087a480 + 0x34)) == '\x01') {
    return false;
  }
  if (*(short *)(iVar5 + 0xae) != 0) {
    cVar2 = FUN_00460f30();
    if (cVar2 != '\0') {
      return false;
    }
    cVar2 = FUN_00460e40();
    if (cVar2 != '\0') {
      return false;
    }
    if (DAT_0087aa10 == 3) {
      return false;
    }
    if (DAT_0087aa10 == 2) {
      return false;
    }
    iVar4 = *(int *)(iVar5 + 0x2c);
    if (0 < iVar4) {
      if ((*(short *)(iVar5 + 2) != -1) &&
         ((((iVar4 == 0x5a || (iVar4 == 0x3c)) || (iVar4 == 0x1e)) || (iVar4 == 1)))) {
        game_engine_queue_multiplayer_sound(0);
      }
      piVar1 = (int *)(iVar5 + 0x2c);
      *piVar1 = *piVar1 + -1;
      bVar6 = *piVar1 == 0;
      if (!bVar6) {
        return bVar6;
      }
      goto LAB_00461047;
    }
  }
  bVar6 = true;
LAB_00461047:
  if ((int)*(uint *)(DAT_006f1d6c + 0xc) < 4) {
    return bVar6;
  }
  uVar3 = *(uint *)(DAT_006f1d6c + 0xc) & 0x8000001f;
  if ((int)uVar3 < 0) {
    uVar3 = (uVar3 - 1 | 0xffffffe0) + 1;
  }
  if (uVar3 != (in_EAX & 0x1f)) {
    return false;
  }
  return bVar6;
}
#endif
