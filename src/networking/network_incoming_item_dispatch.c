// network_incoming_item_dispatch  (Ghidra: FUN_004db630; renamed, no prior name)
// address 0x4db630, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.4)
// evidence: out/phase4/networking_functions.md summary ("Dispatches a single incoming queue
// entry either to the queued-game-action applier or to the network-message decode switch,
// depending on an entry-type flag").
// register convention: Ghidra's own recovered stack parameters (client, item_flag); ESI is also
// a genuine live-in, pushed as network_game_action_queue_drain's own `expected_sequence` stack
// argument (objdump 0x4db652 `push esi`, read with no local setup).
// blam-cc: ECX -> stream, ESI -> sender, stack -> client, item_flag
// FIXED (register inputs, objdump): ESI is a genuine live-in the notes did not map; added as
// `expected_sequence` and forwarded instead of the hardcoded pointer-to-zero local. While
// tracing that call, also found network_game_message_decode_dispatch's own file
// (src/networking/network_game_message_decode_dispatch.c) already recovered its real signature
// as (uint16_t *record /*EDX*/, int32_t record_length /*EDI*/) -- not `(client)` as this file's
// stale extern claimed -- and this function was discarding network_message_read_sized_buffer's
// return value (the record pointer) instead of forwarding it; both fixed below to match
// objdump 0x4db680-0x4db68b (movzx edi,[eax]; mov edx,eax; shr edi,4).
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db630..0x4db6a9): the item's stream
// arrives in ECX and the sender address in ESI. A game-action item (flag 1) goes to
// network_game_action_queue_drain(client, stream, sender); a message item (flag 0) is read into a local 0x1000-byte
// buffer (network_message_read_sized_buffer: EDI buffer, EBX stream, capacity 0xfff) and handed to
// network_game_message_decode_dispatch with the client (EAX), the record (EDX), its length (the first word >> 4,
// EDI) and the sender (stack).

// VERIFIED against disassembly 0x4db630..0x4db6a9 (2026-09-30): flag 1 -> drain(client, stream, sender); flag 0 -> read_sized_buffer(EDI buffer, EBX/ECX stream, 0xfff) then decode_dispatch(EAX client, EDX record, EDI length, stack sender); else 0
#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream,
    const uint32_t *sender); // 0x4db870, stack
extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream); // 0x4de420, EDI, stack, EBX
extern char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record,
    int32_t record_length, const uint32_t *sender); // 0x4db6b0, EAX, EDX, EDI, stack

char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream,
    const uint32_t *sender)
{
    uint16_t buffer[0x800];

    if (item_flag == 1) {
        return network_game_action_queue_drain(client, stream, sender);
    }
    if (item_flag == 0) {
        uint16_t *record = network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return network_game_message_decode_dispatch(client, record, *record >> 4, sender);
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4db630):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004db630(undefined4 param_1,uint param_2)

{
  uint uVar1;
  int iVar2;

  if (param_2 == 1) {
    uVar1 = FUN_004db870(param_1);
    return uVar1;
  }
  if (param_2 == 0) {
    iVar2 = FUN_004de420(0xfff);
    param_2 = 0;
    if (iVar2 != 0) {
      uVar1 = network_game_message_decode_dispatch();
      return uVar1;
    }
  }
  return param_2 & 0xffffff00;
}
#endif
