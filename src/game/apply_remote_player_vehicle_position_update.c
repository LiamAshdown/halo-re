// apply_remote_player_vehicle_position_update  (Ghidra:
// apply_remote_player_vehicle_position_update, already named)
// address 0x477490, size 428 bytes
// name confidence: 0.7   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Reconciles a remote player's vehicle
//   position/orientation against queued network updates, snapping the vehicle's transform when
//   the client and host disagree"); CEA-pdb string match; types/game.h player
//   (vehicle_updates +0x1d0, unknown_1f8/500(0x1f4) counters); types/objects.h object (position
//   +0x5c, velocity +0x68, angular_velocity +0x8c, forward +0x74, up +0x80, parent_object
//   +0x11c); vehicle_update_queue_find_and_remove.c (a later batch of this module) for the
//   established record shape (tick, sequence, 16-dword body).
// register convention: EAX -> plr, EBX -> unit_obj (matching apply_remote_player_position_update,
//   this batch; not independently re-disassembled here given the strong structural parallel).
//   // blam-cc: EAX -> plr, EBX -> unit_obj
//
// UNSURE (heavy): the popped record's first body dword is compared against unit_obj's own
// parent_object and then re-used as the object handle for object_try_and_get -- named here
// `parent_or_tag` since its exact role is not established. unit_propagate_position_delta_to_children (units module, not in
// this batch) is called with no recovered arguments. DAT_007102e0 mirrors
// apply_remote_player_position_update's own wait-counter pattern.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

// UNSURE: every field of vehicle_update_body is inferred from this function's own use of it
// (tick, sequence, 16-dword body), with this function's own view of the body's fields.
// vehicle_update_body / vehicle_update_record are types/game.h's (0x40 / 0x48).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t vehicle_wait_tick_counter;   // 0x007102e0, UNSURE name
extern uint16_t local_player_name_filter[]; // 0x0071c420, UNSURE name/purpose

extern uint8_t vehicle_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick,
    vehicle_update_record *out); // this module (a later batch), 0x47a2c0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void player_update_history_log_printf_filtered(int32_t level, const char *format, ...); // 0x4e5f20
extern void unit_propagate_position_delta_to_children(void); // 0x570cb0, units module, not in this batch; UNSURE args
extern double sqrt(double x); // x87 FSQRT

