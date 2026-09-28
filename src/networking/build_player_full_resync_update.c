// build_player_full_resync_update  (Ghidra: build_player_full_resync_update, already named)
// address 0x4e7d90, size 508 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md ("Flushes the current cached state of three
// network message types to a single target machine, likely used to resync a newly joined
// client."); build_remote_player_vehicle_update.c / build_remote_player_vehicle_attachment_update.c
// (this batch, the same per-player broadcast-cache offsets 0x130/0x16c/0x170.../0x188..); the
// established network_message_scratch buffer (0x00871de0, this batch's
// network_game_broadcast_player_set_changed.c) as message_delta_encode_message's shared output.
// register convention: EBX -> player_index (unaff_EBX in the decompile).
//   // blam-cc: EBX -> player_index
// UNSURE: message types 0x27 and 0x28 are not otherwise named in this batch's evidence; their
// staged records reuse the same cache offsets build_player_full_resync_update's siblings do
// (0x170.. for the first, 0x188.. for the second), so they are almost certainly the vehicle
// (0x29-family) and attachment (0x2a-family) "last known" transforms re-sent verbatim under
// different message type numbers for a resync, but that is not otherwise confirmed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480
extern void *machine_table; // 0x00687558
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930

// Re-encodes and sends player_index's three cached broadcast records (message 0x25's action
// staging at cache+0x130, 0x27's staging at cache+0x170, and 0x28's staging at cache+0x188) as
// three fresh baseline (non-delta) messages to machine 1, likely to resync a newly joined client.
void build_player_full_resync_update(uint32_t player_index)
    // blam-cc: EBX -> player_index
{
    uint8_t *cache;
    uint32_t staged12[12];
    uint32_t staged16[16];
    int32_t network_hash;
    struct { uint8_t update_id; uint8_t baseline_id; } header;
    int32_t body[3];
    int32_t encoded_size;
    void *items_ptr;
    void *previous_ptr;
    int32_t i;

    cache = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;

    header.update_id = *(uint8_t *)(cache + 0x128);
    header.baseline_id = *(uint8_t *)(cache + 300);
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    for (i = 0; i < 12; i = i + 1) {
        staged12[i] = *(uint32_t *)(cache + 0x130 + i * 4);
    }
    items_ptr = staged12;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x25, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = *(uint8_t *)(cache + 0x16c);
    header.baseline_id = 0;
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    body[0] = *(int32_t *)(cache + 0x170);
    body[1] = *(int32_t *)(cache + 0x174);
    body[2] = *(int32_t *)(cache + 0x178);
    items_ptr = body;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x27, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = *(uint8_t *)(cache + 0x16c);
    header.baseline_id = 0;
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    for (i = 0; i < 16; i = i + 1) {
        staged16[i] = *(uint32_t *)(cache + 0x188 + i * 4);
    }
    items_ptr = staged16;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x28, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);
}

#if 0
Original Ghidra decompilation (0x4e7d90), from tools/pack.py 0x4e7d90:

void build_player_full_resync_update(void)

{
  int iVar1;
  int iVar2;
  uint unaff_EBX;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int *local_5c;
  int *local_58;
  int local_54;
  undefined1 local_50;
  undefined1 local_4f;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40 [16];

  iVar3 = (unaff_EBX & 0xffff) * 0x200;
  local_50 = *(undefined1 *)(iVar3 + 0x128 + *(int *)(DAT_0087a480 + 0x34));
  iVar3 = iVar3 + *(int *)(DAT_0087a480 + 0x34);
  local_4f = *(undefined1 *)(iVar3 + 300);
  local_54 = 0;
  if (unaff_EBX != 0xffffffff) {
    local_54 = hash_table_get();
    if (local_54 == -1) {
      local_54 = 0;
    }
  }
  local_5c = local_40;
  puVar4 = (undefined4 *)(iVar3 + 0x130);
  puVar5 = local_40;
  for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  local_58 = &local_54;
  iVar1 = message_delta_encode_message(0,0x25,(int)&local_58,&local_5c,0,1,'\0');
  network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,1);
  local_50 = *(undefined1 *)(iVar3 + 0x16c);
  iVar1 = 0;
  local_4f = 0;
  if (unaff_EBX != 0xffffffff) {
    iVar1 = hash_table_get();
    if (iVar1 == -1) {
      iVar1 = 0;
    }
  }
  local_4c = *(int *)(iVar3 + 0x170);
  local_48 = *(undefined4 *)(iVar3 + 0x174);
  local_44 = *(undefined4 *)(iVar3 + 0x178);
  local_58 = &local_4c;
  local_5c = &local_54;
  local_54 = iVar1;
  iVar1 = message_delta_encode_message(0,0x27,(int)&local_5c,&local_58,0,1,'\0');
  network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,1);
  local_50 = *(undefined1 *)(iVar3 + 0x16c);
  iVar1 = 0;
  local_4f = 0;
  if (unaff_EBX != 0xffffffff) {
    iVar1 = hash_table_get();
    if (iVar1 == -1) {
      iVar1 = 0;
    }
  }
  puVar4 = (undefined4 *)(iVar3 + 0x188);
  puVar5 = local_40;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  local_58 = local_40;
  local_5c = &local_54;
  local_54 = iVar1;
  iVar3 = message_delta_encode_message(0,0x28,(int)&local_5c,&local_58,0,1,'\0');
  network_session_send_to_machine(1,&DAT_00871de0,iVar3,1,0,0,1);
  return;
}
#endif
