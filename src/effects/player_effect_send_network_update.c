// player_effect_send_network_update  (Ghidra: FUN_00456bc0, still unnamed there; named per
//   out/phase4/effects_types_notes.md's misattribution table entry for this address)
// address 0x456bc0, size 293 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x456bc0..0x456ce4 and its callers in object_apply_damage (0x4eeaac, 0x4eeaf3): EAX is
//   the player, EBX the damage direction (damage_data +0x34), the stack (damage_data, random blend, damage
//   amount). Unless the player is gone or marked for deletion, a type 0xb message is encoded from {damage effect
//   tag, network id of the responsible object (0 when none), damage flags, direction, blend, amount}
//   (message_delta_encode_message, items {&fields, 0}) and sent to the player's machine (+0x64, reliable,
//   priority 3) through network_server (0x71c2d4).
// blam-cc: EAX -> player_handle, EBX -> direction, stack -> (dd, random_blend, damage_amount)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern network_id_table *object_network_id_table; // 0x00687130
extern network_server_globals *network_server; // 0x0071c2d4

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server,
    uint32_t param_1, void *data, uint32_t param_3, uint32_t reliable, uint32_t unknown_a,
    char force, uint32_t priority); // 0x4e1930, EAX, ESI, stack

void player_effect_send_network_update(datum_index player_handle, const real_vector3d *direction,
    const damage_data *dd, float random_blend, float damage_amount)
{
    int16_t index = (int16_t)player_handle;
    int16_t salt = (int16_t)(player_handle >> 16);
    player *record;
    uint32_t fields[8];
    void *items[2];
    int32_t encoded_bits;

    if (player_handle == (datum_index)0xffffffff || index < 0 || index >= player_data->maximum_count) {
        return;
    }
    record = (player *)((uint8_t *)player_data->data + index * player_data->size);
    if (record->identifier == 0 || (salt != 0 && record->identifier != salt) || record->marked_for_deletion != 0) {
        return;
    }
    fields[0] = dd->damage_effect_tag;
    fields[1] = 0;
    if (dd->responsible_object != (datum_index)0xffffffff) {
        fields[1] = (uint32_t)hash_table_get(&object_network_id_table->id_to_index,
            (int32_t)dd->responsible_object);
        if (fields[1] == 0xffffffff) {
            fields[1] = 0;
        }
    }
    fields[2] = dd->flags;
    fields[3] = *(const uint32_t *)&direction->i;
    fields[4] = *(const uint32_t *)&direction->j;
    fields[5] = *(const uint32_t *)&direction->k;
    *(float *)&fields[6] = random_blend;
    *(float *)&fields[7] = damage_amount;
    items[0] = fields;
    items[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xb, 0, items, 0, 1, 0);
    if (encoded_bits > 0 && (int8_t)record->unknown_64 != -1) {
        network_session_send_to_machine((int8_t)record->unknown_64, network_server, 1, network_message_scratch,
            (uint32_t)encoded_bits, 1, 0, 1, 3);
    }
}

#if 0
Original Ghidra decompilation (0x456bc0):

void FUN_00456bc0(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  short sVar3;
  undefined4 *unaff_EBX;
  int iVar4;
  undefined4 *local_28;
  undefined4 local_24;
  undefined4 local_20;
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if (((in_EAX != -1) && (sVar1 = (short)in_EAX, -1 < sVar1)) &&
     (sVar1 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar1;
    sVar1 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
    iVar4 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
    if (((sVar1 != 0) && ((sVar3 = (short)((uint)in_EAX >> 0x10), sVar3 == 0 || (sVar1 == sVar3))))
       && (*(char *)(iVar4 + 0xd5) == '\0')) {
      local_20 = *param_1;
      local_1c = 0;
      if ((param_1[3] != -1) && (local_1c = hash_table_get(), local_1c == -1)) {
        local_1c = 0;
      }
      local_18 = param_1[1];
      local_10 = unaff_EBX[1];
      local_14 = *unaff_EBX;
      local_8 = param_2;
      local_c = unaff_EBX[2];
      local_28 = &local_20;
      local_4 = param_3;
      local_24 = 0;
      iVar2 = message_delta_encode_message(0,0xb,0,&local_28,0,1,'\0');
      if ((0 < iVar2) && (*(char *)(iVar4 + 100) != -1)) {
        network_session_send_to_machine(1,&DAT_00871de0,iVar2,1,0,1,3);
      }
    }
  }
  return;
}
#endif
