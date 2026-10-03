// players_client_catchup_on_server_updates  (Ghidra: players_client_catchup_on_server_updates,
// already named)
// address 0x476d40, size 1224 bytes
// VERIFIED against disassembly 0x476d40..0x47720a (2026-09-30)
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Client-side routine that drains a player's buffered
//   server updates, fast-forwarding position/orientation state until it matches the latest
//   server tick"); types/game.h player_update_queue / circular_queue (capacity +0x120,
//   record_size +0x124, records +0x128, write_index +0x12c, read_index +0x130, has_current
//   +0x138, current[8] +0x13c) at player+0x120; types/units.h units_module note that 0x5625b0
//   is the real unit_update and 0x5590a0 is biped_update.
//
// REWRITTEN 2026-09-30 from the disassembly (the earlier draft was "the least-verified file in the batch"). What was wrong:
//  - The queue count is `write - read` (write > read), `capacity - read + write` (write < read), 0 when equal; the draft had the
//    two indices swapped throughout, so it read the WRONG head record (records[write] instead of records[read]) and
//    summed ticks from write to read instead of read to write.
//  - The record is only pre-filled with -1 in its first THREE dwords (not all eleven).
//  - player_unit_has_parent takes the player handle in ECX (iterator index at 0x477157; the unit object's controlling_player at
//    0x476f4a), not plr->unit.
//  - The staged control block: when input is enabled it is built from the popped player_action (control flags, yaw/pitch ->
//    player_compute_view_forward_vector(EAX handle, ECX &action.desired_yaw, ESI out) -> the same vector for the aiming, facing
//    and looking vectors, throttle x/y, trigger, weapon/grenade/zoom); when input is disabled it is built from the unit's own
//    desired vectors, and ONLY when the unit has no actor and no swarm actor. unit_apply_control_block takes
//    (EAX unit, EDX control, stack source_id -1); the draft's 2-argument extern was wrong.
//  - The final log call takes the player in EAX: player_update_history_log_printf_filtered(plr, 1, fmt, ...).
// The thresholds at 0x6887bc / 0x6887c0 hold 2 and 6 in the retail data; 0x6894a1 holds 1.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;      // 0x0087a480
extern data_array *object_data;      // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern int16_t network_game_mode;    // 0x00719720
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t catchup_backlog_threshold;   // 0x006887bc (2): queued records allowed before a catch-up pop
extern int32_t catchup_time_threshold;      // 0x006887c0 (6): summed record tick weights allowed
extern uint8_t network_client_vehicle_ack_enabled; // 0x006894a1
extern real_vector3d *global_origin3d_pointer; // 0x00696714 -> {0,0,0}

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
                                                real_vector3d *out_forward); // 0x473d70, blam-cc: EAX handle, ECX yaw_pitch, ESI out
extern uint8_t player_unit_has_parent(datum_index player_handle); // 0x477210, blam-cc: ECX
extern void apply_remote_player_position_update(player *plr, object *unit_obj); // 0x477350, blam-cc: EAX -> plr, EBX -> unit_obj
extern void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj); // 0x477490, blam-cc: EAX -> plr, EBX -> unit_obj
extern void player_update_history_log_printf_filtered(player *target_player, int32_t unused_arg,
    const char *format, ...); // 0x4e5f20, blam-cc: EAX target_player
extern void object_update(uint32_t object_index); // 0x4f7ef0
extern uint8_t unit_update(uint32_t unit_index); // 0x5625b0, established (units module)
extern uint32_t biped_update(uint32_t object_index); // 0x5590a0, established (units module)
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id); // 0x5639f0, blam-cc: EAX unit_index, EDX control, stack source_id

// Number of records waiting in a player's update ring (write - read, wrapping at capacity).
static int32_t update_queue_count(const circular_queue *queue)
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;

    if (write_index > read_index) {
        return write_index - read_index;
    }
    if (write_index < read_index) {
        return (queue->capacity - read_index) + write_index;
    }
    return 0;
}

