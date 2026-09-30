// chat_server_relay_incoming_message  (Ghidra: FUN_004aabd0, renamed)
// address 0x4aabd0, size 512 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.85 (REWRITTEN; was 0.15)
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
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4aabd0..0x4aadd0): EAX is the decode
// context, the stack holds the sending machine. The chat record {scope, sender byte, text pointer} decodes into
// locals (the text into a local 0x200-byte buffer); the sender's player index comes from
// network_object_owner_team_index_desired(machine) (0x4e0cf0, EAX) and a -1 drops the message. The record is
// re-encoded (message type 0xf) into 0x871de0 and sent through network_server (0x71c2d4): scope 0 to every flagged
// machine (0x4e1a80), scope 1 to every player on the sender's team, scope 2 to every player whose unit's vehicle
// (+0x11c) is the sender's (player_get_vehicle) -- each such player's machine (+0x64, not -1) gets
// network_session_send_to_machine (0x4e1930). The previous C decoded into nothing and sent with invented arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "fn_interface.h"
#include <stdint.h>

extern network_server_globals *network_server;   // 0x0071c2d4
extern data_array *player_data;                     // 0x0087a480
extern uint8_t network_message_scratch[0x7ff8];     // 0x00871de0

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context
extern int32_t network_object_owner_team_index_desired(void *obj); // 0x4e0cf0, EAX
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit,
    void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, EAX, ESI, stack
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server, int32_t status_bit,
    void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused); // 0x4e1a80, EAX, ECX, stack
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

typedef struct chat_relay_message {
    int32_t scope;                 // 0x00: 0 everyone, 1 team, 2 vehicle
    uint8_t sender;                // 0x04, player index
    uint8_t pad_05[3];
    void *text;                    // 0x08 -> the local text buffer
} chat_relay_message;

static void chat_relay_iterator_begin(data_iterator *iterator)
{
    iterator->data = player_data;
    iterator->next_index = 0;
    iterator->index = (datum_index)0xffffffff;
    iterator->signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;
}

void chat_server_relay_incoming_message(void **context, void *machine)
{
    chat_relay_message message;
    void *item;
    uint32_t zero_24;
    uint8_t text[0x200];
    int32_t sender;
    int32_t bits;
    data_iterator iterator;
    uint8_t *entry;

    if (*(int32_t *)context[0] != 0) {
        message_delta_decode_compound_field_staged(context);
        return;
    }
    message.scope = 0;
    message.sender = 0xff;
    message.text = text;
    if (message_delta_decode_compound_field(context, &message) == 0) {
        return;
    }
    sender = network_object_owner_team_index_desired(machine);
    if (sender == -1) {
        return;
    }
    message.sender = (uint8_t)sender;
    item = &message;
    zero_24 = 0;
    (void)zero_24;
    bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, &item, 0, 1, 0);
    if (message.scope == 0) {
        network_session_broadcast_to_flagged(bits, network_server, 1, network_message_scratch, 1, 0, 1, 3);
    } else if (message.scope == 1) {
        uint8_t *sender_player = (uint8_t *)datum_get((datum_index)message.sender, player_data);

        if (sender_player == 0) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)data_iterator_next(&iterator)) != 0) {
            if (*(int32_t *)(entry + 0x20) == ((struct player *)sender_player)->team && *(int8_t *)(entry + 0x64) != -1) {
                network_session_send_to_machine(*(int8_t *)(entry + 0x64), network_server, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    } else if (message.scope == 2) {
        datum_index vehicle = player_get_vehicle((datum_index)message.sender);

        if (vehicle == (datum_index)0xffffffff) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)data_iterator_next(&iterator)) != 0) {
            uint8_t *unit = (uint8_t *)object_try_and_get(*(datum_index *)(entry + 0x34), 3);

            if (unit != 0 && ((unit_object *)unit)->base.parent_object == vehicle && *(int8_t *)(entry + 0x64) != -1) {
                network_session_send_to_machine(*(int8_t *)(entry + 0x64), network_server, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
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
