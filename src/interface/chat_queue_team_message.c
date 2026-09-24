// chat_queue_team_message  (Ghidra: FUN_004aade0, renamed)
// address 0x4aade0, size 398 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.15
// evidence: phase-4 summary "Queues an encoded chat message for delivery to every connected
// machine whose player is on the given team (or everyone if team_index is -1)"; types/game.h
// player::team_index_desired (offset 0x67, matching the `+0x67 == param_1` compare); reuses
// message_delta_encode_message and bit_stream_write_bits_chunked, both established elsewhere.
// UNSURE: this is one of the least-confident rewrites in this pass. The per-machine session
// table walked here (`DAT_0071c2d4 + 0x3c4` as a 0x10-entry array of int16 with a parallel
// 0x60-byte-stride table at +0x3b8`) is kept as raw offsets on an untyped base; no networking
// struct was brought into scope for this rewrite, and the byte at player+100 (0x64) tested here
// is the same unidentified field used the same way in chat_server_relay_incoming_message.c.
// register convention: team_index as the recognized parameter (param_1).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern data_array *player_data; // 0x0087a480
extern network_server_globals *network_server; // 0x0071c2d4, UNSURE: base for the +0x3b8/+0x3c4 tables

extern const uint16_t chat_local_prompt_string[]; // 0x006607a0, UNSURE: format string passed to string_format_wide_va
extern void string_format_wide_va(const uint16_t *format, ...); // 0x557930, UNSURE signature
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type, uint32_t unknown_2,
                                             void **fields, uint32_t unknown_4, uint32_t unknown_5,
                                             uint8_t unknown_6); // 0x4ec940
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern uint8_t network_channel_stream_flush(uint8_t *session, int32_t unknown); // 0x4ddb60
extern void bit_stream_write_bits_chunked(uint32_t value_or_count); // 0x4cf8f0, UNSURE: elided second argument

// Encodes a chat message and, for every connected machine whose player is on team_index (or
// every machine if team_index is -1), queues the message length and payload bits into that
// machine's outgoing bit stream when there is room (or room can be freed).
void chat_queue_team_message(int32_t team_index)
{
    uint8_t formatted[128]; // local_80
    void *fields = formatted;
    int32_t header_size = 4;
    uint8_t terminator = 0xff;
    int32_t encoded_bits;
    data_iterator iterator;
    player *entry;

    (void)header_size;
    (void)terminator;

    string_format_wide_va(chat_local_prompt_string); // UNSURE: real varargs not recoverable

    encoded_bits = message_delta_encode_message(0, 0xf, 0, &fields, 0, 1, 0);
    if (encoded_bits <= 0) {
        return;
    }

    iterator.data = player_data; // UNSURE
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (player *)data_iterator_next(&iterator);

    while (entry != 0) {
        if (team_index == -1 || entry->team_index_desired == team_index) {
            if (*((int8_t *)entry + 0x64) != -1) { // UNSURE offset, see header
                int32_t machine_index = *((int8_t *)entry + 0x64);
                int32_t i;
                int16_t *slot_table = (int16_t *)((uint8_t *)network_server + 0x3c4);
                for (i = 0; i < 0x10; i = i + 1) {
                    if (slot_table[i * 0x30] == machine_index) {
                        uint8_t **session_ptr = (uint8_t **)((uint8_t *)network_server + 0x3b8 + i * 0x60);
                        uint8_t *session = *session_ptr;
                        if (session != 0 && (session[0xa8c] & 1) == 0 &&
                            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) +
                                                   *(int32_t *)(session + 0x1c) * -8) -
                                                      *(int32_t *)(session + 0x20) + 1 ||
                             network_channel_stream_flush(session, 1) != 0)) {
                            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
                            bit_stream_write_bits_chunked(1);
                            session[0x2c] = 0;
                            bit_stream_write_bits_chunked((uint32_t)encoded_bits);
                            session[0x2c] = 0;
                        }
                        break;
                    }
                }
            }
            if (team_index != -1) {
                return;
            }
        }
        entry = (player *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4aade0):

void FUN_004aade0(int param_1)

{
  int *piVar1;
  char cVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  short *psVar6;
  int iVar7;
  undefined4 *local_a4;
  int local_a0;
  uint local_9c;
  undefined2 local_98;
  undefined4 local_94;
  uint local_90;
  undefined4 local_8c;
  undefined1 local_88;
  undefined1 *local_84;
  undefined1 local_80 [128];

  local_84 = local_80;
  local_8c = 4;
  local_88 = 0xff;
  string_format_wide_va(&PTR_s_parameter_handles_0063fff0_0x35_006607a0);
  local_a4 = &local_8c;
  local_a0 = 0;
  puVar3 = (undefined4 *)message_delta_encode_message(0,0xf,0,&local_a4,0,1,'\0');
  iVar7 = DAT_0071c2d4;
  if (0 < (int)puVar3) {
    local_9c = DAT_0087a480;
    local_90 = DAT_0087a480 ^ 0x69746572;
    local_a0 = DAT_0071c2d4;
    local_98 = 0;
    local_94 = 0xffffffff;
    local_a4 = puVar3;
    iVar4 = data_iterator_next();
    while (iVar4 != 0) {
      if ((param_1 == -1) || (*(char *)(iVar4 + 0x67) == param_1)) {
        if (*(char *)(iVar4 + 100) != -1) {
          iVar5 = 0;
          psVar6 = (short *)(iVar7 + 0x3c4);
          do {
            if ((int)*psVar6 == (int)*(char *)(iVar4 + 100)) {
              piVar1 = (int *)(iVar5 * 0x60 + 0x3b8 + iVar7);
              if ((((piVar1 != (int *)0x0) && (iVar7 = *piVar1, iVar7 != 0)) &&
                  ((*(byte *)(iVar7 + 0xa8c) & 1) == 0)) &&
                 (((int)puVar3 + 1 <=
                   ((*(int *)(iVar7 + 0x24) + *(int *)(iVar7 + 0x1c) * -8) - *(int *)(iVar7 + 0x20))
                   + 1 || (cVar2 = FUN_004ddb60(iVar7,1), cVar2 != '\0')))) {
                *(int *)(iVar7 + 0xa80) = *(int *)(iVar7 + 0xa80) + (int)puVar3 + 1;
                bit_stream_write_bits_chunked(1);
                *(undefined1 *)(iVar7 + 0x2c) = 0;
                bit_stream_write_bits_chunked(local_a4);
                *(undefined1 *)(iVar7 + 0x2c) = 0;
              }
              break;
            }
            iVar5 = iVar5 + 1;
            psVar6 = psVar6 + 0x30;
          } while (iVar5 < 0x10);
        }
        puVar3 = local_a4;
        iVar7 = local_a0;
        if (param_1 != -1) {
          return;
        }
      }
      iVar4 = data_iterator_next();
    }
  }
  return;
}
#endif
