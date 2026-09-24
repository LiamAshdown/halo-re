// player_effect_send_network_update  (Ghidra: FUN_00456bc0, still unnamed there; named per
//   out/phase4/effects_types_notes.md's misattribution table entry for this address)
// address 0x456bc0, size 293 bytes
// name confidence: 0.5   rewrite confidence: 0.2 (LOW -- see UNSURE)
// evidence: types/game.h player.marked_for_deletion (+0xd5, the same validate-and-check idiom
//   src/game's own player lookups use against player_data 0x0087a480); src/game/*.c establish
//   message_delta_encode_message's (unknown_0, message_type, unknown_2, fields, unknown_4,
//   unknown_5, unknown_6) signature and hash_table_get's opaque no-argument convention;
//   src/game/game_engine_notify_kill_event.c names 0x00871de0 network_message_scratch.
// register convention: player handle in EAX (in_EAX); a small descriptor pointer as the
//   recognized first stack parameter (param_1); two further stack parameters (param_2, param_3)
//   forwarded straight into the encoded message.
//   // blam-cc: in_EAX -> player_handle, stack -> (descriptor, param_2, param_3)
// UNSURE (heavily): `unaff_EBX`, a 3 dword vector, is a pure register argument Ghidra never
//   resolves a value for anywhere in this function -- modeled as a direction/offset vector by
//   analogy with player_effect_mark_damage_direction 0x456cf0's damage-direction context, since
//   that is the only nearby function in this group that builds one, but no call site proves it.
//   `descriptor[3]`'s hash_table_get lookup (an object index -> network id translation,
//   matching game_engine_notify_kill_event.c's own use of the same idiom) is likewise a guess at
//   the parameter's role, not a read field name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *object_pooled_node_globals; // 0x00687130, the object network-id hash_table sits at +0x0c
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

// Encodes and replicates a small player-effect message (message type 0xb) built from a
// descriptor {tag/definition dword, unknown, unused, object-index-or--1} and a 3 dword vector,
// for the given player, unless that player is already marked for deletion.
void player_effect_send_network_update(datum_index player_handle, uint32_t *descriptor,
    real_vector3d *vector, uint32_t param_2, uint32_t param_3) // blam-cc: in_EAX, stack, UNSURE
                                    // (unaff_EBX), stack, stack
{
    int16_t index = (int16_t)player_handle;

    if (player_handle == (datum_index)0xffffffff || index < 0 || index >= player_data->maximum_count) {
        return;
    }

    {
        player *record = &((player *)player_data->data)[index];
        int16_t salt = (int16_t)(player_handle >> 16);

        if (record->identifier == 0 || (salt != 0 && record->identifier != salt) ||
            record->marked_for_deletion != 0) {
            return;
        }
        uint32_t fields[8];
        void *fields_ptr = fields;
        int32_t encoded_bits;

        fields[0] = descriptor[0];
        fields[1] = 0;
        if (descriptor[3] != 0xffffffff) {
            fields[1] = (uint32_t)hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), (int32_t)descriptor[3]); // 0x456c29..0x456c43
            if (fields[1] == 0xffffffff) {
                fields[1] = 0;
            }
        }
        fields[2] = descriptor[1];
        fields[3] = *(uint32_t *)&vector->i;
        fields[4] = *(uint32_t *)&vector->j;
        fields[5] = *(uint32_t *)&vector->k;
        fields[6] = param_2;
        fields[7] = param_3;

        encoded_bits = message_delta_encode_message(0, 0xb, 0, &fields_ptr, 0, 1, 0);
        if (encoded_bits > 0 && (int8_t)record->unknown_64 != -1) {
            network_session_send_to_machine(1, network_message_scratch, encoded_bits, 1, 0, 1, 3);
        }
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
