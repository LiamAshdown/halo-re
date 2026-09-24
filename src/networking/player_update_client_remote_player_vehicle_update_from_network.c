// player_update_client_remote_player_vehicle_update_from_network  (Ghidra:
// player_update_client_remote_player_vehicle_update_from_network, already named)
// address 0x4e6510, size 1074 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; cea-pdb match on "Received vehicle_update update
// [%d], on [%d] (%d). [%d] actions, [%d] vehicle updates" (0x0066dee0) and "[%d]: Remote player
// vehicle_update_queue overflow.\n" (0x0066df38); types/game.h vehicle_update_body /
// vehicle_update_record / circular_queue; types/objects.h object (velocity 0x68, forward 0x74,
// up 0x80, angular_velocity 0x8c, parent_object 0x11c) -- the six field copies at
// 0x4e686f..0x4e68f2 land on exactly those four vectors, which is the independent confirmation
// of vehicle_update_body field naming in types/game.h.
// Written from the disassembly (objdump -d -M intel --start-address=0x4e6510
// --stop-address=0x4e6945 bin/halo.exe). Ghidra models the by-value 0x40-byte record as twelve
// separate in_stack_000000xx pseudo-locals and loses EAX entirely.
// register convention: EAX -> player_index; update_id, control_sequence and the whole 0x40-byte
// vehicle_update_body are pushed on the stack, the body by value (both callers rep-movsd it onto
// the outgoing frame: 0x4e5c08 and 0x4e6c6c).
//   // blam-cc: EAX -> player_index, stack -> update_id, control_sequence, vehicle
// This is the vehicle twin of player_update_client_remote_player_position_update_from_network
// (0x4e6270) and makes the same queue-or-apply decision, with three differences that are real,
// not transcription artifacts:
//   1. the out-of-range tolerance is 1 here, not 2 (`cmp eax,0x1 ; jle` at 0x4e67d5);
//   2. the "***Ignoring" branch counts position_updates (player+0x170) instead of
//      vehicle_updates (player+0x1d0) -- `lea edx,[ebp+0x170]` at 0x4e68fa, where the
//      "***Applying immediately" branch two blocks above uses `lea edx,[ebp+0x1d0]`. This looks
//      like a copy-paste slip in the original and is preserved;
//   3. the same two "Received pos update ..." format strings the position handler uses are
//      reused verbatim for the two out-of-range lines, so the vehicle path logs itself as a
//      position update.
// UNSURE: the remap table at 0x00687130 is a second table with the same +0x28 shape as
// remote_player_index_remap_table (0x00687558); named vehicle_object_remap_table from what it is
// used for (the result is compared against object::parent_object), not from any symbol.
// UNSURE: FUN_00570cb0 (0x570cb0, foreign) takes the position in EAX and an object datum_index
// in ECX and walks the object header table at 0x008603b0; it is the position setter of the pair
// but its exact contract was not chased past its prologue.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480
extern void *vehicle_object_remap_table; // 0x00687130, table pointer at +0x28, UNSURE name
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a,
    const real_vector3d *b); // 0x4052c0; blam-cc: EAX -> out, ECX -> a, stack -> b;
                             // computes out = b x a. Open-coded inline by this function.
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990; blam-cc: ECX -> v
extern uint8_t is_remote_player_update_in_order(player *target_player, uint8_t control_sequence,
    int32_t update_id); // blam-cc: EAX -> target_player, DL -> control_sequence,
                        // ESI -> update_id; this module, 0x4e6a20
extern int32_t FUN_004e6aa0(player *target_player, int32_t update_id);
    // blam-cc: EDI -> target_player, EDX -> update_id; this module, 0x4e6aa0
extern uint8_t circular_queue_push(circular_queue *queue, void *source);
    // blam-cc: EBX -> queue, stack -> source; 0x47a1a0
extern int32_t circular_queue_count(circular_queue *queue); // blam-cc: EDX -> queue; 0x47a230
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void FUN_00570cb0(const real_point3d *position, datum_index object_index);
    // blam-cc: EAX -> position, ECX -> object_index; foreign, 0x570cb0
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...); // this module, 0x4e5f20