// For every non-local player, drains its update_history circular queue: while the backlog
// (queued record count, or the summed per-record tick weight) exceeds the catch-up thresholds,
// pops the head record, replays it into the player's controlled unit (rebuilding a
// unit_control_data and running the ordinary per-tick update, or an object_update of the parent
// while a network client with a seated unit), and applies a remote position/vehicle update.
// Once caught up, logs how many updates/ticks were skipped.
void players_client_catchup_on_server_updates(void)
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)data_iterator_next(&iter);
    while (plr != (player *)0) {
        int32_t updates_applied = 0;

        if (plr->local_player_index == -1) {
            circular_queue *queue = &plr->update_history.queue;
            int32_t initial_backlog = update_queue_count(queue);

            for (;;) {
                player_update_record record;
                int32_t read_index = queue->read_index;
                int32_t write_index = queue->write_index;

                if (update_queue_count(queue) <= catchup_backlog_threshold) {
                    int32_t summed_ticks = 0;
                    int32_t i = read_index;

                    if (i != write_index) {
                        do {
                            summed_ticks = summed_ticks +
                                ((player_update_record **)queue->records)[i]->references_remaining;
                            i = (i + 1) % 0x78;
                        } while (i != queue->write_index);
                    }
                    if (summed_ticks <= catchup_time_threshold) {
                        break; // caught up
                    }
                }

                // 0x476e2d: only the first three dwords of the local copy are pre-filled
                record.field0 = 0xffffffff;
                record.references_remaining = -1;
                record.reference_count = -1;

                if (read_index != write_index) {
                    player_update_record *head = ((player_update_record **)queue->records)[read_index];

                    head->references_remaining = head->references_remaining - 1;
                    if (head->references_remaining == 0) {
                        // retire the record: advance the read cursor (read != write here, so the original's
                        // "queue emptied under us" path at 0x476f6a is unreachable)
                        queue->read_index = (read_index + 1) % queue->capacity;
                    }
                    memcpy(&record, head, sizeof(record));
                    plr->update_history.has_current = 1;
                    memcpy(plr->update_history.current, &record.action, sizeof(plr->update_history.current));
                }

                updates_applied = updates_applied + 1;

                // The first consumer of a record (remaining == total - 1) applies its position update
                if (record.references_remaining == record.reference_count - 1 && network_game_mode == 1 &&
                    plr->local_player_index == -1 && plr->unit != (datum_index)-1) {
                    int16_t index = (int16_t)plr->unit;
                    int16_t salt = (int16_t)((uint32_t)plr->unit >> 16);
                    object_header *header = 0;

                    if (index >= 0 && index < object_data->maximum_count) {
                        object_header *candidate = (object_header *)((uint8_t *)object_data->data +
                                                                     (int32_t)object_data->size * index);

                        if (candidate->identifier != 0 && (salt == 0 || candidate->identifier == salt)) {
                            header = candidate;
                        }
                    }
                    if (header != 0 && (((1u << (header->type & 0x1f)) & _object_mask_unit) != 0) &&
                        header->data != 0) {
                        object *unit_obj = header->data;
                        uint8_t seated = player_unit_has_parent(*(datum_index *)((uint8_t *)unit_obj + 0x218)); // controlling_player

                        *(uint32_t *)((uint8_t *)unit_obj + 0x4bc) = record.field0;
                        if (seated == 0) {
                            apply_remote_player_position_update(plr, unit_obj);
                        } else {
                            apply_remote_player_vehicle_position_update(plr, unit_obj);
                        }
                    }
                }

                if (plr->unit == (datum_index)-1) {
                    continue;
                }

                {
                    object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
                    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                    unit_control_data control;
                    uint8_t apply = 0;

                    if ((unit->flags & 0x40) == 0) {
                        continue;
                    }

                    memset(&control, 0, sizeof(control));
                    control.animation_state = 3;
                    control.aiming_speed = 0;
                    if (local_player_globals->input_disabled == 0) {
                        const player_action *action = &record.action;

                        control.control_flags = (uint16_t)action->control_flags;
                        control.weapon_index = action->weapon_index;
                        control.grenade_index = action->grenade_index;
                        control.zoom_level = action->zoom_level;
                        control.throttle.i = action->throttle_x;
                        control.throttle.j = action->throttle_y;
                        control.throttle.k = 0.0f;
                        control.primary_trigger = action->primary_trigger;
                        // the view forward vector lands in the aiming vector, then is copied to facing and looking
                        player_compute_view_forward_vector(iter.index, (real *)&action->desired_yaw,
                                                           &control.aiming_vector);
                        control.facing_vector = control.aiming_vector;
                        control.looking_vector = control.aiming_vector;
                        apply = 1;
                    } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                        control.control_flags = 0;
                        control.weapon_index = -1;
                        control.grenade_index = -1;
                        control.zoom_level = -1;
                        control.throttle = *global_origin3d_pointer;
                        control.primary_trigger = 0.0f;
                        control.facing_vector = unit->desired_facing_vector;
                        control.aiming_vector = unit->desired_aiming_vector;
                        control.looking_vector = unit->desired_looking_vector;
                        apply = 1;
                    }
                    if (apply) {
                        unit_apply_control_block(plr->unit, &control, -1);
                    }

                    if (player_unit_has_parent(iter.index) != 0 && network_client_vehicle_ack_enabled != 0) {
                        object_update((uint32_t)unit_obj->parent_object);
                    } else {
                        unit_update(plr->unit);
                        biped_update(plr->unit);
                    }
                }
            }

            if (updates_applied > 0) {
                int32_t remaining_backlog = update_queue_count(queue);

                player_update_history_log_printf_filtered(
                    plr, 1, "[%d]: Caught up on [%d] updates == [%d] ticks.\n", game_time->game_time,
                    initial_backlog - remaining_backlog, updates_applied);
            }
        }

        plr = (player *)data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x476d40), from tools/pack.py 0x476d40:

