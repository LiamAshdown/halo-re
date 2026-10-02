// build_remote_player_action_update  (Ghidra: FUN_004e7890; named per this rewrite)
// address 0x4e7890, size 693 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Builds and rate-limit-caches a message-0x25
// update (an orientation/action payload) for a player slot and broadcasts it to eligible
// connected peers."); types/game.h player_action (the stack layout of param_4..param_7 plus four
// more unnamed trailing dwords matches player_action's 8-dword shape exactly, and param_5/param_6
// individually alias its desired_yaw/desired_pitch fields, exactly as build_remote_player_vehicle_update.c's
// EBX-passed control record does); types/networking.h machine_table (0x00687558).
// register convention: none recognized beyond the stack; Ghidra resolved this call's own
// convention, but several of ITS parameters are themselves heavily reused for unrelated purposes
// partway through the function body (a compiler/decompiler register-reuse artifact), which this
// rewrite untangles into separate, clearly named locals.
// UNSURE (extensive, rewrite confidence lowered accordingly): several callees in this function are
// invoked with zero visible arguments in the batch decompile (FUN_004e0810/network_machine_find_by_id,
// hash_table_get), and the parameters this rewrite supplies for them are the best inference
// available, not a disassembly-confirmed value. The player-slot cache record's type (iVar5, at
// player_data->data + player_index*0x200) is left as a raw byte pointer with every offset
// preserved exactly as Ghidra shows.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sin(double x); // FSIN
extern double cos(double x); // FCOS

extern data_array *player_data; // 0x0087a480
extern uint8_t network_broadcast_event_feed_mode; // 0x006894a0, UNSURE: nonzero routes through
    // network_event_feed_queue_append instead of an immediate message_delta_encode_message/send
extern void *machine_table; // 0x00687558
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick
extern int32_t network_action_resend_interval_ms; // 0x0068948c
extern int32_t network_action_resend_interval_ms_alt; // 0x0071031c, UNSURE: second, separate
    // resend-interval constant this function also gates on

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern network_machine *network_machine_find_by_id(network_server_globals *server,
    int32_t machine_id); // this module, 0x4e0810; UNSURE: arguments not visible at this call site
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void network_event_feed_queue_append(uint8_t *queue, uint32_t *key,
    uint32_t *payload); // this module, 0x4e7ff0
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930

