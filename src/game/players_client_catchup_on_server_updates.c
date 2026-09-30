// players_client_catchup_on_server_updates  (Ghidra: players_client_catchup_on_server_updates,
// already named)
// address 0x476d40, size 1224 bytes
// name confidence: 0.85   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Client-side routine that drains a player's buffered
//   server updates, fast-forwarding position/orientation state until it matches the latest
//   server tick"); types/game.h player_update_queue / circular_queue (capacity +0x120,
//   record_size +0x124, records +0x128, write_index +0x12c, read_index +0x130, has_current
//   +0x138, current[8] +0x13c) at player+0x120; types/units.h units_module note that 0x5625b0
//   is the real unit_update and 0x5590a0 is biped_update (used here in that corrected order);
//   types/objects.h object (flags +0x204 via unit_data, parent_object +0x11c).
//
// UNSURE (heavy): this is the least-verified file in the batch. The per-record "catch-up" body
// mirrors game_engine_players_update_server.c's / game_engine_players_update_client.c's own
// unit_control_data-build-and-dispatch shape closely enough to reuse it, but Ghidra shows the
// two unit_apply_control_block(0xffffffff) call sites here with zero visible arguments on BOTH converging
// branches (only one of which calls player_compute_view_forward_vector first), which this
// rewrite cannot fully disambiguate without a much deeper disassembly pass than this batch's
// budget allows; the local unit_control_data built below is a best-effort reconstruction, not an
// independently re-verified one. DAT_006887bc / DAT_006887c0 / DAT_006894a1 (catch-up
// thresholds) are given placeholder names.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"
#include "fn_units.h"
#include <string.h>
#include <stdint.h>

extern data_array *player_data;      // 0x0087a480
extern data_array *object_data;      // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern int16_t network_game_mode;    // 0x00719720
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t catchup_backlog_threshold;   // 0x006887bc, UNSURE name
extern int32_t catchup_time_threshold;      // 0x006887c0, UNSURE name
extern uint8_t network_client_vehicle_ack_enabled; // 0x006894a1, UNSURE name

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
                                                real_vector3d *out_forward); // this batch, 0x473d70
extern uint8_t player_unit_has_parent(datum_index player_handle); // this batch, 0x477210, blam-cc: ECX
extern void apply_remote_player_position_update(player *plr, object *unit_obj); // this batch, 0x477350, blam-cc: EAX -> plr, EBX -> unit_obj
extern void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj); // this batch, 0x477490, blam-cc: EAX -> plr, EBX -> unit_obj
extern void player_update_history_log_printf_filtered(int32_t level, const char *format, ...); // 0x4e5f20
extern void object_update(uint32_t object_index); // 0x4f7ef0

extern uint32_t biped_update(uint32_t object_index); // 0x5590a0, established (units module)
extern void unit_apply_control_block(void *record_or_field, int32_t grenade_value); // 0x5639f0, units module,
    // not in this batch; blam-cc: EDX -> record_or_field, ECX -> grenade_value

