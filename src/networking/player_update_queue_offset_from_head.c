// player_update_queue_offset_from_head  (Ghidra: FUN_004e6aa0; named per this rewrite)
// address 0x4e6aa0, size 98 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md ("Computes how far a given sequence number is
// ahead of the head of a player's pending-update circular queue, used to decide whether to queue
// or immediately apply an incoming update."); types/game.h player_update_queue (embeds
// circular_queue at player+0x120: capacity/record_size/records/write_index/read_index/storage);
// player_update_record (field0 at +0x00); the caller
// player_update_client_remote_player_position_update_from_network (out/phase2/networking/05.md)
// calls this with unaff_EDI already pointed at a player, confirming plr+0x120.. is the queue
// this reads.
// register convention: EDI -> plr (unaff_EDI in the decompile), EDX -> new_update_id.
//   // blam-cc: EDI -> plr, EDX -> new_update_id
// UNSURE: record->field0 is documented in types/game.h as "forwarded whole to 0x476cf0" and left
// generic; this function's own modulo-0x40 wraparound arithmetic strongly suggests it is really a
// 0..63 sequence id, matching k_network_update_history_maximum, but that is not re-derived here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// Returns how many steps ahead new_update_id is of the sequence id at the head of plr's
// player_update_queue (wrapping modulo 64), or -1 if the queue currently holds nothing to
// compare against.
int32_t player_update_queue_offset_from_head(player *plr, int32_t new_update_id)
    // blam-cc: EDI -> plr, EDX -> new_update_id
{
    circular_queue *queue;
    int32_t used;
    player_update_record *head_record;
    int32_t head_id;

    queue = &plr->update_history.queue;
    if (queue->read_index < queue->write_index) {
        used = -queue->read_index;
    } else {
        if (queue->read_index <= queue->write_index) {
            return -1;
        }
        used = queue->capacity - queue->read_index;
    }
    if (queue->write_index + used < 1) {
        return -1;
    }
    if (queue->read_index == queue->write_index) {
        head_record = 0;
    } else {
        head_record = (player_update_record *)queue->records[queue->read_index];
    }
    head_id = head_record->field0;
    if (head_id <= new_update_id) {
        if (new_update_id <= head_id) {
            return 0;
        }
        return new_update_id - head_id;
    }
    return (new_update_id - head_id) + 0x40;
}

#if 0
Original Ghidra decompilation (0x4e6aa0), from tools/pack.py 0x4e6aa0:

int FUN_004e6aa0(void)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int in_EDX;
  int unaff_EDI;

  iVar1 = *(int *)(unaff_EDI + 300);
  iVar3 = *(int *)(unaff_EDI + 0x130);
  if (iVar3 < iVar1) {
    iVar3 = -iVar3;
  }
  else {
    if (iVar3 <= iVar1) {
      return -1;
    }
    iVar3 = *(int *)(unaff_EDI + 0x120) - iVar3;
  }
  if (iVar1 + iVar3 < 1) {
    return -1;
  }
  if (*(int *)(unaff_EDI + 0x130) == *(int *)(unaff_EDI + 300)) {
    piVar2 = (int *)0x0;
  }
  else {
    piVar2 = *(int **)(*(int *)(unaff_EDI + 0x128) + *(int *)(unaff_EDI + 0x130) * 4);
  }
  iVar1 = *piVar2;
  if (iVar1 <= in_EDX) {
    if (in_EDX <= iVar1) {
      return 0;
    }
    return in_EDX - iVar1;
  }
  return (in_EDX - iVar1) + 0x40;
}
#endif
