// player_update_client_remote_player_position_update_from_network  (Ghidra:
// player_update_client_remote_player_position_update_from_network, already named)
// address 0x4e6270, size 657 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; cea-pdb match on the three "Received pos update
// [%d], on [%d] (%d). [%d] actions, [%d] positions..." formats plus "Apply immediately dist:
// [%f] (%f)" and "Apply immediately saved by tolerance [%f] (%f)"; types/game.h player
// (position_updates at +0x170, update_history at +0x120, player_update_record::field0 as the
// "on [%d]" value) and circular_queue.
// Written from the disassembly (objdump -d -M intel --start-address=0x4e6270
// --stop-address=0x4e6510 bin/halo.exe). Ghidra recovers the stack parameters but loses EAX and
// renders every circular_queue_count call with scrambled or missing arguments.
// register convention: EAX -> player_index, everything else on the stack. The two callers
// (0x4e5870 total-biped, 0x4e5c40 stand-alone position) both set EAX to the remapped player
// datum immediately before the call (0x4e5a01 `mov eax,[esi]`, 0x4e5d34 `mov eax,edi`), which is
// what pins this argument -- the rewrite of 0x4e5c40 in an earlier session dropped it entirely.
//   // blam-cc: EAX -> player_index, stack -> update_id, control_sequence, x, y, z
// The decision this function makes: FUN_004e6aa0 returns how far update_id is ahead of the
// oldest queued action update, modulo 0x40. 0..0x1f means the position belongs to an update the
// client has not replayed yet, so it is queued for ordered replay. Anything else means the
// client has fallen behind; after three such updates in a row the position is applied straight
// to the unit instead (unless it is already within 1.0 world unit of it).
// UNSURE: the two in-range queue counts are open-coded here rather than calls to
// circular_queue_count (0x47a230) -- the compiler inlined the identical body. They are written
// as the inline computation, matching the binary, with the equivalence noted at each site.
// UNSURE: the "(%f)" argument of both "Apply immediately" lines is the literal double 1.0 loaded
// from 0x00672af8, i.e. the tolerance is logged next to the distance; the single-precision 1.0
// the comparison itself uses comes from 0x00672ac4.
// UNSURE: unit_snap_position_if_far is declared with three arguments here (EAX position, ECX
// object, stack unit datum). src/units declares a two-argument form; the third argument is a
// genuine `push ecx` at 0x4e649b and is not optional.
// reconciled: R35 player.unknown_15c (datum_index) -> int32 last_remote_update_id (stores the byte sequence, zero-extended)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern uint8_t is_remote_player_update_in_order(player *target_player, uint8_t control_sequence,
    int32_t update_id); // blam-cc: EAX -> target_player, DL -> control_sequence,
                        // ESI -> update_id; this module, 0x4e6a20
extern int32_t player_update_queue_offset_from_head(player *target_player, int32_t update_id);
    // blam-cc: EDI -> target_player, EDX -> update_id; this module, 0x4e6aa0.
    // Returns -1 when the action queue is empty, else (update_id - oldest_queued_id) mod 0x40.
extern uint8_t position_update_queue_push(circular_queue *queue, real x, real y, real z,
    int32_t tick, int32_t sequence); // blam-cc: EBX -> queue, stack -> the rest; 0x47a0c0
extern int32_t circular_queue_count(circular_queue *queue); // blam-cc: EDX -> queue; 0x47a230
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern real vector3d_distance(const real_point3d *a, const real_point3d *b);
    // blam-cc: EAX -> a, ECX -> b; 0x4088b0
extern void unit_snap_position_if_far(real_point3d *new_position, object *obj,
    datum_index unit_index); // blam-cc: EAX -> new_position, ECX -> obj, stack -> unit_index;
                             // 0x4772e0
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...); // this module, 0x4e5f20

