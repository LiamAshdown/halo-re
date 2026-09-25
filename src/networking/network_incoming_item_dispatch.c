// network_incoming_item_dispatch  (Ghidra: FUN_004db630; renamed, no prior name)
// address 0x4db630, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Dispatches a single incoming queue
// entry either to the queued-game-action applier or to the network-message decode switch,
// depending on an entry-type flag").
// register convention: Ghidra's own recovered stack parameters (client, item_flag); ESI is also
// a genuine live-in, pushed as network_game_action_queue_drain's own `expected_sequence` stack
// argument (objdump 0x4db652 `push esi`, read with no local setup).
// // blam-cc: stack -> client, item_flag, ESI -> expected_sequence
// FIXED (register inputs, objdump): ESI is a genuine live-in the notes did not map; added as
// `expected_sequence` and forwarded instead of the hardcoded pointer-to-zero local. While
// tracing that call, also found network_game_message_decode_dispatch's own file
// (src/networking/network_game_message_decode_dispatch.c) already recovered its real signature
// as (uint16_t *record /*EDX*/, int32_t record_length /*EDI*/) -- not `(client)` as this file's
// stale extern claimed -- and this function was discarding network_message_read_sized_buffer's
// return value (the record pointer) instead of forwarding it; both fixed below to match
// objdump 0x4db680-0x4db68b (movzx edi,[eax]; mov edx,eax; shr edi,4).
// UNSURE: the real return value on the `param_2 == 0` failure path is `param_2 & 0xffffff00`
// (always 0, since param_2 is already 0 there); modeled directly as 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_game_action_queue_drain(network_client_globals *client, int32_t bound,
    const int32_t *expected_sequence); // 0x4db870, this batch
    // UNSURE: `bound` reconstructed as 0; not recoverable at this call site (unflagged register).
extern uint16_t *network_message_read_sized_buffer(int32_t unknown); // 0x4de420, not in this batch; returns a record pointer
extern char network_game_message_decode_dispatch(uint16_t *record, int32_t record_length); // 0x4db6b0, this batch; blam-cc: EDX -> record, EDI -> record_length

// blam-cc: stack -> client, item_flag, ESI -> expected_sequence
int32_t network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag,
    const int32_t *expected_sequence)
{
    if (item_flag == 1) {
        return network_game_action_queue_drain(client, 0, expected_sequence);
    }
    if (item_flag == 0) {
        uint16_t *record = network_message_read_sized_buffer(0xfff);
        if (record != 0) {
            int32_t record_length = (int32_t)(uint16_t)(*record >> 4);
            return network_game_message_decode_dispatch(record, record_length);
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