void players_client_catchup_on_server_updates(void)

{
  int iVar1;
  uint uVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  short sVar10;
  int *piVar11;
  int *piVar12;
  int *piVar13;
  short *psVar14;
  int local_ec;
  int local_e8;
  int local_50 [8];
  int local_30 [12];

  iVar4 = data_iterator_next();
joined_r0x00476d78:
  if (iVar4 == 0) {
    return;
  }
  local_e8 = 0;
  if (*(short *)(iVar4 + 2) == -1) {
    local_ec = *(int *)(iVar4 + 300);
    iVar9 = *(int *)(iVar4 + 0x130);
    if (iVar9 < local_ec) {
      local_ec = local_ec - iVar9;
    }
    else if (local_ec < iVar9) {
      local_ec = (*(int *)(iVar4 + 0x120) - iVar9) + local_ec;
    }
    else {
      local_ec = 0;
    }
LAB_00476dc0:
    iVar9 = *(int *)(iVar4 + 300);
    iVar1 = *(int *)(iVar4 + 0x130);
    if (iVar1 < iVar9) {
      iVar5 = iVar9 - iVar1;
    }
    else if (iVar9 < iVar1) {
      iVar5 = (*(int *)(iVar4 + 0x120) - iVar1) + iVar9;
    }
    else {
      iVar5 = 0;
    }
    if (iVar5 <= DAT_006887bc) {
      iVar8 = 0;
      iVar5 = iVar1;
      if (iVar1 != iVar9) {
        do {
          iVar8 = iVar8 + *(int *)(*(int *)(*(int *)(iVar4 + 0x128) + iVar5 * 4) + 4);
          iVar5 = (iVar5 + 1) % 0x78;
        } while (iVar5 != *(int *)(iVar4 + 300));
      }
      if (iVar8 <= DAT_006887c0) goto LAB_0047719e;
    }
    local_30[0] = -1;
    local_30[1] = -1;
    local_30[2] = -1;
    if (iVar1 != iVar9) {
      piVar11 = *(int **)(*(int *)(iVar4 + 0x128) + iVar1 * 4);
      piVar12 = piVar11 + 1;
      *piVar12 = *piVar12 + -1;
      if (*piVar12 == 0) {
        iVar9 = *(int *)(iVar4 + 0x130);
        if (iVar9 == *(int *)(iVar4 + 300)) {
          piVar11 = (int *)0x0;
        }
        else {
          piVar11 = *(int **)(*(int *)(iVar4 + 0x128) + iVar9 * 4);
          *(int *)(iVar4 + 0x130) = (iVar9 + 1) % *(int *)(iVar4 + 0x120);
        }
      }
      piVar12 = piVar11;
      piVar13 = local_30;
      for (iVar9 = 0xb; iVar9 != 0; iVar9 = iVar9 + -1) {
        *piVar13 = *piVar12;
        piVar12 = piVar12 + 1;
        piVar13 = piVar13 + 1;
      }
      *(undefined1 *)(iVar4 + 0x138) = 1;
      piVar12 = piVar11 + 3;
      piVar11 = (int *)(iVar4 + 0x13c);
      for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
        *piVar11 = *piVar12;
        piVar12 = piVar12 + 1;
        piVar11 = piVar11 + 1;
      }
    }
    iVar9 = local_30[0];
    local_e8 = local_e8 + 1;
    if ((((local_30[1] == local_30[2] + -1) && (DAT_00719720 == 1)) && (*(short *)(iVar4 + 2) == -1)
        ) && (iVar1 = *(int *)(iVar4 + 0x34), iVar1 != -1)) {
      psVar14 = (short *)0x0;
      sVar7 = (short)iVar1;
      if ((-1 < sVar7) && (sVar7 < *(short *)(DAT_008603b0 + 0x20))) {
        psVar6 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar7 +
                          *(int *)(DAT_008603b0 + 0x34));
        sVar7 = *psVar6;
        if ((sVar7 != 0) &&
           ((sVar10 = (short)((uint)iVar1 >> 0x10), sVar10 == 0 || (sVar7 == sVar10)))) {
          psVar14 = psVar6;
        }
      }
      if (((psVar14 != (short *)0x0) && ((1 << (*(byte *)((int)psVar14 + 3) & 0x1f) & 3U) != 0)) &&
         (iVar1 = *(int *)(psVar14 + 4), iVar1 != 0)) {
        cVar3 = FUN_00477210();
        *(int *)(iVar1 + 0x4bc) = iVar9;
        if (cVar3 == '\0') {
          apply_remote_player_position_update();
        }
        else {
          apply_remote_player_vehicle_position_update();
        }
      }
    }
    uVar2 = *(uint *)(iVar4 + 0x34);
    piVar12 = local_30 + 3;
    piVar11 = local_50;
    for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
      *piVar11 = *piVar12;
      piVar12 = piVar12 + 1;
      piVar11 = piVar11 + 1;
    }
    if ((uVar2 != 0xffffffff) &&
       (iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc),
       (*(uint *)(iVar9 + 0x204) >> 6 & 1) != 0)) {
      if (*(char *)(DAT_0087a478 + 0x11) == '\0') {
        FUN_00473d70();
LAB_0047714f:
        FUN_005639f0(0xffffffff);
      }
      else if ((*(int *)(iVar9 + 0x1f8) == -1) && (*(int *)(iVar9 + 500) == -1)) goto LAB_0047714f;
      cVar3 = FUN_00477210();
      if ((cVar3 == '\0') || (DAT_006894a1 == '\0')) {
        FUN_005625b0(*(undefined4 *)(iVar4 + 0x34));
        unit_update(*(undefined4 *)(iVar4 + 0x34));
      }
      else {
        object_update(*(undefined4 *)(iVar9 + 0x11c));
      }
    }
    goto LAB_00476dc0;
  }
  goto LAB_004771f2;
LAB_0047719e:
  iVar9 = *(int *)(iVar4 + 300);
  iVar1 = *(int *)(iVar4 + 0x130);
  if (iVar1 < iVar9) {
    iVar9 = iVar9 - iVar1;
  }
  else if (iVar9 < iVar1) {
    iVar9 = (*(int *)(iVar4 + 0x120) - iVar1) + iVar9;
  }
  else {
    iVar9 = 0;
  }
  if (0 < local_e8) {
    player_update_history_log_printf_filtered
              (1,"[%d]: Caught up on [%d] updates == [%d] ticks.\n",
               *(undefined4 *)(DAT_006f1d6c + 0xc),local_ec - iVar9,local_e8);
  }
LAB_004771f2:
  iVar4 = data_iterator_next();
  goto joined_r0x00476d78;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
