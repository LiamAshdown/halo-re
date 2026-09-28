// unit_dispatch_scripted_event_1b  (Ghidra: FUN_0056dcd0)
// address 0x56dcd0, size 221 bytes, name confidence 0.3, rewrite confidence 0.25
// functions.md: "Dispatches a scripted event (id 0x1b) that includes hashed identifiers for
// both the unit and its currently equipped weapon."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8).
// blam-cc: param_1 -> event_byte, in_ECX -> hash_key (unit).
// UNSURE: same unresolved plumbing as unit_dispatch_scripted_event_9.c (0x56c370).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t event9_target;   // 0x00871de0, shared with unit_dispatch_scripted_event_9.c

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index) // blam-cc: param_1, in_ECX
{
    int32_t unit_hash = 0;
    if (unit_index != 0xffffffff) {
        unit_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)unit_index);
        if (unit_hash == -1) {
            unit_hash = 0;
        }
    }

    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int32_t weapon_hash = 0;
    if (current_weapon != k_datum_index_none) {
        weapon_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)current_weapon);
        if (weapon_hash == -1) {
            weapon_hash = 0;
        }
    }

    // 0x56dcf7..0x56dd64: the item record is {unit_hash, weapon_hash, event_byte}; the items array is one pointer to it
    struct { int32_t unit_hash; int32_t weapon_hash; uint8_t event_byte; } item;
    void *items[1];
    int32_t encoded_len;
    item.unit_hash = unit_hash;
    item.weapon_hash = weapon_hash;
    item.event_byte = event_byte;
    items[0] = &item;
    encoded_len = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x1b, 0, items, 0, 1, 0);
    if (0 < encoded_len) {
        network_session_broadcast_to_flagged(encoded_len, network_server_pointer, 1, network_message_scratch, 1, 0, 0, 3); // 0x871de0 data
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56dcd0):

void FUN_0056dcd0(int *param_1)

{
  short sVar1;
  int iVar2;
  uint in_ECX;
  uint extraout_ECX;
  undefined4 uVar3;
  int local_c;
  int local_8;
  undefined1 local_4;

  local_c = 0;
  if (in_ECX != 0xffffffff) {
    local_c = hash_table_get();
    in_ECX = extraout_ECX;
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_4 = param_1._0_1_;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0x2f2);
  local_8 = -1;
  if (sVar1 != -1) {
    local_8 = *(int *)(iVar2 + 0x2f8 + sVar1 * 4);
  }
  iVar2 = 0;
  if (local_8 != -1) {
    iVar2 = hash_table_get();
    if (iVar2 == -1) {
      iVar2 = 0;
    }
  }
  param_1 = &local_c;
  uVar3 = 0;
  local_8 = iVar2;
  iVar2 = message_delta_encode_message(0,0x1b,0,&param_1,0,1,'\0');
  if (0 < iVar2) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3,uVar3);
  }
  return;
}
#endif
