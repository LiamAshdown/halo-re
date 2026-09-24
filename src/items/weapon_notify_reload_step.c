// weapon_notify_reload_step  (Ghidra: FUN_004c37b0; named from
// out/phase4/items_functions.md, "Packages a trigger's ammo counts and dispatches a
// network/UI event (id 0x2d) signalling that a reload step has finished")
// address 0x4c37b0, size 183 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: identical shape to weapon_notify_reload_begin.c except for the message type
//   (k_message_weapon_reload_end = 0x2d); same UNSURE caveat about the magazine_index field.
// register convention: item index in ECX; magazine index in BX (unaff_BX).
// blam-cc: ECX -> item_index, BX -> magazine_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *object_pooled_node_globals; // 0x00687130
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern int message_delta_encode_message(int flag, int message_type, int changed_offset,
    void **items, int type_offset, int count, char force_changed); // 0x4ec940
extern void network_session_broadcast_to_flagged(uint32_t a1, void *a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6); // 0x4e1a80

// Broadcasts a reload-step-finished network event for one weapon magazine.
void weapon_notify_reload_step(datum_index item_index, int16_t magazine_index)
{
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_ammo_message message;
    void *items[2];

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0x0c), item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds_unloaded = wd->magazines[magazine_index].rounds_unloaded;
    message.rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

    // The 4th argument is an array of record pointers, not the record itself: the
    // original writes `items[0] = &message; items[1] = 0;` and then passes `items`.
    items[0] = &message;
    items[1] = 0;
    message_delta_encode_message(0, k_message_weapon_reload_end, 0, items, 0, 1, 0);
    network_session_broadcast_to_flagged(1, object_network_message_scratch, 1, 0, 0, 3);
}

#if 0
Original Ghidra decompilation (0x4c37b0):

void FUN_004c37b0(void)

{
  int iVar1;
  uint in_ECX;
  short unaff_BX;
  int *local_14;
  undefined4 local_10;
  int local_c;
  undefined2 local_6;
  undefined2 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  local_c = 0;
  if (in_ECX != 0xffffffff) {
    local_c = hash_table_get();
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_6 = *(undefined2 *)(iVar1 + 0x2b6 + unaff_BX * 0xc);
  local_4 = *(undefined2 *)(iVar1 + (unaff_BX * 3 + 0xae) * 4);
  local_14 = &local_c;
  local_10 = 0;
  message_delta_encode_message(0,0x2d,0,&local_14,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  return;
}
#endif
