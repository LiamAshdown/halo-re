// network_join_request_reset_state  (Ghidra: FUN_004e0ab0, unnamed)
// address 0x4e0ab0, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Calls two parameterless helper routines,
// likely resetting some channel/session state as part of the join-request handshake."
// register convention: none visible.
// FIXED (register inputs, objdump): EAX carries a network_machine * (read at 0x4e0ab4,
//   `mov esi,eax`), matching the exact machine->channel / machine->unknown_52 / machine->unknown_5c
//   pattern already established by network_machine_reset.c (0x4df690) and
//   network_session_host_reject_or_cleanup_client.c (0x575ff0, ESI -> channel, initialized to
//   -1 == network_machine::unknown_5c). The rewrite previously called both helpers with no
//   arguments at all. A stack value read at 0x4e0adb ([esp+0x20], this function's first stack
//   argument) and machine->unknown_52's address are also loaded before the second call, but
//   network_session_host_reject_or_cleanup_client's own established signature takes only the
//   channel value, so those two are left unmodeled (UNSURE, may reflect an incomplete
//   convention on that callee, which is outside this file). Likewise the loopback-address
//   substitution at 0x4e0ac9..0x4e0ad5 only feeds EDX, which neither call consumes under the
//   currently established signatures, so it has no modeled effect here.
//   // blam-cc: EAX -> machine

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t network_session_host_reject_or_cleanup_client(int32_t channel); // 0x575ff0, this module; blam-cc: ESI -> channel

// blam-cc: EAX -> machine
void network_join_request_reset_state(network_machine *machine)
{
    network_resolved_address address;

    network_channel_remote_address_or_default(machine ? machine->channel : 0, &address);
    // 0x4e0adf..0x4e0ae2: unconditional, not guarded by the machine-null check above.
    network_session_host_reject_or_cleanup_client(machine->unknown_5c);
}

#if 0
Original Ghidra decompilation (0x4e0ab0):

void FUN_004e0ab0(void)

{
  FUN_004dd390();
  FUN_00575ff0();
  return;
}
#endif