// Builds a message-0x25 (remote-player orientation/action) update for player_index's broadcast
// cache from control, rate-limiting or staging it exactly as the vehicle-update siblings do, then
// (depending on network_broadcast_event_feed_mode) either queues it into the event feed or encodes
// and broadcasts it immediately to every other connected, established machine.
void build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control)
{
    uint8_t *cache;
    uint32_t network_hash;
    uint8_t staged_update_id;
    uint32_t staged[12]; // control[8] + uninitialized[1] + direction[3]
    real direction_x, direction_y, direction_z;
    uint32_t now;
    uint8_t is_full;
    uint8_t skip_delta;
    int32_t encoded_size;
    void *items_ptr;
    void *previous_ptr;
    int32_t previous_offset;
    uint32_t next_id;
    int32_t i;
    data_iterator iter;
    player *candidate;
    network_machine *machine;

    cache = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    staged_update_id = update_id_byte;

    if (network_broadcast_event_feed_mode == 0) {
        network_hash = 0;
        if (player_index != 0xffffffff) {
            network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), network_key);
            if (network_hash == 0xffffffff) {
                network_hash = 0;
            }
        }
    } else {
        network_hash = player_index;
    }

    ((uint8_t *)staged)[0] = (uint8_t)update_id_byte; // dead store: overwritten by the copy below
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)&control)[i];
    }
    // staged[8] is never written (matches the original's undersized 9-dword local_30).
    direction_x = (real)(cos((double)control.desired_pitch) * cos((double)control.desired_yaw));
    direction_y = (real)(cos((double)control.desired_pitch) * sin((double)control.desired_yaw));
    direction_z = (real)sin((double)control.desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;

    now = (uint32_t)game_time->game_time;
    if (now < (uint32_t)(network_action_resend_interval_ms + *(int32_t *)(cache + 0x124)) &&
        *(int32_t *)(cache + 0x124) != -1) {
        if (now < (uint32_t)(*(int32_t *)(cache + 0x120) + network_action_resend_interval_ms_alt)) {
            is_full = 0;
            goto encode;
        }
        skip_delta = 1;
        *(uint32_t *)(cache + 0x120) = now;
    } else {
        for (i = 0; i < 12; i = i + 1) {
            *(uint32_t *)(cache + 0x130 + i * 4) = staged[i];
        }
        *(int32_t *)(cache + 0x124) = game_time->game_time;
        *(int32_t *)(cache + 0x120) = game_time->game_time;
        next_id = (*(uint8_t *)(cache + 300) + 1) & 0x80000001;
        skip_delta = 1;
        *(uint32_t *)(cache + 0x128) = staged_update_id;
        if ((int32_t)next_id < 0) {
            next_id = (next_id - 1 | 0xfffffffe) + 1;
        }
        staged_update_id = (uint8_t)next_id;
        *(uint8_t *)(cache + 300) = (uint8_t)next_id;
    }
    is_full = 1;

encode:
    if (network_broadcast_event_feed_mode == 0) {
        if (is_full) {
            if (skip_delta != 1) {
                items_ptr = (void *)(cache + 0x130);
                previous_ptr = &network_hash;
                previous_offset = 0;
                encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, skip_delta, 0x25,
                    (int32_t)&previous_ptr, &items_ptr, previous_offset, 1, skip_delta);
            } else {
                items_ptr = staged;
                previous_ptr = &network_hash;
                encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, skip_delta, 0x25,
                    (int32_t)&items_ptr, &previous_ptr, 0, 1, skip_delta);
            }
            if (0 < encoded_size) {
                iter.data = player_data;
                iter.next_index = 0;
                iter.index = k_datum_index_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                candidate = (player *)data_iterator_next(&iter);
                while (candidate != 0) {
                    // UNSURE: the original also tests a reused float sentinel against
                    // (float)player_index here (`param_6 != (float)param_1`), an artifact of the
                    // register reuse this file's header describes; only the local_player_index
                    // gate (excluding locally-driven players) is reproduced.
                    if (candidate->local_player_index == -1) {
                        machine = network_machine_find_by_id(0, 0); // UNSURE: arguments not resolved
                        if (machine != 0 &&
                            ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 1 & 1) != 0) &&
                            ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 2 & 1) != 0)) {
                            network_session_send_to_machine(1, 0, encoded_size, is_full, 0, 0, 1);
                        }
                    }
                    candidate = (player *)data_iterator_next(&iter);
                }
            }
        }
    } else if (is_full) {
        network_event_feed_queue_append((uint8_t *)staged, 0, 0); // UNSURE: real key/payload arguments
    }
}

#if 0
Original Ghidra decompilation (0x4e7890), from tools/pack.py 0x4e7890:

void FUN_004e7890(uint param_1,uint *param_2,int param_3,uint *param_4,float param_5,float param_6,
                 uint param_7)

