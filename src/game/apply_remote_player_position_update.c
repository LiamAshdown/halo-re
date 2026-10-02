// apply_remote_player_position_update  (Ghidra: apply_remote_player_position_update, already
// named)
// address 0x477350, size 318 bytes
// name confidence: 0.7   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Reconciles a remote player's object position against
//   a queued network snapshot, logging drift/desync and forcing a resync when the discrepancy
//   is too large"); CEA-pdb string match on both format strings; types/game.h player
//   (name +0x04, position_updates +0x170); types/objects.h object (position +0x5c,
//   parent_object +0x11c, network_role +0x04); position_update_queue_find_and_remove.c (a later
//   batch of this module) for the queue-pop helper. objdump confirms EBX (the unit object
//   pointer) survives unchanged into this call from its one caller in this batch
//   (player_apply_first_position_update.c's own object_try_and_get result), so it is a second,
//   register-passed parameter Ghidra's "void apply_remote_player_position_update(void)" drops
//   entirely.
// register convention: EAX -> plr, EBX -> unit_obj.
//   // blam-cc: EAX -> plr, EBX -> unit_obj
//
// UNSURE: DAT_0071c420 (a wide string constant compared against the player's own name) and
// DAT_007102dc (a wait-tick counter) are not attested elsewhere in this module. The failure-path
// record dereference (`**(int **)(queue->records + read_index*4)`) is transcribed literally.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t wait_tick_counter;           // 0x007102dc, UNSURE name
extern uint16_t local_player_name_filter[]; // 0x0071c420, UNSURE name/purpose

extern uint8_t position_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick,
    real_point3d *out); // this module (a later batch), 0x47a100
extern void player_update_history_log_printf_filtered(int32_t level, const char *format, ...); // 0x4e5f20
extern void unit_snap_position_if_far(real_point3d *new_position, object *obj); // this batch, 0x4772e0
extern double sqrt(double x); // x87 FSQRT

// Tries to pop the queued position update matching unit_obj's own recorded network tick
// (unit_obj+0x4bc). On success, logs the distance between the queued and current position and
// how long it waited, resets the wait counter if this is the "filtered" player's own name, then
// bumps the queue's running wait-count/distance totals; if the unit is unparented and its
// network_role is 1, snaps its position (unit_snap_position_if_far). On failure -- if the head
// of the queue is not simply "not yet due" -- logs a "can't update" message with whatever record
// is now at the head, and bumps the wait counter for the filtered player's own name.
void apply_remote_player_position_update(player *plr, object *unit_obj)
    // blam-cc: EAX -> plr, EBX -> unit_obj
{
    real_point3d queued;
    uint8_t found;
    int32_t target_tick = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);

    found = position_update_queue_find_and_remove(&plr->position_updates, target_tick, &queued);
    if (found == 1) {
        float dx = queued.x - unit_obj->position.x;
        float dy = queued.y - unit_obj->position.y;
        float dz = queued.z - unit_obj->position.z;
        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

        player_update_history_log_printf_filtered(1, "Waited [%d], dist [%f].", wait_tick_counter, (double)dist);
        if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
            wait_tick_counter = 0;
        }
        plr->position_updates_applied_count = plr->position_updates_applied_count + 1;
        *(float *)&plr->position_update_error_total = dist + *(float *)&plr->position_update_error_total;

        if (unit_obj->parent_object == (datum_index)-1 && unit_obj->network_role == 1) {
            unit_snap_position_if_far(&queued, unit_obj);
        }
    } else {
        circular_queue *queue = &plr->position_updates;
        int32_t write_index = queue->write_index;
        int32_t read_index = queue->read_index;
        int32_t distance;

        if (read_index < write_index) {
            distance = -read_index;
        } else {
            if (read_index <= write_index) { // queue empty
                return;
            }
            distance = queue->capacity - read_index;
        }

        if (write_index + distance > 0) {
            int32_t *head_record = *(int32_t **)((uint8_t *)queue->records + read_index * 4);
            player_update_history_log_printf_filtered(1, "Can't update pos: [%d] != [%d]",
                                                        target_tick, *head_record);
            if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
                wait_tick_counter = wait_tick_counter + 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x477350), from tools/pack.py 0x477350:

void apply_remote_player_position_update(void)

{
  float fVar1;
  char cVar2;
  int in_EAX;
  int iVar3;
  int iVar4;
  int unaff_EBX;
  float local_c;
  float local_8;
  float local_4;

  cVar2 = FUN_0047a100((int *)(in_EAX + 0x170),*(undefined4 *)(unaff_EBX + 0x4bc),&local_c);
  if (cVar2 == '\x01') {
    local_c = local_c - *(float *)(unaff_EBX + 0x5c);
    local_8 = local_8 - *(float *)(unaff_EBX + 0x60);
    local_4 = local_4 - *(float *)(unaff_EBX + 100);
    fVar1 = SQRT(local_c * local_c + local_8 * local_8 + local_4 * local_4);
    player_update_history_log_printf_filtered
              (1,"Waited [%d], dist [%f].",DAT_007102dc,(double)fVar1);
    iVar3 = _wcscmp((wchar_t *)(in_EAX + 4),&DAT_0071c420);
    if (iVar3 == 0) {
      DAT_007102dc = 0;
    }
    *(int *)(in_EAX + 0x1ec) = *(int *)(in_EAX + 0x1ec) + 1;
    *(float *)(in_EAX + 0x1f0) = fVar1 + *(float *)(in_EAX + 0x1f0);
    if ((*(int *)(unaff_EBX + 0x11c) == -1) && (*(int *)(unaff_EBX + 4) == 1)) {
      unit_snap_position_if_far();
      return;
    }
  }
  else {
    iVar3 = *(int *)(in_EAX + 0x17c);
    iVar4 = *(int *)(in_EAX + 0x180);
    if (iVar4 < iVar3) {
      iVar4 = -iVar4;
    }
    else {
      if (iVar4 <= iVar3) {
        return;
      }
      iVar4 = *(int *)(in_EAX + 0x170) - iVar4;
    }
    if (0 < iVar3 + iVar4) {
      player_update_history_log_printf_filtered
                (1,"Can\'t update pos: [%d] != [%d]",*(undefined4 *)(unaff_EBX + 0x4bc),
                 **(undefined4 **)(*(int *)(in_EAX + 0x178) + *(int *)(in_EAX + 0x180) * 4));
      iVar3 = _wcscmp((wchar_t *)(in_EAX + 4),&DAT_0071c420);
      if (iVar3 == 0) {
        DAT_007102dc = DAT_007102dc + 1;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