// Tries to pop the queued vehicle update matching unit_obj's own recorded network tick
// (unit_obj+0x4bc). On success, if the record's parent_or_tag matches unit_obj->parent_object
// and that object can be fetched, logs the distance moved, bumps the running wait-count/distance
// totals, calls unit_propagate_position_delta_to_children, and overwrites the parent object's velocity, angular_velocity,
// forward and up vectors from the record. Either way, resets the wait counter if this is the
// filtered player's own name. On failure, if the head of the queue is not simply "not yet due"
// and the queue is not empty, logs a "can't update" message and bumps the wait counter.
void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj)
    // blam-cc: EAX -> plr, EBX -> unit_obj
{
    vehicle_update_record record;
    int32_t target_tick = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);
    uint8_t found = vehicle_update_queue_find_and_remove(&plr->vehicle_updates, target_tick, &record);

    if (found == 1) {
        if (unit_obj->parent_object == (datum_index)record.body.parent_or_tag) {
            object *parent_obj = object_try_and_get((datum_index)record.body.parent_or_tag, 0xffffffff); // UNSURE mask
            if (parent_obj != (object *)0) {
                float dx = record.body.position.x - parent_obj->position.x;
                float dy = record.body.position.y - parent_obj->position.y;
                float dz = record.body.position.z - parent_obj->position.z;
                float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

                player_update_history_log_printf_filtered(1, "Vehicle waited [%d], dist [%f].",
                                                            vehicle_wait_tick_counter, (double)dist);
                // UNSURE: player::unknown_1f8 is documented as int32_t; the disassembly treats
                // it as a running float total (fVar2 + existing value).
                *(float *)&plr->vehicle_update_error_total = dist + *(float *)&plr->vehicle_update_error_total;
                plr->vehicle_updates_applied_count = plr->vehicle_updates_applied_count + 1;
                unit_propagate_position_delta_to_children();
                parent_obj->velocity = record.body.velocity;
                parent_obj->angular_velocity = record.body.angular_velocity;
                parent_obj->forward = record.body.forward;
                parent_obj->up = record.body.up;
            }
        }
        if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
            vehicle_wait_tick_counter = 0;
        }
    } else {
        circular_queue *queue = &plr->vehicle_updates;
        int32_t write_index = queue->write_index;
        int32_t read_index = queue->read_index;
        int32_t distance;

        if (read_index < write_index) {
            distance = write_index - read_index;
        } else {
            if (read_index <= write_index) {
                return;
            }
            distance = (write_index - read_index) + queue->capacity;
        }

        if (distance > 0 && read_index != write_index) {
            int32_t *head_record = *(int32_t **)((uint8_t *)queue->records + read_index * 4);
            player_update_history_log_printf_filtered(1, "Can't update pos: [%d] != [%d]",
                                                        target_tick, *head_record);
            if (wcscmp((const wchar_t *)((uint16_t *)plr->name), (const wchar_t *)local_player_name_filter) == 0) {
                vehicle_wait_tick_counter = vehicle_wait_tick_counter + 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x477490), from tools/pack.py 0x477490:

void apply_remote_player_vehicle_position_update(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  int in_EAX;
  int iVar6;
  int unaff_EBX;
  int local_40;
  float local_3c;
  float local_38;
  float local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  cVar5 = FUN_0047a2c0((int *)(in_EAX + 0x1d0),*(undefined4 *)(unaff_EBX + 0x4bc));
  if (cVar5 == '\x01') {
    if ((*(int *)(unaff_EBX + 0x11c) == local_40) && (iVar6 = object_try_and_get(), iVar6 != 0)) {
      fVar2 = local_3c - *(float *)(iVar6 + 0x5c);
      fVar4 = local_38 - *(float *)(iVar6 + 0x60);
      fVar3 = local_34 - *(float *)(iVar6 + 100);
      fVar2 = SQRT(fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3);
      player_update_history_log_printf_filtered
                (1,"Vehicle waited [%d], dist [%f].",DAT_007102e0,(double)fVar2);
      *(float *)(in_EAX + 0x1f8) = fVar2 + *(float *)(in_EAX + 0x1f8);
      *(int *)(in_EAX + 500) = *(int *)(in_EAX + 500) + 1;
      FUN_00570cb0();
      *(undefined4 *)(iVar6 + 0x68) = local_30;
      *(undefined4 *)(iVar6 + 0x6c) = local_2c;
      *(undefined4 *)(iVar6 + 0x70) = local_28;
      *(undefined4 *)(iVar6 + 0x8c) = local_24;
      *(undefined4 *)(iVar6 + 0x90) = local_20;
      *(undefined4 *)(iVar6 + 0x94) = local_1c;
      *(undefined4 *)(iVar6 + 0x74) = local_18;
      *(undefined4 *)(iVar6 + 0x78) = local_14;
      *(undefined4 *)(iVar6 + 0x7c) = local_10;
      *(undefined4 *)(iVar6 + 0x80) = local_c;
      *(undefined4 *)(iVar6 + 0x84) = local_8;
      *(undefined4 *)(iVar6 + 0x88) = local_4;
    }
    iVar6 = _wcscmp((wchar_t *)(in_EAX + 4),&DAT_0071c420);
    if (iVar6 == 0) {
      DAT_007102e0 = iVar6;
      return;
    }
  }
  else {
    iVar6 = *(int *)(in_EAX + 0x1dc);
    iVar1 = *(int *)(in_EAX + 0x1e0);
    if (iVar1 < iVar6) {
      iVar6 = iVar6 - iVar1;
    }
    else {
      if (iVar1 <= iVar6) {
        return;
      }
      iVar6 = (iVar6 - iVar1) + *(int *)(in_EAX + 0x1d0);
    }
    if ((0 < iVar6) && (*(int *)(in_EAX + 0x1e0) != *(int *)(in_EAX + 0x1dc))) {
      player_update_history_log_printf_filtered
                (1,"Can\'t update pos: [%d] != [%d]",*(undefined4 *)(unaff_EBX + 0x4bc),
                 **(undefined4 **)(*(int *)(in_EAX + 0x1d8) + *(int *)(in_EAX + 0x1e0) * 4));
      iVar6 = _wcscmp((wchar_t *)(in_EAX + 4),&DAT_0071c420);
      if (iVar6 == 0) {
        DAT_007102e0 = DAT_007102e0 + 1;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
