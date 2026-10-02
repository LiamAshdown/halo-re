// player_update_history_play_local_player  (Ghidra: player_update_history_play_local_player,
// already named)
// address 0x4e7730, size 168 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md; types/game.h player (local_player_index at +0x02,
// unit at +0x34); types/networking.h network_client_globals (update_history at +0xf48),
// player_update_history / player_update_history_node.
// register convention: EBX -> target_update_id (unaff_EBX in the decompile; no caller is present
// in this batch to cross-check it).
//   // blam-cc: EBX -> target_update_id
// UNSURE: no caller of this function exists in this batch (callers=0), so target_update_id's
// origin could not be cross-checked. UNSURE: the node used for the replay's starting position is
// the one immediately AFTER the matched update_id, exactly as the decompile computes it (the
// search loop advances piVar2 to ->next before testing the id), not the matched node itself.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern network_client_globals *network_client; // 0x0071c2d8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern void player_update_history_play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack); // this module, 0x4e6ff0

// Finds the local player's controlled unit (if any) and, if network_client's update-history list
// has a node matching target_update_id, replays history from the node after that match using its
// saved position as the reconciliation starting point.
void player_update_history_play_local_player(int32_t target_update_id)
    // blam-cc: EBX -> target_update_id
{
    data_iterator iter;
    player *candidate;
    datum_index unit_index;
    player_update_history_node *node;
    player_update_history_node *after_match;
    int32_t node_id;

    unit_index = (datum_index)-1;
    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)data_iterator_next(&iter);
    while (candidate != 0 && candidate->local_player_index == -1) {
        candidate = (player *)data_iterator_next(&iter);
    }
    if (candidate != 0) {
        unit_index = candidate->unit;
    }

    if (network_client == 0) {
        return;
    }
    node = ((player_update_history *)network_client->update_history)->head;
    if (node == 0) {
        return;
    }
    do {
        node_id = node->update_id;
        after_match = node->next;
        node = after_match;
        if (node_id == target_update_id) {
            break;
        }
        if (node == 0) {
            return;
        }
    } while (1);
    if (after_match != 0) {
        player_update_history_play(0, 0, (player_update_history *)network_client->update_history,
            unit_index, *(float *)(after_match->unit_state + 0x00),
            *(float *)(after_match->unit_state + 0x04),
            *(float *)(after_match->unit_state + 0x08), 0);
    }
}

#if 0
Original Ghidra decompilation (0x4e7730), from tools/pack.py 0x4e7730:

void player_update_history_play_local_player(void)

{
  int iVar1;
  int *piVar2;
  int unaff_EBX;
  uint uVar3;

  uVar3 = 0xffffffff;
  iVar1 = data_iterator_next();
  do {
    if (iVar1 == 0) {
LAB_004e777a:
      if (DAT_0071c2d8 != 0) {
        piVar2 = *(int **)(*(int *)(DAT_0071c2d8 + 0xf48) + 4);
        if (piVar2 != (int *)0x0) {
          while (iVar1 = *piVar2, piVar2 = (int *)piVar2[0x105], iVar1 != unaff_EBX) {
            if (piVar2 == (int *)0x0) {
              return;
            }
          }
          if (piVar2 != (int *)0x0) {
            player_update_history_play
                      (*(int *)(DAT_0071c2d8 + 0xf48),uVar3,(float)piVar2[0xc],(float)piVar2[0xd],
                       (float)piVar2[0xe],0);
          }
        }
      }
      return;
    }
    if (*(short *)(iVar1 + 2) != -1) {
      uVar3 = *(uint *)(iVar1 + 0x34);
      goto LAB_004e777a;
    }
    iVar1 = data_iterator_next();
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
