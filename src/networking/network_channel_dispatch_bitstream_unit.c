// network_channel_dispatch_bitstream_unit  (Ghidra: FUN_004e18b0, unnamed)
// address 0x4e18b0, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Small dispatcher used while draining a
// channel's bitstream, routing each unit to either the queued-message processor or the
// incoming-packet decoder." network_game_process_incoming_message is already named and in
// this batch.
// register convention: EAX read at entry is the single genuine stack argument, `unit`, cached
// into EBP (survives the calls); `machine` (network_machine*) is a live-in forwarded through ESI
// -- never set locally -- straight into network_game_process_incoming_message's own verified
// ECX -> machine slot at its call site (0x4e190b `mov ecx,esi`).
// blam-cc: ESI -> machine, stack -> unit
// FIXED (register inputs, objdump): ESI is a genuine live-in (0x4e18ce `push esi`,
// 0x4e190b `mov ecx,esi`) that the notes did not map; added as `machine`. While tracing that
// call, also found the previously-modeled `server` stack parameter does not exist in the binary
// -- only one stack slot (`unit`) is ever read (0x4e18ba/0x4e18c5) -- and that this function
// discarded FUN_004de420's return value instead of forwarding it as `record`/`length` the way
// network_game_process_incoming_message's own verified convention requires; both are fixed below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern uint16_t *FUN_004de420(int32_t timeout_ms); // other module (UNSURE); returns a record pointer, not char
extern uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine,
    uint16_t *record, network_server_globals *server); // 0x4e1c60, this batch; blam-cc: EAX -> length, ECX -> machine, EDX -> record, stack -> server
extern uint32_t network_client_drain_queued_updates(network_machine *machine,
    network_server_globals *param_1, void *param_2); // 0x4e1f40, this batch; blam-cc: EBX -> machine, stack -> param_1, param_2

// unit == 1 drains the queued-update processor; unit == 0 checks a readiness gate
// (FUN_004de420) and, if ready, decodes one incoming network-game message. Any other value (or
// an unready gate) returns 0.
uint32_t network_channel_dispatch_bitstream_unit(network_machine *machine, uint32_t unit)
{
    if (unit == 1) {
        // UNSURE: network_client_drain_queued_updates also needs EBX -> machine, which this
        // function has no live input for (not flagged by the checker); left unset.
        return network_client_drain_queued_updates(0, (network_server_globals *)(uintptr_t)unit,
            machine);
    }
    if (unit == 0) {
        uint16_t *record = FUN_004de420(0xfff);
        if (record != 0) {
            int32_t length = (int32_t)(uint16_t)(*record >> 4);
            return network_game_process_incoming_message(length, machine, record,
                (network_server_globals *)(uintptr_t)unit);
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
