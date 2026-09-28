// network_player_ping_field_update_and_report  (Ghidra: FUN_004dbaa0; renamed, no prior name)
// address 0x4dbaa0, size 342 bytes
// name confidence: 0.35   rewrite confidence: 0.25 (LOW -- several elided-register inputs; see
// UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Updates a player-table ping/latency-
// related field, scans an iterator of entries to compute an elapsed-time delta, and queues a
// short reliable status message reporting it"). player_data (0x0087a480) offsets +0x20/+0x22/
// +0x34 match types/memory.h's data_array exactly; player+0x02/+0x67/+0xdc match types/game.h's
// player::local_player_index/team_index_desired/unknown_dc. Ghidra's own decompile carries a
// "Removing unreachable block" warning.
// register convention: none recovered by Ghidra; every value this function reads before its
// first real call is either a global or an elided register output. // blam-cc: none
// UNSURE (major): `FUN_004ec590` is called with no visible arguments and its result is read back
// through two locals (`local_58`, a player-table index; `local_54`, a value to store into that
// player's unknown_dc) that Ghidra shows as uninitialized until that call -- the classic
// register-output-not-tracked pattern used throughout this codebase. Modeled as two extra local
// variables assigned from that call's own (unrecoverable) implicit outputs; since no value can
// be recovered, they are left uninitialized here exactly as Ghidra shows, which the compiler is
// asked to accept via explicit (if arbitrary) initialization to 0/0xffffffff matching Ghidra's
// own sentinel conventions elsewhere in this cluster, not a confirmed value.
// UNSURE: the data_iterator (`local_44`/`local_4c`/`local_48` in Ghidra) is reconstructed as a
// real `data_iterator` bound to `player_data`; the "data ^ 0x69746572" dword is its +0x0c
// signature (types/memory.h).
// UNSURE: `network_server+0x9c0` falls inside types/networking.h's still-unresolved
// network_server_globals::unknown_9bc[0x3c] block; accessed via a raw offset rather than a new
// named field.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include "objects.h"
#include "units.h"

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern data_array *player_data; // 0x0087a480
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern int16_t network_game_mode; // 0x00719720
extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t time_query_performance_counter_ms(void); // 0x449210, cseries: current time in milliseconds
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940
extern network_client_globals *network_client; // 0x0071c2d8
extern void network_channel_reliable_pool_store(network_channel *channel, void *message, uint8_t *out_flag,
    int32_t priority); // 0x4dcdb0, not in this batch

// REVIEW PASS 2026-09-20: FUN_004ec590 takes the decode context in EAX and the destination in
// ECX (0x4dbaa3 `lea ecx,[esp+0xc]`). The scratch it fills is what supplies player_index
// (scratch[0], read back at 0x4dbab4) and new_value; both were modelled as elided outputs.
// The EAX argument is this function own incoming EAX, which Ghidra dropped entirely -- the
// parameter below is added to carry it.
//   // blam-cc: EAX -> decode_context
void network_player_ping_field_update_and_report(void *decode_context) // blam-cc: EAX -> decode_context
{
    char ok;
    uint8_t decode_scratch[0x58]; // [esp+0x0c] .. the end of the 0x64-byte frame
    uint8_t player_index; // scratch[0]
    int32_t new_value;    // UNSURE: which scratch dword; see file header
    data_iterator iter;
    void *element;
    char team_index; // the value the encoded message ultimately reports
    uint8_t fields_byte0;
    uint8_t fields_byte1;
    uint8_t *fields_ptr;
    int32_t fields_pad;
    int32_t encoded_bits;
    uint8_t message_buffer[64];

    ok = message_delta_decode_compound_field(decode_context, decode_scratch);
    player_index = decode_scratch[0];
    new_value = 0;          // UNSURE: not re-derived from the scratch; see file header
    if (ok == 1) {
        team_index = -1;
        if (player_index != 0xff && (int16_t)(uint16_t)player_index < player_data->maximum_count) {
            uint8_t *player = (uint8_t *)player_data->data + player_data->size * (int16_t)(uint16_t)player_index;
            if (*(int16_t *)player != 0) {
                ((struct player *)player)->unknown_dc = new_value;
            }
        }

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = data_iterator_next(&iter);
        if (element != 0) {
            team_index = -1;
            do {
                if (((player *)element)->local_player_index != -1) {
                    team_index = ((player *)element)->team_index_desired;
                    if (team_index != -1 && network_game_mode == 2) {
                        int32_t base = *(int32_t *)((uint8_t *)network_server + 0x9c0);
                        int32_t now = time_query_performance_counter_ms();
                        ((player *)element)->unknown_dc = now - base;
                        return;
                    }
                    break;
                }
                element = data_iterator_next(&iter);
            } while (element != 0);
        }

        fields_ptr = &fields_byte0;
        fields_pad = 0;
        fields_byte0 = (uint8_t)team_index;
        (void)fields_pad;
        encoded_bits = message_delta_encode_message(0, 0x34, 0, (void **)&fields_ptr, 0, 1, 0);
        if (encoded_bits > 0) {
            fields_byte1 = 1;
            if ((network_client->channel->flags & 1) == 0) {
                network_channel_reliable_pool_store(network_client->channel, message_buffer, &fields_byte1, 0);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dbaa0):

/* WARNING: Removing unreachable block (ram,0x004dbaf2) */

void FUN_004dbaa0(void)

{
  int iVar1;
  char cVar2;
  short *psVar3;
  int iVar4;
  int iVar5;
  char local_62 [2];
  char *local_60;
  undefined4 local_5c;
  byte local_58;
  undefined4 local_54;
  uint local_50;
  undefined2 local_4c;
  undefined4 local_48;
  uint local_44;
  undefined1 local_40 [64];

  cVar2 = FUN_004ec590();
  local_50 = DAT_0087a480;
  if (cVar2 == '\x01') {
    cVar2 = -1;
    if (((local_58 != 0xffffffff) && ((short)(ushort)local_58 < *(short *)(DAT_0087a480 + 0x20))) &&
       (psVar3 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)(short)(ushort)local_58 +
                          *(int *)(DAT_0087a480 + 0x34)), *psVar3 != 0)) {
      *(undefined4 *)(psVar3 + 0x6e) = local_54;
    }
    local_44 = local_50 ^ 0x69746572;
    local_4c = 0;
    local_48 = 0xffffffff;
    iVar4 = data_iterator_next();
    if (iVar4 != 0) {
      cVar2 = -1;
      do {
        if (*(short *)(iVar4 + 2) != -1) {
          cVar2 = *(char *)(iVar4 + 0x67);
          if ((cVar2 != -1) && (DAT_00719720 == 2)) {
            iVar1 = *(int *)(DAT_0071c2d4 + 0x9c0);
            iVar5 = FUN_00449210();
            *(int *)(iVar4 + 0xdc) = iVar5 - iVar1;
            return;
          }
          break;
        }
        iVar4 = data_iterator_next();
      } while (iVar4 != 0);
    }
    local_60 = local_62;
    local_5c = 0;
    local_62[0] = cVar2;
    iVar4 = message_delta_encode_message(0,0x34,0,&local_60,0,1,'\0');
    if (0 < iVar4) {
      local_62[1] = 1;
      if ((*(byte *)(*(int *)(DAT_0071c2d8 + 0xadc) + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(*(int *)(DAT_0071c2d8 + 0xadc),local_40,local_62 + 1,0);
      }
    }
  }
  return;
}
#endif
