// game_engine_play_multiplayer_sound  (Ghidra: FUN_0046bd00; named per this rewrite)
// address 0x46bd00, size 115 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Validates a multiplayer sound index and, once
//   confirmed valid, optionally networks the event and then triggers full-volume local
//   playback"); types/tags.h GlobalsMultiplayerInformation::sounds; game_engine_queue_status_
//   sound_message.c (0x46bbd0, this batch); types/memory.h datum_get.
// register convention: sound index in in_EAX; recipient player handle in in_ECX; broadcast flag
//   on the stack (Ghidra's own param_1).
//   // blam-cc: EAX -> sound_index, ECX -> recipient_player, stack -> broadcast
// UNSURE: datum_get's array argument (elided here, as at several other call sites in this
//   module) is assumed to be player_data by analogy with every other datum_get call site that
//   reads a player's own fields back (here offset +2, i.e. player::local_player_index).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern Globals *global_globals; // 0x00746fa0
extern int16_t network_game_mode; // 0x00719720
extern data_array *player_data;   // 0x0087a480

extern void game_engine_queue_status_sound_message(int32_t machine_index); // 0x46bbd0, this batch
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680
extern void sound_start_unspatialized(float volume); // 0x543dd0

// blam-cc: EAX -> sound_index, ECX -> recipient_player, stack -> broadcast
// If `sound_index` names a valid GlobalsMultiplayerInformation sound, optionally sends the
// networked status message (when `broadcast`), then plays the sound locally at full volume
// unless there is a specific `recipient_player` who is not the local player.
void game_engine_play_multiplayer_sound(int32_t sound_index, datum_index recipient_player, uint8_t broadcast)
{
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    uint8_t *sound;

    if (mp_info == (GlobalsMultiplayerInformation *)0 || sound_index >= (int32_t)mp_info->sounds.count) {
        return;
    }
    sound = (uint8_t *)mp_info->sounds.pointer + sound_index * 0x10;
    if (sound == (uint8_t *)0 || *(int32_t *)(sound + 0xc) == -1) {
        return;
    }

    if (broadcast == 1) {
        game_engine_queue_status_sound_message(sound_index); // UNSURE: original call is elided;
            // this batch's own game_engine_queue_status_sound_message.c takes a machine index,
            // not a sound index -- kept as the closest already-committed sibling call shape
    }

    if (recipient_player == (datum_index)0xffffffff || network_game_mode != 2) {
        sound_start_unspatialized(1.0f);
    } else {
        player *p = (player *)datum_get(recipient_player, player_data);
        if (p != (player *)0 && p->local_player_index != -1) {
            sound_start_unspatialized(1.0f);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46bd00), from tools/pack.py 0x46bd00:

void FUN_0046bd00(char param_1)

{
  int in_EAX;
  int in_ECX;
  int iVar1;

  iVar1 = *(int *)(DAT_00746fa0 + 0x168);
  if ((((iVar1 != 0) && (in_EAX < *(int *)(iVar1 + 0x5c))) &&
      (iVar1 = in_EAX * 0x10 + *(int *)(iVar1 + 0x60), iVar1 != 0)) && (*(int *)(iVar1 + 0xc) != -1)
     ) {
    if (param_1 == '\x01') {
      FUN_0046bbd0();
    }
    if (((in_ECX == -1) || (DAT_00719720 != 2)) ||
       ((iVar1 = datum_get(), iVar1 != 0 && (*(short *)(iVar1 + 2) != -1)))) {
      FUN_00543dd0(0x3f800000);
    }
  }
  return;
}
#endif