{
  int iVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint *puVar6;
  uint *puVar7;
  bool bVar8;
  float10 fVar9;
  float10 fVar10;
  uint **changed_offset;
  void **items;
  undefined4 *type_offset;
  uint *local_3c;
  uint local_38;
  byte local_34;
  undefined1 local_33;
  uint local_30 [9];
  float local_c;
  float local_8;
  float local_4;

  iVar5 = (param_1 & 0xffff) * 0x200 + DAT_0087a480[0xd];
  local_34 = (byte)param_2;
  if (DAT_006894a0 == '\0') {
    local_38 = 0;
    if ((param_1 != 0xffffffff) && (local_38 = hash_table_get(), local_38 == 0xffffffff)) {
      local_38 = 0;
    }
  }
  else {
    local_38 = param_1;
  }
  iVar1 = DAT_006f1d6c;
  local_33 = *(undefined1 *)(iVar5 + 300);
  fVar9 = (float10)fcos((float10)param_6);
  local_30[0]._0_1_ = (char)param_3;
  puVar7 = local_30;
  puVar6 = (uint *)&param_4;
  for (iVar4 = 8; puVar7 = puVar7 + 1, iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
  }
  param_2 = (uint *)CONCAT31(param_2._1_3_,1);
  fVar10 = (float10)fcos((float10)param_5);
  local_c = (float)(fVar10 * fVar9);
  fVar10 = (float10)fsin((float10)param_5);
  local_8 = (float)(fVar10 * fVar9);
  fVar9 = (float10)fsin((float10)param_6);
  local_4 = (float)fVar9;
  uVar3 = *(uint *)(DAT_006f1d6c + 0xc);
  if ((uVar3 < (uint)(DAT_0068948c + *(int *)(iVar5 + 0x124))) && (*(int *)(iVar5 + 0x124) != -1)) {
    if (uVar3 < (uint)(*(int *)(iVar5 + 0x120) + _DAT_0071031c)) {
      bVar8 = false;
      goto LAB_004e79cd;
    }
    param_3 = (uint)param_3._1_3_ << 8;
    *(uint *)(iVar5 + 0x120) = uVar3;
  }
  else {
    puVar7 = local_30;
    puVar6 = (uint *)(iVar5 + 0x130);
    for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar6 = puVar6 + 1;
    }
    *(undefined4 *)(iVar5 + 0x124) = *(undefined4 *)(iVar1 + 0xc);
    *(undefined4 *)(iVar5 + 0x120) = *(undefined4 *)(iVar1 + 0xc);
    uVar3 = *(int *)(iVar5 + 300) + 1U & 0x80000001;
    param_3 = CONCAT31(param_3._1_3_,1);
    *(uint *)(iVar5 + 0x128) = (uint)local_34;
    if ((int)uVar3 < 0) {
      uVar3 = (uVar3 - 1 | 0xfffffffe) + 1;
    }
    local_33 = (undefined1)uVar3;
    *(uint *)(iVar5 + 300) = uVar3;
  }
  bVar8 = true;
LAB_004e79cd:
  if (DAT_006894a0 == '\0') {
    if (bVar8) {
      bVar8 = (char)param_3 != '\x01';
      if (bVar8) {
        param_4 = local_30;
        local_3c = &local_38;
        type_offset = &param_2;
        items = &param_4;
        changed_offset = &local_3c;
        param_2 = (uint *)(iVar5 + 0x130);
      }
      else {
        type_offset = (undefined4 *)0x0;
        param_2 = local_30;
        items = &param_2;
        changed_offset = &param_4;
        param_4 = &local_38;
      }
      iVar5 = message_delta_encode_message
                        ((uint)bVar8,0x25,(int)changed_offset,items,(int)type_offset,1,bVar8);
      if (0 < iVar5) {
        param_4 = DAT_0087a480;
        param_7 = (uint)DAT_0087a480 ^ 0x69746572;
        param_5 = (float)((uint)param_5 & 0xffff0000);
        param_6 = -NAN;
        iVar4 = data_iterator_next();
        iVar1 = param_3;
        if (iVar4 != 0) {
          do {
            if ((((param_6 != (float)param_1) && (*(short *)(iVar4 + 2) == -1)) &&
                (iVar4 = FUN_004e0810(), iVar4 != 0)) &&
               ((bVar2 = (byte)*(undefined2 *)(iVar4 + 0xe), (bVar2 >> 1 & 1) != 0 &&
                ((bVar2 >> 2 & 1) != 0)))) {
              network_session_send_to_machine(1,&DAT_00871de0,iVar5,iVar1,0,0,1);
            }
            iVar4 = data_iterator_next();
          } while (iVar4 != 0);
          return;
        }
      }
    }
  }
  else if (bVar8) {
    if ((char)param_3 == '\x01') {
      network_event_feed_queue_append(local_30);
      return;
    }
    network_event_feed_queue_append(local_30);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
