// game_engine_ctf_player_touch_flag  (Ghidra: FUN_00468910; named per its summary)
// address 0x468910, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Increments a player's/team's flag-touch counters and
//   fires an associated medal/event notification"); types/game.h player::unknown_c8 ("flag
//   touches; also mirrored by the profile"), player_data (0x0087a480); already-committed
//   game_engine_player_profile_cache_sync_all.c (0x466cb0, blam-cc EBX -> commit, stack ->
//   callback_extra_arg -- called here with only the stack argument visible, so `commit` is a
//   forwarded pass-through exactly like that function's own header documents) and
//   game_engine_broadcast_kill_feed_by_relationship.c (0x460c10, all five arguments visible
//   here).
// register convention: player index on the stack (param_1); team in_EAX.
//   // blam-cc: stack -> player_index, EAX -> team, EBX -> forwarded_commit
// UNSURE: forwarded_commit's identity (see the profile-cache-sync sibling).
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;   // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98

extern void game_engine_player_profile_cache_sync_all(int32_t commit, void *callback_extra_arg); // 0x466cb0
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player,
    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject); // 0x460c10

// blam-cc: stack -> player_index, EAX -> team, EBX -> forwarded_commit
// Increments the team's flag-touch counter and the player's own touch count
// (player::unknown_c8), re-syncs the player-profile cache when hosting, queues the touch
// announcer sound, and broadcasts a relationship-based kill-feed message about the touch.
void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team, int32_t forwarded_commit)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    int16_t *touch_count = (int16_t *)((uint8_t *)p + 0xc8); // player::unknown_c8

    ctf_team_flag_touch_count[team]++;
    (*touch_count)++;

    if (network_game_mode == 2) {
        game_engine_player_profile_cache_sync_all(forwarded_commit, (void *)0xffffffff);
    }
    game_engine_queue_multiplayer_sound(p->team != 0 ? 0xa : 0xd, 0xffffffff, 1); // 0x46895b..0x46896d
    game_engine_broadcast_kill_feed_by_relationship(player_index, 0x21, 0x23, 0x22, player_index);
}

#if 0
Original Ghidra decompilation (0x468910), from tools/pack.py 0x468910:

void FUN_00468910(uint param_1)

{
  short *psVar1;
  int iVar2;
  int in_EAX;

  iVar2 = *(int *)(DAT_0087a480 + 0x34);
  (&DAT_006b0e98)[in_EAX] = (&DAT_006b0e98)[in_EAX] + 1;
  psVar1 = (short *)((param_1 & 0xffff) * 0x200 + iVar2 + 200);
  *psVar1 = *psVar1 + 1;
  if (DAT_00719720 == 2) {
    FUN_00466cb0(0xffffffff);
  }
  game_engine_queue_multiplayer_sound(1);
  FUN_00460c10(param_1,0x21,0x23,0x22,param_1);
  return;
}
#endif
