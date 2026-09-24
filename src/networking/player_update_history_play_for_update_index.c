// player_update_history_play_for_update_index  (Ghidra: player_update_history_play_for_update_index,
// already named)
// address 0x4e6950, size 86 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; types/memory.h data_array (data at +0x34);
// types/game.h player (unit at +0x34, unknown_f0/f4/f8); types/networking.h network_client_globals
// (update_history at +0xf48).
// register convention: EAX -> player_index (low 16 bits used as the index into player_data).
//   // blam-cc: EAX -> player_index
// UNSURE: the final argument to player_update_history_play (0) is passed through verbatim; its
// meaning is not established here (see player_update_history_play.c for that function's own
// analysis of its last parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480
extern network_client_globals *network_client; // 0x0071c2d8

extern int32_t player_update_history_play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack); // this module, 0x4e6ff0;
    // the first two are the AL/ECX register arguments Ghidra drops at every call site

// Resolves the player at player_index and replays the client's update-history against its
// current unit and cached position, using the client's global update_history list.
void player_update_history_play_for_update_index(datum_index player_index)
    // blam-cc: EAX -> player_index
{
    player *plr;

    plr = (player *)((uint8_t *)player_data->data + (uint16_t)player_index * player_data->size);
    player_update_history_play(0, 0, network_client->update_history, plr->unit,
        *(float *)&plr->unknown_f0, *(float *)&plr->unknown_f4, *(float *)&plr->unknown_f8, 0);
        // UNSURE: the AL/ECX prune pair is not visible at this call site; 0/0 preserved
}

#if 0
Original Ghidra decompilation (0x4e6950), from tools/pack.py 0x4e6950:

void player_update_history_play_for_update_index(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  player_update_history_play
            (*(int *)(DAT_0071c2d8 + 0xf48),*(uint *)(iVar1 + 0x34),*(float *)(iVar1 + 0xf0),
             *(float *)(iVar1 + 0xf4),*(float *)(iVar1 + 0xf8),0);
  return;
}
#endif
