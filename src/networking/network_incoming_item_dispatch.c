// network_incoming_item_dispatch  (Ghidra: FUN_004db630; renamed, no prior name)
// address 0x4db630, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Dispatches a single incoming queue
// entry either to the queued-game-action applier or to the network-message decode switch,
// depending on an entry-type flag").
// register convention: Ghidra's own recovered stack parameters (client, item_flag).
// // blam-cc: stack -> client, item_flag
// UNSURE: `network_message_read_sized_buffer` (0x4de420, not in this batch) is called with a literal argument, no
// reconstruction needed. `network_game_message_decode_dispatch` is called with no visible
// argument; reconstructed as taking `client`, matching every sibling dispatcher in this cluster.
// UNSURE: the real return value on the `param_2 == 0` failure path is `param_2 & 0xffffff00`
// (always 0, since param_2 is already 0 there); modeled directly as 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_game_action_queue_drain(network_client_globals *client, int32_t bound,
    const int32_t *expected_sequence); // 0x4db870, this batch
    // UNSURE: called here with no visible arguments; bound/expected_sequence reconstructed as 0
    // and a pointer to a zeroed local, since neither value is recoverable at this call site.
extern int32_t network_message_read_sized_buffer(int32_t unknown); // 0x4de420, not in this batch
extern char network_game_message_decode_dispatch(network_client_globals *client); // 0x4db6b0, this batch

// blam-cc: stack -> client, item_flag
int32_t network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag)
{
    if (item_flag == 1) {
        int32_t placeholder_expected_sequence = 0;
        return network_game_action_queue_drain(client, 0, &placeholder_expected_sequence); // UNSURE argument
    }
    if (item_flag == 0) {
        if (network_message_read_sized_buffer(0xfff) != 0) {
            return network_game_message_decode_dispatch(client); // UNSURE argument
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