// Handles a decoded remote-player position update: either queues it for ordered replay against
// the pending action updates, or -- once the client has been out of range three times running --
// snaps the player unit straight to it.
void player_update_client_remote_player_position_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, real x, real y, real z)
    // blam-cc: EAX -> player_index, stack -> update_id, control_sequence, x, y, z
{
    int16_t index;
    int16_t salt;
    player *target;
    int32_t on_update_id;
    int32_t distance;

    if (player_index == -1) {
        return;
    }
    index = (int16_t)player_index;
    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    target = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
    salt = (int16_t)((uint32_t)player_index >> 16);
    if (target->identifier == 0 || (salt != 0 && target->identifier != salt)) {
        return;
    }

    if (is_remote_player_update_in_order(target, (uint8_t)control_sequence, update_id) != 1) {
        return;
    }

    if (target->last_position_update_id != -1) {
        if (target->update_history.queue.read_index == target->update_history.queue.write_index) {
            on_update_id = -1;
        } else {
            player_update_record *oldest = (player_update_record *)
                target->update_history.queue.records[target->update_history.queue.read_index];
            on_update_id = (int32_t)oldest->field0;
        }

        distance = player_update_queue_offset_from_head(target, update_id);
        if (distance >= 0 && distance < 0x20) {
            int32_t position_count;
            int32_t action_count;
            int32_t write_index;
            int32_t read_index;

            if (position_update_queue_push(&target->position_updates, x, y, z, update_id,
                    distance) == 0) {
                player_update_history_log_printf_filtered(target, 1,
                    "[%d]: Remote player position_queue overflow.\n",
                    game_time->game_time);
            }

            // inlined circular_queue_count(&target->position_updates)
            write_index = target->position_updates.write_index;
            read_index = target->position_updates.read_index;
            if (write_index > read_index) {
                position_count = write_index - read_index;
            } else if (write_index < read_index) {
                position_count = (write_index - read_index) + target->position_updates.capacity;
            } else {
                position_count = 0;
            }

            // inlined circular_queue_count(&target->update_history.queue)
            write_index = target->update_history.queue.write_index;
            read_index = target->update_history.queue.read_index;
            if (write_index > read_index) {
                action_count = write_index - read_index;
            } else if (write_index < read_index) {
                action_count = (write_index - read_index) + target->update_history.queue.capacity;
            } else {
                action_count = 0;
            }

            player_update_history_log_printf_filtered(target, 1,
                "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions",
                update_id, on_update_id, distance, action_count, position_count);
            target->position_update_ignored_count = 0;
        } else {
            int32_t out_of_range_count;

            out_of_range_count = target->position_update_ignored_count + 1;
            target->position_update_ignored_count = out_of_range_count;
            if (out_of_range_count <= 2) {
                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Ignoring [%d (%d)] ",
                    update_id, on_update_id, distance, action_count, position_count,
                    out_of_range_count, 2);
                // note: unknown_188 is deliberately NOT reset on this branch
            } else {
                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Applying immediately",
                    update_id, on_update_id, distance, action_count, position_count);
                target->position_update_ignored_count = 0;

                if (target->unit != -1) {
                    object *unit = object_try_and_get(target->unit, 3);

                    if (unit != 0 && unit->parent_object == -1 && unit->network_role == 1) {
                        real_point3d new_position;
                        real snap_distance;

                        new_position.x = x;
                        new_position.y = y;
                        new_position.z = z;
                        snap_distance = vector3d_distance(&new_position, &unit->position);
                        if (snap_distance <= 1.0f) {
                            player_update_history_log_printf_filtered(target, 1,
                                "Apply immediately saved by tolerance [%f] (%f)",
                                (double)snap_distance, 1.0);
                        } else {
                            player_update_history_log_printf_filtered(target, 1,
                                "Apply immediately dist: [%f] (%f)",
                                (double)snap_distance, 1.0);
                            unit_snap_position_if_far(&new_position, unit, target->unit);
                        }
                    }
                }
            }
        }
        target->last_remote_update_id = (int32_t)(uint8_t)control_sequence;
    }
    target->last_position_update_id = update_id;
}

#if 0
Original Ghidra decompilation (0x4e6270), from tools/pack.py 0x4e6270:

void player_update_client_remote_player_position_update_from_network
               (undefined4 param_1,byte param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5)

