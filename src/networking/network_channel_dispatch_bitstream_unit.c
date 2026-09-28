// network_channel_dispatch_bitstream_unit  (Ghidra: FUN_004e18b0, unnamed)
// address 0x4e18b0, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.45)
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
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e18b0..0x4e1927): besides (server, unit)
// on the stack it takes the item's stream in ECX and the machine in ESI. A game-action item (1) drains through
// network_client_drain_queued_updates (ECX stream; stack server, machine); a message item (0) is read into a local
// 0x1000-byte buffer (EDI buffer, EBX stream, capacity 0xfff) and handed to network_game_process_incoming_message
// (EAX length = first word >> 4, ECX machine, EDX record, stack server). The previous C passed the unit flag as the
// server and no stream.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream); // 0x4de420, EDI, stack, EBX
extern uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine,
    uint16_t *record, network_server_globals *server); // 0x4e1c60, EAX, ECX, EDX, stack
extern char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine,
    bit_stream *stream); // 0x4e1f40, stack, stack, ECX

char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream,
    network_machine *machine)
{
    uint16_t buffer[0x800];

    if (unit == 1) {
        return network_client_drain_queued_updates(server, machine, stream);
    }
    if (unit == 0) {
        uint16_t *record = network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return (char)network_game_process_incoming_message(*record >> 4, machine, record, server);
        }
    }
    return 0;
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
