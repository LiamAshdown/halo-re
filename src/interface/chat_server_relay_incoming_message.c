// chat_server_relay_incoming_message  (Ghidra: FUN_004aabd0, renamed)
// address 0x4aabd0, size 512 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.15
// evidence: phase-4 summary "Server-side relay that re-broadcasts a received chat message only
// to the machines/players matching its addressed scope (everyone, team, or vehicle)"; reuses
// message_delta_encode_message, network_session_send_to_machine, player_get_vehicle,
// object_try_and_get, datum_get and network_message_scratch (0x00871de0), all established
// elsewhere in this pass or in src/effects/.
// UNSURE: this is one of the least-confident rewrites in this pass. The incoming message
// pointer (`in_EAX`, a pointer-to-pointer) and the "scope" encoding built into `local_220`/
// `local_21c` before message_delta_encode_message are not understood beyond their literal
// field writes; `local_224[8]` (player->team at the datum_get result) and the seat-search
// pattern shared with chat_default_team_channel/player_get_vehicle are the only
// pieces independently corroborated. Kept as a mechanical transcription rather than a
// confidently-named data flow.
// register convention: incoming message pointer in EAX (in_EAX, unresolved register read).
//   // blam-cc: message -> EAX
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern data_array *player_data; // 0x0087a480

extern void message_delta_decode_compound_field_staged(void); // 0x4ec670, UNSURE signature, not in this module's range (client path)
extern uint8_t message_delta_decode_compound_field(void); // 0x4ec590, UNSURE signature, not in this module's range
extern int32_t network_object_owner_team_index_desired(void); // 0x4e0cf0, UNSURE signature, not in this module's range
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
                                             uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5,
                                             uint32_t unknown_6); // 0x4e1930
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, int32_t length, uint32_t unknown_3,
                          uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80, UNSURE signature
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680
extern datum_index player_get_vehicle(datum_index player_index); // 0x4ab170, blam-cc: ECX player_index; the vehicle of the player unit or -1
extern void *object_try_and_get(int32_t kind); // 0x4f6ec0

// blam-cc: message -> EAX
// If the given network message is not a local echo, re-encodes it and rebroadcasts it to every
// connected machine matching its addressed scope: 0 everyone, 1 the addressed player's team, 2
// the addressed unit's vehicle's occupants.
void chat_server_relay_incoming_message(int32_t **message)
{
    if (**message != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    {
        int32_t scope;
        uint8_t sender_index = 0xff;

        if (message_delta_decode_compound_field()) {
            int32_t sender = network_object_owner_team_index_desired();
            if (sender != -1) {
                sender_index = (uint8_t)sender;
            }
        }

        {
            void *fields = &scope;
            int32_t local_204 = 0;
            int32_t encoded_bits;
            scope = 0;
            (void)local_204;
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, &fields, 0, 1, 0);

            if (scope == 0) {
                network_session_broadcast_to_flagged(1, network_message_scratch, 1, 0, 1, 3);
                return;
            }
            if (scope == 1) {
                player *target = (player *)datum_get((datum_index)player_data, player_data); // UNSURE: real handle argument not recoverable
                if (target != 0) {
                    data_iterator iterator;
                    player *entry;
                    iterator.data = player_data; // UNSURE
                    iterator.next_index = 0;
                    iterator.index = (datum_index)-1;
                    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
                    entry = (player *)data_iterator_next(&iterator);
                    while (entry != 0) {
                        if (entry->team == target->team && *((int8_t *)entry + 0x64) != -1) { // UNSURE offset, see header
                            network_session_send_to_machine(1, network_message_scratch, encoded_bits, 1, 0, 1, 3);
                        }
                        entry = (player *)data_iterator_next(&iterator);
                    }
                }
            } else if (scope == 2) {
                int32_t target_player = player_get_vehicle((datum_index)0); // UNSURE: real argument not recoverable
                if (target_player != -1) {
                    data_iterator iterator;
                    player *entry;
                    iterator.data = player_data; // UNSURE
                    iterator.next_index = 0;
                    iterator.index = (datum_index)-1;
                    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
                    entry = (player *)data_iterator_next(&iterator);
                    while (entry != 0) {
                        object *unit_obj = (object *)object_try_and_get(3);
                        if (unit_obj != 0 && (int32_t)unit_obj->parent_object == target_player &&
                            *((int8_t *)entry + 0x64) != -1) { // UNSURE offset, see header
                            network_session_send_to_machine(1, network_message_scratch, encoded_bits, 1, 0, 1, 3);
                        }
                        entry = (player *)data_iterator_next(&iterator);
                    }
                }
            }
            (void)sender_index;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4aabd0):

void FUN_004aabd0(void)

{
  uint uVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int iVar4;
  int iVar5;
  int *local_224;
  int local_220;
  undefined1 local_21c;
  undefined1 *local_218;
  uint local_214;
  undefined2 local_210;
  undefined4 local_20c;
  uint local_208;
  undefined4 local_204;
  undefined1 local_200 [512];

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  local_218 = local_200;
  local_220 = 0;
  local_21c = 0xff;
  cVar2 = FUN_004ec590();
  if ((cVar2 != '\0') && (iVar3 = FUN_004e0cf0(), iVar3 != -1)) {
    local_21c = (undefined1)iVar3;
    local_224 = &local_220;
    local_204 = 0;
    iVar3 = message_delta_encode_message(0,0xf,0,&local_224,0,1,'\0');
    uVar1 = DAT_0087a480;
    if (local_220 == 0) {
      FUN_004e1a80(1,&DAT_00871de0,1,0,1,3);
      return;
    }
    if (local_220 == 1) {
      local_224 = (int *)datum_get();
      if (local_224 != (int *)0x0) {
        local_214 = uVar1;
        local_210 = 0;
        local_208 = uVar1 ^ 0x69746572;
        local_20c = 0xffffffff;
        iVar4 = data_iterator_next();
        if (iVar4 != 0) {
          do {
            if ((*(int *)(iVar4 + 0x20) == local_224[8]) && (*(char *)(iVar4 + 100) != -1)) {
              network_session_send_to_machine(1,&DAT_00871de0,iVar3,1,0,1,3);
            }
            iVar4 = data_iterator_next();
          } while (iVar4 != 0);
          return;
        }
      }
    }
    else if ((local_220 == 2) &&
            (local_224 = (int *)player_index_from_unit_index(), local_224 != (int *)0xffffffff)) {
      local_214 = DAT_0087a480;
      local_210 = 0;
      local_208 = DAT_0087a480 ^ 0x69746572;
      local_20c = 0xffffffff;
      iVar4 = data_iterator_next();
      while (iVar4 != 0) {
        iVar5 = object_try_and_get(3);
        if (((iVar5 != 0) && (*(int **)(iVar5 + 0x11c) == local_224)) &&
           (*(char *)(iVar4 + 100) != -1)) {
          network_session_send_to_machine(1,&DAT_00871de0,iVar3,1,0,1,3);
        }
        iVar4 = data_iterator_next();
      }
    }
  }
  return;
}
#endif
