// weapon_notify_ammo_pickup  (Ghidra: FUN_004c2510)
// address 0x4c2510, size 135 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: types/items.h weapon_ammo_pickup_message (network delta message type
//   k_message_weapon_ammo_pickup = 0x2c); src/objects/object_delete_unparented.c and
//   src/objects/object_queue_pickup_denied_event.c establish the hash_table_get(object
//   _pooled_node_globals+0x0c, object_index) idiom and the message_delta_encode_message /
//   network_session_broadcast_to_flagged pair (encode into the message-scratch global, then network_session_broadcast_to_flagged broadcasts it).
// register convention: item index in ECX; magazine index and rounds are Ghidra-recognized stack
//   parameters (only their low 16 bits are used, matching the message's int16_t fields).
// blam-cc: ECX -> item_index, stack -> (magazine_index, rounds)
// UNSURE: network_session_broadcast_to_flagged's role (broadcast vs. local dispatch) is inferred from the sibling
// objects-module evidence above, not re-derived here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "units.h"
#include "game.h"
#include "networking.h"

extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

// Broadcasts a weapon-ammo-pickup network event for one magazine.
void weapon_notify_ammo_pickup(datum_index item_index, int16_t magazine_index, int16_t rounds)
{
    weapon_ammo_pickup_message message;
    void *items[2];

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds = rounds;

    // The 4th argument is an array of record pointers, not the record itself: the
    // original writes `items[0] = &message; items[1] = 0;` and then passes `items`.
    items[0] = &message;
    items[1] = 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_weapon_ammo_pickup, 0, items, 0, 1, 0), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

#if 0
Original Ghidra decompilation (0x4c2510):

void FUN_004c2510(int *param_1,undefined4 param_2)

{
  int in_ECX;
  int local_8;
  undefined2 local_4;
  undefined2 local_2;

  local_8 = 0;
  if (in_ECX != -1) {
    local_8 = hash_table_get();
    if (local_8 == -1) {
      local_8 = 0;
    }
  }
  local_4 = param_1._0_2_;
  param_1 = &local_8;
  local_2 = (undefined2)param_2;
  param_2 = 0;
  message_delta_encode_message(0,0x2c,0,&param_1,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  return;
}
#endif