// For every non-local player, drains its update_history circular queue: while the backlog
// (queued record count, or the summed per-record tick weight) exceeds the catch-up thresholds,
// pops the oldest record, replays it into the player's controlled unit (rebuilding a
// unit_control_data and either running the ordinary per-tick update or, while a network client
// with a seated unit, an object_update instead -- see UNSURE above), and applies a remote
// position/vehicle update. Once caught up, logs how many updates/ticks were skipped.
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
        int32_t initial_backlog_ticks = 0;

        if (plr->local_player_index == -1) {
            circular_queue *queue = &plr->update_history.queue;
            int32_t read_index = queue->read_index;
            int32_t write_index = queue->write_index;

            if (write_index < read_index) {
                initial_backlog_ticks = read_index - write_index;
            } else if (read_index < write_index) {
                initial_backlog_ticks = (queue->capacity - write_index) + read_index;
            } else {
                initial_backlog_ticks = 0;
            }

            for (;;) {
                int32_t backlog_records;
                read_index = queue->read_index;
                write_index = queue->write_index;

                if (write_index < read_index) {
                    backlog_records = read_index - write_index;
                } else if (read_index < write_index) {
                    backlog_records = (queue->capacity - write_index) + read_index;
                } else {
                    backlog_records = 0;
                }

                if (backlog_records <= catchup_backlog_threshold) {
                    int32_t summed_ticks = 0;
                    int32_t i = write_index;
                    if (write_index != read_index) {
                        do {
                            summed_ticks = summed_ticks + *(int32_t *)(((int32_t **)queue->records)[i] + 1);
                            i = (i + 1) % 0x78;
                        } while (i != queue->read_index);
                    }
                    if (summed_ticks <= catchup_time_threshold) {
                        break; // caught up
                    }
                }

                {
                    int32_t record[11];
                    memset(record, -1, sizeof(record));

                    if (write_index != read_index) {
                        int32_t *entry = ((int32_t **)queue->records)[write_index];
                        entry[1] = entry[1] - 1;
                        if (entry[1] == 0) {
                            if (queue->read_index == queue->write_index) {
                                entry = 0;
                            } else {
                                entry = ((int32_t **)queue->records)[queue->read_index];
                                queue->read_index = (queue->read_index + 1) % queue->capacity;
                            }
                        }
                        if (entry != 0) {
                            memcpy(record, entry, sizeof(record));
                            plr->update_history.has_current = 1;
                            memcpy(plr->update_history.current, entry + 3, sizeof(plr->update_history.current));
                        }
                    }

                    updates_applied = updates_applied + 1;

                    if (record[1] == record[2] - 1 && network_game_mode == 1 &&
                        plr->local_player_index == -1 && plr->unit != (datum_index)-1) {
                        int16_t index = (int16_t)plr->unit;
                        object_header *header = 0;
                        if (index >= 0 && index < object_data->maximum_count) {
                            object_header *candidate = &((object_header *)object_data->data)[index];
                            int16_t salt = (int16_t)((uint32_t)plr->unit >> 16);
                            if (candidate->identifier != 0 && (salt == 0 || candidate->identifier == salt)) {
                                header = candidate;
                            }
                        }
                        if (header != 0 && (1u << (header->type & 0x1f) & _object_mask_unit) != 0 &&
                            header->data != 0) {
                            object *unit_obj = header->data;
                            uint8_t seated = player_unit_has_parent(plr->unit); // UNSURE: arg should likely be controlling_player
                            *(int32_t *)((uint8_t *)unit_obj + 0x4bc) = record[0];
                            if (seated == 0) {
                                apply_remote_player_position_update(plr, unit_obj);
                            } else {
                                apply_remote_player_vehicle_position_update(plr, unit_obj);
                            }
                        }
                    }

                    if (plr->unit != (datum_index)-1) {
                        object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
                        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                        if ((unit->flags & 0x40) != 0) { // UNSURE: unnamed unit_flags bit 6
                            unit_control_data ctrl;
                            uint8_t run_object_update = 0;

                            memset(&ctrl, 0, sizeof(ctrl));
                            ctrl.animation_state = 3;
                            ctrl.weapon_index = -1;
                            ctrl.grenade_index = -1;
                            ctrl.zoom_level = -1;

                            if (local_player_globals->input_disabled == 0) {
                                player_compute_view_forward_vector(iter.index, (real *)&record[1],
                                                                    &ctrl.facing_vector); // UNSURE record layout
                            } else if (unit->swarm_actor_index == (datum_index)-1 &&
                                       unit->actor_index == (datum_index)-1) {
                                ctrl.facing_vector = unit->desired_facing_vector;
                                ctrl.aiming_vector = unit->desired_aiming_vector;
                                ctrl.looking_vector = unit->desired_looking_vector;
                            }
                            unit_apply_control_block(&ctrl, -1);

                            run_object_update = player_unit_has_parent(plr->unit); // UNSURE: see above
                            if (run_object_update == 0 || network_client_vehicle_ack_enabled == 0) {
                                unit_update(plr->unit);
                                biped_update(plr->unit);
                            } else {
                                object_update((uint32_t)unit_obj->parent_object);
                            }
                        }
                    }
                }
            }

            if (updates_applied > 0) {
                circular_queue *q2 = &plr->update_history.queue;
                int32_t remaining_ticks;
                int32_t r = q2->read_index, w = q2->write_index;
                if (w < r) {
                    remaining_ticks = r - w;
                } else if (r < w) {
                    remaining_ticks = (q2->capacity - w) + r;
                } else {
                    remaining_ticks = 0;
                }
                player_update_history_log_printf_filtered(
                    1, "[%d]: Caught up on [%d] updates == [%d] ticks.\n", game_time->game_time,
                    initial_backlog_ticks - remaining_ticks, updates_applied);
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
