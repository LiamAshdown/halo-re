// game_engine_player_new_life  (Ghidra: FUN_0045c440)
// address 0x45c440, size 304 bytes
// name confidence: 0.5 (still FUN_0045c440 in Ghidra; types/game.h's own player-struct notes
//   cite this exact address as the writer of unknown_70/74/78/7c, and
//   game_engine_definition::player_new_life documents 0x45c440 as its implementation)
// rewrite confidence: 0.45
// evidence: types/game.h player (unknown_70/74/78/7c seeded -1, speed 0x6c, objective_time
//   0xc4, team 0x20, team_index 0x66, team_index_desired 0x67), game_variant::teams (0x34,
//   live copy at 0x006f1cbc), game_engine_auto_team_counter (0x0087aa04), current_game_engine
//   (0x006f1d20, player_new_life slot at +0x14); data_array player_data (0x0087a480).
// register convention: __cdecl, the player handle is a genuine stack parameter (Ghidra's own
//   "FUN_0045c440(uint param_1)", no register-arg reconstruction needed).
//
// UNSURE: the two & 0x80000001 / correction sequences in the original are the classic compiled
// form of a signed `% 2` and are rewritten as such (verified bit-for-bit against several inputs);
// see the #if 0 block for the literal version. The team-disabled branch falls through to the
// same `game_engine_auto_team_counter += 1` line the auto-assign branch's correction uses --
// preserved exactly, even though it looks like an unrelated side effect of resetting a player
// whose game has teams turned off. The kill-feed loop's `local_8` sentinel is read but never
// reassigned inside the loop in Ghidra's own decompilation (mirroring the same
// data_iterator_next-elided-argument pattern already flagged in cheat_get_target_object_index.c);
// transcribed as-is rather than guessing at the missing iterator plumbing. The top-of-function
// field reset does not itself guard `player_handle != 0xffffffff`, even though the later
// kill-feed loop treats -1 as a sentinel; not fixed here.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data;                     // 0x0087a480
extern game_engine_definition *current_game_engine;  // 0x006f1d20
extern int16_t network_game_mode;                   // 0x00719720
extern game_variant game_engine_variant;            // 0x006f1c88 (::teams at 0x006f1cbc)
extern int32_t game_engine_auto_team_counter;       // 0x0087aa04

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module;
    // blam-cc: EDI -> iterator
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this module. CORRECTED by review: the
    // first pass declared a 4-parameter form that did not match chimera__kill_feed's own
    // definition. objdump 0x45c512..0x45c541 shows the real shape -- a data_iterator built at
    // esp+0xc, and per iteration
    //     mov edi,[esp+0x14]                     <- iterator::index, i.e. the RECIPIENT
    //     push 0 ; push esi ; push 0 ; push (ebp != esi ? ebp : edi)
    // so the four stack arguments are param_1 = the kill_feed_handle ternary below,
    // message_type = 0, subject = ESI, broadcast = 0, and the recipient rides in EDI.

// Resets a player's per-life fields (four -1 sentinels, speed back to 1.0, objective time to
// 0), then -- unless this is a network client -- assigns/derives its team, ticking the
// round-robin auto-team counter, before clearing that player's kill-feed entries and notifying
// the active game engine's player_new_life slot.
void game_engine_player_new_life(uint32_t player_handle)
{
    player *p;
    uint32_t kill_feed_handle;
    uint32_t sentinel;
    int32_t team;

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    p->hud_message_index = (datum_index)0xffffffff;
    p->hud_message_player = (datum_index)0xffffffff;
    p->speed = 1.0f;
    p->teleporter_flag_index = (datum_index)0xffffffff;
    p->nameplate_target_player = (datum_index)0xffffffff;
    p->objective_time = 0;

    if (current_game_engine == (game_engine_definition *)0) {
        return;
    }

    if (network_game_mode != 1) {
        if (game_engine_variant.teams == 0) {
            p->team_index = p->team_index_desired;
            p->team = (int32_t)p->team_index_desired;
            game_engine_auto_team_counter = game_engine_auto_team_counter + 1;
        } else if (network_game_mode == 1 || network_game_mode == 2) {
            team = (int32_t)p->team_index % 2;
            p->team = team;
        } else {
            p->team_index = (int8_t)game_engine_auto_team_counter;
            p->team = (int32_t)(int8_t)game_engine_auto_team_counter;
            game_engine_auto_team_counter = (game_engine_auto_team_counter + 1) % 2;
        }
    }

    // UNSURE: which data_array the iterator walks is not visible in Ghidra's rendering; the
    // recipient being the iterator's own current handle makes player_data the only sensible
    // reading (clear this player's kill-feed entry on every machine-local player).
    sentinel = 0xffffffff;
    {
        data_iterator iter;
        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        p = (player *)data_iterator_next(&iter);
        while (p != (player *)0) {
            kill_feed_handle = (player_handle == 0xffffffff) ? sentinel : player_handle;
            chimera__kill_feed(iter.index, kill_feed_handle, 0, (datum_index)sentinel, 0);
            p = (player *)data_iterator_next(&iter);
        }
    }

    if (current_game_engine->player_new_life != (void *)0) {
        ((void (*)(uint32_t))current_game_engine->player_new_life)(player_handle);
    }
}

#if 0
Original Ghidra decompilation (0x45c440), from tools/pack.py 0x45c440:

void FUN_0045c440(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint local_8;

  iVar2 = DAT_0087a480;
  iVar3 = (param_1 & 0xffff) * 0x200;
  iVar1 = *(int *)(DAT_0087a480 + 0x34) + iVar3;
  *(undefined4 *)(iVar1 + 0x74) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x78) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x6c) = 0x3f800000;
  *(undefined4 *)(iVar1 + 0x70) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x7c) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0xc4) = 0;
  if (DAT_006f1d20 == 0) {
    return;
  }
  iVar3 = *(int *)(iVar2 + 0x34) + iVar3;
  if (DAT_00719720 == 1) goto LAB_0045c507;
  if (DAT_006f1cbc == '\0') {
    *(char *)(iVar3 + 0x66) = *(char *)(iVar3 + 0x67);
    *(int *)(iVar3 + 0x20) = (int)*(char *)(iVar3 + 0x67);
  }
  else {
    if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
      uVar4 = (int)*(char *)(iVar3 + 0x66) & 0x80000001;
      if ((int)uVar4 < 0) {
        uVar4 = (uVar4 - 1 | 0xfffffffe) + 1;
      }
      *(uint *)(iVar3 + 0x20) = uVar4;
      goto LAB_0045c507;
    }
    *(char *)(iVar3 + 0x66) = (char)DAT_0087aa04;
    *(int *)(iVar3 + 0x20) = (int)(char)DAT_0087aa04;
    DAT_0087aa04 = DAT_0087aa04 + 1 & 0x80000001;
    if (-1 < (int)DAT_0087aa04) goto LAB_0045c507;
    DAT_0087aa04 = DAT_0087aa04 - 1 | 0xfffffffe;
  }
  DAT_0087aa04 = DAT_0087aa04 + 1;
LAB_0045c507:
  local_8 = 0xffffffff;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    uVar4 = param_1;
    if (param_1 == 0xffffffff) {
      uVar4 = local_8;
    }
    chimera__kill_feed(uVar4,0,0xffffffff,0);
    iVar2 = data_iterator_next();
  }
  if (*(code **)(DAT_006f1d20 + 0x14) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0x14))(param_1);
  }
  return;
}
#endif
