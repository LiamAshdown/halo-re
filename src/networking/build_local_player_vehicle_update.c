// build_local_player_vehicle_update  (Ghidra: build_local_player_vehicle_update, already named)
// address 0x4e82f0, size 467 bytes
// name confidence: 0.9   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md; build_local_player_position_update.c (this batch's
// sibling, same 0xe8/0xec/0xf0/0xf4 sequence-tracking fields and resend-interval gate);
// types/objects.h object_header/object; types/projectiles.h network-records section ("Every
// object handle is run through hash_table_get against the object network-id table at
// PTR_DAT_00687130 + 0x0c first, and a lookup that returns -1 is written as 0" -- object_pooled_node_globals
// is this batch's own established name for PTR_DAT_00687130, from src/items/equipment_build_creation_message.c);
// types/networking.h local_player_vehicle_update_ack, types/game.h vehicle_update_body.
// register convention: EDI -> plr; the caller-owned out_changed byte is the one stack parameter
// Ghidra itself recognized.
//   // blam-cc: EDI -> plr, stack -> out_changed
// UNSURE (load-bearing): the stack layout proves that the hash-table lookup result (local_40)
// lands at exactly the byte offset of vehicle_update_body::parent_or_tag inside the staged ack
// (offset 4 of the 0x44-byte scratch block, which is offset 0 of the embedded vehicle_update_body)
// and is never overwritten afterward -- i.e. this call site actually populates
// vehicle_update_body's "UNSURE" parent_or_tag field with the vehicle's pooled-node network hash,
// not a tag reference. types/game.h's comment on that field is left alone since it is shared by
// other call sites this batch does not re-derive.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *object_pooled_node_globals; // 0x00687130
extern int32_t network_vehicle_ack_resend_interval_ms; // 0x00689488
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern uint32_t GetTickCount(void);
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void network_player_update_history_log_write(const char *format, ...); // this module, 0x4e7f90

// If plr's queued vehicle-ack sequence number is valid (0..63), stages a local_player_vehicle_update_ack
// from plr's cached fields and plr's vehicle object's transform, and, unless the previous ack is
// still within the resend interval, encodes and logs a rate-limited message-0x24 acknowledgement.
// Returns the encoded message size, or 0 if nothing was sent.
int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr)
    // blam-cc: EDI -> plr, stack -> out_changed
{
    uint8_t *plr_bytes;
    local_player_vehicle_update_ack ack;
    object *unit_obj;
    object *vehicle_obj;
    datum_index parent_object;
    int32_t network_hash;
    int32_t encoded_size;
    uint32_t next_id;
    uint32_t logged_id;
    void *ack_ptr;

    *out_changed = 0;
    plr_bytes = (uint8_t *)plr;
    if (*(int32_t *)(plr_bytes + 0xf4) < 0 || 0x3f < *(int32_t *)(plr_bytes + 0xf4)) {
        return 0;
    }
    ack.update_id = *(uint8_t *)(plr_bytes + 0xe8);
    ack.baseline_id = *(uint8_t *)(plr_bytes + 0xf4);
    unit_obj = ((object_header *)object_data->data)[*(uint32_t *)(plr_bytes + 0x34) & 0xffff].data;
    parent_object = unit_obj->parent_object;
    vehicle_obj = ((object_header *)object_data->data)[parent_object & 0xffff].data;
    network_hash = 0;
    if (parent_object != (datum_index)-1) {
        network_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c),
            parent_object);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ack.vehicle.parent_or_tag = network_hash;
    *(uint32_t *)&ack.vehicle.position.x = *(uint32_t *)(plr_bytes + 0xf8);
    *(uint32_t *)&ack.vehicle.position.y = *(uint32_t *)(plr_bytes + 0xfc);
    *(uint32_t *)&ack.vehicle.position.z = *(uint32_t *)(plr_bytes + 0x100);
    ack.vehicle.velocity = vehicle_obj->velocity;
    ack.vehicle.angular_velocity = vehicle_obj->angular_velocity;
    ack.vehicle.forward = vehicle_obj->forward;
    ack.vehicle.up = vehicle_obj->up;

    if (*(int32_t *)(plr_bytes + 0xec) != -1 &&
        (uint32_t)game_time->game_time <
            (uint32_t)(*(int32_t *)(plr_bytes + 0xf0) + network_vehicle_ack_resend_interval_ms)) {
        return 0;
    }
    ack_ptr = &ack;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x24, 0, &ack_ptr, 0, 1, '\0');
    *out_changed = 0;
    next_id = (*(uint32_t *)(plr_bytes + 0xe8) + 1) & 0x8000001f;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xffffffe0) + 1;
    }
    logged_id = ack.baseline_id;
    *(uint32_t *)(plr_bytes + 0xe8) = next_id;
    network_player_update_history_log_write("[%d]: [%d]:\t Acked vehicle [%d]\n", GetTickCount(),
        game_time->game_time, logged_id);
    *(int32_t *)(plr_bytes + 0xec) = *(int32_t *)(plr_bytes + 0xf4);
    *(int32_t *)(plr_bytes + 0xf0) = game_time->game_time;
    return encoded_size;
}

