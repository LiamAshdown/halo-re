// game_engine_send_unit_weapon_loadout  (Ghidra: FUN_00477a80; named per
// out/phase4/game_functions.md: "Sends a network message replicating a unit's full weapon
// loadout (current weapon plus 4 inventory slots) to observers.")
// address 0x477a80, size 481 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: types/units.h unit_data (vehicle_seat_index 0x2f0, current_weapon_index 0x2f2,
//   desired_weapon_index 0x2f4, weapons[4] 0x2f8); types/objects.h object::parent_object
//   (0x11c); types/game.h player::kill_streak (0x68); the player-index-from-handle idiom
//   `(handle & 0xffff) * sizeof(player) + player_data` used throughout this module; the
//   already-rewritten game_engine_notify_kill_event.c / game_engine_koth_broadcast_hill_times.c
//   establish the message_delta_encode_message / network_session_send_to_machine /
//   network_session_broadcast_to_flagged broadcast-or-unicast shape reused verbatim here. `object_network_id_table`
//   (0x00687130) is the same foreign, not-owned-by-this-module byte pointer already declared in
//   src/objects/*.c and src/items/*.c; the inline open-hash-bucket walk at +0xc/+0x10/+0x14 is
//   this function's own (no other rewritten file in the repository indexes those particular
//   offsets), kept as raw pointer arithmetic rather than invented as a new named struct.
// register convention: the unit object's index in EAX (in_EAX); `player_handle`, `value` and
//   `machine_index` are this function's own three stack parameters.
//   // blam-cc: EAX -> unit_index, stack -> player_handle, value, machine_index
// UNSURE: the exact identity/shape of the hash table object_network_id_table+0xc/+0x10/+0x14
//   points into (kept as raw offsets with the struct-shaped access it compiles to); hash_table_
//   get's real argument list (elided by Ghidra at all three of its call sites here, same as
//   every other call site of it in this module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *object_headers;      // 0x008603b0
extern data_array *player_data;         // 0x0087a480
extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t shared_hud_text_draw_state; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *machine_table; // 0x00687558, its hash_table sits at +0x0c
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

// UNSURE: manual open-hash lookup mirroring hash_table_get's own chained-bucket walk, inlined
// by the compiler against object_network_id_table' own (unnamed) hash table sub-structure.
static int32_t pooled_node_hash_lookup(int32_t key)
{
    int32_t result = 0;
    if (object_network_id_table->id_to_index.initialized == 1) {
        int32_t magnitude = (key < 0) ? -key : key;
        int32_t bucket_count = object_network_id_table->id_to_index.bucket_count;
        int32_t *bucket_table = (int32_t *)object_network_id_table->id_to_index.buckets;
        int32_t *node = *(int32_t **)((uint8_t *)bucket_table + 4 + (magnitude % bucket_count) * 8);
        result = -1;
        while (node != 0) {
            if (*node == key) {
                result = node[1];
                break;
            }
            node = (int32_t *)node[2];
        }
    }
    if (result == -1) {
        result = 0;
    }
    return result;
}

