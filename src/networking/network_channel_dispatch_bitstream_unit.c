// network_channel_dispatch_bitstream_unit  (Ghidra: FUN_004e18b0, unnamed)
// address 0x4e18b0, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Small dispatcher used while draining a
// channel's bitstream, routing each unit to either the queued-message processor or the
// incoming-packet decoder." network_game_process_incoming_message is already named and in
// this batch.
// register convention: both parameters are genuine stack (cdecl) parameters.
// blam-cc: stack -> server, unit

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char FUN_004de420(int32_t timeout_ms); // other module (UNSURE)
extern uint32_t network_game_process_incoming_message(network_server_globals *server); // 0x4e1c60, this batch (UNSURE args)
extern uint32_t network_client_drain_queued_updates(network_server_globals *server); // 0x4e1f40, this batch (UNSURE args)

// unit == 1 drains the queued-update processor; unit == 0 checks a readiness gate
// (FUN_004de420) and, if ready, decodes one incoming network-game message. Any other value (or
// an unready gate) returns 0.
uint32_t network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit)
{
    if (unit == 1) {
        return network_client_drain_queued_updates(server);
    }
    if (unit == 0) {
        if (FUN_004de420(0xfff) != 0) {
            return network_game_process_incoming_message(server);
        }
    }
    return unit & 0xffffff00;
}

#if 0
Original Ghidra decompilation (0x4e18b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004e18b0(undefined4 param_1,uint param_2)

{
  uint uVar1;
  int iVar2;

  if (param_2 == 1) {
    uVar1 = FUN_004e1f40(param_1);
    return uVar1;
  }
  if (param_2 == 0) {
    iVar2 = FUN_004de420(0xfff);
    param_2 = 0;
    if (iVar2 != 0) {
      uVar1 = network_game_process_incoming_message(param_1);
      return uVar1;
    }
  }
  return param_2 & 0xffffff00;
}
#endif
