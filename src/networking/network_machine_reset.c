// network_machine_reset  (Ghidra: FUN_004df690; named per this rewrite)
// address 0x4df690, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/networking.h cites this address directly: "network_machine (0x4dec40 init,
// 0x4df690 reset, ...)". flags |= k_network_machine_pending, timer_14/timer_18/unknown_50, and
// the 0xd-dword (0x34-byte) zero of connect_state[0x34] at +0x1c all match exactly.
// The leading network_channel_remote_address_or_default call takes EAX = machine->channel and
// ECX = &local scratch; its result is unused (verified 0x4df6a8..0x4df6b9).
// register convention: machine in ESI (unaff_ESI). blam-cc: ESI -> machine

// VERIFIED against disassembly 0x4df690..0x4df6e2 (2026-09-30)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module

// blam-cc: ESI -> machine
int32_t network_machine_reset(network_machine *machine)
{
    network_resolved_address sender;

    // FIXED in the review pass: 0x4df6a8 is `mov eax,[esi]` (machine->channel) and 0x4df6b1
    // is `lea ecx,[esp+0x8]` (a local scratch), so the leading call resolves this machine's
    // remote address into a local that the rest of the function ignores.
    network_channel_remote_address_or_default(machine->channel, &sender);
    machine->flags = machine->flags | k_network_machine_pending;
    machine->disconnect_timer_active = 0;
    machine->timer_14 = 0;
    machine->timer_18 = 0;
    machine->player_joined = 0;
    memset(machine->connect_state, 0, sizeof(machine->connect_state));
    return 1;
}

#if 0
Original Ghidra decompilation (0x4df690):

undefined4 FUN_004df690(void)

{
  int iVar1;
  int unaff_ESI;
  undefined4 *puVar2;

  FUN_004dd390();
  *(byte *)(unaff_ESI + 0xe) = *(byte *)(unaff_ESI + 0xe) | 2;
  *(undefined1 *)(unaff_ESI + 0x10) = 0;
  *(undefined4 *)(unaff_ESI + 0x14) = 0;
  *(undefined4 *)(unaff_ESI + 0x18) = 0;
  *(undefined1 *)(unaff_ESI + 0x50) = 0;
  puVar2 = (undefined4 *)(unaff_ESI + 0x1c);
  for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  return 1;
}
#endif