// blam-cc: EAX -> unit_index, stack -> player_handle, value, machine_index
// Builds a network message carrying: hashes of `player_handle` and `unit_index` (0 if either
// is -1 or unmapped), `value` verbatim, a hash of the unit's parent object (if it has one), the
// unit's vehicle seat index, its current weapon index (falling back to desired_weapon_index
// when unarmed), a hash of each of its four weapon inventory slots, and the owning player's
// kill-streak pair -- then sends it to `machine_index`, or broadcasts it via network_session_broadcast_to_flagged when
// machine_index is -1.
void game_engine_send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle,
    int32_t value, int32_t machine_index)
{
    struct {
        int32_t player_hash;
        int32_t unit_hash;
        int32_t value;
        int32_t parent_hash;
        int16_t vehicle_seat_index;
        int16_t pad;
        int32_t weapon_hashes[4];
        int16_t current_weapon_index;
        int16_t kill_streak[2];
    } fields;
    void *fields_ptr;
    int32_t encoded_bits;
    object *obj;
    unit_data *unit;
    int i;

    obj = (object *)((object_header *)object_headers->data)[unit_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset); // unit_data starts at object +0x1f4 (0x477b14: +0x2f0)

    fields.player_hash = 0;
    if (player_handle != (datum_index)0xffffffff) {
        fields.player_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle); // 0x477aa4..0x477ab4
        if (fields.player_hash == -1) {
            fields.player_hash = 0;
        }
    }

    fields.unit_hash = 0;
    if (unit_index != 0xffffffff) {
        fields.unit_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)unit_index); // 0x477ac9..0x477ad6 (ECX = EBX)
        if (fields.unit_hash == -1) {
            fields.unit_hash = 0;
        }
    }

    fields.value = value;

    fields.parent_hash = 0;
    if (obj->parent_object != (datum_index)0xffffffff) {
        fields.parent_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)obj->parent_object); // 0x477aee..0x477b04
        if (fields.parent_hash == -1) {
            fields.parent_hash = 0;
        }
    }

    fields.vehicle_seat_index = unit->vehicle_seat_index;
    fields.current_weapon_index = unit->current_weapon_index;
    if (fields.current_weapon_index == -1) {
        fields.current_weapon_index = unit->desired_weapon_index;
    }

    for (i = 0; i < 4; i++) {
        int32_t weapon_handle = (int32_t)unit->weapons[i];
        fields.weapon_hashes[i] = (weapon_handle != -1) ? pooled_node_hash_lookup(weapon_handle) : 0;
    }

    {
        player *owner = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        fields.kill_streak[0] = owner->kill_streak[0];
        fields.kill_streak[1] = owner->kill_streak[1];
    }

    fields_ptr = &fields;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 8, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server_pointer, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
            return;
        }
        network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 1, 3);
    }
}

#if 0
Original Ghidra decompilation (0x477a80), from tools/pack.py 0x477a80:

void FUN_00477a80(int *param_1,undefined4 param_2,int param_3)

{
  int *piVar1;
  void *pvVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int *piVar7;
  int local_2c;
  int local_28;
  undefined4 local_24;
  int local_20;
  undefined2 local_1c;
  int aiStack_18 [4];
  short local_8;
  undefined2 local_6;
  undefined2 local_4;

  pvVar2 = param_1;
  iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  local_2c = 0;
  if ((param_1 != (void *)0xffffffff) && (local_2c = hash_table_get(), local_2c == -1)) {
    local_2c = 0;
  }
  local_28 = 0;
  if ((in_EAX != 0xffffffff) && (local_28 = hash_table_get(), local_28 == -1)) {
    local_28 = 0;
  }
  local_24 = param_2;
  local_20 = 0;
  if ((*(int *)(iVar6 + 0x11c) != -1) && (local_20 = hash_table_get(), local_20 == -1)) {
    local_20 = 0;
  }
  local_1c = *(undefined2 *)(iVar6 + 0x2f0);
  local_8 = *(short *)(iVar6 + 0x2f2);
  if (local_8 == -1) {
    local_8 = *(short *)(iVar6 + 0x2f4);
  }
  iVar5 = 0;
  piVar7 = (int *)(iVar6 + 0x2f8);
  do {
    iVar6 = *piVar7;
    iVar3 = 0;
    if (iVar6 != -1) {
      iVar3 = -1;
      if (PTR_DAT_00687130[0xc] == '\x01') {
        iVar4 = iVar6;
        if (iVar6 < 0) {
          iVar4 = -iVar6;
        }
        for (piVar1 = *(int **)(*(int *)(PTR_DAT_00687130 + 0x14) + 4 +
                               (iVar4 % *(int *)(PTR_DAT_00687130 + 0x10)) * 8);
            piVar1 != (int *)0x0; piVar1 = (int *)piVar1[2]) {
          if (*piVar1 == iVar6) {
            iVar3 = piVar1[1];
            break;
          }
        }
      }
      if (iVar3 == -1) {
        iVar3 = 0;
      }
    }
    aiStack_18[iVar5] = iVar3;
    iVar5 = iVar5 + 1;
    piVar7 = piVar7 + 1;
    if (3 < iVar5) {
      iVar6 = ((uint)pvVar2 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
      local_6 = *(undefined2 *)(iVar6 + 0x68);
      local_4 = *(undefined2 *)(iVar6 + 0x6a);
      param_1 = &local_2c;
      param_2 = 0;
      iVar6 = message_delta_encode_message(0,8,0,&param_1,0,1,'\0');
      if (0 < iVar6) {
        if (param_3 == -1) {
          FUN_004e1a80(1,&DAT_00871de0,1,0,0);
          return;
        }
        network_session_send_to_machine(1,&DAT_00871de0,iVar6,1,0,1,3);
      }
      return;
    }
  } while( true );
}
#endif