#if 0
Original Ghidra decompilation (0x4e82f0), from tools/pack.py 0x4e82f0:

int build_local_player_vehicle_update(undefined1 *param_1)

{
  int iVar1;
  undefined1 *puVar2;
  int iVar3;
  uint uVar4;
  DWORD DVar5;
  uint uVar6;
  int unaff_EDI;
  undefined4 uVar7;
  undefined1 local_44;
  byte local_43;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
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

  puVar2 = param_1;
  *param_1 = 0;
  if ((*(int *)(unaff_EDI + 0xf4) < 0) || (0x3f < *(int *)(unaff_EDI + 0xf4))) {
    return 0;
  }
  local_44 = *(undefined1 *)(unaff_EDI + 0xe8);
  local_43 = *(byte *)(unaff_EDI + 0xf4);
  uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                            (*(uint *)(unaff_EDI + 0x34) & 0xffff) * 0xc) + 0x11c);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
  local_40 = 0;
  if (uVar4 != 0xffffffff) {
    local_40 = hash_table_get();
    if (local_40 == -1) {
      local_40 = 0;
    }
  }
  local_3c = *(undefined4 *)(unaff_EDI + 0xf8);
  local_38 = *(undefined4 *)(unaff_EDI + 0xfc);
  local_34 = *(undefined4 *)(unaff_EDI + 0x100);
  local_30 = *(undefined4 *)(iVar1 + 0x68);
  local_2c = *(undefined4 *)(iVar1 + 0x6c);
  local_28 = *(undefined4 *)(iVar1 + 0x70);
  local_24 = *(undefined4 *)(iVar1 + 0x8c);
  local_20 = *(undefined4 *)(iVar1 + 0x90);
  local_1c = *(undefined4 *)(iVar1 + 0x94);
  local_18 = *(undefined4 *)(iVar1 + 0x74);
  local_14 = *(undefined4 *)(iVar1 + 0x78);
  local_10 = *(undefined4 *)(iVar1 + 0x7c);
  local_c = *(undefined4 *)(iVar1 + 0x80);
  local_8 = *(undefined4 *)(iVar1 + 0x84);
  local_4 = *(undefined4 *)(iVar1 + 0x88);
  if ((*(int *)(unaff_EDI + 0xec) != -1) &&
     (*(uint *)(DAT_006f1d6c + 0xc) < (uint)(*(int *)(unaff_EDI + 0xf0) + DAT_00689488))) {
    return 0;
  }
  param_1 = &local_44;
  iVar3 = message_delta_encode_message(0,0x24,0,&param_1,0,1,'\0');
  *puVar2 = 0;
  iVar1 = DAT_006f1d6c;
  uVar4 = *(int *)(unaff_EDI + 0xe8) + 1U & 0x8000001f;
  if ((int)uVar4 < 0) {
    uVar4 = (uVar4 - 1 | 0xffffffe0) + 1;
  }
  uVar6 = (uint)local_43;
  *(uint *)(unaff_EDI + 0xe8) = uVar4;
  uVar7 = *(undefined4 *)(iVar1 + 0xc);
  DVar5 = GetTickCount();
  network_player_update_history_log_write("[%d]: [%d]:\t Acked vehicle [%d]\n",DVar5,uVar7,uVar6);
  iVar1 = DAT_006f1d6c;
  *(undefined4 *)(unaff_EDI + 0xec) = *(undefined4 *)(unaff_EDI + 0xf4);
  *(undefined4 *)(unaff_EDI + 0xf0) = *(undefined4 *)(iVar1 + 0xc);
  return iVar3;
}
#endif
