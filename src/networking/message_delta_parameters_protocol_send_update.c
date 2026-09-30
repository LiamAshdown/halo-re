// message_delta_parameters_protocol_send_update  (Ghidra: message_delta_parameters_protocol_send_update, already named)
// address 0x4ebf50, size 172 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary; calls message_delta_encode_message with
// message type 0x22, which network dispatch (0x4da320, out of this module's range) routes to
// message_delta_parameters_protocol_receive_update.
// register convention: no register-passed arguments.
// UNSURE: network_session_broadcast_to_all's exact parameter meaning (it is outside this module's rewritten range);
// kept as a plain int32_t forward per its call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t message_delta_parameters_enabled;   // 0x0071cfa8
extern uint8_t message_delta_parameters_sending;   // 0x0071cfb4, UNSURE: reentrancy/in-progress flag
extern int32_t message_delta_parameters_protocol_sequence; // 0x0071cfac, rolling 0..3 sequence number
extern uint8_t message_delta_parameters_protocol_broadcast_target[]; // 0x00871de0, UNSURE: passed straight to network_session_broadcast_to_all


extern void message_delta_parameters_protocol_pack_values(void);              // 0x4ec1a0, this module
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_all(int32_t a1, void *a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6); // 0x4e19c0, other batch (UNSURE args)

// Builds and broadcasts a message-delta message (type 0x22) carrying the current dynamic
// parameter values, advancing a rolling 0..3 sequence number once the broadcast succeeds.
void message_delta_parameters_protocol_send_update(void)
{
    uint32_t next_sequence;
    int32_t encoded_bits;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_sending = 1;
        next_sequence = (message_delta_parameters_protocol_sequence + 1) & 0x80000003;
        if ((int32_t)next_sequence < 0) {
            next_sequence = (next_sequence - 1 | 0xfffffffc) + 1;
        }
        message_delta_parameters_protocol_format_registered_values();
        message_delta_parameters_protocol_pack_values();
        {
            uint8_t local_104[260];
            uint8_t *local_10c;
            int32_t local_108;

            local_104[0] = (uint8_t)next_sequence;
            local_10c = local_104;
            local_108 = 0;
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x22, 0, (void **)&local_10c, 0, 1, '\0');
            if (0 < encoded_bits) {
                if (network_session_broadcast_to_all(1, message_delta_parameters_protocol_broadcast_target, 1, 0, 1, 3) != '\0') {
                    message_delta_parameters_protocol_sequence = next_sequence;
                }
            }
        }
        message_delta_parameters_sending = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ebf50):

void message_delta_parameters_protocol_send_update(void)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  undefined1 *local_10c;
  undefined4 local_108;
  undefined1 local_104 [260];

  if (DAT_0071cfa8 == '\x01') {
    DAT_0071cfb4 = 1;
    uVar2 = DAT_0071cfac + 1 & 0x80000003;
    if ((int)uVar2 < 0) {
      uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
    }
    local_104[0] = (undefined1)uVar2;
    message_delta_parameters_protocol_format_registered_values();
    message_delta_parameters_protocol_pack_values();
    local_10c = local_104;
    local_108 = 0;
    iVar3 = message_delta_encode_message(0,0x22,0,&local_10c,0,1,'\0');
    if (0 < iVar3) {
      cVar1 = FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
      if (cVar1 != '\0') {
        DAT_0071cfac = uVar2;
      }
    }
    DAT_0071cfb4 = 0;
  }
  return;
}
#endif