// Handles a decoded remote-player vehicle position/orientation update: remaps the vehicle datum,
// orthonormalizes the orientation basis, and then either queues the record for ordered replay or
// -- once the client has been out of range twice running -- writes the position and the four
// vectors straight onto the vehicle object.
void player_update_client_remote_player_vehicle_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, vehicle_update_body vehicle)
    // blam-cc: EAX -> player_index, stack -> update_id, control_sequence, vehicle
{
    int16_t index;
    int16_t salt;
    player *target;
    int32_t remapped_vehicle;
    real_vector3d temp;
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

    remapped_vehicle = -1;
    if (vehicle.parent_or_tag != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)vehicle_object_remap_table + 0x28);
        remapped_vehicle = table_base[vehicle.parent_or_tag];
    }
    vehicle.parent_or_tag = remapped_vehicle;

    // Orthonormalize the basis in place, open-coded here (the two sibling handlers at 0x4e5a30
    // and 0x4e5d60 call vector3d_cross_product / vector3d_normalize_with_length for the same
    // three steps):  temp = forward x up ;  up = temp x forward ;  normalize both.
    temp.i = vehicle.forward.j * vehicle.up.k - vehicle.forward.k * vehicle.up.j;
    temp.j = vehicle.forward.k * vehicle.up.i - vehicle.forward.i * vehicle.up.k;
    temp.k = vehicle.forward.i * vehicle.up.j - vehicle.forward.j * vehicle.up.i;
    vehicle.up.i = temp.j * vehicle.forward.k - temp.k * vehicle.forward.j;
    vehicle.up.j = temp.k * vehicle.forward.i - vehicle.forward.k * temp.i;
    vehicle.up.k = vehicle.forward.j * temp.i - temp.j * vehicle.forward.i;
    vector3d_normalize_with_length(&vehicle.forward);
    vector3d_normalize_with_length(&vehicle.up);

    if (is_remote_player_update_in_order(target, (uint8_t)control_sequence, update_id) != 1) {
        return;
    }

    if (target->unknown_18c != -1) {
        if (target->update_history.queue.read_index == target->update_history.queue.write_index) {
            on_update_id = -1;
        } else {
            player_update_record *oldest = (player_update_record *)
                target->update_history.queue.records[target->update_history.queue.read_index];
            on_update_id = (int32_t)oldest->field0;
        }

        distance = FUN_004e6aa0(target, update_id);
        if (distance >= 0 && distance < 0x20) {
            vehicle_update_record record;
            int32_t vehicle_count;
            int32_t action_count;
            int32_t write_index;
            int32_t read_index;

            record.tick = update_id;
            record.sequence = distance;
            record.body = vehicle;
            if (circular_queue_push(&target->vehicle_updates, &record) == 0) {
                player_update_history_log_printf_filtered(target, 1,
                    "[%d]: Remote player vehicle_update_queue overflow.\n",
                    game_time->game_time);
            }

            // inlined circular_queue_count(&target->vehicle_updates)
            write_index = target->vehicle_updates.write_index;
            read_index = target->vehicle_updates.read_index;
            if (write_index > read_index) {
                vehicle_count = write_index - read_index;
            } else if (write_index < read_index) {
                vehicle_count = (target->vehicle_updates.capacity - read_index) + write_index;
            } else {
                vehicle_count = 0;
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
                "Received vehicle_update update [%d], on [%d] (%d). [%d] actions, "
                "[%d] vehicle updates",
                update_id, on_update_id, distance, action_count, vehicle_count);
            target->unknown_1e8 = 0;
        } else {
            int32_t out_of_range_count;

            out_of_range_count = target->unknown_1e8 + 1;
            target->unknown_1e8 = out_of_range_count;
            if (out_of_range_count <= 1) {
                // note: position_updates, not vehicle_updates -- see the file header
                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Ignoring [%d (%d)] ",
                    update_id, on_update_id, distance, action_count, position_count,
                    out_of_range_count, 1);
                // note: unknown_1e8 is deliberately NOT reset on this branch
            } else {
                int32_t vehicle_count = circular_queue_count(&target->vehicle_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Applying immediately",
                    update_id, on_update_id, distance, action_count, vehicle_count);
                target->unknown_1e8 = 0;

                if (target->unit != -1) {
                    object *unit = object_try_and_get(target->unit, 3);

                    if (unit != 0 && unit->parent_object == vehicle.parent_or_tag) {
                        object *vehicle_object = object_try_and_get(vehicle.parent_or_tag, 3);

                        if (vehicle_object != 0) {
                            FUN_00570cb0(&vehicle.position, vehicle.parent_or_tag);
                            vehicle_object->velocity = vehicle.velocity;
                            vehicle_object->angular_velocity = vehicle.angular_velocity;
                            vehicle_object->forward = vehicle.forward;
                            vehicle_object->up = vehicle.up;
                        }
                    }
                }
            }
        }
        target->unknown_15c = (datum_index)(uint32_t)(uint8_t)control_sequence;
    }
    target->unknown_18c = update_id;
}

