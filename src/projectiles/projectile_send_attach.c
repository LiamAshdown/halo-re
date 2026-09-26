// projectile_send_attach  (Ghidra: FUN_004bf120; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes")
// address 0x4bf120, size 158 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/projectiles_types_notes.md: "message 0x33 with {projectile hash, parent
//   hash, marker}... ecx is the projectile index ..., and edi is collision_result.object_index
//   ..., so the projectile handle is first and the parent handle second"; types/projectiles.h
//   projectile_attach_message (object_hash, parent_hash, parent_marker_index; size 0x0a) and
//   k_message_projectile_attach = 0x33. The hash_table_get / message_delta_encode_message /
//   network_session_broadcast_to_flagged sequence follows the same shape as projectile_send_detonation.c and
//   src/items/weapon_notify_ammo_pickup.c.
// register convention: projectile index in ECX, parent object index in EDI (both confirmed by
//   `cmp ecx,0xffffffff` / `cmp edi,0xffffffff` immediately guarding each hash lookup); the
//   marker index is Ghidra's own recognized stack parameter (`param_1`, used only for its low
//   16 bits).
// blam-cc: ECX -> projectile_index, EDI -> parent_object_index, stack -> marker_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, EAX bits, ECX server

// Broadcasts a projectile-attach network event: the projectile's own hash, the hash of the
// object it just stuck to, and the marker it attached at. Sent by the attach response
// (projectile_response, this batch) when both ends are authoritative.
void projectile_send_attach(datum_index projectile_index, datum_index parent_object_index, int16_t marker_index)
{
    projectile_attach_message message;
    void *items[1];

    message.object_hash = 0;
    if (projectile_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), projectile_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.parent_hash = 0;
    if (parent_object_index != (datum_index)0xffffffff) {
        message.parent_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), parent_object_index);
        if (message.parent_hash == -1) {
            message.parent_hash = 0;
        }
    }
    message.parent_marker_index = marker_index;

    items[0] = &message;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_projectile_attach, 0, items, 0, 1, 0), network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3);
}

#if 0
Original Ghidra decompilation (0x4bf120):

void FUN_004bf120(int *param_1)

{
  int in_ECX;
  int unaff_EDI;
  int local_c;
  int local_8;
  undefined2 local_4;

  local_c = 0;
  if (in_ECX != -1) {
    local_c = hash_table_get();
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_8 = 0;
  if (unaff_EDI != -1) {
    local_8 = hash_table_get();
    if (local_8 == -1) {
      local_8 = 0;
    }
  }
  local_4 = param_1._0_2_;
  param_1 = &local_c;
  message_delta_encode_message(0,0x33,0,&param_1,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  return;
}
#endif
