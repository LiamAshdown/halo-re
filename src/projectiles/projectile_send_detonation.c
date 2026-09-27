// projectile_send_detonation  (Ghidra: FUN_004bda60; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes")
// address 0x4bda60, size 214 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: out/phase4/projectiles_types_notes.md: "builds {hash, object.position} and encodes
//   message 0x30, then forces object.network_role = 3"; types/projectiles.h
//   projectile_detonation_message (size 0x10, object_hash + position) and
//   k_message_projectile_detonation = 0x30. The hash_table_get / message_delta_encode_message /
//   network_session_broadcast_to_flagged sequence and the object_pooled_node_globals name follow the precedent in
//   src/items/weapon_notify_ammo_pickup.c; the network_index_cache_remove(globals, object_index) signature
//   follows src/objects/object_delete_by_pooled_node_id.c, and 0x006870d8 (this function's own
//   "globals referenced" list) matches that call's globals argument exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *object_pooled_node_globals; // 0x00687130
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0
extern void *object_pooled_node_globals_006870d8; // 0x006870d8, see
    // src/objects/object_delete_by_pooled_node_id.c

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, EAX bits, ECX server
extern void network_index_cache_remove(void *globals, uint32_t object_index); // 0x4e9d40, opaque, out of range

// Broadcasts a projectile-detonation network event (thrown-grenade projectiles only; see
// projectile_update's thrown_grenade check) with the projectile's own hash and its current
// position, forces the object into network_role 3 (the "waiting to be deleted by the network"
// role the receiver 0x4bdb40 also uses), and, unless the object is already pending delete,
// notifies the pooled-node globals of the role change.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> projectile_index
void projectile_send_detonation(datum_index projectile_index)
{
    object *obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    projectile_detonation_message message;
    void *items[1];

    message.object_hash = 0;
    if (projectile_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), projectile_index);
    }
    message.position = obj->position;

    items[0] = &message;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_projectile_detonation, 0, items, 0, 1, 0), network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3);

    obj->network_role = 3;
    if ((((object_header *)object_data->data)[projectile_index & 0xffff].flags & _object_header_delete_pending_bit) == 0) {
        network_index_cache_remove(&object_pooled_node_globals_006870d8, projectile_index); // FIXED: EAX is the container ADDRESS 0x6870d8 (mov eax,imm); its dword is 0xd, not a pointer
    }
}

#if 0
Original Ghidra decompilation (0x4bda60):

void FUN_004bda60(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar2 = ((uint)param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2);
  local_10 = 0;
  if (param_1 != (void *)0xffffffff) {
    local_10 = hash_table_get();
  }
  local_c = *(undefined4 *)(iVar1 + 0x5c);
  local_8 = *(undefined4 *)(iVar1 + 0x60);
  local_4 = *(undefined4 *)(iVar1 + 100);
  param_1 = &local_10;
  message_delta_encode_message(0,0x30,0,&param_1,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  iVar1 = DAT_008603b0;
  *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) + 4) = 3;
  if ((*(byte *)(*(int *)(iVar1 + 0x34) + 2 + iVar2) & 8) == 0) {
    FUN_004e9d40();
  }
  return;
}
#endif