#if 0
Original Ghidra decompilation (0x4e6510), from tools/pack.py 0x4e6510:

void player_update_client_remote_player_vehicle_update_from_network
               (undefined4 param_1,byte param_2,int param_3)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  short sVar6;
  int in_EAX;
  int iVar7;
  int iVar8;
  undefined4 uVar9;
  int iVar10;
  short sVar11;
  int iVar12;
  undefined4 *puVar13;
  undefined4 *puVar14;
  undefined4 in_stack_0000001c;
  undefined4 in_stack_00000020;
  undefined4 in_stack_00000024;
  undefined4 in_stack_00000028;
  undefined4 in_stack_0000002c;
  undefined4 in_stack_00000030;
  float in_stack_00000034;
  float in_stack_00000038;
  float in_stack_0000003c;
  float in_stack_00000040;
  float in_stack_00000044;
  float in_stack_00000048;
  undefined4 local_68;
  undefined4 local_48;
  int local_44;
  undefined4 local_40 [16];

  if (((in_EAX != -1) && (sVar6 = (short)in_EAX, -1 < sVar6)) &&
     (sVar6 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar12 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar6;
    sVar6 = *(short *)(iVar12 + *(int *)(DAT_0087a480 + 0x34));
    iVar12 = iVar12 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar6 != 0) && ((sVar11 = (short)((uint)in_EAX >> 0x10), sVar11 == 0 || (sVar6 == sVar11)))
       ) {
      iVar7 = -1;
      if (param_3 != 0) {
        iVar7 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + param_3 * 4);
      }
      fVar2 = in_stack_00000048 * in_stack_00000038 - in_stack_00000044 * in_stack_0000003c;
      fVar3 = in_stack_0000003c * in_stack_00000040 - in_stack_00000048 * in_stack_00000034;
      fVar4 = in_stack_00000044 * in_stack_00000034 - in_stack_00000038 * in_stack_00000040;
      in_stack_00000040 = fVar3 * in_stack_0000003c - fVar4 * in_stack_00000038;
      in_stack_00000044 = fVar4 * in_stack_00000034 - in_stack_0000003c * fVar2;
      in_stack_00000048 = in_stack_00000038 * fVar2 - fVar3 * in_stack_00000034;
      param_3 = iVar7;
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      cVar5 = is_remote_player_update_in_order();
      if (cVar5 == '\x01') {
        if (*(int *)(iVar12 + 0x18c) != -1) {
          if (*(int *)(iVar12 + 0x130) == *(int *)(iVar12 + 300)) {
            local_68 = 0xffffffff;
          }
          else {
            local_68 = **(undefined4 **)(*(int *)(iVar12 + 0x128) + *(int *)(iVar12 + 0x130) * 4);
          }
          iVar7 = FUN_004e6aa0();
          if ((iVar7 < 0) || (0x1f < iVar7)) {
            iVar10 = *(int *)(iVar12 + 0x1e8) + 1;
            *(int *)(iVar12 + 0x1e8) = iVar10;
            if (iVar10 < 2) {
              uVar9 = circular_queue_count(iVar10,1);
              uVar9 = circular_queue_count(uVar9);
              player_update_history_log_printf_filtered
                        (1,
                         "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions ***Ignoring [%d (%d)] "
                         ,param_1,local_68,iVar7,uVar9);
            }
            else {
              uVar9 = circular_queue_count();
              uVar9 = circular_queue_count(uVar9);
              player_update_history_log_printf_filtered
                        (1,
                         "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions ***Applying immediately"
                         ,param_1,local_68,iVar7,uVar9);
              *(undefined4 *)(iVar12 + 0x1e8) = 0;
              if ((((*(int *)(iVar12 + 0x34) != -1) && (iVar7 = object_try_and_get(3), iVar7 != 0))
                  && (*(int *)(iVar7 + 0x11c) == param_3)) &&
                 (iVar7 = object_try_and_get(3), iVar7 != 0)) {
                FUN_00570cb0();
                *(undefined4 *)(iVar7 + 0x68) = in_stack_0000001c;
                *(undefined4 *)(iVar7 + 0x6c) = in_stack_00000020;
                *(undefined4 *)(iVar7 + 0x70) = in_stack_00000024;
                *(undefined4 *)(iVar7 + 0x8c) = in_stack_00000028;
                *(undefined4 *)(iVar7 + 0x90) = in_stack_0000002c;
                *(undefined4 *)(iVar7 + 0x94) = in_stack_00000030;
                *(float *)(iVar7 + 0x74) = in_stack_00000034;
                *(float *)(iVar7 + 0x78) = in_stack_00000038;
                *(float *)(iVar7 + 0x7c) = in_stack_0000003c;
                *(float *)(iVar7 + 0x80) = in_stack_00000040;
                *(float *)(iVar7 + 0x84) = in_stack_00000044;
                *(float *)(iVar7 + 0x88) = in_stack_00000048;
              }
            }
          }
          else {
            local_48 = param_1;
            puVar13 = &param_3;
            puVar14 = local_40;
            for (iVar10 = 0x10; iVar10 != 0; iVar10 = iVar10 + -1) {
              *puVar14 = *puVar13;
              puVar13 = puVar13 + 1;
              puVar14 = puVar14 + 1;
            }
            local_44 = iVar7;
            cVar5 = circular_queue_push(&local_48);
            if (cVar5 == '\0') {
              player_update_history_log_printf_filtered
                        (1,"[%d]: Remote player vehicle_update_queue overflow.\n",
                         *(undefined4 *)(DAT_006f1d6c + 0xc));
            }
            iVar10 = *(int *)(iVar12 + 0x1dc);
            iVar8 = *(int *)(iVar12 + 0x1e0);
            if (iVar8 < iVar10) {
              iVar10 = iVar10 - iVar8;
            }
            else if (iVar10 < iVar8) {
              iVar10 = (*(int *)(iVar12 + 0x1d0) - iVar8) + iVar10;
            }
            else {
              iVar10 = 0;
            }
            iVar8 = *(int *)(iVar12 + 300);
            iVar1 = *(int *)(iVar12 + 0x130);
            if (iVar1 < iVar8) {
              iVar8 = iVar8 - iVar1;
            }
            else if (iVar8 < iVar1) {
              iVar8 = (iVar8 - iVar1) + *(int *)(iVar12 + 0x120);
            }
            else {
              iVar8 = 0;
            }
            player_update_history_log_printf_filtered
                      (1,
                       "Received vehicle_update update [%d], on [%d] (%d). [%d] actions, [%d] vehicle updates"
                       ,param_1,local_68,iVar7,iVar8,iVar10);
            *(undefined4 *)(iVar12 + 0x1e8) = 0;
          }
          *(uint *)(iVar12 + 0x15c) = (uint)param_2;
        }
        *(undefined4 *)(iVar12 + 0x18c) = param_1;
      }
    }
  }
  return;
}
#endif