{
  int iVar1;
  char cVar2;
  short sVar3;
  int in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 uVar7;
  short sVar8;
  int iVar9;
  float10 fVar10;
  undefined4 local_4;

  if (((in_EAX != -1) && (sVar3 = (short)in_EAX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar9 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34));
    iVar9 = iVar9 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar3 != 0) && ((sVar8 = (short)((uint)in_EAX >> 0x10), sVar8 == 0 || (sVar3 == sVar8)))) {
      cVar2 = is_remote_player_update_in_order();
      if (cVar2 == '\x01') {
        if (*(int *)(iVar9 + 0x160) != -1) {
          if (*(int *)(iVar9 + 0x130) == *(int *)(iVar9 + 300)) {
            local_4 = 0xffffffff;
          }
          else {
            local_4 = **(undefined4 **)(*(int *)(iVar9 + 0x128) + *(int *)(iVar9 + 0x130) * 4);
          }
          iVar4 = FUN_004e6aa0();
          if ((iVar4 < 0) || (0x1f < iVar4)) {
            iVar5 = *(int *)(iVar9 + 0x188) + 1;
            *(int *)(iVar9 + 0x188) = iVar5;
            if (iVar5 < 3) {
              circular_queue_count(iVar5,2);
              uVar7 = circular_queue_count();
              player_update_history_log_printf_filtered
                        (1,
                         "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions ***Ignoring [%d (%d)] "
                         ,param_1,local_4,iVar4,uVar7);
            }
            else {
              circular_queue_count();
              uVar7 = circular_queue_count();
              player_update_history_log_printf_filtered
                        (1,
                         "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions ***Applying immediately"
                         ,param_1,local_4,iVar4,uVar7);
              *(undefined4 *)(iVar9 + 0x188) = 0;
              if (*(int *)(iVar9 + 0x34) != -1) {
                iVar4 = object_try_and_get();
                if (((iVar4 != 0) && (*(int *)(iVar4 + 0x11c) == -1)) && (*(int *)(iVar4 + 4) == 1))
                {
                  fVar10 = (float10)vector3d_distance();
                  if (fVar10 <= (float10)1.0) {
                    player_update_history_log_printf_filtered
                              (1,"Apply immediately saved by tolerance [%f] (%f)",(double)fVar10,
                               0x3ff0000000000000);
                  }
                  else {
                    player_update_history_log_printf_filtered
                              (1,"Apply immediately dist: [%f] (%f)",(double)fVar10,
                               0x3ff0000000000000);
                    unit_snap_position_if_far(*(undefined4 *)(iVar9 + 0x34));
                  }
                }
              }
            }
          }
          else {
            cVar2 = FUN_0047a0c0(param_3,param_4,param_5,param_1,iVar4);
            if (cVar2 == '\0') {
              player_update_history_log_printf_filtered();
            }
            iVar5 = *(int *)(iVar9 + 0x17c);
            iVar6 = *(int *)(iVar9 + 0x180);
            if (iVar6 < iVar5) {
              iVar5 = iVar5 - iVar6;
            }
            else if (iVar5 < iVar6) {
              iVar5 = (iVar5 - iVar6) + *(int *)(iVar9 + 0x170);
            }
            else {
              iVar5 = 0;
            }
            iVar6 = *(int *)(iVar9 + 300);
            iVar1 = *(int *)(iVar9 + 0x130);
            if (iVar1 < iVar6) {
              iVar6 = iVar6 - iVar1;
            }
            else if (iVar6 < iVar1) {
              iVar6 = (iVar6 - iVar1) + *(int *)(iVar9 + 0x120);
            }
            else {
              iVar6 = 0;
            }
            player_update_history_log_printf_filtered
                      (1,"Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions",
                       param_1,local_4,iVar4,iVar6,iVar5);
            *(undefined4 *)(iVar9 + 0x188) = 0;
          }
          *(uint *)(iVar9 + 0x15c) = (uint)param_2;
        }
        *(undefined4 *)(iVar9 + 0x160) = param_1;
      }
    }
  }
  return;
}
#endif
